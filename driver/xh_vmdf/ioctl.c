#include <wdm.h>
#include <hidport.h>
#include "hidfilter.h"
#include "acciones.h"

#define _IOCTL_
#include "ioctl.h"
#undef _IOCTL_

NTSTATUS
HF_InternIoCtl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PIO_STACK_LOCATION              irpStack;
    NTSTATUS                        status = STATUS_SUCCESS;
    PDEVICE_EXTENSION devExt;
    devExt = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    irpStack = IoGetCurrentIrpStackLocation(Irp);

	if(irpStack->Parameters.DeviceIoControl.IoControlCode==IOCTL_HID_READ_REPORT)
	{
		status = ReadReport(DeviceObject,Irp);
	} else {
		status = STATUS_NOT_SUPPORTED;
	    Irp->IoStatus.Information = 0;
		Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
	}

	return status;
}

NTSTATUS 
ReadReport(
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp
    )
/*++

Routine Description:

    Creates reports and sends it back to the requester.

Arguments:

    DeviceObject - pointer to a device object.

    Irp - Pointer to Interrupt Request Packet.

Return Value:

    NT status code.

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;//STATUS_PENDING;
    PIO_STACK_LOCATION  IrpStack;
    PDEVICE_EXTENSION devExt;
	PREAD_WORKITEM rwi;
	KIRQL irql;

    devExt = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
	IrpStack = IoGetCurrentIrpStackLocation(Irp);

    rwi = ExAllocatePoolWithTag(NonPagedPool, 
                                sizeof(READ_WORKITEM), 
                                (ULONG)'wpHV'
                                );

    if(rwi==NULL) {
        Irp->IoStatus.Status = ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    } else {
		rwi->WorkItem = IoAllocateWorkItem(devExt->self);
		KeAcquireSpinLock(&devExt->slIrpLectura,&irql);
		if((devExt->IrpLectura==NULL) && (rwi->WorkItem!=NULL) ) {
			devExt->IrpLectura = Irp;
			IoMarkIrpPending(Irp);
			KeReleaseSpinLock(&devExt->slIrpLectura, irql);

			rwi->DeviceExtension = devExt;
			IoQueueWorkItem(rwi->WorkItem,WIReadReport,DelayedWorkQueue,rwi);

			ntStatus = STATUS_PENDING;
		} else {
			ExFreePool(rwi);
			KeReleaseSpinLock(&devExt->slIrpLectura, irql);

			Irp->IoStatus.Status = ntStatus = STATUS_CANCELLED;
			Irp->IoStatus.Information = 0;
			IoCompleteRequest(Irp,IO_NO_INCREMENT);
		}
	}

    return ntStatus;
}

VOID WIReadReport(IN PDEVICE_OBJECT DevObj,IN PVOID Context)
{
	PIO_WORKITEM wi = ((PREAD_WORKITEM)Context)->WorkItem;
	PDEVICE_EXTENSION devExt = ((PREAD_WORKITEM)Context)->DeviceExtension;
	HANDLE hHilo = NULL;
	OBJECT_ATTRIBUTES oa;
	PREAD_CONTEXT ctx = NULL;

	ExFreePool(Context);

	PAGED_CODE();

	ctx = ExAllocatePoolWithTag(PagedPool, sizeof(READ_CONTEXT),'npHV');
	if(ctx!=NULL) {
		KeInitializeEvent(&ctx->Ev, NotificationEvent, FALSE);
		ctx->DeviceExtension = devExt;
		InitializeObjectAttributes(&oa, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);

		if(PsCreateSystemThread(&hHilo,THREAD_ALL_ACCESS,&oa,NULL,NULL,DpcRoutineLectura,ctx)==STATUS_SUCCESS) {
			ObReferenceObjectByHandle(hHilo,THREAD_ALL_ACCESS,NULL,KernelMode,&ctx->Hilo,NULL);
			KeSetEvent(&ctx->Ev,0,FALSE);
			ZwClose(hHilo);
		} else {
			KIRQL irql;
			PIRP Irp;
			KeAcquireSpinLock(&devExt->slIrpLectura,&irql);
				Irp = devExt->IrpLectura;
				devExt->IrpLectura = NULL;
			KeReleaseSpinLock(&devExt->slIrpLectura, irql);
			Irp->IoStatus.Status = STATUS_CANCELLED;
			Irp->IoStatus.Information = 0;
			IoCompleteRequest(Irp,IO_NO_INCREMENT);
			ExFreePool(ctx);
		}
	} else {
		KIRQL irql;
		PIRP Irp;
		KeAcquireSpinLock(&devExt->slIrpLectura,&irql);
			Irp = devExt->IrpLectura;
			devExt->IrpLectura = NULL;
		KeReleaseSpinLock(&devExt->slIrpLectura, irql);
		Irp->IoStatus.Status = STATUS_CANCELLED;
		Irp->IoStatus.Information = 0;
		IoCompleteRequest(Irp,IO_NO_INCREMENT);
	}

	IoFreeWorkItem(wi);
}


VOID DpcRoutineLectura(IN PVOID Context)
{
	NTSTATUS			ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION	devExt = ((PREAD_CONTEXT)Context)->DeviceExtension;
    PIO_STACK_LOCATION	IrpStack;
    PIRP                Irp;
	PETHREAD			pHilo;
	LARGE_INTEGER		wait;
	KIRQL				irql, irqlIRP;


	PAGED_CODE();

	KeWaitForSingleObject(&((PREAD_CONTEXT)Context)->Ev,Executive,KernelMode,FALSE,NULL);
	pHilo = ((PREAD_CONTEXT)Context)->Hilo;

	ExFreePoolWithTag(Context,(ULONG)'npHV');
	KeSetPriorityThread(KeGetCurrentThread(), 20 );


	wait = RtlConvertLongToLargeInteger(-10*1000*devExt->TickRaton);
	if( KeWaitForSingleObject(&devExt->evAccion, Executive, KernelMode,	FALSE, &wait)==STATUS_TIMEOUT) {
		KeAcquireSpinLock(&devExt->slIrpLectura,&irqlIRP); //*********** OJO
		if(devExt->IrpLectura==NULL) goto salir;
		Irp = devExt->IrpLectura;
		IrpStack = IoGetCurrentIrpStackLocation(Irp);

		if(IrpStack->Parameters.DeviceIoControl.OutputBufferLength<(sizeof(devExt->stRaton)+1))
		{
			Irp->IoStatus.Status = ntStatus =STATUS_BUFFER_TOO_SMALL;
		} else {
			*((PUCHAR)Irp->UserBuffer)=2;	// Report ID
			KeAcquireSpinLock(&devExt->slRaton,&irql);
				RtlCopyMemory((PUCHAR)Irp->UserBuffer+1,devExt->stRaton, sizeof(devExt->stRaton));
			KeReleaseSpinLock(&devExt->slRaton,irql);
			Irp->IoStatus.Information = sizeof(devExt->stRaton)+1;
			Irp->IoStatus.Status = ntStatus;
		}
	} else {
		KeAcquireSpinLock(&devExt->slIrpLectura,&irqlIRP); //************ OJO
		if(devExt->IrpLectura==NULL) goto salir;
		Irp = devExt->IrpLectura;
		IrpStack = IoGetCurrentIrpStackLocation(Irp);

		if( devExt->TickRatonTimer ) {
			if(IrpStack->Parameters.DeviceIoControl.OutputBufferLength<(sizeof(devExt->stRaton)+1))
			{
				Irp->IoStatus.Status = ntStatus =STATUS_BUFFER_TOO_SMALL;
			} else {
				KIRQL irql1,irql2;

				*((PUCHAR)Irp->UserBuffer)=2;	// Report ID
				KeAcquireSpinLock(&devExt->slListaAcciones,&irql1);
					ProcesarAccionRaton(devExt);
					KeAcquireSpinLock(&devExt->slRaton,&irql2);
						RtlCopyMemory((PUCHAR)Irp->UserBuffer+1,devExt->stRaton, sizeof(devExt->stRaton));
					KeReleaseSpinLock(&devExt->slRaton,irql2);
				KeReleaseSpinLock(&devExt->slListaAcciones,irql1);
				Irp->IoStatus.Information = sizeof(devExt->stRaton)+1;
				Irp->IoStatus.Status = ntStatus;
			}
		} else {
			if(IrpStack->Parameters.DeviceIoControl.OutputBufferLength<(sizeof(devExt->stTeclado)+1))
			{
				ntStatus=STATUS_BUFFER_TOO_SMALL;
			} else {
				*((PUCHAR)Irp->UserBuffer)=1;	// Report ID
				KeAcquireSpinLock(&devExt->slListaAcciones,&irql);
					ProcesarAccionTeclado(devExt);
					RtlCopyMemory((PUCHAR)Irp->UserBuffer+1,devExt->stTeclado, sizeof(devExt->stTeclado));
				KeReleaseSpinLock(&devExt->slListaAcciones,irql);
				Irp->IoStatus.Information =  sizeof(devExt->stTeclado)+1;
				Irp->IoStatus.Status = ntStatus;
			}
		}

		devExt->TickRatonTimer = !devExt->TickRatonTimer;
	}

salir:
	if(	devExt->IrpLectura != NULL) {
		devExt->IrpLectura = NULL;
		KeReleaseSpinLock(&devExt->slIrpLectura, irqlIRP);
	    IoCompleteRequest(Irp, IO_NO_INCREMENT);
	} else {
		KeReleaseSpinLock(&devExt->slIrpLectura, irqlIRP);
	}


	ObDereferenceObject(pHilo);
	PsTerminateSystemThread(0);
}