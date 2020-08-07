//------------------------------------------------------------------------------
// aeTerrain.h
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
#ifndef AETERRAIN_H
#define AETERRAIN_H

//------------------------------------------------------------------------------
// Headers
//------------------------------------------------------------------------------
#include <cstdint>
//#include "Vector2.h"
//#include "Vector3.h"
//#include "Matrix4.h"
//#include "Noise.h"
//#include "float.h"
//#include "PoolAllocator.h"
//#include "Utils.h"
//#include "VertexData.h"
#include "aeCompactingAllocator.h"
#include "aeMath.h"
#include "aeObjectPool.h"
#include "aeRender.h"

typedef float float16_t;
#define PACK( _ae_something ) _ae_something

static const uint32_t kChunkSize = 16;
static const uint32_t kChunkCountMax = kChunkSize * kChunkSize * kChunkSize;
static const uint32_t kWorldChunksWidth = 128;
static const uint32_t kWorldChunksHeight = 32;
static const uint32_t kWorldMaxWidth = kWorldChunksWidth * kChunkSize;
static const uint32_t kWorldMaxHeight = kWorldChunksHeight * kChunkSize;
static const uint32_t kMaxActiveChunks = 128;
static const uint32_t kMaxLoadedChunks = kMaxActiveChunks * 2;
static const uint32_t kMaxChunkVerts = kChunkSize * kChunkSize * kChunkSize;
static const uint32_t kMaxChunkIndices = kChunkSize * kChunkSize * kChunkSize * 6;
static const float16_t kSkyBrightness = float16_t( 5.0f );

struct Block
{
  enum Type
  {
    Exterior,
    Interior,
    Surface,
    Blocking,
    Unloaded,
    COUNT
  };

  static const char* Name[ Block::COUNT ];
};

PACK( struct TerrainVertex
{
  aeFloat3 position;
  aeFloat3 normal;
  uint8_t info[ 4 ];
});
typedef uint16_t TerrainIndex;

struct RaycastResult
{
  bool hit;
  uint8_t type;
  float distance;
  int32_t posi[ 3 ];
  aeFloat3 posf;
  aeFloat3 normal;
  bool touchedUnloaded;
};

struct EdgeCompact
{
  uint8_t f;
  int8_t nx;
  int8_t ny;
  int8_t nz;
};

struct DCInfo
{
  uint16_t edgeFlags;
  EdgeCompact edges[ 12 ];
};

struct Chunk
{
  uint32_t check;
  int32_t pos[ 3 ];
  bool active;
  bool lightDirty;
  aeVertexData data;
  TerrainVertex* vertices;
  Chunk* next;
  Chunk* prev;
  
  uint8_t t[ kChunkSize ][ kChunkSize ][ kChunkSize ];
  float16_t l[ kChunkSize ][ kChunkSize ][ kChunkSize ];
  TerrainIndex i[ kChunkSize ][ kChunkSize ][ kChunkSize ];
};

class TerrainBoss
{
public:
  void Initialize( class ProgramInterface* program );
  void RenderInitialize( class ProgramInterface* program );
  void Update( class ProgramInterface* program );
  void BuildScene( class ProgramInterface* program );
  uint8_t GetVoxel( uint32_t x, uint32_t y, uint32_t z );
  uint8_t GetVoxel( aeFloat3 position );
  bool GetCollision( uint32_t x, uint32_t y, uint32_t z );
  bool GetCollision( aeFloat3 position );
  float16_t GetLight( uint32_t x, uint32_t y, uint32_t z );
  Chunk* GetChunk( int32_t cx, int32_t cy, int32_t cz );
  void VoxelizeAndAddMesh(
    aeFloat3* vertices, uint16_t* indices,
    uint32_t vertexCount, uint32_t indexCount,
    aeFloat4x4& modelToWorld, Block::Type type );
  bool Loaded() const { return m_loaded; }
  
  bool VoxelRaycast( aeFloat3 start, aeFloat3 ray, int32_t minSteps );
  RaycastResult RaycastFast( aeFloat3 start, aeFloat3 ray, bool allowSourceCollision );
  RaycastResult Raycast( aeFloat3 start, aeFloat3 ray );
  
  float GetBaseHeight( aeFloat3 p ) const;
  float TerrainValue( aeFloat3 p ) const;
private:
  inline int32_t TerrainType( aeFloat3 p ) const;
  void GetChunkVerts( Chunk* chunk, TerrainVertex *vertices, TerrainIndex *indices, uint32_t* vertexCount, uint32_t* indexCount );
  void UpdateChunkLighting( Chunk* chunk );
  void UpdateChunkLightingHelper( Chunk *chunk, uint32_t x, uint32_t y, uint32_t z, float16_t l );
  Chunk* AllocChunk( int32_t cx, int32_t cy, int32_t cz );
  void FreeChunk( Chunk* chunk );
  static void GetOffsetsFromEdge( uint32_t edgeBit, int32_t (&offsets)[ 4 ][ 3 ] );
  
  aeCompactingAllocator m_compactAlloc;
  struct Chunk **m_chunks;
  int16_t* m_voxelCounts;
  public: struct Chunk *m_activeChunks[ kMaxActiveChunks ];
  aeObjectPool<struct Chunk, kMaxLoadedChunks> m_chunkPool;
  uint32_t m_activeChunkCount;
  aeShader* m_shader;
  aeShader* m_leavesShader;
  bool m_loaded;
  
  struct Chunk* m_headChunk;
  struct Chunk* m_tailChunk;
  uint32_t m_totalChunks;
  uint8_t* m_chunkRawAlloc;
  
  bool m_blockCollision[ Block::COUNT ];
  float16_t m_blockDensity[ Block::COUNT ];
  //UMAT::Noise m_noise;
  ProgramInterface* m_program;
  
  aeTexture2D* grassTexture;
  aeTexture2D* rockTexture;
  aeTexture2D* dirtTexture;
  aeTexture2D* treeTexture;
  aeTexture2D* leavesTexture;
  
public:
  aeTexture2D* spiralTexture;
  aeTexture2D* m_mapWallTex;
  aeTexture2D* m_mapFloorHeightTex;
};

#endif
