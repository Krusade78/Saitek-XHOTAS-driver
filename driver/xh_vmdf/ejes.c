#include <wdm.h>
#include "hidfilter.h"
#include "acciones.h"

#define _EJES_
#include "ejes.h"
#undef _EJES


#pragma region "IOCTL"

NTSTATUS
HF_IoEscribirCalibrado(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
	)
{
	PIO_STACK_LOCATION              irpStack;
    PITFDEVICE_EXTENSION            devExt;
    NTSTATUS                        status = STATUS_SUCCESS;
	UCHAR							i;

	PAGED_CODE();

    devExt = (PITFDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    irpStack = IoGetCurrentIrpStackLocation(Irp);
	if(irpStack->Parameters.DeviceIoControl.InputBufferLength==(sizeof(CALIBRADO)*4)) {
		PCALIBRADO bufCal=(PCALIBRADO)Irp->AssociatedIrp.SystemBuffer;
		KIRQL irql;
		for(i=0; i<4; i++) {
			KeAcquireSpinLock(&devExt->slCalibrado,&irql);
			devExt->limites[i].c=bufCal[i].c;
			devExt->limites[i].i=bufCal[i].i;
			devExt->limites[i].d=bufCal[i].d;
			devExt->limites[i].n=bufCal[i].n;
			devExt->limites[i].cal=bufCal[i].cal;
			devExt->jitter[i].Margen=bufCal[i].Margen;
			devExt->jitter[i].Resistencia=bufCal[i].Resistencia;
			devExt->jitter[i].antiv=bufCal[i].antiv;
			KeReleaseSpinLock(&devExt->slCalibrado,irql);
		}
		Irp->IoStatus.Information = sizeof(CALIBRADO)*4;
	} else {
		status=STATUS_BUFFER_TOO_SMALL;
		Irp->IoStatus.Information = 0;
	}

	Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS
HF_IoDescalibrar(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
	)
{
    PIO_STACK_LOCATION              irpStack;
    PITFDEVICE_EXTENSION            devExt;
    NTSTATUS                        status = STATUS_SUCCESS;
	KIRQL irql;

    devExt = (PITFDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    irpStack = IoGetCurrentIrpStackLocation(Irp);

	devExt->descalibrar=TRUE;

	Irp->IoStatus.Information = 0;
	Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return status;
}

NTSTATUS
HF_IoRecalibrar(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
	)
{
    PIO_STACK_LOCATION              irpStack;
    PITFDEVICE_EXTENSION            devExt;
    NTSTATUS                        status = STATUS_SUCCESS;
	KIRQL irql;

    devExt = (PITFDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    irpStack = IoGetCurrentIrpStackLocation(Irp);

	devExt->descalibrar=FALSE;

	Irp->IoStatus.Information = 0;
	Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return status;
}
#pragma endregion

VOID Calibrado(PITFDEVICE_EXTENSION idevExt, PHID_INPUT_DATA datosHID, BOOLEAN esX52)
{
	UCHAR idx;
	LONG pollEje[4];
	KIRQL irql;

	// Filtrado de ejes
	for(idx=0;idx<4;idx++) {
		pollEje[idx]=*((UINT16*)&datosHID->Ejes[idx*2]);
#ifdef PRO
		KeAcquireSpinLock(&idevExt->slCalibrado,&irql);
		if(idevExt->jitter[idx].antiv) {
			// Antivibraciones
			if( (pollEje[idx]<(idevExt->jitter[idx].PosElegida-idevExt->jitter[idx].Margen)) || (pollEje[idx]>(idevExt->jitter[idx].PosElegida+idevExt->jitter[idx].Margen)) ) {
				idevExt->jitter[idx].PosRepetida=0;
				idevExt->jitter[idx].PosElegida=(UINT16)pollEje[idx];
			} else {
				if(idevExt->jitter[idx].PosRepetida<idevExt->jitter[idx].Resistencia) {
					idevExt->jitter[idx].PosRepetida++;
					pollEje[idx]=idevExt->jitter[idx].PosElegida;
				} else {
					idevExt->jitter[idx].PosRepetida=0;
					idevExt->jitter[idx].PosElegida=(UINT16)pollEje[idx];
				}
			}
		}
		KeReleaseSpinLock(&idevExt->slCalibrado,irql);
#endif
		KeAcquireSpinLock(&idevExt->slCalibrado,&irql);
		if(idevExt->limites[idx].cal) {
			// Calibrado
			UINT16 ancho1,ancho2;
			UINT16 topesX36[]={4095,4095,255,255};
			UINT16 topesX52[]={2047,2047,1023,255};
			ancho1=(idevExt->limites[idx].c-idevExt->limites[idx].n)-idevExt->limites[idx].i;
			ancho2=idevExt->limites[idx].d-(idevExt->limites[idx].c+idevExt->limites[idx].n);
			if( ((pollEje[idx]>=(idevExt->limites[idx].c-idevExt->limites[idx].n)) && (pollEje[idx]<=(idevExt->limites[idx].c+idevExt->limites[idx].n)) ))
			{
				if(idx==0 || idx==1) {
					pollEje[idx]=esX52?1023:2047;
					KeReleaseSpinLock(&idevExt->slCalibrado,irql);
					continue;
				} else {
					if(idx==2 && esX52) { pollEje[2]=511; } else {pollEje[idx]=127; }
					KeReleaseSpinLock(&idevExt->slCalibrado,irql);
					continue;
				}
			} else {
		        if(pollEje[idx]<idevExt->limites[idx].i)
				{
					pollEje[idx]=idevExt->limites[idx].i;
				} else {
					if(pollEje[idx]>idevExt->limites[idx].d)
					{
						pollEje[idx]=idevExt->limites[idx].d;
					}
				}
				if(pollEje[idx]<idevExt->limites[idx].c)
				{
					pollEje[idx]=((idevExt->limites[idx].c-idevExt->limites[idx].n)-pollEje[idx]);
					if(ancho1>ancho2)
					{
						pollEje[idx]= ((pollEje[idx]*ancho2)/ancho1);
					}
					pollEje[idx]=0-pollEje[idx];
				} else {
					pollEje[idx]-= (idevExt->limites[idx].c+idevExt->limites[idx].n);
					if(ancho2>ancho1)
					{
						pollEje[idx]= ((pollEje[idx]*ancho1)/ancho2);
					}
				}
			}
			//if(ancho1>ancho2) { pollEje[idx]+=ancho2; } else { pollEje[idx]+=ancho1; }
			if(ancho2 > ancho1) {
				pollEje[idx] = ((pollEje[idx] + ancho1) * ((esX52)?topesX52[idx]:topesX36[idx])) / (2 * ancho1);
			} else {

				pollEje[idx] = ((pollEje[idx] + ancho2) * ((esX52)?topesX52[idx]:topesX36[idx])) / (2 * ancho2);
			}
		}
		KeReleaseSpinLock(&idevExt->slCalibrado,irql);
	}

	for(idx=0;idx<4;idx++) { *((UINT16*)&datosHID->Ejes[idx*2])=(UINT16)pollEje[idx];}
}

VOID SensibilidadYMapeado(
			PITFDEVICE_EXTENSION devExt,
			BOOLEAN esX52,
			PHID_INPUT_DATA entrada,
			PHID_INPUT_DATA salida
			)
{
	UCHAR idx;
	UINT16 topesX36[]={4095,4095,255,255,255,255,255,15,15};
	UINT16 topesX52[]={2047,2047,1023,255,255,255,255,15,15};
	ULONG x;
	UCHAR pos;
	UCHAR sy1;
	UCHAR sy2;
	UINT16 stope;
	UINT16* topes;
	KIRQL irql;
	if(esX52) topes=topesX52; else topes=topesX36;
#ifdef PRO
	//Sensibilidad
	for(idx=0;idx<4;idx++) {
		x=*((UINT16*)&entrada->Ejes[idx*2]);
		if(x==(topes[idx]/2)) {
			continue;
		} else {
			pos=(UCHAR) ((x*20)/topes[idx]);
			if(pos==20) pos=19;
			if( (UINT16)x<(topes[idx]/2) ) {
				KeAcquireSpinLock(&devExt->slMapas,&irql);
					sy1=100-devExt->MapaEjes[devExt->stPinkie][devExt->stModo][devExt->stAux][idx].Sensibilidad[9-pos];
					sy2=(pos==9)?100:100-devExt->MapaEjes[devExt->stPinkie][devExt->stModo][devExt->stAux][idx].Sensibilidad[8-pos];
				KeReleaseSpinLock(&devExt->slMapas,irql);
				stope=(topes[idx]/2)+1;
			//x=( (10*sy1*topesX52[idx]) + (100*x*(sy2-sy1)) - (y1*10*topesX52[idx]*(sy2-sy1)) / 1000);
				x=(  ( (sy2-sy1)*((20*x)-(pos*topes[idx])) ) + (2*sy1*stope) ) /200;
			} else {
				KeAcquireSpinLock(&devExt->slMapas,&irql);
					sy1=(pos==10)?0:devExt->MapaEjes[devExt->stPinkie][devExt->stModo][devExt->stAux][idx].Sensibilidad[pos-11];
					sy2=devExt->MapaEjes[devExt->stPinkie][devExt->stModo][devExt->stAux][idx].Sensibilidad[pos-10];
				KeReleaseSpinLock(&devExt->slMapas,irql);
				stope=(topes[idx]/2)+1;
				x=stope + ((  ( (sy2-sy1)*((20*x)-(pos*topes[idx])) ) + (2*sy1*stope) ) /200);
			}
			*((UINT16*)&entrada->Ejes[idx*2])=(UINT16)x;
		}
	}
#endif

	//Mapeado
	for(idx=0;idx<7;idx++) {
		UCHAR nEje;
		UCHAR sRaton;
		BOOLEAN invertido=FALSE;

		KeAcquireSpinLock(&devExt->slMapas,&irql);
			if(idx<4) {
				nEje=devExt->MapaEjes[devExt->stPinkie][devExt->stModo][devExt->stAux][idx].nEje & 127;
				sRaton=devExt->MapaEjes[devExt->stPinkie][devExt->stModo][devExt->stAux][idx].Mouse;
			} else {
				nEje=devExt->MapaEjesPeque[devExt->stPinkie][devExt->stModo][devExt->stAux][idx-4].nEje & 127;
				sRaton=devExt->MapaEjesPeque[devExt->stPinkie][devExt->stModo][devExt->stAux][idx-4].Mouse;
			}
		KeReleaseSpinLock(&devExt->slMapas,irql);
		if(nEje>19) { invertido=TRUE; nEje-=20; }
		if(nEje!=0) {
			if(nEje<8) {
				*((USHORT*)&salida->Ejes[(nEje-1)*2])=((*((USHORT*)&entrada->Ejes[idx*2]))* topes[nEje-1] )/ topes[idx];
			} else if(nEje<10) {
				salida->MiniStick&=(nEje==8)?0xf0:0x0f; 
				salida->MiniStick|=( ( ((*((USHORT*)&entrada->Ejes[idx*2]))* topes[nEje-1])/ topes[idx] )<<(4*(nEje-8)) );
			} else {
				KIRQL irql;
				if( *((UINT16*)&entrada->Ejes[idx]) == (topes[idx]/2) ) {
					KeAcquireSpinLock(&devExt->vDevExt->slRaton,&irql);
						devExt->vDevExt->stRaton[nEje-9]=0;
					KeReleaseSpinLock(&devExt->vDevExt->slRaton,irql);
				} else if( *((UINT16*)&entrada->Ejes[idx]) < (topes[idx]/2) ) {
					if( *((UINT16*)&entrada->Ejes[idx]) == 0) {
						KeAcquireSpinLock(&devExt->vDevExt->slRaton,&irql);
							devExt->vDevExt->stRaton[nEje-9]=-sRaton;
						KeReleaseSpinLock(&devExt->vDevExt->slRaton,irql);
					} else {
						KeAcquireSpinLock(&devExt->vDevExt->slRaton,&irql);
							devExt->vDevExt->stRaton[nEje-9]=(*((UINT16*)&entrada->Ejes[idx*2])*sRaton)/(topes[idx]/2);
							devExt->vDevExt->stRaton[nEje-9]*=-1;
						KeReleaseSpinLock(&devExt->vDevExt->slRaton,irql);
					}
				} else {
					KeAcquireSpinLock(&devExt->vDevExt->slRaton,&irql);
						devExt->vDevExt->stRaton[nEje-9]=((*((UINT16*)&entrada->Ejes[idx*2])-(topes[idx]/2))*sRaton)/(topes[idx]/2);
					KeReleaseSpinLock(&devExt->vDevExt->slRaton,irql);
				}
			}
			if(invertido) {
				if(nEje<8) {
					*((USHORT*)&salida->Ejes[(nEje-1)*2])=topes[nEje-1]-( *((USHORT*)&salida->Ejes[(nEje-1)*2]) );
				} else if(nEje<10) {
					salida->MiniStick=15-salida->MiniStick;
				} else {
					KIRQL irql;
					KeAcquireSpinLock(&devExt->vDevExt->slRaton,&irql);
						devExt->vDevExt->stRaton[nEje-9]*=-1;
					KeReleaseSpinLock(&devExt->vDevExt->slRaton,irql);
				}
			}
		}
	}
	//if(esX52) {
		for(idx=0;idx<2;idx++) {
			UCHAR nEje;
			UCHAR sRaton;
			BOOLEAN invertido=FALSE;
			KeAcquireSpinLock(&devExt->slMapas,&irql);
				nEje=devExt->MapaEjesMini[devExt->stPinkie][devExt->stModo][devExt->stAux][idx].nEje & 127;
				sRaton=devExt->MapaEjesMini[devExt->stPinkie][devExt->stModo][devExt->stAux][idx].Mouse;
			KeReleaseSpinLock(&devExt->slMapas,irql);
			if(nEje>19) { invertido=TRUE; nEje-=20; }
			if(nEje!=0) {
				if(nEje<8) {
					*((USHORT*)&salida->Ejes[(nEje-1)*2])=( ((idx==0)?(entrada->MiniStick&0xf):(entrada->MiniStick>>4))* topes[nEje-1] )/ 15;
				} else if(nEje<10) {
					salida->MiniStick&=(nEje==8)?0xf0:0x0f; 
					salida->MiniStick|=((idx==0)?(entrada->MiniStick&0xf):(entrada->MiniStick>>4))<<(4*(nEje-8));
				} else {
					KIRQL irql;
					if( ((entrada->MiniStick>>(4*(nEje-10)))&0xf) == 8 ) {
						KeAcquireSpinLock(&devExt->vDevExt->slRaton,&irql);
							devExt->vDevExt->stRaton[nEje-9]=0;
						KeReleaseSpinLock(&devExt->vDevExt->slRaton,irql);
					} else if( ((entrada->MiniStick>>(4*(nEje-10)))&0xf) < 8 ) {
						if( ((entrada->MiniStick>>(4*(nEje-10)))&0xf)==0) {
							KeAcquireSpinLock(&devExt->vDevExt->slRaton,&irql);
								devExt->vDevExt->stRaton[nEje-9]=-sRaton;
							KeReleaseSpinLock(&devExt->vDevExt->slRaton,irql);
						} else {
							KeAcquireSpinLock(&devExt->vDevExt->slRaton,&irql);
								devExt->vDevExt->stRaton[nEje-9]=(((entrada->MiniStick>>(4*(nEje-10)))&0xf)*sRaton)/(15/2);
								devExt->vDevExt->stRaton[nEje-9]*=-1;
							KeReleaseSpinLock(&devExt->vDevExt->slRaton,irql);
						}
					} else {
						KeAcquireSpinLock(&devExt->vDevExt->slRaton,&irql);
							devExt->vDevExt->stRaton[nEje-9]=( (((entrada->MiniStick>>(4*(nEje-10)))&0xf)-(15/2))*sRaton )/(15/2);
						KeReleaseSpinLock(&devExt->vDevExt->slRaton,irql);
					}
				}
				if(invertido) {
					if(nEje<8) {
						*((USHORT*)&salida->Ejes[(nEje-1)*2])=topes[nEje-1]-( *((USHORT*)&salida->Ejes[(nEje-1)*2]) );
					} else if(nEje<10) {
						salida->MiniStick=15-salida->MiniStick;
					} else {
						KIRQL irql;
						KeAcquireSpinLock(&devExt->vDevExt->slRaton,&irql);
							devExt->vDevExt->stRaton[nEje-9]*=-1;
						KeReleaseSpinLock(&devExt->vDevExt->slRaton,irql);
					}
				}
			}
		}
	//}
}

VOID AccionEje
(	PDEVICE_EXTENSION devExt,
	BOOLEAN esX52,
	UCHAR idx,
	USHORT nuevo)
{
	PITFDEVICE_EXTENSION idevExt = (PITFDEVICE_EXTENSION)devExt->itfObj->DeviceExtension;
	UINT16 accionID;
	KIRQL irql;
	UCHAR cambio;

	KeAcquireSpinLock(&idevExt->slMapas,&irql);
		cambio=TraducirGiratorio(idevExt, esX52, idx, nuevo);
		if(cambio!=0) {
			if(idx<4) {
				accionID=idevExt->MapaEjes[idevExt->stPinkie][idevExt->stModo][idevExt->stAux][idx].Indices[cambio-1];
			} else {
				accionID=idevExt->MapaEjesPeque[idevExt->stPinkie][idevExt->stModo][idevExt->stAux][idx-4].Indices[cambio-1];
			}
		}
	KeReleaseSpinLock(&idevExt->slMapas,irql);

	if(cambio!=0) Accionar(devExt,accionID,0);
}


UCHAR TraducirGiratorio(
	PITFDEVICE_EXTENSION idevExt,
	BOOLEAN esX52,
	UCHAR eje,
	USHORT nueva
	)
{
	USHORT topesX36[]={4095,4095,255,255,255,255,255,15,15};
	USHORT topesX52[]={2047,2047,1023,255,255,255,255,15,15};
	USHORT* topes;
    BOOLEAN incremental;
	UCHAR idn=0;
	USHORT vieja=idevExt->posVieja[idevExt->stPinkie][idevExt->stModo][idevExt->stAux][eje];	

	if(eje<4) {
		incremental=(idevExt->MapaEjes[idevExt->stPinkie][idevExt->stModo][idevExt->stAux][eje].nEje & 128)==128;
	} else {
		incremental=(idevExt->MapaEjesPeque[idevExt->stPinkie][idevExt->stModo][idevExt->stAux][eje-4].nEje & 128)==128;
	}

	if(esX52) topes=topesX52; else topes=topesX36;

	if (incremental) {
		if (nueva>vieja) {
			UCHAR posiciones;
			if(eje<4) {
				posiciones=(UCHAR)idevExt->MapaEjes[idevExt->stPinkie][idevExt->stModo][idevExt->stAux][eje].Indices[3];
			} else {
				posiciones=(UCHAR)idevExt->MapaEjesPeque[idevExt->stPinkie][idevExt->stModo][idevExt->stAux][eje-4].Indices[3];
			}
			if(vieja<(topes[eje]-posiciones)) {
				if (nueva>(vieja+posiciones)) {
					idevExt->posVieja[(UCHAR)idevExt->stPinkie][idevExt->stModo][idevExt->stAux][eje]=nueva;
					idn=1;
				}
			}
		} else {
			UCHAR posiciones;
			if(eje<4) {
				posiciones=(UCHAR)idevExt->MapaEjes[idevExt->stPinkie][idevExt->stModo][idevExt->stAux][eje].Indices[2];
			} else {
				posiciones=(UCHAR)idevExt->MapaEjesPeque[idevExt->stPinkie][idevExt->stModo][idevExt->stAux][eje-4].Indices[2];
			}
			if(vieja>posiciones) {
				if (nueva<(vieja-posiciones)) {
					idevExt->posVieja[(UCHAR)idevExt->stPinkie][idevExt->stModo][idevExt->stAux][eje]=nueva;
					idn=2;
				}
			}
		}
	} else {
		USHORT posActual=10000;
		UCHAR posant=0;
		UCHAR idc;
		//nueva=topes[eje]-nueva;
		for(idc=0; idc<15; idc++) {
			UCHAR banda;
			if(eje<4) {
				banda=idevExt->MapaEjes[idevExt->stPinkie][idevExt->stModo][idevExt->stAux][eje].Bandas[idc];
			} else {
				banda=idevExt->MapaEjesPeque[idevExt->stPinkie][idevExt->stModo][idevExt->stAux][eje-4].Bandas[idc];
			}
			if(banda==0) {
				break;
			} else {
				if(nueva>=posant && nueva<((banda*topes[eje])/100) ) {
					posActual=idc;
					break;
				}
				posant=(banda*topes[eje])/100;
			}
		}
		if(posActual==10000) {
			if(nueva>=posant && nueva<(topes[eje]+1) ) posActual=idc;
		}
		if(posActual!=10000 && posActual!=vieja) {
			idevExt->posVieja[idevExt->stPinkie][idevExt->stModo][idevExt->stAux][eje]=posActual;
			idn=posActual+1;
		}
	}

	return idn;
}