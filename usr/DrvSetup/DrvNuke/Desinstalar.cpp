// Desinstalar.cpp: Instalación del X36 Driver
//

#include "stdafx.h"
#include <setupapi.h>
#include "joyconfig.h"
#include "desinstalar.h"
//#include "newdev.h"


CDesinstalar::CDesinstalar(/*HWND hWnd,HINSTANCE hInst*/)
{
	//this->hWnd = hWnd;
	//this->hInst = hInst;
}

CDesinstalar::~CDesinstalar(void){}

void CDesinstalar::Iniciar()
{
	Joystick();
	JoystickUSB();
	Wrapper();
	VHID();
	FiltroTeclado();
	Base();
}

bool CDesinstalar::Base()
{
	// Borrar archivos

	bool ok=true;
	HINF inf=SetupOpenInfFile(".\\uninstall.inf",NULL,INF_STYLE_WIN4,NULL);
	if(inf==INVALID_HANDLE_VALUE)
	{
		//Error("Un-Base[0]");
		return false;
	}

	HSPFILEQ fq=SetupOpenFileQueue();
	if(fq==INVALID_HANDLE_VALUE)
	{
		//Error("Un-Base[1]");
		ok=false;
		goto inf_clear;
	}
	PVOID ctx=SetupInitDefaultQueueCallback(NULL);

	if(!SetupInstallFromInfSection(NULL,inf,"uninstall.reg",SPINST_REGISTRY,NULL,NULL,NULL,NULL,NULL,NULL,NULL))
	{
		//Error("Un-Base[2]");
		ok=false;
		goto clear;
	}

	if(!SetupInstallFilesFromInfSection(inf,inf,fq,"uninstall.base",NULL,SP_COPY_NEWER_OR_SAME))
	{
		//Error("Un-Base[3]");
		ok=false;
		goto clear;
	}

	if(!SetupCommitFileQueue(NULL,fq,SetupDefaultQueueCallback,ctx))
	{
		//Error("Un-Base[4]");
		ok=false;
	}

clear:
	SetupTermDefaultQueueCallback(ctx);
	SetupCloseFileQueue(fq);
inf_clear:
	SetupCloseInfFile(inf);

	return ok;
}
bool CDesinstalar::FiltroTeclado()
{
	CONST GUID kbdc={0x4D36E96B,0xe325,0x11CE,{0xbF,0xC1,0x08,0x00,0x2B,0xE1,0x03,0x18}};
	SP_DEVINFO_DATA dev;
	DWORD idx=0;

	HDEVINFO di=SetupDiGetClassDevs(&kbdc,NULL,NULL,DIGCF_PRESENT);
	if(di==INVALID_HANDLE_VALUE)
	{
		//Error("Un-Teclado[0]");
		return false;
	}
			
	dev.cbSize=sizeof(SP_DEVINFO_DATA);
	while(SetupDiEnumDeviceInfo(di,idx,&dev)) {
		idx++;
		DWORD tam=0;
		BYTE* desc=NULL;
		SetupDiGetDeviceRegistryProperty(di,&dev,SPDRP_UPPERFILTERS,NULL,NULL,0,&tam);
		if(tam!=0) {
			desc=new BYTE[tam];
			if(!SetupDiGetDeviceRegistryProperty(di,&dev,SPDRP_UPPERFILTERS,NULL,desc,tam,NULL))
			{
				//Error("Un-Teclado[1]");
				delete []desc; desc = NULL;
				//SetupDiDestroyDeviceInfoList(di);
				//return false;
				continue;
			}

			if(_stricmp((char*)desc,"X36KbFilter")==0) {
				if(!SetupDiSetDeviceRegistryProperty(di,&dev,SPDRP_UPPERFILTERS,(BYTE*)"",1)) {
					//Error("Un-Teclado[2]");
					//SetupDiDestroyDeviceInfoList(di);
					//return false;
					continue;
				}
			}

			delete []desc; desc = NULL;
		}
	}
	SetupDiDestroyDeviceInfoList(di);

	return true;

	// Borrar archivos

//	bool ok=true;
//	HINF inf=SetupOpenInfFile(".\\uninstall.inf",NULL,INF_STYLE_WIN4,NULL);
//	if(inf==INVALID_HANDLE_VALUE)
//	{
//		Error("Un-Teclado[3]");
//		return false;
//	}
//
//	HSPFILEQ fq=SetupOpenFileQueue();
//	if(fq==INVALID_HANDLE_VALUE)
//	{
//		Error("Un-Teclado[4]");
//		ok=false;
//		goto inf_clear;
//	}
//	PVOID ctx=SetupInitDefaultQueueCallback(NULL);
//
//	if(!SetupInstallServicesFromInfSection(inf,"uninstall.services.kb",0))
//	{
//		Error("Un-Teclado[5]");
//		ok=false;
//		goto clear;
//	}
//
//	if(!SetupCommitFileQueue(NULL,fq,SetupDefaultQueueCallback,ctx))
//	{
//		Error("Un-Teclado[6]");
//		ok=false;
//	}
//
//clear:
//	SetupTermDefaultQueueCallback(ctx);
//	SetupCloseFileQueue(fq);
//inf_clear:
//	SetupCloseInfFile(inf);
//
//	return ok;
}


bool CDesinstalar::Wrapper()
{
	//Borrar del Pnp
	{
		CONST GUID guidHid={0x745a17a0,0x74d3,0x11d0,{0xb6,0xfe,0x00,0xa0,0xc9,0x0f,0x57,0xda}};
		SP_DEVINFO_DATA dev;
		DWORD idx=0;

		// Mini driver
		HDEVINFO di=SetupDiGetClassDevs(&guidHid,NULL,NULL,DIGCF_PRESENT);
		if(di==INVALID_HANDLE_VALUE)
		{
			//Error("Un-Wrapper[0]");
			return false;
		}

		dev.cbSize=sizeof(SP_DEVINFO_DATA);
		while(SetupDiEnumDeviceInfo(di,idx,&dev)) {
			idx++;
			DWORD tam=0;
			BYTE* desc=NULL;
			SetupDiGetDeviceRegistryProperty(di,&dev,SPDRP_HARDWAREID,NULL,NULL,0,&tam);
			if(tam!=0) {
				desc=new BYTE[tam];
				if(!SetupDiGetDeviceRegistryProperty(di,&dev,SPDRP_HARDWAREID,NULL,desc,tam,NULL))
				{
					//Error("Un-Wrapper[1]");
					delete []desc; desc = NULL;
					//SetupDiDestroyDeviceInfoList(di);
					//return false;
					continue;
				}
				if(_stricmp((char*)desc,"VHID\\X36WrapperMinidriver")==0 || _stricmp((char*)desc,"HID\\NullX36WrapperHIDDevice")==0) {
					SetupDiRemoveDevice(di,&dev);
				}
			}

			delete []desc; desc = NULL;
		}

		SetupDiDestroyDeviceInfoList(di);
	}

	// Archivos

	bool ok=true;

	HINF inf=SetupOpenInfFile(".\\uninstall.inf",NULL,INF_STYLE_WIN4,NULL);
	if(inf==INVALID_HANDLE_VALUE)
	{
		//Error("Un-Wrapper[2]");
		return false;
	}

	HSPFILEQ fq=SetupOpenFileQueue();
	if(fq==INVALID_HANDLE_VALUE)
	{
		//Error("Un-Wrapper[3]");
		ok=false;
		goto inf_clear;
	}
	PVOID ctx=SetupInitDefaultQueueCallback(NULL);

	if(!SetupInstallServicesFromInfSection(inf,"uninstall.services.wrapper",0))
	{
		//Error("Un-Wrapper[4]");
		ok=false;
		goto clear;
	}

	if(!SetupCommitFileQueue(NULL,fq,SetupDefaultQueueCallback,ctx))
	{
		//Error("Un-Wrapper[5]");
		ok=false;
	}

clear:
	SetupTermDefaultQueueCallback(ctx);
	SetupCloseFileQueue(fq);
inf_clear:
	SetupCloseInfFile(inf);

	if(!ok) {
		return false;
	} else {
		return true;//Base();
	}
}

bool CDesinstalar::VHID()
{
	//Borrar del Pnp
	{
		CONST GUID guidKb={0x4D36E96B,0xe325,0x11CE,{0xbF,0xC1,0x08,0x00,0x2B,0xE1,0x03,0x18}};
		CONST GUID guidMou={0x4D36E96F,0xe325,0x11CE,{0xbF,0xC1,0x08,0x00,0x2B,0xE1,0x03,0x18}};
		CONST GUID guidHid={0x745a17a0,0x74d3,0x11d0,{0xb6,0xfe,0x00,0xa0,0xc9,0x0f,0x57,0xda}};
		SP_DEVINFO_DATA dev;
		DWORD idx=0;
		
		// Teclado

		HDEVINFO di=SetupDiGetClassDevs(&guidKb,NULL,NULL,DIGCF_PRESENT);
		if(di==INVALID_HANDLE_VALUE)
		{
			//Error("Un-VHID[0]");
			goto siguiente;
			//return false;
		}
			
		dev.cbSize=sizeof(SP_DEVINFO_DATA);
		while(SetupDiEnumDeviceInfo(di,idx,&dev)) {
			idx++;
			DWORD tam=0;
			BYTE* desc=NULL;
			SetupDiGetDeviceRegistryProperty(di,&dev,SPDRP_HARDWAREID,NULL,NULL,0,&tam);
			if(tam!=0) {
				desc=new BYTE[tam];
				if(!SetupDiGetDeviceRegistryProperty(di,&dev,SPDRP_HARDWAREID,NULL,desc,tam,NULL))
				{
					//Error("Un-VHID[1]");
					delete []desc; desc = NULL;
					//SetupDiDestroyDeviceInfoList(di);
					//return false;
					continue;
				}
				if(tam>24) desc[24]=0;
				if(_stricmp((char*)desc,"HID\\NullVirtualHidDevice")==0) {
					SetupDiRemoveDevice(di,&dev);
				}
			}

			delete []desc; desc = NULL;
		}

		SetupDiDestroyDeviceInfoList(di);

		// Ratón
siguiente:
		di=SetupDiGetClassDevs(&guidMou,NULL,NULL,DIGCF_PRESENT);
		if(di==INVALID_HANDLE_VALUE)
		{
			//Error("Un-VHID[2]");
			goto siguiente2;
			//return false;
		}
		idx=0;	
		dev.cbSize=sizeof(SP_DEVINFO_DATA);
		while(SetupDiEnumDeviceInfo(di,idx,&dev)) {
			idx++;
			DWORD tam=0;
			BYTE* desc=NULL;
			SetupDiGetDeviceRegistryProperty(di,&dev,SPDRP_HARDWAREID,NULL,NULL,0,&tam);
			if(tam!=0) {
				desc=new BYTE[tam];
				if(!SetupDiGetDeviceRegistryProperty(di,&dev,SPDRP_HARDWAREID,NULL,desc,tam,NULL))
				{
					//Error("Un-VHID[3]");
					delete []desc; desc = NULL;
					//SetupDiDestroyDeviceInfoList(di);
					//return false;
					continue;
				}
				if(tam>24) desc[24]=0;
				if(_stricmp((char*)desc,"HID\\NullVirtualHidDevice")==0) {
					SetupDiRemoveDevice(di,&dev);
				}
			}

			delete []desc; desc = NULL;
		}

		SetupDiDestroyDeviceInfoList(di);

		// Mini driver
siguiente2:
		di=SetupDiGetClassDevs(&guidHid,NULL,NULL,DIGCF_PRESENT);
		if(di==INVALID_HANDLE_VALUE)
		{
			//Error("Un-VHID[4]");
			goto archivos;
			//return false;
		}
		idx=0;
		while(SetupDiEnumDeviceInfo(di,idx,&dev)) {
			idx++;
			DWORD tam=0;
			BYTE* desc=NULL;
			SetupDiGetDeviceRegistryProperty(di,&dev,SPDRP_HARDWAREID,NULL,NULL,0,&tam);
			if(tam!=0) {
				desc=new BYTE[tam];
				if(!SetupDiGetDeviceRegistryProperty(di,&dev,SPDRP_HARDWAREID,NULL,desc,tam,NULL))
				{
					//Error("Un-VHID[5]");
					delete []desc; desc = NULL;
					//SetupDiDestroyDeviceInfoList(di);
					//return false;
					continue;
				}
				if(_stricmp((char*)desc,"VHID\\XHOTASVirtualMinidriver")==0 || _stricmp((char*)desc,"HID\\NullVirtualHidDevice")==0) {
					SetupDiRemoveDevice(di,&dev);
				}
			}

			delete []desc; desc = NULL;
		}

		SetupDiDestroyDeviceInfoList(di);
	}


	// Archivos
archivos:
	bool ok=true;

	HINF inf=SetupOpenInfFile(".\\uninstall.inf",NULL,INF_STYLE_WIN4,NULL);
	if(inf==INVALID_HANDLE_VALUE)
	{
		//Error("Un-VHID[6]");
		return false;
	}

	HSPFILEQ fq=SetupOpenFileQueue();
	if(fq==INVALID_HANDLE_VALUE)
	{
		//Error("Un-VHID[7]");
		ok=false;
		goto inf_clear;
	}
	PVOID ctx=SetupInitDefaultQueueCallback(NULL);

	if(!SetupInstallServicesFromInfSection(inf,"uninstall.services.vhid",0))
	{
		//Error("Un-VHID[8]");
		ok=false;
		goto clear;
	}

	if(!SetupCommitFileQueue(NULL,fq,SetupDefaultQueueCallback,ctx))
	{
		//Error("Un-VHID[9]");
		ok=false;
	}

clear:
	SetupTermDefaultQueueCallback(ctx);
	SetupCloseFileQueue(fq);
inf_clear:
	SetupCloseInfFile(inf);

	if(!ok) {
		return false;
	} else {
		return true;//Base();
	}
}

bool CDesinstalar::Joystick()
{
	CjoyConfig jc;
	/*if(!*/jc.Desinstalar();//) return false;

	//Borrar del Pnp
	{
		GUID guidHid={0x745a17a0,0x74d3,0x11d0,{0xb6,0xfe,0x00,0xa0,0xc9,0x0f,0x57,0xda}};
		SP_DEVINFO_DATA dev;
		DWORD idx=0;
		HDEVINFO di=SetupDiGetClassDevs(&guidHid,NULL,NULL,0);//DIGCF_PRESENT);
		if(di==INVALID_HANDLE_VALUE)
		{
			//Error("Un-Joystick[0]");
			return false;
		}
			
		dev.cbSize=sizeof(SP_DEVINFO_DATA);
		while(SetupDiEnumDeviceInfo(di,idx,&dev)) {
			idx++;
			DWORD tam=0;
			BYTE* desc=NULL;
			SetupDiGetDeviceRegistryProperty(di,&dev,SPDRP_HARDWAREID,NULL,NULL,0,&tam);
			if(tam!=0) {
				desc=new BYTE[tam];
				if(!SetupDiGetDeviceRegistryProperty(di,&dev,SPDRP_HARDWAREID,NULL,desc,tam,NULL))
				{
					//Error("Un-Joystick[1]");
					delete []desc; desc = NULL;
					//SetupDiDestroyDeviceInfoList(di);
					//return false;
					continue;
				}
				if(_stricmp((char*)desc,"GamePort\\VID_06A3&PID_3635")==0 || _stricmp((char*)desc,"GamePort\\VID_06A3&PID_3600")==0 ||
					_stricmp((char*)desc,"HID\\VID_06A3&PID_3635")==0 || _stricmp((char*)desc,"HID\\VID_06A3&PID_3600")==0) {
					SetupDiRemoveDevice(di,&dev);
				}
			}

			delete []desc; desc = NULL;
		}

		SetupDiDestroyDeviceInfoList(di);
	}

	// Borrar archivos

	bool ok=true;
	HINF inf=SetupOpenInfFile(".\\uninstall.inf",NULL,INF_STYLE_WIN4,NULL);
	if(inf==INVALID_HANDLE_VALUE)
	{
		//Error("Un-Joystick[2]");
		return false;
	}

	HSPFILEQ fq=SetupOpenFileQueue();
	if(fq==INVALID_HANDLE_VALUE)
	{
		//Error("Un-Joystick[3]");
		ok=false;
		goto inf_clear;
	}
	PVOID ctx=SetupInitDefaultQueueCallback(NULL);

	if(!SetupInstallServicesFromInfSection(inf,"uninstall.services.joy",0))
	{
		//Error("Un-Joystick[4]");
		ok=false;
		goto clear;
	}

	if(!SetupCommitFileQueue(NULL,fq,SetupDefaultQueueCallback,ctx))
	{
		//Error("Un-Joystick[5]");
		ok=false;
	}

clear:
	SetupTermDefaultQueueCallback(ctx);
	SetupCloseFileQueue(fq);
inf_clear:
	SetupCloseInfFile(inf);

	return ok;
}
bool CDesinstalar::JoystickUSB()
{
	//Borrar del Pnp
	{
		GUID guidHid={0x745a17a0,0x74d3,0x11d0,{0xb6,0xfe,0x00,0xa0,0xc9,0x0f,0x57,0xda}};
		SP_DEVINFO_DATA dev;
		DWORD idx=0;
		HDEVINFO di=SetupDiGetClassDevs(&guidHid,NULL,NULL,0);//DIGCF_PRESENT);
		if(di==INVALID_HANDLE_VALUE)
		{
			//Error("Un-JoystickUSB[0]");
			return false;
		}
			
		dev.cbSize=sizeof(SP_DEVINFO_DATA);
		while(SetupDiEnumDeviceInfo(di,idx,&dev)) {
			idx++;
			DWORD tam=0;
			BYTE* desc=NULL;
			SetupDiGetDeviceRegistryProperty(di,&dev,SPDRP_HARDWAREID,NULL,NULL,0,&tam);
			if(tam!=0) {
				desc=new BYTE[tam];
				if(!SetupDiGetDeviceRegistryProperty(di,&dev,SPDRP_HARDWAREID,NULL,desc,tam,NULL))
				{
					//Error("Un-JoystickUSB[1]");
					delete []desc; desc = NULL;
					//SetupDiDestroyDeviceInfoList(di);
					//return false;
					continue;
				}
				if(tam>=21) desc[21]=0;
				if((_stricmp((char*)desc,"USB\\Vid_06a3&Pid_0255")==0) || (_stricmp((char*)desc,"USB\\VID_06A3&PID_053F")==0) ||
				   (_stricmp((char*)desc,"USB\\VID_06A3&PID_803F")==0) || (_stricmp((char*)desc,"USB\\VID_06A3&PID_053C")==0) ||
				   (_stricmp((char*)desc,"USB\\VID_06A3&PID_803C")==0) || (_stricmp((char*)desc,"USB\\VID_06A3&PID_2541")==0) ||
				   (_stricmp((char*)desc,"HID\\Vid_06a3&Pid_0255")==0) || (_stricmp((char*)desc,"HID\\VID_06A3&PID_053F")==0) ||
				   (_stricmp((char*)desc,"HID\\VID_06A3&PID_803F")==0) || (_stricmp((char*)desc,"HID\\VID_06A3&PID_053C")==0) ||
				   (_stricmp((char*)desc,"HID\\VID_06A3&PID_803C")==0) || (_stricmp((char*)desc,"HID\\VID_06A3&PID_2541")==0)) {
						SetupDiRemoveDevice(di,&dev);
						//Sleep(2000);//UpdateDriverForPlugAndPlayDevices(NULL,(char*)desc,"input.inf",0,NULL);
				}
			}

			delete []desc; desc = NULL;
		}

		SetupDiDestroyDeviceInfoList(di);
	}

	// Borrar archivos

	bool ok=true;
	HINF inf=SetupOpenInfFile(".\\uninstall.inf",NULL,INF_STYLE_WIN4,NULL);
	if(inf==INVALID_HANDLE_VALUE)
	{
		//Error("Un-JoystickUSB[2]");
		return false;
	}

	HSPFILEQ fq=SetupOpenFileQueue();
	if(fq==INVALID_HANDLE_VALUE)
	{
		//Error("Un-JoystickUSB[3]");
		ok=false;
		goto inf_clear;
	}
	PVOID ctx=SetupInitDefaultQueueCallback(NULL);

	if(!SetupInstallServicesFromInfSection(inf,"uninstall.services.usb",0))
	{
		//Error("Un-JoystickUSB[4]");
		ok=false;
		goto clear;
	}

	if(!SetupCommitFileQueue(NULL,fq,SetupDefaultQueueCallback,ctx))
	{
		//Error("Un-JoystickUSB[5]");
		ok=false;
	}


clear:
	SetupTermDefaultQueueCallback(ctx);
	SetupCloseFileQueue(fq);
inf_clear:
	SetupCloseInfFile(inf);

	return ok;
}

//void CDesinstalar::Error(char* tit)
//{
//	LPCTSTR msg;
//	FormatMessage( 
//	    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
//		NULL,
//		GetLastError(),
//		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
//		(LPTSTR) &msg,
//		0, NULL );
//	MessageBox(NULL,msg,tit,MB_ICONERROR);
//}