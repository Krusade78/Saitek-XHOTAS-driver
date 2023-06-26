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
	    Irp->IoStatus.Information = 0;
		Irp->IoStatus.Status = status;
		status = STATUS_NOT_SUPPORTED;
	}

	if(status != STATUS_PENDING)
    {
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
    NTSTATUS ntStatus = STATUS_PENDING;
    PIO_STACK_LOCATION  IrpStack;
    PDEVICE_EXTENSION devExt;
    PREAD_CONTEXT    ctx;

    devExt = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
	IrpStack = IoGetCurrentIrpStackLocation(Irp);

    ctx = ExAllocatePoolWithTag(NonPagedPool, 
                                sizeof(READ_CONTEXT), 
                                (ULONG)'npHV'
                                );

    if(!ctx) {
        Irp->IoStatus.Status = ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    } else {
		KIRQL irql;

        RtlZeroMemory(ctx, sizeof(READ_CONTEXT));

		IoMarkIrpPending(Irp);

        ctx->Irp = Irp;
		ctx->DeviceObject=DeviceObject;
		KeInitializeDpc(&ctx->ReadTimerDpc,
				            DpcRoutineLectura,
					        (PVOID)ctx
						    );

		if(KeGetCurrentIrql()<=APC_LEVEL)
		{
			LARGE_INTEGER timeout=RtlConvertLongToLargeInteger(-10*1000*500);
			KeWaitForSingleObject(&devExt->evAccion,
								Executive,
								KernelMode,
								FALSE,
								&timeout
								);
		}		

        KeInitializeTimer(&ctx->ReadTimer);

		KeAcquireSpinLock(&devExt->slDPCsActivos,&irql);
			devExt->DPCsActivos++;
		KeReleaseSpinLock(&devExt->slDPCsActivos,irql);
        KeSetTimer(&ctx->ReadTimer,
                   RtlConvertLongToLargeInteger(-10*1000),
                   &ctx->ReadTimerDpc
                   );
	}

    return ntStatus;
}

VOID 
DpcRoutineLectura(   
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )
{
	NTSTATUS status=STATUS_SUCCESS;
    PIO_STACK_LOCATION	IrpStack;
	PDEVICE_EXTENSION   devExt;
    PIRP                Irp;
	PREAD_CONTEXT       rctx;
	LARGE_INTEGER		tiempo;

	rctx=(PREAD_CONTEXT)DeferredContext;
	Irp = rctx->Irp;
	devExt = (PDEVICE_EXTENSION) rctx->DeviceObject->DeviceExtension;
	ExFreePoolWithTag(DeferredContext,(ULONG)'npHV');

	IrpStack = IoGetCurrentIrpStackLocation(Irp);

	if(((devExt->TickRatonTimer++)>=devExt->TickRaton) || IrpStack->Parameters.DeviceIoControl.OutputBufferLength==5) {
		if(IrpStack->Parameters.DeviceIoControl.OutputBufferLength<(sizeof(devExt->stRaton)+1))
		{
		    status=STATUS_BUFFER_TOO_SMALL;
		} else {
			devExt->TickRatonTimer=0;
			*((PUCHAR)Irp->UserBuffer)=2;	// Report ID
			KeAcquireSpinLockAtDpcLevel(&devExt->slListaAcciones);
				ProcesarAccionRaton(devExt);
				KeAcquireSpinLockAtDpcLevel(&devExt->slRaton);
					RtlCopyMemory((PUCHAR)Irp->UserBuffer+1,devExt->stRaton, sizeof(devExt->stRaton));
				KeReleaseSpinLockFromDpcLevel(&devExt->slRaton);
			KeReleaseSpinLockFromDpcLevel(&devExt->slListaAcciones);
			Irp->IoStatus.Information = sizeof(devExt->stRaton)+1;
		}
	} else {
		if(IrpStack->Parameters.DeviceIoControl.OutputBufferLength<(sizeof(devExt->stTeclado)+1))
		{
		    status=STATUS_BUFFER_TOO_SMALL;
		} else {
			*((PUCHAR)Irp->UserBuffer)=1;	// Report ID
			KeAcquireSpinLockAtDpcLevel(&devExt->slListaAcciones);
				ProcesarAccionTeclado(devExt);
				RtlCopyMemory((PUCHAR)Irp->UserBuffer+1,devExt->stTeclado, sizeof(devExt->stTeclado));
			KeReleaseSpinLockFromDpcLevel(&devExt->slListaAcciones);
			Irp->IoStatus.Information =  sizeof(devExt->stTeclado)+1;
		}
	}

    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

	KeAcquireSpinLockAtDpcLevel(&devExt->slDPCsActivos);
		devExt->DPCsActivos--;
	KeReleaseSpinLockFromDpcLevel(&devExt->slDPCsActivos);
}