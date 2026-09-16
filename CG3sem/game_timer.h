#ifndef GAME_TIMER
#define GAME_TIMER
#include <Windows.h>

class GameTimer {
private:
	double SecsPerCount;
	double Delta;

	__int64 TimeBase;
	__int64 TimePause;
	__int64 TimeStop;
	__int64 TimeCurr;
	__int64 TimePrev;

	bool m_stopped_;
public:
	GameTimer();

	float TotalTime() const;
	float DeltaTime() const;

	void Start();
	void Stop();
	void Reset();
	void Tick();
};

#endif