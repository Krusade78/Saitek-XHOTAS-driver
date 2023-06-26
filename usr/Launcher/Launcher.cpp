// Launcher.cpp: define el punto de entrada de la aplicación.
//
// we need commctrl v6 for LoadIconMetric()
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include "stdafx.h"
#include "MenuLauncher.h"
#include "resource.h"

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR /*lpCmdLine*/, int )
{
    HWND hwnd = CreateWindow(L"STATIC", L"XLauncher", 0, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);
	if(hwnd)
	{
		CMenuLauncher menu(hInstance, hwnd);
		SetWindowLongPtr(hwnd,GWLP_USERDATA,(LONG)&menu);
		SetWindowLongPtr(hwnd,GWLP_WNDPROC,(LONG)WndProc);
		if(menu.AddNotificationIcon())
		{
			// Main message loop:
			MSG msg;
			while (GetMessage(&msg, NULL, 0, 0))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}
    return 0;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	CMenuLauncher* menu = (CMenuLauncher*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	if(menu != NULL) menu->MenuProc(hwnd, message, wParam, lParam);
    switch (message)
    {
		case WM_COMMAND:
			if(wParam == IDM_SALIR)
			{
				if(menu != NULL) menu->DeleteNotificationIcon();
				PostQuitMessage(0);
				break;
			}
			break;
		case WM_DESTROY:
			if(menu != NULL) menu->DeleteNotificationIcon();
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;
}
