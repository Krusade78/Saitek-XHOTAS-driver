/*--
Copyright (c) 2005-2007 Alfredo Costalago
--*/

#include <windows.h>
#include <aclapi.h>
#include "servicio.h"

SERVICE_STATUS          svEstado; 
SERVICE_STATUS_HANDLE   svHandle; 
CServicio* serv=NULL;
HANDLE hSalir = CreateEvent(NULL, FALSE, FALSE, NULL);

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

	StartServiceCtrlDispatcher( DispatchTable);
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
		CloseHandle(hSalir);
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

		CloseHandle(hSalir);
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
		delete serv; serv=NULL;
		CloseHandle(hSalir);
		return;
    }


	bool salir=false;
	PSID pEveryoneSID = NULL, pSIDAdmin = NULL;
	PACL pACL = NULL;
	PSECURITY_DESCRIPTOR pSD = NULL;
	SECURITY_ATTRIBUTES sa;
		sa.nLength = sizeof(sa);
		sa.bInheritHandle = FALSE;
		sa.lpSecurityDescriptor = NULL; 

	SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
	if(AllocateAndInitializeSid(&SIDAuthWorld, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &pEveryoneSID))
	{
		SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;
		if(AllocateAndInitializeSid(&SIDAuthNT, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &pSIDAdmin))
		{
			EXPLICIT_ACCESS ea[2] = {0};

			//First Everyone
			ea[0].grfAccessPermissions = GENERIC_ALL;
			ea[0].grfAccessMode = SET_ACCESS;
			ea[0].grfInheritance= NO_INHERITANCE;
			ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
			ea[0].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
			ea[0].Trustee.ptstrName  = (LPTSTR)pEveryoneSID;

			//Then admin
			ea[1].grfAccessPermissions = GENERIC_ALL;
			ea[1].grfAccessMode = SET_ACCESS;
			ea[1].grfInheritance= NO_INHERITANCE;
			ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
			ea[1].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
			ea[1].Trustee.ptstrName  = (LPTSTR)pSIDAdmin; 

			SetEntriesInAcl(2, ea, NULL, &pACL);

			pSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
			if(pSD)
			{
				InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION);

				//Add the ACL to the security descriptor
				SetSecurityDescriptorDacl(pSD, TRUE, pACL, FALSE);
				sa.lpSecurityDescriptor = pSD; 
			}
		}
	}

	HANDLE hTimer = CreateEvent(&sa, FALSE, FALSE, "Global\\eXHOTASTimer");
	HANDLE hHora = CreateEvent(&sa, TRUE, FALSE, "Global\\eXHOTASHora");
	HANDLE hFecha = CreateEvent(&sa, TRUE, FALSE, "Global\\eXHOTASFecha");
	HANDLE hConfig = CreateEvent(&sa, FALSE, FALSE, "Global\\eXHOTASCargar");

    if (pEveryoneSID) FreeSid(pEveryoneSID);
    if (pSIDAdmin) FreeSid(pSIDAdmin);
    if (pACL) LocalFree(pACL);
    if (pSD) LocalFree(pSD);

	while(!salir)
	{
		if( WaitForSingleObject(hTimer,2000) == WAIT_TIMEOUT )
		{
			serv->Tick();
		}
		else
		{
			if( WaitForSingleObject(hSalir, 1) == WAIT_OBJECT_0 ) salir = true;
			if( WaitForSingleObject(hHora, 1) == WAIT_OBJECT_0 ) serv->horaActiva =	true; else serv->horaActiva = false;
			if( WaitForSingleObject(hFecha, 1) == WAIT_OBJECT_0 ) serv->fechaActiva = true; else serv->fechaActiva = false;
			if( WaitForSingleObject(hConfig, 1) == WAIT_OBJECT_0 ) serv->CargarConfiguracion();
		}
	}

	CloseHandle(hSalir);
	CloseHandle(hTimer);
	CloseHandle(hHora);
	CloseHandle(hFecha);
	CloseHandle(hConfig);
}


DWORD WINAPI ServicioCtrlHandler(
  DWORD dwControl,     // requested control code
  DWORD dwEventType,   // event type
  LPVOID lpEventData,  // event data
  LPVOID lpContext     // user-defined context data
)
{
	bool salir = false;
	HANDLE h = NULL;
    switch(dwControl) 
    { 
        case SERVICE_CONTROL_STOP:
			SetEvent(hSalir);
			h = OpenEvent(EVENT_MODIFY_STATE, FALSE, "Global\\eXHOTASTimer");
			if(h!=NULL) { SetEvent(h); CloseHandle(h); }
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
		return 6;
	} else {
		if(salir && serv!=NULL) {
			delete serv; serv=NULL;
		}
	}

    return NO_ERROR; 
} 