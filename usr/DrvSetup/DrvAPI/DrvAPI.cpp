// DrvAPI.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "DrvAPI.h"
#include <stdio.h>
#include "desinstalar.h"
#include "instalar.h"

int tipo = -1;
int kb;
int gp;
int resultado = 0;
HINSTANCE hinst =NULL;
HANDLE mutex = NULL;
DWORD WINAPI Procesar(LPVOID hwnd);

INT_PTR CALLBACK DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg) {
		case (WM_USER+1):{
			if(WaitForSingleObject(mutex,0)!=WAIT_OBJECT_0) return TRUE;
			CreateThread(0,0,Procesar,hWnd,0,0);
			return TRUE;}
		case WM_DESTROY: 
			WaitForSingleObject(mutex,INFINITE);
		    // Perform cleanup tasks. 
		    PostQuitMessage(0); 
			return FALSE;
	} 
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	hinst = hInstance;

	if(strlen(lpCmdLine)==0) {
		tipo=-1;
	} else {
		try {
			sscanf(lpCmdLine,"%u %u %u",&tipo,&kb,&gp);
		} catch(...) {
			return 0;
		}
	}

	mutex = CreateEvent(NULL,FALSE,TRUE,NULL);
	HWND hWnd=CreateDialog(hInstance, MAKEINTRESOURCE(IDD_FORMVIEW), NULL, (DLGPROC)DialogProc);
	ShowWindow(hWnd,SW_SHOW);

	SendMessage(hWnd,WM_USER+1,0,0);

	// Main message loop:
	MSG msg;
	while(GetMessage(&msg, NULL, 0, 0) != 0)
	{
		if(msg.message==WM_QUIT) break;
		DispatchMessage(&msg); 
	}

	CloseHandle(mutex);
	return resultado;
}

DWORD WINAPI Procesar(LPVOID param)
{
	HWND hwnd=(HWND) param;
	switch(tipo) {
		case -1: 
			{
				SendMessage(GetDlgItem(hwnd,IDC_LABEL),WM_SETTEXT,0,(LPARAM)"Uninstalling drivers...");
				UpdateWindow(hwnd);
				CDesinstalar unins(hwnd,hinst);
				if(unins.Iniciar()) resultado = 1;
			}
			break;
		case 0: //x36 gameport
			{
				char r = 0;
				CInstalar inst;
				if(inst.InstalarTeclado(kb)) {
					r++;
					if(inst.InstalarWrapper(hwnd)) {
						r++;
						if(inst.InstalarVHID(hwnd)) {
							r++;
							if(inst.InstalarJoystick(hwnd,hinst,gp,false)) r++;
						}
					}
				}
				resultado = r;
			}
			break;
		case 1: //x36+35 gameport
			{
				char r = 0;
				CInstalar inst;
				if(inst.InstalarTeclado(kb)) {
					r++;
					if(inst.InstalarWrapper(hwnd)) {
						r++;
						if(inst.InstalarVHID(hwnd)) {
							r++;
							if(inst.InstalarJoystick(hwnd,hinst,gp,true)) r++;
						}
					}
				}
				resultado = r;
			}
			break;
		case 2: //x36+35 gameport manual
			{
				char r = 0;
				CInstalar inst;
				if(inst.InstalarTeclado(kb)) {
					r++;
					if(inst.InstalarWrapper(hwnd)) {
						r++;
						if(inst.InstalarVHID(hwnd)) {
							r++;
						}
					}
				}
				resultado = r;
			}
			break;
		default:
			{
				char r = 0;
				CInstalar inst;
				if(inst.InstalarVHID(hwnd)) {
					r++;
					if(inst.InstalarJoystickUSB()) r++;
				}
				resultado = r;
			}
	}

	SetEvent(mutex);
	SendMessage(hwnd,WM_CLOSE,0,0);
	return 0;
}