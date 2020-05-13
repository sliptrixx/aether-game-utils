//------------------------------------------------------------------------------
// aeOpenGLRender.cpp
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
#include "aeRender.h"
#include "aeAlloc.h"
#include "aeCommonRender.h"
#include "aeLog.h"
#include "aePlatform.h"
#include "aeVfs.h"
#include "aeWindow.h"

#include "SDL.h"
#if _AE_WINDOWS_
  #define GLEW_STATIC 1
  #include <GL\glew.h>
#elif _AE_EMSCRIPTEN_
  #include <GLES2/gl2.h>
#elif _AE_LINUX_
  #include <GL/gl.h>
  #include <GLES3/gl3.h>
#else
  #include <OpenGL/gl3.h>
#endif

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#if _AE_EMSCRIPTEN_
  #define GL_SAMPLER_3D 0x0
  #define GL_TEXTURE_3D 0x0
  void glGenVertexArrays( GLsizei n, GLuint *arrays ) {}
  void glDeleteVertexArrays( GLsizei n, const GLuint *arrays ) {}
  void glBindVertexArray( GLuint array ) {}
  void glBindFragDataLocation( GLuint program, GLuint colorNumber, const char* name ) {}
  void glClearDepth( float depth ) { glClearDepthf( depth ); }
#endif

//------------------------------------------------------------------------------
// Helpers
//------------------------------------------------------------------------------
GLenum aeVertexDataTypeToGL( aeVertexDataType::Type type )
{
  switch ( type )
  {
    case aeVertexDataType::UInt8:
      return GL_UNSIGNED_BYTE;
    case aeVertexDataType::UInt16:
      return GL_UNSIGNED_SHORT;
    case aeVertexDataType::UInt32:
      return GL_UNSIGNED_INT;
    case aeVertexDataType::Float:
      return GL_FLOAT;
    default:
      AE_FAIL();
      return 0;
  }
}

#define AE_CHECK_GL_ERROR() do { if ( GLenum err = glGetError() ) { AE_FAIL_MSG( "GL Error: #", err ); } } while ( 0 )

void CheckFramebufferComplete( GLuint framebuffer )
{
  GLenum fboStatus = glCheckFramebufferStatus( GL_FRAMEBUFFER );
  if ( fboStatus != GL_FRAMEBUFFER_COMPLETE )
  {
    const char* errStr = "unknown";
    switch ( fboStatus )
    {
      case GL_FRAMEBUFFER_UNDEFINED:
        errStr = "GL_FRAMEBUFFER_UNDEFINED";
        break;
      case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
        errStr = "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
        break;
      case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
        errStr = "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
        break;
      case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
        errStr = "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER";
        break;
      case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
        errStr = "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER";
        break;
      case GL_FRAMEBUFFER_UNSUPPORTED:
        errStr = "GL_FRAMEBUFFER_UNSUPPORTED";
        break;
      case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
        errStr = "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
        break;
      case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
        errStr = "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS";
        break;
      default:
        break;
    }
    AE_FAIL_MSG( "GL FBO Error: (#) #", fboStatus, errStr );
  }
}

#if _AE_DEBUG_
void aeOpenGLDebugCallback( GLenum source,
  GLenum type,
  GLuint id,
  GLenum severity,
  GLsizei length,
  const GLchar* message,
  const void* userParam )
{
  AE_INFO( message );

  std::cout << "---------------------opengl-callback-start------------" << std::endl;
  std::cout << "message: " << message << std::endl;
  std::cout << "type: ";
  switch ( type )
  {
    case GL_DEBUG_TYPE_ERROR:
      std::cout << "ERROR";
      break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
      std::cout << "DEPRECATED_BEHAVIOR";
      break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
      std::cout << "UNDEFINED_BEHAVIOR";
      break;
    case GL_DEBUG_TYPE_PORTABILITY:
      std::cout << "PORTABILITY";
      break;
    case GL_DEBUG_TYPE_PERFORMANCE:
      std::cout << "PERFORMANCE";
      break;
    case GL_DEBUG_TYPE_OTHER:
      std::cout << "OTHER";
      break;
  }
  std::cout << std::endl;

  std::cout << "id: " << id << std::endl;
  std::cout << "severity: ";
  switch ( severity )
  {
    case GL_DEBUG_SEVERITY_LOW:
      std::cout << "LOW";
      break;
    case GL_DEBUG_SEVERITY_MEDIUM:
      std::cout << "MEDIUM";
      break;
    case GL_DEBUG_SEVERITY_HIGH:
      std::cout << "HIGH";
      break;
  }
  std::cout << std::endl;
  std::cout << "---------------------opengl-callback-end--------------" << std::endl;
}
#endif

//------------------------------------------------------------------------------
// aeVertexData member functions
//------------------------------------------------------------------------------
aeVertexData::aeVertexData()
{
  memset( this, 0, sizeof(aeVertexData) );
  m_primitive = (aeVertexPrimitive::Type)-1;
  m_vertexUsage = (aeVertexUsage::Type)-1;
  m_indexUsage = (aeVertexUsage::Type)-1;
  m_vertices = ~0;
  m_indices = ~0;
}

void aeVertexData::Initialize( uint32_t vertexSize, uint32_t indexSize, uint32_t maxVertexCount, uint32_t maxIndexCount, aeVertexPrimitive::Type primitive, aeVertexUsage::Type vertexUsage, aeVertexUsage::Type indexUsage )
{
  AE_ASSERT( m_vertexSize == 0 );
  AE_ASSERT( vertexSize );
  AE_ASSERT( m_indexSize == 0 );
  AE_ASSERT( indexSize == sizeof(uint8_t) || indexSize == sizeof(uint16_t) || indexSize == sizeof(uint32_t) );

  memset( this, 0, sizeof(aeVertexData) );
  m_maxVertexCount = maxVertexCount;
  m_maxIndexCount = maxIndexCount;
  m_primitive = primitive;
  m_vertexUsage = vertexUsage;
  m_indexUsage = indexUsage;
  m_vertices = ~0;
  m_indices = ~0;
  m_vertexSize = vertexSize;
  m_indexSize = indexSize;
  
  glGenVertexArrays( 1, &m_array );
  glBindVertexArray( m_array );
}

void aeVertexData::Destroy()
{
  if ( m_vertexReadable ) { aeAlloc::Release( (uint8_t*)m_vertexReadable ); }
  if ( m_indexReadable ) { aeAlloc::Release( (uint8_t*)m_indexReadable ); }
  
  glDeleteVertexArrays( 1, &m_array );
  if ( m_vertices != ~0 ) { glDeleteBuffers( 1, &m_vertices ); }
  if ( m_indices != ~0 ) { glDeleteBuffers( 1, &m_indices ); }
  
  memset( this, 0, sizeof(aeVertexData) );
  m_primitive = (aeVertexPrimitive::Type)-1;
  m_vertexUsage = (aeVertexUsage::Type)-1;
  m_indexUsage = (aeVertexUsage::Type)-1;
  m_vertices = ~0;
  m_indices = ~0;
}

void aeVertexData::AddAttribute( const char *name, uint32_t componentCount, aeVertexDataType::Type type, uint32_t offset )
{
  AE_ASSERT( m_vertices == ~0 && m_indices == ~0 );
  
  AE_ASSERT( m_attributeCount < countof(m_attributes) );
  aeVertexAttribute* attribute = &m_attributes[ m_attributeCount ];
  m_attributeCount++;
  
  size_t length = strlen( name );
  AE_ASSERT( length < kMaxShaderAttributeNameLength );
  strcpy( attribute->name, name );
  attribute->componentCount = componentCount;
  attribute->type = aeVertexDataTypeToGL( type );
  attribute->offset = offset;
}

void aeVertexData::m_SetVertices( const void* vertices, uint32_t count )
{
  AE_ASSERT( m_vertexSize );
  AE_ASSERT_MSG( count <= m_maxVertexCount, "# #", count, m_maxVertexCount );

  if ( m_indices != ~0 )
  {
    AE_ASSERT( m_indexSize != 0 );
  }
  if ( m_indexSize )
  {
    AE_ASSERT_MSG( count <= (uint64_t)1 << ( m_indexSize * 8 ), "Vertex count (#) too high for index of size #", count, m_indexSize );
  }
  
  if( m_vertexUsage == aeVertexUsage::Static )
  {
    AE_ASSERT( count );
    AE_ASSERT_MSG( !m_vertexCount, "Cannot re-set vertices, buffer was created as static!" );
    AE_ASSERT( m_vertices == ~0 );

    m_vertexCount = count;

    glGenBuffers( 1, &m_vertices );
    glBindVertexArray( m_array );
    glBindBuffer( GL_ARRAY_BUFFER, m_vertices );
    glBufferData( GL_ARRAY_BUFFER, count * m_vertexSize, vertices, GL_STATIC_DRAW );
    return;
  }
  
  if( m_vertexUsage == aeVertexUsage::Dynamic )
  {
    m_vertexCount = count;

    if ( !m_vertexCount )
    {
      return;
    }
    
    if( m_vertices == ~0 )
    {
      glGenBuffers( 1, &m_vertices );
      glBindVertexArray( m_array );
      glBindBuffer( GL_ARRAY_BUFFER, m_vertices );
      glBufferData( GL_ARRAY_BUFFER, m_vertexSize * m_maxVertexCount, nullptr, GL_DYNAMIC_DRAW );
    }
    
    glBindVertexArray( m_array );
    glBindBuffer( GL_ARRAY_BUFFER, m_vertices );
    glBufferSubData( GL_ARRAY_BUFFER, 0, count * m_vertexSize, vertices );
    return;
  }
  
  AE_FAIL();
}

void aeVertexData::m_SetIndices( const void* indices, uint32_t count )
{
  AE_ASSERT( m_indexSize );
  AE_ASSERT( count % 3 == 0 );
  AE_ASSERT( count <= m_maxIndexCount );
  
  if( m_indexUsage == aeVertexUsage::Static )
  {
    AE_ASSERT( count );
    AE_ASSERT( !m_indexCount );
    AE_ASSERT( m_indices == ~0 );

    m_indexCount = count;

    glGenBuffers( 1, &m_indices );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_indices );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, m_indexCount * m_indexSize, indices, GL_STATIC_DRAW );
    return;
  }
  
  if( m_indexUsage == aeVertexUsage::Dynamic )
  {
    m_indexCount = count;

    if ( !m_indexCount )
    {
      return;
    }
    
    if( m_indices == ~0 )
    {
      glGenBuffers( 1, &m_indices );
      glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_indices );
      glBufferData( GL_ELEMENT_ARRAY_BUFFER, m_indexSize * m_maxIndexCount, nullptr, GL_DYNAMIC_DRAW );
    }
    
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_indices );
    glBufferSubData( GL_ELEMENT_ARRAY_BUFFER, 0, m_indexCount * m_indexSize, indices );
    return;
  }
  
  AE_FAIL();
}

void aeVertexData::SetVertices( const void *vertices, uint32_t count )
{
  AE_ASSERT( m_vertexSize );
  if ( m_vertexUsage == aeVertexUsage::Static )
  {
    m_SetVertices( vertices, count );
    AE_ASSERT( !m_vertexReadable );
    m_vertexReadable = aeAlloc::AllocateArray< uint8_t >( count * m_vertexSize );
    memcpy( m_vertexReadable, vertices, count * m_vertexSize );
  }
  else if ( m_vertexUsage == aeVertexUsage::Dynamic )
  {
    m_SetVertices( vertices, count );
    if ( !m_vertexReadable ) { m_vertexReadable = aeAlloc::AllocateArray< uint8_t >( m_maxVertexCount * m_vertexSize ); }
    memcpy( m_vertexReadable, vertices, count * m_vertexSize );
  }
  else
  {
    AE_FAIL_MSG( "Invalid vertex usage" );
  }
}

void aeVertexData::SetIndices( const void* indices, uint32_t count )
{
  AE_ASSERT( m_indexSize );

  if ( count && _AE_DEBUG_ )
  {
    int32_t badIndex = -1;
    
    if ( m_indexSize == 1 )
    {
      uint8_t* indicesCheck = (uint8_t*)indices;
      for ( uint32_t i = 0; i < count; i++ )
      {
        if ( indicesCheck[ i ] >= m_maxVertexCount )
        {
          badIndex = indicesCheck[ i ];
          break;
        }
      }
    }
    else if ( m_indexSize == 2 )
    {
      uint16_t* indicesCheck = (uint16_t*)indices;
      for ( uint32_t i = 0; i < count; i++ )
      {
        if ( indicesCheck[ i ] >= m_maxVertexCount )
        {
          badIndex = indicesCheck[ i ];
          break;
        }
      }
    }
    else if ( m_indexSize == 4 )
    {
      uint32_t* indicesCheck = (uint32_t*)indices;
      for ( uint32_t i = 0; i < count; i++ )
      {
        if ( indicesCheck[ i ] >= m_maxVertexCount )
        {
          badIndex = indicesCheck[ i ];
          break;
        }
      }
    }

    if ( badIndex >= 0 )
    {
      AE_FAIL_MSG( "Out of range index detected #", badIndex );
    }
  }

  if ( m_indexUsage == aeVertexUsage::Static )
  {
    m_SetIndices( indices, count );
    AE_ASSERT( !m_indexReadable );
    m_indexReadable = aeAlloc::AllocateArray< uint8_t >( count * m_indexSize );
    memcpy( m_indexReadable, indices, count * m_indexSize );
  }
  else if ( m_indexUsage == aeVertexUsage::Dynamic )
  {
    m_SetIndices( indices, count );
    if ( !m_indexReadable ) { m_indexReadable = aeAlloc::AllocateArray< uint8_t >( m_maxIndexCount * m_indexSize ); }
    memcpy( m_indexReadable, indices, count * m_indexSize );
  }
  else
  {
    AE_FAIL_MSG( "Invalid index usage" );
  }
}

const void* aeVertexData::GetVertices() const
{
  AE_ASSERT( m_vertexReadable != nullptr );
  return m_vertexReadable;
}

const void* aeVertexData::GetIndices() const
{
  AE_ASSERT( m_indexReadable != nullptr );
  return m_indexReadable;
}

void aeVertexData::Render( const aeShader* shader, const aeUniformList& uniforms )
{
  Render( shader, 0, uniforms ); // Draw all
}

void aeVertexData::Render( const aeShader* shader, uint32_t primitiveCount, const aeUniformList& uniforms )
{
  AE_ASSERT( shader );
  
  if ( m_vertices == ~0 || !m_vertexCount || ( m_indices != ~0 && !m_indexCount ) )
  {
    return;
  }

  shader->Activate( uniforms );

  glBindVertexArray( m_array );
  AE_CHECK_GL_ERROR();

  glBindBuffer( GL_ARRAY_BUFFER, m_vertices );
  AE_CHECK_GL_ERROR();

  if ( m_indexCount && m_primitive != aeVertexPrimitive::Point )
  {
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_indices );
    AE_CHECK_GL_ERROR();
  }

  for ( uint32_t i = 0; i < shader->GetAttributeCount(); i++ )
  {
    const aeShaderAttribute* shaderAttribute = shader->GetAttributeByIndex( i );
    const aeVertexAttribute* vertexAttribute = m_GetAttributeByName( shaderAttribute->name );

    AE_ASSERT_MSG( vertexAttribute, "No vertex attribute named '#'", shaderAttribute->name );
    // @TODO: Verify attribute type and size match

    GLint location = shaderAttribute->location;
    AE_ASSERT( location != -1 );
    glEnableVertexAttribArray( location );
    AE_CHECK_GL_ERROR();

    uint32_t componentCount = vertexAttribute->componentCount;
    uint64_t attribOffset = vertexAttribute->offset;
    glVertexAttribPointer( location, componentCount, vertexAttribute->type, GL_FALSE, m_vertexSize, (void*)attribOffset );
    AE_CHECK_GL_ERROR();
  }

  int64_t start = 0; // TODO: Add support to start drawing at non-zero index
  int32_t count = 0;

  // Draw
  GLenum mode;
  if( m_primitive == aeVertexPrimitive::Triangle )
  {
    count = primitiveCount ? primitiveCount * 3 : m_indexCount;
    mode = GL_TRIANGLES;
  }
  else if( m_primitive == aeVertexPrimitive::Line )
  {
    count = primitiveCount ? primitiveCount * 2 : m_indexCount;
    mode = GL_LINES;
  }
  else if( m_primitive == aeVertexPrimitive::Point )
  {
    count = primitiveCount ? primitiveCount : m_indexCount;
    mode = GL_POINTS;
  }
  else
  {
    AE_FAIL();
    return;
  }
  
  if ( m_indexCount && mode != GL_POINTS )
  {
    if ( count == 0 ) { count = m_indexCount; }
    AE_ASSERT( start + count <= m_indexCount );
    if ( mode == GL_TRIANGLES ) { AE_ASSERT( count % 3 == 0 && start % 3 == 0 ); }
    
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_indices );
    AE_CHECK_GL_ERROR();
    GLenum type = 0;
    if ( m_indexSize == sizeof(uint8_t) ) { type = GL_UNSIGNED_BYTE; }
    else if ( m_indexSize == sizeof(uint16_t) ) { type = GL_UNSIGNED_SHORT; }
    else if ( m_indexSize == sizeof(uint32_t) ) { type = GL_UNSIGNED_INT; }
    glDrawElements( mode, count, type, (void*)start );
    AE_CHECK_GL_ERROR();
  }
  else
  {
    if ( count == 0 ) { count = m_vertexCount; }
    AE_ASSERT( start + count <= m_vertexCount );
    if ( mode == GL_TRIANGLES ) { AE_ASSERT( count % 3 == 0 && start % 3 == 0 ); }
    
    glDrawArrays( mode, start, count );
    AE_CHECK_GL_ERROR();
  }
}

const aeVertexAttribute* aeVertexData::m_GetAttributeByName( const char* name ) const
{
  for ( uint32_t i = 0; i < m_attributeCount; i++ )
  {
    if ( strcmp( m_attributes[ i ].name, name ) == 0 )
    {
      return &m_attributes[ i ];
    }
  }

  return nullptr;
}

//------------------------------------------------------------------------------
// aeShader member functions
//------------------------------------------------------------------------------
aeShader::aeShader()
{
  m_fragmentShader = 0;
  m_vertexShader = 0;
  m_program = 0;
  
  m_blending = false;
  m_depthTest = false;
  m_depthWrite = false;
  m_culling = aeShaderCulling::None;

  m_attributeCount = 0;
}

aeShader::~aeShader()
{
  Destroy();
}

void aeShader::Initialize( const char* vertexStr, const char* fragStr, const char* const* defines, int32_t defineCount )
{
  AE_CHECK_GL_ERROR();
  AE_ASSERT( !m_program );

  m_program = glCreateProgram();
  
  m_vertexShader = m_LoadShader( vertexStr, aeShaderType::Vertex, defines, defineCount );
  m_fragmentShader = m_LoadShader( fragStr, aeShaderType::Fragment, defines, defineCount );
  
  if ( !m_vertexShader || !m_fragmentShader )
  {
    AE_LOG( "Failed to load shader!" );
    AE_FAIL();
  }
  
  glAttachShader( m_program, m_vertexShader );
  glAttachShader( m_program, m_fragmentShader );
  
  glLinkProgram( m_program );
  
  GLint status;
  glGetProgramiv( m_program, GL_LINK_STATUS, &status );
  if( status == GL_FALSE )
  {
    GLint logLength = 0;
    glGetProgramiv( m_program, GL_INFO_LOG_LENGTH, &logLength );
    
    char* log = nullptr;
    if( logLength > 0 )
    {
      log = new char[ logLength ];
      glGetProgramInfoLog( m_program, logLength, NULL, (GLchar*)log );
    }
    
    if ( log )
    {
      AE_FAIL_MSG( log );
      delete[] log;
    }
    else
    {
      AE_FAIL();
    }
    Destroy();
  }
  
  GLint attribCount = 0;
  glGetProgramiv( m_program, GL_ACTIVE_ATTRIBUTES, &attribCount );
  AE_ASSERT( 0 < attribCount && attribCount <= kMaxShaderAttributeCount );
  GLint maxLen = 0;
  glGetProgramiv( m_program, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &maxLen );
  AE_ASSERT( 0 < maxLen && maxLen <= kMaxShaderAttributeNameLength );
  for ( int32_t i = 0; i < attribCount; i++ )
  {
    AE_ASSERT( m_attributeCount < countof(m_attributes) );
    aeShaderAttribute* attribute = &m_attributes[ m_attributeCount ];
    m_attributeCount++;

    GLsizei length;
    GLint size;
    glGetActiveAttrib( m_program, i, kMaxShaderAttributeNameLength, &length, &size, (GLenum*)&attribute->type, (GLchar*)attribute->name );
    
    attribute->location = glGetAttribLocation( m_program, attribute->name );
    AE_ASSERT( attribute->location != -1 );
  }
  
  GLint uniformCount = 0;
  maxLen = 0;
  glGetProgramiv( m_program, GL_ACTIVE_UNIFORMS, &uniformCount );
  glGetProgramiv( m_program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxLen );
  AE_ASSERT( maxLen <= aeStr32::kMaxLength ); // @TODO: Read from aeShaderUniform

  for( int32_t i = 0; i < uniformCount; i++ )
  {
    aeShaderUniform uniform;

    GLint size = 0;
    char name[ aeStr32::kMaxLength ]; // @TODO: Read from aeShaderUniform
    glGetActiveUniform( m_program, i, sizeof(name), nullptr, &size, (GLenum*)&uniform.type, (GLchar*)name );
    AE_ASSERT( size == 1 );
    
    switch ( uniform.type )
    {
      case GL_SAMPLER_2D:
      case GL_SAMPLER_3D:
      case GL_FLOAT:
      case GL_FLOAT_VEC2:
      case GL_FLOAT_VEC3:
      case GL_FLOAT_VEC4:
      case GL_FLOAT_MAT4:
        break;
      default:
        AE_FAIL_MSG( "Unsupported uniform '#' type #", name, uniform.type );
        break;
    }

    uniform.name = name;
    uniform.location = glGetUniformLocation( m_program, name );
    AE_ASSERT( uniform.location != -1 );

    m_uniforms.Set( name, uniform );
  }

  AE_CHECK_GL_ERROR();
}

void aeShader::Destroy()
{
  if( m_fragmentShader != 0 )
  {
    glDeleteShader( m_fragmentShader );
    m_fragmentShader = 0;
  }
  
  if( m_vertexShader != 0 )
  {
    glDeleteShader( m_vertexShader );
    m_vertexShader = 0;
  }
  
  if( m_program != 0 )
  {
    glDeleteProgram( m_program );
    m_program = 0;
  }
}

void aeShader::Activate( const aeUniformList& uniforms ) const
{
  AE_CHECK_GL_ERROR();

  glUseProgram( m_program );

  // Set rendering params
  if ( m_blending )
  {
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
  }
  else
  {
    glDisable( GL_BLEND );
  }

  glDepthMask( m_depthWrite ? GL_TRUE : GL_FALSE );

  if ( m_depthTest )
  {
    glDepthFunc( GL_LEQUAL );
    glEnable( GL_DEPTH_TEST );
  }
  else
  {
    glDisable( GL_DEPTH_TEST );
  }

  if ( m_culling == aeShaderCulling::None )
  {
    glDisable( GL_CULL_FACE );
  }
  else
  {
    glEnable( GL_CULL_FACE );
    glFrontFace( ( m_culling == aeShaderCulling::ClockwiseFront ) ? GL_CW : GL_CCW );
  }

  // Set shader uniforms
  uint32_t textureIndex = 0;
  for ( uint32_t i = 0; i < m_uniforms.Length(); i++ )
  {
    const char* uniformVarName = m_uniforms.GetKey( i ).c_str();
    const aeShaderUniform* uniformVar = &m_uniforms.GetValue( i );
    const aeUniformList::Value* uniformValue = uniforms.Get( uniformVarName );
    
    // Start validation
    AE_ASSERT_MSG( uniformValue, "Shader uniform '#' value is not set", uniformVarName );
    uint32_t typeSize = 0;
    switch ( uniformVar->type )
    {
      case GL_SAMPLER_2D:
        typeSize = 0;
        break;
      case GL_SAMPLER_3D:
        typeSize = 0;
        break;
      case GL_FLOAT:
        typeSize = 1;
        break;
      case GL_FLOAT_VEC2:
        typeSize = 2;
        break;
      case GL_FLOAT_VEC3:
        typeSize = 3;
        break;
      case GL_FLOAT_VEC4:
        typeSize = 4;
        break;
      case GL_FLOAT_MAT4:
        typeSize = 16;
        break;
      default:
        AE_FAIL_MSG( "Unsupported uniform '#' type #", uniformVarName, uniformVar->type );
        break;
    }
    AE_ASSERT_MSG( uniformValue->size == typeSize, "Uniform type mismatch '#' type:# size:#", uniformVarName, uniformVar->type, uniformValue->size );
    // End validation

    if ( uniformVar->type == GL_SAMPLER_2D )
    {
      AE_ASSERT_MSG( uniformValue->sampler, "Uniform sampler 2d '#' value is invalid #", uniformVarName, uniformValue->sampler );
      glActiveTexture( GL_TEXTURE0 + textureIndex );
      glBindTexture( uniformValue->target, uniformValue->sampler );
      glUniform1i( uniformVar->location, textureIndex );
      textureIndex++;
    }
    else if ( uniformVar->type == GL_SAMPLER_3D )
    {
      AE_ASSERT_MSG( uniformValue->sampler, "Uniform sampler 2d '#' value is invalid #", uniformVarName, uniformValue->sampler );
      glActiveTexture( GL_TEXTURE0 + textureIndex );
      glBindTexture( GL_TEXTURE_3D, uniformValue->sampler );
      glUniform1i( uniformVar->location, textureIndex );
      textureIndex++;
    }
    else if ( uniformVar->type == GL_FLOAT )
    {
      glUniform1fv( uniformVar->location, 1, uniformValue->value.data );
    }
    else if ( uniformVar->type == GL_FLOAT_VEC2 )
    {
      glUniform2fv( uniformVar->location, 1, uniformValue->value.data );
    }
    else if ( uniformVar->type == GL_FLOAT_VEC3 )
    {
      glUniform3fv( uniformVar->location, 1, uniformValue->value.data );
    }
    else if ( uniformVar->type == GL_FLOAT_VEC4 )
    {
      glUniform4fv( uniformVar->location, 1, uniformValue->value.data );
    }
    else if ( uniformVar->type == GL_FLOAT_MAT4 )
    {
#if _AE_EMSCRIPTEN_
      // WebGL/Emscripten doesn't support glUniformMatrix4fv auto-transpose
      aeFloat4x4 transposedTransform = uniformValue->value.GetTransposeCopy();
      glUniformMatrix4fv( uniformVar->location, 1, GL_FALSE, transposedTransform.data );
#else
      glUniformMatrix4fv( uniformVar->location, 1, GL_TRUE, uniformValue->value.data );
#endif
    }
    else
    {
      AE_ASSERT_MSG( false, "Invalid uniform type '#': #", uniformVarName, uniformVar->type );
    }

    AE_CHECK_GL_ERROR();
  }
}

const aeShaderAttribute* aeShader::GetAttributeByIndex( uint32_t index ) const
{
  AE_ASSERT( index < m_attributeCount );
  return &m_attributes[ index ];
}

int aeShader::m_LoadShader( const char* shaderStr, aeShaderType::Type type, const char* const* defines, int32_t defineCount )
{
  GLenum glType = -1;
  if ( type == aeShaderType::Vertex )
  {
    glType = GL_VERTEX_SHADER;
  }
  if ( type == aeShaderType::Fragment )
  {
    glType = GL_FRAGMENT_SHADER;
  }
  
  const uint32_t kPrependMax = 16;
  uint32_t sourceCount = 0;
  const char* shaderSource[ kPrependMax + kMaxShaderDefines * 2 + 1 ]; // x2 max defines to make room for newlines. Plus one for actual shader.

  // Version
#if _AE_IOS_
  // shaderSource[ 0 ] = "#version 300 es\n";
#elif _AE_EMSCRIPTEN_
  // No version specified
#else
  shaderSource[ sourceCount++ ] = "#version 330 core\n";
#endif

  // Utility
  shaderSource[ sourceCount++ ] = "float AE_SRGB_TO_RGB( float _x ) { return pow( _x, 2.2 ); }\n";
  shaderSource[ sourceCount++ ] = "float AE_RGB_TO_SRGB( float _x ) { return pow( _x, 1.0 / 2.2 ); }\n";
  shaderSource[ sourceCount++ ] = "vec3 AE_SRGB_TO_RGB( vec3 _x ) { return vec3( AE_SRGB_TO_RGB( _x.r ), AE_SRGB_TO_RGB( _x.g ), AE_SRGB_TO_RGB( _x.b ) ); }\n";
  shaderSource[ sourceCount++ ] = "vec3 AE_RGB_TO_SRGB( vec3 _x ) { return vec3( AE_RGB_TO_SRGB( _x.r ), AE_RGB_TO_SRGB( _x.g ), AE_RGB_TO_SRGB( _x.b ) ); }\n";
  shaderSource[ sourceCount++ ] = "vec4 AE_SRGBA_TO_RGBA( vec4 _x ) { return vec4( AE_SRGB_TO_RGB( _x.rgb ), _x.a ); }\n";
  shaderSource[ sourceCount++ ] = "vec4 AE_RGBA_TO_SRGBA( vec4 _x ) { return vec4( AE_RGB_TO_SRGB( _x.rgb ), _x.a ); }\n";

  // Input/output
#if _AE_IOS_
  // shaderSource[ 1 ] = "precision highp float;\n";
  // shaderSource[ 2 ] = "precision mediump sampler3D;\n";
#elif _AE_EMSCRIPTEN_
  shaderSource[ sourceCount++ ] = "precision highp float;\n";
  shaderSource[ sourceCount++ ] = "#define AE_COLOR gl_FragColor\n";
  shaderSource[ sourceCount++ ] = "#define AE_TEXTURE2D texture2d\n";
  shaderSource[ sourceCount++ ] = "#define AE_UNIFORM_HIGHP uniform highp\n";
  // shaderSource[ sourceCount++ ] = "#define AE_FLOAT_HIGHP highp float\n";
  if ( type == aeShaderType::Vertex )
  {
    shaderSource[ sourceCount++ ] = "#define AE_IN_HIGHP attribute highp\n";
    shaderSource[ sourceCount++ ] = "#define AE_OUT_HIGHP varying highp\n";
  }
  else if ( type == aeShaderType::Fragment )
  {
    shaderSource[ sourceCount++ ] = "#define AE_IN_HIGHP varying highp\n";
    shaderSource[ sourceCount++ ] = "#define AE_UNIFORM_HIGHP uniform highp\n";
  }
#else
  shaderSource[ sourceCount++ ] = "#define AE_TEXTURE2D texture\n";
  shaderSource[ sourceCount++ ] = "#define AE_UNIFORM uniform\n";
  shaderSource[ sourceCount++ ] = "#define AE_UNIFORM_HIGHP uniform\n";
  shaderSource[ sourceCount++ ] = "#define AE_IN_HIGHP in\n";
  shaderSource[ sourceCount++ ] = "#define AE_OUT_HIGHP out\n";
  if ( type == aeShaderType::Fragment )
  {
    shaderSource[ sourceCount++ ] = "out vec4 AE_COLOR;\n";
  }
#endif
  AE_ASSERT( sourceCount <= kPrependMax );

  for ( int32_t i = 0; i < defineCount; i++ )
  {
    shaderSource[ sourceCount ] = defines[ i ];
    sourceCount++;
    shaderSource[ sourceCount ] = "\n";
    sourceCount++;
  }

  shaderSource[ sourceCount ] = shaderStr;
  sourceCount++;
  
  GLuint shader = glCreateShader( glType );
  glShaderSource( shader, sourceCount, shaderSource, nullptr );
  glCompileShader( shader );
  
  GLint status;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
  if(status == GL_FALSE)
  {
    GLint logLength;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
    
    if (logLength > 0)
    {
      unsigned char* log = new unsigned char[ logLength ];
      glGetShaderInfoLog(shader, logLength, NULL, (GLchar*)log);
      AE_LOG( "Error compiling shader #", log );
      delete[] log;
    }
    
    return 0;
  }
  
  AE_CHECK_GL_ERROR();
  return shader;
}

//------------------------------------------------------------------------------
// aeTexture2D member functions
//------------------------------------------------------------------------------
aeTexture2D::aeTexture2D()
{
  m_width = 0;
  m_height = 0;
  m_hasAlpha = false;
}

aeTexture2D::~aeTexture2D()
{
  Destroy();
}

void aeTexture2D::Initialize( const uint8_t* data, uint32_t width, uint32_t height, uint32_t depth, aeTextureFilter::Type filter, aeTextureWrap::Type wrap )
{
  m_width = width;
  m_height = height;
  m_target = GL_TEXTURE_2D;

  glGenTextures( 1, &m_texture );
  glBindTexture( m_target, m_texture );
  glTexParameteri( m_target, GL_TEXTURE_MIN_FILTER, ( filter == aeTextureFilter::Nearest ) ? GL_NEAREST : GL_LINEAR );
  glTexParameteri( m_target, GL_TEXTURE_MAG_FILTER, ( filter == aeTextureFilter::Nearest ) ? GL_NEAREST : GL_LINEAR );
  glTexParameteri( m_target, GL_TEXTURE_WRAP_S, ( wrap == aeTextureWrap::Clamp ) ? GL_CLAMP_TO_EDGE : GL_REPEAT );
  glTexParameteri( m_target, GL_TEXTURE_WRAP_T, ( wrap == aeTextureWrap::Clamp ) ? GL_CLAMP_TO_EDGE : GL_REPEAT );

  GLint internalFormat = 0;
  GLenum format = 0;
  GLint unpackAlignment;
  switch ( depth )
  {
    case 1:
      internalFormat = GL_RED;
      format = GL_RED;
      unpackAlignment = 1;
      m_hasAlpha = false;
      break;
    case 3:
      internalFormat = GL_RGB;
      format = GL_RGB;
      unpackAlignment = 1;
      m_hasAlpha = false;
      break;
    case 4:
      internalFormat = GL_RGBA;
      format = GL_RGBA;
      unpackAlignment = 4;
      m_hasAlpha = true;
      break;
    default:
      AE_ASSERT_MSG( false, "Invalid texture depth #", depth );
      return;
  }

  glPixelStorei( GL_UNPACK_ALIGNMENT, unpackAlignment );
  glTexImage2D( m_target, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data );
}

void aeTexture2D::Initialize( const char* file, aeTextureFilter::Type filter, aeTextureWrap::Type wrap )
{
  uint32_t fileSize = aeVfs::GetSize( file );
  AE_ASSERT_MSG( fileSize, "Could not load #", file );
  
  uint8_t* fileBuffer = (uint8_t*)malloc( fileSize );
  aeVfs::Read( file, fileBuffer, fileSize );

  int32_t width = 0;
  int32_t height = 0;
  int32_t channels = 0;
  stbi_set_flip_vertically_on_load( true );
  uint8_t* image = stbi_load_from_memory( fileBuffer, fileSize, &width, &height, &channels, STBI_default );
  AE_ASSERT( image );

  uint32_t depth = 0;
  switch ( channels )
  {
    case STBI_grey:
      depth = 1;
      break;
    case STBI_grey_alpha:
      AE_FAIL();
      break;
    case STBI_rgb:
      depth = 3;
      break;
    case STBI_rgb_alpha:
      depth = 4;
      break;
  }
  
  Initialize( image, width, height, depth, filter, wrap );
  
  stbi_image_free( image );
  free( fileBuffer );
}

void aeTexture2D::Destroy()
{
  glDeleteTextures( 1, &m_texture );

  m_width = 0;
  m_height = 0;
  m_hasAlpha = false;
}

//------------------------------------------------------------------------------
// aeRenderTexture member functions
//------------------------------------------------------------------------------
aeRenderTexture::aeRenderTexture( uint32_t texture, uint32_t target )
{
  m_texture = texture;
  m_target = target;
}

aeRenderTarget::aeRenderTarget()
{
  m_fbo = 0;

  m_width = 0;
  m_height = 0;
}

aeRenderTarget::~aeRenderTarget()
{
  Destroy();
}

void aeRenderTarget::Initialize( uint32_t width, uint32_t height )
{
  AE_ASSERT( m_fbo == 0 );

  AE_ASSERT( width != 0 );
  AE_ASSERT( height != 0 );

  m_width = width;
  m_height = height;

  glGenFramebuffers( 1, &m_fbo );
  AE_ASSERT( m_fbo );
  glBindFramebuffer( GL_FRAMEBUFFER, m_fbo );

  AE_CHECK_GL_ERROR();
  Vertex quadVerts[] =
  {
    { aeQuadVertPos[ 0 ], aeQuadVertUvs[ 0 ] },
    { aeQuadVertPos[ 1 ], aeQuadVertUvs[ 1 ] },
    { aeQuadVertPos[ 2 ], aeQuadVertUvs[ 2 ] },
    { aeQuadVertPos[ 3 ], aeQuadVertUvs[ 3 ] }
  };
  AE_STATIC_ASSERT( countof( quadVerts ) == aeQuadVertCount );
  m_quad.Initialize( sizeof( Vertex ), sizeof( aeQuadIndex ), aeQuadVertCount, aeQuadIndexCount, aeVertexPrimitive::Triangle, aeVertexUsage::Static, aeVertexUsage::Static );
  m_quad.AddAttribute( "a_position", 3, aeVertexDataType::Float, offsetof( Vertex, pos ) );
  m_quad.AddAttribute( "a_uv", 2, aeVertexDataType::Float, offsetof( Vertex, uv ) );
  m_quad.SetVertices( quadVerts, aeQuadVertCount );
  m_quad.SetIndices( aeQuadIndices, aeQuadIndexCount );
  AE_CHECK_GL_ERROR();

  // @TODO: Figure out if there are any implicit SRGB conversions happening here. Improve interface and visibility to user if there are.
  const char* vertexStr = "\
    AE_UNIFORM_HIGHP mat4 u_localToNdc;\
    AE_IN_HIGHP vec3 a_position;\
    AE_IN_HIGHP vec2 a_uv;\
    AE_OUT_HIGHP vec2 v_uv;\
    void main()\
    {\
      v_uv = a_uv;\
      gl_Position = u_localToNdc * vec4( a_position, 1.0 );\
    }";
  const char* fragStr = "\
    uniform sampler2D u_tex;\
    AE_IN_HIGHP vec2 v_uv;\
    void main()\
    {\
      AE_COLOR = AE_TEXTURE2D( u_tex, v_uv );\
    }";
  m_shader.Initialize( vertexStr, fragStr, nullptr, 0 );

  AE_CHECK_GL_ERROR();
}

void aeRenderTarget::Destroy()
{
  m_shader.Destroy();
  m_quad.Destroy();

  for ( uint32_t i = 0; i < m_targets.Length(); i++ )
  {
    uint32_t tex = m_targets[ i ].GetTexture();
    glDeleteTextures( 1, &tex );
  }
  m_targets.Clear();

  if ( m_depth.GetTexture() )
  {
    uint32_t tex = m_depth.GetTexture();
    glDeleteTextures( 1, &tex );
    m_depth = aeRenderTexture();
  }

  if ( m_fbo )
  {
    glDeleteFramebuffers( 1, &m_fbo );
    m_fbo = 0;
  }

  m_width = 0;
  m_height = 0;
}

void aeRenderTarget::AddTexture( aeTextureFilter::Type filter, aeTextureWrap::Type wrap )
{
  AE_ASSERT( m_targets.Length() < 16 );

  uint32_t texture = 0;
  uint32_t target = GL_TEXTURE_2D;
  GLenum attachement = GL_COLOR_ATTACHMENT0 + m_targets.Length();

  glGenTextures( 1, &texture );
  AE_ASSERT( texture );
  glBindTexture( target, texture );
  glTexParameteri( target, GL_TEXTURE_MIN_FILTER, ( filter == aeTextureFilter::Nearest ) ? GL_NEAREST : GL_LINEAR );
  glTexParameteri( target, GL_TEXTURE_MAG_FILTER, ( filter == aeTextureFilter::Nearest ) ? GL_NEAREST : GL_LINEAR );
  glTexParameteri( target, GL_TEXTURE_WRAP_S, ( wrap == aeTextureWrap::Clamp ) ? GL_CLAMP_TO_EDGE : GL_REPEAT );
  glTexParameteri( target, GL_TEXTURE_WRAP_T, ( wrap == aeTextureWrap::Clamp ) ? GL_CLAMP_TO_EDGE : GL_REPEAT );
  glTexImage2D( target, 0, GL_RGBA16F, m_width, m_height, 0, GL_RGBA, GL_HALF_FLOAT, nullptr );
  
  glBindFramebuffer( GL_FRAMEBUFFER, m_fbo );
  glFramebufferTexture2D( GL_FRAMEBUFFER, attachement, target, texture, 0 );

  m_targets.Append( aeRenderTexture( texture, target ) );

  AE_CHECK_GL_ERROR();
}

void aeRenderTarget::AddDepth( aeTextureFilter::Type filter, aeTextureWrap::Type wrap )
{
  uint32_t texture = 0;
  uint32_t target = GL_TEXTURE_2D;
  GLint depthFormat = GL_DEPTH_COMPONENT24;
  
  glGenTextures( 1, &texture );
  glBindTexture( target, texture );
  glTexParameteri( target, GL_TEXTURE_MIN_FILTER, ( filter == aeTextureFilter::Nearest ) ? GL_NEAREST : GL_LINEAR );
  glTexParameteri( target, GL_TEXTURE_MAG_FILTER, ( filter == aeTextureFilter::Nearest ) ? GL_NEAREST : GL_LINEAR );
  glTexParameteri( target, GL_TEXTURE_WRAP_S, ( wrap == aeTextureWrap::Clamp ) ? GL_CLAMP_TO_EDGE : GL_REPEAT );
  glTexParameteri( target, GL_TEXTURE_WRAP_T, ( wrap == aeTextureWrap::Clamp ) ? GL_CLAMP_TO_EDGE : GL_REPEAT );
  glTexImage2D( target, 0, depthFormat, m_width, m_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr );

  glBindFramebuffer( GL_FRAMEBUFFER, m_fbo );
  glFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, target, texture, 0 );

  m_depth = aeRenderTexture( texture, target );

  AE_CHECK_GL_ERROR();
}

void aeRenderTarget::Activate()
{
  CheckFramebufferComplete( m_fbo );
  glBindFramebuffer( GL_DRAW_FRAMEBUFFER, m_fbo );
  
  GLenum buffers[] =
  {
    GL_COLOR_ATTACHMENT0_EXT,
    GL_COLOR_ATTACHMENT1_EXT,
    GL_COLOR_ATTACHMENT2_EXT,
    GL_COLOR_ATTACHMENT3_EXT,
    GL_COLOR_ATTACHMENT4_EXT,
    GL_COLOR_ATTACHMENT5_EXT,
    GL_COLOR_ATTACHMENT6_EXT,
    GL_COLOR_ATTACHMENT7_EXT,
    GL_COLOR_ATTACHMENT8_EXT,
    GL_COLOR_ATTACHMENT9_EXT,
    GL_COLOR_ATTACHMENT10_EXT,
    GL_COLOR_ATTACHMENT11_EXT,
    GL_COLOR_ATTACHMENT12_EXT,
    GL_COLOR_ATTACHMENT13_EXT,
    GL_COLOR_ATTACHMENT14_EXT,
    GL_COLOR_ATTACHMENT15_EXT
  };
  glDrawBuffers( m_targets.Length(), buffers );

  glViewport( 0, 0, GetWidth(), GetHeight() );
}

void aeRenderTarget::Clear( aeColor color )
{
  Activate();

  AE_CHECK_GL_ERROR();

  aeFloat3 clearColor = color.GetSRGB(); // Unclear why glClearColor() expects srgb. Maybe because of framebuffer type.
  glClearColor( clearColor.x, clearColor.y, clearColor.z, 1.0f );
  glClearDepth( 1.0f );

  glDepthMask( GL_TRUE );
  glDisable( GL_DEPTH_TEST );
  glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

  AE_CHECK_GL_ERROR();
}

void aeRenderTarget::Render2D( uint32_t textureIndex, aeRect ndc, float z )
{
  glBindFramebuffer( GL_READ_FRAMEBUFFER, m_fbo );

  aeUniformList uniforms;
  uniforms.Set( "u_localToNdc", aeRenderTarget::GetQuadToNDCTransform( ndc, z ) );
  uniforms.Set( "u_tex", GetTexture( textureIndex ) );
  m_quad.Render( &m_shader, uniforms );
}

const aeRenderTexture* aeRenderTarget::GetTexture( uint32_t index ) const
{
  return &m_targets[ index ];
}

uint32_t aeRenderTarget::GetWidth() const
{
  return m_width;
}

uint32_t aeRenderTarget::GetHeight() const
{
  return m_height;
}

aeFloat4x4 aeRenderTarget::GetQuadToNDCTransform( aeRect ndc, float z )
{
  aeFloat4x4 localToNdc = aeFloat4x4::Translation( aeFloat3( ndc.x, ndc.y, z ) );
  localToNdc.Scale( aeFloat3( ndc.w, ndc.h, 1.0f ) );
  localToNdc.Translate( aeFloat3( 0.5f, 0.5f, 0.0f ) );
  return localToNdc;
}

//------------------------------------------------------------------------------
// aeOpenGLRender member functions
//------------------------------------------------------------------------------
aeOpenGLRender::aeOpenGLRender()
{
  m_context = nullptr;
  m_defaultFbo = 0;
}

void aeOpenGLRender::Initialize( aeRender* render )
{
  SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
  SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 3 );
  SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );
#if _AE_DEBUG_
  SDL_GL_SetAttribute( SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG );
#endif
  SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
  SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 24 );

  m_context = SDL_GL_CreateContext( (SDL_Window*)render->GetWindow()->window );
  SDL_GL_MakeCurrent( (SDL_Window*)render->GetWindow()->window, m_context );

  SDL_GL_SetSwapInterval( 1 );

#if _AE_WINDOWS_
  glewExperimental = GL_TRUE;
  GLenum err = glewInit();
  glGetError(); // Glew currently has an issue which causes a GL_INVALID_ENUM on init
  AE_ASSERT_MSG( err == GLEW_OK, "Could not initialize glew" );
#endif

#if _AE_DEBUG_
  glDebugMessageCallback( aeOpenGLDebugCallback, nullptr );
#endif

  glGetIntegerv( GL_FRAMEBUFFER_BINDING, &m_defaultFbo );

  AE_CHECK_GL_ERROR();
}

void aeOpenGLRender::Terminate( aeRender* render )
{
  SDL_GL_DeleteContext( m_context );
}

void aeOpenGLRender::StartFrame( aeRender* render )
{}

void aeOpenGLRender::EndFrame( aeRender* render )
{
  AE_CHECK_GL_ERROR();

  glBindFramebuffer( GL_DRAW_FRAMEBUFFER, m_defaultFbo );
  glViewport( 0, 0, render->GetWindow()->GetWidth(), render->GetWindow()->GetHeight() );

  // Clear window target in case canvas doesn't fit exactly
  glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
  glClearDepth( 1.0f );

  glDepthMask( GL_TRUE );
  glDisable( GL_DEPTH_TEST );
  glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
  AE_CHECK_GL_ERROR();

  render->GetCanvas()->Render2D( 0, render->GetNDCRect(), 0.5f );

#if !_AE_EMSCRIPTEN_
  SDL_GL_SwapWindow( (SDL_Window*)render->GetWindow()->window );
#endif

  AE_CHECK_GL_ERROR();
}
