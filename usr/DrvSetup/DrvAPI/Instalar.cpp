// Instalar.cpp: Instalación del X36 Driver
//


#include "stdafx.h"
#include <setupapi.h>
#include "newdev.h"
#include "instalar.h"
#include "joyConfig.h"
#include "desinstalar.h"

//#pragma comment(lib,"newdev.lib")
//#pragma comment(lib,"dxguid.lib")
//#pragma comment(lib,"dinput8.lib")

CInstalar::CInstalar()//,bool manual)
{
}

CInstalar::~CInstalar(void){}

// Instalación

bool CInstalar::InstalarTeclado(DWORD st)
{
	if(!FiltroTeclado(st)) {
		CDesinstalar des(NULL,NULL);
		des.FiltroTeclado();
		return false;
	}

	return true;
}
bool CInstalar::InstalarWrapper(HWND hwnd)
{
	if(!Wrapper(hwnd)) {
		CDesinstalar des(NULL,NULL);
		des.FiltroTeclado();
		des.Wrapper();
		des.Base();
		return false;
	}

	return true;
}
bool CInstalar::InstalarVHID(HWND hwnd)
{
	if(!VHID(hwnd)) {
		CDesinstalar des(NULL,NULL);
		des.FiltroTeclado();
		des.Wrapper();
		des.VHID();
		des.Base();
		return false;
	}

	return true;
}
bool CInstalar::InstalarJoystickUSB()
{
	if(!JoystickUSB()) {
		CDesinstalar des(NULL,NULL);
		des.JoystickUSB();
		des.VHID();
		des.Base();
		return false;
	}

	return true;
}
bool CInstalar::InstalarJoystick(HWND hWnd,HINSTANCE hInst,DWORD s_interfaz, bool x35)
{
	CDesinstalar des(hWnd,hInst);
	if(!Joystick(s_interfaz,x35)) {
		des.FiltroTeclado();
		des.Joystick();
		des.Wrapper();
		des.VHID();
		des.Base();
		return false;
	}

	return true;
}

bool CInstalar::FiltroTeclado(DWORD s_teclado)
{
	CONST GUID kbdc={0x4D36E96B,0xe325,0x11CE,{0xbF,0xC1,0x08,0x00,0x2B,0xE1,0x03,0x18}};

	HDEVINFO di=SetupDiGetClassDevs(&kbdc,NULL,NULL,DIGCF_PRESENT);
	if(di==INVALID_HANDLE_VALUE)
	{
		Error("FiltroTeclado[0]");
		return false;
	}

	SP_DEVINFO_DATA dev;
	dev.cbSize=sizeof(SP_DEVINFO_DATA);
	if(SetupDiEnumDeviceInfo(di,s_teclado,&dev)) {
		if(!SetupDiSetDeviceRegistryProperty(di,&dev,SPDRP_UPPERFILTERS,(BYTE*)"X36KbFilter\0",13)) {
			Error("FiltroTeclado[1]");
			SetupDiDestroyDeviceInfoList(di);
			return false;
		}
	} else {
		Error("FiltroTeclado[2]");
		SetupDiDestroyDeviceInfoList(di);
		return false;
	}

	SetupDiDestroyDeviceInfoList(di);

	return true;
}

bool CInstalar::Wrapper(HWND hwnd)
{
	GUID guidHid={0x745a17a0,0x74d3,0x11d0,{0xb6,0xfe,0x00,0xa0,0xc9,0x0f,0x57,0xda}};
	const char hardwareId[]="VHID\\X36WrapperMinidriver\0";
	HDEVINFO di=SetupDiCreateDeviceInfoList(&guidHid,NULL);
	if(di==INVALID_HANDLE_VALUE)
	{
		Error("Wrapper[0]");
		return false;
	}

	SP_DEVINFO_DATA dev;
	dev.cbSize=sizeof(SP_DEVINFO_DATA);
	if(!SetupDiCreateDeviceInfo(di,"VHID",&guidHid,NULL,NULL,DICD_GENERATE_ID,&dev))
	{
		Error("Wrapper[1]");
		SetupDiDestroyDeviceInfoList(di);
		return false;
	}
	if(!SetupDiSetDeviceRegistryProperty(di,&dev,SPDRP_HARDWAREID,(BYTE*)hardwareId,27)) {
		Error("Wrapper[2]");
		SetupDiDestroyDeviceInfoList(di);
		return false;
	}

	//Prepara .inf
	SP_DEVINSTALL_PARAMS dip;
	dip.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
	if (!SetupDiGetDeviceInstallParams(di,&dev,&dip)) {
		Error("Wrapper[3.0]");
		SetupDiDestroyDeviceInfoList(di);
		return false;
    }
	dip.Flags |= DI_ENUMSINGLEINF;
	strcpy(dip.DriverPath,".\\x36_hid.inf");
	if (!SetupDiSetDeviceInstallParams(di,&dev,&dip)) {
		Error("Wrapper[3.1]");
		SetupDiDestroyDeviceInfoList(di);
		return false;
    }

	// Registrar dispositivo
	if (!SetupDiCallClassInstaller(DIF_REGISTERDEVICE,di,&dev))
	{
		Error("Wrapper[4]");
		SetupDiDestroyDeviceInfoList(di);
		return false;
	}

	// Instalar
	if(!SetupDiBuildDriverInfoList(di,&dev,SPDIT_COMPATDRIVER)) {
		Error("Wrapper[5.0]");
		SetupDiDestroyDeviceInfoList(di);
		return false;
	}
	SP_DRVINFO_DATA drvd;
	drvd.cbSize=sizeof(SP_DRVINFO_DATA);
	if(!SetupDiCallClassInstaller(DIF_SELECTBESTCOMPATDRV,di,&dev)){
		Error("Wrapper[5.1]");
		SetupDiDestroyDeviceInfoList(di);
		return false;
	}
	if(!SetupDiGetSelectedDriver(di,&dev,&drvd)) {
		Error("Wrapper[5.2]");
		SetupDiDestroyDeviceInfoList(di);
		return false;
	}
	if(!SetupDiCallClassInstaller(DIF_INSTALLDEVICE,di,&dev))
	{
		Error("Wrapper[5.3]");
		SetupDiDestroyDeviceInfoList(di);
		return false;
	}

	SetupDiDestroyDeviceInfoList(di);

	return true;
}

bool CInstalar::VHID(HWND hwnd)
{
	GUID guidHid={0x745a17a0,0x74d3,0x11d0,{0xb6,0xfe,0x00,0xa0,0xc9,0x0f,0x57,0xda}};
	const char hardwareId[]="VHID\\XHOTASVirtualMinidriver\0";
	HDEVINFO di=SetupDiCreateDeviceInfoList(&guidHid,NULL);
	if(di==INVALID_HANDLE_VALUE)
	{
		Error("VHID[0]");
		return false;
	}

	SP_DEVINFO_DATA dev;
	dev.cbSize=sizeof(SP_DEVINFO_DATA);
	if(!SetupDiCreateDeviceInfo(di,"VHID",&guidHid,NULL,NULL,DICD_GENERATE_ID,&dev))
	{
		Error("VHID[1]");
		SetupDiDestroyDeviceInfoList(di);
		return false;
	}
	if(!SetupDiSetDeviceRegistryProperty(di,&dev,SPDRP_HARDWAREID,(BYTE*)hardwareId,30)) {
		Error("VHID[2]");
		SetupDiDestroyDeviceInfoList(di);
		return false;
	}

	//Prepara .inf
	SP_DEVINSTALL_PARAMS dip;
	dip.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
	if (!SetupDiGetDeviceInstallParams(di,&dev,&dip)) {
		Error("VHID[3.0]");
		SetupDiDestroyDeviceInfoList(di);
		return false;
    }
	dip.Flags |= DI_ENUMSINGLEINF;
	strcpy(dip.DriverPath,".\\xh_hid.inf");
	if (!SetupDiSetDeviceInstallParams(di,&dev,&dip)) {
		Error("VHID[3.1]");
		SetupDiDestroyDeviceInfoList(di);
		return false;
    }

	if (!SetupDiCallClassInstaller(DIF_REGISTERDEVICE,di,&dev))
    {
		Error("VHID[4]");
		SetupDiDestroyDeviceInfoList(di);
		return false;
	}

	// Instalar
	if(!SetupDiBuildDriverInfoList(di,&dev,SPDIT_COMPATDRIVER)) {
		Error("VHID[5.0]");
		SetupDiDestroyDeviceInfoList(di);
		return false;
	}
	SP_DRVINFO_DATA drvd;
	drvd.cbSize=sizeof(SP_DRVINFO_DATA);
	if(!SetupDiCallClassInstaller(DIF_SELECTBESTCOMPATDRV,di,&dev)){
		Error("VHID[5.1]");
		SetupDiDestroyDeviceInfoList(di);
		return false;
	}
	if(!SetupDiGetSelectedDriver(di,&dev,&drvd)) {
		Error("VHID[5.2]");
		SetupDiDestroyDeviceInfoList(di);
		return false;
	}
	if(!SetupDiCallClassInstaller(DIF_INSTALLDEVICE,di,&dev))
	{
		Error("VHID[5.3]");
		SetupDiDestroyDeviceInfoList(di);
		return false;
	}

	SetupDiDestroyDeviceInfoList(di);

	return true;
}

bool CInstalar::JoystickUSB()
{
	const char hardwareId1[]="USB\\VID_06A3&PID_053F\0";
	const char hardwareId2[]="USB\\VID_06A3&PID_803F\0";
	const char hardwareId3[]="USB\\VID_06A3&PID_053C\0";
	const char hardwareId4[]="USB\\VID_06A3&PID_803C\0";
	const char hardwareId5[]="USB\\VID_06A3&PID_2541\0";
	const char hardwareId6[]="USB\\Vid_06a3&Pid_0255\0";
	BOOL boot;
	if(!UpdateDriverForPlugAndPlayDevices(NULL,hardwareId1,".\\xhotas.inf",INSTALLFLAG_FORCE,&boot)) {
		if(!UpdateDriverForPlugAndPlayDevices(NULL,hardwareId2,".\\xhotas.inf",INSTALLFLAG_FORCE,&boot)) {
			if(!UpdateDriverForPlugAndPlayDevices(NULL,hardwareId3,".\\xhotas.inf",INSTALLFLAG_FORCE,&boot)) {
				if(!UpdateDriverForPlugAndPlayDevices(NULL,hardwareId4,".\\xhotas.inf",INSTALLFLAG_FORCE,&boot)) {
					if(!UpdateDriverForPlugAndPlayDevices(NULL,hardwareId5,".\\xhotas.inf",INSTALLFLAG_FORCE,&boot)) {
						if(!UpdateDriverForPlugAndPlayDevices(NULL,hardwareId6,".\\xhotas.inf",INSTALLFLAG_FORCE,&boot)) {
							Error("JoystickUSB[0]");
							return false;
						}
					}
				}
			}
		}
	}

	return true;
}

bool CInstalar::Joystick(DWORD s_interface, bool x35)
{
	CjoyConfig jc;
	if(jc.Instalar((DWORD)s_interface, x35)) {
		GUID guidHid={0x745a17a0,0x74d3,0x11d0,{0xb6,0xfe,0x00,0xa0,0xc9,0x0f,0x57,0xda}};
		HDEVINFO di=SetupDiGetClassDevs(&guidHid,NULL,NULL,DIGCF_PRESENT);
		if(di==INVALID_HANDLE_VALUE)
		{
			Error("Joystick[0]");
			return false;
		}

		SP_DEVINFO_DATA dev;
		dev.cbSize=sizeof(SP_DEVINFO_DATA);
		DWORD idx,intentos=0;
reintentar:
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
					delete []desc; desc = NULL;
					continue;
				}
				if(_stricmp((char*)desc,"GamePort\\VID_06A3&PID_3635")==0 || _stricmp((char*)desc,"GamePort\\VID_06A3&PID_3600")==0) {
					delete []desc; desc = NULL;
					intentos=100;
					break;
				}
			}

			delete []desc; desc = NULL;
		}

		if(intentos<40) { intentos++; goto reintentar;}

		if (intentos==100) {
			// Instalar
			if(!SetupDiBuildDriverInfoList(di,&dev,SPDIT_COMPATDRIVER)) {
				Error("Joystick[1]");
				SetupDiDestroyDeviceInfoList(di);
				return false;
			}
			SP_DRVINFO_DATA drvd;
			drvd.cbSize=sizeof(SP_DRVINFO_DATA);
			if(!SetupDiCallClassInstaller(DIF_SELECTBESTCOMPATDRV,di,&dev)){
				Error("Joystick[2]");
				SetupDiDestroyDeviceInfoList(di);
				return false;
			}
			if(!SetupDiGetSelectedDriver(di,&dev,&drvd)) {
				Error("Joystick[3]");
				SetupDiDestroyDeviceInfoList(di);
				return false;
			}
			if(!SetupDiCallClassInstaller(DIF_INSTALLDEVICE,di,&dev))
			{
				Error("Joystick[4]");
				SetupDiDestroyDeviceInfoList(di);
				return false;
			}

			SetupDiDestroyDeviceInfoList(di);
			return true;
		} else {
			Error("Joystick[5]",ERROR_SERVICE_NOT_FOUND);
			SetupDiDestroyDeviceInfoList(di);
			return false;
		}

	} else {
		return false;
	}
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
