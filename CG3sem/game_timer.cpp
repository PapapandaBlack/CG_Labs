#include "game_timer.h"

GameTimer::GameTimer() : Delta(-1.0f), SecsPerCount(0.0f), TimeBase(0.0f), TimePause(0.0f), TimeStop(0.0f), TimePrev(0.0f), TimeCurr(0.0f), m_stopped_(false)
{
	__int64 countsPerSec;
	QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec);
	SecsPerCount = 1.0f / (double)countsPerSec;
}

void GameTimer::Tick()
{
	if (m_stopped_) {
		Delta = 0.0;
		return;
	}

	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
	TimeCurr = currTime;
	Delta = (TimeCurr - TimePrev) * SecsPerCount;
	TimePrev = TimeCurr;

	if (Delta < 0.0) {
		Delta = 0.0;
	}

	if (Delta > 0.2) {
		Delta = 0.2;
	}
}

float GameTimer::DeltaTime() const
{
	return (float)Delta;
}

void GameTimer::Stop()
{
	if (!m_stopped_) {
		__int64 currTime;
		QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
		TimeStop = currTime;
		m_stopped_ = true;
	}
}

void GameTimer::Start()
{
	__int64 startTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&startTime);

	if (m_stopped_) {
		TimePause += (startTime - TimeStop);
		TimePrev = startTime;
		TimeStop = 0;
		m_stopped_ = false;
	}
}

float GameTimer::TotalTime() const
{

	if (m_stopped_) {
		return (float)(((TimeStop - TimePause) - TimeBase) * SecsPerCount);
	}

	else {
		return (float)(((TimeCurr - TimePause) - TimeBase) * SecsPerCount);
	}
}

void GameTimer::Reset()
{
	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
	TimeBase = currTime;
	TimePrev = currTime;
	TimeStop = 0;
	m_stopped_ = false;
}


