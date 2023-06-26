/*--
Copyright (c) 2005-2007 Alfredo Costalago

Module Name:

    servicio.c
--*/

#include <windows.h>
#include <winioctl.h>
#include "servicio.h"

#define IOCTL_X36_JOYINICIAR	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0100, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_X36_CALIBRADO		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0100, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_USR_CALIBRADO		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0100, METHOD_BUFFERED, FILE_WRITE_ACCESS)

CServicio::CServicio(){}

/****************************************************
			FUNCIONES DE INICIO
*****************************************************/

void CServicio::IniciarServicio()
{
	IniciarX36();
	CargarCalibradoX36();
	CargarCalibradoHID();
}

void CServicio::IniciarX36()		// Error 1
{
	DWORD ret;

	HANDLE driver=CreateFile(
			"\\\\.\\X36KbInterface",
			GENERIC_WRITE,
			FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			0,
			NULL);
	if(driver==INVALID_HANDLE_VALUE) {
		MessageBox(NULL,"Error opening device","[X36-Service][1.1]",MB_SERVICE_NOTIFICATION|MB_ICONWARNING);
		return;
	}
	if(!DeviceIoControl(driver,IOCTL_X36_JOYINICIAR,NULL,0,NULL,0,&ret,NULL))
		MessageBox(NULL,"Error accesing device","[X36-Service][1.2]",MB_SERVICE_NOTIFICATION|MB_ICONWARNING);
	CloseHandle(driver);
}


void CServicio::CargarCalibradoX36()
{
	CALIBRADOX36 ejes[4];
	RtlZeroMemory(ejes,sizeof(CALIBRADOX36)*4);
	LeerRegistroX36(ejes);
	
	DWORD ret;

	HANDLE driver=CreateFile(
			"\\\\.\\X36KbWrapperInterface",
			GENERIC_WRITE,
			FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			0,
			NULL);
	if(driver==INVALID_HANDLE_VALUE) {
		MessageBox(NULL,"Error opening device","[X36-Service][2.1]",MB_SERVICE_NOTIFICATION|MB_ICONWARNING);
		return;
	}
	if(!DeviceIoControl(driver,IOCTL_X36_CALIBRADO,ejes,sizeof(CALIBRADOX36)*4,NULL,0,&ret,NULL))
		MessageBox(NULL,"Error accesing device","[X36-Service][2.2]",MB_SERVICE_NOTIFICATION|MB_ICONWARNING);

	CloseHandle(driver);
}

void CServicio::LeerRegistroX36(CALIBRADOX36* datosEje)
{
	HKEY key;
	DWORD tipo,tam=sizeof(CALIBRADOX36);
	LONG res;

	if(ERROR_SUCCESS!=RegOpenKeyEx(HKEY_LOCAL_MACHINE,"SYSTEM\\CurrentControlSet\\Control\\MediaProperties\\PrivateProperties\\Joystick\\Calibrado",0,KEY_READ,&key))
		return ;

	res=RegQueryValueEx(key,"GameportEje1",0,&tipo,(BYTE*)&datosEje[0],&tam);
	if(ERROR_SUCCESS!=res || tam!=sizeof(CALIBRADOX36)) RtlZeroMemory(&datosEje[0],sizeof(CALIBRADOX36));
	res=RegQueryValueEx(key,"GameportEje2",0,&tipo,(BYTE*)&datosEje[1],&tam);
	if(ERROR_SUCCESS!=res || tam!=sizeof(CALIBRADOX36)) RtlZeroMemory(&datosEje[1],sizeof(CALIBRADOX36));
	res=RegQueryValueEx(key,"GameportEje3",0,&tipo,(BYTE*)&datosEje[2],&tam);
	if(ERROR_SUCCESS!=res || tam!=sizeof(CALIBRADOX36)) RtlZeroMemory(&datosEje[2],sizeof(CALIBRADOX36));
	res=RegQueryValueEx(key,"GameportEje4",0,&tipo,(BYTE*)&datosEje[3],&tam);
	if(ERROR_SUCCESS!=res || tam!=sizeof(CALIBRADOX36)) RtlZeroMemory(&datosEje[3],sizeof(CALIBRADOX36));

	RegCloseKey(key);
}

void CServicio::CargarCalibradoHID()
{
	CALIBRADOHID ejes[4];
	RtlZeroMemory(ejes,sizeof(CALIBRADOHID)*4);
	LeerRegistroHID(ejes);
	
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
		MessageBox(NULL,"Error opening device","[X52-Service][3.1]",MB_SERVICE_NOTIFICATION|MB_ICONWARNING);
		return;
	}
	if(!DeviceIoControl(driver,IOCTL_USR_CALIBRADO,ejes,sizeof(CALIBRADOHID)*4,NULL,0,&ret,NULL))
		MessageBox(NULL,"Error accesing device","[X52-Service][3.2]",MB_SERVICE_NOTIFICATION|MB_ICONWARNING);

	CloseHandle(driver);
}

void CServicio::LeerRegistroHID(CALIBRADOHID* datosEje)
{
	HKEY key;
	DWORD tipo,tam=sizeof(CALIBRADOHID);
	LONG res;

	if(ERROR_SUCCESS!=RegOpenKeyEx(HKEY_LOCAL_MACHINE,"SYSTEM\\CurrentControlSet\\Control\\MediaProperties\\PrivateProperties\\Joystick\\Calibrado",0,KEY_READ,&key))
		return ;

	res=RegQueryValueEx(key,"Eje1",0,&tipo,(BYTE*)&datosEje[0],&tam);
	if(ERROR_SUCCESS!=res || tam!=sizeof(CALIBRADOHID)) RtlZeroMemory(&datosEje[0],sizeof(CALIBRADOHID));
	res=RegQueryValueEx(key,"Eje2",0,&tipo,(BYTE*)&datosEje[1],&tam);
	if(ERROR_SUCCESS!=res || tam!=sizeof(CALIBRADOHID)) RtlZeroMemory(&datosEje[1],sizeof(CALIBRADOHID));
	res=RegQueryValueEx(key,"Eje3",0,&tipo,(BYTE*)&datosEje[2],&tam);
	if(ERROR_SUCCESS!=res || tam!=sizeof(CALIBRADOHID)) RtlZeroMemory(&datosEje[2],sizeof(CALIBRADOHID));
	res=RegQueryValueEx(key,"Eje4",0,&tipo,(BYTE*)&datosEje[3],&tam);
	if(ERROR_SUCCESS!=res || tam!=sizeof(CALIBRADOHID)) RtlZeroMemory(&datosEje[3],sizeof(CALIBRADOHID));

	RegCloseKey(key);
}