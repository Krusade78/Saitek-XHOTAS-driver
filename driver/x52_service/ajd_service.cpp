/*--
Copyright (c) 2005-2007 Alfredo Costalago
--*/

#include <windows.h>
#include "servicio.h"

SERVICE_STATUS          svEstado; 
SERVICE_STATUS_HANDLE   svHandle; 
CServicio* serv=NULL;
HWND hWnd = NULL;

void  InicioServicio (DWORD argc, LPTSTR *argv);
DWORD WINAPI ServicioCtrlHandler(
  DWORD dwControl,     // requested control code
  DWORD dwEventType,   // event type
  LPVOID lpEventData,  // event data
  LPVOID lpContext     // user-defined context data
); 
 
void _cdecl main()
{ 
    SERVICE_TABLE_ENTRY DispatchTable[2]=
		{{"HidUserService",(LPSERVICE_MAIN_FUNCTION)InicioServicio},
		{NULL, NULL}};

	if (!StartServiceCtrlDispatcher( DispatchTable)) 
    { 
		MessageBox(NULL,"StartServiceCtrlDispatcher error","[X52-Service]",MB_SERVICE_NOTIFICATION|MB_ICONSTOP);
    } 
} 



void InicioServicio (DWORD argc, LPTSTR *argv)
{
	svEstado.dwServiceType        = SERVICE_WIN32_OWN_PROCESS|
									SERVICE_INTERACTIVE_PROCESS; 
    svEstado.dwCurrentState       = SERVICE_START_PENDING; 
    svEstado.dwControlsAccepted   = SERVICE_ACCEPT_STOP |
									SERVICE_ACCEPT_POWEREVENT; 
    svEstado.dwWin32ExitCode      = NO_ERROR; 
    svEstado.dwServiceSpecificExitCode = 0; 
    svEstado.dwCheckPoint         = 0; 
    svEstado.dwWaitHint           = 0;

	svHandle = RegisterServiceCtrlHandlerEx( 
        "X52-Service", ServicioCtrlHandler,NULL); 
 
    if (svHandle == (SERVICE_STATUS_HANDLE)0) 
    { 
		MessageBox(NULL,"RegisterServiceCtrlHandler error","[X52-Service]",MB_SERVICE_NOTIFICATION|MB_ICONSTOP);
        return; 
    }

	serv = new CServicio();
	if(serv==NULL) {
		svEstado.dwCurrentState       = SERVICE_STOPPED; 
        svEstado.dwCheckPoint         = 0; 
        svEstado.dwWaitHint           = 0; 
        svEstado.dwWin32ExitCode      = 1062; 
        svEstado.dwServiceSpecificExitCode = 0; 
 
        SetServiceStatus (svHandle, &svEstado); 

        return; 
	}

	serv->IniciarServicio();
/*    if (!serv->IniciarServicio()) 
    { 
		delete serv; serv=NULL;

		svEstado.dwCurrentState       = SERVICE_STOPPED; 
        svEstado.dwCheckPoint         = 0; 
        svEstado.dwWaitHint           = 0; 
        svEstado.dwWin32ExitCode      = 1062; 
        svEstado.dwServiceSpecificExitCode = 0; 
 
        SetServiceStatus (svHandle, &svEstado); 

        return; 
    }*/
    svEstado.dwCurrentState       = SERVICE_RUNNING; 
    svEstado.dwCheckPoint         = 0; 
    svEstado.dwWaitHint           = 0; 
 
    if (!SetServiceStatus (svHandle, &svEstado)) 
    { 
		MessageBox(NULL,"SetServiceStatus error","[X52-Service]",MB_SERVICE_NOTIFICATION|MB_ICONSTOP);
		delete serv; serv=NULL;
		return;
    }

	MSG msg;
	hWnd=CreateWindow("STATIC",NULL,0,0,0,0,0,NULL,NULL,NULL,NULL);
	UINT call=RegisterWindowMessage("x52servicemsg");
	bool salir=false;
	SetTimer(hWnd,1,2000,0);
	while(GetMessage(&msg,NULL,0,0)!=0) {
		switch(msg.message) {
			case WM_QUIT:
				salir=true;
				break;
			case WM_DESTROY:
				salir=true;
				break;
			case WM_TIMECHANGE:
				KillTimer(hWnd,1);
				SetTimer(hWnd,1,2000,0);
				break;
			case WM_TIMER:
				serv->Tick();
				break;
			default:
				if(msg.message==call) {
					switch(msg.wParam) {
						case 0:
							salir=true;
							break;
						case 1:
							serv->horaActiva=msg.lParam?true:false;
							break;
						case 2:
							serv->fechaActiva=msg.lParam?true:false;
							break;
						case 3:
							serv->CargarConfiguracion();
					}
				}
		}
		if(salir) break;
	}
	DestroyWindow(hWnd);
}


DWORD WINAPI ServicioCtrlHandler(
  DWORD dwControl,     // requested control code
  DWORD dwEventType,   // event type
  LPVOID lpEventData,  // event data
  LPVOID lpContext     // user-defined context data
)
{
	bool salir=false;
    switch(dwControl) 
    { 
        case SERVICE_CONTROL_STOP:
			BroadcastSystemMessage(BSF_POSTMESSAGE,0,RegisterWindowMessage("x52servicemsg"),0,0);
			salir=true;
            svEstado.dwWin32ExitCode = 0; 
            svEstado.dwCurrentState  = SERVICE_STOPPED; 
            svEstado.dwCheckPoint    = 0; 
            svEstado.dwWaitHint      = 0;
			break;

         case SERVICE_CONTROL_INTERROGATE: 
            break;
		 
		 case SERVICE_CONTROL_POWEREVENT:
			if(dwEventType==PBT_APMRESUMESUSPEND) 
			{
				serv->ClearX52();
			}
			break;
		default:
			return ERROR_CALL_NOT_IMPLEMENTED;
    } 
 
    // Send current status. 
    if (!SetServiceStatus (svHandle,  &svEstado)) 
    {
		MessageBox(NULL,"Handler SetServiceStatus error","[X52-Service]",MB_SERVICE_NOTIFICATION|MB_ICONWARNING);
		return 6;
	} else {
		if(salir && serv!=NULL) {
			delete serv; serv=NULL;
		}
	}

    return NO_ERROR; 
} 