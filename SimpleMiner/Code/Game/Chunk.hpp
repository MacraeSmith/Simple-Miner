#pragma once
#include "Game/GameCommon.hpp"
#include "Game/Block.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Math/AABB3.hpp"
#include <atomic>

class World;
class VertexBuffer;
class IndexBuffer;
class SpriteSheet;
struct Vec3;
struct IntVec3;
struct Frustum;
class Timer;
class Texture;
struct BlockDefinition;
class BlockIterator;
enum class BlockDirection : int;

enum class TreeType : int;

struct FileHeader
{
	uint8_t m_gchk[4] = {'G', 'C', 'H', 'K'};
	uint8_t m_version = CHUNK_VERSION;
	uint8_t m_bitsX = CHUNK_BITS_X;
	uint8_t m_bitsY = CHUNK_BITS_Y;
	uint8_t m_bitsZ = CHUNK_BITS_Z;
};

struct FileRun
{
	uint8_t m_blockType;
	uint8_t m_runLength;
};

enum class ChunkState : int
{
	INACTIVE = -1,

	PENDING_GENERATION,
	GENERATING,

	PENDING_LOAD,
	LOADING,

	ACTIVE,

	PENDING_SAVE,
	SAVING,
	SAVED,

	GARBAGE,

	COUNT,
};

class Chunk
{
friend class World;
public:
	explicit Chunk(World* world, IntVec2 const& chunkCoords);
	~Chunk();
	void Update();
	void Render(Frustum const& cameraViewFrustrum) const;
	void RenderWater(Frustum const& cameraViewFrustrum) const;

	//Job Functions
	bool TryCreateBlocksFromFile();
	void CreateProceduralBlocks();
	void SaveChunkToDisk();


	IntVec3 GetBlockGlobalCoordsFromIndex(int index) const;
private:

	void InitNeighbors();
	void ClearNeighbors();
	Chunk* GetNeighbor(IntVec3 const& direction) const;
	bool HasAllNeighbors() const;
	bool AllNeighborsFullyActivated() const;

	bool GrowTreeAtGlobalCoords(IntVec3 const& globalCoords, TreeType treeType);

	//Lighting
	void ActivateLightingData();
	void MarkBorderLightBlocksDirty(BlockDirection side);
	void ThreadProcessLightData();

	//Liquid
	void ActivateLiquidData();
	void MarkBorderLiquidBlocksDirty(BlockDirection side);
	void ThreadProcessLiquidData();

	//Chunk Mesh
	void CreateTerrainMesh();
	bool AddBlock(int blockIndex, uint8_t blockType);
	bool RemoveBlock(int blockIndex);
	void SetDirty();
	void Regenerate();


	//Helpers
	bool IsChunkInFrustum(Frustum frustrum) const;
	Vec2 GetCenterXYCoords() const;
	int GetBlockIndexFromLocalCoords(IntVec3 const& blockCoords) const;
	int GetBlockIndexFromLocalCoords(Vec3 const& blockCoords) const;
	int GetBlockIndexFromGlobalCoords(Vec3 const& coords) const;
	int GetBlockIndexFromGlobalCoords(IntVec3 const& coords) const;
	IntVec3 GetBlockLocalCoordsFromIndex(int index) const;
	bool AreLocalCoordsInChunk(IntVec3 const& coords) const;
	bool AreGlobalCoordsInChunk(IntVec3 const& coords) const;

	bool ShouldRenderFace(BlockDefinition const* currentDef, Block const& block, BlockIterator const& neighborIterator, bool& out_faceIsWater, float& out_neighborFlowFrac, bool testFlowValue = false, bool isUpFace = false) const;
	void GetMaxIndoorAndOutdoorLightFromNeighbors(uint8_t& out_indoorLight, uint8_t& out_outdoorLight, BlockIterator const& iter);

public:
	World* m_world = nullptr;
	IntVec2 m_chunkCoords = IntVec2::ZERO;
	AABB3 m_chunkBounds;
	Block m_blocks[NUM_BLOCKS_IN_CHUNK] = {};
	bool m_isDirty = false;
	bool m_needsSaving = false;

	VertexBuffer* m_vbo = nullptr;
	IndexBuffer* m_ibo = nullptr;

	VertexBuffer* m_waterVbo = nullptr;
	IndexBuffer* m_waterIbo = nullptr;

	Verts m_debugVerts;
	Chunk* m_neighbors[4] = {};
	std::atomic<ChunkState> m_state = ChunkState::INACTIVE;

private:
	float m_frustrumBoundsRadius = 0.f;
	Timer* m_animateTimer = nullptr;
	bool m_isAnimatingIn = false;
	bool m_hasAnimatedIn = false;
	bool m_hasActivatedLighting = false;
	bool m_hasActivatedLiquid = false;
	double m_timeDirtied = 0.0;
	double m_timeCreated = 0.0;

};

