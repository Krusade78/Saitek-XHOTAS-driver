/*--
Copyright (c) 2005-2007 Alfredo Costalago

Module Name:

    x36filtr.c

Abstract:

	Funciones nuevas del filtro para el X36

--*/

#include "x36filtr.h"
#include "x36.c"

#ifdef ALLOC_PRAGMA
	#pragma alloc_text (PAGE, X36Filter_Iniciar)
	#pragma alloc_text (PAGE, X36Filter_InterfaceDispatch)
#endif

VOID X36Filter_Iniciar(
	IN PX36DEVICE_EXTENSION x36DevExt
	)
{
	PAGED_CODE();

	// Variables de estado
	x36DevExt->faseIni = FALSE;
	x36DevExt->bytesIni = 0;
	x36DevExt->fase = 0;
	//x36DevExt->acelPed = 0;

	//x36DevExt->stSeta1=0;
	//x36DevExt->stSeta2=0;
	x36DevExt->gPosicion = 0;
	x36DevExt->gEje = 0;

	// Variables de configuración
	RtlZeroMemory(&x36DevExt->hidData,sizeof(HIDGAME_INPUT_DATA));
}

VOID X36Filter_IniciarJoy(
	PX36DEVICE_EXTENSION x36DevExt//,
	//UCHAR ace_ped
	)
{
	PDEVICE_EXTENSION devExt=x36DevExt->kDevExt;

	x36DevExt->faseIni=TRUE;
	x36DevExt->fase = 0;
	//if(ace_ped<4) x36DevExt->acelPed=ace_ped;

	// Encabezado
	devExt->IsrWritePort(devExt->CallContext,110);
	devExt->IsrWritePort(devExt->CallContext,16);
	devExt->IsrWritePort(devExt->CallContext,11);
	// Botones 1,2,3,4,5,6
	devExt->IsrWritePort(devExt->CallContext,0);
	devExt->IsrWritePort(devExt->CallContext,0);
	devExt->IsrWritePort(devExt->CallContext,0);
	devExt->IsrWritePort(devExt->CallContext,0);
	devExt->IsrWritePort(devExt->CallContext,0);
	devExt->IsrWritePort(devExt->CallContext,0);
	// POVs
	devExt->IsrWritePort(devExt->CallContext,0);
	devExt->IsrWritePort(devExt->CallContext,0);
	// Acelerador y Timón
	devExt->IsrWritePort(devExt->CallContext, 0); //x36DevExt->acelPed);
	// Fin
	devExt->IsrWritePort(devExt->CallContext,8);
	devExt->IsrWritePort(devExt->CallContext,16);
}

NTSTATUS
X36Filter_InterfaceDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Interfaz de control X36                                          
                                         
Arguments:

    DeviceObject - Pointer to the device object.

    Irp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/
{
    PIO_STACK_LOCATION          irpStack;
    PX36DEVICE_EXTENSION        x36DevExt=(PX36DEVICE_EXTENSION) DeviceObject->DeviceExtension;
    NTSTATUS					status=STATUS_SUCCESS;

	PAGED_CODE();

    Irp->IoStatus.Information = 0;
    irpStack = IoGetCurrentIrpStackLocation(Irp);

	switch(irpStack->MajorFunction) {
        case IRP_MJ_CREATE:
        case IRP_MJ_CLOSE:
        case IRP_MJ_CLEANUP:
	        break;
		case IRP_MJ_DEVICE_CONTROL:
		{
			if(irpStack->Parameters.DeviceIoControl.IoControlCode==IOCTL_X36_JOYINICIAR)
			{
				/*if(irpStack->Parameters.DeviceIoControl.InputBufferLength==1)
				{
					UCHAR buf=*((UCHAR*)Irp->AssociatedIrp.SystemBuffer);
					*((UCHAR*)Irp->AssociatedIrp.SystemBuffer)=x36DevExt->acelPed;
					if(buf!=x36DevExt->acelPed) X36Filter_IniciarJoy(x36DevExt,buf);
					Irp->IoStatus.Information = 1;
				} else*/ if(irpStack->Parameters.DeviceIoControl.InputBufferLength==0){
					X36Filter_IniciarJoy(x36DevExt);//255);
				} else {
					status= STATUS_INVALID_BUFFER_SIZE;
				}
			} else {
				status=STATUS_NOT_SUPPORTED;
			}
			break;
		}
		default:
			status=STATUS_NOT_SUPPORTED;
	}
	
	Irp->IoStatus.Status = status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}


VOID
X36Filter_ServiceCallback(
    IN PDEVICE_OBJECT DeviceObject,
    IN PKEYBOARD_INPUT_DATA InputDataStart,
    IN PKEYBOARD_INPUT_DATA InputDataEnd,
    IN OUT PULONG InputDataConsumed
    )
/*++

Routine Description:

    Called when there are keyboard packets to report to the RIT.
                    
Arguments:

    DeviceObject - Context passed during the connect IOCTL
    
    InputDataStart - First packet to be reported
    
    InputDataEnd - One past the last packet to be reported.  Total number of
                   packets is equal to InputDataEnd - InputDataStart
    
    InputDataConsumed - Set to the total number of packets consumed by the RIT
                        (via the function pointer we replaced in the connect
                        IOCTL)

Return Value:

    Status is returned.

--*/
{
	USHORT auxCode;
    PDEVICE_EXTENSION   devExt = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
	PX36DEVICE_EXTENSION   x36DevExt = (PX36DEVICE_EXTENSION) devExt->x36Obj->DeviceExtension;
	UCHAR				npaquetes =(UCHAR)(InputDataEnd-InputDataStart);
	UCHAR				i = 0;

	KeAcquireSpinLockAtDpcLevel(&devExt->SpinLock);

	for(i=0;i<npaquetes;i++) {

		if (x36DevExt->faseIni) {
			x36DevExt->bytesIni++;
			if(x36DevExt->bytesIni>9) {
				x36DevExt->bytesIni=0;
				x36DevExt->faseIni=FALSE;
			}
			(InputDataStart+i)->MakeCode=111;
			continue;
		}

		auxCode=(InputDataStart+i)->MakeCode;
		switch(x36DevExt->fase)
		{
			case 6:
				if(auxCode>47 && auxCode<64) {
					x36DevExt->fase=0;
					(InputDataStart+i)->MakeCode=111;
					TraducirGiratorio(x36DevExt,auxCode);
				} else {
					if(((InputDataStart+i)->Flags&KEY_BREAK)!=KEY_BREAK) (InputDataStart+i)->MakeCode=111;
				}
				break;
			case 5:
				if(auxCode>47 && auxCode<64) {
					x36DevExt->fase=6;
					(InputDataStart+i)->MakeCode=111;
					x36DevExt->gPosicion=auxCode;
				} else {
					if(((InputDataStart+i)->Flags&KEY_BREAK)!=KEY_BREAK) (InputDataStart+i)->MakeCode=111;
				}
				break;
			case 4:
				if(auxCode==33) {
					x36DevExt->gEje=0;
				} else if(auxCode==34) {
					x36DevExt->gEje=1;
				} else if(auxCode==35) {
					x36DevExt->gEje=2;
				} else if(auxCode==36) {
					x36DevExt->gEje=3;
				} else {
					if(((InputDataStart+i)->Flags&KEY_BREAK)!=KEY_BREAK) (InputDataStart+i)->MakeCode=111;
					//x36DevExt->fase=0;
					break;
				}
				x36DevExt->fase=5;
				(InputDataStart+i)->MakeCode=111;
				break;
			case 3:
				x36DevExt->fase=0;
				(InputDataStart+i)->MakeCode=111;
                TraducirBoton(x36DevExt,auxCode);
				break;
			case 2:
				if (auxCode==1) {
					x36DevExt->fase=3;
					(InputDataStart+i)->MakeCode=111;
				} else if (auxCode==3) {
					x36DevExt->fase=4;
					(InputDataStart+i)->MakeCode=111;
				} else {
					if(((InputDataStart+i)->Flags&KEY_BREAK)!=KEY_BREAK) (InputDataStart+i)->MakeCode=111;
					//x36DevExt->fase=0;
				}
				break;
			case 1:
				if(auxCode==16) {
					x36DevExt->fase=2;
					(InputDataStart+i)->MakeCode=111;
				} else {
					if(((InputDataStart+i)->Flags&KEY_BREAK)!=KEY_BREAK) (InputDataStart+i)->MakeCode=111;
					//x36DevExt->fase=0;
				}
				break;
			case 0:
				if (auxCode==111) {
					x36DevExt->fase=1;
				}
				break;
		}
	}

	// Eliminar los paquetes 111 (Paquetes del joy)
	for(i=0;i<npaquetes;i++) {
		if((InputDataStart+i)->MakeCode==111)
		{
			npaquetes--;
			RtlCopyMemory(InputDataStart+i,InputDataStart+i+1,sizeof(KEYBOARD_INPUT_DATA)*npaquetes);
			i--;
		}
	}

	i=(UCHAR)(InputDataEnd-InputDataStart);
	InputDataEnd=InputDataStart+npaquetes;

	KeReleaseSpinLockFromDpcLevel(&devExt->SpinLock);

	(*(PSERVICE_CALLBACK_ROUTINE) devExt->UpperConnectData.ClassService)(
			devExt->UpperConnectData.ClassDeviceObject,
			InputDataStart,
			InputDataEnd,
			InputDataConsumed);

	// Para paquetes extra largos que no se pueden procesar
	// enteros, rellamar con lo que falte.

	if(*InputDataConsumed!=(InputDataEnd-InputDataStart))
	{
		InputDataStart+=*InputDataConsumed;
		(*(PSERVICE_CALLBACK_ROUTINE) devExt->UpperConnectData.ClassService)(
			devExt->UpperConnectData.ClassDeviceObject,
			InputDataStart,
			InputDataEnd,
			InputDataConsumed);
	}

	*InputDataConsumed=i;
}
