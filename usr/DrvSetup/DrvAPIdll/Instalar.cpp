// Instalar.cpp: Instalación del X36 Driver
//


#include "stdafx.h"
#include <setupapi.h>
#include "instalar.h"


CInstalar::CInstalar()//,bool manual)
{
}

CInstalar::~CInstalar(void){}

// Prepara dialógos

char CInstalar::PrepararDlgTeclados(HWND lista)
{
	CONST GUID kbdc={0x4D36E96B,0xe325,0x11CE,{0xbF,0xC1,0x08,0x00,0x2B,0xE1,0x03,0x18}};
	SP_DEVINFO_DATA dev;
	DWORD idx=0;

	HDEVINFO di=SetupDiGetClassDevs(&kbdc,NULL,NULL,DIGCF_PRESENT);
	if(di==INVALID_HANDLE_VALUE)
	{
		Error("PrepararDlgTeclados[0]");
		return 0;
	}
			
	dev.cbSize=sizeof(SP_DEVINFO_DATA);
	while(SetupDiEnumDeviceInfo(di,idx,&dev)) {
		idx++;
		DWORD tam=0;
		BYTE* desc=NULL;
		SetupDiGetDeviceRegistryProperty(di,&dev,SPDRP_DEVICEDESC,NULL,NULL,0,&tam);
		desc=new BYTE[tam];
		if(!SetupDiGetDeviceRegistryProperty(di,&dev,SPDRP_DEVICEDESC,NULL,desc,tam,NULL))
		{
			Error("PrepararDlgTeclados[1]");
			delete []desc; desc = NULL;
			SetupDiDestroyDeviceInfoList(di);
			return 0;
		}

		SendMessage(lista,LB_ADDSTRING,0,(LPARAM)desc);

		delete []desc; desc = NULL;
	}
	SetupDiDestroyDeviceInfoList(di);

	return (char)idx;
}


char CInstalar::PrepararDlgInterfaces(HWND lista)
{
	DWORD i=0;
	CONST GUID enumclass={0xcae56030,0x684a,0x11d0,{0xd6,0xf6,0x00,0xa0,0xc9,0x0f,0x57,0xda}};
	HDEVINFO di=SetupDiGetClassDevs(&enumclass,NULL,NULL,DIGCF_PRESENT|DIGCF_DEVICEINTERFACE );
	if(di==INVALID_HANDLE_VALUE)
	{
		Error("CInstalar::EnumerarGameports[0]");
		return 0;
	}
	SP_DEVINFO_DATA dev;
	dev.cbSize=sizeof(SP_DEVINFO_DATA);

	while(SetupDiEnumDeviceInfo(di,i,&dev)) {
		i++;
		DWORD tam=0;
		BYTE* desc=NULL;
		SetupDiGetDeviceRegistryProperty(di,&dev,SPDRP_DEVICEDESC,NULL,NULL,0,&tam);
		desc=new BYTE[tam];
		if(!SetupDiGetDeviceRegistryProperty(di,&dev,SPDRP_DEVICEDESC,NULL,desc,tam,NULL))
		{
			Error("CInstalar::EnumerarGameports[1]");
			delete []desc; desc = NULL;
			SetupDiDestroyDeviceInfoList(di);
			return 0;
		}

		SendMessage(lista,LB_ADDSTRING,0,(LPARAM)desc);

		delete []desc; desc = NULL;
	}
	SetupDiDestroyDeviceInfoList(di);

	return (char)i;
}

// Errores

void CInstalar::Error(char* tit)
{
	LPCTSTR msg;
	FormatMessage( 
	    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &msg,
		0, NULL );
	MessageBox(NULL,msg,tit,MB_ICONERROR);
}
void CInstalar::Error(char* tit, HRESULT res)
{
	LPCTSTR msg;
	FormatMessage( 
	    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		res,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &msg,
		0, NULL );
	MessageBox(NULL,msg,tit,MB_ICONERROR);
}
