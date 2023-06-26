#pragma once
class CMenuLauncher
{
public:
	CMenuLauncher(HINSTANCE hInst, HWND hWnd);
	~CMenuLauncher(void);

	BOOL AddNotificationIcon();
	BOOL DeleteNotificationIcon();
	void MenuProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
	static const UINT WMAPP_NOTIFYCALLBACK = WM_APP + 1;
	HINSTANCE g_hInst;
	HWND hWnd;

	void CargarPerfilesMenu(HMENU menu);
	void ShowContextMenu(HWND hwnd, POINT pt);
};

