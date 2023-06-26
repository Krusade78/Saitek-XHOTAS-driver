#include <windows.h>
#include <winioctl.h>
#include "joyConfig.h"
#include "gameport.h"

void CjoyConfig::Error(char* tit)
{
	LPCTSTR msg;
	FormatMessage( 
	    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &msg,
		0, NULL );
	MessageBox(NULL,msg,tit,MB_ICONWARNING);
}
void CjoyConfig::Error(char* tit, HRESULT res)
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

bool CjoyConfig::LeerRegistro(char** interfaz, BYTE** gameenum,UINT16 gametam)
{
	DWORD tam=gametam;
	HKEY hk;
	LONG res = RegOpenKeyEx(HKEY_LOCAL_MACHINE,"SYSTEM\\CurrentControlSet\\Control\\MediaProperties\\PrivateProperties\\Joystick\\OEM\\VID_06A3&PID_3635",0,KEY_READ,&hk);
	if(res!=ERROR_SUCCESS) {
		Error("CjoyConfig::LeerRegistro[1]");
		return false;
	}

	res=RegQueryValueEx(hk,"Gameenum",0,NULL,*gameenum,&tam);
	if(res!=ERROR_SUCCESS) {
		Error("CjoyConfig::LeerRegistro[2]");
		RegCloseKey(hk);
		return false;
	}

	tam=1;
	res=RegQueryValueEx(hk,"Port",0,NULL,NULL,&tam);
	if(tam==1) {
		Error("CjoyConfig::LeerRegistro[3]");
		RegCloseKey(hk);
		return false;
	}
	*interfaz=new char[tam];
	res=RegQueryValueEx(hk,"Port",0,NULL,(BYTE*)*interfaz,&tam);
	if(res!=ERROR_SUCCESS) {
		Error("CjoyConfig::LeerRegistro[4]");
		RegCloseKey(hk);
		delete[] *interfaz; *interfaz=NULL;
		return false;
	}

	RegCloseKey(hk);

	return true;
}

CjoyConfig::CjoyConfig()
{
}

CjoyConfig::~CjoyConfig()
{
}

bool CjoyConfig::Comprobar()
{
	const WCHAR hid3600[]=L"GamePort\\VID_06A3&PID_3600\0";
	const size_t hidlen=sizeof(hid3600);
	char* interfaz = NULL;
	BYTE* gehbuff=new BYTE[sizeof(GAMEENUM_EXPOSE_HARDWARE)+hidlen];
	if(!LeerRegistro(&interfaz,&gehbuff,sizeof(GAMEENUM_EXPOSE_HARDWARE)+hidlen)) {
		delete[] gehbuff; gehbuff=NULL;
		return false;
	}

	HANDLE driver = CreateFile(interfaz,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
	if(driver==INVALID_HANDLE_VALUE) {
		Error("CjoyConfig::Comprobar[1]");
		delete[] interfaz; interfaz=NULL;
		delete[] gehbuff; gehbuff=NULL;
		return false;
	}
	delete[] interfaz; interfaz=NULL;

	GAMEENUM_PORT_DESC gpd;
	DWORD ret=0;
	gpd.Size=sizeof(GAMEENUM_PORT_DESC);
	if(!DeviceIoControl(driver,IOCTL_GAMEENUM_PORT_DESC,&gpd,sizeof(GAMEENUM_PORT_DESC),&gpd,sizeof(GAMEENUM_PORT_DESC),&ret,NULL)) {
		Error("CjoyConfig::Comprobar[2]");
		delete[] gehbuff; gehbuff=NULL;
		CloseHandle(driver);
		return false;
	}

	((GAMEENUM_EXPOSE_HARDWARE*)gehbuff)->PortHandle=gpd.PortHandle;
	((GAMEENUM_EXPOSE_HARDWARE*)gehbuff)->HardwareHandle=0;
	if(DeviceIoControl(driver,IOCTL_GAMEENUM_EXPOSE_HARDWARE,gehbuff,sizeof(GAMEENUM_EXPOSE_HARDWARE)+hidlen,gehbuff,sizeof(GAMEENUM_EXPOSE_HARDWARE)+hidlen,&ret,NULL)){
		GuardarRegistro(gehbuff,sizeof(GAMEENUM_EXPOSE_HARDWARE)+hidlen);
	}

	CloseHandle(driver);
	delete[] gehbuff; gehbuff=NULL;
	return true;
}

bool CjoyConfig::GuardarRegistro(BYTE* datos,DWORD tam)
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
	RegCloseKey(hk);

	return true;
}
