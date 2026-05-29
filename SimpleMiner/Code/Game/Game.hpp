#pragma once
#include "Game/GameCommon.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include <map>
#include <string>


class RandomNumberGenerator;
class Camera;
struct Vec2;
class Timer;
class Clock;
class Player;
class Entity;
class World;
struct AABB2;
class WorldDefinition;

enum GameMusic : int
{
	ATTRACT_SCREEN,
	NUM_GAME_MUSIC
};

enum GameSFX : int
{
	BUTTON_SELECT,
	NUM_GAME_SFX
};

enum class GameState : int
{
	ATTRACT_SCREEN,
	GAMEPLAY,
};

class Game
{
public:
	 Game();
	~Game();

	//Game Flow Management
	void Startup();
	void Shutdown();
	void BeginFrame();
	void Update();
	void Render() const;
	void EndFrame();
	void ReloadWorld(WorldDefinition const* worldDef);
	void SpawnTNT(Vec3 const& position);

	//Events
	static bool Event_ShowGameControls(EventArgs& args);
	static bool Event_TimeScale(EventArgs& args);
	static bool Event_ControlsSettings(EventArgs& args);
	static bool Event_Explosion(EventArgs& args);


	//Helpers
	//---------------------------------------------------------- 
	

	//Music and SFX
	void const PlayGameSFX(GameSFX const& sfx);
	void const PlayGameSFX(GameSFX const& sfx, Vec2 const& worldPos);
	void const PlayLoopingGameSFX(GameSFX const& sfx);
	void const StopLoopingGameSFX(GameSFX const& sfx);
	void const PlayGameMusic(GameMusic const& music);
	void const StopGameMusic();
	float GetAudioBalanceFromWorldPosition(Vec2 const& inPosition) const;
	void const SetAudioBalanceAndVolumeFromWorldPosition(SoundPlaybackID& sound, Vec2 const& worldPos); // mutes sound if it is off screen

	void SwitchGameState(GameState gameState);
	bool IsAttractScreen() const;
	AABB2 GetScreenBounds() const;

private:
	//Startup
	void LoadAllAudioAssets();

	//Update Methods
	void UpdateAttractMode(double timeUpdateStarted);
	void UpdateGameplay(double timeUpdateStarted);
	void UpdateCameras();

	//Render Methods
	void RenderAttractMode() const;
	void RenderGameplay() const;
	void RenderDebugWorld() const;
	void RenderPerformanceStats(double const& timeStarted) const;

	//Input
	void CheckKeyboardInputs();
	void CheckControllerInputs();

	//Commands
	void AdjustTimeScaleCommand(float scale);
	void PrintControlsToDevConsoleCommand();
	void AdjustControlsSettingsCommand(EventArgs& args);

	void StartGame();
	void ExitGame();

	void HandleExplosion(EventArgs& args);

	//Helpers
	//---------------------------------------------------------- 

public:

	Player* m_player = nullptr;
	World* m_currentWorld = nullptr;
	Clock* m_gameClock = nullptr;


private:
	GameState m_gameState = GameState::ATTRACT_SCREEN;

	//Attract mode Delete Later
	float m_circleRadius = 200.f;
	Rgba8 m_circleColor;
	Timer* m_attractTimer = nullptr;

	//Camera
	Camera* m_screenCamera;
	Vec2 m_worldCamBottomLeft;
	Vec2 m_worldCamTopRight;

	//Restart
	bool m_shouldRestart = false;
	bool m_showPerformance = false;

	//Game State


	//Audio
	GameMusic m_gameMusic;
	std::vector<std::string> m_gameMusicDataPaths;
	std::vector<std::string> m_sfxDataPaths;
	SoundPlaybackID* m_musicSoundPlaybacks [NUM_GAME_MUSIC];
	std::map<GameSFX, SoundPlaybackID> m_loopingSFXSoundPlaybackPairs;
	float m_musicVolume = 0.f;
	SoundPlaybackID m_playingMusic = MISSING_SOUND_ID;

	//Time
	bool m_isSlowMo = false;
	double m_updateSpeed = 0.0;
	double m_renderSpeed = 0.0;

	std::vector<Entity*> m_entities;

};

