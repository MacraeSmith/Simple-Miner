#include "Game/WorldDefinition.hpp"
#include "Game/GameCommon.hpp"
#include "Game/BlockDefinition.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"
#include "Engine/Renderer/RendererDX11.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/Curves.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Renderer/IndexBuffer.hpp"

std::vector<WorldDefinition> WorldDefinition::s_worldDefinitions;

void WorldDefinition::InitWorldDefinitionsFromFile(std::string const& filePath)
{
	XmlDocument worldDefXml;
	XmlResult result = worldDefXml.LoadFile(filePath.c_str());
	GUARANTEE_OR_DIE(result == tinyxml2::XML_SUCCESS, Stringf("Failed to open world definition document: %s", filePath.c_str()));

	XmlElement* rootElement = worldDefXml.RootElement();
	GUARANTEE_OR_DIE(rootElement, Stringf("Failed to find root element in world def xml: %s", filePath.c_str()));

	XmlElement* worldDefElement = rootElement->FirstChildElement();
	while (worldDefElement != nullptr)
	{
		std::string elementName = worldDefElement->Name();
		GUARANTEE_OR_DIE(elementName == "WorldDefinition", Stringf("Root chiled element in %s was <%s>, must be <WorldDefinition>", filePath.c_str(), elementName.c_str()));
		WorldDefinition* newWorldDef = new WorldDefinition(*worldDefElement);
		s_worldDefinitions.push_back(*newWorldDef);
		worldDefElement = worldDefElement->NextSiblingElement();
	}
}

WorldDefinition::WorldDefinition(XmlElement const& worldDefElement)
{
	m_name = ParseXmlAttribute(worldDefElement, "name", "UNAMED_WORLD");
	m_seaHeight = ParseXmlAttribute(worldDefElement, "seaHeight", m_seaHeight);
	m_fillWater = ParseXmlAttribute(worldDefElement, "fillWater", m_fillWater);

	std::string spriteSheetFilePath = ParseXmlAttribute(worldDefElement, "spriteSheet", "INVALID_TEXTURE_PATH");
	IntVec2 spriteSheetDims = ParseXmlAttribute(worldDefElement, "spriteSheetDimensions", IntVec2::ONE);
	int numMipLevels = ParseXmlAttribute(worldDefElement, "mipLevels", 5);
	Texture* spriteTex = g_renderer->CreateOrGetMipMapTextureFromFile(spriteSheetFilePath.c_str(), numMipLevels);
	m_blockSpriteSheet = new SpriteSheet(*spriteTex, spriteSheetDims);

	spriteSheetDims = ParseXmlAttribute(worldDefElement, "blockSpriteSheetDimensions", IntVec2::ONE);
	spriteSheetFilePath = ParseXmlAttribute(worldDefElement, "blockIconsSpriteSheet", "INVALID_TEXTURE_PATH");
	spriteTex = g_renderer->CreateOrGetTextureFromFile(spriteSheetFilePath.c_str());
	m_blockIconSpriteSheet = new SpriteSheet(*spriteTex, spriteSheetDims);


	std::string waterTexFilePath = ParseXmlAttribute(worldDefElement, "waterTexture", "INVALID_TEXTURE_PATH");
	m_waterTexture = g_renderer->CreateOrGetMipMapTextureFromFile(waterTexFilePath.c_str(), numMipLevels);

	std::string hudTexFilePath = ParseXmlAttribute(worldDefElement, "hudTexture", "INVALID_TEXTURE_PATH");
	m_hudTexture = g_renderer->CreateOrGetTextureFromFile(hudTexFilePath.c_str());

	std::string selectedBlockFilePath = ParseXmlAttribute(worldDefElement, "selectedBlockTexture", "InvalidTexturePath");
	m_selectedBlockTexture = g_renderer->CreateOrGetTextureFromFile(selectedBlockFilePath.c_str());

	std::string shaderFilePath = ParseXmlAttribute(worldDefElement, "chunkShader", "INVALID_SHADER_PATH");
	m_chunkShader = g_renderer->CreateOrGetShaderFromFile(shaderFilePath.c_str());

	shaderFilePath = ParseXmlAttribute(worldDefElement, "waterShader", "INVALID_SHADER_PATH");
	m_waterShader = g_renderer->CreateOrGetShaderFromFile(shaderFilePath.c_str());

	shaderFilePath = ParseXmlAttribute(worldDefElement, "underWaterShader", "INVALID_SHADER_PATH");
	m_underWaterShader = g_renderer->CreateOrGetShaderFromFile(shaderFilePath.c_str());

	shaderFilePath = ParseXmlAttribute(worldDefElement, "underWaterSurfaceShader", "INVALID_SHADER_PATH");
	m_underWaterSurfaceShader = g_renderer->CreateOrGetShaderFromFile(shaderFilePath.c_str());

	ParseNoiseInfo(worldDefElement);
	ParseTreeInfo(worldDefElement);
	CreateTntBuffers();
}

WorldDefinition::~WorldDefinition()
{
	for (int i = 0; i < (int)WorldNoiseType::COUNT; ++i)
	{
		NoiseInfo& noiseInfo = m_noiseInfos[i];
		delete noiseInfo.m_heightOffsetCurve;
		noiseInfo.m_heightOffsetCurve = nullptr;

		delete noiseInfo.m_squashCurve;
		noiseInfo.m_squashCurve = nullptr;

	}

	delete m_tntVbo;
	m_tntVbo = nullptr;

	delete m_tntIbo;
	m_tntIbo = nullptr;
}

void WorldDefinition::CreateTntBuffers()
{

	Verts verts;
	IndexList indexes;
	uint8_t TNT_TYPE = BlockDefinition::GetBlockDefinitionIndexFromName("Tnt");
	BlockDefinition* blockDef = BlockDefinition::GetBlockDefinitionFromIndex(TNT_TYPE);
	AABB2 topUvs = m_blockSpriteSheet->GetSpriteUVs(blockDef->m_topSpriteCoords);
	AABB2 sideUvs = m_blockSpriteSheet->GetSpriteUVs(blockDef->m_sideSpriteCoords);
	AABB2 bottomUvs = m_blockSpriteSheet->GetSpriteUVs(blockDef->m_bottomSpriteCoords);

	float boxRadius = TNT_BOX_RADIUS;
	float boxHeight = TNT_BOX_RADIUS * 2.f;

	// BOTTOM FACE (z = 0)
	{
		Vec3 bl = Vec3(-boxRadius, -boxRadius, 0.f);
		Vec3 br = Vec3(boxRadius, -boxRadius, 0.f);
		Vec3 tr = Vec3(boxRadius, boxRadius, 0.f);
		Vec3 tl = Vec3(-boxRadius, boxRadius, 0.f);

		// FIXED winding: bl, br, tr, tl
		AddVertsForIndexedQuad3D(verts, indexes, br, bl, tl, tr, Rgba8::WHITE, bottomUvs);
	}

	// TOP FACE (z = 1)
	{
		Vec3 bl = Vec3(-boxRadius, -boxRadius, boxHeight);
		Vec3 br = Vec3(boxRadius, -boxRadius, boxHeight);
		Vec3 tr = Vec3(boxRadius, boxRadius, boxHeight);
		Vec3 tl = Vec3(-boxRadius, boxRadius, boxHeight);

		AddVertsForIndexedQuad3D(verts, indexes, bl, br, tr, tl, Rgba8::WHITE, topUvs);
	}

	// FRONT FACE (-Y)
	{
		Vec3 bl = Vec3(-boxRadius, -boxRadius, 0.f);
		Vec3 br = Vec3(boxRadius, -boxRadius, 0.f);
		Vec3 tr = Vec3(boxRadius, -boxRadius, boxHeight);
		Vec3 tl = Vec3(-boxRadius, -boxRadius, boxHeight);

		AddVertsForIndexedQuad3D(verts, indexes, bl, br, tr, tl, Rgba8::WHITE, sideUvs);
	}

	// BACK FACE (+Y)
	{
		Vec3 bl = Vec3(boxRadius, boxRadius, 0.f);
		Vec3 br = Vec3(-boxRadius, boxRadius, 0.f);
		Vec3 tr = Vec3(-boxRadius, boxRadius, boxHeight);
		Vec3 tl = Vec3(boxRadius, boxRadius, boxHeight);

		AddVertsForIndexedQuad3D(verts, indexes, bl, br, tr, tl, Rgba8::WHITE, sideUvs);
	}

	// RIGHT FACE (+X)
	{
		Vec3 bl = Vec3(boxRadius, -boxRadius, 0.f);
		Vec3 br = Vec3(boxRadius, boxRadius, 0.f);
		Vec3 tr = Vec3(boxRadius, boxRadius, boxHeight);
		Vec3 tl = Vec3(boxRadius, -boxRadius, boxHeight);

		AddVertsForIndexedQuad3D(verts, indexes, bl, br, tr, tl, Rgba8::WHITE, sideUvs);
	}

	// LEFT FACE (-X)
	{
		Vec3 bl = Vec3(-boxRadius, boxRadius, 0.f);
		Vec3 br = Vec3(-boxRadius, -boxRadius, 0.f);
		Vec3 tr = Vec3(-boxRadius, -boxRadius, boxHeight);
		Vec3 tl = Vec3(-boxRadius, boxRadius, boxHeight);

		AddVertsForIndexedQuad3D(verts, indexes, bl, br, tr, tl, Rgba8::WHITE, sideUvs);
	}

	m_tntVbo = g_renderer->CreateVertexBuffer(sizeof(Vertex_PCU) * (int)verts.size(), sizeof(Vertex_PCU));
	m_tntIbo = g_renderer->CreateIndexBuffer(sizeof(unsigned int) * (unsigned int)indexes.size());

	g_renderer->CopyCPUToGPU(verts.data(), (unsigned int)(verts.size() * sizeof(Vertex_PCU)), m_tntVbo);
	g_renderer->CopyCPUToGPU(indexes.data(), (unsigned int)(indexes.size() * sizeof(unsigned int)), m_tntIbo);

}

void WorldDefinition::ParseNoiseInfo(XmlElement const& worldDefElement)
{
	XmlElement const* noiseRoot = worldDefElement.FirstChildElement("Noise");
	GUARANTEE_OR_DIE(noiseRoot, "Could not find noise root element");

	m_gameSeed = ParseXmlAttribute(*noiseRoot, "gameSeed", 0);
	m_defaultOctavePersistance = ParseXmlAttribute(*noiseRoot, "defaultOctavePersistance", 0.5f);
	m_defaultOctaveScale = ParseXmlAttribute(*noiseRoot, "defaultOctaveScale", 2.f);
	m_baseTerrainSquashFactor = ParseXmlAttribute(*noiseRoot, "baseTerrainSquashFactor", 0.5f);
	m_finalTerrainSquashFactor = ParseXmlAttribute(*noiseRoot, "finalTerrainSquashFactor", 0.05f);
	m_defaultTerrainHeight = ParseXmlAttribute(*noiseRoot, "defaultTerrainHeight", 0);
	m_terrainMinHeight = ParseXmlAttribute(*noiseRoot, "terrainMinHeight", 0);
	m_cavesStartDepth = ParseXmlAttribute(*noiseRoot, "cavesStartDepth", m_cavesStartDepth);
	m_cavesFade = ParseXmlAttribute(*noiseRoot, "cavesFade", m_cavesFade);

	XmlElement const* noiseInfoElement = noiseRoot->FirstChildElement("NoiseInfo");
	int noiseInfoNum = 0;
	while (noiseInfoElement)
	{
		NoiseInfo& noiseInfo = m_noiseInfos[noiseInfoNum];
		std::string noiseTypeName = ParseXmlAttribute(*noiseInfoElement, "type", "fractal");
		if(noiseTypeName == "rawZeroToOne")
			noiseInfo.m_noiseType = NoiseType::RAW_ZERO_TO_ONE;
		if (noiseTypeName == "negOneToOne")
			noiseInfo.m_noiseType = NoiseType::RAW_NEG_ONE_TO_ONE;
		if (noiseTypeName == "fractal")
			noiseInfo.m_noiseType = NoiseType::FRACTAL;
		if (noiseTypeName == "perlin")
			noiseInfo.m_noiseType = NoiseType::PERLIN;
		if (noiseTypeName == "ridged")
			noiseInfo.m_noiseType = NoiseType::RIDGED;

		noiseInfo.m_scale = ParseXmlAttribute(*noiseInfoElement, "scale", 1.f);
		noiseInfo.m_numOctaves = ParseXmlAttribute(*noiseInfoElement, "numOctaves", 2);
		noiseInfo.m_seed = m_gameSeed + ParseXmlAttribute(*noiseInfoElement, "seed", 0);
		noiseInfo.m_warpStrength = ParseXmlAttribute(*noiseInfoElement, "warpStrength", 0.f);
		noiseInfo.m_isActive = ParseXmlAttribute(*noiseInfoElement, "isActive", true);
		noiseInfo.m_ridgeGain = ParseXmlAttribute(*noiseInfoElement, "ridgeGain", 0.5f);

		XmlElement const* curveElement = noiseInfoElement->FirstChildElement("Curve");
		while (curveElement)
		{
			std::vector<CurveWithTStart<float>> curvesToAdd;
			XmlElement const* plotPointElement = curveElement->FirstChildElement("PlotPoint");
			if(plotPointElement == nullptr)
				continue;

			Vec2 curveStart = ParseXmlAttribute(*plotPointElement, "point", Vec2::ZERO);
			plotPointElement = plotPointElement->NextSiblingElement("PlotPoint");
			while (plotPointElement)
			{
				CurveWithTStart<float> curve = GetCurveAndTPlotFromXMLElement(*plotPointElement, curveStart);
				curvesToAdd.push_back(curve);
				plotPointElement = plotPointElement->NextSiblingElement("PlotPoint");
			}

			
			std::string curveTypeName = ParseXmlAttribute(*curveElement, "type", "IVALID_CURVE");

			if (curveTypeName == "heightOffset")
			{
				noiseInfo.m_heightOffsetCurve = new PieceWiseCurve(curvesToAdd, 1.f);
			}

			else if (curveTypeName == "squashCurve")
			{
				noiseInfo.m_squashCurve = new PieceWiseCurve(curvesToAdd, 1.f);
			}

			else
			{
				ERROR_AND_DIE(Stringf("Invalid Curve type : %s", curveTypeName.c_str()));
			}

			curveElement = curveElement->NextSiblingElement("Curve");
		}

		noiseInfoElement = noiseInfoElement->NextSiblingElement("NoiseInfo");
		noiseInfoNum++;
	}
}

TreeType WorldDefinition::GetTreeTypeFromName(std::string const& name)
{
	if(name == "Cactus")
		return TreeType::CACTUS;
	if(name == "Acacia")
		return TreeType::ACACIA;
	if(name == "Oak")
		return TreeType::OAK;
	if(name == "Small Oak")
		return TreeType::SMALL_OAK;
	if(name == "Leafless Oak")
		return TreeType::LEAFLESS_OAK;
	if(name == "Birch")
		return TreeType::BIRCH;
	if(name == "Jungle")
		return TreeType::JUNGLE;
	if(name == "Jungle Brush")
		return TreeType::JUNGLE_BRUSH;
	if (name == "Mega Jungle")
		return TreeType::MEGA_JUNGLE;
	if(name == "Spruce")
		return TreeType::SPRUCE;
	if(name == "Snowy Spruce")
		return TreeType::SNOWY_SPRUCE;

	return TreeType::NONE;
}

void WorldDefinition::ParseTreeInfo(XmlElement const& worldDefElement)
{
	XmlElement const* treesRoot = worldDefElement.FirstChildElement("Trees");
	GUARANTEE_OR_DIE(treesRoot, "Could not find 'Trees' element");

	m_fadeLeaves = ParseXmlAttribute(*treesRoot, "fadeLeaves", false);
	m_activeTrees = ParseXmlAttribute(*treesRoot, "isActive", true);

	XmlElement const* treeStampElement = treesRoot->FirstChildElement("TreeStamp");
	while (treeStampElement)
	{
		std::string treeName = ParseXmlAttribute(*treeStampElement, "name", "INVALID_NAME");
		TreeType treeType = GetTreeTypeFromName(treeName);
		if (treeType == TreeType::NONE)
		{
			treeStampElement = treeStampElement->NextSiblingElement("TreeStamp");
			continue;
		}

		TreeStamp& stamp = m_treeStamps[(int)treeType];

		stamp.m_heightRange = ParseXmlAttribute(*treeStampElement, "heightRange", IntRange(0,0));
		stamp.m_baseHeight = ParseXmlAttribute(*treeStampElement, "treeHeight", 1);
		std::string logName = ParseXmlAttribute(*treeStampElement, "log", "INVALID_NAME");
		std::string leavesName = ParseXmlAttribute(*treeStampElement, "leaves", "INVALID_NAME");
		stamp.m_logType = BlockDefinition::GetBlockDefinitionIndexFromName(logName);
		stamp.m_leavesType = BlockDefinition::GetBlockDefinitionIndexFromName(leavesName);

		XmlElement const* blockElement = treeStampElement->FirstChildElement("Block");
		while (blockElement)
		{
			IntVec3 offset = ParseXmlAttribute(*blockElement, "offset", IntVec3::ZERO);
			uint8_t blockType = ParseXmlAttribute(*blockElement, "type", "log") == "log" ? stamp.m_logType : stamp.m_leavesType;
			stamp.m_treeBlocks.push_back({offset, blockType});
			blockElement = blockElement->NextSiblingElement("Block");
		}

		treeStampElement = treeStampElement->NextSiblingElement("TreeStamp");
	}
}

NoiseInfo WorldDefinition::GetNoiseInfoFromType(WorldNoiseType type) const
{
	return m_noiseInfos[(int)type];
}

CurveWithTStart<float> WorldDefinition::GetCurveAndTPlotFromXMLElement(XmlElement const& curveElement, Vec2& out_startPlotPoint) const
{
	std::string curveType = ParseXmlAttribute(curveElement, "type", "linear");
	Vec2 curveStart = out_startPlotPoint;
	Vec2 curveEnd = ParseXmlAttribute(curveElement, "point", Vec2(1.f, 0.f));
	int exponent = ParseXmlAttribute(curveElement, "exponent", 1);
	out_startPlotPoint = curveEnd;

	if (curveType == "linear")
		return CurveWithTStart(new LinearCurve<float>(curveStart.y, curveEnd.y), curveStart.x);
	if (curveType == "smoothStep3")
		return CurveWithTStart(new SmoothStep3Curve<float>(curveStart.y, curveEnd.y), curveStart.x);
	if (curveType == "smoothStep5")
		return CurveWithTStart(new SmoothStep5Curve<float>(curveStart.y, curveEnd.y), curveStart.x);
	if (curveType == "smoothStart")
		return CurveWithTStart(new SmoothStartCurve<float>(curveStart.y, curveEnd.y, exponent), curveStart.x);
	if (curveType == "smoothStop")
		return CurveWithTStart(new SmoothStopCurve<float>(curveStart.y, curveEnd.y, exponent), curveStart.x);
	if (curveType == "hesitate3")
		return CurveWithTStart(new Hesitate3Curve<float>(curveStart.y, curveEnd.y), curveStart.x);
	if (curveType == "hesitate5")
		return CurveWithTStart(new Hesitate5Curve<float>(curveStart.y, curveEnd.y), curveStart.x);
	if (curveType == "easeInBack")
		return CurveWithTStart(new EaseInBackCurve<float>(curveStart.y, curveEnd.y), curveStart.x);
	if (curveType == "easeOutBack")
		return CurveWithTStart(new EaseOutBackCurve<float>(curveStart.y, curveEnd.y), curveStart.x);

	ERROR_AND_DIE("Could not create curve");
}

/*
LandType WorldDefinition::GetLandTypeFromContinentalness(float continentalness)
{
	if(continentalness <= -0.455f)
		return LandType::DEEP_OCEAN;

	if(continentalness <= -0.19f)
		return LandType::OCEAN;

	if(continentalness <= -0.11f)
		return LandType::COAST;

	if(continentalness <= 0.03f)
		return LandType::NEAR_INLAND;

	if(continentalness <= 0.3f)
		return LandType::MID_INLAND;

	return LandType::FAR_INLAND;
}
*/

LandType WorldDefinition::GetLandTypeFromTerrainHeight(int terrainHeight, int seaHeight, float noiseFluctuation)
{
	float terrainHeightF = (float)terrainHeight + (noiseFluctuation * -2.f);
	float inlandFrac = GetFractionWithinRange(terrainHeightF, (float)seaHeight - 1.f, (float)CHUNK_SIZE_Z);

	if (inlandFrac <= 0.f)
	{
		float seaFrac = GetFractionWithinRange(terrainHeightF, 0.f, (float)seaHeight);
		if(seaFrac < 0.5f)
			return LandType::DEEP_OCEAN;

		return LandType::OCEAN;
	}

	if(inlandFrac < 0.025f)
		return LandType::COAST;

	if (inlandFrac < 0.55f)
		return LandType::NEAR_INLAND;

	if (inlandFrac < 0.85f)
		return LandType::MID_INLAND;

	return LandType::FAR_INLAND;
}

int WorldDefinition::GetErosionSectorFromErosion(float erosion, float noiseFluctuation)
{
	float adjustedErosion = erosion + (noiseFluctuation * 0.01f);
	if(adjustedErosion <= 0.78f)
		return 0;

	if(adjustedErosion <= -0.375f)
		return 1;

	if(adjustedErosion <= -0.2225f)
		return 2;

	if(adjustedErosion <= 0.05f)
		return 3;

	if(adjustedErosion <= 0.4f)
		return 4;

	if(adjustedErosion < 0.55f)
		return 5;

	return 6;
}


BiomeType WorldDefinition::GetBiomeTypeFrom2DNoiseValues(NoiseValues const& values, int terrainHeight, int seaHeight, float noiseFluctuation)
{
	//peaks and valleys
	PeakType peakType = GetPeakTypeFromPeaksAndValleys(values.m_noiseValues[(int)WorldNoiseType::PEAKS_VALLEYS], noiseFluctuation);
	int erosion = GetErosionSectorFromErosion(values.m_noiseValues[(int)WorldNoiseType::EROSION], noiseFluctuation);
	int temperature = GetTemperatureSectorFromTemperature(values.m_noiseValues[(int)WorldNoiseType::TEMPERATURE], noiseFluctuation);
	int humidity = GetHumiditySectorFromHumidity(values.m_noiseValues[(int)WorldNoiseType::HUMIDITY], noiseFluctuation);

	LandType landType = GetLandTypeFromTerrainHeight(terrainHeight, seaHeight, noiseFluctuation);

	//Oceans
	if (landType == LandType::OCEAN)
		return BiomeType::OCEAN;

	else if (landType == LandType::DEEP_OCEAN)
		return BiomeType::DEEP_OCEAN;



	//Valleys
	if (peakType == PeakType::VALLEY)
	{
		if (landType == LandType::COAST)
			return GetBeachBiome(temperature);

		else if (temperature < 4)
			return GetMiddleBiomes(temperature, humidity);

		return GetBadlandBiomes(humidity);
	}

	//low PV
	else if (peakType == PeakType::LOW)
	{
		if (landType == LandType::COAST)
			return GetBeachBiome(temperature);

		if (erosion < 4)
		{
			if (landType == LandType::NEAR_INLAND)
				return GetMiddleBiomes(temperature, humidity);

			if (temperature < 4)
				return GetMiddleBiomes(temperature, humidity);

			else
				return GetBadlandBiomes(humidity);
		}

		return GetMiddleBiomes(temperature, humidity);
	}

	//middle PV
	else if (peakType == PeakType::MID)
	{
		if (landType == LandType::COAST)
		{
			if (erosion < 3)
				return GetBeachBiome(temperature);

			if (erosion < 5)
				return GetMiddleBiomes(temperature, humidity);

			return GetBeachBiome(temperature);
		}

		else if (erosion < 2)
		{
			if (temperature < 4)
				return GetMiddleBiomes(temperature, humidity);

			return GetBadlandBiomes(humidity);
		}

		else if (erosion == 2 && landType == LandType::MID_INLAND)
		{
			if (temperature < 4)
				return GetMiddleBiomes(temperature, humidity);
			else
				return GetBadlandBiomes(humidity);
		}


		else if (erosion == 3 && (landType == LandType::MID_INLAND || landType == LandType::FAR_INLAND))
		{
			if (temperature < 4)
				return GetMiddleBiomes(temperature, humidity);
			else
				return GetBadlandBiomes(humidity);
		}


		return GetMiddleBiomes(temperature, humidity);

	}

	//high PV
	else if (peakType == PeakType::HIGH)
	{
		if (landType == LandType::MID_INLAND || landType == LandType::FAR_INLAND)
		{
			if (erosion == 0)
			{
				if (temperature <= 2)
					return BiomeType::SNOWY_PEAKS;
				else
					return BiomeType::STONY_PEAKS;
			}

			if (landType == LandType::MID_INLAND && erosion == 3)
			{
				if (temperature < 4)
					return GetMiddleBiomes(temperature, humidity);
				else
					return GetBadlandBiomes(humidity);
			}
		}

		else if (landType == LandType::NEAR_INLAND && erosion == 1)
		{
			if (temperature < 4)
				return GetMiddleBiomes(temperature, humidity);
			else
				return GetBadlandBiomes(humidity);
		}

		return GetMiddleBiomes(temperature, humidity);
	}

	//Peaks
	else if (peakType == PeakType::PEAK)
	{
		if (landType == LandType::COAST || landType == LandType::NEAR_INLAND)
		{
			if (erosion == 0)
			{
				if (temperature <= 2)
					return BiomeType::SNOWY_PEAKS;
				else
					return BiomeType::STONY_PEAKS;
			}

			else if (erosion == 1)
			{
				if (temperature < 4)
					return GetMiddleBiomes(temperature, humidity);
				else
					return GetBadlandBiomes(humidity);
			}

			else
				return GetMiddleBiomes(temperature, humidity);
		}

		else if (erosion < 2)
		{
			if (erosion == 0)
			{
				if (temperature <= 2)
					return BiomeType::SNOWY_PEAKS;
				else
					return BiomeType::STONY_PEAKS;
			}
		}

		else if (landType == LandType::MID_INLAND && erosion == 3)
		{
			if (temperature < 4)
				return GetMiddleBiomes(temperature, humidity);
			else
				return GetBadlandBiomes(humidity);
		}

		return GetMiddleBiomes(temperature, humidity);
	}

	return BiomeType::GENERAL;
}

BiomeType WorldDefinition::GetBeachBiome(int temperature)
{
	if (temperature < 2)
		return BiomeType::SNOWY_BEACH;

	if (temperature == 4)
		return BiomeType::BEACH;

	return BiomeType::DESERT;
}

BiomeType WorldDefinition::GetBadlandBiomes(int humidity)
{
	if (humidity == 2)
		return BiomeType::DESERT;

	return BiomeType::SAVANNA;
}

BiomeType WorldDefinition::GetMiddleBiomes(int temperature, int humidity)
{
	if (temperature == 4)
		return BiomeType::DESERT;

	if (humidity < 2)
	{
		if (temperature < 1)
			return BiomeType::SNOWY_PLAINS;

		if (temperature < 3)
			return BiomeType::PLAINS;

		if (temperature < 4)
			return BiomeType::SAVANNA;

		return BiomeType::DESERT;
	}

	if (humidity < 3)
	{
		if (temperature < 1)
			return BiomeType::SNOWY_TAIGA;

		if (temperature < 3)
			return BiomeType::FOREST;

		if (temperature < 4)
			return BiomeType::PLAINS;

		return BiomeType::DESERT;
	}

	if (humidity < 4)
	{
		if (temperature < 1)
			return BiomeType::SNOWY_TAIGA;

		if (temperature < 2)
			return BiomeType::TAIGA;

		if (temperature < 3)
			return BiomeType::FOREST;

		if (temperature < 4)
			return BiomeType::JUNGLE;

		return BiomeType::DESERT;
	}

	if (temperature < 1)
		return BiomeType::TAIGA;

	if (temperature < 2)
		return BiomeType::TAIGA;

	if (temperature < 3)
		return BiomeType::JUNGLE;

	if (temperature < 4)
		return BiomeType::JUNGLE;

	return BiomeType::DESERT;
}

PeakType WorldDefinition::GetPeakTypeFromPeaksAndValleys(float pv, float noiseFluctuation)
{
	float adjustedPV = pv + (noiseFluctuation * 0.01f);
	if(adjustedPV <= -0.85f)
		return PeakType::VALLEY;

	if(adjustedPV <= -0.2f)
		return PeakType::LOW;

	if(adjustedPV <= 0.2f)
		return PeakType::MID;

	if(adjustedPV <= 0.7f)
		return PeakType::HIGH;

	return PeakType::PEAK;
}

int WorldDefinition::GetTemperatureSectorFromTemperature(float temperature, float noiseFluctuation)
{
	float adjustedTemp = temperature + (noiseFluctuation * 0.01f);
	if (adjustedTemp <= -0.45f)
		return 0;
	if (adjustedTemp <= -0.2f)
		return 1;
	if (adjustedTemp <= 0.2f)
		return 2;
	if (adjustedTemp <= 0.55f)
		return 3;
	return 4;
}

int WorldDefinition::GetHumiditySectorFromHumidity(float humidity, float noiseFluctuation)
{
	float adjustedHumidity = humidity + (noiseFluctuation * 0.01f);
	if(adjustedHumidity <= -0.35f)
		return 0;
	if(adjustedHumidity <= -0.1f)
		return 1;
	if(adjustedHumidity <= 0.1f)
		return 2;
	if(adjustedHumidity <= 0.3f)
		return 3;
	return 4;
}

bool WorldDefinition::DoesBiomeHaveDirtDepthLayer(BiomeType biome)
{
	switch (biome)
	{
	case BiomeType::GENERAL: return true; 
	case BiomeType::OCEAN: return true;
	case BiomeType::BEACH: return true;
	case BiomeType::SNOWY_BEACH: return true;
	case BiomeType::DESERT: return true;
	case BiomeType::SAVANNA: return true;
	case BiomeType::PLAINS: return true;
	case BiomeType::SNOWY_PLAINS: return true;
	case BiomeType::FOREST: return true;
	case BiomeType::JUNGLE: return true;
	case BiomeType::TAIGA: return true;
	case BiomeType::SNOWY_TAIGA: return true;

	case BiomeType::DEEP_OCEAN: return false;
	case BiomeType::FROZEN_OCEAN: return false;
	case BiomeType::STONY_PEAKS: return false;
	case BiomeType::SNOWY_PEAKS: return false;
	case BiomeType::COUNT:
	default: return true;
	}
}

uint8_t WorldDefinition::GetSurfaceBlockTypeFromBiome(BiomeType biome, int seaHeight, int terrainHeight, float temperature)
{
	bool aboveSeaLevel = seaHeight <= terrainHeight;
	switch (biome)
	{
	case BiomeType::GENERAL: return BlockDefinition::GetBlockDefinitionIndexFromName("Stone");
	case BiomeType::OCEAN: 
		return aboveSeaLevel ? BlockDefinition::GetBlockDefinitionIndexFromName("Sand") : BlockDefinition::GetBlockDefinitionIndexFromName("Dirt");
	case BiomeType::DEEP_OCEAN: 
		if (aboveSeaLevel)
		{
			if (temperature <= 0)
			{
				return BlockDefinition::GetBlockDefinitionIndexFromName("Snow");
			}

			else
			{
				return BlockDefinition::GetBlockDefinitionIndexFromName("Sand");
			}
		}

		return BlockDefinition::GetBlockDefinitionIndexFromName("Stone");
		
	case BiomeType::FROZEN_OCEAN: return aboveSeaLevel ? BlockDefinition::GetBlockDefinitionIndexFromName("Snow") : BlockDefinition::GetBlockDefinitionIndexFromName("Stone");
	case BiomeType::BEACH: return BlockDefinition::GetBlockDefinitionIndexFromName("Sand");
	case BiomeType::SNOWY_BEACH: return BlockDefinition::GetBlockDefinitionIndexFromName("Snow");
	case BiomeType::DESERT: return BlockDefinition::GetBlockDefinitionIndexFromName("Sand");
	case BiomeType::SAVANNA: return BlockDefinition::GetBlockDefinitionIndexFromName("GrassYellow");
	case BiomeType::PLAINS: return BlockDefinition::GetBlockDefinitionIndexFromName("GrassLight");
	case BiomeType::SNOWY_PLAINS: return BlockDefinition::GetBlockDefinitionIndexFromName("Snow");
	case BiomeType::FOREST: return BlockDefinition::GetBlockDefinitionIndexFromName("Grass");
	case BiomeType::JUNGLE: return BlockDefinition::GetBlockDefinitionIndexFromName("GrassDark");
	case BiomeType::TAIGA: return BlockDefinition::GetBlockDefinitionIndexFromName("GrassLight");
	case BiomeType::SNOWY_TAIGA: return BlockDefinition::GetBlockDefinitionIndexFromName("Snow");
	case BiomeType::STONY_PEAKS: return BlockDefinition::GetBlockDefinitionIndexFromName("Stone");
	case BiomeType::SNOWY_PEAKS: return BlockDefinition::GetBlockDefinitionIndexFromName("Snow");
	default: return BlockDefinition::GetBlockDefinitionIndexFromName("Grass");
	}
}

bool WorldDefinition::DoesBiomeHaveTrees(BiomeType biome, std::vector<TreeType>& out_treeType)
{
	switch (biome)
	{
	case BiomeType::DESERT:
		out_treeType.push_back(TreeType::CACTUS);
	return true;

	case BiomeType::SAVANNA:
		out_treeType.push_back(TreeType::ACACIA);
		out_treeType.push_back(TreeType::ACACIA);
		out_treeType.push_back(TreeType::CACTUS);
	return true;

	case BiomeType::PLAINS:
		out_treeType.push_back(TreeType::SMALL_OAK);
	return true;

	case BiomeType::SNOWY_PLAINS:
		out_treeType.push_back(TreeType::LEAFLESS_OAK);
	return true;

	case BiomeType::FOREST:
		out_treeType.push_back(TreeType::OAK);
		out_treeType.push_back(TreeType::BIRCH);
	return true;

	case BiomeType::JUNGLE:
		out_treeType.push_back(TreeType::JUNGLE);
		out_treeType.push_back(TreeType::JUNGLE);
		out_treeType.push_back(TreeType::JUNGLE);
		out_treeType.push_back(TreeType::JUNGLE_BRUSH);
		out_treeType.push_back(TreeType::JUNGLE_BRUSH);
		out_treeType.push_back(TreeType::MEGA_JUNGLE);
	return true;

	case BiomeType::TAIGA: 
		out_treeType.push_back(TreeType::SPRUCE);
	return true;

	case BiomeType::SNOWY_TAIGA:
		out_treeType.push_back(TreeType::SNOWY_SPRUCE);
	return true;

	case BiomeType::GENERAL: return false;
	case BiomeType::OCEAN: return false;
	case BiomeType::DEEP_OCEAN: return false;
	case BiomeType::FROZEN_OCEAN: return false;
	case BiomeType::BEACH: return false;
	case BiomeType::SNOWY_BEACH: return false;
	case BiomeType::STONY_PEAKS: return false;
	case BiomeType::SNOWY_PEAKS: return false;
	default: return false;
	}
}

float WorldDefinition::GetMinVegetationSpawnNoiseFromBiome(BiomeType biome)
{
	switch (biome)
	{
	case BiomeType::DESERT: return 0.9993f;
	case BiomeType::SAVANNA: return 0.99f;
	case BiomeType::PLAINS: return 0.9975f;
	case BiomeType::SNOWY_PLAINS: return 0.999f;
	case BiomeType::FOREST: return 0.975f;
	case BiomeType::JUNGLE: return 0.975f;
	case BiomeType::TAIGA: return 0.985f;
	case BiomeType::SNOWY_TAIGA: return 0.985f;

	case BiomeType::GENERAL: return 1.f;
	case BiomeType::OCEAN: return 1.f;
	case BiomeType::DEEP_OCEAN: return 1.f;
	case BiomeType::FROZEN_OCEAN: return 1.f;
	case BiomeType::BEACH: return 1.f;
	case BiomeType::SNOWY_BEACH: return 1.f;
	case BiomeType::STONY_PEAKS: return 1.f;
	case BiomeType::SNOWY_PEAKS: return 1.f;
	default: return 1.f;
	}
}

float WorldDefinition::GetMinWildGrassSpawnNoiseFromBiome(BiomeType biome)
{
	switch (biome)
	{
	case BiomeType::PLAINS: return 0.9f;
	case BiomeType::FOREST: return 0.95f;
	case BiomeType::JUNGLE: return 0.99f;
	case BiomeType::TAIGA: return 0.975f;
	case BiomeType::SAVANNA: return 0.995f;
	case BiomeType::SNOWY_TAIGA: return 0.9975f;
	case BiomeType::SNOWY_PLAINS: return 0.995f;
	}
	return 0.0f;
}

float WorldDefinition::GetMinWaterFallSpawnNoiseFromBiome(BiomeType biome)
{
	switch (biome)
	{
	case BiomeType::PLAINS: return 0.995f;
	case BiomeType::FOREST: return 0.775f;
	case BiomeType::JUNGLE: return 0.78f;
	case BiomeType::SAVANNA: return 0.775f;
	case BiomeType::STONY_PEAKS: return 0.9995f;
	case BiomeType::DESERT: return 0.9935f;
	case BiomeType::SNOWY_PEAKS: return 0.995f;
	case BiomeType::SNOWY_TAIGA: return 0.995f;
	}
	return 0.99999f;
}


uint8_t WorldDefinition::GetWaterBlockTypeFromBiome(BiomeType biome, int seaHeight, int blockHeight, float temperature, float iceMeltNoise)
{
	uint8_t defaultWaterType = BlockDefinition::GetBlockDefinitionIndexFromName("Water");

	bool isSeaLevel = seaHeight == blockHeight;

	float heatNudge = 0.f;
	
	switch (biome)
	{
	case BiomeType::OCEAN: 
		heatNudge = iceMeltNoise * 0.2f;
		break;
	case BiomeType::DEEP_OCEAN:
		heatNudge = iceMeltNoise * 0.5f;
		break;
	case BiomeType::FROZEN_OCEAN: 
		heatNudge = iceMeltNoise * -1.f;
		break;
	case BiomeType::SNOWY_PLAINS:
		heatNudge = iceMeltNoise * -1.f;
		break;
	case BiomeType::SNOWY_TAIGA:
		heatNudge = iceMeltNoise * -1.f;
		break;
	case BiomeType::SNOWY_PEAKS:
		heatNudge = iceMeltNoise * -1.f;
		break;
	case BiomeType::DESERT:
		heatNudge = iceMeltNoise * 1.2f;
		break;
	case BiomeType::BEACH:
		heatNudge = iceMeltNoise * 1.2f;
		break;
	default:
		heatNudge = 0.95f;
		break;
	
	}

	bool isColdTemp = temperature + heatNudge < -0.15f;
	return isColdTemp && isSeaLevel ? BlockDefinition::GetBlockDefinitionIndexFromName("Ice") : defaultWaterType;
	
}

void WorldDefinition::GetDirtLayersFromBiome(BiomeType biome, std::vector<uint8_t>& out_dirtTypes, std::vector<IntRange>& out_dirtDepths)
{
	switch (biome)
	{
	case BiomeType::DESERT:
		out_dirtTypes.push_back(BlockDefinition::GetBlockDefinitionIndexFromName("Sand"));
		out_dirtDepths.push_back(IntRange(3,4));
		out_dirtTypes.push_back(BlockDefinition::GetBlockDefinitionIndexFromName("Dirt"));
		out_dirtDepths.push_back(IntRange(1, 2));
		break;
	case BiomeType::OCEAN: 
		out_dirtTypes.push_back(BlockDefinition::GetBlockDefinitionIndexFromName("Dirt"));
		out_dirtDepths.push_back(IntRange(3, 5));
		break;
	case BiomeType::DEEP_OCEAN: break;
	case BiomeType::FROZEN_OCEAN: break;
	case BiomeType::STONY_PEAKS: break;
	case BiomeType::SNOWY_PEAKS: break;
	default: 
		out_dirtTypes.push_back(BlockDefinition::GetBlockDefinitionIndexFromName("Dirt"));
		out_dirtDepths.push_back(IntRange(3, 5));
		break;;
	}

}

std::vector<uint8_t> WorldDefinition::GetWildGrassFromBiome(BiomeType biome)
{
	std::vector<uint8_t> grassTypes;
	switch (biome)
	{
	case BiomeType::PLAINS:
		grassTypes.push_back(BlockDefinition::GetBlockDefinitionIndexFromName("WildGrass"));
		grassTypes.push_back(BlockDefinition::GetBlockDefinitionIndexFromName("WildGrass"));
		grassTypes.push_back(BlockDefinition::GetBlockDefinitionIndexFromName("WildGrassTall"));
		break;
	case BiomeType::FOREST:
		grassTypes.push_back(BlockDefinition::GetBlockDefinitionIndexFromName("WildGrass"));
		grassTypes.push_back(BlockDefinition::GetBlockDefinitionIndexFromName("WildGrassTall"));
		break;
	case BiomeType::JUNGLE:
		grassTypes.push_back(BlockDefinition::GetBlockDefinitionIndexFromName("WildGrass"));
		break;
	case BiomeType::SAVANNA:
		grassTypes.push_back(BlockDefinition::GetBlockDefinitionIndexFromName("WildGrass"));
		grassTypes.push_back(BlockDefinition::GetBlockDefinitionIndexFromName("WildGrassTall"));
		break;
	case BiomeType::TAIGA:
		grassTypes.push_back(BlockDefinition::GetBlockDefinitionIndexFromName("WildGrass"));
		grassTypes.push_back(BlockDefinition::GetBlockDefinitionIndexFromName("WildGrassTall"));
		break;
	case BiomeType::SNOWY_TAIGA:
		grassTypes.push_back(BlockDefinition::GetBlockDefinitionIndexFromName("WildGrassFrozen"));
		grassTypes.push_back(BlockDefinition::GetBlockDefinitionIndexFromName("WildGrassFrozenTall"));
		break;
	case BiomeType::SNOWY_PLAINS:
		grassTypes.push_back(BlockDefinition::GetBlockDefinitionIndexFromName("WildGrassFrozen"));
		grassTypes.push_back(BlockDefinition::GetBlockDefinitionIndexFromName("WildGrassFrozen"));
		grassTypes.push_back(BlockDefinition::GetBlockDefinitionIndexFromName("WildGrassFrozenTall"));
		break;
	}
	return grassTypes;
}




