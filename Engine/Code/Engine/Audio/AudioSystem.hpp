#pragma once

#include "Engine/Math/Mat44.hpp"
//-----------------------------------------------------------------------------------------------
#include "ThirdParty/fmod/fmod.hpp"
#include <string>
#include <vector>
#include <map>

//-----------------------------------------------------------------------------------------------
typedef size_t SoundID;
typedef size_t SoundPlaybackID;
constexpr size_t MISSING_SOUND_ID = (size_t)(-1); // for bad SoundIDs and SoundPlaybackIDs

struct Vec3;
//-----------------------------------------------------------------------------------------------
class AudioSystem;

struct AudioConfig
{

};
/////////////////////////////////////////////////////////////////////////////////////////////////
class AudioSystem
{
public:
	AudioSystem(AudioConfig const& config);
	virtual ~AudioSystem();

public:
	void						Startup();
	void						Shutdown();
	virtual void				BeginFrame();
	virtual void				EndFrame();

	virtual SoundID				CreateOrGetSound( const std::string& soundFilePath, bool threeDimensional = false);
	virtual SoundPlaybackID		StartSound( SoundID soundID, bool isLooped=false, float volume=1.f, float balance=0.0f, float speed=1.0f, bool isPaused=false );
	virtual SoundPlaybackID		StartSoundAt(SoundID soundID, Vec3 const& soundPos, bool isLooped = false, float volume = 1.f, float balance = 0.0f, float speed = 1.0f, bool isPaused = false);
	virtual void				StopSound( SoundPlaybackID soundPlaybackID );
	virtual void				SetSoundPlaybackVolume( SoundPlaybackID soundPlaybackID, float volume );	// volume is in [0,1]
	virtual void				SetSoundPlaybackBalance( SoundPlaybackID soundPlaybackID, float balance );	// balance is in [-1,1], where 0 is L/R centered
	virtual void				SetSoundPlaybackSpeed( SoundPlaybackID soundPlaybackID, float speed );		// speed is frequency multiplier (1.0 == normal)

	virtual void				ValidateResult( FMOD_RESULT result );

	void						SetNumListeners(int numListeners);
	void						UpdateListeners(int listenerIndex, Vec3 const& listenerPos, Vec3 const& listenerFwrd, Vec3 const& listenerUp);
	void						SetSoundPosition(SoundPlaybackID soundPlaybackID, Vec3 const& position);
	bool						IsPlaying(SoundPlaybackID soundPlaybackID);

protected:
	FMOD::System*						m_fmodSystem;
	std::map< std::string, SoundID >	m_registeredSoundIDs;
	std::vector< FMOD::Sound* >			m_registeredSounds;
	std::vector<SoundPlaybackID> m_3DSoundPlaybacks;

private:
	AudioConfig m_config;
};


