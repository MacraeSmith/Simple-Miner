#pragma once
#include "Engine/Core/EventSystem.hpp"
class Game;

class App
{
public:
	App() {};
	~App() {};

	
	//App Flow
	void Startup();
	void Shutdown();
	void RunFrame();

	static bool QuitEvent(EventArgs& args);

	//Mutators
	void HandleQuitRequested();
	void RestartGame();

	//Accessors
	bool IsQuitting() const { return m_isQuitting; }

private:
	//Frame flow
	void BeginFrame();
	void Update();
	void Render() const;
	void EndFrame();
	void LoadGameConfig(char const* gameConfigFilePath);

	
private:
	bool m_isQuitting = false;
	float m_timeLastFrameStart = 0.f;

};



