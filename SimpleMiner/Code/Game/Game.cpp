#include "Game/Game.hpp"

#include "Game/GameCommon.hpp"
#include "Game/App.hpp"
#include "Game/Player.hpp"
#include "Game/BlockDefinition.hpp"
#include "Game/World.hpp"
#include "Game/WorldDefinition.hpp"
#include "Game/GameCamera.hpp"
#include "Game/LitTnt.hpp"

#include <Engine/Core/ErrorWarningAssert.hpp>
#include "Engine/Renderer/RendererDX11.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/Timer.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Window/Window.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Renderer/DebugRender.hpp"
#include "Engine/Core/JobSystem.hpp"
#include "Engine/Core/Time.hpp"
#include <vector>


RandomNumberGenerator* g_rng;
bool g_debugMode = false;
bool g_invertMouseX = false;
bool g_invertMouseY = false;
bool g_invertControllerViewX = false;
bool g_invertControllerViewY = false;
float g_mouseSensitivity = 0.125f;
float g_controllerSensitivity = 76.f;

float g_SCREEN_SIZE_X = 1600.f;
float g_SCREEN_SIZE_Y = 800.f;
float g_SCREEN_CENTER_X = g_SCREEN_SIZE_X / 2.f;
float g_SCREEN_CENTER_Y = g_SCREEN_SIZE_Y / 2.f;


Game::Game()
{

}

Game::~Game()
{
	delete m_player;
	m_player = nullptr;
	delete m_currentWorld;
	m_currentWorld = nullptr;
	delete m_screenCamera;
	m_screenCamera = nullptr;
	delete g_rng;
	g_rng = nullptr;
	delete m_gameClock;
	m_gameClock = nullptr;
}

//Game management
//--------------------------------------------------------------------
void Game::Startup()
{
	m_screenCamera = new Camera();
	IntVec2 screenDims = g_window->GetClientDimensions();
	g_SCREEN_SIZE_X = (float)screenDims.x;
	g_SCREEN_SIZE_Y = (float)screenDims.y;
	g_SCREEN_CENTER_X = g_SCREEN_SIZE_X / 2.f;
	g_SCREEN_CENTER_Y = g_SCREEN_SIZE_Y / 2.f;

	g_rng = new RandomNumberGenerator();

	m_worldCamBottomLeft = Vec2(-1.f, -1.f);
	m_worldCamTopRight = Vec2(1.f, 1.f);

	m_gameClock = new Clock(Clock::GetSystemClock());
	LoadAllAudioAssets();

	//Event subscriptions
	g_eventSystem->SubscribeEventCallbackFunction("Controls", Game::Event_ShowGameControls);
	Strings timeScaleArguments;
	timeScaleArguments.push_back("Scale=");
	timeScaleArguments.push_back("Scale=1.0");
	g_eventSystem->SubscribeEventCallbackFunction("TimeScale", timeScaleArguments, Game::Event_TimeScale);
	Strings inputControlsArguments;
	inputControlsArguments.push_back("MouseSensitivity=");
	inputControlsArguments.push_back("ControllerSensitivity=");
	inputControlsArguments.push_back("InvertMouseX=");
	inputControlsArguments.push_back("InvertMouseY=");
	inputControlsArguments.push_back("InvertControllerX=");
	inputControlsArguments.push_back("InvertControllerY=");
	inputControlsArguments.push_back("Reset");
	g_eventSystem->SubscribeEventCallbackFunction("ControlsSettings", inputControlsArguments, Game::Event_ControlsSettings);
	PrintControlsToDevConsoleCommand();

	SubscribeEventCallbackFunction("ExplosionFired", Game::Event_Explosion);

	g_renderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);

	//Protogame attract screen delete later
	m_attractTimer = new Timer(2.f, m_gameClock);

	m_player = new Player(this);
	m_entities.push_back(m_player);

	BlockDefinition::InitBlockDefinitions();
	WorldDefinition::InitWorldDefinitionsFromFile(g_gameConfigBlackboard.GetValue("DefaultWorldFile", "Data/Definitions/DefaultWorldDefinition.xml"));
	m_currentWorld = new World(&WorldDefinition::s_worldDefinitions[0]);
}

void Game::Shutdown()
{
	if (m_currentWorld)
	{
		m_currentWorld->ShutDown();
	}
}

//Frame Flow
//--------------------------------------------------------------------
void Game::BeginFrame()
{
	if(m_player)
		m_player->m_camera->SetOrthoView(m_worldCamBottomLeft, m_worldCamTopRight);
	m_screenCamera->SetOrthoView(Vec2(0.f, 0.f), Vec2(g_SCREEN_SIZE_X, g_SCREEN_SIZE_Y));
}

void Game::Update()
{
	double timeStarted = GetCurrentTimeSeconds();
	CheckKeyboardInputs();
	CheckControllerInputs();
	UpdateAttractMode(timeStarted);
	UpdateGameplay(timeStarted);
	UpdateCameras();
	m_updateSpeed = GetCurrentTimeSeconds() - timeStarted;
}

void Game::Render() const
{
	double timeStarted = GetCurrentTimeSeconds();
	Rgba8 clearColor = Rgba8::DARK_GREY;
	if(m_currentWorld)
		clearColor = m_currentWorld->GetSkyColor();

 	g_renderer->ClearScreen(clearColor);
	g_renderer->ClearDepth();

    //Player Camera
	if (m_player)
	{
		g_renderer->BeginCamera(*m_player->m_camera);
		RenderDebugWorld();
		RenderGameplay();
		g_renderer->EndCamera(*m_player->m_camera);
	}

 	//Screen Camera
 	g_renderer->BeginCamera(*m_screenCamera);
	if (m_gameState == GameState::GAMEPLAY && m_player)
	{
		m_player->RenderInventory();
	}
	RenderAttractMode();
	g_devConsole->Render(m_screenCamera);
	RenderPerformanceStats(timeStarted);
	DebugRenderScreen(*m_screenCamera);

	g_renderer->EndCamera(*m_screenCamera);
}

void Game::EndFrame()
{

}

void Game::ReloadWorld(WorldDefinition const* worldDef)
{
	if(!m_currentWorld)
		return;

	m_currentWorld->ShutDown();
	delete m_currentWorld;
	m_currentWorld = new World(worldDef);
}

void Game::SpawnTNT(Vec3 const& position)
{
	LitTnt* tnt = new LitTnt(this);
	tnt->m_position = position;
	Vec3 boundsCenter = position;
	boundsCenter.z += TNT_BOX_RADIUS;
	tnt->m_physicsBounds.SetCenter(boundsCenter);
	Vec3 impulse = Vec3(0.1f, 0.1f, 1.f);
	tnt->AddImpulse(impulse);

	for (int entityNum = 0; entityNum < (int)m_entities.size(); ++entityNum)
	{
		if (m_entities[entityNum] == nullptr)
		{
			m_entities[entityNum] = tnt;
			return;
		}
	}

	m_entities.push_back(tnt);
}

//Update Functions
//--------------------------------------------------------------------
void Game::UpdateAttractMode(double timeUpdateStarted)
{
	if(m_gameState != GameState::ATTRACT_SCREEN)
		return;


	m_circleRadius = Lerp(100.f, 300.f, m_attractTimer->GetElapsedFraction());
	m_circleColor = Rgba8::ColorLerp(Rgba8(0, 255, 0), Rgba8(0, 0, 255), m_attractTimer->GetElapsedFraction());
	if (m_attractTimer->Tick())
	{
		m_circleRadius = 100.f;
	}

	if (m_currentWorld)
	{
		m_currentWorld->AttractScreenUpdate(timeUpdateStarted);
	}
}

void Game::UpdateGameplay(double timeUpdateStarted)
{
	if (m_gameState == GameState::ATTRACT_SCREEN)
		return;

	m_currentWorld->Update(timeUpdateStarted);

	for (int entityNum = 0; entityNum < (int)m_entities.size(); ++entityNum)
	{
		Entity* currentEntity = m_entities[entityNum];
		if (currentEntity)
		{
			currentEntity->Update();
		}
	}

	for (int entityNum = 0; entityNum < (int)m_entities.size(); ++entityNum)
	{
		Entity* currentEntity = m_entities[entityNum];
		if (currentEntity && currentEntity->m_isGarbage)
		{
			delete currentEntity;
			m_entities[entityNum] = nullptr;
		}
	}
}

void Game::UpdateCameras()
{

}

//Render Functions
//--------------------------------------------------------------------

void Game::RenderAttractMode() const
{
	if (m_gameState != GameState::ATTRACT_SCREEN)
		return;

	g_renderer->BeginRendererEvent("Draw - AttractScreen");
	g_renderer->SetBlendMode(BlendMode::OPAQUE);
	g_renderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
	g_renderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_renderer->SetSamplerMode(SamplerMode::BILINEAR_WRAP);
	g_renderer->BindShader(nullptr);
	DebugDrawRing(Vec2(g_SCREEN_CENTER_X, g_SCREEN_CENTER_Y), m_circleRadius, 10.f, m_circleColor);
	g_renderer->EndRendererEvent();

}

void Game::RenderGameplay() const
{
	if (m_gameState == GameState::ATTRACT_SCREEN)
		return;


	for (int entityNum = 0; entityNum < (int)m_entities.size(); ++entityNum)
	{
		Entity* currentEntity = m_entities[entityNum];
		if (currentEntity)
		{
			currentEntity->Render();
		}
	}

	m_currentWorld->Render();
}

void Game::RenderDebugWorld() const
{
	if (m_gameState == GameState::ATTRACT_SCREEN)
		return;

	DebugRenderWorld(*m_player->m_camera);
}

void Game::RenderPerformanceStats(double const& timeStarted) const
{
	if(!m_showPerformance)
		return;
	float deltaSeconds = Clock::GetSystemClock().GetDeltaSeconds();
	float fps = 1.f / deltaSeconds;
	Rgba8 color = fps >= 60.f ? Rgba8::WHITE : Rgba8::ORANGE;
	if (fps < 30.f)
		color = Rgba8::RED;

	AABB2 screenBounds = GetScreenBounds();
	AABB2 bounds = screenBounds;
	bounds.AddPadding(-.9f, -.95f, -.01f, -.0001f);
	std::string text = Stringf("FPS: %.1f", 1.f / deltaSeconds);
	DebugAddScreenText(text, bounds, 0.1f * g_SCREEN_SIZE_Y, Vec2(1.f, 0.5f), 0.f, color);

	color = m_updateSpeed <= 0.0167 ? Rgba8::WHITE : Rgba8::ORANGE;
	if (m_updateSpeed > 0.0334f)
		color = Rgba8::RED;

	bounds = screenBounds;
	bounds.AddPadding(-0.9f, -.9f, -.01f, 0.f);
	text = Stringf("Update: %.1f ms", m_updateSpeed * 1000.0);
	DebugAddScreenText(text, bounds, 0.1f * g_SCREEN_SIZE_Y, Vec2(1.f, 0.5f), 0.f, color);

	double renderSpeed = GetCurrentTimeSeconds() - timeStarted;
	color = renderSpeed <= 0.0167 ? Rgba8::WHITE : Rgba8::RED;
	bounds = screenBounds;
	bounds.AddPadding(-0.9f, -.85f, -.01f, 0.f);
	text = Stringf("Render: %.1f ms", renderSpeed * 1000.0);
	DebugAddScreenText(text, bounds, 0.1f * g_SCREEN_SIZE_Y, Vec2(1.f, 0.5f), 0.f, color);
}

//Input
//--------------------------------------------------------------------
void Game::CheckKeyboardInputs()
{
	//Escape
	if (g_inputSystem->WasKeyJustPressed(KEYCODE_ESC))
	{
		if (m_gameState == GameState::ATTRACT_SCREEN)
		{
			g_app->HandleQuitRequested();
		}

		//Return to attract screen
		else
		{
			ExitGame();
		}
	}

	if (g_inputSystem->WasKeyJustPressed(' '))
	{
		StartGame();
	}

	//Pause
	if (g_inputSystem->WasKeyJustPressed('P'))
	{
		m_gameClock->TogglePause();
	}

	//SloMo
	if (g_inputSystem->WasKeyJustPressed('T'))
	{
		m_isSlowMo = !m_isSlowMo;
		if (m_isSlowMo)
		{
			m_gameClock->SetTimeScale(0.1f);
		}
		else
		{
			m_gameClock->SetTimeScale(1.f);
		}
	}

	//Move one Frame
	if (g_inputSystem->WasKeyJustPressed('O'))
	{
		m_gameClock->StepSingleFrame();
	}

	//Toggle Debug
	if (g_inputSystem->WasKeyJustPressed(KEYCODE_F2)) //F2 key
	{
		g_debugMode = !g_debugMode;
	}

	if (g_inputSystem->WasKeyJustPressed(KEYCODE_F1))
	{
		m_showPerformance = !m_showPerformance;
	}

	//Restart Game
	if (g_inputSystem->WasKeyJustPressed(KEYCODE_F8)) //F8 key
	{
		m_shouldRestart = true;
	}
}

void Game::CheckControllerInputs()
{
	XboxController controller = g_inputSystem->GetController(0);
	if (controller.WasButtonJustPressed(XboxButtonID::BUTTON_START))
	{
		StartGame();
	}

	if (controller.WasButtonJustPressed(XboxButtonID::BUTTON_A))
	{
		g_debugMode = !g_debugMode;
	}
}

//Game Flow
//-----------------------------------------------------------------------------------------------


void Game::StartGame()
{
	if(m_gameState != GameState::ATTRACT_SCREEN)
		return;

	if(m_currentWorld)
		m_currentWorld->AdjustPlayerStartingPosition();

	m_gameState = GameState::GAMEPLAY;
	g_inputSystem->SetCursorMode(CursorMode::FPS);
}

void Game::ExitGame()
{
	m_gameState = GameState::ATTRACT_SCREEN;
	g_inputSystem->SetCursorMode(CursorMode::POINTER);
}

void Game::HandleExplosion(EventArgs& args)
{
	Vec3 center = args.GetValue("Center", Vec3(-100.f, -100.f, -100.f));
	float radius = args.GetValue("Radius", 0.f);
	float force = args.GetValue("Force", 0.f);

	float radiusSqrd = radius * radius;

	for (int i = 0; i < (int)m_entities.size(); ++i)
	{
		Entity* entity = m_entities[i];
		if(!entity)
			continue;

		Vec3 pos = entity->m_position;
		if (GetDistanceSquared3D(center, pos) <= radiusSqrd)
		{
			Vec3 dir = (pos - center).GetNormalized();
			Vec3 impulse = dir * force;
			entity->AddImpulse(impulse);
		}
	}

}

//Helper Functions
//-----------------------------------------------------------------------------------------------



//Audio and SFX
//-----------------------------------------------------------------------------------------------
void Game::LoadAllAudioAssets()
{
	//Music
	m_gameMusicDataPaths.reserve(NUM_GAME_MUSIC);

	g_audioSystem->CreateOrGetSound("Data/Audio/Music/AttractScreenMusic.mp3");
	m_gameMusicDataPaths.push_back("Data/Audio/Music/AttractScreenMusic.mp3");

	//SFX
	m_sfxDataPaths.reserve(NUM_GAME_SFX);

	g_audioSystem->CreateOrGetSound("Data/Audio/SFX/ButtonSelect.mp3");
	m_sfxDataPaths.push_back("Data/Audio/SFX/ButtonSelect.mp3");
}


void const Game::PlayGameSFX(GameSFX const& sfx)
{
	if ((int)m_sfxDataPaths.size() <= sfx)
		return;

	SoundID newSound = g_audioSystem->CreateOrGetSound(m_sfxDataPaths[sfx]);
	g_audioSystem->StartSound(newSound);
}

void const Game::PlayGameSFX(GameSFX const& sfx, Vec2 const& worldPos)
{
	if ((int)m_sfxDataPaths.size() <= sfx)
		return;

 	float balance = GetAudioBalanceFromWorldPosition(worldPos);
	SoundID newSound = g_audioSystem->CreateOrGetSound(m_sfxDataPaths[sfx]);
	g_audioSystem->StartSound(newSound, false, 1.f, balance);
}

void const Game::PlayLoopingGameSFX(GameSFX const& sfx)
{
	if ((int)m_sfxDataPaths.size() <= sfx)
		return;

	SoundID newSound = g_audioSystem->CreateOrGetSound(m_sfxDataPaths[sfx]);
	auto foundLoopingSFX = m_loopingSFXSoundPlaybackPairs.find(sfx);
	if (foundLoopingSFX == m_loopingSFXSoundPlaybackPairs.end())
	{
		m_loopingSFXSoundPlaybackPairs.insert({ sfx, g_audioSystem->StartSound(newSound, true) });
	}
}

void const Game::StopLoopingGameSFX(GameSFX const& sfx)
{
	auto foundLoopingSFX = m_loopingSFXSoundPlaybackPairs.find(sfx);
	if (foundLoopingSFX != m_loopingSFXSoundPlaybackPairs.end())
	{
		g_audioSystem->StopSound(foundLoopingSFX->second);
		m_loopingSFXSoundPlaybackPairs.erase(sfx);
	}
}

void const Game::PlayGameMusic(GameMusic const& music)
{
	if (m_playingMusic != MISSING_SOUND_ID)
	{
		g_audioSystem->StopSound(m_playingMusic);
	}
	SoundID newMusic = g_audioSystem->CreateOrGetSound(m_gameMusicDataPaths[music]);
	m_playingMusic = g_audioSystem->StartSound(newMusic, true, m_musicVolume);
}

void const Game::StopGameMusic()
{
	if (m_playingMusic != MISSING_SOUND_ID)
	{
		g_audioSystem->StopSound(m_playingMusic);
	}

	m_playingMusic = MISSING_SOUND_ID;
}

float Game::GetAudioBalanceFromWorldPosition(Vec2 const& inPosition) const
{
	return RangeMapClamped(inPosition.x, m_worldCamBottomLeft.x, m_worldCamTopRight.x, -1.f, 1.f);;
}

void const Game::SetAudioBalanceAndVolumeFromWorldPosition(SoundPlaybackID& sound, Vec2 const& worldPos)
{
	AABB2 cameraView = AABB2(m_worldCamBottomLeft, m_worldCamTopRight);
	if (!cameraView.IsPointOnOrInside(worldPos)) // volume = 0 if position is not in camera view
	{
		g_audioSystem->SetSoundPlaybackVolume(sound, 0.f);
	}

	g_audioSystem->SetSoundPlaybackBalance(sound, RangeMapClamped(worldPos.x, m_worldCamBottomLeft.x, m_worldCamTopRight.x, -1.f, 1.f));
}

void Game::SwitchGameState(GameState gameState)
{
	m_gameState = gameState;
}

bool Game::IsAttractScreen() const
{
	return m_gameState == GameState::ATTRACT_SCREEN;
}

AABB2 Game::GetScreenBounds() const
{
	return AABB2(m_screenCamera->GetOrthoBottomLeft(), m_screenCamera->GetOrthoTopRight());
}

//Commands
//-----------------------------------------------------------------------------------------------
void Game::PrintControlsToDevConsoleCommand()
{
	g_devConsole->AddLine(DevConsole::INFO_MAJOR, "--Protogame3D Controls--", 1.f, true);
	g_devConsole->AddLine(DevConsole::INFO_MINOR, "SPACE    Start", 0.75f, true);
	g_devConsole->AddLine(DevConsole::INFO_MINOR, "ESC      Exit/Quit", 0.75f, true);
	g_devConsole->AddLine(DevConsole::INFO_MINOR, "P        Pause", 0.75f, true);
	g_devConsole->AddLine(DevConsole::INFO_MINOR, "O        Step One Frame", 0.75f, true);
	g_devConsole->AddLine(DevConsole::INFO_MINOR, "T        Slow Mode", 0.75f, true);
	g_devConsole->AddLine(DevConsole::INFO_MINOR, "MOUSE    Look", 0.75f, true);
	g_devConsole->AddLine(DevConsole::INFO_MINOR, "WASD     Move", 0.75f, true);
	g_devConsole->AddLine(DevConsole::INFO_MINOR, "Q/E      Move Up/Down", 0.75f, true);
	g_devConsole->AddLine(DevConsole::INFO_MINOR, "F8		Reset World", 0.75f, true);
	g_devConsole->AddLine(DevConsole::INFO_MINOR, "F2		Chunk Bounds", 0.75f, true);
	g_devConsole->AddLine(DevConsole::INFO_MINOR, "F3		Debug Job System", 0.75f, true);
	g_devConsole->AddLine(DevConsole::INFO_MINOR, "F4		Toggle Chunk Load", 0.75f, true);
	g_devConsole->AddLine(DevConsole::INFO_MINOR, "[c]		Change Camera Mode", 0.75f, true);
	g_devConsole->AddLine(DevConsole::INFO_MINOR, "[v]		Change Physics Mode", 0.75f, true);
	g_devConsole->AddLine(DevConsole::INFO_MINOR, "[r]	    Lock Player Raycast", 0.75f, true);
	g_devConsole->AddLine(DevConsole::INFO_MINOR, "[LMB]	Remove Block", 0.75f, true);
	g_devConsole->AddLine(DevConsole::INFO_MINOR, "[RMB]	Add Block", 0.75f, true);
	
}


void Game::AdjustControlsSettingsCommand(EventArgs& args)
{
	if (args.HasKey("Reset"))
	{
		g_mouseSensitivity = 0.125f;
		g_invertMouseX = false;
		g_invertMouseY = false;
		g_controllerSensitivity = 50.f;
		g_invertControllerViewX = false;
		g_invertControllerViewY = false;
	}

	if (args.HasKey("MouseSensitivity"))
	{
		g_mouseSensitivity = RangeMapClamped(args.GetValue("MouseSensitivity", 0.5f), 0.f, 1.f, 0.05f, 0.2f);
	}

	if (args.HasKey("InvertMouseX"))
	{
		g_invertMouseX = args.GetValue("InvertMouseX", false);
	}

	if (args.HasKey("InvertMouseY"))
	{
		g_invertMouseX = args.GetValue("InvertMouseY", false);
	}

	if (args.HasKey("ControllerSensitivity"))
	{
		g_controllerSensitivity = RangeMapClamped(args.GetValue("ControllerSensitivity", 0.5f), 0.f, 1.f, 20.f, 200.f);
	}

	if (args.HasKey("InvertControllerX"))
	{
		g_invertControllerViewX = args.GetValue("InvertControllerX", false);
	}

	if (args.HasKey("InvertControllerY"))
	{
		g_invertControllerViewX = args.GetValue("InvertControllerY", false);
	}
}

void Game::AdjustTimeScaleCommand(float scale)
{
	m_gameClock->SetTimeScale(scale);
}

//Events
//-----------------------------------------------------------------------------------------------
bool Game::Event_ShowGameControls(EventArgs& args)
{
	UNUSED(args);
	if (g_game != nullptr)
	{
		g_game->PrintControlsToDevConsoleCommand();
		return true;
	}

	return false;
}

bool Game::Event_TimeScale(EventArgs& args)
{
	float scale = args.GetValue("Scale", 1.f);
	if (g_game != nullptr)
	{
		g_game->AdjustTimeScaleCommand(scale);
		return true;
	}
	return false;
}

bool Game::Event_ControlsSettings(EventArgs& args)
{
	if (g_game)
	{
		g_game->AdjustControlsSettingsCommand(args);
		return true;
	}
	return false;
}

bool Game::Event_Explosion(EventArgs& args)
{
	if (g_game)
	{
		g_game->HandleExplosion(args);
		return true;
	}
	return false;
}
