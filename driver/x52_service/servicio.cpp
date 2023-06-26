/*--
Copyright (c) 2005-2007 Alfredo Costalago

Module Name:

    servicio.c
--*/

#include <windows.h>
#include <winioctl.h>
#include "servicio.h"

#define IOCTL_MFD_LUZ		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0100, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_GLOBAL_LUZ	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0101, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_INFO_LUZ		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0102, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_TEXTO			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0104, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_HORA			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0105, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_HORA24		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0106, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_FECHA			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0107, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_USR_CALIBRADO		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0100, METHOD_BUFFERED, FILE_WRITE_ACCESS)

CServicio::CServicio(){}

/****************************************************
			FUNCIONES DE INICIO
*****************************************************/

void CServicio::IniciarServicio()
{
	this->horaActiva=true;
	this->fechaActiva=true;
	CargarCalibrado();
	CargarConfiguracion();
	SetTextoInicio();
}

void CServicio::CargarConfiguracion()
{
	HKEY key;
	DWORD tipo,tam;
	LONG res;

	if(ERROR_SUCCESS!=RegOpenKeyEx(HKEY_LOCAL_MACHINE,"SYSTEM\\CurrentControlSet\\Control\\MediaProperties\\PrivateProperties\\Joystick\\Calibrado",0,KEY_READ,&key))
		return ;

	tam=1; res=RegQueryValueEx(key,"LuzMFD",0,&tipo,(BYTE*)&this->luzMFD,&tam);
	if(ERROR_SUCCESS!=res || tam!=1) this->luzMFD=1;

	tam=1; res=RegQueryValueEx(key,"Luz",0,&tipo,(BYTE*)&this->luces,&tam);
	if(ERROR_SUCCESS!=res || tam!=1) this->luces=1;

	BYTE bf[3];
	tam=3; res=RegQueryValueEx(key,"Hora1",0,&tipo,bf,&tam);
	if(ERROR_SUCCESS!=res || tam!=3) {
		this->hora24[0]=false;
		this->hora[0]=0;
	} else {
		this->hora24[0]=bf[0]?true:false;
		this->hora[0]=*((WORD*)&bf[1]);
	}
	tam=3; res=RegQueryValueEx(key,"Hora2",0,&tipo,bf,&tam);
	if(ERROR_SUCCESS!=res || tam!=3) {
		this->hora24[1]=false;
		this->hora[1]=0;
	} else {
		this->hora24[1]=bf[0]?true:false;
		this->hora[1]=*((WORD*)&bf[1]);
	}
	tam=3; res=RegQueryValueEx(key,"Hora3",0,&tipo,bf,&tam);
	if(ERROR_SUCCESS!=res || tam!=3) {
		this->hora24[2]=false;
		this->hora[2]=0;
	} else {
		this->hora24[2]=bf[0]?true:false;
		this->hora[2]=*((WORD*)&bf[1]);
	}

	RegCloseKey(key);

	DWORD ret;
	HANDLE driver=CreateFile(
			"\\\\.\\XUSBInterface",
			GENERIC_WRITE,
			FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			0,
			NULL);
	if(driver==INVALID_HANDLE_VALUE) {
		//MessageBox(NULL,"Error opening device","[X52-Service][1.1]",MB_SERVICE_NOTIFICATION|MB_ICONWARNING);
		return;
	}
	if(!DeviceIoControl(driver,IOCTL_MFD_LUZ,&this->luzMFD,1,NULL,0,&ret,NULL))
		MessageBox(NULL,"Error accesing device","[X52-Service][1.2]",MB_SERVICE_NOTIFICATION|MB_ICONWARNING);
	if(!DeviceIoControl(driver,IOCTL_GLOBAL_LUZ,&this->luces,1,NULL,0,&ret,NULL))
		MessageBox(NULL,"Error accesing device","[X52-Service][1.3]",MB_SERVICE_NOTIFICATION|MB_ICONWARNING);
	SYSTEMTIME t;
	GetLocalTime(&t);

	bf[0]=1; bf[1]=(BYTE)t.wDay;
	if(!DeviceIoControl(driver,IOCTL_FECHA,bf,2,NULL,0,&ret,NULL))
		MessageBox(NULL,"Error accesing device","[X52-Service][1.4]",MB_SERVICE_NOTIFICATION|MB_ICONWARNING);
	bf[0]=2; bf[1]=(BYTE)t.wMonth;
	if(!DeviceIoControl(driver,IOCTL_FECHA,bf,2,NULL,0,&ret,NULL))
		MessageBox(NULL,"Error accesing device","[X52-Service][1.5]",MB_SERVICE_NOTIFICATION|MB_ICONWARNING);
	bf[0]=3; bf[1]=t.wYear % 100;
	if(!DeviceIoControl(driver,IOCTL_FECHA,bf,2,NULL,0,&ret,NULL))
		MessageBox(NULL,"Error accesing device","[X52-Service][1.6]",MB_SERVICE_NOTIFICATION|MB_ICONWARNING);

	FILETIME ft;
	ULARGE_INTEGER uft;
	SystemTimeToFileTime(&t,&ft);
	uft.LowPart=ft.dwLowDateTime; uft.HighPart=ft.dwHighDateTime;
	uft.QuadPart+=(__int64)((__int16)this->hora[0])*600000000;
	ft.dwLowDateTime=uft.LowPart; ft.dwHighDateTime=uft.HighPart;
	FileTimeToSystemTime(&ft,&t);

	WORD auxHora=this->hora[0];
	this->hora[0]=(t.wMinute<<8)+t.wHour;
	for(char i=0;i<3;i++) {
		bf[0]=i+1;
		*((WORD*)&bf[1])=this->hora[i];
		if(this->hora24[i]) {
			if(!DeviceIoControl(driver,IOCTL_HORA24,bf,3,NULL,0,&ret,NULL))
				MessageBox(NULL,"Error accesing device","[X52-Service][1.7]",MB_SERVICE_NOTIFICATION|MB_ICONWARNING);
		} else {
			if(!DeviceIoControl(driver,IOCTL_HORA,bf,3,NULL,0,&ret,NULL))
				MessageBox(NULL,"Error accesing device","[X52-Service][1.8]",MB_SERVICE_NOTIFICATION|MB_ICONWARNING);
		}
	}
	this->hora[0]=auxHora;

	CloseHandle(driver);
}

void CServicio::SetTextoInicio() {
	DWORD ret;

	HANDLE driver=CreateFile(
			"\\\\.\\XUSBInterface",
			GENERIC_WRITE,
			FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			0,
			NULL);
	if(driver==INVALID_HANDLE_VALUE) {return;}

	char texto[18];
	const char linea1[]="   Saitek X52";
	const char linea2[]="XHOTAS ver. 6.0";
	const char linea3[]="      RC 1";
	texto[0]=1;
	RtlCopyMemory(&texto[1],linea1,sizeof(linea1));
	DeviceIoControl(driver,IOCTL_TEXTO,texto,strlen(texto),NULL,0,&ret,NULL);
	texto[0]=2;
	RtlCopyMemory(&texto[1],linea2,sizeof(linea2));
	DeviceIoControl(driver,IOCTL_TEXTO,texto,strlen(texto),NULL,0,&ret,NULL);
	texto[0]=3;
	RtlCopyMemory(&texto[1],linea3,sizeof(linea3));
	DeviceIoControl(driver,IOCTL_TEXTO,texto,strlen(texto),NULL,0,&ret,NULL);

	CloseHandle(driver);
}

void CServicio::ClearX52()
{
	CargarConfiguracion();

	DWORD ret;

	HANDLE driver=CreateFile(
			"\\\\.\\XUSBInterface",
			GENERIC_WRITE,
			FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			0,
			NULL);
	if(driver==INVALID_HANDLE_VALUE) {return;}

	char texto[]={1,0};
	texto[0]=1;
	DeviceIoControl(driver,IOCTL_TEXTO,texto,2,NULL,0,&ret,NULL);
	texto[0]=2;
	DeviceIoControl(driver,IOCTL_TEXTO,texto,2,NULL,0,&ret,NULL);
	texto[0]=3;
	DeviceIoControl(driver,IOCTL_TEXTO,texto,2,NULL,0,&ret,NULL);

	CloseHandle(driver);

}
void CServicio::Tick()
{
	DWORD ret;
	HANDLE driver=CreateFile(
			"\\\\.\\XUSBInterface",
			GENERIC_WRITE,
			FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			0,
			NULL);
	if(driver==INVALID_HANDLE_VALUE) {return;}

	BYTE bf[3];
	SYSTEMTIME t;
	GetLocalTime(&t);

	if(this->fechaActiva) {
		bf[0]=1; bf[1]=(BYTE)t.wDay;
		DeviceIoControl(driver,IOCTL_FECHA,bf,2,NULL,0,&ret,NULL);
		bf[0]=2; bf[1]=(BYTE)t.wMonth;
		DeviceIoControl(driver,IOCTL_FECHA,bf,2,NULL,0,&ret,NULL);
		bf[0]=3; bf[1]=t.wYear % 100;
		DeviceIoControl(driver,IOCTL_FECHA,bf,2,NULL,0,&ret,NULL);
	}

	if(this->horaActiva) {
		FILETIME ft;
		ULARGE_INTEGER uft;
		SystemTimeToFileTime(&t,&ft);
		uft.LowPart=ft.dwLowDateTime; uft.HighPart=ft.dwHighDateTime;
		uft.QuadPart+=(__int64)((__int16)this->hora[0])*600000000;
		ft.dwLowDateTime=uft.LowPart; ft.dwHighDateTime=uft.HighPart;
		FileTimeToSystemTime(&ft,&t);

		WORD auxHora=(t.wMinute<<8)+t.wHour;
		bf[0]=1;
		*((WORD*)&bf[1])=auxHora;
		if(this->hora24[0]) {
			DeviceIoControl(driver,IOCTL_HORA24,bf,3,NULL,0,&ret,NULL);
		} else {
			DeviceIoControl(driver,IOCTL_HORA,bf,3,NULL,0,&ret,NULL);
		}
	}

	CloseHandle(driver);
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