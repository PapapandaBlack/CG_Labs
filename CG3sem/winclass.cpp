#include <Windows.h>
#include "winclass.h"

bool WinClass::initWin(WNDPROC WndProc) {
	wc = {};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.hInstance = hInstance;
	wc.lpszClassName = L"WindowClass";
	wc.lpfnWndProc = WndProc;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hIcon = LoadIcon(nullptr, IDI_WINLOGO);
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName = nullptr;
	wc.hIconSm = wc.hIcon;

	if (!RegisterClassEx(&wc)) {
		return false;
	}
	return true;
}

void WinClass::CreateWin() {
	hWnd = CreateWindowExW(0, L"WindowClass", L"WindowName", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, nullptr);
}

void WinClass::ShowWin() {
	ShowWindow(hWnd, SW_SHOW);
}

bool WinClass::CheckCreation() {
	return (hWnd != NULL);
}

void WinClass::RegisterRawInputDevice() {

	rid[0].usUsagePage = 0x01;
	rid[0].usUsage = 0x02;
	rid[0].dwFlags = RIDEV_INPUTSINK;
	rid[0].hwndTarget = hWnd;

	rid[1].usUsagePage = 0x01;
	rid[1].usUsage = 0x06;
	rid[1].dwFlags = 0;
	rid[1].hwndTarget = hWnd;

	if (RegisterRawInputDevices(rid, 2, sizeof(RAWINPUTDEVICE)) == FALSE) {
		MessageBoxW(hWnd, L"Failed to register raw input devices", L"Error", MB_OK);
	}
}
