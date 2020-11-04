//------------------------------------------------------------------------------
// 11_Terrain.cpp
//------------------------------------------------------------------------------
// Copyright (c) 2020 John Hughes
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files( the "Software" ), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//------------------------------------------------------------------------------
// Headers
//------------------------------------------------------------------------------
#include "aeBinaryStream.h"
#include "aeClock.h"
#include "aeEditorCamera.h"
#include "aeInput.h"
#include "aeLog.h"
#include "aeMeta.h"
#include "aeRender.h"
#include "aeSpline.h"
#include "aeTerrain.h"
#include "aeWindow.h"
#include "aeVfs.h"

#include "aeImGui.h"
#include "ImGuizmo.h"

const char* kFileName = "objects.dat";
const uint32_t kFileVersion = 1;

//------------------------------------------------------------------------------
// Terrain Shader
//------------------------------------------------------------------------------
const char* kTerrainVertShader = "\
  AE_UNIFORM mat4 u_worldToProj;\
  AE_IN_HIGHP vec3 a_position;\
  AE_IN_HIGHP vec3 a_normal;\
  AE_IN_HIGHP vec4 a_materials;\
  AE_OUT_HIGHP vec3 v_normal;\
  AE_OUT_HIGHP vec4 v_materials;\
  void main()\
  {\
    v_normal = a_normal;\
    v_materials = a_materials;\
    gl_Position = u_worldToProj * vec4( a_position, 1.0 );\
  }";

const char* kTerrainFragShader = "\
  AE_UNIFORM vec4 u_topColor;\
  AE_UNIFORM vec4 u_sideColor;\
  AE_UNIFORM vec4 u_pathColor;\
  AE_IN_HIGHP vec3 v_normal;\
  AE_IN_HIGHP vec4 v_materials;\
  void main()\
  {\
    float top = max( 0.0, v_normal.z );\
    top *= top;\
    top *= top;\
    vec4 color = mix( u_sideColor, u_topColor, top ) * v_materials.r;\
    color += u_pathColor * v_materials.g;\
    \
    float light = dot( normalize( v_normal ), normalize( vec3( 1.0 ) ) );\
    light = max( 0.0, light );\
    light = mix( 0.8, 4.0, light );\
    \
    AE_COLOR = vec4( AE_RGB_TO_SRGB( color.rgb * vec3( light ) ), color.a );\
  }";

//------------------------------------------------------------------------------
// Grid Shader
//------------------------------------------------------------------------------
const char* kGridVertexStr = "\
    AE_UNIFORM_HIGHP mat4 u_screenToWorld;\
    AE_IN_HIGHP vec4 a_position;\
    AE_OUT_HIGHP vec3 v_worldPos;\
    void main()\
    {\
      v_worldPos = vec3( u_screenToWorld * a_position );\
      gl_Position = a_position;\
    }";

const char* kGridFragStr = "\
    AE_IN_HIGHP vec3 v_worldPos;\
    void main()\
    {\
      int x = int( floor( v_worldPos.x ) ) % 2;\
      int y = int( floor( v_worldPos.y ) ) % 2;\
      AE_COLOR.rgb = mix( vec3( 0.3 ), vec3( 0.35 ), int( x != y ) );\
      float gridX = mod( v_worldPos.x + 16.0, 32.0 ) - 16.0;\
      float gridY = mod( v_worldPos.y + 16.0, 32.0 ) - 16.0;\
      if ( abs( gridX ) < 0.05 || abs( gridY ) < 0.05 ) { AE_COLOR.rgb = vec3( 0.25 ); } \
      AE_COLOR.a = 1.0;\
    }";

class Grid
{
public:
  void Initialize()
  {
    struct BGVertex
    {
      aeFloat4 pos;
    };
    
    BGVertex bgVertices[ aeQuadVertCount ];
    uint8_t bgIndices[ aeQuadIndexCount ];
    for ( uint32_t i = 0; i < aeQuadVertCount; i++ )
    {
      bgVertices[ i ].pos = aeFloat4( aeQuadVertPos[ i ] * 2.0f, 1.0f );
    }
    for ( uint32_t i = 0; i < aeQuadIndexCount; i++ )
    {
      bgIndices[ i ] = aeQuadIndices[ i ];
    }

    m_bgVertexData.Initialize( sizeof( BGVertex ), sizeof( uint8_t ), countof( bgVertices ), countof( bgIndices ), aeVertexPrimitive::Triangle, aeVertexUsage::Static, aeVertexUsage::Static );
    m_bgVertexData.AddAttribute( "a_position", 4, aeVertexDataType::Float, offsetof( BGVertex, pos ) );
    m_bgVertexData.SetVertices( bgVertices, countof( bgVertices ) );
    m_bgVertexData.SetIndices( bgIndices, countof( bgIndices ) );

    m_gridShader.Initialize( kGridVertexStr, kGridFragStr, nullptr, 0 );
  }

  void Render( aeFloat4x4 worldToProj )
  {
    aeUniformList uniforms;
    uniforms.Set( "u_screenToWorld", worldToProj.Inverse() );
    m_bgVertexData.Render( &m_gridShader, uniforms );
  }

private:
  aeShader m_gridShader;
  aeVertexData m_bgVertexData;
};

//------------------------------------------------------------------------------
// Objects
//------------------------------------------------------------------------------
struct Object
{
  Object() = default;
  Object( const char* n, ae::Sdf::Shape* s ) : name( n ), shape( s ) {}
  void Serialize( aeBinaryStream* stream );
  
  aeStr16 name;
  ae::Sdf::Shape* shape;
};

void Object::Serialize( aeBinaryStream* stream )
{
  stream->SerializeString( name );
  
  if ( stream->IsWriter() )
  {
    stream->SerializeRaw( shape->GetTransform() );
  }
  else
  {
    aeFloat4x4 transform;
    stream->SerializeRaw( transform );
    shape->SetTransform( transform );
  }
}

void WriteObjects( aeVfs* vfs, const aeArray< Object* >& objects )
{
  aeBinaryStream wStream = aeBinaryStream::Writer();
  wStream.SerializeUint32( kFileVersion );

  wStream.SerializeUint32( objects.Length() );
  for ( uint32_t i = 0; i < objects.Length(); i++ )
  {
    Object* object = objects[ i ];
    
    aeStr16 type = "";
    if ( aeCast< ae::Sdf::Box >( object->shape ) ) { type = "box"; }
    else if ( aeCast< ae::Sdf::Cylinder >( object->shape ) ) { type = "cylinder"; }
    else if ( aeCast< ae::Sdf::Heightmap >( object->shape ) ) { type = "heightmap"; }
    else { AE_FAIL(); }
    wStream.SerializeString( type );
    
    wStream.SerializeObject( *object );
  }
  vfs->Write( aeVfs::User, kFileName, wStream.GetData(), wStream.GetOffset() );
}

bool ReadObjects( aeVfs* vfs, aeTerrain* terrain, ae::Image* heightmapImage, aeArray< Object* >& objects )
{
  aeAlloc::Scratch< uint8_t > scratch( vfs->GetSize( aeVfs::User, kFileName ) );
  vfs->Read( aeVfs::User, kFileName, scratch.Data(), scratch.Length() );
  aeBinaryStream rStream = aeBinaryStream::Reader( scratch.Data(), scratch.Length() );

  uint32_t version = 0;
  rStream.SerializeUint32( version );
  if ( version != kFileVersion )
  {
    return false;
  }
  objects.Clear();
  
  uint32_t len = 0;
  rStream.SerializeUint32( len );
  if ( !len )
  {
    return false;
  }
  
  for ( uint32_t i = 0; i < len; i++ )
  {
    Object* object = aeAlloc::Allocate< Object >();
    
    aeStr16 type = "";
    rStream.SerializeString( type );
    if ( type == "box" ) { object->shape = terrain->sdf.CreateSdf< ae::Sdf::Box >(); }
    else if ( type == "cylinder" ) { object->shape = terrain->sdf.CreateSdf< ae::Sdf::Cylinder >(); }
    else if ( type == "heightmap" )
    {
      ae::Sdf::Heightmap* heightmap = terrain->sdf.CreateSdf< ae::Sdf::Heightmap >();
      heightmap->SetImage( heightmapImage );
      object->shape = heightmap;
    }
    else { AE_FAIL(); }
    
    rStream.SerializeObject( *object );
    objects.Append( object );
  }
  
  return true;
}

//------------------------------------------------------------------------------
// Main
//------------------------------------------------------------------------------
int main()
{
  AE_INFO( "Initialize" );

  bool headless = _AE_LINUX_;

  aeVfs vfs;
  aeWindow window;
  aeRender render;
  aeInput input;
  aeDebugRender debug;
  aeFixedTimeStep timeStep;
  aeShader terrainShader;
  aeEditorCamera camera;
  aeTextRender textRender;
  class aeImGui* ui = nullptr;

  vfs.Initialize( "", "ae", "terrainedit" );
  ui = aeAlloc::Allocate< aeImGui >();
  if ( headless )
  {
    ui->InitializeHeadless();
  }
  else
  {
    window.Initialize( 800, 600, false, true );
    window.SetTitle( "terrain edit" );
    render.InitializeOpenGL( &window );
    debug.Initialize();
    ui->Initialize();
  }

  input.Initialize( headless ? nullptr : &window );
  timeStep.SetTimeStep( 1.0f / 60.0f );
  camera.SetFocusDistance( 100.0f );
  
  if ( !headless )
  {
    terrainShader.Initialize( kTerrainVertShader, kTerrainFragShader, nullptr, 0 );
    terrainShader.SetDepthTest( true );
    terrainShader.SetDepthWrite( true );

    textRender.Initialize( "font.png", aeTextureFilter::Nearest, 8 );
  }

  ae::Image heightmapImage;
  aeAlloc::Scratch< uint8_t > fileBuffer( vfs.GetSize( aeVfs::Data, "terrain.png" ) );
  vfs.Read( aeVfs::Data, "terrain.png", fileBuffer.Data(), fileBuffer.Length() );
  heightmapImage.LoadFile( fileBuffer.Data(), fileBuffer.Length(), ae::Image::Extension::PNG, ae::Image::Format::R );

  uint32_t terrainThreads = aeMath::Max( 1u, (uint32_t)( aeGetMaxConcurrentThreads() * 0.75f ) );
  aeTerrain* terrain = aeAlloc::Allocate< aeTerrain >();
  terrain->Initialize( terrainThreads, !headless );

  aeFloat4x4 worldToText = aeFloat4x4::Identity();
  auto drawWorldText = [&]( aeFloat3 p, const char* str )
  {
    p = aeFloat3::ProjectPoint( worldToText, p );
    aeFloat2 fontSize( (float)textRender.GetFontSize() );
    textRender.Add( p, fontSize, str, aeColor::White(), 0, 0 );
  };

  bool wireframe = false;
  static bool s_showTerrainDebug = false;

  aeArray< Object* > objects;
  Object* currentObject = nullptr;

  if ( !ReadObjects( &vfs, terrain, &heightmapImage, objects ) )
  {
    ae::Sdf::Box* box = terrain->sdf.CreateSdf< ae::Sdf::Box >();
    box->SetTransform( aeFloat4x4::Translation( camera.GetFocus() ) * aeFloat4x4::Scaling( aeFloat3( 10.0f ) ) );
    currentObject = objects.Append( aeAlloc::Allocate< Object >( "Box", box ) );
  }

  bool gizmoClickedPrev = false;

  AE_INFO( "Run" );
  while ( !input.GetState()->exit )
  {
    input.Pump();

    ui->NewFrame( &render, &input, timeStep.GetTimeStep() );

    ImGuizmo::SetOrthographic( false );
    ImGuizmo::BeginFrame();

    if ( ImGui::Begin( "edit", nullptr ) )
    {
      // New terrain objects
      if ( ImGui::CollapsingHeader( "create" ) )
      {
        if ( ImGui::Button( "box" ) )
        {
          ae::Sdf::Box* box = terrain->sdf.CreateSdf< ae::Sdf::Box >();
          box->SetTransform(
            aeFloat4x4::Translation( camera.GetFocus() ) *
            aeFloat4x4::Scaling( aeFloat3( 10.0f ) ) );

          currentObject = objects.Append( aeAlloc::Allocate< Object >( "Box", box ) );
        }

        if ( ImGui::Button( "cylinder" ) )
        {
          ae::Sdf::Cylinder* cylinder = terrain->sdf.CreateSdf< ae::Sdf::Cylinder >();
          cylinder->SetTransform(
            aeFloat4x4::Translation( camera.GetFocus() ) *
            aeFloat4x4::Scaling( aeFloat3( 10.0f ) ) );

          currentObject = objects.Append( aeAlloc::Allocate< Object >( "Cylinder", cylinder ) );
        }
        
        if ( ImGui::Button( "height map" ) )
        {
          ae::Sdf::Heightmap* heightMap = terrain->sdf.CreateSdf< ae::Sdf::Heightmap >();
          heightMap->SetTransform(
            aeFloat4x4::Translation( camera.GetFocus() ) *
            aeFloat4x4::Scaling( aeFloat3( 10.0f ) ) );
          heightMap->SetImage( &heightmapImage );

          currentObject = objects.Append( aeAlloc::Allocate< Object >( "Height Map", heightMap ) );
        }

        // @TODO: Disabled because "material" is used in properties, and having both creates a conflict
        //if ( ImGui::Button( "material" ) )
        //{
        //  ae::Sdf::Box* box = terrain->sdf.CreateSdf< ae::Sdf::Box >();
        //  box->SetTransform(
        //    aeFloat4x4::Translation( camera.GetFocus() ) *
        //    aeFloat4x4::Scaling( aeFloat3( 10.0f ) ) );
        //  box->type = ae::Sdf::Shape::Type::Material;
        //  box->materialId = 1;

        //  currentObject = objects.Append( aeAlloc::Allocate< Object >( "Material", box ) );
        //}
      }
      
      if ( ImGui::CollapsingHeader( "properties" ) )
      {
        if ( currentObject )
        {
          ae::Sdf::Shape* currentShape = currentObject->shape;

          aeImGui::InputText( "name", &currentObject->name );

          bool changed = false;

          aeFloat4x4 temp = currentShape->GetTransform().GetTransposeCopy();
          float matrixTranslation[ 3 ], matrixRotation[ 3 ], matrixScale[ 3 ];
          ImGuizmo::DecomposeMatrixToComponents( temp.data, matrixTranslation, matrixRotation, matrixScale );
          changed |= ImGui::InputFloat3( "Translation", matrixTranslation, 3 );
          changed |= ImGui::InputFloat3( "Rotation", matrixRotation, 3 );
          changed |= ImGui::InputFloat3( "Scale", matrixScale, 3 );
          if ( changed )
          {
            ImGuizmo::RecomposeMatrixFromComponents( matrixTranslation, matrixRotation, matrixScale, temp.data );
            temp.SetTranspose();
            currentShape->SetTransform( temp );
          }

          const char* types[] = { "union", "subtraction", "smooth union", "smooth subtraction", "material" };
          changed |= ImGui::Combo( "type", (int*)&currentShape->type, types, countof( types ) );

          if ( currentShape->type == ae::Sdf::Shape::Type::SmoothUnion || currentShape->type == ae::Sdf::Shape::Type::SmoothSubtraction )
          {
            aeFloat3 halfSize = currentShape->GetHalfSize();
            float maxLength = aeMath::Max( halfSize.x, halfSize.y, halfSize.z );
            changed |= ImGui::SliderFloat( "smoothing", &currentShape->smoothing, 0.0f, maxLength );
          }

          const char* materialNames[] = { "grass", "sand" };
          changed |= ImGui::Combo( "material", (int32_t*)&currentShape->materialId, materialNames, countof( materialNames ) );

          if ( auto box = aeCast< ae::Sdf::Box >( currentShape ) )
          {
            aeFloat3 halfSize = box->GetHalfSize();
            float minLength = aeMath::Min( halfSize.x, halfSize.y, halfSize.z );
            changed |= ImGui::SliderFloat( "cornerRadius", &box->cornerRadius, 0.0f, minLength );
          }
          else if ( auto cylinder = aeCast< ae::Sdf::Cylinder >( currentShape ) )
          {
            changed |= ImGui::SliderFloat( "top", &cylinder->top, 0.0f, 1.0f );
            changed |= ImGui::SliderFloat( "bottom", &cylinder->bottom, 0.0f, 1.0f );
          }

          if ( changed )
          {
            currentShape->Dirty();
          }
        }
        else
        {
          ImGui::Text( "no selection" );
        }
      }

      if ( ImGui::CollapsingHeader( "objects" ) )
      {
        ImGui::BeginChild( "ChildL" );
        for ( uint32_t i = 0; i < objects.Length(); i++ )
        {
          Object* object = objects[ i ];
          aeStr32 displayName( "(#) #", i, object->name );
          if ( ImGui::Selectable( displayName.c_str(), object == currentObject ) )
          {
            currentObject = object;
          }
        }
        ImGui::EndChild();
      }
    }
    ImGui::End();

    if ( !ImGuizmo::IsUsing() )
    {
      if ( s_showTerrainDebug )
      {
        terrain->SetDebugTextCallback( drawWorldText );
      }
      else
      {
        terrain->SetDebugTextCallback( nullptr );
      }

      // Wait for click release of gizmo before starting new terrain jobs
      terrain->Update( camera.GetFocus(), 1250.0f );
    }

    // Camera input
    camera.SetInputEnabled( !ImGui::GetIO().WantCaptureMouse && !ImGuizmo::IsUsing() );
    camera.Update( &input, timeStep.GetTimeStep() );

    // Camera focus
    if ( currentObject && !input.GetPrevState()->Get( aeKey::F ) && input.GetState()->Get( aeKey::F ) )
    {
      camera.Refocus( currentObject->shape->GetAABB().GetCenter() );
    }

    // Render mode
    if ( !input.GetPrevState()->Get( aeKey::Num1 ) && input.GetState()->Get( aeKey::Num1 ) )
    {
      wireframe = true;
      s_showTerrainDebug = true;
    }
    else if ( !input.GetPrevState()->Get( aeKey::Num2 ) && input.GetState()->Get( aeKey::Num2 ) )
    {
      wireframe = false;
      s_showTerrainDebug = true;
    }
    else if ( input.GetState()->Get( aeKey::Num3 ) && !input.GetPrevState()->Get( aeKey::Num3 ) )
    {
      wireframe = false;
      s_showTerrainDebug = false;
    }

    if ( !headless )
    {
      static aeFloat3 rayPos, rayDir;
      if ( input.GetState()->Get( aeKey::R ) )
      {
        rayPos = camera.GetPosition();
        rayDir = camera.GetForward() * 200.0f;
      }
      RaycastResult result = terrain->Raycast( rayPos, rayDir );
      if ( result.hit )
      {
        debug.AddCircle( result.posf, result.normal, 0.5f, aeColor::Green(), 16 );
      }

      render.Activate();
      render.Clear( aeColor::PicoDarkPurple() );

      aeFloat4x4 worldToView = aeFloat4x4::WorldToView( camera.GetPosition(), camera.GetForward(), aeFloat3( 0.0f, 0.0f, 1.0f ) );
      aeFloat4x4 viewToProj = aeFloat4x4::ViewToProjection( 0.525f, render.GetAspectRatio(), 0.5f, 1000.0f );
      aeFloat4x4 worldToProj = viewToProj * worldToView;
      // UI units in pixels, origin in bottom left
      aeFloat4x4 textToNdc = aeFloat4x4::Scaling( aeFloat3( 2.0f / render.GetWidth(), 2.0f / render.GetHeight(), 1.0f ) );
      textToNdc *= aeFloat4x4::Translation( aeFloat3( render.GetWidth() / -2.0f, render.GetHeight() / -2.0f, 0.0f ) );
      worldToText = textToNdc.Inverse() * worldToProj;

      aeColor top = aeColor::PS( 46, 65, 35 );
      aeColor side = aeColor::PS( 84, 84, 74 );
      aeColor path = aeColor::PS( 64, 64, 54 );
      aeUniformList uniformList;
      uniformList.Set( "u_worldToProj", worldToProj );
      if ( wireframe )
      {
        uniformList.Set( "u_topColor", top.GetLinearRGBA() );
        uniformList.Set( "u_sideColor", side.GetLinearRGBA() );
        uniformList.Set( "u_pathColor", path.GetLinearRGBA() );
        terrainShader.SetBlending( false );
        terrainShader.SetCulling( aeShaderCulling::None );
        terrainShader.SetWireframe( true );
        terrain->Render( &terrainShader, uniformList );

        uniformList.Set( "u_topColor", top.SetA( 0.5f ).GetLinearRGBA() );
        uniformList.Set( "u_sideColor", side.SetA( 0.5f ).GetLinearRGBA() );
        uniformList.Set( "u_pathColor", path.SetA( 0.5f ).GetLinearRGBA() );
        terrainShader.SetBlending( true );
        terrainShader.SetCulling( aeShaderCulling::CounterclockwiseFront );
        terrainShader.SetWireframe( false );
        terrain->Render( &terrainShader, uniformList );
      }
      else
      {
        uniformList.Set( "u_topColor", top.GetLinearRGBA() );
        uniformList.Set( "u_sideColor", side.GetLinearRGBA() );
        uniformList.Set( "u_pathColor", path.GetLinearRGBA() );
        terrainShader.SetBlending( false );
        terrainShader.SetCulling( aeShaderCulling::CounterclockwiseFront );
        terrainShader.SetWireframe( false );
        terrain->Render( &terrainShader, uniformList );
      }

      terrain->SetDebug( s_showTerrainDebug ? &debug : nullptr );

      ImGuiIO& io = ImGui::GetIO();
      ImGuizmo::SetRect( 0, 0, io.DisplaySize.x, io.DisplaySize.y );

      aeKey delKey = _AE_OSX_ ? aeKey::Backspace : aeKey::Delete;
      static ImGuizmo::OPERATION s_operation = ImGuizmo::TRANSLATE;
      if ( input.GetState()->Get( aeKey::Q ) && !input.GetPrevState()->Get( aeKey::Q ) )
      {
        currentObject = nullptr;
      }
      else if ( input.GetState()->Get( aeKey::W ) && !input.GetPrevState()->Get( aeKey::W ) )
      {
        s_operation = ImGuizmo::TRANSLATE;
      }
      else if ( input.GetState()->Get( aeKey::E ) && !input.GetPrevState()->Get( aeKey::E ) )
      {
        s_operation = ImGuizmo::ROTATE;
      }
      else if ( input.GetState()->Get( aeKey::R ) && !input.GetPrevState()->Get( aeKey::R ) )
      {
        s_operation = ImGuizmo::SCALE;
      }
      else if ( currentObject && input.GetState()->Get( delKey ) && !input.GetPrevState()->Get( delKey ) )
      {
        objects.Remove( objects.Find( currentObject ) );
        terrain->sdf.DestroySdf( currentObject->shape );
        aeAlloc::Release( currentObject );
        currentObject = nullptr;
      }

      if ( currentObject )
      {
        ae::Sdf::Shape* currentShape = currentObject->shape;

        // Use ImGuizmo::IsUsing() to only update terrain when finished dragging
        bool gizmoClicked = ImGuizmo::IsUsing();
        aeFloat4x4 gizmoTransform = currentShape->GetTransform();

        gizmoTransform.SetTranspose();
        ImGuizmo::Manipulate(
          worldToView.GetTransposeCopy().data,
          viewToProj.GetTransposeCopy().data,
          s_operation,
          ( s_operation == ImGuizmo::SCALE ) ? ImGuizmo::LOCAL : ImGuizmo::WORLD,
          gizmoTransform.data
        );
        gizmoTransform.SetTranspose();

        debug.AddCube( gizmoTransform, aeColor::Green() );
        
        if ( gizmoClicked )
        {
          // Dragging
          currentShape->SetTransform( gizmoTransform );
        }
          
        if ( gizmoClickedPrev && !gizmoClicked )
        {
          // Release
          currentShape->Dirty();
        }

        gizmoClickedPrev = gizmoClicked;
      }

      debug.Render( worldToProj );
      textRender.Render( textToNdc );

      ui->Render();

      render.Present();
    }
    
    timeStep.Wait();
  }
  
  WriteObjects( &vfs, objects );

  AE_INFO( "Terminate" );
  ui->Terminate();
  terrain->Terminate();
  aeAlloc::Release( terrain );
  input.Terminate();
  if ( !headless )
  {
    render.Terminate();
    window.Terminate();
  }
  
  return 0;
}
