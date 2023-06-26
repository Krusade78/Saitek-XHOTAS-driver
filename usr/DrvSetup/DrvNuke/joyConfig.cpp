#include "stdafx.h"
#include <setupapi.h>
#include <winioctl.h>
#include "gameport.h"
#include "joyConfig.h"

//void Error(char* tit)
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
//void Error(char* tit, HRESULT res)
//{
//	LPCTSTR msg;
//	FormatMessage( 
//	    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
//		NULL,
//		res,
//		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
//		(LPTSTR) &msg,
//		0, NULL );
//	MessageBox(NULL,msg,tit,MB_ICONWARNING);
//}


CjoyConfig::CjoyConfig()
{
}

void/*bool*/ CjoyConfig::Desinstalar()
{
	// Quitar joystick

	char* interfaz = NULL;
	PVOID puerto;
	if(!LeerRegistro(&interfaz,&puerto)) return;// true;

	HANDLE driver = CreateFile(interfaz,GENERIC_WRITE,FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL);
	if(driver==INVALID_HANDLE_VALUE) {
		//Error("CjoyConfig::Desinstalar[0]");
		delete[] interfaz; interfaz=NULL;
		return;// true;
	}
	delete[] interfaz; interfaz=NULL;

	GAMEENUM_REMOVE_HARDWARE grh;
	DWORD tam=0;
	grh.Size=sizeof(GAMEENUM_REMOVE_HARDWARE);
	grh.HardwareHandle=puerto;
	if(!DeviceIoControl(driver,IOCTL_GAMEENUM_REMOVE_HARDWARE,&grh,sizeof(GAMEENUM_REMOVE_HARDWARE),&grh,sizeof(GAMEENUM_REMOVE_HARDWARE),&tam,NULL)) {
		//Error("CjoyConfig::Desinstalar[1]");
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

bool CjoyConfig::BorrarRegistro()
{
	HKEY hk;
	LONG res = RegOpenKeyEx(HKEY_LOCAL_MACHINE,"SYSTEM\\CurrentControlSet\\Control\\MediaProperties\\PrivateProperties\\Joystick\\OEM\\VID_06A3&PID_3635",0,KEY_WRITE,&hk);
	if(res!=ERROR_SUCCESS && res!=2) {
		//Error("CjoyConfig::BorrarRegistro[0]",res);
		return false;
	}
	if(res==2) return true;

	res = RegDeleteValue(hk,"Gameenum");
	if(res!=ERROR_SUCCESS && res!=2) {
		//Error("CjoyConfig::BorrarRegistro[1]",res);
		RegCloseKey(hk);
		return false;
	}
	res = RegDeleteValue(hk,"Port");
	if(res!=ERROR_SUCCESS && res!=2) {
		//Error("CjoyConfig::BorrarRegistro[1]",res);
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
		//Error("CjoyConfig::LeerRegistro[1]");
		return false;
	}

	tam=1;
	RegQueryValueEx(hk,"Gameenum",0,NULL,NULL,&tam);
	if(tam==1) {
		//Error("CjoyConfig::LeerRegistro[2]");
		RegCloseKey(hk);
		return false;
	}
	gameenum=new BYTE[tam];
	res=RegQueryValueEx(hk,"Gameenum",0,NULL,gameenum,&tam);
	if(res!=ERROR_SUCCESS) {
		//Error("CjoyConfig::LeerRegistro[3]");
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
		//Error("CjoyConfig::LeerRegistro[4]");
		RegCloseKey(hk);
		return false;
	}
	*interfaz=new char[tam];
	res=RegQueryValueEx(hk,"Port",0,NULL,(BYTE*)*interfaz,&tam);
	if(res!=ERROR_SUCCESS) {
		//Error("CjoyConfig::LeerRegistro[5]");
		RegCloseKey(hk);
		delete[] *interfaz; *interfaz=NULL;
		return false;
	}

	RegCloseKey(hk);

	return true;
}