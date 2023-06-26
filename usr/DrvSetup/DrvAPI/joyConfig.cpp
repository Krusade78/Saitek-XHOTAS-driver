#include "stdafx.h"
#include <setupapi.h>
#include <winioctl.h>
#include "gameport.h"
#include "joyConfig.h"

void Error(char* tit)
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
void Error(char* tit, HRESULT res)
{
	LPCTSTR msg;
	FormatMessage( 
	    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		res,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &msg,
		0, NULL );
	MessageBox(NULL,msg,tit,MB_ICONWARNING);
}


CjoyConfig::CjoyConfig()
{
}

bool CjoyConfig::Instalar(DWORD s_int, bool x35)
{
	CONST GUID enumclass={0xcae56030,0x684a,0x11d0,{0xd6,0xf6,0x00,0xa0,0xc9,0x0f,0x57,0xda}};
	HDEVINFO di;
	SP_DEVINFO_DATA dev;
		dev.cbSize=sizeof(SP_DEVINFO_DATA);
	DWORD i=0;
	SP_DEVICE_INTERFACE_DATA devidata;
		devidata.cbSize=sizeof(SP_DEVICE_INTERFACE_DATA);
	SP_DEVICE_INTERFACE_DETAIL_DATA* deviddata = NULL;
	BYTE* diddbuff = NULL;

	di=SetupDiGetClassDevs(&enumclass,NULL,NULL,DIGCF_PRESENT|DIGCF_DEVICEINTERFACE );
	if(di==INVALID_HANDLE_VALUE)
	{
		Error("CjoyConfig::Instalar[0]");
		return false;
	}

	if(!SetupDiEnumDeviceInfo(di,s_int,&dev)) {
		Error("CjoyConfig::Instalar[1]");
		SetupDiDestroyDeviceInfoList(di);
		return false;
	}

	while(SetupDiEnumDeviceInterfaces(di,&dev,&enumclass,i,&devidata)) {
		i++;
		if((devidata.Flags&SPINT_ACTIVE)==SPINT_ACTIVE) {
			DWORD ret=0;
			SetupDiGetDeviceInterfaceDetail(di,&devidata,NULL,0,&ret,NULL);
			diddbuff = new BYTE[ret];
			deviddata =(SP_DEVICE_INTERFACE_DETAIL_DATA*)diddbuff;
			deviddata->cbSize=sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
			if(!SetupDiGetDeviceInterfaceDetail(di,&devidata,deviddata,ret,&ret,NULL))
			{
				Error("CjoyConfig::Instalar[2]");
				SetupDiDestroyDeviceInfoList(di);
				goto mal;
			 }
			break;
		}
	}
	SetupDiDestroyDeviceInfoList(di);
	if(deviddata==NULL) {
		Error("CjoyConfig::Instalar[3]");
		goto mal;
	}

	HANDLE driver = CreateFile(deviddata->DevicePath,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
	if(driver==INVALID_HANDLE_VALUE) {
		Error("CjoyConfig::Instalar[4]");
		goto mal;
	}


	GAMEENUM_PORT_DESC gpd;
	gpd.Size=sizeof(GAMEENUM_PORT_DESC);
	if(!DeviceIoControl(driver,IOCTL_GAMEENUM_PORT_DESC,&gpd,sizeof(GAMEENUM_PORT_DESC),&gpd,sizeof(GAMEENUM_PORT_DESC),&i,NULL)) {
		Error("CjoyConfig::Instalar[5]");
		CloseHandle(driver);
		goto mal;
	}

	const WCHAR hid3600[]=L"GamePort\\VID_06A3&PID_3600\0";
	const WCHAR hid3635[]=L"GamePort\\VID_06A3&PID_3635\0";
	const size_t hidlen=sizeof(hid3600);
	BYTE* gehbuff=new BYTE[sizeof(GAMEENUM_EXPOSE_HARDWARE)+hidlen];
	GAMEENUM_EXPOSE_HARDWARE* geh=(GAMEENUM_EXPOSE_HARDWARE*)gehbuff;
	ZeroMemory(geh,sizeof(GAMEENUM_EXPOSE_HARDWARE));
	geh->Size=sizeof(GAMEENUM_EXPOSE_HARDWARE);
	geh->PortHandle=gpd.PortHandle;
	geh->NumberJoysticks=1;
	geh->Flags=GAMEENUM_FLAG_NOCOMPATID|GAMEENUM_FLAG_COMPATIDCTRL;
	if(x35) {
		BYTE oem[8] = {0xa3,0x06,0x35,0x36,0x17,0x00,0x08,0x08};
		memcpy(&geh->OemData,oem,8);
		geh->NumberAxis=4;
		geh->NumberButtons=24;
		memcpy(geh->HardwareIDs,hid3635,hidlen);
	} else {
		BYTE oem[8] = {0xa3,0x06,0x00,0x36,0x16,0x00,0x00,0x08};
		memcpy(&geh->OemData,oem,8);
		geh->NumberAxis=2;
		geh->NumberButtons=14;
		memcpy(geh->HardwareIDs,hid3600,hidlen);
	}

	if(!DeviceIoControl(driver,IOCTL_GAMEENUM_EXPOSE_HARDWARE,geh,sizeof(GAMEENUM_EXPOSE_HARDWARE)+hidlen,geh,sizeof(GAMEENUM_EXPOSE_HARDWARE)+hidlen,&i,NULL)) {
		Error("CjoyConfig::Instalar[6]");
		CloseHandle(driver);
		goto mal1;
	}

	if(!GuardarRegistro((BYTE*)deviddata->DevicePath,gehbuff,sizeof(GAMEENUM_EXPOSE_HARDWARE)+hidlen)) {
		GAMEENUM_REMOVE_HARDWARE grh;
		grh.Size=sizeof(GAMEENUM_REMOVE_HARDWARE);
		grh.HardwareHandle=geh->HardwareHandle;
		DeviceIoControl(driver,IOCTL_GAMEENUM_REMOVE_HARDWARE,&grh,sizeof(GAMEENUM_REMOVE_HARDWARE),&grh,sizeof(GAMEENUM_REMOVE_HARDWARE),&i,NULL);
		CloseHandle(driver);
		BorrarRegistro();
		goto mal1;
	}

	CloseHandle(driver);

	delete[] gehbuff; gehbuff=NULL;
	delete[] diddbuff; diddbuff=NULL;
	return true;


mal1:
	delete[] gehbuff; gehbuff=NULL;
	delete[] diddbuff; diddbuff=NULL;
	return false;
mal:
	delete[] diddbuff; diddbuff=NULL;
	return false;
}

void/*bool*/ CjoyConfig::Desinstalar()
{
	// Quitar joystick

	char* interfaz = NULL;
	PVOID puerto;
	if(!LeerRegistro(&interfaz,&puerto)) return;// true;

	HANDLE driver = CreateFile(interfaz,GENERIC_WRITE,FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL);
	if(driver==INVALID_HANDLE_VALUE) {
		Error("CjoyConfig::Desinstalar[0]");
		delete[] interfaz; interfaz=NULL;
		return;// true;
	}
	delete[] interfaz; interfaz=NULL;

	GAMEENUM_REMOVE_HARDWARE grh;
	DWORD tam=0;
	grh.Size=sizeof(GAMEENUM_REMOVE_HARDWARE);
	grh.HardwareHandle=puerto;
	if(!DeviceIoControl(driver,IOCTL_GAMEENUM_REMOVE_HARDWARE,&grh,sizeof(GAMEENUM_REMOVE_HARDWARE),&grh,sizeof(GAMEENUM_REMOVE_HARDWARE),&tam,NULL)) {
		Error("CjoyConfig::Desinstalar[1]");
		CloseHandle(driver);
		return;// true;
	}
	CloseHandle(driver);
	
	BorrarRegistro();

	return;// true;
}

//===============================================================
//					FUNCIONES del registro
//===============================================================

bool CjoyConfig::GuardarRegistro(BYTE* ruta,BYTE* datos,DWORD tam)
{
	HKEY hk;
	LONG res = RegOpenKeyEx(HKEY_LOCAL_MACHINE,"SYSTEM\\CurrentControlSet\\Control\\MediaProperties\\PrivateProperties\\Joystick\\OEM\\VID_06A3&PID_3635",0,KEY_WRITE,&hk);
	if(res!=ERROR_SUCCESS) {
		Error("CjoyConfig::GuardarRegistro[0]",res);
		return false;
	}
	res = RegSetValueEx(hk,"Gameenum",0,REG_BINARY,(BYTE*)datos,tam);
	if(res!=ERROR_SUCCESS) {
		Error("CjoyConfig::GuardarRegistro[1]",res);
		RegCloseKey(hk);
		return false;
	}
	res = RegSetValueEx(hk,"Port",0,REG_SZ,(BYTE*)ruta,strlen((char*)ruta));
	if(res!=ERROR_SUCCESS) {
		Error("CjoyConfig::GuardarRegistro[2]",res);
		RegCloseKey(hk);
		return false;
	}
	RegCloseKey(hk);

	return true;
}

bool CjoyConfig::BorrarRegistro()
{
	HKEY hk;
	LONG res = RegOpenKeyEx(HKEY_LOCAL_MACHINE,"SYSTEM\\CurrentControlSet\\Control\\MediaProperties\\PrivateProperties\\Joystick\\OEM\\VID_06A3&PID_3635",0,KEY_WRITE,&hk);
	if(res!=ERROR_SUCCESS && res!=2) {
		Error("CjoyConfig::BorrarRegistro[0]",res);
		return false;
	}
	if(res==2) return true;

	res = RegDeleteValue(hk,"Gameenum");
	if(res!=ERROR_SUCCESS && res!=2) {
		Error("CjoyConfig::BorrarRegistro[1]",res);
		RegCloseKey(hk);
		return false;
	}
	res = RegDeleteValue(hk,"Port");
	if(res!=ERROR_SUCCESS && res!=2) {
		Error("CjoyConfig::BorrarRegistro[1]",res);
		RegCloseKey(hk);
		return false;
	}
	RegCloseKey(hk);

	return true;
}

bool CjoyConfig::LeerRegistro(char** interfaz,PVOID* puerto)
{
	BYTE* gameenum=NULL;
	DWORD tam=0;
	HKEY hk;
	LONG res = RegOpenKeyEx(HKEY_LOCAL_MACHINE,"SYSTEM\\CurrentControlSet\\Control\\MediaProperties\\PrivateProperties\\Joystick\\OEM\\VID_06A3&PID_3635",0,KEY_READ,&hk);
	if(res!=ERROR_SUCCESS) {
		Error("CjoyConfig::LeerRegistro[1]");
		return false;
	}

	tam=1;
	RegQueryValueEx(hk,"Gameenum",0,NULL,NULL,&tam);
	if(tam==1) {
		Error("CjoyConfig::LeerRegistro[2]");
		RegCloseKey(hk);
		return false;
	}
	gameenum=new BYTE[tam];
	res=RegQueryValueEx(hk,"Gameenum",0,NULL,gameenum,&tam);
	if(res!=ERROR_SUCCESS) {
		Error("CjoyConfig::LeerRegistro[3]");
		RegCloseKey(hk);
		delete[] gameenum; gameenum=NULL;
		return false;
	} else {
		GAMEENUM_EXPOSE_HARDWARE* geh=(GAMEENUM_EXPOSE_HARDWARE*) gameenum;
		*puerto=geh->HardwareHandle;
		delete[] gameenum; gameenum=NULL;
	}

	tam=1;
	RegQueryValueEx(hk,"Port",0,NULL,NULL,&tam);
	if(tam==1) {
		Error("CjoyConfig::LeerRegistro[4]");
		RegCloseKey(hk);
		return false;
	}
	*interfaz=new char[tam];
	res=RegQueryValueEx(hk,"Port",0,NULL,(BYTE*)*interfaz,&tam);
	if(res!=ERROR_SUCCESS) {
		Error("CjoyConfig::LeerRegistro[5]");
		RegCloseKey(hk);
		delete[] *interfaz; *interfaz=NULL;
		return false;
	}

	RegCloseKey(hk);

	return true;
}