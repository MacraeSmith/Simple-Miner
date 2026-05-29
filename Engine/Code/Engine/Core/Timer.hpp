#pragma once

class Clock;
class Timer
{
public:
	Timer();
	explicit Timer(double period, Clock const* clock = nullptr, bool autoStart = true);
	~Timer() {}

	void Start();
	void Stop();
	void Restart();
	void SetPeriod(float period);

	double GetElapsedTime() const;
	float GetElapsedFraction() const;

	bool IsStopped() const;
	bool HasPeriodElapsed() const;

	bool DecrementPeriodIfElapsed();
	bool Tick(); //same as DecrementPeriodIfElapsed() I liked the name for functional use better;

public:
	Clock const* m_clock = nullptr;
	double m_startTime = -1.0;
	double m_period = 0.0;
};

