// DrvAPI.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "instalar.h"

static HINSTANCE hInst;

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	hInst = hModule;
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
    return TRUE;
}

__declspec(dllexport) BYTE PrepararTeclados(HWND lista)
{
	CInstalar inst;
	return inst.PrepararDlgTeclados(lista);
}
__declspec(dllexport) BYTE PrepararInterfaces(HWND lista)
{
	CInstalar inst;
	return inst.PrepararDlgInterfaces(lista);
}