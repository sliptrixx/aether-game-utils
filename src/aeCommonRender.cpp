//------------------------------------------------------------------------------
// aeRenderCommon.cpp
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
#include "aeCommonRender.h"
#include "aeRender.h"
#include "aeWindow.h"

//------------------------------------------------------------------------------
// aeRender constants
//------------------------------------------------------------------------------
const ae::Vec3 aeQuadVertPos[ aeQuadVertCount ] = {
  ae::Vec3( -0.5f, -0.5f, 0.0f ),
  ae::Vec3( 0.5f, -0.5f, 0.0f ),
  ae::Vec3( 0.5f, 0.5f, 0.0f ),
  ae::Vec3( -0.5f, 0.5f, 0.0f )
};

const ae::Vec2 aeQuadVertUvs[ aeQuadVertCount ] = {
  ae::Vec2( 0.0f, 0.0f ),
  ae::Vec2( 1.0f, 0.0f ),
  ae::Vec2( 1.0f, 1.0f ),
  ae::Vec2( 0.0f, 1.0f )
};

const aeQuadIndex aeQuadIndices[ aeQuadIndexCount ] = {
  3, 0, 1,
  3, 1, 2
};
 
//------------------------------------------------------------------------------
// aeSpriteRender member functions
//------------------------------------------------------------------------------
aeSpriteRender::aeSpriteRender()
{
  m_count = 0;
  m_maxCount = 0;
  m_sprites = nullptr;
  
  m_shaderAll = nullptr;
  m_shaderOpaque = nullptr;
  m_shaderTransparent = nullptr;
  
  m_blending = false;
  m_depthTest = false;
  m_depthWrite = false;
  m_sorting = false;
}

void aeSpriteRender::Initialize( uint32_t maxCount )
{
  AE_ASSERT( maxCount );

  m_maxCount = maxCount;
  m_count = 0;
  m_sprites = ae::NewArray< Sprite >( AE_ALLOC_TAG_RENDER, m_maxCount );

  m_vertexData.Initialize( sizeof(Vertex), sizeof(uint16_t), aeQuadVertCount * maxCount, aeQuadIndexCount * maxCount, ae::VertexData::Primitive::Triangle, ae::VertexData::Usage::Dynamic, ae::VertexData::Usage::Static );
  m_vertexData.AddAttribute( "a_position", 3, ae::VertexData::Type::Float, offsetof(Vertex, pos) );
  m_vertexData.AddAttribute( "a_color", 4, ae::VertexData::Type::Float, offsetof(Vertex, color) );
  m_vertexData.AddAttribute( "a_uv", 2, ae::VertexData::Type::Float, offsetof(Vertex, uv) );

  ae::Scratch< uint16_t > scratch( AE_ALLOC_TAG_RENDER, m_maxCount * aeQuadIndexCount );
  uint16_t* indices = scratch.Data();
  for ( uint32_t i = 0; i < m_maxCount; i++ )
  {
    indices[ i * aeQuadIndexCount + 0 ] = i * aeQuadVertCount + aeQuadIndices[ 0 ];
    indices[ i * aeQuadIndexCount + 1 ] = i * aeQuadVertCount + aeQuadIndices[ 1 ];
    indices[ i * aeQuadIndexCount + 2 ] = i * aeQuadVertCount + aeQuadIndices[ 2 ];
    indices[ i * aeQuadIndexCount + 3 ] = i * aeQuadVertCount + aeQuadIndices[ 3 ];
    indices[ i * aeQuadIndexCount + 4 ] = i * aeQuadVertCount + aeQuadIndices[ 4 ];
    indices[ i * aeQuadIndexCount + 5 ] = i * aeQuadVertCount + aeQuadIndices[ 5 ];
  }
  m_vertexData.SetIndices( indices, m_maxCount * aeQuadIndexCount );
}

void aeSpriteRender::Destroy()
{
  if ( m_shaderAll )
  {
    ae::Delete( m_shaderAll );
    m_shaderAll = nullptr;
  }
  
  if ( m_shaderOpaque )
  {
    ae::Delete( m_shaderOpaque );
    m_shaderOpaque = nullptr;
  }
  
  if ( m_shaderTransparent )
  {
    ae::Delete( m_shaderTransparent );
    m_shaderTransparent = nullptr;
  }
  
  m_vertexData.Destroy();
  
  ae::Delete( m_sprites );
  m_sprites = nullptr;
}

void aeSpriteRender::Render( const ae::Matrix4& worldToScreen )
{
  if ( m_count == 0 )
  {
    return;
  }
  
  if ( m_sorting )
  {
    ae::Vec3 cameraView = worldToScreen.GetRow( 2 ).GetXYZ();
    for ( uint32_t i = 0; i < m_count; i++ )
    {
      m_sprites[ i ].sort = cameraView.Dot( m_sprites[ i ].transform.GetTranslation() );
    }

    auto sortFn = []( const Sprite& a, const Sprite& b ) -> bool
    {
      return a.sort > b.sort;
    };
    std::sort( m_sprites, m_sprites + m_count, sortFn );
  }
  
  if ( m_depthWrite && m_blending )
  {
    if ( !m_shaderOpaque )
    {
      m_LoadShaderOpaque();
      m_shaderOpaque->SetDepthWrite( true );
    }
    if ( !m_shaderTransparent )
    {
      m_LoadShaderTransparent();
      m_shaderTransparent->SetBlending( true );
    }
    
    m_shaderOpaque->SetDepthTest( m_depthTest );
    m_shaderTransparent->SetDepthTest( m_depthTest );
    
    m_Render( worldToScreen, m_shaderOpaque );
    m_Render( worldToScreen, m_shaderTransparent );
  }
  else
  {
    if ( !m_shaderAll )
    {
      m_LoadShaderAll();
    }
    
    m_shaderAll->SetDepthTest( m_depthTest );
    m_shaderAll->SetBlending( m_blending );
    
    m_Render( worldToScreen, m_shaderAll );
  }
  
  Clear();
}

void aeSpriteRender::m_Render( const ae::Matrix4& worldToScreen, ae::Shader* shader )
{
  for ( uint32_t i = 0; i < m_textures.Length(); i++ )
  {
    const ae::Texture2D* texture = m_textures.GetKey( i );
    if ( !texture )
    {
      continue;
    }
    uint32_t textureId = texture->GetTexture();

    uint32_t count = 0;
    ae::Scratch< Vertex > scratch( AE_ALLOC_TAG_RENDER, m_count * aeQuadVertCount );
    Vertex* vertices = scratch.Data();
    for ( uint32_t j = 0; j < m_count; j++ )
    {
      Sprite* sprite = &m_sprites[ j ];
      if ( sprite->textureId != textureId )
      {
        continue;
      }
    
      uint32_t idx0 = count * aeQuadVertCount + 0;
      uint32_t idx1 = count * aeQuadVertCount + 1;
      uint32_t idx2 = count * aeQuadVertCount + 2;
      uint32_t idx3 = count * aeQuadVertCount + 3;
      
      vertices[ idx0 ].pos = ae::Vec3( sprite->transform * ae::Vec4( aeQuadVertPos[ 0 ], 1.0f ) );
      vertices[ idx1 ].pos = ae::Vec3( sprite->transform * ae::Vec4( aeQuadVertPos[ 1 ], 1.0f ) );
      vertices[ idx2 ].pos = ae::Vec3( sprite->transform * ae::Vec4( aeQuadVertPos[ 2 ], 1.0f ) );
      vertices[ idx3 ].pos = ae::Vec3( sprite->transform * ae::Vec4( aeQuadVertPos[ 3 ], 1.0f ) );

      vertices[ idx0 ].uv = ae::Vec2( sprite->uvMin.x, sprite->uvMin.y );
      vertices[ idx1 ].uv = ae::Vec2( sprite->uvMax.x, sprite->uvMin.y );
      vertices[ idx2 ].uv = ae::Vec2( sprite->uvMax.x, sprite->uvMax.y );
      vertices[ idx3 ].uv = ae::Vec2( sprite->uvMin.x, sprite->uvMax.y );

      vertices[ idx0 ].color = sprite->color.GetLinearRGBA();
      vertices[ idx1 ].color = sprite->color.GetLinearRGBA();
      vertices[ idx2 ].color = sprite->color.GetLinearRGBA();
      vertices[ idx3 ].color = sprite->color.GetLinearRGBA();
      
      count++;
    }
    // @TODO: Should set all vertices first then render multiple times
    m_vertexData.SetVertices( vertices, count * 4 );

    ae::UniformList uniforms;
    uniforms.Set( "u_worldToScreen", worldToScreen );
    uniforms.Set( "u_tex", texture );

    m_vertexData.Render( shader, count * 2, uniforms );
  }
}

void aeSpriteRender::SetBlending( bool enabled )
{
  m_blending = enabled;
}
void aeSpriteRender::SetDepthTest( bool enabled )
{
  m_depthTest = enabled;
}
void aeSpriteRender::SetDepthWrite( bool enabled )
{
  m_depthWrite = enabled;
}

void aeSpriteRender::SetSorting( bool enabled )
{
  m_sorting = enabled;
}

void aeSpriteRender::AddSprite( const ae::Texture2D* texture, ae::Matrix4 transform, ae::Vec2 uvMin, ae::Vec2 uvMax, aeColor color )
{
  AE_ASSERT_MSG( m_maxCount, "aeSpriteRender is not initialized" );

  if ( !texture )
  {
    return;
  }

  if ( m_count < m_maxCount )
  {
    Sprite* sprite = &m_sprites[ m_count ];
    sprite->transform = transform;
    sprite->uvMin = uvMin;
    sprite->uvMax = uvMax;
    sprite->color = color;
    sprite->textureId = texture->GetTexture();
    sprite->sort = 0.0f;
    m_count++;

    m_textures.Set( texture, 0 );
  }
}

void aeSpriteRender::Clear()
{
  m_count = 0;
  m_textures.Clear();
}

void aeSpriteRender::m_LoadShaderAll()
{
  if ( m_shaderAll )
  {
    return;
  }
  
  const char* vertexStr = "\
    AE_UNIFORM_HIGHP mat4 u_worldToScreen;\
    AE_IN_HIGHP vec3 a_position;\
    AE_IN_HIGHP vec2 a_uv;\
    AE_IN_HIGHP vec4 a_color;\
    AE_OUT_HIGHP vec2 v_uv;\
    AE_OUT_HIGHP vec4 v_color;\
    void main()\
    {\
      v_uv = a_uv;\
      v_color = a_color;\
      gl_Position = u_worldToScreen * vec4( a_position, 1.0 );\
    }";
  const char* fragStr = "\
    uniform sampler2D u_tex;\
    AE_IN_HIGHP vec2 v_uv;\
    AE_IN_HIGHP vec4 v_color;\
    void main()\
    {\
      AE_COLOR = AE_TEXTURE2D( u_tex, v_uv ) * v_color;\
    }";
  
  m_shaderAll = ae::New< ae::Shader >( AE_ALLOC_TAG_RENDER );
  m_shaderAll->Initialize( vertexStr, fragStr, nullptr, 0 );
}

void aeSpriteRender::m_LoadShaderOpaque()
{
  if ( m_shaderOpaque )
  {
    return;
  }
  
  const char* vertexStr = "\
    AE_UNIFORM_HIGHP mat4 u_worldToScreen;\
    AE_IN_HIGHP vec3 a_position;\
    AE_IN_HIGHP vec2 a_uv;\
    AE_IN_HIGHP vec4 a_color;\
    AE_OUT_HIGHP vec2 v_uv;\
    AE_OUT_HIGHP vec4 v_color;\
    void main()\
    {\
      v_uv = a_uv;\
      v_color = a_color;\
      gl_Position = u_worldToScreen * vec4( a_position, 1.0 );\
    }";
  const char* fragStr = "\
    uniform sampler2D u_tex;\
    AE_IN_HIGHP vec2 v_uv;\
    AE_IN_HIGHP vec4 v_color;\
    void main()\
    {\
      vec4 color = AE_TEXTURE2D( u_tex, v_uv ) * v_color;\
      if ( color.a < 0.99 ) { discard; }\
      AE_COLOR = color;\
    }";
  
  m_shaderOpaque = ae::New< ae::Shader >( AE_ALLOC_TAG_RENDER );
  m_shaderOpaque->Initialize( vertexStr, fragStr, nullptr, 0 );
}

void aeSpriteRender::m_LoadShaderTransparent()
{
  if ( m_shaderTransparent )
  {
    return;
  }
  
  const char* vertexStr = "\
    AE_UNIFORM_HIGHP mat4 u_worldToScreen;\
    AE_IN_HIGHP vec3 a_position;\
    AE_IN_HIGHP vec2 a_uv;\
    AE_IN_HIGHP vec4 a_color;\
    AE_OUT_HIGHP vec2 v_uv;\
    AE_OUT_HIGHP vec4 v_color;\
    void main()\
    {\
      v_uv = a_uv;\
      v_color = a_color;\
      gl_Position = u_worldToScreen * vec4( a_position, 1.0 );\
    }";
  const char* fragStr = "\
    uniform sampler2D u_tex;\
    AE_IN_HIGHP vec2 v_uv;\
    AE_IN_HIGHP vec4 v_color;\
    void main()\
    {\
      vec4 color = AE_TEXTURE2D( u_tex, v_uv ) * v_color;\
      if ( color.a >= 0.99 ) { discard; }\
      AE_COLOR = color;\
    }";
  
  m_shaderTransparent = ae::New< ae::Shader >( AE_ALLOC_TAG_RENDER );
  m_shaderTransparent->Initialize( vertexStr, fragStr, nullptr, 0 );
}

//------------------------------------------------------------------------------
// Text constants
//------------------------------------------------------------------------------
const uint32_t kCharsPerString = 64;

//------------------------------------------------------------------------------
// aeTextRender member functions
//------------------------------------------------------------------------------
void aeTextRender::Initialize( const ae::Texture2D* texture, uint32_t fontSize )
{
  m_texture = texture;
  m_fontSize = fontSize;
  m_rectCount = 0;

  m_vertexData.Initialize( sizeof( Vertex ), sizeof( uint16_t ), kMaxTextRects * m_rects[ 0 ].text.Size() * aeQuadVertCount, kMaxTextRects * kCharsPerString * aeQuadIndexCount, ae::VertexData::Primitive::Triangle, ae::VertexData::Usage::Dynamic, ae::VertexData::Usage::Dynamic );
  m_vertexData.AddAttribute( "a_position", 3, ae::VertexData::Type::Float, offsetof( Vertex, pos ) );
  m_vertexData.AddAttribute( "a_uv", 2, ae::VertexData::Type::Float, offsetof( Vertex, uv ) );
  m_vertexData.AddAttribute( "a_color", 4, ae::VertexData::Type::Float, offsetof( Vertex, color ) );

  // Load shader
  const char* vertexStr = "\
    AE_UNIFORM_HIGHP mat4 u_uiToScreen;\
    AE_IN_HIGHP vec3 a_position;\
    AE_IN_HIGHP vec2 a_uv;\
    AE_IN_HIGHP vec4 a_color;\
    AE_OUT_HIGHP vec2 v_uv;\
    AE_OUT_HIGHP vec4 v_color;\
    void main()\
    {\
      v_uv = a_uv;\
      v_color = a_color;\
      gl_Position = u_uiToScreen * vec4( a_position, 1.0 );\
    }";
  const char* fragStr = "\
    uniform sampler2D u_tex;\
    AE_IN_HIGHP vec2 v_uv;\
    AE_IN_HIGHP vec4 v_color;\
    void main()\
    {\
      if ( AE_TEXTURE2D( u_tex, v_uv ).r < 0.5 ) { discard; };\
      AE_COLOR = v_color;\
    }";
  m_shader.Initialize( vertexStr, fragStr, nullptr, 0 );
}

void aeTextRender::Terminate()
{
  m_shader.Destroy();
  m_vertexData.Destroy();
}

void aeTextRender::Render( const ae::Matrix4& uiToScreen )
{
  uint32_t vertCount = 0;
  uint32_t indexCount = 0;
  ae::Scratch< Vertex > verts( AE_ALLOC_TAG_RENDER, m_vertexData.GetMaxVertexCount() );
  ae::Scratch< uint16_t > indices( AE_ALLOC_TAG_RENDER, m_vertexData.GetMaxIndexCount() );

  for ( uint32_t i = 0; i < m_rectCount; i++ )
  {
    const TextRect& rect = m_rects[ i ];
    ae::Vec3 pos = rect.pos;
    pos.y -= rect.size.y;

    const char* start = rect.text.c_str();
    const char* str = start;
    while ( str[ 0 ] )
    {
      if ( !isspace( str[ 0 ] ) )
      {
        int32_t index = str[ 0 ];
        uint32_t columns = m_texture->GetWidth() / m_fontSize;
        ae::Vec2 offset( index % columns, columns - index / columns - 1 ); // @HACK: Assume same number of columns and rows

        for ( uint32_t j = 0; j < aeQuadIndexCount; j++ )
        {
          indices.GetSafe( indexCount ) = aeQuadIndices[ j ] + vertCount;
          indexCount++;
        }

        AE_ASSERT( vertCount + aeQuadVertCount <= verts.Length() );
        // Bottom Left
        verts[ vertCount ].pos = pos;
        verts[ vertCount ].uv = ( aeQuadVertUvs[ 0 ] + offset ) / columns;
        verts[ vertCount ].color = rect.color;
        vertCount++;
        // Bottom Right
        verts[ vertCount ].pos = pos + ae::Vec3( rect.size.x, 0.0f, 0.0f );
        verts[ vertCount ].uv = ( aeQuadVertUvs[ 1 ] + offset ) / columns;
        verts[ vertCount ].color = rect.color;
        vertCount++;
        // Top Right
        verts[ vertCount ].pos = pos + ae::Vec3( rect.size.x, rect.size.y, 0.0f );
        verts[ vertCount ].uv = ( aeQuadVertUvs[ 2 ] + offset ) / columns;
        verts[ vertCount ].color = rect.color;
        vertCount++;
        // Top Left
        verts[ vertCount ].pos = pos + ae::Vec3( 0.0f, rect.size.y, 0.0f );
        verts[ vertCount ].uv = ( aeQuadVertUvs[ 3 ] + offset ) / columns;
        verts[ vertCount ].color = rect.color;
        vertCount++;
      }

      if ( str[ 0 ] == '\n' || str[ 0 ] == '\r' )
      {
        pos.x = rect.pos.x;
        pos.y -= rect.size.y;
      }
      else
      {
        pos.x += rect.size.x;
      }
      str++;
    }
  }

  m_vertexData.SetVertices( verts.Data(), vertCount );
  m_vertexData.SetIndices( indices.Data(), indexCount );

  ae::UniformList uniforms;
  uniforms.Set( "u_uiToScreen", uiToScreen );
  uniforms.Set( "u_tex", m_texture );
  m_vertexData.Render( &m_shader, uniforms );

  m_rectCount = 0;
}

void aeTextRender::Add( ae::Vec3 pos, ae::Vec2 size, const char* str, aeColor color, uint32_t lineLength, uint32_t charLimit )
{
  if ( m_rectCount >= kMaxTextRects )
  {
    return;
  }

  TextRect* rect = &m_rects[ m_rectCount ];
  m_rectCount++;

  rect->pos = pos;
  rect->size = size;
  m_ParseText( str, lineLength, charLimit, &rect->text );
  rect->color = color;
}

uint32_t aeTextRender::GetLineCount( const char* str, uint32_t lineLength, uint32_t charLimit ) const
{
  return m_ParseText( str, lineLength, charLimit, nullptr );
}

uint32_t aeTextRender::m_ParseText( const char* str, uint32_t lineLength, uint32_t charLimit, ae::Str512* outText ) const
{
  if ( outText )
  {
    *outText = "";
  }

  uint32_t lineCount = 1;
  const char* start = str;
  uint32_t lineChars = 0;
  while ( str[ 0 ] )
  {
    // Truncate displayed string based on param
    if ( charLimit && (uint32_t)( str - start ) >= charLimit )
    {
      break;
    }

    bool isNewlineChar = ( str[ 0 ] == '\n' || str[ 0 ] == '\r' );

    if ( lineLength && !isNewlineChar && isspace( str[ 0 ] ) )
    {
      // Prevent words from being split across lines
      uint32_t wordRemainder = 1;
      while ( str[ wordRemainder ] && !isspace( str[ wordRemainder ] ) )
      {
        wordRemainder++;
      }

      if ( lineChars + wordRemainder > lineLength )
      {
        if ( outText )
        {
          outText->Append( "\n" );
        }
        lineCount++;
        lineChars = 0;
      }
    }

    // Skip non-newline whitespace at the beginning of a line
    if ( lineChars || isNewlineChar || !isspace( str[ 0 ] ) )
    {
      if ( outText )
      {
        // @TODO: aeStr should support appending chars
        char hack[] = { str[ 0 ], 0 };
        outText->Append( hack );
      }

      lineChars = isNewlineChar ? 0 : lineChars + 1;
    }
    if ( isNewlineChar )
    {
      lineCount++;
    }

    str++;
  }

  return lineCount;
}
