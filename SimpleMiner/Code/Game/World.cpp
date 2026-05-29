#include "Game/World.hpp"
#include "Game/Chunk.hpp"
#include "Game/Player.hpp"
#include "Game/Game.hpp"
#include "Game/GameCamera.hpp"
#include "Game/GameCommon.hpp"
#include "Game/BlockDefinition.hpp"
#include "Game/WorldDefinition.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Renderer/RendererDX11.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/TileHeatMap.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Math/Curves.hpp"
#include "Engine/Math/RawNoise.hpp"
#include "Engine/Math/SmoothNoise.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"
#include "Engine/Renderer/ConstantBuffer.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Window/Window.hpp"
#include <unordered_set>
#include <queue>

ChunkGenerateJob::ChunkGenerateJob(Chunk* chunk)
	:m_chunk(chunk)
	,m_timeStarted(GetCurrentTimeSeconds())
{
}

//Jobs
//-------------------------------------------------------------------------------------
void ChunkGenerateJob::Execute()
{
	m_chunk->m_state = ChunkState::GENERATING;
	m_chunk->CreateProceduralBlocks();
}

ChunkLoadJob::ChunkLoadJob(Chunk* chunk)
	:m_chunk(chunk)
	, m_timeStarted(GetCurrentTimeSeconds())
{
}

void ChunkLoadJob::Execute()
{
	m_chunk->m_state = ChunkState::LOADING;
	if (!m_chunk->TryCreateBlocksFromFile())
	{
		m_chunk->m_state = ChunkState::GENERATING;
		m_chunk->CreateProceduralBlocks();
	}
}

void ChunkSaveJob::Execute()
{
	m_chunk->m_state = ChunkState::SAVING;
	m_chunk->SaveChunkToDisk();
}

//World
//-------------------------------------------------------------------------------------
World::World(WorldDefinition const* worldDef)
	:m_worldDef(worldDef)
{
	if (!FolderExists("Saves"))
	{
		if (!CreateFolder("Saves"))
		{
			ERROR_AND_DIE("Could not find or create Saves folder");
		}
	}

	Rgba8 indoorColor(255,230,204);
	indoorColor.GetAsFloats(m_worldConstants.m_indoorLightColor);

	Rgba8 outdoorColor(255,255,255);
	outdoorColor.GetAsFloats(m_worldConstants.m_outdoorLightColor);

	m_worldConstants.m_fogNearDistance = 0.01f;
	m_worldConstants.m_fogFarDistance = (float)CHUNK_ACTIVATION_RANGE - 100.f;

	m_worldConstantBuffer = g_renderer->CreateConstantBuffer(sizeof(WorldConstants));
}

World::~World()
{
	delete m_worldConstantBuffer;
	m_worldConstantBuffer = nullptr;
}

void World::ShutDown()
{
	m_isShuttingDown = true;
	m_dirtyLiquidBlocks.clear();
	m_dirtyLightBlocks.clear();
	m_pendingActivationChunks.clear();

	// finish loading chunks
	while (m_numSentJobs > 0)
	{
		ProcessFinishedChunkJobs();
		m_numSentJobs = GetMin(m_numSentJobs, g_jobSystem->GetTotalNumberJobs());
	}

	//Delete all active chunks
	for (int i = 0; i < (int)m_activeChunks.size(); ++i)
	{
		if (m_activeChunks[i])
		{
			DeleteChunk(m_activeChunks[i]);
		}
	}

	//Save all chunks that need saving
	while (!m_needsSavingChunks.empty())
	{
		Chunk* chunk = m_needsSavingChunks.front();
		m_needsSavingChunks.pop_front();
		ExecuteChunkJob(chunk, ChunkJobType::SAVE);
	}

	//Finish saving chunks
	while (m_numSentJobs > 0)
	{
		ProcessFinishedChunkJobs();
	}

	//delete all chunks that are marked for delete
	ProcessPendingDeleteChunks();

	//Final pass delete all chunks that somehow got past other lists
	for (auto it = m_coordsChunkPair.begin(); it != m_coordsChunkPair.end(); ++it)
	{
		Chunk* chunk = it->second;
		if (chunk)
		{
			delete chunk;
			chunk = nullptr;
		}
	}

	m_dirtyChunks.clear();
	m_activeChunks.clear();
	m_coordsChunkPair.clear();
	m_pendingChunks.clear();
	m_pendingDeleteChunks.clear();
}

void World::AttractScreenUpdate(double timeUpdateStarted)
{
	UpdateWorldTimeAndLighting();
	ProcessPendingActivation();
	ProcessChunkLoading();
	ProcessDirtyLiquid();
	ProcessDirtyLighting();
	ProcessFinishedChunkJobs();

	RegenerateDirtyChunks(timeUpdateStarted);
	m_maxGenerateMeshSpeed = 0.f;
}

void World::Update(double timeUpdateStarted)
{

	CheckKeyboardControls();
	CheckControllerControls();
	UpdateWorldTimeAndLighting();

	for (int i = 0; i < (int)m_activeChunks.size(); ++i)
	{
		if (m_activeChunks[i])
		{
			m_activeChunks[i]->Update();
		}
	}


	ProcessDebugMessages();

	ProcessPendingActivation();
	ProcessChunkLoading();
	ProcessDirtyLiquid();
	ProcessDirtyLighting();

	ProcessPendingSaveChunks();

	ProcessFinishedChunkJobs();

	
	double timeMeshBuildStarted = GetCurrentTimeSeconds();
	if (RegenerateDirtyChunks(timeUpdateStarted))
	{
		m_totalGenerateMesh++;
		m_generateMeshSpeed = (GetCurrentTimeSeconds() - timeMeshBuildStarted) * 1000.0;
		m_totalGenerateMeshSpeed += m_generateMeshSpeed;
		m_avgGenerateMeshSpeed = m_totalGenerateMeshSpeed / m_totalGenerateMesh;
	}
	


	if (m_generateMeshSpeed > m_maxGenerateMeshSpeed)
	{
		m_maxGenerateMeshSpeed = m_generateMeshSpeed;
	}

	ProcessPendingDeleteChunks(true);

	if (m_shouldReload)
	{
		g_game->ReloadWorld(m_worldDef);
	}

}

void World::Render() const
{
	g_renderer->BeginRendererEvent("Draw Chunks");

	g_renderer->CopyCPUToGPU(&m_worldConstants, sizeof(WorldConstants), m_worldConstantBuffer);
	g_renderer->BindConstantBuffer(4, m_worldConstantBuffer);

	Frustum cameraFrustrum = g_game->m_player->GetViewFrustrum();
	for (int i = 0; i < (int)m_activeChunks.size(); ++i)
	{
		if (m_activeChunks[i])
		{
			m_activeChunks[i]->Render(cameraFrustrum);
		}
	}
	g_renderer->EndRendererEvent();

	//Render Water Surface
	PerFrameConstants perFrameConst;
	perFrameConst.m_time = (float)GetCurrentTimeSeconds();
	g_renderer->SetPerFrameConstants(perFrameConst);
	RenderTarget terrainRenderTarget = g_renderer->GetCopyOfCurrentRenderTarget();

	g_renderer->BeginRendererEvent("Draw Water");
	g_renderer->SetBlendMode(BlendMode::ALPHA);
	g_renderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
	g_renderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
	g_renderer->SetSamplerMode(SamplerMode::BILINEAR_WRAP);
	g_renderer->SetSamplerMode(SamplerMode::POINT_CLAMP, 1);
	g_renderer->BindTexture(m_worldDef->m_waterTexture);
	g_renderer->BindTexture(terrainRenderTarget.m_color, 1);
	g_renderer->BindTexture(terrainRenderTarget.m_depth, 2);
	g_renderer->BindShader(m_worldDef->m_waterShader);
	for (int i = 0; i < (int)m_activeChunks.size(); ++i)
	{
		if (m_activeChunks[i])
		{
			m_activeChunks[i]->RenderWater(cameraFrustrum);
		}
	}
	g_renderer->EndRendererEvent();
	
	if (g_game->m_player->IsUnderWater())
	{
		
		//Back Surface of water while looking out
		
		g_renderer->BeginRendererEvent("Draw Water Under Surface");
		g_renderer->SetBlendMode(BlendMode::ALPHA);
		g_renderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
		g_renderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_FRONT);
		g_renderer->SetSamplerMode(SamplerMode::BILINEAR_WRAP);
		g_renderer->SetSamplerMode(SamplerMode::POINT_CLAMP, 1);
		g_renderer->BindTexture(m_worldDef->m_waterTexture);
		g_renderer->BindTexture(terrainRenderTarget.m_color, 1);
		g_renderer->BindTexture(terrainRenderTarget.m_depth, 2);
		g_renderer->BindShader(m_worldDef->m_underWaterSurfaceShader);
		for (int i = 0; i < (int)m_activeChunks.size(); ++i)
		{
			if (m_activeChunks[i])
			{
				m_activeChunks[i]->RenderWater(cameraFrustrum);
			}
		}
		
		g_renderer->EndRendererEvent();
		
		
		//Under water terrain
		g_renderer->BeginRendererEvent("Draw Under Water Post Process");
		g_renderer->SetBlendMode(BlendMode::ALPHA);
		g_renderer->EndCamera(*(g_game->m_player->m_camera));
		RenderTarget waterAndTerrainRenderTarget = g_renderer->GetCopyOfCurrentRenderTarget();
		g_renderer->BindTexture(waterAndTerrainRenderTarget.m_depth, 2);
		g_renderer->DrawFullScreenQuad(waterAndTerrainRenderTarget.m_color, terrainRenderTarget.m_depth, m_worldDef->m_underWaterShader, DepthMode::DISABLED);
		g_renderer->BeginCamera(*(g_game->m_player->m_camera));
		g_renderer->EndRendererEvent();
		

	}

}

void World::CheckKeyboardControls()
{
	if (g_inputSystem->WasKeyJustPressed(KEYCODE_F8))
	{
		m_shouldReload = true;
	}

	if (g_inputSystem->WasKeyJustPressed(KEYCODE_F3))
	{
		m_showJobInfo = !m_showJobInfo;
	}

	if (g_inputSystem->WasKeyJustPressed(KEYCODE_F4))
	{
		m_loadChunks = !m_loadChunks;
	}

	if (g_inputSystem->WasKeyJustPressed(KEYCODE_F6))
	{
		m_useDayCycle = !m_useDayCycle;
	}
}

void World::CheckControllerControls()
{
	XboxController controller = g_inputSystem->GetController(0);
	if (controller.WasButtonJustPressed(XboxButtonID::BUTTON_B))
	{
		m_shouldReload = true;
	}
}

void World::ProcessDebugMessages()
{
	AABB2 screenBounds = g_game->GetScreenBounds();
	AABB2 bounds = screenBounds;
	//bounds.AddPadding(-0.01f, -.95f, -.75f, -.0001f);

	//std::string text = Stringf("%i/%i Chunks", (int)m_activeChunks.size() - 1, MAX_ACTIVE_CHUNKS);
	//DebugAddScreenText(text, bounds, 0.1f * g_SCREEN_SIZE_Y, Vec2(1.f, 0.5f), 0.f);

	if (m_showJobInfo)
	{
		g_game->m_player->UpdateDebugMessages();

		int numChunkTypes[(int)ChunkState::COUNT] = {};
		for (auto it = m_coordsChunkPair.begin(); it != m_coordsChunkPair.end(); ++it)
		{
			Chunk* chunk = it->second;
			ChunkState state = chunk->m_state;
			numChunkTypes[(int)state] += 1;
		}
		
		bounds = screenBounds;
		bounds.AddPadding(-0.01f, -0.01f, -0.25f, -0.25f);
		constexpr int NUM_LINES = 58;
		std::vector<AABB2> boundsSlices = bounds.GetHorizontalSlicedBoxesTopToBottom(NUM_LINES);
		std::string lines[NUM_LINES] =
		{
			"-------Chunks-------",
			Stringf("Num Chunks:       %i / %i",(int)m_activeChunks.size() - 1, MAX_ACTIVE_CHUNKS),
			Stringf("Pending Generate: %i", numChunkTypes[0]),
			Stringf("Generating:       %i", numChunkTypes[1]),
			Stringf("Pending Load:     %i", numChunkTypes[2]),
			Stringf("Loading:          %i", numChunkTypes[3]),
			Stringf("Active:           %i", numChunkTypes[4]),
			Stringf("Saving:           %i", numChunkTypes[6]),
			Stringf("Saved:            %i", numChunkTypes[7]),
			" ",
			"[Pending]", //10
			Stringf("Activation:       %i", m_pendingActivationChunks.size()),
			Stringf("Mesh:             %i", m_dirtyChunks.size()),
			Stringf("Save:             %i", numChunkTypes[5]),
			Stringf("Deletion:         %i", numChunkTypes[8]),
			" ",
			"--------Jobs--------", //16
			Stringf("Loading:          %i", m_numInFlightLoad),
			Stringf("Generating:       %i", m_numInFlightGenerate),
			Stringf("Saving:           %i", m_numInFlightSave),
			Stringf("Max Num Jobs:     %i", MAX_NUM_CHUNK_JOBS),
			Stringf("Num Workers:      %i", g_jobSystem->GetNumWorkerThreads()),
			Stringf("Pending:          %i", g_jobSystem->GetNumPendingJobs()),
			Stringf("Executing:        %i", g_jobSystem->GetNumExecutingJobs()),
			Stringf("Completed:        %i", g_jobSystem->GetNumCompletedJobs()),
			" ",
			"--Chunk Creation Speed-" ,//26
			"[Chunk Load]",
			Stringf("Avg:              %.2f ms", m_avgChunkLoadSpeed),
			Stringf("Max:              %.2f ms", m_maxChunkLoadSpeed),
			" ",
			"[Chunk Generate]",
			Stringf("Avg:              %.2f ms", m_avgChunkGenerateSpeed),
			Stringf("Max:              %.2f ms", m_maxChunkGenerateSpeed),
			" ",
			"[Chunk Pending Activation]",
			Stringf("Avg:              %.2f ms", m_avgActivationSpeed),
			Stringf("Max:              %.2f ms", m_maxActivationSpeed),
			" ",
			"[Chunk Pending Mesh]",
			Stringf("Avg:              %.2f ms", m_avgMeshWaitSpeed),
			Stringf("Max:              %.2f ms", m_maxMeshWaitSpeed),

			" ",
			"----Update Speed----",//43
			"[Mesh Building]", //44
			Stringf("Current:          %.2f ms", m_generateMeshSpeed),
			Stringf("Avg:              %.2f ms", m_avgGenerateMeshSpeed),
			Stringf("Max:              %.2f ms", m_maxGenerateMeshSpeed),
			" ",
			"[Light Processing]", //49
			Stringf("Current:          %.2f ms", m_processLightSpeed),
			Stringf("Avg:              %.2f ms", m_avgProcessLightSpeed),
			Stringf("Max:              %.2f ms", m_maxProcessLightSpeed),
			" ",
			"[Liquid Processing]", //54
			Stringf("Current:          %.2f ms", m_processLiquidSpeed),
			Stringf("Avg:              %.2f ms", m_avgProcessLiquidSpeed),
			Stringf("Max:              %.2f ms", m_maxProcessLiquidSpeed),
		};

		for (int i = 0; i < NUM_LINES; ++i)
		{
			Rgba8 color = Rgba8::WHITE;

			if((i >= 1 && i < 10) || (i > 10 && i < 16))
				color = Rgba8(220,220,220);

			else if (i >= 17 && i < 26)
				color = Rgba8(100, 100, 255);

			else if (i >= 27 && i < 42)
				color = Rgba8::ORANGE;

			else if(i >= 45 && i < 49)
				color = Rgba8::GREEN;
			else if(i >= 50 && i < 54)
				color = Rgba8(255, 255, 100);
			else if(i >= 55)
				color = Rgba8::CYAN;

			if(i == 27 || i == 31 || i == 35 || i == 39)
				color = Rgba8::WHITE;

			if (i == 49)
			{
				if (m_processLightSpeed > 1.0)
					color = Rgba8::ORANGE;
				if (m_processLightSpeed > 3.0)
					color = Rgba8::RED;
			}

			else if (i == 54)
			{
				if (m_processLiquidSpeed > 1.0)
					color = Rgba8::ORANGE;
				if (m_processLiquidSpeed > 3.0)
					color = Rgba8::RED;
			}

			DebugAddScreenText(lines[i], boundsSlices[i], g_SCREEN_SIZE_Y, Vec2(0.f, 0.5f), 0.f, color, color);
		}
	}
}

void World::UpdateWorldTimeAndLighting()
{
	if (m_useDayCycle)
	{
		float deltaSeconds = g_game->m_gameClock->GetDeltaSeconds();
		float acceleration = g_inputSystem->IsKeyDown('Y') ? 50.0f : 1.0f;
		float baseWorldTimeScale = 500.0f;
		float worldTimeScale = (acceleration * baseWorldTimeScale) / (60.0f * 60.0f * 24.0f);
		m_worldTimeDays += (deltaSeconds * worldTimeScale);
	}
	else
	{
		m_worldTimeDays = 0.5f;
	}


	float lightningPerlin = Compute1dPerlinNoise(m_worldTimeDays * 200.f, 1.f, 9, m_worldDef->m_defaultOctavePersistance, m_worldDef->m_defaultOctaveScale, true, m_worldDef->m_gameSeed);
	m_lightningStrength = RangeMapClamped(lightningPerlin, 0.6f, 0.9f, 0.0f, 1.0f);


	Rgba8 skyColor = GetSkyColor();
	skyColor.GetAsFloats(m_worldConstants.m_skyColor);
	Rgba8 outdoorLightColor = GetOutdoorLightColor();
	outdoorLightColor.GetAsFloats(m_worldConstants.m_outdoorLightColor);

	Rgba8 indoorColor(255, 230, 204);

	float glowPerlin = Compute1dPerlinNoise(m_worldTimeDays * 500.f, 1.f, 9, m_worldDef->m_defaultOctavePersistance, m_worldDef->m_defaultOctaveScale, true, m_worldDef->m_gameSeed + 1);
	float glowStrength = RangeMapClamped(glowPerlin, -1.f, 1.f, 0.8f, 1.0f);
	indoorColor *= glowStrength;
	indoorColor.a = 255;
	indoorColor.GetAsFloats(m_worldConstants.m_indoorLightColor);

	m_worldConstants.m_underWater = g_game->m_player->IsUnderWater() ? 1 : 0;
	m_worldConstants.m_dayTime = GetTimeOfDay();
}


float World::GetTimeOfDay() const
{
	return static_cast<float>(fmod(m_worldTimeDays, 1.0));
}
Rgba8 World::GetOutdoorLightColor() const
{
	const Rgba8 NIGHT_COLOR(30, 40, 60, 255);
	const Rgba8 DAWN_COLOR(255, 160, 100, 255);
	const Rgba8 DAY_COLOR(255, 255, 240, 255);
	const Rgba8 DUSK_COLOR(255, 140, 80, 255);
	float dawnDuskSharpness = 2.5f;

	float t = GetTimeOfDay();

	// Daylight strength: 0 at midnight, 1 at noon
	float dayFactor = 0.5f - 0.5f * cosf(t * 6.283185f);

	// Warmth peaks at dawn (0.25) and dusk (0.75)
	float warmStrength = sinf(t * 6.283185f);
	float warm = powf(fabsf(warmStrength), dawnDuskSharpness);

	// Base sky (night to bright blue day)
	Rgba8 nightToDay = Rgba8::ColorLerp(NIGHT_COLOR, DAY_COLOR, dayFactor);

	// Warm tint for sunrise/sunset
	Rgba8 warmTint = Rgba8::ColorLerp(DUSK_COLOR, DAWN_COLOR, (warm > 0.0f) ? warm : -warm);

	// Blend warm tint in only when warmStrength is high (dawn/dusk)
	Rgba8 finalRGB = Rgba8::ColorLerp(nightToDay, warmTint, warm * 0.7f);
	
	float isNight = (dayFactor < 0.15f) ? 1.0f : 0.0f;
	float lightning = m_lightningStrength * isNight;
	finalRGB = Rgba8::ColorLerp(finalRGB, Rgba8::WHITE, lightning);

	return finalRGB;
}

Rgba8 World::GetSkyColor() const
{
	const Rgba8 NIGHT_COLOR(10, 10, 30, 255);     // deep blue-black
	const Rgba8 DAWN_COLOR(255, 140, 70, 255);       // warm sunrise
	const Rgba8 DAY_COLOR(200, 230, 255, 255);       // bright sky blue
	const Rgba8 DUSK_COLOR(255, 100, 40, 255);       // orange-red sunset
	float dawnDuskSharpness = 2.5f;

	float t = GetTimeOfDay();

	// Daylight strength: 0 at midnight, 1 at noon
	float dayFactor = 0.5f - 0.5f * cosf(t * 6.283185f);

	// Warmth peaks at dawn (0.25) and dusk (0.75)
	float warmStrength = sinf(t * 6.283185f);
	float warm = powf(fabsf(warmStrength), dawnDuskSharpness);

	// Base sky (night to bright blue day)
	Rgba8 nightToDay = Rgba8::ColorLerp(NIGHT_COLOR, DAY_COLOR, dayFactor);

	// Warm tint for sunrise/sunset
	Rgba8 warmTint = Rgba8::ColorLerp(DUSK_COLOR, DAWN_COLOR, (warm > 0.0f) ? warm : -warm);

	// Blend warm tint in only when warmStrength is high (dawn/dusk)
	Rgba8 finalRGB = Rgba8::ColorLerp(nightToDay, warmTint, warm * 0.7f);

	float isNight = (dayFactor < 0.15f) ? 1.0f : 0.0f;
	float lightning = m_lightningStrength * isNight;
	finalRGB = Rgba8::ColorLerp(finalRGB, Rgba8::WHITE, lightning);

	return finalRGB;
}

//Chunk Management
//-------------------------------------------------------------------------------------

void World::RemoveBlock(BlockIterator blockIterator)
{
	Chunk* chunk = blockIterator.GetChunk();
	int blockIndex = blockIterator.m_blockIndex;
	if (chunk && blockIndex >= STRIDE_Z)
	{
		if (chunk->RemoveBlock(blockIndex))
		{
			std::vector<Chunk*> neighborTouchingChunks = blockIterator.GetEdgeNeighborChunks();
			for (int i = 0; i < (int)neighborTouchingChunks.size(); ++i)
			{
				neighborTouchingChunks[i]->SetDirty();
			}
		}
	}
}

void World::AddBlock(BlockIterator blockIterator, uint8_t blockType)
{
	Chunk* chunk = blockIterator.GetChunk();
	int blockIndex = blockIterator.m_blockIndex;
	if (chunk && blockIndex >= 0)
	{
		if (chunk->AddBlock(blockIndex, blockType))
		{
			std::vector<Chunk*> neighborTouchingChunks = blockIterator.GetEdgeNeighborChunks();
			for (int i = 0; i < (int)neighborTouchingChunks.size(); ++i)
			{
				neighborTouchingChunks[i]->SetDirty();
			}
		}
	}
}

BlockRaycastResult3D World::RaycastVsWorld(Vec3 const& startPos, Vec3 const& fwrdNormal, float maxLength)
{
	BlockRaycastResult3D result;
	result.m_didImpact = false;
	result.m_impactDistance = maxLength;
	result.m_impactPos = startPos + (fwrdNormal * maxLength);
	result.m_impactNormal = -fwrdNormal;
	result.m_blockIterator = BlockIterator(nullptr, -1);
	result.m_blockGlobalCoords = IntVec3::ZERO;
	result.m_impactFraction = 1.f;

	if (fwrdNormal == Vec3::ZERO)
		return result;

	IntVec3 voxelCoords = IntVec3(startPos);
	IntVec2 startingChunkCoords = GetChunkCoordsFromPosition(startPos);
	Chunk* startChunk = GetChunkFromChunkCoords(startingChunkCoords);

	if (!startChunk)
		return result;

	int startIndex = startChunk->GetBlockIndexFromGlobalCoords(voxelCoords);
	if (startIndex < 0)
		return result;

	BlockIterator iter(startChunk, startIndex);

	IntVec3 step(
		fwrdNormal.x > 0.f ? 1 : (fwrdNormal.x < 0.f ? -1 : 0),
		fwrdNormal.y > 0.f ? 1 : (fwrdNormal.y < 0.f ? -1 : 0),
		fwrdNormal.z > 0.f ? 1 : (fwrdNormal.z < 0.f ? -1 : 0)
	);

	Vec3 floatStep(step);

	Vec3 voxelMin = Vec3(voxelCoords);
	Vec3 voxelMax = voxelMin + Vec3(1.f, 1.f, 1.f);

	Vec3 tDelta = Vec3(
		(fwrdNormal.x != 0.f) ? (fabsf(1.f / fwrdNormal.x)) : FLT_MAX,
		(fwrdNormal.y != 0.f) ? (fabsf(1.f / fwrdNormal.y)) : FLT_MAX,
		(fwrdNormal.z != 0.f) ? (fabsf(1.f / fwrdNormal.z)) : FLT_MAX
	);

	Vec3 tMax;
	tMax.x = (fwrdNormal.x > 0.f) ? ((voxelMax.x - startPos.x) / fwrdNormal.x) : ((startPos.x - voxelMin.x) / -fwrdNormal.x);
	tMax.y = (fwrdNormal.y > 0.f) ? ((voxelMax.y - startPos.y) / fwrdNormal.y) : ((startPos.y - voxelMin.y) / -fwrdNormal.y);
	tMax.z = (fwrdNormal.z > 0.f) ? ((voxelMax.z - startPos.z) / fwrdNormal.z) : ((startPos.z - voxelMin.z) / -fwrdNormal.z);

	if (fwrdNormal.x == 0.f) tMax.x = FLT_MAX;
	if (fwrdNormal.y == 0.f) tMax.y = FLT_MAX;
	if (fwrdNormal.z == 0.f) tMax.z = FLT_MAX;

	float traveled = 0.f;
	Vec3 lastStepNormal = Vec3::ZERO;

	// --- Traverse
	while (traveled <= maxLength)
	{
		// check voxel contents
		Block* block = iter.GetBlock();
		if (block && block->IsSolid())
		{
			result.m_didImpact = true;
			result.m_impactDistance = traveled;
			result.m_impactPos = startPos + (fwrdNormal * traveled);
			result.m_blockIterator = iter;
			result.m_blockGlobalCoords = voxelCoords;
			result.m_impactNormal = lastStepNormal;
			result.m_impactFraction = traveled / maxLength;
			return result;
		}

		// step to next voxel
		if (tMax.x < tMax.y)
		{
			if (tMax.x < tMax.z)
			{
				voxelCoords.x += step.x;
				traveled = tMax.x;
				tMax.x += tDelta.x;
				iter = iter.GetIterator(step.x > 0 ? BlockDirection::EAST : BlockDirection::WEST);
				lastStepNormal = Vec3(-floatStep.x, 0.f, 0.f);
			}
			else
			{
				voxelCoords.z += step.z;
				traveled = tMax.z;
				tMax.z += tDelta.z;
				iter = iter.GetIterator(step.z > 0 ? BlockDirection::UP : BlockDirection::DOWN);
				lastStepNormal = Vec3(0.f, 0.f, -floatStep.z);
			}
		}
		else
		{
			if (tMax.y < tMax.z)
			{
				voxelCoords.y += step.y;
				traveled = tMax.y;
				tMax.y += tDelta.y;
				iter = iter.GetIterator(step.y > 0 ? BlockDirection::NORTH : BlockDirection::SOUTH);
				lastStepNormal = Vec3(0.f, -floatStep.y, 0.f);
			}
			else
			{
				voxelCoords.z += step.z;
				traveled = tMax.z;
				tMax.z += tDelta.z;
				iter = iter.GetIterator(step.z > 0 ? BlockDirection::UP : BlockDirection::DOWN);
				lastStepNormal = Vec3(0.f, 0.f, -floatStep.z);
			}
		}

		if (iter.m_chunk == nullptr)
			break;
	}

	// No hit
	result.m_didImpact = false;
	result.m_impactDistance = maxLength;
	result.m_impactPos = startPos + (fwrdNormal * maxLength);
	result.m_impactFraction = 1.f;
	return result;
}

void World::MarkLightingDirty(BlockIterator iterator)
{
	if (IGNORE_LIGHTING)
		return;

	Block* block = iterator.GetBlock();
	if(!block)
		return;

	if (block->IsLightDirty())
		return;

	block->SetIsLightDirty(true);
	m_dirtyLightBlocks.push_back(iterator);
}

void World::UndirtyLightForAllBlocksInChunk(Chunk* chunk)
{
	std::deque<BlockIterator> pocketDirtyLightBlocks;
	while (!m_dirtyLightBlocks.empty())
	{
		BlockIterator iter = m_dirtyLightBlocks.front();
		m_dirtyLightBlocks.pop_front();

		if (iter.GetChunk() == chunk)
		{
			Block* block = iter.GetBlock();
			if (block)
			{
				block->SetIsLightDirty(false);
			}
		}

		else
		{
			pocketDirtyLightBlocks.push_back(iter);
		}
	}

	m_dirtyLightBlocks = pocketDirtyLightBlocks;
}

void World::MarkLiquidDirty(BlockIterator iterator, float timeAdded)
{
	Block* block = iterator.GetBlock();
	if (!block)
		return;

	if (block->IsLiquidDirty())
		return;

	block->SetIsLiquidDirty(true);
	m_dirtyLiquidBlocks.push_back({iterator, timeAdded});
}

void World::UndirtyLiquidForAllBlocksInChunk(Chunk* chunk)
{
	std::deque<DelayedDirtyLiquidBlock> pocketDirtyLiquidBlocks;
	while (!m_dirtyLiquidBlocks.empty())
	{
		DelayedDirtyLiquidBlock delayedIter = m_dirtyLiquidBlocks.front();
		BlockIterator iter = delayedIter.m_blockIterator;
		m_dirtyLiquidBlocks.pop_front();

		if (iter.GetChunk() == chunk)
		{
			Block* block = iter.GetBlock();
			if (block)
			{
				block->SetIsLiquidDirty(false);
			}
		}

		else
		{
			pocketDirtyLiquidBlocks.push_back(delayedIter);
		}
	}

	m_dirtyLiquidBlocks = pocketDirtyLiquidBlocks;
}

void World::ProcessPendingSaveChunks()
{
	if (!m_needsSavingChunks.empty())
	{
		for (int i = m_numSentJobs; i < MAX_NUM_CHUNK_JOBS; ++i)
		{
			if (i >= m_needsSavingChunks.size())
				break;

			Chunk* chunk = m_needsSavingChunks.front();
			m_needsSavingChunks.pop_front();
			ExecuteChunkJob(chunk, ChunkJobType::SAVE);
		}
	}
}

void World::ProcessPendingDeleteChunks(bool overrideMax)
{
	if (overrideMax)
	{
		while (!m_pendingDeleteChunks.empty())
		{
			Chunk* chunk = m_pendingDeleteChunks.front();
			m_pendingDeleteChunks.pop_front();
			if (chunk)
			{
				IntVec2 chunkCoords = chunk->m_chunkCoords;
				m_coordsChunkPair.erase(chunkCoords);
			}

			delete chunk;
			chunk = nullptr;
		}
	}

	else
	{
		for (int i = 0; i < MAX_NUM_CHUNK_DELETE_PER_FRAME; ++i)
		{
			if(m_pendingDeleteChunks.empty())
				return;

			Chunk* chunk = m_pendingDeleteChunks.front();
			m_pendingDeleteChunks.pop_front();
			if (chunk)
			{
				IntVec2 chunkCoords = chunk->m_chunkCoords;
				m_coordsChunkPair.erase(chunkCoords);
			}

			delete chunk;
			chunk = nullptr;
		}
	}
}

void World::ProcessChunkLoading()
{
	if(m_shouldReload)
		return;

	if (m_loadChunks)
	{
		double timeStarted = GetCurrentTimeSeconds();
		if ((int)m_coordsChunkPair.size() < MAX_ACTIVE_CHUNKS)
		{
			CreateAllChunksInActivationRadius();
		}

		else
		{
			DeactivateAllChunksOutsideRadius();
		}
		m_loadChunks = (GetCurrentTimeSeconds() - timeStarted) * 1000.0;
	}
}


void World::ProcessDirtyLighting()
{
	if (m_dirtyLightBlocks.empty() || IGNORE_LIGHTING || m_shouldReload)
		return;

	double timeStarted = GetCurrentTimeSeconds();
	int numRunsThroughLoop = 0;

	const uint8_t LAVA_TYPE = BlockDefinition::GetBlockDefinitionIndexFromName("Lava");

	while (!m_dirtyLightBlocks.empty())
	{
		numRunsThroughLoop++;
		BlockIterator iter = m_dirtyLightBlocks.front();
		m_dirtyLightBlocks.pop_front();

		Block* block = iter.GetBlock();
		if (!block)
			continue;

		block->SetIsLightDirty(false);

		int localX = iter.m_blockIndex & CHUNK_MASK_X;
		int localY = (iter.m_blockIndex & CHUNK_MASK_Y) >> CHUNK_BITS_X;
		int localZ = (iter.m_blockIndex & CHUNK_MASK_Z) >> (CHUNK_BITS_X + CHUNK_BITS_Y);

		uint8_t curIndoor = block->GetIndoorLight();
		uint8_t curOutdoor = block->GetOutdoorLight();
		uint8_t desiredIndoor = 0;
		uint8_t desiredOutdoor = 0;

		// handle emissive opaque blocks
		if (block->IsFullOpaque())
		{
			const BlockDefinition* def = BlockDefinition::GetBlockDefinitionFromIndex(block->m_blockType);
			desiredIndoor = (uint8_t)def->m_indoorLighting;

			if (block->m_blockType == LAVA_TYPE)
			{
				desiredOutdoor = MAX_LIGHT_INFLUENCE;
			}
		}


		else
		{
			uint8_t maxOut = 0;
			uint8_t maxIn = 0;

			auto tryCheckNeighbor = [&](int neighborIndex, Chunk* c)
				{
					Block* n = &c->m_blocks[neighborIndex];
					uint8_t nOut = n->GetOutdoorLight();
					uint8_t nIn = n->GetIndoorLight();

					if (nOut > 0) --nOut;
					if (nIn > 0) --nIn;

					if (nOut > maxOut) maxOut = nOut;
					if (nIn > maxIn)  maxIn = nIn;
				};

			// iterate all 6 directions
			auto checkDir = [&](BlockDirection dir, int delta, bool useNeighbor)
				{
					if (useNeighbor)
					{
						BlockIterator n = iter.GetIterator(dir);
						if (Block* b = n.GetBlock()) tryCheckNeighbor(n.m_blockIndex, n.m_chunk);
					}
					else
					{
						tryCheckNeighbor(iter.m_blockIndex + delta, iter.m_chunk);
					}
				};

			// X
			checkDir(BlockDirection::WEST, -STRIDE_X, localX == 0 && iter.m_chunk->m_neighbors[(int)BlockDirection::WEST]);
			checkDir(BlockDirection::EAST, +STRIDE_X, localX == CHUNK_MAX_X && iter.m_chunk->m_neighbors[(int)BlockDirection::EAST]);

			// Y
			checkDir(BlockDirection::SOUTH, -STRIDE_Y, localY == 0 && iter.m_chunk->m_neighbors[(int)BlockDirection::SOUTH]);
			checkDir(BlockDirection::NORTH, +STRIDE_Y, localY == CHUNK_MAX_Y && iter.m_chunk->m_neighbors[(int)BlockDirection::NORTH]);

			// Z (no vertical neighbors)
			if (localZ > 0)           tryCheckNeighbor(iter.m_blockIndex - STRIDE_Z, iter.m_chunk);
			if (localZ < CHUNK_MAX_Z) tryCheckNeighbor(iter.m_blockIndex + STRIDE_Z, iter.m_chunk);

			// Sky blocks get full outdoor, others get neighbor max
			desiredOutdoor = block->IsSky() ? SKY_LIGHT_INFLUENCE : maxOut;
			desiredIndoor = maxIn;
		}

		// apply new values
		bool changedLight = false;
		if (curOutdoor != desiredOutdoor)
		{
			block->SetOutdoorLight(desiredOutdoor);
			changedLight = true;
		}
		if (curIndoor != desiredIndoor)
		{
			block->SetIndoorLight(desiredIndoor);
			changedLight = true;
		}
		if (!changedLight)
			continue;

		if (iter.m_chunk)
			iter.m_chunk->SetDirty();

		bool isDarkening = (desiredOutdoor < curOutdoor) || (desiredIndoor < curIndoor);

		auto tryMark = [&](int neighborIndex, Chunk* c)
			{
				Block* n = &c->m_blocks[neighborIndex];
				if (!n || n->IsFullOpaque())
					return;

				uint8_t nOut = n->GetOutdoorLight();
				uint8_t nIn = n->GetIndoorLight();

				if (isDarkening)
				{
					if (nOut > desiredOutdoor || nIn > desiredIndoor)
						MarkLightingDirty(BlockIterator(c, neighborIndex));
				}
				else if ((nOut + 1 < desiredOutdoor) || (nIn + 1 < desiredIndoor))
				{
					MarkLightingDirty(BlockIterator(c, neighborIndex));
				}
			};

		// X
		if (localX > 0)             tryMark(iter.m_blockIndex - STRIDE_X, iter.m_chunk);
		else if (iter.m_chunk->m_neighbors[(int)BlockDirection::WEST])
			tryMark(iter.GetIterator(BlockDirection::WEST).m_blockIndex, iter.GetIterator(BlockDirection::WEST).m_chunk);

		if (localX < CHUNK_MAX_X)   tryMark(iter.m_blockIndex + STRIDE_X, iter.m_chunk);
		else if (iter.m_chunk->m_neighbors[(int)BlockDirection::EAST])
			tryMark(iter.GetIterator(BlockDirection::EAST).m_blockIndex, iter.GetIterator(BlockDirection::EAST).m_chunk);

		// Y
		if (localY > 0)             tryMark(iter.m_blockIndex - STRIDE_Y, iter.m_chunk);
		else if (iter.m_chunk->m_neighbors[(int)BlockDirection::SOUTH])
			tryMark(iter.GetIterator(BlockDirection::SOUTH).m_blockIndex, iter.GetIterator(BlockDirection::SOUTH).m_chunk);

		if (localY < CHUNK_MAX_Y)   tryMark(iter.m_blockIndex + STRIDE_Y, iter.m_chunk);
		else if (iter.m_chunk->m_neighbors[(int)BlockDirection::NORTH])
			tryMark(iter.GetIterator(BlockDirection::NORTH).m_blockIndex, iter.GetIterator(BlockDirection::NORTH).m_chunk);

		// Z
		if (localZ > 0)             tryMark(iter.m_blockIndex - STRIDE_Z, iter.m_chunk);
		if (localZ < CHUNK_MAX_Z)   tryMark(iter.m_blockIndex + STRIDE_Z, iter.m_chunk);
	}

	m_totalLightProcess++;
	m_processLightSpeed = (GetCurrentTimeSeconds() - timeStarted) * 1000.0;
	m_totalProcessLightSpeed += m_processLightSpeed;
	m_avgProcessLightSpeed = m_totalProcessLightSpeed / m_totalLightProcess;

	if (m_processLightSpeed > m_maxProcessLightSpeed)
	{
		m_maxProcessLightSpeed = m_processLightSpeed;
	}
}

void World::ProcessDirtyLiquid()
{
	if (m_dirtyLiquidBlocks.empty() || m_shouldReload)
		return;

	const uint8_t WATER_TYPE = BlockDefinition::GetBlockDefinitionIndexFromName("Water");
	const uint8_t STONE_TYPE = BlockDefinition::GetBlockDefinitionIndexFromName("Stone");
	const uint8_t OBSIDIAN_TYPE = BlockDefinition::GetBlockDefinitionIndexFromName("Obsidian");
	const uint8_t LAVA_TYPE = BlockDefinition::GetBlockDefinitionIndexFromName("Lava");

	std::deque<DelayedDirtyLiquidBlock> skippedBlocks;

	double timeStarted = GetCurrentTimeSeconds();
	float timeStartedF = (float)timeStarted;

	while (!m_dirtyLiquidBlocks.empty())
	{
		DelayedDirtyLiquidBlock delayedIter = m_dirtyLiquidBlocks.front();
		BlockIterator iter = delayedIter.m_blockIterator;
		m_dirtyLiquidBlocks.pop_front();

		Chunk* chunk = iter.m_chunk;
		if (!chunk)
			continue;

		Block* block = iter.GetBlock();
		if (!block)
			continue;

		if (delayedIter.m_timeAdded > timeStartedF)
		{
			skippedBlocks.push_back(delayedIter);
			continue;
		}


		block->SetIsLiquidDirty(false);

		if(block->IsSolid())
			continue;

		int localZ = ((iter.m_blockIndex & CHUNK_MASK_Z) >> (CHUNK_BITS_X + CHUNK_BITS_Y));

		//----------------------------------------------------------------------
		// STORE OLD FLOW — do NOT overwrite until the end
		//----------------------------------------------------------------------
		bool isSource = block->IsLiquidSource();
		uint8_t oldFlow = block->GetFlowValue();
		uint8_t desiredFlow = isSource ? LIQUID_INFLUENCE : 0;
		uint8_t curBlockType = block->m_blockType;
		uint8_t desiredBlockType = WATER_TYPE;
		if (block->IsLiquid())
		{
			desiredBlockType = curBlockType;
		}

		
		//----------------------------------------------------------------------
		// RULE 1: fed from above?
		//----------------------------------------------------------------------
		bool fedFromAbove = false;

		if (localZ < CHUNK_MAX_Z && !isSource)
		{
			int aboveIndex = iter.m_blockIndex + STRIDE_Z;
			Block* above = &chunk->m_blocks[aboveIndex];

			if (above && above->IsLiquid() && !above->IsSolid())
			{
				uint8_t aboveFlow = above->GetFlowValue();
				desiredFlow = aboveFlow > 0 ? LIQUID_INFLUENCE : 0;
				fedFromAbove = aboveFlow > 0;
				desiredBlockType = above->m_blockType;
			}
		}

		//----------------------------------------------------------------------
		// RULE 2: lateral flow if not fed from above
		//----------------------------------------------------------------------
		
		
		if (!fedFromAbove && !isSource)
		{
			uint8_t maxFlow = 0;

			auto gather = [&](BlockIterator n)
				{
					Block* nb = n.GetBlock();
					if (!nb || nb->IsSolid() || !nb->IsLiquid())
						return;

					uint8_t nf = nb->GetFlowValue();

					if (nf == oldFlow)
						return;

					BlockIterator nIterDown = n.GetIterator(BlockDirection::DOWN);
					Block* nbDown = nIterDown.GetBlock();
					if(!nbDown)
						return;

					if(nbDown->IsAir() || nbDown->IsLiquid())
						return;

					if (nf > maxFlow)
					{
						desiredBlockType = nb->m_blockType;
						maxFlow = nf;
					}
				};

			gather(iter.GetIterator(BlockDirection::NORTH));
			gather(iter.GetIterator(BlockDirection::SOUTH));
			gather(iter.GetIterator(BlockDirection::EAST));
			gather(iter.GetIterator(BlockDirection::WEST));

			if (maxFlow > 0)
				desiredFlow = maxFlow - 1;
			else
				desiredFlow = 0;
		}


		//----------------------------------------------------------------------
		// COMPUTE DRYING *BEFORE* CHANGING FLOW
		//----------------------------------------------------------------------
		bool drying = (desiredFlow < oldFlow) && !isSource;

		//----------------------------------------------------------------------
		// PROPAGATION: DOWN -> sideways if supported
		//----------------------------------------------------------------------
		float flowDelay = 0.f;
		if (desiredBlockType == WATER_TYPE)
		{
			flowDelay = WATER_FLOW_DELAY;
		}

		else if (desiredBlockType == LAVA_TYPE)
		{
			flowDelay = LAVA_FLOW_DELAY;
		}

		float timeToAdd = delayedIter.m_timeAdded < 0.f ? -1.f : delayedIter.m_timeAdded + flowDelay;

		if (desiredFlow > 0 && !drying)
		{
			BlockIterator below = iter.GetIterator(BlockDirection::DOWN);
			Block* bb = below.GetBlock();

			if (bb && (bb->IsAir() || bb->IsLiquid()))
			{
				// direct gravity down
				MarkLiquidDirty(below, timeToAdd);
			}

			else
			{
				// spread sideways if sitting on solid
				auto trySide = [&](BlockDirection dir)
					{
						BlockIterator s = iter.GetIterator(dir);
						Block* sb = s.GetBlock();
						if (sb)
						{
							if (sb->IsAir())
								MarkLiquidDirty(s, timeToAdd);

							else
							{
								uint8_t sbFlow = sb->GetFlowValue();
								if (sbFlow < desiredFlow - 1)
								{
									MarkLiquidDirty(s, timeToAdd);
								}

							}
						}

					};

				trySide(BlockDirection::NORTH);
				trySide(BlockDirection::SOUTH);
				trySide(BlockDirection::EAST);
				trySide(BlockDirection::WEST);
			}
		}


		//----------------------------------------------------------------------
		// DRYING / RE-ADJUSTMENT PROPAGATION (lateral + vertical)
		//----------------------------------------------------------------------
		auto tryDry = [&](BlockDirection direction)
			{
				BlockIterator n = iter.GetIterator(direction);
				Block* nb = n.GetBlock();
				if (!nb || nb->IsSolid())
					return;

				uint8_t nf = nb->GetFlowValue();

				// skip empty air with zero flow
				if (!nb->IsLiquid())// && nf == 0)
					return;


				if (drying)
				{
					float dryDelay = 0.f;
					if (nb->m_blockType == WATER_TYPE)
						dryDelay = WATER_FLOW_DELAY;
					else if (nb->m_blockType == LAVA_TYPE)
						dryDelay = LAVA_FLOW_DELAY;

					float timeToAddDry = delayedIter.m_timeAdded < 0.f ? -1.f : delayedIter.m_timeAdded + dryDelay;

					// drying wave: neighbor has more flow -> re-evaluate
					if (direction == BlockDirection::DOWN)
					{
						if (nf > desiredFlow)
							MarkLiquidDirty(n, timeToAddDry);
					}

					else if (nf >= desiredFlow)
						MarkLiquidDirty(n, timeToAddDry);
				}

				else
				{
					// increasing wave
					if (nf + 1 < desiredFlow)
						MarkLiquidDirty(n, timeToAdd);
				}
			};

		// neighbors

		tryDry(BlockDirection::NORTH);
		tryDry(BlockDirection::SOUTH);
		tryDry(BlockDirection::EAST);
		tryDry(BlockDirection::WEST);
		tryDry(BlockDirection::DOWN);


		//----------------------------------------------------------------------
		// FINALLY APPLY THE NEW FLOW AND TYPE
		//----------------------------------------------------------------------

		if (desiredFlow > 0)
		{
			if (!block->IsLiquid())
			{
				desiredBlockType = isSource ? block->m_blockType : desiredBlockType;
				block->m_blockType = desiredBlockType;
				block->SetIsLiquid(true);
				block->SetIsSolid(false);
				block->SetIsFullOpaque(desiredBlockType == LAVA_TYPE);
				block->SetIsVisible(true);
			}
		}

		else
		{
			// become air
			desiredBlockType = 0;
			block->m_blockType = desiredBlockType;
			block->SetIsLiquid(false);
			block->SetIsSolid(false);
			block->SetIsFullOpaque(false);
			block->SetIsVisible(false);
		}

		if (SOLIDIFY_LIQUID && (desiredBlockType == LAVA_TYPE || desiredBlockType == WATER_TYPE))
		{
			bool testWater = desiredBlockType == LAVA_TYPE;
			bool shouldSolidify = false;
			bool turnToObsidian = false;
			auto trySolidify = [&](BlockDirection direction)
				{

					BlockIterator n = iter.GetIterator(direction);
					Block* nb = n.GetBlock();
					if(!nb)
						return;

					if (testWater && nb->m_blockType == WATER_TYPE)
					{
						uint8_t nFlow = nb->GetFlowValue();

						if (direction == BlockDirection::DOWN && nFlow < LIQUID_INFLUENCE)
						{
							MarkLiquidDirty(n, timeToAdd);
							return;
						}

						shouldSolidify = true;
						if (nFlow == LIQUID_INFLUENCE)
						{
							turnToObsidian = true;
						}
					}

					else if (!testWater && nb->m_blockType == LAVA_TYPE)
					{
						uint8_t nFlow = nb->GetFlowValue();

						if (direction == BlockDirection::DOWN && nFlow < LIQUID_INFLUENCE)
						{
							MarkLiquidDirty(n, timeToAdd);
							return;
						}

						shouldSolidify = true;
						if (nb->IsLiquidSource())
						{
							turnToObsidian = true;
						}
					}

				};

			trySolidify(BlockDirection::UP);
			trySolidify(BlockDirection::NORTH);
			trySolidify(BlockDirection::EAST);
			trySolidify(BlockDirection::SOUTH);
			trySolidify(BlockDirection::WEST);
			trySolidify(BlockDirection::DOWN);

			if (shouldSolidify)
			{
				block->m_blockType = turnToObsidian ? OBSIDIAN_TYPE : STONE_TYPE;
				block->SetIsLiquid(false);
				block->SetIsSolid(true);
				block->SetIsFullOpaque(true);
				block->SetIsVisible(true);
				block->SetIsLiquidSource(false);
				desiredFlow = 0;

				BlockIterator up = iter.GetIterator(BlockDirection::UP);
				MarkLiquidDirty(up, timeToAdd);

				BlockIterator n = up.GetIterator(BlockDirection::NORTH);
				MarkLiquidDirty(n, timeToAdd);

				n = up.GetIterator(BlockDirection::SOUTH);
				MarkLiquidDirty(n, timeToAdd);

				n = up.GetIterator(BlockDirection::EAST);
				MarkLiquidDirty(n, timeToAdd);

				n = up.GetIterator(BlockDirection::WEST);
				MarkLiquidDirty(n, timeToAdd);
			}
		}

		block->SetFlowValue(desiredFlow);

		// mark for meshing + lighting
		chunk->SetDirty();
		MarkLightingDirty(iter);
	}

	m_processLiquidSpeed = (GetCurrentTimeSeconds() - timeStarted) * 1000.0;
	m_totalLiquidProcess++;
	m_totalProcessLiquidSpeed += m_processLiquidSpeed;
	m_avgProcessLiquidSpeed = m_totalProcessLiquidSpeed / m_totalLiquidProcess;
	if (m_processLiquidSpeed > m_maxProcessLiquidSpeed)
	{
		m_maxProcessLiquidSpeed = m_processLiquidSpeed;
	}

	m_dirtyLiquidBlocks = skippedBlocks;
}


void World::ProcessPendingActivation()
{
	if (m_pendingActivationChunks.empty() || m_shouldReload)
		return;

	Vec2 playerPosXY = g_game->m_player->m_position.GetXY();
	IntVec2 startingCoords = GetChunkCoordsFromPosition(g_game->m_player->m_position);
	Vec2 startingPos = GetGlobalCenterPosFromChunkCoords(startingCoords);
	std::queue<IntVec2>chunkCoordsQueue;
	chunkCoordsQueue.push(startingCoords);
	std::unordered_set<IntVec2> checkedCoordsSet;
	constexpr float ACTIVATION_SQRD = (float)CHUNK_ACTIVATION_RANGE * CHUNK_ACTIVATION_RANGE;
	constexpr float DEACTIVATION_SQRD = (float)CHUNK_DEACTIVATION_RANGE * CHUNK_DEACTIVATION_RANGE;


	IntVec2 currentCoords = startingCoords;
	int numActivations = 0;

	double currTime = GetCurrentTimeSeconds();

	while (!chunkCoordsQueue.empty() && numActivations < MAX_NUM_CHUNK_ACTIVATIONS_PER_FRAME)
	{
		currentCoords = chunkCoordsQueue.front();
		auto found = m_pendingActivationChunks.find(currentCoords);
		if (found != m_pendingActivationChunks.end())
		{
			Chunk* pendingChunk = found->second;
			if (pendingChunk)
			{
			
				Vec2 centerPosToCheck = GetGlobalCenterPosFromChunkCoords(currentCoords);
				if (GetDistanceSquared2D(centerPosToCheck, startingPos) >= ACTIVATION_SQRD)
				{
					m_pendingActivationChunks.erase(currentCoords);
					m_dirtyChunks.erase(currentCoords);
					pendingChunk->ClearNeighbors();
					m_pendingDeleteChunks.push_back(pendingChunk);
					//numActivations++;
				}
			

				if (pendingChunk->HasAllNeighbors() )
				{
					pendingChunk->ActivateLiquidData();
					pendingChunk->ActivateLightingData();
					m_pendingActivationChunks.erase(currentCoords);
					ActivateChunk(pendingChunk);
					numActivations++;

					double activationTime = currTime - pendingChunk->m_timeCreated;
					m_totalActivations++;
					m_totalActivationSpeed += activationTime;
					m_avgActivationSpeed = m_totalActivationSpeed / m_totalActivations;
					m_maxActivationSpeed = GetMax(m_avgActivationSpeed, m_maxActivationSpeed);
				}

			}
		}

		for (int i = 0; i < IntVec2::NUM_DIRECTIONS_N_E_S_W_NE_SE_SW_NW; ++i)
		{
			IntVec2 coordsToCheck = currentCoords + IntVec2::DIRECTIONS_N_E_S_W_NE_SE_SW_NW[i];
			if (checkedCoordsSet.find(coordsToCheck) != checkedCoordsSet.end()) //coords have already been checked
				continue;

			//If chunk is outside activation range do not add chunk or add to queue for checking neighbors
			Vec2 centerPosToCheck = GetGlobalCenterPosFromChunkCoords(coordsToCheck);
			float distanceSqrd = GetDistanceSquared2D(centerPosToCheck, startingPos);
			if (distanceSqrd > DEACTIVATION_SQRD)
				continue;

			chunkCoordsQueue.push(coordsToCheck);
			checkedCoordsSet.insert(coordsToCheck);
		}

		chunkCoordsQueue.pop();
	}
}

void World::CreateAllChunksInActivationRadius()
{

	Vec2 playerPosXY = g_game->m_player->m_position.GetXY();
	IntVec2 startingCoords = GetChunkCoordsFromPosition(g_game->m_player->m_position);
	Vec2 startingPos = GetGlobalCenterPosFromChunkCoords(startingCoords);

	//If chunk that player is on is not loaded, load that one first
	if (!DoesChunkExist(startingCoords))
	{
		AddNewChunkAtCoords(startingCoords);
	}

	//Spread out from player chunk, check if surrounding chunks are loaded, if not load, if so keep spreading out until you reach activation range
	std::queue<IntVec2>chunkCoordsQueue;
	chunkCoordsQueue.push(startingCoords);
	std::unordered_set<IntVec2> checkedCoordsSet;

	constexpr float ACTIVATION_SQRD = (float)(CHUNK_ACTIVATION_RANGE * CHUNK_ACTIVATION_RANGE);

	IntVec2 currentCoords = startingCoords;
	while (!chunkCoordsQueue.empty() && (m_coordsChunkPair.size() < MAX_ACTIVE_CHUNKS) && (m_numSentJobs < MAX_NUM_CHUNK_JOBS))
	{
		currentCoords = chunkCoordsQueue.front();
		for (int i = 0; i < IntVec2::NUM_DIRECTIONS_N_E_S_W; ++i)
		{
			IntVec2 coordsToCheck = currentCoords + IntVec2::DIRECTIONS_N_E_S_W[i];
			if (checkedCoordsSet.find(coordsToCheck) != checkedCoordsSet.end()) //coords have already been checked
				continue;

			//If chunk is outside activation range do not add chunk or add to queue for checking neighbors
			Vec2 centerPosToCheck = GetGlobalCenterPosFromChunkCoords(coordsToCheck);
			if (GetDistanceSquared2D(centerPosToCheck, startingPos) > ACTIVATION_SQRD)
				continue;

			if (!DoesChunkExist(coordsToCheck))
			{
				AddNewChunkAtCoords(coordsToCheck);
			}

			chunkCoordsQueue.push(coordsToCheck);
			checkedCoordsSet.insert(coordsToCheck);
		}

		chunkCoordsQueue.pop();
	}
}

void World::DeactivateAllChunksOutsideRadius()
{
	Vec2 playerPosXY = g_game->m_player->m_position.GetXY();
	float deactivationSquared = (float)(CHUNK_DEACTIVATION_RANGE * CHUNK_DEACTIVATION_RANGE);

	for (int i = 0; i < m_activeChunks.size(); ++i)
	{
		Chunk* chunk = m_activeChunks[i];
		if (chunk == nullptr)
			continue;

		float distanceSqrd = GetDistanceSquared2D(chunk->GetCenterXYCoords(), playerPosXY);
		if (distanceSqrd > deactivationSquared)
		{			
			m_activeChunks[i] = nullptr;
			DeleteChunk(chunk);
		}
	}
}

void World::ActivateChunk(Chunk* chunk)
{
	chunk->SetDirty();
	for (int i = 0; i < (int)m_activeChunks.size(); ++i)
	{
		if (m_activeChunks[i] == nullptr)
		{
			m_activeChunks[i] = chunk;
			return;
		}
	}

	m_activeChunks.push_back(chunk);
}

void World::DeleteChunk(Chunk* chunk)
{
	if(!chunk)
		return;

	UndirtyLightForAllBlocksInChunk(chunk);
	UndirtyLiquidForAllBlocksInChunk(chunk);

	m_dirtyChunks.erase(chunk->m_chunkCoords);

	m_pendingActivationChunks.erase(chunk->m_chunkCoords);


	if (chunk->m_needsSaving)
	{
		chunk->m_state = ChunkState::PENDING_SAVE;
		m_needsSavingChunks.push_back(chunk);
	}

	else
	{
		chunk->m_state = ChunkState::GARBAGE;
		chunk->ClearNeighbors();
		m_pendingDeleteChunks.push_back(chunk);
	}
}

bool World::RegenerateDirtyChunks(double timeUpdateStarted)
{
	if(m_dirtyChunks.empty())
		return false;

	Vec2 playerPosXY = g_game->m_player->m_position.GetXY();
	IntVec2 startingCoords = GetChunkCoordsFromPosition(g_game->m_player->m_position);
	Vec2 startingPos = GetGlobalCenterPosFromChunkCoords(startingCoords);
	std::queue<IntVec2>chunkCoordsQueue;
	chunkCoordsQueue.push(startingCoords);
	std::unordered_set<IntVec2> checkedCoordsSet;
	constexpr float ACTIVATION_SQRD = (float)(CHUNK_ACTIVATION_RANGE * CHUNK_ACTIVATION_RANGE);

	IntVec2 currentCoords = startingCoords;
	int numRebuiltChunks = 0;

	while (!chunkCoordsQueue.empty() && numRebuiltChunks < MAX_NUM_CHUNK_REGENS_PER_FRAME)
	{
		double currTime = GetCurrentTimeSeconds();
		
		if(numRebuiltChunks > 0 && currTime - timeUpdateStarted > MAX_TIME_WORLD_UPDATE)
			return numRebuiltChunks > 0;
		

		currentCoords = chunkCoordsQueue.front();
		auto found = m_dirtyChunks.find(currentCoords);
		if (found != m_dirtyChunks.end())
		{
			Chunk* dirtyChunk = found->second;
			if (dirtyChunk && dirtyChunk->AllNeighborsFullyActivated())
			{
				dirtyChunk->Regenerate();
				double waitMeshSpeed = currTime - dirtyChunk->m_timeDirtied;
				m_totalMeshWaitSpeed += waitMeshSpeed;
				m_totalMeshWaits++;
				m_avgMeshWaitSpeed = m_totalMeshWaitSpeed / m_totalMeshWaits;
				m_maxMeshWaitSpeed = GetMax(m_avgMeshWaitSpeed, m_maxMeshWaitSpeed);

				numRebuiltChunks++;
				m_dirtyChunks.erase(currentCoords);
			}
		}

		for (int i = 0; i < IntVec2::NUM_DIRECTIONS_N_E_S_W_NE_SE_SW_NW; ++i)
		{
			IntVec2 coordsToCheck = currentCoords + IntVec2::DIRECTIONS_N_E_S_W_NE_SE_SW_NW[i];
			if (checkedCoordsSet.find(coordsToCheck) != checkedCoordsSet.end()) //coords have already been checked
				continue;

			//If chunk is outside activation range do not add chunk or add to queue for checking neighbors
			Vec2 centerPosToCheck = GetGlobalCenterPosFromChunkCoords(coordsToCheck);
			if (GetDistanceSquared2D(centerPosToCheck, startingPos) > ACTIVATION_SQRD)
				continue;

			chunkCoordsQueue.push(coordsToCheck);
			checkedCoordsSet.insert(coordsToCheck);
		}

		chunkCoordsQueue.pop();
	}

	return numRebuiltChunks > 0;
}

void World::AddNewChunkAtCoords(IntVec2 const& coords)
{
	Chunk* newChunk = new Chunk(this, coords);

	//Attempt to load chunk from disk and generate if no file exists
	std::string fileName = Stringf("Saves/Chunk(%i,%i).chunk", coords.x, coords.y);
	std::vector<uint8_t> fileBuffer;
	int fileSize = FileReadToBuffer(fileBuffer, fileName, true);
	if (fileSize <= (int)sizeof(FileHeader)) // file does not exist or is too small to be consistent with header file
	{
		ExecuteChunkJob(newChunk, ChunkJobType::GENERATE);
	}

	else
	{
		ExecuteChunkJob(newChunk, ChunkJobType::LOAD);
	}

	AddChunkCoordsPair(coords, newChunk);
}

void World::AddChunkCoordsPair(IntVec2 const& key, Chunk* value)
{
	auto found = m_coordsChunkPair.find(key);
	if (found != m_coordsChunkPair.end())
	{
		found->second = value;
		return;
	}

	m_coordsChunkPair.insert({key, value});
}

//Job Management
//-------------------------------------------------------------------------------------
void World::ExecuteChunkJob(Chunk* chunk, ChunkJobType jobType)
{
	if (!chunk)
		return;

	Job* job = nullptr;
	switch (jobType)
	{
	case ChunkJobType::GENERATE:
		chunk->m_state = ChunkState::PENDING_GENERATION;
		job = new ChunkGenerateJob(chunk);
		m_numInFlightGenerate++;
		break;
	case ChunkJobType::LOAD:
		chunk->m_state = ChunkState::PENDING_LOAD;
		job = new ChunkLoadJob(chunk);
		m_numInFlightLoad++;
		break;
	case ChunkJobType::SAVE:
		chunk->m_state = ChunkState::PENDING_SAVE;
		job = new ChunkSaveJob(chunk);
		m_numInFlightSave++;
		break;
	default:
		break;
	}

	if (job)
	{
		m_pendingChunks.insert(chunk);
		m_numSentJobs++;
		g_jobSystem->SubmitJob(job);
	}
}

void World::ProcessFinishedChunkJobs()
{
	std::vector<Job*> finishedJobs = g_jobSystem->RetrieveCompletedJobs();
	double currTime = GetCurrentTimeSeconds();
	for (int i = 0; i < (int)finishedJobs.size(); ++i)
	{
		ChunkLoadJob* loadJob = dynamic_cast<ChunkLoadJob*>(finishedJobs[i]);
		if (loadJob)
		{
			m_pendingChunks.erase(loadJob->m_chunk);
			loadJob->m_chunk->m_state = ChunkState::ACTIVE;
			loadJob->m_chunk->InitNeighbors();
			m_pendingActivationChunks.insert({loadJob->m_chunk->m_chunkCoords, loadJob->m_chunk});
			loadJob->m_chunk->m_timeCreated = currTime;
			m_numInFlightLoad--;
			double loadSpeed = currTime - loadJob->m_timeStarted;
			m_totalChunkLoadSpeed += loadSpeed;
			m_totalLoadJobs++;
			m_avgChunkLoadSpeed = m_totalChunkLoadSpeed / m_totalLoadJobs;
			m_maxChunkLoadSpeed = GetMax(m_avgChunkLoadSpeed, m_maxChunkLoadSpeed);
			continue;
		}

		ChunkGenerateJob* generateJob = dynamic_cast<ChunkGenerateJob*>(finishedJobs[i]);
		if (generateJob)
		{
			m_pendingChunks.erase(generateJob->m_chunk);
			generateJob->m_chunk->m_state = ChunkState::ACTIVE;
			generateJob->m_chunk->InitNeighbors();
			m_pendingActivationChunks.insert({ generateJob->m_chunk->m_chunkCoords, generateJob->m_chunk });
			generateJob->m_chunk->m_timeCreated = currTime;
			m_numInFlightGenerate--;
			double generateSpeed = currTime - generateJob->m_timeStarted;
			m_totalChunkGenerateSpeed += generateSpeed;
			m_totalGenerateJobs++;
			m_avgChunkGenerateSpeed = m_totalChunkGenerateSpeed / m_totalGenerateJobs;
			m_maxChunkGenerateSpeed = GetMax(m_avgChunkGenerateSpeed, m_maxChunkGenerateSpeed);
			continue;
		}

		ChunkSaveJob* saveJob = dynamic_cast<ChunkSaveJob*>(finishedJobs[i]);
		if (saveJob)
		{
			m_pendingChunks.erase(saveJob->m_chunk);
			saveJob->m_chunk->m_state = ChunkState::SAVED;
			saveJob->m_chunk->ClearNeighbors();
			saveJob->m_chunk->m_state = ChunkState::GARBAGE;
			m_pendingDeleteChunks.push_back(saveJob->m_chunk);
			m_numInFlightSave--;
			continue;
		}

		ERROR_AND_DIE("World Received Job that wasn't a chunk job");
	}

	for (int i = 0; i < (int)finishedJobs.size(); ++i)
	{
		delete finishedJobs[i];
		finishedJobs[i] = nullptr;
		m_numSentJobs--;
	}

}

void World::AdjustPlayerStartingPosition()
{
	Vec3 startPos = g_game->m_player->m_position;
	startPos.z = (float)CHUNK_MAX_Z;

	IntVec2 chunkCoords = GetChunkCoordsFromPosition(startPos);
	Chunk* chunk = GetChunkFromChunkCoords(chunkCoords);
	if(!chunk)
		return;

	int blockIndex = chunk->GetBlockIndexFromGlobalCoords(IntVec3(startPos));
	if(blockIndex < 0)
		return;

	float playerNewZHeight = startPos.z + 2.f;
	BlockIterator iter(chunk,blockIndex);
	Block* block = iter.GetBlock();
	while (block && !block->IsSolid())
	{
		iter = iter.GetIterator(BlockDirection::DOWN);
		block = iter.GetBlock();
		playerNewZHeight--;
	}

	Vec3 newPos = startPos;
	newPos.z = playerNewZHeight;

	g_game->m_player->Teleport(newPos);
	g_game->m_player->RestartJumpTimer();
}

//Helpers
//-------------------------------------------------------------------------------------

IntVec2 World::GetChunkCoordsFromPosition(Vec3 const& pos) const
{
	return IntVec2((int)floorf(pos.x / CHUNK_SIZE_X), (int)floorf(pos.y / CHUNK_SIZE_Y));
}

Chunk* World::GetChunkFromChunkCoords(IntVec2 const& chunkCoords) const
{
	auto found = m_coordsChunkPair.find(chunkCoords);
	if (found != m_coordsChunkPair.end())
	{
		Chunk* chunk = found->second;
		if(chunk && chunk->m_state == ChunkState::ACTIVE)
			return chunk;
	}
		

	return nullptr;
}

Vec2 World::GetGlobalCenterPosFromChunkCoords(IntVec2 const& chunkCoords) const
{
	IntVec2 globalCoords(chunkCoords.x * CHUNK_SIZE_X, chunkCoords.y * CHUNK_SIZE_Y);
	return Vec2((float)globalCoords.x + ((float)CHUNK_SIZE_X * 0.5f), (float)globalCoords.y + ((float)CHUNK_SIZE_Y * 0.5f));
}

bool World::DoesChunkExistAtPosition(Vec3 const& pos) const
{
	IntVec2 chunkCoords = IntVec2((int)floorf(pos.x / CHUNK_SIZE_X), (int)floorf(pos.y / CHUNK_SIZE_Y));
	auto found = m_coordsChunkPair.find(chunkCoords);
	if (found != m_coordsChunkPair.end())
		return true;

	return false;
}

float World::GetBaseTerrainDensityAtPosition(Vec3 const& pos) const
{
	NoiseInfo noiseInfo = m_worldDef->GetNoiseInfoFromType(WorldNoiseType::BASE_TERRAIN);
	float baseTerrainNoise = 0.f;
	if (noiseInfo.m_isActive)
	{
		switch (noiseInfo.m_noiseType)
		{
		case NoiseType::PERLIN:
			baseTerrainNoise = Compute3dPerlinNoise(pos.x, pos.y, pos.z, noiseInfo.m_scale, noiseInfo.m_numOctaves, m_worldDef->m_defaultOctavePersistance, m_worldDef->m_defaultOctaveScale, true, noiseInfo.m_seed);
		break;
		case NoiseType::FRACTAL:
			baseTerrainNoise = Compute3dFractalNoise(pos.x, pos.y, pos.z, noiseInfo.m_scale, noiseInfo.m_numOctaves, m_worldDef->m_defaultOctavePersistance, m_worldDef->m_defaultOctaveScale, true, noiseInfo.m_seed);
		break;
		case NoiseType::RAW_NEG_ONE_TO_ONE:
			baseTerrainNoise = Get3dNoiseNegOneToOne((int)pos.x, (int)pos.y, (int)pos.z, noiseInfo.m_seed);
		break;
		case NoiseType::RAW_ZERO_TO_ONE:
			baseTerrainNoise = Get3dNoiseZeroToOne((int)pos.x, (int)pos.y, (int)pos.z, noiseInfo.m_seed);
		default:
			baseTerrainNoise = Compute3dFractalNoise(pos.x, pos.y, pos.z, noiseInfo.m_scale, noiseInfo.m_numOctaves, m_worldDef->m_defaultOctavePersistance, m_worldDef->m_defaultOctaveScale, true, noiseInfo.m_seed);
			break;
		}
		
	}

	return baseTerrainNoise;
}

float World::GetCavesNoiseAtPosition(Vec3 const& pos, int terrainHeight, float surfacePierceNoise, float slope) const
{

	NoiseInfo caveNoiseInfo = m_worldDef->GetNoiseInfoFromType(WorldNoiseType::CAVES);
	NoiseInfo tunnelNoiseInfo = m_worldDef->GetNoiseInfoFromType(WorldNoiseType::TUNNELS);
	if (!caveNoiseInfo.m_isActive && !tunnelNoiseInfo.m_isActive)
	{
		return 0.f;
	}

	Vec2 warpedCoords = Vec2
	(
		Compute2dFractalNoise(pos.x, pos.y, 900.f, 3, m_worldDef->m_defaultOctavePersistance, m_worldDef->m_defaultOctaveScale, true, m_worldDef->m_gameSeed + 25),
		Compute2dFractalNoise(pos.x, pos.y, 800.f, 3, m_worldDef->m_defaultOctavePersistance, m_worldDef->m_defaultOctaveScale, true, m_worldDef->m_gameSeed + 10)
	);

	float depthBelowSurface = (float)terrainHeight - pos.z;


	float baseCave = 0.f;
	if (caveNoiseInfo.m_isActive)
	{
		Vec2 noiseCoords = pos.GetXY() + (warpedCoords * caveNoiseInfo.m_warpStrength);
		switch (caveNoiseInfo.m_noiseType)
		{
		case NoiseType::PERLIN:
			baseCave = Compute3dPerlinNoise(noiseCoords.x, noiseCoords.y, pos.z, caveNoiseInfo.m_scale, caveNoiseInfo.m_numOctaves, m_worldDef->m_defaultOctavePersistance, m_worldDef->m_defaultOctaveScale, true, caveNoiseInfo.m_seed);
			break;
		case NoiseType::FRACTAL:
			baseCave = Compute3dFractalNoise(noiseCoords.x, noiseCoords.y, pos.z, caveNoiseInfo.m_scale, caveNoiseInfo.m_numOctaves, m_worldDef->m_defaultOctavePersistance, m_worldDef->m_defaultOctaveScale, true, caveNoiseInfo.m_seed);
			break;
		case NoiseType::RAW_NEG_ONE_TO_ONE:
			baseCave = Get3dNoiseNegOneToOne((int)noiseCoords.x, (int)noiseCoords.y, (int)pos.z, caveNoiseInfo.m_seed);
			break;
		case NoiseType::RAW_ZERO_TO_ONE:
			baseCave = Get3dNoiseZeroToOne((int)noiseCoords.x, (int)noiseCoords.y, (int)pos.z, caveNoiseInfo.m_seed);
			break;
		default:
			baseCave = Compute3dFractalNoise(noiseCoords.x, noiseCoords.y, pos.z, caveNoiseInfo.m_scale, caveNoiseInfo.m_numOctaves, m_worldDef->m_defaultOctavePersistance, m_worldDef->m_defaultOctaveScale, true, caveNoiseInfo.m_seed);
			break;
		}
		float depthFactor = GetClampedFractionWithinRange(depthBelowSurface, (float)m_worldDef->m_cavesStartDepth, (float)m_worldDef->m_cavesFade);
		depthFactor = SmoothStep3(depthFactor);
		baseCave *= depthFactor * 0.75f;

	}

	float ridgedTunnel = 0.f;
	if (tunnelNoiseInfo.m_isActive)
	{
		// Keep full 3D frequency balance
		Vec3 noiseCoords(pos.x, pos.y, pos.z);

		// Gentle horizontal warp (no vertical stretching)
		float warpAmp = 30.f;
		float warpX = Compute3dPerlinNoise(pos.x * 0.004f, pos.y * 0.004f, pos.z * 0.004f,
			400.f, 3, 0.5f, 2.0f, true, tunnelNoiseInfo.m_seed + 100);
		float warpY = Compute3dPerlinNoise(pos.x * 0.004f, pos.y * 0.004f, pos.z * 0.004f,
			400.f, 3, 0.5f, 2.0f, true, tunnelNoiseInfo.m_seed + 200);
		noiseCoords.x += warpX * warpAmp;
		noiseCoords.y += warpY * warpAmp;

		// Optionally modulate z-warp a little to break sheets
		noiseCoords.z += 10.f * Compute3dPerlinNoise(pos.x * 0.002f, pos.y * 0.002f, pos.z * 0.002f,
			250.f, 2, 0.5f, 2.0f, true, tunnelNoiseInfo.m_seed + 321);

		ridgedTunnel = Compute3dRidgedNoise(
			noiseCoords.x, noiseCoords.y, noiseCoords.z,
			tunnelNoiseInfo.m_scale,
			tunnelNoiseInfo.m_numOctaves,
			m_worldDef->m_defaultOctavePersistance,
			m_worldDef->m_defaultOctaveScale,
			true,
			tunnelNoiseInfo.m_ridgeGain,
			tunnelNoiseInfo.m_seed);

		// Shape and thin
		ridgedTunnel = 1.f - fabsf(ridgedTunnel);
		ridgedTunnel = powf(ridgedTunnel, 2.3f);

		// Bias toward deeper regions (don’t kill surface ones entirely)
		float depthFade = GetClampedFractionWithinRange(depthBelowSurface, -25.f, (float)m_worldDef->m_cavesFade);
		depthFade = SmoothStep3(depthFade);

		float slopeBias = Lerp(0.5f, 1.0f, slope); // 50% persistence on flats, full on cliffs

		float fadeWeight = Lerp(0.4f, 1.0f, depthFade); // stronger across all depths
		ridgedTunnel *= fadeWeight * slopeBias;

		// Slight chance to breach surface
		if (surfacePierceNoise > 0.95f)
		{
			float surfaceBlend = 1.f - SmoothStep3(GetClampedFractionWithinRange(depthBelowSurface, -10.f, 20.f));
			ridgedTunnel += 0.2f * ridgedTunnel * surfaceBlend;
		}
	}


	return GetClampedZeroToOne(baseCave + ridgedTunnel);
}

float World::GetHeightOffsetFromNoiseValues(NoiseValues const& values) const
{
	float terrainHeightOffset = 0.f;

	//Continentalness
	WorldNoiseType noiseType = WorldNoiseType::CONTINENTALNESS;
	NoiseInfo noiseInfo = m_worldDef->GetNoiseInfoFromType(noiseType);
	float continentalness = values.m_noiseValues[(int)noiseType];
	bool activeContinentalness = noiseInfo.m_isActive;

	if (noiseInfo.m_heightOffsetCurve && activeContinentalness)
	{
		continentalness = noiseInfo.m_heightOffsetCurve->EvaluateAt(values.m_noiseValues[(int)noiseType]);
		terrainHeightOffset += continentalness;
	}

	
	//Erosion
	noiseType = WorldNoiseType::EROSION;
	noiseInfo = m_worldDef->GetNoiseInfoFromType(noiseType);

	if (noiseInfo.m_heightOffsetCurve)
	{
		float erosionNoise = values.m_noiseValues[(int)noiseType];
		float erosionInput = activeContinentalness ? continentalness : erosionNoise;
		float erosion = noiseInfo.m_heightOffsetCurve->EvaluateAt(erosionInput);
		terrainHeightOffset += erosion * erosionNoise;
	}

	
	//Peaks and Valleys
	noiseType = WorldNoiseType::PEAKS_VALLEYS;
	noiseInfo = m_worldDef->GetNoiseInfoFromType(noiseType);

	if (noiseInfo.m_heightOffsetCurve)
	{
		float pvNoise = values.m_noiseValues[(int)noiseType];
		float pvInput = activeContinentalness ? continentalness : pvNoise;
		float pv = noiseInfo.m_heightOffsetCurve->EvaluateAt(pvInput);
		terrainHeightOffset += pv * pvNoise;
	}
	
	

	return terrainHeightOffset;
}

float World::GetSquashFactorFromNoiseValues(NoiseValues const& values) const 
{
	float squashFactor = 0.f;
	
	//Continentalness
	WorldNoiseType noiseType = WorldNoiseType::CONTINENTALNESS;
	NoiseInfo noiseInfo = m_worldDef->GetNoiseInfoFromType(noiseType);
	float continentalnessSquash = values.m_noiseValues[(int)noiseType];

	if (noiseInfo.m_squashCurve && noiseInfo.m_isActive)
	{
		continentalnessSquash = noiseInfo.m_squashCurve->EvaluateAt(values.m_noiseValues[(int)noiseType]);
		squashFactor += continentalnessSquash;
	}

	//Erosion
	noiseType = WorldNoiseType::EROSION;
	noiseInfo = m_worldDef->GetNoiseInfoFromType(noiseType);

	if (noiseInfo.m_squashCurve && noiseInfo.m_isActive)
	{
		float erosionSquash = noiseInfo.m_squashCurve->EvaluateAt(continentalnessSquash);
		squashFactor += erosionSquash * values.m_noiseValues[(int)noiseType];
	}

	//Peaks And Valleys
	noiseType = WorldNoiseType::PEAKS_VALLEYS;
	noiseInfo = m_worldDef->GetNoiseInfoFromType(noiseType);

	if (noiseInfo.m_squashCurve && noiseInfo.m_isActive)
	{
		float pvSquash = noiseInfo.m_squashCurve->EvaluateAt(continentalnessSquash);
		squashFactor += pvSquash * values.m_noiseValues[(int)noiseType];
	}

	
	return squashFactor;
}

bool World::IsPositionWater(Vec3 const& position) const
{
	IntVec2 chunkCoords = GetChunkCoordsFromPosition(position);
	Chunk* chunk = GetChunkFromChunkCoords(chunkCoords);
	if(!chunk)
		return false;

	int blockIndex = chunk->GetBlockIndexFromGlobalCoords(position);
	if(blockIndex < 0 || blockIndex >= NUM_BLOCKS_IN_CHUNK)
		return false;

	uint8_t blockType = chunk->m_blocks[blockIndex].m_blockType;
	BlockDefinition* blockDef = BlockDefinition::GetBlockDefinitionFromIndex(blockType);
	return blockDef->IsWater();
}

BlockIterator World::GetBlockIteratorAtPosition(Vec3 const& position) const
{
	IntVec2 chunkCoords = GetChunkCoordsFromPosition(position);
	Chunk* chunk = GetChunkFromChunkCoords(chunkCoords);
	if (!chunk)
		return BlockIterator(nullptr, -1);

	int blockIndex = chunk->GetBlockIndexFromGlobalCoords(position);
	if (blockIndex < 0 || blockIndex >= NUM_BLOCKS_IN_CHUNK)
		return BlockIterator(nullptr, -1);

	return BlockIterator(chunk, blockIndex);
}

NoiseValues World::Get2DNoiseValuesAtPosition(Vec2 const& pos)
{
	Vec2 warpedCoords = Vec2
	(
		Compute2dFractalNoise(pos.x, pos.y, 900.f, 3, m_worldDef->m_defaultOctavePersistance, m_worldDef->m_defaultOctaveScale, true, m_worldDef->m_gameSeed + 25),
		Compute2dFractalNoise(pos.x, pos.y, 800.f, 3, m_worldDef->m_defaultOctavePersistance, m_worldDef->m_defaultOctaveScale, true, m_worldDef->m_gameSeed + 10)
	);

	NoiseValues values;
	//Skips Base_Terrain and Caves/Tunnels since that is 3D noise
	for (int i = 1; i < (int)WorldNoiseType::CAVES; ++i)
	{
		WorldNoiseType noiseType = (WorldNoiseType)i;
		NoiseInfo noiseInfo = m_worldDef->GetNoiseInfoFromType(noiseType);

		
		if (!noiseInfo.m_isActive)
		{
			values.m_noiseValues[i] = 0.f;
			continue;
		}
		

		Vec2 noiseCoords = pos + (warpedCoords * noiseInfo.m_warpStrength);
		switch (noiseInfo.m_noiseType)
		{
		case::NoiseType::RAW_ZERO_TO_ONE:
			values.m_noiseValues[i] = Get2dNoiseZeroToOne((int)noiseCoords.x, (int)noiseCoords.y, noiseInfo.m_seed);
			break;
		case::NoiseType::RAW_NEG_ONE_TO_ONE:
			values.m_noiseValues[i] = Get2dNoiseNegOneToOne((int)noiseCoords.x, (int)noiseCoords.y, noiseInfo.m_seed);
			break;
		case::NoiseType::FRACTAL:
			values.m_noiseValues[i] = Compute2dFractalNoise(noiseCoords.x, noiseCoords.y, noiseInfo.m_scale, noiseInfo.m_numOctaves, m_worldDef->m_defaultOctavePersistance, m_worldDef->m_defaultOctaveScale, true, noiseInfo.m_seed);
			break;
		case::NoiseType::PERLIN:
			values.m_noiseValues[i] = Compute2dPerlinNoise(noiseCoords.x, noiseCoords.y, noiseInfo.m_scale, noiseInfo.m_numOctaves, m_worldDef->m_defaultOctavePersistance, m_worldDef->m_defaultOctaveScale, true, noiseInfo.m_seed);
			break;
		default:
			values.m_noiseValues[i] = Get2dNoiseZeroToOne((int)noiseCoords.x, (int)noiseCoords.y, noiseInfo.m_seed);
			break;
		}
	}

	return values;
}


bool World::DoesChunkExist(IntVec2 const& chunkCoords)
{
	auto found = m_coordsChunkPair.find(chunkCoords);
	if (found != m_coordsChunkPair.end() && found->second != nullptr)
	{
		return true;
	}

	return false;
}

bool World::DoesChunkExist(IntVec2 const& chunkCoords, Chunk*& out_foundChunk)
{
	auto found = m_coordsChunkPair.find(chunkCoords);
	if (found != m_coordsChunkPair.end() && found->second != nullptr)
	{
		out_foundChunk = found->second;
		return true;
	}

	out_foundChunk = nullptr;
	return false;
}

bool World::IsActiveChunk(IntVec2 const& chunkCoords)
{
	Chunk* foundChunk = nullptr;
	auto found = m_coordsChunkPair.find(chunkCoords);
	if (found != m_coordsChunkPair.end() && found->second != nullptr)
	{
		foundChunk = found->second;
	}

	if(foundChunk && foundChunk->m_state == ChunkState::ACTIVE)
		return true;

	return false;
}

bool World::IsActiveChunk(IntVec2 const& chunkCoords, Chunk*& out_foundChunk)
{
	auto found = m_coordsChunkPair.find(chunkCoords);
	if (found != m_coordsChunkPair.end() && found->second != nullptr)
	{
		out_foundChunk = found->second;
	}

	if (out_foundChunk && out_foundChunk->m_state == ChunkState::ACTIVE)
		return true;

	out_foundChunk = nullptr;
	return false;
}

bool World::IsChunkPending(Chunk* chunk) const
{
	return m_pendingChunks.find(chunk) != m_pendingChunks.end();
}



