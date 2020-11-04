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
#include <atomic>
#include "aeCompactingAllocator.h"
#include "aeImage.h"
#include "aeList.h"
#include "aeMath.h"
#include "aeObjectPool.h"
#include "aeRender.h"
#include <map>

namespace ctpl
{
  class thread_pool;
}

//------------------------------------------------------------------------------
// Constants
//------------------------------------------------------------------------------
typedef float float16_t;
typedef uint8_t aeTerrainMaterialId;
#define PACK( _ae_something ) _ae_something

const uint32_t kChunkSize = 64;
const int32_t kTempChunkSize = kChunkSize + 2; // Include a 1 voxel border
const int32_t kTempChunkSize3 = kTempChunkSize * kTempChunkSize * kTempChunkSize; // Temp voxel count
const uint32_t kChunkCountMax = kChunkSize * kChunkSize * kChunkSize;
const uint32_t kMaxActiveChunks = 512;
const uint32_t kMaxLoadedChunks = kMaxActiveChunks * 2;
const uint32_t kMaxChunkVerts = kChunkSize * kChunkSize * kChunkSize;
const uint32_t kMaxChunkIndices = kChunkSize * kChunkSize * kChunkSize * 6;
const uint32_t kMaxChunkAllocationsPerTick = 1;
const float16_t kSkyBrightness = float16_t( 5.0f );
const float kSdfBoundary = 2.0f;

struct Block
{
  enum Type : uint8_t
  {
    Exterior,
    Interior,
    Surface,
    Blocking,
    Unloaded,
    COUNT
  };
};

//------------------------------------------------------------------------------
// TerrainVertex
//------------------------------------------------------------------------------
PACK( struct TerrainVertex
{
  aeFloat3 position;
  aeFloat3 normal;
  uint8_t materials[ 4 ];
  uint8_t info[ 4 ];
});
typedef uint16_t TerrainIndex;
const TerrainIndex kInvalidTerrainIndex = ~0;

struct RaycastResult
{
  bool hit;
  Block::Type type;
  float distance;
  aeInt3 posi;
  aeFloat3 posf;
  aeFloat3 normal;
  bool touchedUnloaded;

  const struct aeTerrainChunk* chunk;
  TerrainIndex index;
};

struct EdgeCompact
{
  uint8_t f;
  int8_t nx;
  int8_t ny;
  int8_t nz;
};

float aeUnion( float d1, float d2 );
float aeSubtraction( float d1, float d2 );
float aeIntersection( float d1, float d2 );
float aeSmoothUnion( float d1, float d2, float k );
float aeSmoothSubtraction( float d1, float d2, float k );

//------------------------------------------------------------------------------
// ae::Sdf
//------------------------------------------------------------------------------
namespace ae { namespace Sdf {

//------------------------------------------------------------------------------
// Shape class
//------------------------------------------------------------------------------
class Shape
{
public:
  Shape();

  aeAABB GetAABB() const { return m_aabb; }
  aeOBB GetOBB() const { return aeOBB( m_localToWorld ); }
  
  void SetTransform( const aeFloat4x4& transform );
  const aeFloat4x4& GetTransform() const { return m_localToWorld; }
  aeFloat3 GetHalfSize() const { return m_halfSize; }

  void Dirty() { m_dirty = true; } // Must be be explicitly called if object is modified after creation

  virtual float GetValue( aeFloat3 p ) const = 0;

  enum class Type
  {
    Union,
    Subtraction,
    SmoothUnion,
    SmoothSubtraction,
    Material
  };
  Type type = Type::Union;
  aeTerrainMaterialId materialId = 0;
  float smoothing = 0.0f; // Works with SmoothUnion and SmoothSubtraction types

protected:
  const aeFloat4x4& GetWorldToScaled() const { return m_worldToScaled; }

private:
  aeAABB m_aabb;
  aeFloat3 m_halfSize;
  aeFloat4x4 m_localToWorld;
  aeFloat4x4 m_worldToScaled;

public:
  // Internal
  bool m_dirty = false;
  aeAABB m_aabbPrev;
};

//------------------------------------------------------------------------------
// Box class
//------------------------------------------------------------------------------
class Box : public Shape
{
public:
  float GetValue( aeFloat3 p ) const override;

  float cornerRadius = 0.0f;
};

//------------------------------------------------------------------------------
// Cylinder class
//------------------------------------------------------------------------------
class Cylinder : public Shape
{
public:
  float GetValue( aeFloat3 p ) const override;

  // Valid range is 0-1, are multiplied by obb size
  float top = 1.0f;
	float bottom = 1.0f;
};

//------------------------------------------------------------------------------
// Heightmap class
//------------------------------------------------------------------------------
class Heightmap : public Shape
{
public:
  void SetImage( ae::Image* heightMap ) { m_heightMap = heightMap; }
  float GetValue( aeFloat3 p ) const override;

private:
  ae::Image* m_heightMap = nullptr;
};

} } // ae::Sdf

//------------------------------------------------------------------------------
// aeTerrainSDF class
//------------------------------------------------------------------------------
class aeTerrainSDF
{
public:
  aeTerrainSDF( class aeTerrain* terrain );

  template < typename T >
  T* CreateSdf();
  void DestroySdf( ae::Sdf::Shape* sdf );

  void UpdatePending();
  bool HasPending() const;
  void RenderDebug( aeDebugRender* debug );

  // @NOTE: This will be called from multiple threads simultaneously and so must be const
  float GetValue( aeFloat3 pos ) const;
  aeFloat3 GetDerivative( aeFloat3 pos ) const;
  aeTerrainMaterialId GetMaterial( aeFloat3 pos ) const;

private:
  friend class aeTerrain;

  class aeTerrain* m_terrain;
  aeArray< ae::Sdf::Shape* > m_shapes;
  aeArray< ae::Sdf::Shape* > m_shapesPrev;
  aeArray< ae::Sdf::Shape* > m_pendingCreated;
  aeArray< ae::Sdf::Shape* > m_pendingDestroy;

  // Internal use by aeTerrain
  uint32_t GetShapeCount() const { return m_shapes.Length(); }
  ae::Sdf::Shape* GetShapeAtIndex( uint32_t index ) const { return m_shapes[ index ]; }
};

template < typename T >
T* aeTerrainSDF::CreateSdf()
{
  ae::Sdf::Shape* sdf = aeAlloc::Allocate< T >();
  m_pendingCreated.Append( sdf );
  return static_cast< T* >( sdf );
}

//------------------------------------------------------------------------------
// aeTerrainSDFCache class
//------------------------------------------------------------------------------
class aeTerrainSDFCache
{
public:
  aeTerrainSDFCache();
  ~aeTerrainSDFCache();

  void Generate( aeInt3 chunk, const aeTerrainSDF* sdf );
  float GetValue( aeFloat3 pos ) const;
  float GetValue( aeInt3 pos ) const;
  aeFloat3 GetDerivative( aeFloat3 p ) const;
  uint8_t GetMaterial( aeFloat3 pos ) const;

private:
  float m_GetValue( aeInt3 pos ) const;

  const aeTerrainSDF* m_sdf;

  const int32_t kDim = kChunkSize + 5; // TODO: What should this value actually be? Corresponds to chunkPlus
  static const int32_t kOffset = 2;
  
  aeInt3 m_chunk;
  aeInt3 m_offseti; // Pre-computed chunk integer offset
  aeFloat3 m_offsetf; // Pre-computed chunk float offset

  float16_t* m_values;
};

//------------------------------------------------------------------------------
// aeTerrainJob class
//------------------------------------------------------------------------------
class aeTerrainJob
{
public:
  aeTerrainJob();
  ~aeTerrainJob();
  void StartNew( const aeTerrainSDF* sdf, struct aeTerrainChunk* chunk );
  void Do();
  void Finish();

  bool HasJob() const { return m_hasJob; }
  bool HasChunk( aeInt3 pos ) const;
  bool IsPendingFinish() const { return m_hasJob && !m_running; }

  const aeTerrainChunk* GetChunk() const { return m_chunk; }
  aeTerrainChunk* GetChunk() { return m_chunk; }
  const TerrainVertex* GetVertices() const { return m_vertexCount ? &m_vertices[ 0 ] : nullptr; }
  const TerrainIndex* GetIndices() const { return m_indexCount ? &m_indices[ 0 ] : nullptr; }
  uint32_t GetVertexCount() const { return m_vertexCount; }
  uint32_t GetIndexCount() const { return m_indexCount; }

private:
  // Management
  bool m_hasJob;
  std::atomic_bool m_running;

  // Input
  const aeTerrainSDF* m_sdf;
  struct aeTerrainChunk* m_chunk;

  // Pre-computed sdf
  aeTerrainSDFCache m_sdfCache;

  // Output
  uint32_t m_vertexCount;
  uint32_t m_indexCount;
  aeArray< TerrainVertex > m_vertices;
  aeArray< TerrainIndex > m_indices;

public:
  // Temp edges (pre-allocate edges for all future jobs)
  struct TempEdges
  {
    int32_t x;
    int32_t y;
    int32_t z;
    uint16_t b;

    // 3 planes which whose intersections are used to position vertices within voxel
    // EDGE_TOP_FRONT_BIT
    // EDGE_TOP_RIGHT_BIT
    // EDGE_SIDE_FRONTRIGHT_BIT
    aeFloat3 p[ 3 ];
    aeFloat3 n[ 3 ];
  };

  TempEdges* edgeInfo;
};

//------------------------------------------------------------------------------
// Terrain Chunk class
//------------------------------------------------------------------------------
struct aeTerrainChunk
{
  aeTerrainChunk();
  ~aeTerrainChunk();

  static uint32_t GetIndex( aeInt3 pos );
  static void GetPosFromWorld( aeInt3 pos, aeInt3* chunkPos, aeInt3* localPos );

  uint32_t GetIndex() const;
  void Generate( const aeTerrainSDFCache* sdf, aeTerrainJob::TempEdges* edgeBuffer, TerrainVertex* verticesOut, TerrainIndex* indexOut, uint32_t* vertexCountOut, uint32_t* indexCountOut );

  aeAABB GetAABB() const;
  static aeAABB GetAABB( aeInt3 chunkPos );

  uint32_t m_check;
  aeInt3 m_pos;
  bool m_geoDirty;
  bool m_lightDirty;
  aeVertexData m_data;
  TerrainVertex* m_vertices;
  aeListNode< aeTerrainChunk > m_generatedList;
  
  Block::Type m_t[ kChunkSize ][ kChunkSize ][ kChunkSize ];
  float16_t m_l[ kChunkSize ][ kChunkSize ][ kChunkSize ];
  TerrainIndex m_i[ kChunkSize ][ kChunkSize ][ kChunkSize ];

private:
  static void m_GetQuadVertexOffsetsFromEdge( uint32_t edgeBit, int32_t( &offsets )[ 4 ][ 3 ] );
};

//------------------------------------------------------------------------------
// aeTerrain class
//------------------------------------------------------------------------------
class aeTerrain
{
public:
  aeTerrain();
  ~aeTerrain();

  void Initialize( uint32_t maxThreads, bool render );
  void Terminate();
  void Update( aeFloat3 center, float radius );
  void Render( const class aeShader* shader, const aeUniformList& shaderParams );
  void SetDebug( class aeDebugRender* debug );

  void SetDebugTextCallback( std::function< void( aeFloat3, const char* ) > fn ) { m_debugTextFn = fn; }
  
  Block::Type GetVoxel( int32_t x, int32_t y, int32_t z ) const;
  Block::Type GetVoxel( aeFloat3 position ) const;
  bool GetCollision( int32_t x, int32_t y, int32_t z ) const;
  bool GetCollision( aeFloat3 position ) const;
  float16_t GetLight( int32_t x, int32_t y, int32_t z ) const;

  aeTerrainChunk* GetChunk( uint32_t chunkIndex );
  aeTerrainChunk* GetChunk( aeInt3 pos );
  const aeTerrainChunk* GetChunk( uint32_t chunkIndex ) const;
  const aeTerrainChunk* GetChunk( aeInt3 pos ) const;
  int32_t GetVoxelCount( uint32_t chunkIndex ) const;
  int32_t GetVoxelCount( aeInt3 pos ) const;

  bool VoxelRaycast( aeFloat3 start, aeFloat3 ray, int32_t minSteps ) const;
  RaycastResult RaycastFast( aeFloat3 start, aeFloat3 ray, bool allowSourceCollision ) const;
  RaycastResult Raycast( aeFloat3 start, aeFloat3 ray ) const;
  // @NOTE: SweepSphere returns true on collision and writes out the following:
  // distanceOut is distance traveled before collision
  // normalOut is ground normal
  // posOut is ground collision point
  bool SweepSphere( aeSphere sphere, aeFloat3 ray, float* distanceOut = nullptr, aeFloat3* normalOut = nullptr, aeFloat3* posOut = nullptr ) const;
  bool PushOutSphere( aeSphere sphere, aeFloat3* offsetOut = nullptr, class aeDebugRender* debug = nullptr ) const;
  
  aeTerrainSDF sdf;

private:
  const TerrainVertex* m_GetVertex( int32_t x, int32_t y, int32_t z ) const;
  void UpdateChunkLighting( aeTerrainChunk* chunk );
  
  aeTerrainChunk* AllocChunk( aeInt3 pos );
  void FreeChunk( aeTerrainChunk* chunk );
  void m_SetVoxelCount( uint32_t chunkIndex, int32_t count );
  float GetChunkScore( aeInt3 pos ) const;

  aeDebugRender* m_debug = nullptr;

  bool m_render = false;
  aeFloat3 m_center = aeFloat3( 0.0f );
  float m_radius = 0.0f;
  
  //aeCompactingAllocator m_compactAlloc;
  std::map< uint32_t, struct aeTerrainChunk* > m_chunks3;
  std::map< uint32_t, int16_t > m_voxelCounts3; // Kept even when chunks are freed so they are not regenerated again if they are empty
  //aeMap<>
  aeObjectPool< aeTerrainChunk, kMaxLoadedChunks > m_chunkPool;

  // Keep these across frames instead of allocating temporary space for each generated chunk
  struct ChunkSort
  {
    aeTerrainChunk* c;
    aeInt3 pos;
    float score;
  };
  //aeMap< aeInt3, ChunkSort > t_chunkMap;
  std::map< uint32_t, ChunkSort > t_chunkMap_hack;
  aeArray< ChunkSort > t_chunkSorts;

  aeList< aeTerrainChunk > m_generatedList;
  
  bool m_blockCollision[ Block::COUNT ];
  float16_t m_blockDensity[ Block::COUNT ];
  
  ctpl::thread_pool* m_threadPool = nullptr;
  aeArray< aeTerrainJob* > m_terrainJobs;

  std::function< void( aeFloat3, const char* ) > m_debugTextFn;

public:
  // Internal
  void m_Dirty( aeAABB aabb );
};

#endif
