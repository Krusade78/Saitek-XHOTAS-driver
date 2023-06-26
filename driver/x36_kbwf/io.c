#include <wdm.h>
#include "hidfilter.h"
#include "control.h"

#define _IO_
#include "io.h"
#undef _IO_

#ifdef ALLOC_PRAGMA
    #pragma alloc_text( PAGE, HF_IoLeer)
    #pragma alloc_text( PAGE, HF_IoWrite)
#endif /* ALLOC_PRAGMA */

NTSTATUS
HF_IoLeer(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PIO_STACK_LOCATION              irpStack;
    PITFDEVICE_EXTENSION            devExt;
    NTSTATUS                        status = STATUS_SUCCESS;

	PAGED_CODE();

    devExt = (PITFDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    irpStack = IoGetCurrentIrpStackLocation(Irp);

	if(irpStack->Parameters.DeviceIoControl.OutputBufferLength==sizeof(HIDGAME_INPUT_DATA)) {
		KIRQL irql;
		HIDGAME_INPUT_DATA gd;
		KeAcquireSpinLock(&devExt->slGameData,&irql);
			RtlCopyMemory(&gd,&devExt->stX36,sizeof(HIDGAME_INPUT_DATA));
			if(devExt->auxLibre) {
				devExt->reportLeido=TRUE;
			} else {
				RtlCopyMemory(&devExt->stX36,&devExt->stAux,sizeof(HIDGAME_INPUT_DATA));
				devExt->auxLibre=TRUE;
			}
		KeReleaseSpinLock(&devExt->slGameData,irql);
		RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer,&gd,sizeof(HIDGAME_INPUT_DATA));
		Irp->IoStatus.Information = sizeof(HIDGAME_INPUT_DATA);
	} else {
		status=STATUS_BUFFER_TOO_SMALL;
		Irp->IoStatus.Information = 0;
	}

	Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

HF_IoWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PIO_STACK_LOCATION              irpStack;
    PITFDEVICE_EXTENSION            devExt;
    NTSTATUS                        status = STATUS_SUCCESS;

	PAGED_CODE();

    devExt = (PITFDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    irpStack = IoGetCurrentIrpStackLocation(Irp);
    Irp->IoStatus.Information = 0;

	if(irpStack->Parameters.Write.Length==sizeof(X36KB_INPUT_DATA))
	{
		X36KB_INPUT_DATA hidGameData;
		KIRQL irql;
		RtlCopyMemory(&hidGameData,Irp->AssociatedIrp.SystemBuffer,sizeof(X36KB_INPUT_DATA));
	    Irp->IoStatus.Information = sizeof(X36KB_INPUT_DATA);
		Irp->IoStatus.Status = status;
	    IoCompleteRequest(Irp, IO_NO_INCREMENT);

		ProcesarWrite(devExt->vDevExt,&hidGameData);
	} else {
		status=STATUS_INVALID_BUFFER_SIZE;
		Irp->IoStatus.Status = status;
	    IoCompleteRequest(Irp, IO_NO_INCREMENT);
	}

	return status;
}

VOID EscribirReport(PITFDEVICE_EXTENSION idevExt, PHIDGAME_INPUT_DATA report)
{
	KIRQL irql;
	KeAcquireSpinLock(&idevExt->slGameData,&irql);
		if(idevExt->reportLeido) {
			RtlCopyMemory(&idevExt->stX36,report,sizeof(HIDGAME_INPUT_DATA));
			idevExt->reportLeido=FALSE;
		} else {
			if(idevExt->auxLibre) {
				RtlCopyMemory(&idevExt->stAux,report,sizeof(HIDGAME_INPUT_DATA));
				idevExt->auxLibre=FALSE;
			} else {
				RtlCopyMemory(&idevExt->stX36,&idevExt->stAux,sizeof(HIDGAME_INPUT_DATA));
				RtlCopyMemory(&idevExt->stAux,report,sizeof(HIDGAME_INPUT_DATA));
			}
		}
	KeReleaseSpinLock(&idevExt->slGameData,irql);
}
VOID 
ProcesarWrite(   
    PDEVICE_EXTENSION devExt,
	PX36KB_INPUT_DATA hidGameData
    )
{
	PITFDEVICE_EXTENSION idevExt = (PITFDEVICE_EXTENSION)devExt->itfObj->DeviceExtension;
	HIDGAME_INPUT_DATA report;

	report.Ejes[0]=hidGameData->Ejes[1];
	report.Ejes[1]=hidGameData->Ejes[0];
	report.Botones[0] = hidGameData->Botones[0];
	if(hidGameData->Modos!=0) {
		report.Botones[1] = hidGameData->Modos;
	} else {
		report.Botones[1] = hidGameData->Botones[1];
	}
	report.Botones[1] += hidGameData->Setas[0]<<6;
	report.Setas[0] = hidGameData->Setas[0]>>2;
	report.Setas[0] += hidGameData->Setas[1]<< 2;
	report.Setas[0] += hidGameData->Setas[2]<<6;
	report.Setas[1] = hidGameData->Setas[2]>>2;
	switch(hidGameData->Setas[3]) {
		case 1: 
			report.Setas[1] += 1<<4;
			break;
		case 2:
			report.Setas[1] += 3<<4;
			break;
		case 3: 
			report.Setas[1] += 2<<4;
			break;
		case 4: 
			report.Setas[1] += 5<<4;
			break;
		case 6: 
			report.Setas[1] += 4<<4;
			break;
		case 8: 
			report.Setas[1] += 7<<4;
			break;
		case 9: 
			report.Setas[1] += 8<<4;
			break;
		case 12:
			report.Setas[1] += 6<<4;
	}
	EscribirReport(idevExt,&report);

	if(hidGameData->Modos!=0) {
		report.Botones[1] = hidGameData->Botones[1];
		report.Botones[1] += hidGameData->Setas[0] << 6;
		EscribirReport(idevExt,&report);
	}

}