// DrvNuke.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "desinstalar.h"


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	CDesinstalar unins;
	unins.Iniciar();
	
	MessageBox(NULL,"OK","",0);

	return 0;
}