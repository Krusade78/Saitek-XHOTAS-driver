#include "StdAfx.h"
#include "MenuLauncher.h"
#include "resource.h"
#include <shellapi.h>
#include <strsafe.h>

CMenuLauncher::CMenuLauncher(HINSTANCE hInst, HWND hWnd)
{
	this->g_hInst = hInst;
	this->hWnd = hWnd;
}


CMenuLauncher::~CMenuLauncher(void)
{
}

#pragma region "Icono"
BOOL CMenuLauncher::AddNotificationIcon()
{
    NOTIFYICONDATA nid = {sizeof(nid)};
    nid.hWnd = hWnd;
    nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE | NIF_SHOWTIP;
	nid.uID = 1;
    nid.uCallbackMessage = WMAPP_NOTIFYCALLBACK;
    nid.hIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_LAUNCHER));
    StringCchCopy(nid.szTip,128,L"XLauncher");
    Shell_NotifyIcon(NIM_ADD, &nid);

    // NOTIFYICON_VERSION_4 is prefered
    nid.uVersion = NOTIFYICON_VERSION_4;
    return Shell_NotifyIcon(NIM_SETVERSION, &nid);
}

BOOL CMenuLauncher::DeleteNotificationIcon()
{
    NOTIFYICONDATA nid = {sizeof(nid)};
	nid.hWnd = hWnd;
    nid.uID = 1;
    return Shell_NotifyIcon(NIM_DELETE, &nid);
}
#pragma endregion

void CMenuLauncher::CargarPerfilesMenu(HMENU menu)
{
	UINT_PTR id = 100;
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind = FindFirstFile(L".\\*.xhp", &FindFileData);
	if (hFind != INVALID_HANDLE_VALUE) 
	{
		AppendMenu(menu, MF_STRING, id, FindFileData.cFileName);
		id++;
		while (FindNextFile(hFind, &FindFileData) != 0)
		{
			AppendMenu(menu, MF_STRING, id, FindFileData.cFileName);
			id++;
		}
		FindClose(hFind);
	}
}

void CMenuLauncher::ShowContextMenu(HWND hwnd, POINT pt)
{
    HMENU hMenu = LoadMenu(g_hInst, MAKEINTRESOURCE(IDC_LAUNCHER));
    if (hMenu)
    {
		HANDLE bmp1 = LoadImage(g_hInst, MAKEINTRESOURCE(IDB_BITMAP1), 0, 0, 0, LR_CREATEDIBSECTION);
		HANDLE bmp2 = LoadImage(g_hInst, MAKEINTRESOURCE(IDB_BITMAP2), 0, 0, 0, LR_CREATEDIBSECTION);
		HANDLE bmp3 = LoadImage(g_hInst, MAKEINTRESOURCE(IDB_BITMAP3), 0, 0, 0, LR_CREATEDIBSECTION);
		
		HMENU mu1 = CreatePopupMenu();
		HMENU mu2 = CreatePopupMenu();
		CargarPerfilesMenu(mu1);
		CargarPerfilesMenu(mu2);

        HMENU hSubMenu = GetSubMenu(hMenu, 0);
        if (hSubMenu)
        {
			MENUITEMINFO mi;
				ZeroMemory(&mi,sizeof(mi));
				mi.cbSize=sizeof(MENUITEMINFO);
				mi.fMask=MIIM_SUBMENU|MIIM_BITMAP;

				mi.hbmpItem = (HBITMAP)bmp3;
				mi.hSubMenu = mu1;
			SetMenuItemInfo(hSubMenu, 0, TRUE, &mi);
				mi.hbmpItem = (HBITMAP)bmp2;
				mi.hSubMenu = mu2;
			SetMenuItemInfo(hSubMenu, 3, TRUE, &mi);

				mi.fMask=MIIM_BITMAP;
				mi.hbmpItem = (HBITMAP)bmp1;
			SetMenuItemInfo(hSubMenu, 2, TRUE, &mi);

				mi.hbmpItem = HBMMENU_MBAR_CLOSE;
			SetMenuItemInfo(hSubMenu, 7, TRUE, &mi);
			DrawMenuBar(hwnd);

			// our window must be foreground before calling TrackPopupMenu or the menu will not disappear when the user clicks away
            SetForegroundWindow(hwnd);

            // respect menu drop alignment
            UINT uFlags = TPM_RIGHTBUTTON;
            if (GetSystemMetrics(SM_MENUDROPALIGNMENT) != 0)
            {
                uFlags |= TPM_RIGHTALIGN;
            }
            else
            {
                uFlags |= TPM_LEFTALIGN;
            }

            TrackPopupMenuEx(hSubMenu, uFlags, pt.x, pt.y, hwnd, NULL);
        }

		DeleteObject(bmp1);
		DeleteObject(bmp2);
		DeleteObject(bmp3);
		DestroyMenu(mu1);
		DestroyMenu(mu2);
        DestroyMenu(hMenu);
    }
}

void CMenuLauncher::MenuProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
		case WM_COMMAND:
			switch(wParam)
			{
			case IDM_SALIR:
				break;
			}
			break;
		case WMAPP_NOTIFYCALLBACK:
			switch (LOWORD(lParam))
			{
			case WM_CONTEXTMENU:
				{
					POINT const pt = { LOWORD(wParam), HIWORD(wParam) };
					ShowContextMenu(hwnd, pt);
				}
				break;
			}
			break;
		default:
			break;
    }
}