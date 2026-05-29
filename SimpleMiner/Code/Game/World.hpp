#pragma once
#include "Game/BlockIterator.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Core/JobSystem.hpp"
#include "Engine/Math/IntVec3.hpp"

#include <vector>
#include <unordered_map>
#include <set>

class WorldDefinition;
class Chunk;
struct Vec3;
struct NoiseValues;
class ConstantBuffer;
class Clock;

namespace std
{
	template<>
	struct hash<IntVec2>
	{
		size_t operator()(IntVec2 const& v) const noexcept
		{
			size_t h1 = std::hash<int>()(v.x);
			size_t h2 = std::hash<int>()(v.y);
			return h1 ^ (h2 + 0x9e3779b97f4a7c15ULL + (h1 << 6) + (h1 >> 2));
		}
	};
}

namespace std
{
	template<>
	struct hash<IntVec3>
	{
		size_t operator()(IntVec3 const& v) const noexcept
		{
			size_t h1 = std::hash<int>()(v.x);
			size_t h2 = std::hash<int>()(v.y);
			size_t h3 = std::hash<int>()(v.z);
			size_t seed = h1;
			seed ^= h2 + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
			seed ^= h3 + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
			return seed;
		}
	};
}

class ChunkGenerateJob : public Job
{
public:
	ChunkGenerateJob(Chunk* chunk);
	virtual void Execute() override;
	Chunk* m_chunk = nullptr;
	double m_timeStarted = 0.0;
};

class ChunkLoadJob : public Job
{
public:
	ChunkLoadJob(Chunk* chunk);
	virtual void Execute() override;
	Chunk* m_chunk = nullptr;
	double m_timeStarted = 0.0;
};

class ChunkSaveJob : public Job
{
public:
	ChunkSaveJob(Chunk* chunk) { m_chunk = chunk; }
	virtual void Execute() override;
	Chunk* m_chunk = nullptr;
};

enum class ChunkJobType : int
{
	GENERATE,
	LOAD,
	SAVE,
	COUNT,
};

struct BlockRaycastResult3D : public RaycastResult3D
{
	BlockIterator m_blockIterator = BlockIterator(nullptr, -1);
	IntVec3 m_blockGlobalCoords;
	float m_impactFraction = 1.f;
};

struct DelayedDirtyLiquidBlock
{
	BlockIterator m_blockIterator = BlockIterator(nullptr, -1);
	float m_timeAdded = 0.0f;
};

struct WorldConstants
{
	float m_indoorLightColor[4] = {};
	float m_outdoorLightColor[4] = {};
	float m_skyColor[4] = {};
	float m_fogNearDistance = 0;
	float m_fogFarDistance = 0;
	int m_underWater = 0;
	float m_dayTime = 0.5f;
};


class World
{
public:
	World(WorldDefinition const* worldDef);
	~World();
	void ShutDown();
	void AttractScreenUpdate(double timeUpdateStarted);
	void Update(double timeUpdateStarted);
	void Render() const;

	//Helpers
	IntVec2 GetChunkCoordsFromPosition(Vec3 const& pos) const;
	Chunk* GetChunkFromChunkCoords(IntVec2 const& chunkCoords) const;
	Vec2	GetGlobalCenterPosFromChunkCoords(IntVec2 const& chunkCoords) const;
	bool	DoesChunkExistAtPosition(Vec3 const& pos) const;

	float GetBaseTerrainDensityAtPosition(Vec3 const& pos) const;
	float GetCavesNoiseAtPosition(Vec3 const& pos, int terrainHeight, float surfacePierceNoise, float slope) const;
	float GetHeightOffsetFromNoiseValues(NoiseValues const& values) const;
	float GetSquashFactorFromNoiseValues(NoiseValues const& values) const;
	bool IsPositionWater(Vec3 const& position) const;
	BlockIterator GetBlockIteratorAtPosition(Vec3 const& position) const;
	NoiseValues Get2DNoiseValuesAtPosition(Vec2 const& pos);

	void RemoveBlock(BlockIterator blockIterator);
	void AddBlock(BlockIterator blockIterator, uint8_t blockType);

	BlockRaycastResult3D RaycastVsWorld(Vec3 const& startPos, Vec3 const& fwrdNormal, float maxLength);

	void MarkLightingDirty(BlockIterator iterator);
	void UndirtyLightForAllBlocksInChunk(Chunk* chunk);

	void MarkLiquidDirty(BlockIterator iterator, float timeAdded = -1.f);
	void UndirtyLiquidForAllBlocksInChunk(Chunk* chunk);

	Rgba8 GetSkyColor() const;
	void AdjustPlayerStartingPosition();
private:
	void CheckKeyboardControls();
	void CheckControllerControls();
	void ProcessDebugMessages();
	void UpdateWorldTimeAndLighting();

	//Chunk Management
	void ProcessPendingSaveChunks();
	void ProcessPendingDeleteChunks(bool overrideMax = false);
	void ProcessChunkLoading();
	void ProcessDirtyLighting();
	void ProcessDirtyLiquid();
	void ProcessPendingActivation();
	void CreateAllChunksInActivationRadius();
	void DeactivateAllChunksOutsideRadius();
	void ActivateChunk(Chunk* chunk);
	void DeleteChunk(Chunk* chunk);
	bool RegenerateDirtyChunks(double timeUpdateStarted);


	void AddNewChunkAtCoords(IntVec2 const& coords);
	void AddChunkCoordsPair(IntVec2 const& key, Chunk* value);

	//Job Management
	void ExecuteChunkJob(Chunk* chunk, ChunkJobType jobType);
	void ProcessFinishedChunkJobs();


	//Helpers
	bool DoesChunkExist(IntVec2 const& chunkCoords);
	bool DoesChunkExist(IntVec2 const& chunkCoords, Chunk*& out_foundChunk);
	bool IsActiveChunk(IntVec2 const& chunkCoords);
	bool IsActiveChunk(IntVec2 const& chunkCoords, Chunk*& out_foundChunk);
	bool IsChunkPending(Chunk* chunk) const;

	Rgba8 GetOutdoorLightColor() const;
	float GetTimeOfDay() const;


public:
	std::unordered_map<IntVec2, Chunk*> m_dirtyChunks;
	WorldDefinition const* m_worldDef = nullptr;
	std::atomic<bool> m_isShuttingDown = false;

private:
	std::vector<Chunk*> m_activeChunks;
	std::set<Chunk*> m_pendingChunks;
	std::deque<Chunk*> m_needsSavingChunks;
	std::deque<Chunk*> m_pendingDeleteChunks;
	std::unordered_map<IntVec2, Chunk*> m_pendingActivationChunks;

	std::deque<BlockIterator> m_dirtyLightBlocks;
	std::deque<DelayedDirtyLiquidBlock> m_dirtyLiquidBlocks;

	int m_numDirtyChunks = 0;
	std::unordered_map<IntVec2, Chunk*> m_coordsChunkPair;

	bool m_loadChunks = true;

	bool m_showJobInfo = false;
	int m_numSentJobs = 0;
	int m_numInFlightLoad = 0;
	int m_numInFlightGenerate = 0;
	int m_numInFlightSave = 0;

	bool m_shouldReload = false;

	WorldConstants m_worldConstants;
	ConstantBuffer* m_worldConstantBuffer = nullptr;

	double m_avgChunkLoadSpeed = 0.0;
	double m_maxChunkLoadSpeed = 0.0;
	double m_totalChunkLoadSpeed = 0.0;
	int m_totalLoadJobs = 0;

	double m_avgChunkGenerateSpeed = 0.0;
	double m_maxChunkGenerateSpeed = 0.0;
	double m_totalChunkGenerateSpeed = 0.0;
	int m_totalGenerateJobs = 0;

	double m_avgActivationSpeed = 0.0;
	double m_maxActivationSpeed = 0.0;
	double m_totalActivationSpeed = 0.0;
	int m_totalActivations = 0;

	double m_avgMeshWaitSpeed = 0.0;
	double m_maxMeshWaitSpeed = 0.0;
	double m_totalMeshWaitSpeed = 0.0;
	double m_totalMeshWaits = 0;

	double m_processLightSpeed = 0.0;
	double m_avgProcessLightSpeed = 0.0;
	double m_totalProcessLightSpeed = 0.0;
	double m_maxProcessLightSpeed = 0.0;
	int m_totalLightProcess = 0;

	double m_generateMeshSpeed = 0.0;
	double m_avgGenerateMeshSpeed = 0.0;
	double m_totalGenerateMeshSpeed = 0.0;
	double m_maxGenerateMeshSpeed = 0.0;
	int m_totalGenerateMesh = 0;

	double m_processLiquidSpeed = 0.0;
	double m_avgProcessLiquidSpeed = 0.0;
	double m_totalProcessLiquidSpeed = 0.0;
	double m_maxProcessLiquidSpeed = 0.0;
	int m_totalLiquidProcess = 0;

	bool m_useDayCycle = true;
	float m_worldTimeDays = 0.5f;
	float m_lightningStrength = 0.f;
};

