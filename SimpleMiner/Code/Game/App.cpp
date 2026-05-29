#include "Game/App.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Renderer/RendererDX11.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Window/Window.hpp"
#include "Engine/Renderer/DebugRender.hpp"
#include "Engine/Core/JobSystem.hpp"

#include "Game/Game.hpp"

App* g_app = nullptr;
RendererDX11* g_renderer = nullptr;
AudioSystem* g_audioSystem = nullptr;
Window* g_window = nullptr; 
Game* g_game = nullptr;
JobSystem* g_jobSystem = nullptr;


//App Management
//-----------------------------------------------------------------------------------------------
void App::Startup()
{
	LoadGameConfig("Data/Definitions/GameConfig.xml");
	InputConfig inputConfig;
	g_inputSystem = new InputSystem(inputConfig);

	WindowConfig windowConfig;
	windowConfig.m_aspectRatio = 2.0f;
	windowConfig.m_fullScreen = g_gameConfigBlackboard.GetValue("windowFullscreen", false);
	windowConfig.m_inputSystem = g_inputSystem;
	windowConfig.m_windowTitle = "SimpleMiner";
	g_window = new Window(windowConfig);

	RendererConfig rendererConfig;
	rendererConfig.m_window = g_window;
	rendererConfig.m_useImGUI = false;
	g_renderer = new RendererDX11(rendererConfig);

	DebugRenderConfig debugRenderConfig;
	debugRenderConfig.m_renderer = g_renderer;
	debugRenderConfig.m_fontName = "Data/Font/SquirrelFixedFont";

	EventSystemConfig eventSystemConfig;
	g_eventSystem = new EventSystem(eventSystemConfig);

	DevConsoleConfig devConsoleConfig;
	devConsoleConfig.m_defaultFontName = "Data/Font/SquirrelFixedFont";
	devConsoleConfig.m_defaultLinesOnScreen = 29.5f;
	devConsoleConfig.m_defaultRenderer = g_renderer;
	devConsoleConfig.m_defaultFontAspect = 1.f;
	devConsoleConfig.m_activateChatSystem = false;
	g_devConsole = new DevConsole(devConsoleConfig);

	AudioConfig audioConfig;
	g_audioSystem = new AudioSystem(audioConfig);

	JobSystemConfig jobConfig;
	jobConfig.m_numWorkerThreads = MAX_NUM_WORKER_THREADS;
	g_jobSystem = new JobSystem(jobConfig);

	g_window->Startup();
	g_renderer->Startup();
	DebugRenderSystemStartup(debugRenderConfig);
	g_eventSystem->Startup();
	g_devConsole->Startup();
	g_inputSystem->Startup();
	g_audioSystem->Startup();
	g_jobSystem->Startup();

	g_game = new Game();
	g_game->Startup();

	SubscribeEventCallbackFunction("Quit", QuitEvent);
	Clock::GetSystemClock().SetMaxFrameRate(120.f);
}

void App::Shutdown()
{
	g_game->Shutdown();
	delete g_game;
	g_game = nullptr;

	g_jobSystem->Shutdown();
	g_audioSystem->Shutdown();
	g_devConsole->ShutDown();
	g_eventSystem->ShutDown();
	DebugRenderSystemShutdown();
	g_renderer->Shutdown();
	g_window->Shutdown();
	g_inputSystem->Shutdown();

	delete g_jobSystem;
	g_jobSystem = nullptr;

	delete g_audioSystem;
	g_audioSystem = nullptr;

	delete g_devConsole;
	g_devConsole = nullptr;

	delete g_eventSystem;
	g_eventSystem = nullptr;

	delete g_renderer;
	g_renderer = nullptr;

	delete g_window;
	g_window = nullptr;

	delete g_inputSystem;
	g_inputSystem = nullptr;
}

//Frame Flow
//-----------------------------------------------------------------------------------------------
void App::RunFrame()
{
	BeginFrame();
	Update();
	Render();
	EndFrame();
}


void App::BeginFrame()
{
	Clock::TickSystemClock();
	g_window->BeginFrame();
	g_inputSystem->BeginFrame();
	g_renderer->BeginFrame();
	DebugRenderBeginFrame();
	g_eventSystem->BeginFrame();
	g_devConsole->BeginFrame();
	g_audioSystem->BeginFrame();
	g_game->BeginFrame();
}

void App::Update()
{
	g_inputSystem->m_cursorState.m_cursorMode;
	if (g_game->IsAttractScreen() || g_devConsole->GetMode() != DevConsoleMode::HIDDEN || !Window::s_mainWindow->IsWindowActive())
	{
		if (g_inputSystem->GetCursorMode() != CursorMode::POINTER)
		{
			g_inputSystem->SetCursorMode(CursorMode::POINTER);
		}
	}

	else if(g_inputSystem->GetCursorMode() != CursorMode::FPS)
	{
		g_inputSystem->SetCursorMode(CursorMode::FPS);
	}


	g_game->Update();
}

void App::Render() const
{
	g_game->Render();
}

void App::EndFrame()
{
	g_game->EndFrame();
	g_audioSystem->EndFrame();
	g_eventSystem->EndFrame();
	g_devConsole->EndFrame();
	g_inputSystem->EndFrame();
	DebugRenderEndFrame();
	g_renderer->EndFrame();
	g_window->EndFrame();
}

void App::LoadGameConfig(char const* gameConfigFilePath)
{
	XmlDocument gameConfigXml;
	XmlResult result = gameConfigXml.LoadFile(gameConfigFilePath);
	if (result == tinyxml2::XML_SUCCESS)
	{
		XmlElement* rootElement = gameConfigXml.RootElement();
		if (rootElement != nullptr)
		{
			g_gameConfigBlackboard.PopulateFromXmlElementAttributes(*rootElement);
		}

		else
		{
			DebuggerPrintf("WARNING: game config from file \"%s\" was invalid (missing root element)\n", gameConfigFilePath);
		}
	}

	else
	{
		DebuggerPrintf("WARNING: failed to load game config from file \"%s\"\n", gameConfigFilePath);
	}
}


//Input
//-----------------------------------------------------------------------------------------------
void App::HandleQuitRequested()
{
	m_isQuitting = true;
}

bool App::QuitEvent(EventArgs& args)
{
	UNUSED(args);
	if (g_app != nullptr)
	{
		g_app->HandleQuitRequested();
	}

	return true;
}
//Game Management
//-----------------------------------------------------------------------------------------------
void App::RestartGame()
{
	g_game->Shutdown();
	delete g_game;
	g_game = nullptr;

	g_game = new Game();
	g_game->Startup();
}
