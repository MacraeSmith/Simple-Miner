#pragma once
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Core/XmlUtils.hpp"
#include "Engine/Math/IntVec3.hpp"
#include "Engine/Math/IntRange.hpp"
#include <string>
#include <vector>

class SpriteSheet;
class Shader;
class Texture;
class VertexBuffer;
class IndexBuffer;

template <typename T>
class PieceWiseCurve;
template <typename T>
class Curve;

template <typename T>
struct CurveWithTStart;

enum class WorldNoiseType : int
{
	BASE_TERRAIN,
	CONTINENTALNESS,
	EROSION,
	PEAKS_VALLEYS,
	TEMPERATURE,
	ICE_MELT,
	HUMIDITY,
	VEGETATION,
	WILD_GRASS,
	WATERFALLS,
	NOISE_FLUCTUATION,
	TUNNEL_SURFACE_PIERCE,
	CAVES,
	TUNNELS,
	COUNT,
};

enum class NoiseType : int 
{
	RAW_ZERO_TO_ONE,
	RAW_NEG_ONE_TO_ONE,
	FRACTAL,
	PERLIN,
	RIDGED,
	COUNT
};

struct NoiseValues
{
	float m_noiseValues[(int)WorldNoiseType::COUNT] = {};
};

struct NoiseInfo
{
	bool m_isActive = true;
	float m_scale = 100.f;
	int m_numOctaves = 2;
	int m_seed = 0;
	float m_warpStrength = 0.f;
	float m_ridgeGain = 0.5f;
	PieceWiseCurve<float>* m_heightOffsetCurve = nullptr;
	PieceWiseCurve<float>* m_squashCurve = nullptr;
	NoiseType m_noiseType = NoiseType::FRACTAL;
};

enum class BiomeType : int
{
	GENERAL,
	OCEAN,
	DEEP_OCEAN,
	FROZEN_OCEAN,
	BEACH,
	SNOWY_BEACH,
	DESERT,
	SAVANNA,
	PLAINS,
	SNOWY_PLAINS,
	FOREST,
	JUNGLE,
	TAIGA,
	SNOWY_TAIGA,
	STONY_PEAKS,
	SNOWY_PEAKS,
	COUNT
};

enum class LandType : int 
{
	DEEP_OCEAN,
	OCEAN,
	COAST,
	NEAR_INLAND,
	MID_INLAND,
	FAR_INLAND,
	COUNT
};

enum class PeakType : int
{
	VALLEY,
	LOW,
	MID,
	HIGH,
	PEAK,
	COUNT,
};

enum class TreeType : int
{
	NONE = -1,
	CACTUS,
	ACACIA,
	OAK,
	SMALL_OAK,
	LEAFLESS_OAK,
	BIRCH,
	JUNGLE,
	JUNGLE_BRUSH,
	MEGA_JUNGLE,
	SPRUCE,
	SNOWY_SPRUCE,
	COUNT,
};

struct TreeBlock
{
	IntVec3 m_offsetFromBase;
	uint8_t m_blockType;
};


struct TreeStamp
{
	uint8_t m_logType = 0;
	uint8_t m_leavesType = 0;
	IntRange m_heightRange = IntRange(0,0);
	int m_baseHeight = 1;
	std::vector<TreeBlock> m_treeBlocks;
};


class WorldDefinition
{
public:
	
	std::string m_name;
	SpriteSheet* m_blockSpriteSheet = nullptr;
	SpriteSheet* m_blockIconSpriteSheet = nullptr;
	Texture* m_waterTexture = nullptr;
	Texture* m_hudTexture = nullptr;
	Texture* m_selectedBlockTexture = nullptr;
	Shader* m_chunkShader = nullptr;
	Shader* m_waterShader = nullptr;
	Shader* m_underWaterShader = nullptr;
	Shader* m_underWaterSurfaceShader = nullptr;
	int m_gameSeed = 0;
	float m_defaultOctavePersistance = 0.5f;
	float m_defaultOctaveScale = 2.f;
	float m_baseTerrainSquashFactor = 0.5f;
	float m_finalTerrainSquashFactor = 0.05f;
	int m_defaultTerrainHeight = 0;
	int m_terrainMinHeight = 0;

	int m_cavesStartDepth = 0;
	int m_cavesFade = 10;

	int m_seaHeight = 50;
	bool m_activeTrees = true;
	bool m_fadeLeaves = true;
	bool m_fillWater = true;

	VertexBuffer* m_tntVbo = nullptr;
	IndexBuffer* m_tntIbo = nullptr;

	NoiseInfo m_noiseInfos[(int)WorldNoiseType::COUNT] = {};
	TreeStamp m_treeStamps[(int)TreeType::COUNT] = {};

	static std::vector<WorldDefinition> s_worldDefinitions;

	explicit WorldDefinition(XmlElement const& worldDefElement);
	~WorldDefinition();

	void CreateTntBuffers();
	void ParseNoiseInfo(XmlElement const& worldDefElement);
	void ParseTreeInfo(XmlElement const& worldDefElement);
	NoiseInfo GetNoiseInfoFromType(WorldNoiseType type) const;

	CurveWithTStart<float> GetCurveAndTPlotFromXMLElement(XmlElement const& curveElement, Vec2& out_startPlotPoint) const;
	static void InitWorldDefinitionsFromFile(std::string const& filePath);

	static TreeType GetTreeTypeFromName(std::string const& name);

	static LandType	GetLandTypeFromTerrainHeight(int terrainHeight, int seaHeight, float noiseFluctuation);
	static PeakType GetPeakTypeFromPeaksAndValleys(float pv, float noiseFluctuation);
	static int GetErosionSectorFromErosion(float erosion, float noiseFluctuation);
	static int GetTemperatureSectorFromTemperature(float temperature, float noiseFluctuation);
	static int GetHumiditySectorFromHumidity(float humidity, float noiseFluctuation);

	static BiomeType GetBiomeTypeFrom2DNoiseValues(NoiseValues const& values, int terrainHeight, int seaHeight, float noiseFluctuation);
	static BiomeType GetBeachBiome(int temperature);
	static BiomeType GetBadlandBiomes(int humidity);
	static BiomeType GetMiddleBiomes(int temperature, int humidity);

	static uint8_t GetSurfaceBlockTypeFromBiome(BiomeType biome, int seaHeight, int terrainHeight, float temperature);
	static uint8_t GetWaterBlockTypeFromBiome(BiomeType biome, int seaHeight, int blockHeight, float temperature, float iceMeltNoise);
	static void	   GetDirtLayersFromBiome(BiomeType biome, std::vector<uint8_t>& out_dirtTypes, std::vector<IntRange>& out_dirtDepths);
	static std::vector<uint8_t> GetWildGrassFromBiome(BiomeType biome);
	static bool DoesBiomeHaveDirtDepthLayer(BiomeType biome);
	static bool DoesBiomeHaveTrees(BiomeType biome, std::vector<TreeType>& out_treeType);
	static float GetMinVegetationSpawnNoiseFromBiome(BiomeType biome);
	static float GetMinWildGrassSpawnNoiseFromBiome(BiomeType biome);
	static float GetMinWaterFallSpawnNoiseFromBiome(BiomeType biome);
};

