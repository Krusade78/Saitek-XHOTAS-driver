/*++

Copyright (c) 2005-2007 Alfredo Costalago
Module Name:

    hidfilter.c

Abstract: Filtro para - Human Interface Device (HID) Gameport driver

Environment:

    Kernel mode

--*/

#include <wdm.h>
#include "control.h"

#define _HIDFILTER_
#include "hidfilter.h"
#undef _HIDFILTER_

#ifdef ALLOC_PRAGMA
    #pragma alloc_text( INIT, DriverEntry )
    #pragma alloc_text( PAGE, HF_AddDevice)
    #pragma alloc_text( PAGE, IniciarExtensiones)
    #pragma alloc_text( PAGE, HF_Unload)
    #pragma alloc_text( PAGE, HF_InternIoCtl)
    #pragma alloc_text( PAGE, HF_PnP)
    #pragma alloc_text( PAGE, HF_Power)
#endif /* ALLOC_PRAGMA */


NTSTATUS DriverEntry
    (
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
{
    ULONG i;
	UNREFERENCED_PARAMETER(RegistryPath);

    // 
    // Fill in all the dispatch entry points with the pass through function
    // and the explicitly fill in the functions we are going to intercept
    // 
    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
        DriverObject->MajorFunction[i] = HF_DispatchPassThrough;
    }

    DriverObject->DriverUnload = HF_Unload;
    DriverObject->DriverExtension->AddDevice = HF_AddDevice;

    return STATUS_SUCCESS;
}

NTSTATUS
HF_AddDevice(
    IN PDRIVER_OBJECT   Driver,
    IN PDEVICE_OBJECT   PDO
    )
{
    PDEVICE_EXTENSION       devExt;
    PDEVICE_OBJECT          device;
    NTSTATUS                status = STATUS_SUCCESS;

    PAGED_CODE();

	status = IoCreateDevice(Driver,                   
                            sizeof(DEVICE_EXTENSION), 
                            NULL,                    
                            FILE_DEVICE_UNKNOWN,   
                            0,                     
                            FALSE,                
                            &device              
                            );

    if (!NT_SUCCESS(status)) {
        return (status);
    }

    RtlZeroMemory(device->DeviceExtension, sizeof(DEVICE_EXTENSION));

    devExt = (PDEVICE_EXTENSION) device->DeviceExtension;
	devExt->id=(ULONG)'X36D';
    devExt->TopOfStack = IoAttachDeviceToDeviceStack(device, PDO);

    if (devExt->TopOfStack == NULL) {
        IoDeleteDevice(device);
        return STATUS_DEVICE_NOT_CONNECTED; 
    }

	IoInitializeRemoveLock(&devExt->removeLock,'fwRL',1,0);

    device->Flags |= DO_POWER_PAGABLE;
    device->Flags &= ~DO_DEVICE_INITIALIZING;

	// Registrar interfaz
	{
	    PITFDEVICE_EXTENSION	itfDevExt = NULL;
	    PDEVICE_OBJECT          itfDevice = NULL;
		UNICODE_STRING      DeviceName,link;

		RtlInitUnicodeString(&DeviceName, L"\\Device\\X36_KbWF");
		RtlInitUnicodeString(&link, L"\\??\\X36KbWrapperInterface");

		status = IoCreateDevice(Driver,                   
                            sizeof(ITFDEVICE_EXTENSION), 
                            &DeviceName,                    
                            FILE_DEVICE_UNKNOWN,   
                            0,                     
                            FALSE,                
                            &itfDevice
                            );
		if (NT_SUCCESS( status )) {
			devExt->itfObj=itfDevice;
			itfDevExt = (PITFDEVICE_EXTENSION)devExt->itfObj->DeviceExtension;
			itfDevExt->id=(ULONG)'XKWI';
			itfDevExt->vDevExt = devExt;

			KeInitializeSpinLock(&itfDevExt->slGameData);
			KeInitializeSpinLock(&itfDevExt->slEstado);
			KeInitializeSpinLock(&itfDevExt->slCalibrado);
			itfDevExt->Estado=0;
			RtlZeroMemory(&itfDevExt->stX36,sizeof(HIDGAME_INPUT_DATA));
			RtlZeroMemory(&itfDevExt->calibrado,sizeof(CALIBRADO)*4);
			itfDevExt->auxLibre=TRUE;
			itfDevExt->reportLeido=TRUE;

			status = IoCreateSymbolicLink( &link, &DeviceName );
			if ( !NT_SUCCESS( status )) {
				IoDeleteDevice(devExt->itfObj);
				IoDeleteDevice(device);
				return status;
	        }

			devExt->itfObj->Flags |= DO_BUFFERED_IO;
			devExt->itfObj->Flags &= ~DO_DEVICE_INITIALIZING;
		} else {
			IoDeleteDevice(device);
		}
	}

    return status;
}

VOID 
HF_Unload(
   IN PDRIVER_OBJECT Driver
   )
/*++

Routine Description:

   Free all the allocated resources associated with this driver.

Arguments:

   DriverObject - Pointer to the driver object.

Return Value:

   None.

--*/

{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(Driver);

    ASSERT(NULL == Driver->DeviceObject);
}



NTSTATUS
HF_DispatchPassThrough(
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
        )
/*++
Routine Description:

    Passes a request on to the lower driver.     
--*/
{
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
	PDEVICE_EXTENSION devExt = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
	NTSTATUS status;

	if(devExt->id == (ULONG)'XKWI') {
		return HF_ControlDispatchPassThrough(DeviceObject, Irp);
	} else {
		status = IoAcquireRemoveLock(&devExt->removeLock,Irp);
		if (!NT_SUCCESS (status)) {
			Irp->IoStatus.Status = status;
			IoCompleteRequest (Irp, IO_NO_INCREMENT);
			return status;
		}
		switch(irpStack->MajorFunction) {
			case IRP_MJ_INTERNAL_DEVICE_CONTROL:
				status = HF_InternIoCtl(DeviceObject,Irp);
				IoReleaseRemoveLock(&devExt->removeLock, Irp); 
				return status;
			case IRP_MJ_PNP:
				return HF_PnP(DeviceObject,Irp); // Cuidado con el removelock
			case IRP_MJ_POWER:
				status = HF_Power(DeviceObject,Irp);
				IoReleaseRemoveLock(&devExt->removeLock, Irp); 
				return status;
			default:
			    IoSkipCurrentIrpStackLocation(Irp);
			    status = IoCallDriver(devExt->TopOfStack, Irp);
				IoReleaseRemoveLock(&devExt->removeLock, Irp); 
				return status;
		}
	}
}           

NTSTATUS
HF_InternIoCtl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PIO_STACK_LOCATION              irpStack;
    NTSTATUS                        status = STATUS_SUCCESS;
    PDEVICE_EXTENSION devExt;

	PAGED_CODE();

    devExt = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    Irp->IoStatus.Information = 0;
	Irp->IoStatus.Status = status;
	status = STATUS_NOT_SUPPORTED;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return status;
}

//IO_COMPLETION_ROUTINE PnP_Complete;
NTSTATUS
PnP_Complete(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    )
/*++
Routine Description:

    Generic completion routine that allows the driver to send the irp down the 
    stack, catch it on the way up, and do more processing at the original IRQL.
    
--*/
{
    PKEVENT  event = (PKEVENT) Context;

    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);

    KeSetEvent(event, 0, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
HF_PnP(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine is the dispatch routine for plug and play irps 

Arguments:

    DeviceObject - Pointer to the device object.

    Irp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/
{
    PDEVICE_EXTENSION       devExt; 
    PIO_STACK_LOCATION      irpStack;
    NTSTATUS                status = STATUS_SUCCESS;
	KIRQL					Old;
	PIRP					pIrp;
	KEVENT                  event; 

    PAGED_CODE();


    devExt = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    irpStack = IoGetCurrentIrpStackLocation(Irp);

    switch (irpStack->MinorFunction) {
	case IRP_MN_START_DEVICE:
    {
        IoCopyCurrentIrpStackLocationToNext(Irp);
        KeInitializeEvent(&event,
                          NotificationEvent,
                          FALSE
                          );

        IoSetCompletionRoutine(Irp,
                               (PIO_COMPLETION_ROUTINE) PnP_Complete, 
                               &event,
                               TRUE,
                               TRUE,
                               TRUE); // No need for Cancel

        status = IoCallDriver(devExt->TopOfStack, Irp);

        if (STATUS_PENDING == status) {
            KeWaitForSingleObject(
               &event,
               Executive, // Waiting for reason of a driver
               KernelMode, // Waiting in kernel mode
               FALSE, // No allert
               NULL); // No timeout
           status = Irp->IoStatus.Status;
        }

		Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        break;
    }
	case IRP_MN_SURPRISE_REMOVAL:
    case IRP_MN_REMOVE_DEVICE:
	{
		//PITFDEVICE_EXTENSION idevExt = (PITFDEVICE_EXTENSION) devExt->itfObj->DeviceExtension;

		IoReleaseRemoveLockAndWait(&devExt->removeLock, Irp);

        IoSkipCurrentIrpStackLocation(Irp);
        status = IoCallDriver(devExt->TopOfStack, Irp);

		// Quitar interfaz de control
		{
			UNICODE_STRING      SymbolicLinkName;
			RtlInitUnicodeString(&SymbolicLinkName, L"\\??\\X36KbWrapperInterface");
			IoDeleteSymbolicLink(&SymbolicLinkName);
			IoDeleteDevice(devExt->itfObj);
		}

		IoDetachDevice(devExt->TopOfStack); 
		IoDeleteDevice(DeviceObject);
		return status;
    }
	default:
        IoSkipCurrentIrpStackLocation(Irp);
        status = IoCallDriver(devExt->TopOfStack, Irp);
        break;
    }

    IoReleaseRemoveLock(&devExt->removeLock, Irp); 
    return status;
}

NTSTATUS
HF_Power(
    IN PDEVICE_OBJECT    DeviceObject,
    IN PIRP              Irp
    )
{
    PDEVICE_EXTENSION   devExt = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    PAGED_CODE();

	PoStartNextPowerIrp(Irp);
    IoSkipCurrentIrpStackLocation(Irp);
    return PoCallDriver(devExt->TopOfStack, Irp);
}
