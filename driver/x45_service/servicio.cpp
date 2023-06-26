/*--
Copyright (c) 2005-2007 Alfredo Costalago

Module Name:

    servicio.c
--*/

#include <windows.h>
#include <winioctl.h>
#include "servicio.h"

#define IOCTL_USR_CALIBRADO		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0100, METHOD_BUFFERED, FILE_WRITE_ACCESS)

CServicio::CServicio(){}

/****************************************************
			FUNCIONES DE INICIO
*****************************************************/

void CServicio::IniciarServicio()
{
	CargarCalibrado();
}

void CServicio::CargarCalibrado()
{
	CALIBRADO ejes[4];
	RtlZeroMemory(ejes,sizeof(CALIBRADO)*4);
	LeerRegistro(ejes);
	
	DWORD ret;

	HANDLE driver=CreateFile(
			"\\\\.\\XHOTASHidInterface",
			GENERIC_WRITE,
			FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			0,
			NULL);
	if(driver==INVALID_HANDLE_VALUE) {
		//MessageBox(NULL,"Error opening device","[X52-Service][2.1]",MB_SERVICE_NOTIFICATION|MB_ICONWARNING);
		return;
	}
	if(!DeviceIoControl(driver,IOCTL_USR_CALIBRADO,ejes,sizeof(CALIBRADO)*4,NULL,0,&ret,NULL))
		MessageBox(NULL,"Error accesing device","[X52-Service][2.2]",MB_SERVICE_NOTIFICATION|MB_ICONWARNING);

	CloseHandle(driver);
}

void CServicio::LeerRegistro(CALIBRADO* datosEje)
{
	HKEY key;
	DWORD tipo,tam=sizeof(CALIBRADO);
	LONG res;

	if(ERROR_SUCCESS!=RegOpenKeyEx(HKEY_LOCAL_MACHINE,"SYSTEM\\CurrentControlSet\\Control\\MediaProperties\\PrivateProperties\\Joystick\\Calibrado",0,KEY_READ,&key))
		return ;

	res=RegQueryValueEx(key,"Eje1",0,&tipo,(BYTE*)&datosEje[0],&tam);
	if(ERROR_SUCCESS!=res || tam!=sizeof(CALIBRADO)) RtlZeroMemory(&datosEje[0],sizeof(CALIBRADO));
	res=RegQueryValueEx(key,"Eje2",0,&tipo,(BYTE*)&datosEje[1],&tam);
	if(ERROR_SUCCESS!=res || tam!=sizeof(CALIBRADO)) RtlZeroMemory(&datosEje[1],sizeof(CALIBRADO));
	res=RegQueryValueEx(key,"Eje3",0,&tipo,(BYTE*)&datosEje[2],&tam);
	if(ERROR_SUCCESS!=res || tam!=sizeof(CALIBRADO)) RtlZeroMemory(&datosEje[2],sizeof(CALIBRADO));
	res=RegQueryValueEx(key,"Eje4",0,&tipo,(BYTE*)&datosEje[3],&tam);
	if(ERROR_SUCCESS!=res || tam!=sizeof(CALIBRADO)) RtlZeroMemory(&datosEje[3],sizeof(CALIBRADO));

	RegCloseKey(key);
}