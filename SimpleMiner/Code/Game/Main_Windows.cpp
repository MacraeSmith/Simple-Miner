#define WIN32_LEAN_AND_MEAN		// Always #define this before #including <windows.h>
#include <windows.h>			// #include this (massive, platform-specific) header in VERY few places (and .CPPs only)
#include <math.h>
#include <cassert>
#include <crtdbg.h>

#include "Engine/Input/InputSystem.hpp"
#include "Engine/Core/EngineCommon.hpp"

#include "Game/App.hpp"
#include "Game/Game.hpp"

extern App* g_app;

//-----------------------------------------------------------------------------------------------
int WINAPI WinMain( HINSTANCE applicationInstanceHandle, HINSTANCE, LPSTR commandLineString, int )
{
	UNUSED( commandLineString );
	UNUSED(applicationInstanceHandle);

	g_app = new App();
	g_app->Startup();

	// Program main loop; keep running frames until it's time to quit
	while(!g_app->IsQuitting())		
	{
		g_app->RunFrame();
	}
	g_app->Shutdown();
	
	delete g_app;
	g_app = nullptr;

	return 0;
}


