#include "Engine/Core/Clock.hpp"
#include "Engine/Core/Time.hpp"
#include <thread>

static Clock* s_systemClock = new Clock();

Clock::Clock()
{
	if (s_systemClock != nullptr)
	{
		m_parent = s_systemClock;
		m_parent->AddChild(this);
	}
}

Clock::Clock(Clock& parent)
	:m_parent(&parent)
{
	m_parent->AddChild(this);
}

void Clock::Reset()
{
	m_lastUpdateTimeInSeconds = 0.0;
	m_totalSeconds = 0.0;
	m_deltaSeconds = 0.0;
	m_frameCount = 0;

	m_lastUpdateTimeInSeconds = GetCurrentTimeSeconds();
}

bool Clock::IsPaused() const
{
	return m_isPaused;
}

void Clock::Pause()
{
	m_isPaused = true;
}

void Clock::Unpause()
{
	m_isPaused = false;
}

void Clock::TogglePause()
{
	m_isPaused = !m_isPaused;
}

void Clock::StepSingleFrame()
{
	m_stepSingleFrame = true;
	m_isPaused = false;
}

Clock::~Clock()
{
	if (m_parent)
	{
		m_parent->RemoveChild(this);
	}

	for (int clockNum = 0; clockNum < (int)m_children.size(); ++clockNum)
	{
		m_children[clockNum]->m_parent = nullptr;
		m_children[clockNum] = nullptr;
	}

	m_children.clear();
}

void Clock::SetTimeScale(float timeScale)
{
	m_timeScale = (double)timeScale;
}

float Clock::GetTimeScale() const
{
	return (float)m_timeScale;
}

void Clock::SetMinDeltaSeconds(float deltaSeconds)
{
	m_minDeltaSeconds = (double)deltaSeconds;
}

void Clock::SetMaxFrameRate(float frameRate)
{
	m_minDeltaSeconds = (double)(1.f/frameRate);
}

float Clock::GetDeltaSeconds() const
{
	return (float)m_deltaSeconds;
}

float Clock::GetTotalSeconds() const
{
	return (float)m_totalSeconds;
}

int Clock::GetFrameCount() const
{
	return m_frameCount;
}

Clock& Clock::GetSystemClock()
{
	return *s_systemClock;
}

void Clock::TickSystemClock()
{
	s_systemClock->Tick();
}

void Clock::Tick()
{
	double deltaSeconds = (GetCurrentTimeSeconds() - m_lastUpdateTimeInSeconds);
	double curTime = GetCurrentTimeSeconds();
	while (deltaSeconds < m_minDeltaSeconds)
	{
		std::this_thread::yield();
		curTime = GetCurrentTimeSeconds();
		deltaSeconds = curTime - m_lastUpdateTimeInSeconds;
	}

	if (m_isPaused && !m_stepSingleFrame)
	{
		deltaSeconds = 0;
	}
		
	Advance(deltaSeconds);
}

void Clock::Advance(double deltaTimeSeconds)
{
	double deltaSeconds = deltaTimeSeconds * m_timeScale;

	if (m_isPaused)
	{
		m_deltaSeconds = 0.0;
	}

	if (!m_isPaused)
	{
		m_deltaSeconds = deltaSeconds;
		m_lastUpdateTimeInSeconds += deltaSeconds;
		m_totalSeconds += deltaSeconds;

		for (int clockNum = 0; clockNum < (int)m_children.size(); ++clockNum)
		{
			if (m_children[clockNum] != nullptr)
			{
				m_children[clockNum]->Advance(deltaSeconds);
			}
		}
	}


	if (m_stepSingleFrame)
	{
		m_isPaused = true;
		m_stepSingleFrame = false;
	}

}

void Clock::AddChild(Clock* childClock)
{
	for (int i = 0; i < m_children.size(); ++i)
	{
		if (!m_children[i])
		{
			m_children[i] = childClock;
			return;
		}
	}

	m_children.push_back(childClock);
}

void Clock::RemoveChild(Clock* childClock)
{
	for (int clockNum = 0; clockNum < (int)m_children.size(); ++clockNum)
	{
		if (childClock == m_children[clockNum])
		{
			m_children[clockNum] = nullptr;
			return;
		}
	}
}
