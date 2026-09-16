#include <Windows.h>
#include <d3d12.h>
#include <dxgi.h>
#include <DirectXHelpers.h>

#include "winclass.h"
#include "app.h"
#include "game_timer.h"
#include "vertex.h"


#pragma comment(linker, "/entry:wWinMainCRTStartup")

HWND g_hWnd = 0;
App myApp;

using namespace DirectX;
using namespace DX12;
using namespace Microsoft::WRL;

LRESULT CALLBACK WinProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_CREATE:
		{
			CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
			SetWindowLongPtr(hwnd, GWLP_USERDATA,reinterpret_cast<LONG_PTR>(pCreate->lpCreateParams));
			return 0;
		}
		case WM_KEYDOWN:
		{
			if (wParam == VK_ESCAPE)
			{
				DestroyWindow(hwnd);
			}
			return 0;
		}
		case WM_INPUT:
		{
			UINT dwSize = 0;
			GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize,sizeof(RAWINPUTHEADER));
			BYTE* lpb = new BYTE[dwSize];
			if (lpb == NULL) return 0;
			if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize,	sizeof(RAWINPUTHEADER)) != dwSize) 
			{
				delete[] lpb;
				return 0;
			}

			RAWINPUT* raw = (RAWINPUT*)lpb;
			if (raw->header.dwType == RIM_TYPEMOUSE)
			{
				short dx = raw->data.mouse.lLastX;
				short dy = raw->data.mouse.lLastY;
				USHORT buttons = raw->data.mouse.usButtonFlags;
				if (buttons & RI_MOUSE_LEFT_BUTTON_DOWN)		myApp.OnMouseDown(hwnd);
				if (buttons & RI_MOUSE_LEFT_BUTTON_UP)			myApp.OnMouseUp();
				bool leftDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
				myApp.OnMouseMove(leftDown ? MK_LBUTTON : 0, dx, dy);
			}
			else 
				if (raw->header.dwType == RIM_TYPEKEYBOARD)	{
						RAWKEYBOARD& keyboard = raw->data.keyboard;
						UINT virtualKey = keyboard.VKey;
						UINT scanCode = keyboard.MakeCode;
						UINT flags = keyboard.Flags;
						bool keyDown = !(flags & RI_KEY_BREAK);
				}

			delete[] lpb;
			return 0;
		}
		case WM_ERASEBKGND:
			return 1;
		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hwnd, &ps);
			EndPaint(hwnd, &ps);
			return 0;
		}
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WinClass::WRun(GameTimer* gt) {
	MSG msg = {};
	gt->Reset();

	while (true) {
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				break;
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {
			gt->Tick();
			myApp.Stats(*gt, hWnd);
			myApp.Update(*gt);
			myApp.Draw(*gt);
		}
	}

	return static_cast<int>(msg.wParam);
}

void myAppInit() {
	myApp.InitializeDevice();
	myApp.InitializeCommandObjects();
	myApp.CreateSwapChain(g_hWnd);
	myApp.CreateRTVAndDSVDescriptorHeaps();
	myApp.CreateRTV();
	myApp.CreateDSV();
	myApp.SetViewport();
	myApp.SetScissor();
	myApp.BuildLayout();
	myApp.InitProjectionMatrix();
	myApp.ParseFile();
	myApp.CreateVertexBuffer();
	myApp.CreateIndexBuffer();
	myApp.InitBuffer();
	myApp.CreateCBVDescriptorHeap();
	myApp.CreateConstantBufferView();
	myApp.CreateRootSignature();
	myApp.CompileShaders();
	myApp.CreatePSO();
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
	WinClass win(hInstance, hPrevInstance);
	win.initWin(WinProc);
	win.CreateWin();
	if (!win.CheckCreation()) {
		return 0;
	}
	g_hWnd = win.getHWND();

	myAppInit();
	
	GameTimer timer;

	win.RegisterRawInputDevice();
	win.ShowWin();

	win.WRun(&timer);

	return 0;
}