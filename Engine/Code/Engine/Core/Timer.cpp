#include "Engine/Core/Timer.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Math/MathUtils.hpp"

Timer::Timer()
	:m_period(0.0)
	,m_clock(&Clock::GetSystemClock())
{
}

Timer::Timer(double period, Clock const* clock, bool autoStart)
	:m_period(period)
	,m_clock(clock)
{
	if (m_clock == nullptr)
	{
		m_clock = &Clock::GetSystemClock();
	}
	if (autoStart)
	{
		Start();
	}
}

void Timer::Start()
{
	m_startTime = m_clock->GetTotalSeconds();
}

void Timer::Stop()
{
	m_startTime = -1.0;
}

void Timer::Restart()
{
	Start();
}

void Timer::SetPeriod(float period)
{
	m_period = (double)period;
}

double Timer::GetElapsedTime() const
{
	if(IsStopped())
		return 0.0;

	return m_clock->GetTotalSeconds() - m_startTime;
}

float Timer::GetElapsedFraction() const
{
	float elapsedTime = (float)GetElapsedTime();
	return  GetFractionWithinRange(elapsedTime, 0.f, (float)m_period);
}

bool Timer::IsStopped() const
{
	return m_startTime <= -1.0;
}

bool Timer::HasPeriodElapsed() const
{
	if (IsStopped())
		return false;

	return GetElapsedTime() >= m_period;
}

bool Timer::DecrementPeriodIfElapsed()
{
	if (HasPeriodElapsed())
	{
		m_startTime += m_period;
		return true;
	}

	return false;
}

bool Timer::Tick()
{
	int decrementCount = 0;
	while (DecrementPeriodIfElapsed())
	{
		decrementCount++;
	}
	return decrementCount > 0;
}
