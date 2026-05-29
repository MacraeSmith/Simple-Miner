#pragma once
#include <vector>
class Clock
{
public:
	Clock();
	explicit Clock(Clock& parent);
	~Clock();
	Clock(Clock const& copy) = delete;

	void Reset();

	bool IsPaused() const;
	void Pause();
	void Unpause();
	void TogglePause();

	void StepSingleFrame();

	void SetTimeScale(float timeScale);
	float GetTimeScale() const;

	void SetMinDeltaSeconds(float deltaSeconds);
	void SetMaxFrameRate(float frameRate);

	float GetDeltaSeconds() const;
	float GetTotalSeconds() const;
	int GetFrameCount() const;

	static Clock& GetSystemClock();

	static void TickSystemClock();

protected:
	void Tick();

	void Advance(double deltaTimeSeconds);

	void AddChild(Clock* childClock);
	void RemoveChild(Clock* childClock);

protected:
	Clock* m_parent = nullptr;
	std::vector<Clock*> m_children;

	double m_lastUpdateTimeInSeconds = 0.0;
	double m_totalSeconds = 0.0;
	double m_deltaSeconds = 0.0;
	int m_frameCount = 0;

	double m_timeScale = 1.0;

	bool m_isPaused = false;

	bool m_stepSingleFrame = false;

	double m_maxDeltaSeconds = 0.1;
	double m_minDeltaSeconds = 0;
};

