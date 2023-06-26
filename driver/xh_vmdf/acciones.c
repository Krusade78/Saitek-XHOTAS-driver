#include <wdm.h>
#include "hidfilter.h"

#define _ACCIONES_
#include "acciones.h"
#undef _ACCIONES_



/*************************************************************
					FUNCIONES DE ENTRADA
**************************************************************/

BOOLEAN EncolarAcciones(PITFDEVICE_EXTENSION idevExt,PCOLA eventos,UINT16 accion,UCHAR boton)
{
	UCHAR idx;
	KIRQL irql;
	
	KeAcquireSpinLock(&idevExt->slComandos,&irql);
		if(idevExt->nComandos==0 || idevExt->Comandos==NULL) {
			KeReleaseSpinLock(&idevExt->slComandos,irql);
			ColaBorrar(eventos);
			return FALSE;
		}
		for(idx=0;idx<idevExt->Comandos[accion-1].tam;idx++) {
			PVOID evt=ExAllocatePoolWithTag(NonPagedPool, 
					                        sizeof(UCHAR)*2, // Tamaño de un evento
							                (ULONG)'vepV'
					                        );
			if(evt!=NULL) {
				RtlCopyMemory(evt,&(idevExt->Comandos[accion-1].datos[idx]),sizeof(UCHAR)*2);
				if(*((PUCHAR)evt)==11 || *((PUCHAR)evt)==12) ((PUCHAR)evt)[1]=boton;
				if(!ColaPush(eventos,evt)) {
					ExFreePoolWithTag(evt,(ULONG)'vepV');
					KeReleaseSpinLock(&idevExt->slComandos,irql);
					ColaBorrar(eventos);
					return FALSE;
				}
			} else {
				KeReleaseSpinLock(&idevExt->slComandos,irql);
				ColaBorrar(eventos);
				return FALSE;
			}
		}
	KeReleaseSpinLock(&idevExt->slComandos,irql);

	return TRUE;
}

VOID Accionar
(
	PDEVICE_EXTENSION devExt,
	UINT16 accionId,
	UCHAR boton
	)
{
	PITFDEVICE_EXTENSION idevExt = (PITFDEVICE_EXTENSION)devExt->itfObj->DeviceExtension;
	
	if(accionId!=0) {
		PCOLA eventos=ColaCrear();
		if(eventos!=NULL) {
			if(EncolarAcciones(idevExt,eventos,accionId,boton)) {
				KIRQL irql;
				KeAcquireSpinLock(&devExt->slListaAcciones,&irql);
					if(!ColaPush(&devExt->ListaAcciones,eventos)) {
						ColaBorrar(eventos);
					} else {
						KeSetEvent(&devExt->evAccion,0,FALSE);
					}
				KeReleaseSpinLock(&devExt->slListaAcciones,irql);
			}
		}
	}
}


/*************************************************************
					FUNCIONES DE SALIDA
**************************************************************/
BOOLEAN EstaHold(PITFDEVICE_EXTENSION idevExt,UCHAR boton)
{
	if(boton<100) {
		return ( ((idevExt->DeltaHidData.Botones[boton/8]>>(boton%8))&1)==1);
	} else { //Seta
		boton-=100;
		return (idevExt->DeltaHidData.Setas[boton/8]==(boton%8));
	}
}

BOOLEAN ProcesarEventoRaton(PDEVICE_EXTENSION devExt,PCOLA cola, BOOLEAN* siguiente)
{
	PITFDEVICE_EXTENSION idevExt = (PITFDEVICE_EXTENSION)devExt->itfObj->DeviceExtension;
	BOOLEAN soltar;
	PNODO nodo=cola->principio,nodoA=NULL;
	KIRQL irql;
	struct {
		UCHAR tipo;
		UCHAR dato;
	} evento;

	if(ColaEstaVacia(cola)) {*siguiente=TRUE; return TRUE;}
volver:
	RtlCopyMemory(&evento,(PUCHAR)nodo->Datos,sizeof(UCHAR)*2);
	soltar=((evento.tipo>>5)==1) ? TRUE : FALSE;
	evento.tipo&=0x1f;

	if(evento.tipo==0 || evento.tipo==10 || evento.tipo>13) {
		if(nodo->link==NULL) {
			*siguiente=TRUE;
			return FALSE;
		} else {
			nodoA=nodo;
			nodo=nodo->link;
			goto volver;
		}
	}

	switch(evento.tipo) {
#ifdef PRO
		case 1:
			KeAcquireSpinLock(&devExt->slRaton,&irql);
				if(!soltar)	devExt->stRaton[0]|=1; else devExt->stRaton[0]&=254; 
			KeReleaseSpinLock(&devExt->slRaton,irql);
			break;
		case 2:
			KeAcquireSpinLock(&devExt->slRaton,&irql);
				if(!soltar)	devExt->stRaton[0]|=2; else devExt->stRaton[0]&=253; 
			KeReleaseSpinLock(&devExt->slRaton,irql);
			break;
		case 3:
			KeAcquireSpinLock(&devExt->slRaton,&irql);
				if(!soltar)	devExt->stRaton[0]|=4; else devExt->stRaton[0]&=251; 
			KeReleaseSpinLock(&devExt->slRaton,irql);
			break;
		case 4: //Eje -x
			KeAcquireSpinLock(&devExt->slRaton,&irql);
				if(!soltar)	devExt->stRaton[1]=-evento.dato; else devExt->stRaton[1]=0; 
			KeReleaseSpinLock(&devExt->slRaton,irql);
			break;
		case 5: //Eje x
			KeAcquireSpinLock(&devExt->slRaton,&irql);
				if(!soltar)	devExt->stRaton[1]=evento.dato; else devExt->stRaton[1]=0; 
			KeReleaseSpinLock(&devExt->slRaton,irql);
			break;
		case 6: //Eje -y
			KeAcquireSpinLock(&devExt->slRaton,&irql);
				if(!soltar)	devExt->stRaton[2]=-evento.dato; else devExt->stRaton[2]=0; 
			KeReleaseSpinLock(&devExt->slRaton,irql);
			break;
		case 7: //Eje y
			KeAcquireSpinLock(&devExt->slRaton,&irql);
				if(!soltar)	devExt->stRaton[2]=evento.dato; else devExt->stRaton[2]=0; 
			KeReleaseSpinLock(&devExt->slRaton,irql);
			break;
		case 8: // Wheel up
			KeAcquireSpinLock(&devExt->slRaton,&irql);
				if(!soltar)	devExt->stRaton[3]=127; else devExt->stRaton[3]=0; 
			KeReleaseSpinLock(&devExt->slRaton,irql);
			break;
		case 9: // Wheel down
			KeAcquireSpinLock(&devExt->slRaton,&irql);
				if(!soltar)	devExt->stRaton[3]=-127; else devExt->stRaton[3]=0; 
			KeReleaseSpinLock(&devExt->slRaton,irql);
			break;
#endif
		case 11: // Autorepeat hold
			if(!EstaHold(idevExt,evento.dato)) {
				if(ColaBorrarNodo(cola,nodo,nodoA)) {
					*siguiente=TRUE;
					return TRUE;
				} else {
					nodo=cola->principio;
					goto volver;
				}
			} else {
				*siguiente=TRUE;
				return FALSE;
			}
			break;
		case 12:
		case 13:
			*siguiente=TRUE;
			return FALSE;
	}

	*siguiente=FALSE;
	return ColaBorrarNodo(cola,nodo,nodoA);
}

BOOLEAN ProcesarEventoTeclado(PDEVICE_EXTENSION devExt, PCOLA cola, BOOLEAN* procesarOtraAccion)
{
	PITFDEVICE_EXTENSION idevExt = (PITFDEVICE_EXTENSION)devExt->itfObj->DeviceExtension;
	BOOLEAN soltar,dentroDeBucle=FALSE;
	PNODO nodo=cola->principio,nodoA=NULL;
	struct {
		UCHAR tipo;
		UCHAR dato;
	} evento;

	if(ColaEstaVacia(cola)) {*procesarOtraAccion=TRUE; return TRUE;}
volver:
	RtlCopyMemory(&evento,(PUCHAR)nodo->Datos,sizeof(UCHAR)*2);
	soltar=((evento.tipo>>5)==1) ? TRUE : FALSE;
	evento.tipo&=0x1f;

	if(evento.tipo>0 && evento.tipo<10) {
		if(nodo->link==NULL) {
			*procesarOtraAccion=TRUE;
			return FALSE;
		} else {
			nodoA=nodo;
			nodo=nodo->link;
			goto volver;
		}
	}

	switch(evento.tipo) {
		case 0:	//Tecla
			if(!soltar) devExt->stTeclado[evento.dato/8]|=1<<(evento.dato%8); else devExt->stTeclado[evento.dato/8]&=~(1<<(evento.dato%8));
			break;
#ifdef PRO
		case 10: // Delay
			{
				PDELAY_CONTEXT ctx = ExAllocatePoolWithTag(NonPagedPool, 
                                sizeof(DELAY_CONTEXT), 
                                (ULONG)'npHV'
                                );
				if(ctx==NULL) {
					return FALSE;
				} else {
					LONG timeout=(-10*1000*100*evento.dato);
					ctx->DeviceExtension=devExt;
					KeInitializeDpc(&ctx->DelayTimerDpc,
				            DpcRoutineDelay,
					        (PVOID)ctx
						    );
					KeInitializeTimer(&ctx->DelayTimer);
					ctx->NodoFin=cola->fin;
					ctx->NodoIni=nodo->link;
					ColaBorrarNodo(cola,nodo,nodoA);
					if(nodoA==NULL) {
						cola->principio=cola->fin=NULL;
					} else {
						nodoA->link=NULL;
						cola->fin=nodoA;
					}
			        KeSetTimer(&ctx->DelayTimer,
					       RtlConvertLongToLargeInteger(timeout),
						   &ctx->DelayTimerDpc
						);
					*procesarOtraAccion=TRUE;
					return ColaEstaVacia(cola);
				}
			}
			break;
#endif
#pragma region "repeats"
		case 11: // Autorepeat hold
			if(!EstaHold(idevExt,evento.dato)) {
				if(ColaBorrarNodo(cola,nodo,nodoA)) { // Se han acabado los eventos (cola vacia)
					*procesarOtraAccion=TRUE;
					return TRUE;
				} else {
					nodo=cola->principio;
					goto volver;
				}
			} else {
				*procesarOtraAccion=TRUE;
				return FALSE;
			}
			break;
		case 12: // Autorepeat infinito
#ifdef PRO
			if(!EstaHold(idevExt,evento.dato))
#endif
			{
				PNODO nodos=nodo->link;
				while(*((PUCHAR)nodos->Datos)!=44) // fin autorepeat infinito
				{
					ColaBorrarNodo(cola,nodos,nodo);
					nodos=nodo->link;
				}
				ColaBorrarNodo(cola,nodos,nodo);
				if(ColaBorrarNodo(cola,nodo,nodoA)) { // Se han acabado los eventos (cola vacia)
					*procesarOtraAccion=TRUE;
					return TRUE;
				} else {
					nodo=cola->principio;
					goto volver;
				}
			}
#ifdef PRO
			else {
				PNODO nodos=nodo->link;
				PNODO pos=NULL;
				UCHAR idx=0;
				while(*((PUCHAR)nodos->Datos)!=44) // fin autorepeat infinito
				{
					PNODO nodon = (PNODO)ExAllocatePoolWithTag(NonPagedPool, 
																sizeof(NODO), 
																(ULONG)'npHV'
																);
					if(nodon!=NULL) {
						PVOID datos=ExAllocatePoolWithTag(NonPagedPool, 
																sizeof(UCHAR)*2, 
																(ULONG)'npHV'
																);
						if(datos!=NULL) {
							RtlCopyMemory(datos,nodos->Datos,2);
							nodon->Datos=datos;
							if(pos==NULL) {
								nodon->link=cola->principio;
								cola->principio=nodon;
							} else {
								nodon->link=pos->link;
								pos->link=nodon;
							}
							pos=nodon;
							idx++;
						} else {
							ExFreePoolWithTag((PVOID)nodon,(ULONG)'npHV');
						}
					}

					nodos=nodos->link;
				}
				if(idx!=0) {
					PNODO nodon = (PNODO)ExAllocatePoolWithTag(NonPagedPool, 
														sizeof(NODO), 
														(ULONG)'npHV'
														);
					if(nodon!=NULL) {
						PVOID datos=ExAllocatePoolWithTag(NonPagedPool, 
																sizeof(UCHAR)*2, 
																(ULONG)'npHV'
																);
						if(datos!=NULL) {
							*((PUCHAR)datos)=17;
							*((PUCHAR)datos+1)=0;
							nodon->Datos=datos;
							nodon->link=cola->principio;
							cola->principio=nodon;
						} else {
							ExFreePoolWithTag((PVOID)nodon,(ULONG)'npHV');
						}
					}

					nodo=cola->principio;
					goto volver;
				}
			}
#endif
			break;
		case 13: // Autorepeat
#ifndef PRO
			evento.dato=0;
#endif
			if(evento.dato!=0) {
				PNODO nodos=nodo->link;
				PNODO pos=NULL;
				UCHAR idx=0;
				*((PUCHAR)nodo->Datos+1)-=1; //Reduce la cuenta (evento.dato)
				while(*((PUCHAR)nodos->Datos)!=45) // fin autorepeat infinito
				{
					PNODO nodon = (PNODO)ExAllocatePoolWithTag(NonPagedPool, 
																sizeof(NODO), 
																(ULONG)'npHV'
																);
					if(nodon!=NULL) {
						PVOID datos=ExAllocatePoolWithTag(NonPagedPool, 
																sizeof(UCHAR)*2, 
																(ULONG)'npHV'
																);
						if(datos!=NULL) {
							RtlCopyMemory(datos,nodos->Datos,2);
							nodon->Datos=datos;
							if(pos==NULL) {
								nodon->link=cola->principio;
								cola->principio=nodon;
							} else {
								nodon->link=pos->link;
								pos->link=nodon;
							}
							pos=nodon;
							idx++;
						} else {
							ExFreePoolWithTag((PVOID)nodon,(ULONG)'npHV');
						}
					}

					nodos=nodos->link;
				}
				if(idx!=0) {
					PNODO nodon = (PNODO)ExAllocatePoolWithTag(NonPagedPool, 
														sizeof(NODO), 
														(ULONG)'npHV'
														);
					if(nodon!=NULL) {
						PVOID datos=ExAllocatePoolWithTag(NonPagedPool, 
																sizeof(UCHAR)*2, 
																(ULONG)'npHV'
																);
						if(datos!=NULL) {
							*((PUCHAR)datos)=17;
							*((PUCHAR)datos+1)=0;
							nodon->Datos=datos;
							nodon->link=cola->principio;
							cola->principio=nodon;
						} else {
							ExFreePoolWithTag((PVOID)nodon,(ULONG)'npHV');
						}
					}

					nodo=cola->principio;
					goto volver;
				}
			} else {
				if(ColaBorrarNodo(cola,nodo,nodoA)) { // Se han acabado los eventos (cola vacia)
					*procesarOtraAccion=TRUE;
					return TRUE;
				} else {
					nodo=cola->principio;
					goto volver;
				}
			}
			break;
#pragma endregion
		case 14: //Cambio modo
			idevExt->stModo=evento.dato;
			break;
#ifdef PRO
		case 15: //Cambio modo Aux
			idevExt->stAux=evento.dato;
			break;
#endif
		case 16: //Cambio modo Pinkie
			idevExt->stPinkie=evento.dato;
			break;
		case 17: // estoy procesando repeat
			if(nodo->link!=NULL) {
				if(*((PUCHAR)(nodo->link->Datos))==12 || *((PUCHAR)(nodo->link->Datos))==13) {
					ColaBorrarNodo(cola,nodo,nodoA); // Borra la cabeza (17)
					nodo=cola->principio;
					dentroDeBucle=FALSE;
				} else {
					nodoA=nodo;
					nodo=nodo->link;
					dentroDeBucle=TRUE;
				}
				goto volver;
			}
			break;
		case 18: // Botón DX
			{
				HID_INPUT_DATA st;
				KeAcquireSpinLockAtDpcLevel(&devExt->slGameData);
					if(!soltar) devExt->stHidData.Botones[evento.dato/8]|=1<<(evento.dato%8); else devExt->stHidData.Botones[evento.dato/8]&=~(1<<(evento.dato%8));
					RtlCopyMemory(&st,&devExt->stHidData,sizeof(HID_INPUT_DATA));
				KeReleaseSpinLockFromDpcLevel(&devExt->slGameData);
				if(idevExt->tipoJoy!=0) EnviarEstadoX52(devExt,&st);
			}
			break;
		case 19: // Seta DX
			{
				HID_INPUT_DATA st;
				KeAcquireSpinLockAtDpcLevel(&devExt->slGameData);
					if(!soltar) devExt->stHidData.Setas[evento.dato/8]=(evento.dato%8)+1; else devExt->stHidData.Setas[evento.dato/8]=0;
					RtlCopyMemory(&st,&devExt->stHidData,sizeof(HID_INPUT_DATA));
				KeReleaseSpinLockFromDpcLevel(&devExt->slGameData);
				if(idevExt->tipoJoy!=0) EnviarEstadoX52(devExt,&st);
			}
			break;
#pragma region "X52"
#ifdef PRO
		case 20: //mfd_luz
			if(idevExt->tipoJoy!=3) break;
			{
				UCHAR* params=(UCHAR*)ExAllocatePoolWithTag(NonPagedPool, 1,'1pVX');
				if(params!=NULL) {
					*params=evento.dato;
					EnviarOrdenX52(devExt,0,params,1);
				}
			}
			break;
		case 21: // luz
			if(idevExt->tipoJoy!=3) break;
			{
				UCHAR* params=(UCHAR*) ExAllocatePoolWithTag(NonPagedPool, 1,'1pVX');
				if(params!=NULL) {
					*params=evento.dato;
					EnviarOrdenX52(devExt,1,params,1);
				}
			}
			break;
		case 22: // info luz
			if(idevExt->tipoJoy!=3) break;
			{
				UCHAR* params=(UCHAR*) ExAllocatePoolWithTag(NonPagedPool, 1,'1pVX');
				if(params!=NULL) {
					*params=evento.dato;
					EnviarOrdenX52(devExt,2,params,1);
				}
			}
			break;
		case 23: // pinkie;
			if(idevExt->tipoJoy!=3) break;
			{
				UCHAR* params=(UCHAR*) ExAllocatePoolWithTag(NonPagedPool, 1,'1pVX');
				if(params!=NULL) {
					*params=evento.dato;
					EnviarOrdenX52(devExt,3,params,1);
				}
			}
			break;
#endif
		case 24: // texto
#ifdef PRO
			if(idevExt->tipoJoy!=3)
#endif
			{
				PNODO nodos=nodo->link;
				while(*((PUCHAR)nodos->Datos)!=56) {// fin texto
					ColaBorrarNodo(cola,nodos,nodo);
					nodos=nodos->link;
				}
				ColaBorrarNodo(cola,nodos,nodo);
			}
#ifdef PRO
			else {
				UCHAR* texto=(UCHAR*) ExAllocatePoolWithTag(NonPagedPool, 17,'1pVX');
				if(texto!=NULL) {
					PNODO nodos=nodo->link;
					UCHAR idx=1;
					RtlZeroMemory(texto,17);
					texto[0]=evento.dato;
					while(*((PUCHAR)nodos->Datos)!=56) {// fin texto
						texto[idx]=*((PUCHAR)nodos->Datos+1);
						idx++;
						ColaBorrarNodo(cola,nodos,nodo);
						nodos=nodos->link;
					}
					ColaBorrarNodo(cola,nodos,nodo);
					EnviarOrdenX52(devExt,4,texto,idx);
				} else {
					PNODO nodos=nodo->link;
					while(*((PUCHAR)nodos->Datos)!=24) {// fin texto
						ColaBorrarNodo(cola,nodos,nodo);
						nodos=nodos->link;
					}
					ColaBorrarNodo(cola,nodos,nodo);
				}
			}
#endif
			break;
		case 25: // hora
#ifdef PRO
			if(idevExt->tipoJoy!=3)
#endif
			{
				ColaBorrarNodo(cola,nodo->link,nodo);
				ColaBorrarNodo(cola,nodo->link,nodo);
			}
#ifdef PRO
			else {
				UCHAR* params=(UCHAR*) ExAllocatePoolWithTag(NonPagedPool, 3,'1pVX');
				if(params!=NULL) {
					params[0]=evento.dato;
					params[1]=*((PUCHAR)nodo->link->Datos+1);
					ColaBorrarNodo(cola,nodo->link,nodo);
					params[2]=*((PUCHAR)nodo->link->Datos+1);
					ColaBorrarNodo(cola,nodo->link,nodo);
					EnviarOrdenX52(devExt,5,params,3);
				} else {
					ColaBorrarNodo(cola,nodo->link,nodo);
					ColaBorrarNodo(cola,nodo->link,nodo);
				}
			}
#endif
			break;
		case 26: // hora 24
#ifdef PRO
			if(idevExt->tipoJoy!=3)
#endif
			{
				ColaBorrarNodo(cola,nodo->link,nodo);
				ColaBorrarNodo(cola,nodo->link,nodo);
			}
#ifdef PRO
			else {
				UCHAR* params=(UCHAR*) ExAllocatePoolWithTag(NonPagedPool, 3,'1pVX');
				if(params!=NULL) {
					params[0]=evento.dato;
					params[1]=*((PUCHAR)nodo->link->Datos+1);
					ColaBorrarNodo(cola,nodo->link,nodo);
					params[2]=*((PUCHAR)nodo->link->Datos+1);
					ColaBorrarNodo(cola,nodo->link,nodo);
					EnviarOrdenX52(devExt,6,params,3);
				} else {
					ColaBorrarNodo(cola,nodo->link,nodo);
					ColaBorrarNodo(cola,nodo->link,nodo);
				}
			}
#endif
			break;
		case 27: // fecha
#ifdef PRO
			if(idevExt->tipoJoy!=3)
#endif
			{
				ColaBorrarNodo(cola,nodo->link,nodo);
			}
#ifdef PRO
			else {
				UCHAR* params=(UCHAR*) ExAllocatePoolWithTag(NonPagedPool, 2,'1pVX');
				if(params!=NULL) {
					params[0]=evento.dato;
					params[1]=*((PUCHAR)nodo->link->Datos+1);
					ColaBorrarNodo(cola,nodo->link,nodo);
					EnviarOrdenX52(devExt,7,params,2);
				} else {
					ColaBorrarNodo(cola,nodo->link,nodo);
				}
			}
#endif
			break;
#pragma endregion
	}

	*procesarOtraAccion=dentroDeBucle;
	return ColaBorrarNodo(cola,nodo,nodoA);
}

VOID ProcesarAccion(PDEVICE_EXTENSION devExt,BOOLEAN raton)
{
	if(!ColaEstaVacia(&devExt->ListaAcciones)) {
		PNODO nodo=devExt->ListaAcciones.principio;
		PNODO nodoA=NULL;
		BOOLEAN finEvt,sig=FALSE;
		do {
			if(raton) {
				finEvt=ProcesarEventoRaton(devExt,(PCOLA)nodo->Datos,&sig);
			} else {
				finEvt=ProcesarEventoTeclado(devExt,(PCOLA)nodo->Datos,&sig);
			}
			if(finEvt) { // Fin eventos
				PNODO nodos=nodo->link;
				ColaBorrar((PCOLA)nodo->Datos);
				nodo->Datos=NULL;
				ColaBorrarNodo(&devExt->ListaAcciones,nodo,nodoA);
				nodo=nodos;
			} else {	// Quedan eventos
				nodoA=nodo;
				nodo=nodo->link;
			}
		} while(sig && nodo!=NULL);

		if(ColaEstaVacia(&devExt->ListaAcciones)) {
			KeClearEvent(&devExt->evAccion);
		}
	} else {
		KeClearEvent(&devExt->evAccion);
	}
}

VOID ProcesarAccionRaton(PDEVICE_EXTENSION devExt)
{
	ProcesarAccion(devExt,TRUE);
}

VOID ProcesarAccionTeclado(PDEVICE_EXTENSION devExt)
{
	ProcesarAccion(devExt,FALSE);
}


VOID 
DpcRoutineDelay(   
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )
{
	PDELAY_CONTEXT ctx=(PDELAY_CONTEXT)DeferredContext;
	PDEVICE_EXTENSION devExt=ctx->DeviceExtension;
	PNODO nodoini=ctx->NodoIni,nodofin=ctx->NodoFin;
	PCOLA eventos;
	ExFreePoolWithTag(DeferredContext,(ULONG)'npHV');

	eventos=ColaCrear();
	if(eventos!=NULL) {
		eventos->principio=nodoini;
		eventos->fin=nodofin;
		KeAcquireSpinLockAtDpcLevel(&devExt->slListaAcciones);
		if(!ColaPush(&devExt->ListaAcciones,eventos)) {
			KeReleaseSpinLockFromDpcLevel(&devExt->slListaAcciones);
			ColaBorrar(eventos);
		} else {
			KeSetEvent(&devExt->evAccion,0,FALSE);
			KeReleaseSpinLockFromDpcLevel(&devExt->slListaAcciones);
		}
	}
}

VOID EnviarEstadoX52(PDEVICE_EXTENSION devExt, PHID_INPUT_DATA sthid)
{
	PESTADO_WORKITEM wi = ExAllocatePoolWithTag(NonPagedPool, sizeof(ESTADO_WORKITEM),'iwVX');
	if(wi!=NULL) {
		wi->WorkItem = IoAllocateWorkItem(devExt->self);
		if(wi->WorkItem!=NULL) {
			wi->DeviceExtension = devExt;
			RtlCopyMemory(&wi->Estado, sthid, sizeof(HID_INPUT_DATA));
			IoQueueWorkItem(wi->WorkItem,PreEstadoX52,DelayedWorkQueue,wi);
		} else {
			ExFreePool(wi);
		}
	} 
}
VOID EnviarOrdenX52(PDEVICE_EXTENSION devExt, UCHAR tipo, PVOID* params, UCHAR tam)
{
	PORDEN_WORKITEM wi = ExAllocatePoolWithTag(NonPagedPool, sizeof(ORDEN_WORKITEM),'iwVX');
	if(wi!=NULL) {
		wi->WorkItem = IoAllocateWorkItem(devExt->self);
		if(wi->WorkItem!=NULL) {
			wi->DeviceExtension = devExt;
			wi->Params = params;
			wi->Tam = tam;
			wi->Tipo = tipo;
			IoQueueWorkItem(wi->WorkItem,PreEnviarX52,DelayedWorkQueue,wi);
		} else {
			ExFreePool(wi);
			ExFreePool(params);
		}
	} else {
		ExFreePool(params);
	}
}


VOID PreEstadoX52(IN PDEVICE_OBJECT DevObj,IN PVOID Context)
{
	PIO_WORKITEM wi = ((PESTADO_WORKITEM)Context)->WorkItem;
	PDEVICE_EXTENSION devExt = ((PESTADO_WORKITEM)Context)->DeviceExtension;
	HANDLE hHilo = NULL;
	OBJECT_ATTRIBUTES oa;
	PESTADOX52_CONTEXT ctx = NULL;

	ExFreePool(Context);

	PAGED_CODE();
	ctx = ExAllocatePoolWithTag(PagedPool, sizeof(ESTADOX52_CONTEXT),'xeVX');
	if(ctx!=NULL) {
		KeInitializeEvent(&ctx->Ev, NotificationEvent, FALSE);
		ctx->DeviceExtension	= devExt;
		RtlCopyMemory(&ctx->Estado, &((PESTADO_WORKITEM)Context)->Estado, sizeof(HID_INPUT_DATA));
		InitializeObjectAttributes(&oa, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);

		if(PsCreateSystemThread(&hHilo,THREAD_ALL_ACCESS,&oa,NULL,NULL,EstadoX52,ctx)==STATUS_SUCCESS) {
			ObReferenceObjectByHandle(hHilo,THREAD_ALL_ACCESS,NULL,KernelMode,&ctx->Hilo,NULL);
			KeSetEvent(&ctx->Ev,0,FALSE);
			ZwClose(hHilo);
		}
	}

	IoFreeWorkItem(wi);
}
VOID PreEnviarX52(IN PDEVICE_OBJECT DevObj,IN PVOID Context)
{
	PIO_WORKITEM wi			= ((PORDEN_WORKITEM)Context)->WorkItem;
	PVOID params			= ((PORDEN_WORKITEM)Context)->Params;
	UCHAR tam				= ((PORDEN_WORKITEM)Context)->Tam;
	UCHAR tipo				= ((PORDEN_WORKITEM)Context)->Tipo;
	PDEVICE_EXTENSION devExt= ((PORDEN_WORKITEM)Context)->DeviceExtension;
	PVOID paramsPaged = NULL;
	HANDLE hHilo = NULL;
	OBJECT_ATTRIBUTES oa;
	PORDENX52_CONTEXT ctx = NULL;

	ExFreePool(Context);

	PAGED_CODE();
	paramsPaged = ExAllocatePoolWithTag(PagedPool, tam,'2pVX');
	if( paramsPaged!=NULL) {
		RtlCopyMemory(paramsPaged,params,tam);
		ExFreePool(params);
		ctx = ExAllocatePoolWithTag(PagedPool, sizeof(ORDENX52_CONTEXT),'xeVX');
		if(ctx!=NULL) {
			KeInitializeEvent(&ctx->Ev, NotificationEvent, FALSE);
			ctx->DeviceExtension	= devExt;
			ctx->Params				= paramsPaged;
			ctx->Tam				= tam;
			ctx->Tipo				= tipo;
			InitializeObjectAttributes(&oa, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);

			if(PsCreateSystemThread(&hHilo,THREAD_ALL_ACCESS,&oa,NULL,NULL,EnviarX52,ctx)==STATUS_SUCCESS) {
				ObReferenceObjectByHandle(hHilo,THREAD_ALL_ACCESS,NULL,KernelMode,&ctx->Hilo,NULL);
				KeSetEvent(&ctx->Ev,0,FALSE);
				ZwClose(hHilo);
			}
		} else {
			ExFreePool(paramsPaged);
		}
	} else {
		ExFreePool(params);
	}

	IoFreeWorkItem(wi);
}


#define IOCTL_ESTADO_USB	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0108, METHOD_BUFFERED, FILE_WRITE_ACCESS)
VOID EstadoX52(IN PVOID Context)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	PDEVICE_EXTENSION devExt = ((PESTADOX52_CONTEXT)Context)->DeviceExtension;
	HID_INPUT_DATA params;
	PETHREAD pHilo;

	PAGED_CODE();
	
	RtlCopyMemory(&params, &((PESTADOX52_CONTEXT)Context)->Estado, sizeof(HID_INPUT_DATA));
	KeWaitForSingleObject(&((PESTADOX52_CONTEXT)Context)->Ev,Executive,KernelMode,FALSE,NULL);
	pHilo = ((PESTADOX52_CONTEXT)Context)->Hilo;

	ExFreePool(Context);
	KeSetPriorityThread(KeGetCurrentThread(), LOW_REALTIME_PRIORITY );

	KeWaitForSingleObject(&devExt->mutexUSB,Executive,KernelMode,FALSE,NULL);

	if(!devExt->finUSB) {
		NTSTATUS st = STATUS_SUCCESS;
		if(devExt->ufo == NULL) {
			//=========================== Filtro USB =========================================
			UNICODE_STRING devname;
			RtlInitUnicodeString(&devname,L"\\Device\\XUsb_HidF");
			st = IoGetDeviceObjectPointer(&devname,GENERIC_WRITE,&devExt->ufo,&devExt->udo);
			//================================================================================
		}
		if(NT_SUCCESS(st) && (KeGetCurrentIrql()<=APC_LEVEL)) { 
			KEVENT                  IoctlCompleteEvent;
			IO_STATUS_BLOCK         IoStatus;
			PIRP                    pIrp;
			//HID_INPUT_DATA			params;
			KIRQL					irql;

			//KeAcquireSpinLock(&devExt->slGameData, &irql);
			//	RtlCopyMemory(&params, &devExt->stHidData, sizeof(HID_INPUT_DATA));
			//KeReleaseSpinLock(&devExt->slGameData, irql);

			KeInitializeEvent(&IoctlCompleteEvent, NotificationEvent, FALSE);
			pIrp=IoBuildDeviceIoControlRequest(IOCTL_ESTADO_USB,devExt->udo,&params,sizeof(HID_INPUT_DATA),NULL,0,TRUE,&IoctlCompleteEvent,&IoStatus);
			if(pIrp!=NULL)
			{
				st = IoCallDriver(devExt->udo, pIrp);
				if (st == STATUS_PENDING) {
					KeWaitForSingleObject(&IoctlCompleteEvent,Executive,KernelMode,FALSE,NULL);
				}
			}
		}
	}

	KeSetEvent(&devExt->mutexUSB,0,FALSE);

	ObDereferenceObject(pHilo);
	PsTerminateSystemThread(0);
}
#define IOCTL_MFD_LUZ		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0100, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_GLOBAL_LUZ	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0101, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_INFO_LUZ		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0102, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_PINKIE		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0103, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_TEXTO			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0104, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_HORA			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0105, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_HORA24		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0106, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_FECHA			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0107, METHOD_BUFFERED, FILE_WRITE_ACCESS)
VOID EnviarX52(IN PVOID Context)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION devExt= ((PORDENX52_CONTEXT)Context)->DeviceExtension;
	PVOID params			= ((PORDENX52_CONTEXT)Context)->Params;
	UCHAR tipo				= ((PORDENX52_CONTEXT)Context)->Tipo;
	UCHAR tam				= ((PORDENX52_CONTEXT)Context)->Tam;
	UINT32 ctlCode;

	PETHREAD pHilo;

	PAGED_CODE();
	
	KeWaitForSingleObject(&((PORDENX52_CONTEXT)Context)->Ev,Executive,KernelMode,FALSE,NULL);
	pHilo = ((PORDENX52_CONTEXT)Context)->Hilo;

	ExFreePool(Context);
	KeSetPriorityThread(KeGetCurrentThread(), LOW_REALTIME_PRIORITY );

	KeWaitForSingleObject(&devExt->mutexUSB,Executive,KernelMode,FALSE,NULL);

	if(!devExt->finUSB) {
		NTSTATUS st = STATUS_SUCCESS;
		if(devExt->ufo == NULL) {
			//=========================== Filtro USB =========================================
			UNICODE_STRING devname;
			RtlInitUnicodeString(&devname,L"\\Device\\XUsb_HidF");
			st = IoGetDeviceObjectPointer(&devname,GENERIC_WRITE,&devExt->ufo,&devExt->udo);
			//================================================================================
		}
		if(NT_SUCCESS(st) && (KeGetCurrentIrql()<=APC_LEVEL)) { 
			KEVENT                  IoctlCompleteEvent;
			IO_STATUS_BLOCK         IoStatus;
			PIRP                    pIrp;
			switch(tipo) {
				case 0:
					ctlCode = IOCTL_MFD_LUZ;
					break;
				case 1:
					ctlCode = IOCTL_GLOBAL_LUZ;
					break;
				case 2:
					ctlCode = IOCTL_INFO_LUZ;
					break;
				case 3:
					ctlCode = IOCTL_PINKIE;
					break;
				case 4:
					ctlCode = IOCTL_TEXTO;
					break;
				case 5:
					ctlCode = IOCTL_HORA;
					break;
				case 6:
					ctlCode = IOCTL_HORA24;
					break;
				case 7:
					ctlCode = IOCTL_FECHA;
			}


			KeInitializeEvent(&IoctlCompleteEvent, NotificationEvent, FALSE);
			pIrp=IoBuildDeviceIoControlRequest(ctlCode,devExt->udo,params,tam,NULL,0,FALSE,&IoctlCompleteEvent,&IoStatus);

			if(pIrp!=NULL)
			{
				st = IoCallDriver(devExt->udo, pIrp);
				//if (st == STATUS_PENDING) {
				//	KeWaitForSingleObject(&IoctlCompleteEvent,Executive,KernelMode,FALSE,NULL);
				//}
			}
		}
	}

	ExFreePool(params);
	KeSetEvent(&devExt->mutexUSB,0,FALSE);

	ObDereferenceObject(pHilo);
	PsTerminateSystemThread(0);
}