#ifndef WIN_CLASS
#define WIN_CLASS
#include <Windows.h>

#include "game_timer.h"
#include "app.h"

class WinClass 
{
private:
	HINSTANCE hInstance;
	HINSTANCE hPrevInstance;
	WNDCLASSEX wc;
	HWND hWnd = nullptr;
	RAWINPUTDEVICE rid[2];
public:
	WinClass(HINSTANCE hInstance, HINSTANCE hPrevInstance) : hInstance(hInstance), hPrevInstance(hPrevInstance) {}
	bool initWin(WNDPROC WndProc);
	void CreateWin();
	void ShowWin();
	void RegisterRawInputDevice();
	int WRun(GameTimer* timer);
	bool CheckCreation();
	HWND getHWND() const { return hWnd; }
};

#endif
