/*++

Copyright (c) 2007 Alfredo Costalago
Module Name:

    hidfilter.c

Abstract: Filtro para - Human Interface Device (HID) USB driver

Environment:

    Kernel mode

--*/

#include <wdm.h>
#include "control.h"
#include "ioctl.h"

#define _HIDFILTER_
#include "hidfilter.h"
#undef _HIDFILTER_


#ifdef ALLOC_PRAGMA
        #pragma alloc_text(INIT, DriverEntry)
        #pragma alloc_text(PAGE, HF_AddDevice)
        #pragma alloc_text(PAGE, HF_Unload)
        #pragma alloc_text(PAGE, HF_PnP)
#endif

NTSTATUS DriverEntry(
                        IN PDRIVER_OBJECT DriverObject, 
                        IN PUNICODE_STRING RegistryPath
                    )
/*++

Routine Description:

    Installable driver initialization entry point.
    This entry point is called directly by the I/O system.

Arguments:

    DriverObject - pointer to the driver object

    RegistryPath - pointer to a unicode string representing the path,
                   to driver-specific key in the registry.

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    ULONG i;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(RegistryPath);

    /*
     *  Route all IRPs on device objects created by this driver
     *  to our IRP dispatch routine.
     */
    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++){
        DriverObject->MajorFunction[i] = HF_DispatchPassThrough; 
    }
    
    DriverObject->DriverExtension->AddDevice = HF_AddDevice;
    DriverObject->DriverUnload = HF_Unload;


    return STATUS_SUCCESS;
}


NTSTATUS HF_AddDevice(
    IN PDRIVER_OBJECT   Driver,
    IN PDEVICE_OBJECT   PDO
    )
{
    NTSTATUS status = STATUS_SUCCESS;
	PDEVICE_EXTENSION       devExt;
    PDEVICE_OBJECT device = NULL;
    
    PAGED_CODE();
    
    status = IoCreateDevice(    Driver, 
                                sizeof(DEVICE_EXTENSION),
                                NULL,
                                FILE_DEVICE_UNKNOWN, 
                                FILE_AUTOGENERATED_DEVICE_NAME,
                                FALSE,
                                &device);

    if (!NT_SUCCESS(status)){
		return status;
	}

	RtlZeroMemory(device->DeviceExtension, sizeof(DEVICE_EXTENSION));
    devExt = (PDEVICE_EXTENSION) device->DeviceExtension;
	devExt->id=(ULONG)'XUSD';
	devExt->self = PDO;
    devExt->TopOfStack = IoAttachDeviceToDeviceStack(device, PDO);

    if (devExt->TopOfStack == NULL) {
        IoDeleteDevice(device);
        return STATUS_UNSUCCESSFUL; 
    }

	devExt->vdo = NULL;
	devExt->vfo = NULL;
	devExt->irpLectura = NULL;
	devExt->stModos=devExt->stAux=0xff;
	devExt->cerrando =FALSE;
	RtlZeroMemory(&devExt->ultimosDatosOK,sizeof(HID_INPUT_DATA));
	KeInitializeSpinLock(&devExt->slIrpLectura);
	IoInitializeRemoveLock (&devExt->removeLock, 'LRUX',1,0);

	device->Characteristics = devExt->TopOfStack->Characteristics;
    device->Flags |= (devExt->TopOfStack->Flags & (DO_BUFFERED_IO | DO_DIRECT_IO | DO_POWER_INRUSH | DO_POWER_PAGABLE)); 
    device->Flags &= ~DO_DEVICE_INITIALIZING;

	// Registrar interfaz
	{
	    PITFDEVICE_EXTENSION	itfDevExt = NULL;
	    PDEVICE_OBJECT          itfDevice = NULL;
		UNICODE_STRING      DeviceName,link;

		RtlInitUnicodeString(&DeviceName, L"\\Device\\XUsb_HidF");
		RtlInitUnicodeString(&link, L"\\??\\XUSBInterface");

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
			itfDevExt->id=(ULONG)'XUSI';
			itfDevExt->vDevExt = devExt;
			itfDevExt->fecha = 0;

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


VOID HF_Unload(IN PDRIVER_OBJECT Driver)
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
    PDEVICE_EXTENSION devExt;
    PIO_STACK_LOCATION irpSp;
    NTSTATUS status;
    
    devExt = DeviceObject->DeviceExtension;
    irpSp = IoGetCurrentIrpStackLocation(Irp);

	if(devExt->id == (ULONG)'XUSI') {
		return HF_ControlDispatchPassThrough(DeviceObject, Irp);
	} else {
		status = IoAcquireRemoveLock (&devExt->removeLock, Irp);
		if (!NT_SUCCESS (status)) {
			Irp->IoStatus.Status = status;
			IoCompleteRequest (Irp, IO_NO_INCREMENT);
			return status;
		}

		switch (irpSp->MajorFunction){
			case IRP_MJ_INTERNAL_DEVICE_CONTROL:
				{
					BOOLEAN unlock = TRUE;
					status = HF_InternIoCtl(DeviceObject,Irp,&unlock);
					if(unlock) IoReleaseRemoveLock(&devExt->removeLock, Irp); 
				return status;
				}
			case IRP_MJ_PNP:
				return HF_PnP(DeviceObject, Irp); //Cuidado con el removelock

			case IRP_MJ_POWER:
				status = VA_Power(devExt, Irp);
				IoReleaseRemoveLock(&devExt->removeLock, Irp); 
				return status;
 
			default:
			    IoSkipCurrentIrpStackLocation(Irp);
				status = IoCallDriver(devExt->TopOfStack, Irp);
				IoReleaseRemoveLock(&devExt->removeLock, Irp); 
				break;
		}

	    return status;
	}
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
FilterDeviceUsageNotificationCompletionRoutine(
    __in PDEVICE_OBJECT   DeviceObject,
    __in PIRP             Irp,
    __in PVOID            Context
    )
/*++
Routine Description:
    A completion routine for use when calling the lower device objects to
    which our filter deviceobject is attached.

Arguments:

    DeviceObject - Pointer to deviceobject
    Irp          - Pointer to a PnP Irp.
    Context      - NULL
Return Value:

    NT Status is returned.

--*/

{
    PDEVICE_EXTENSION       deviceExtension;

    UNREFERENCED_PARAMETER(Context);

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;


    if (Irp->PendingReturned) {

        IoMarkIrpPending(Irp);
    }

    //
    // On the way up, pagable might become clear. Mimic the driver below us.
    //
    if (!(deviceExtension->TopOfStack->Flags & DO_POWER_PAGABLE)) {

        DeviceObject->Flags &= ~DO_POWER_PAGABLE;
    }

    IoReleaseRemoveLock(&deviceExtension->removeLock, Irp); 

    return STATUS_CONTINUE_COMPLETION;

}

NTSTATUS GetDeviceCapabilities(PDEVICE_EXTENSION devExt)
/*++

Routine Description:

    Function retrieves the DEVICE_CAPABILITIES descriptor from the device

Arguments:

    devExt - device extension for targetted device object

Return Value:

    NT status code

--*/
{
    NTSTATUS status;
    PIRP irp;

    PAGED_CODE();

    irp = IoAllocateIrp(devExt->TopOfStack->StackSize, FALSE);
    if (irp){
		KEVENT event;
        PIO_STACK_LOCATION nextSp = IoGetNextIrpStackLocation(irp);

        // must initialize DeviceCapabilities before sending...
        RtlZeroMemory(  &devExt->deviceCapabilities, 
                        sizeof(DEVICE_CAPABILITIES));
        devExt->deviceCapabilities.Size = sizeof(DEVICE_CAPABILITIES);
        devExt->deviceCapabilities.Version = 1;
        devExt->deviceCapabilities.Address = -1;
        devExt->deviceCapabilities.UINumber = -1;

        // setup irp stack location...
        nextSp->MajorFunction = IRP_MJ_PNP;
        nextSp->MinorFunction = IRP_MN_QUERY_CAPABILITIES;
        nextSp->Parameters.DeviceCapabilities.Capabilities = 
                        &devExt->deviceCapabilities;

        /*
         *  For any IRP you create, you must set the default status
         *  to STATUS_NOT_SUPPORTED before sending it.
         */
        irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

        KeInitializeEvent(&event,
                          NotificationEvent,
                          FALSE
                          );

        IoSetCompletionRoutine(irp,
                               (PIO_COMPLETION_ROUTINE) PnP_Complete, 
                               &event,
                               TRUE,
                               TRUE,
                               TRUE); // No need for Cancel

		
        status = IoCallDriver(devExt->TopOfStack, irp);
        if (STATUS_PENDING == status) {
            KeWaitForSingleObject(
               &event,
               Executive, // Waiting for reason of a driver
               KernelMode, // Waiting in kernel mode
               FALSE, // No allert
               NULL); // No timeout
        }

        IoFreeIrp(irp);
    }
    else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return status;
}

NTSTATUS HF_PnP(IN PDEVICE_OBJECT DeviceObject, PIRP irp)
/*++

Routine Description:

    Dispatch routine for PnP IRPs (MajorFunction == IRP_MJ_PNP)

Arguments:

    devExt - device extension for the targetted device object
    irp - IO Request Packet

Return Value:

    NT status code

--*/
{
    PDEVICE_EXTENSION       devExt; 
	PIO_STACK_LOCATION irpSp;
    NTSTATUS status = STATUS_SUCCESS;
	KEVENT event;

    PAGED_CODE();

    irpSp = IoGetCurrentIrpStackLocation(irp);
    devExt = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

   	
    switch (irpSp->MinorFunction){

    case IRP_MN_START_DEVICE:
        /*
         *  First, send the START_DEVICE irp down the stack
         *  synchronously to start the lower stack.
         *  We cannot do anything with our device object
         *  before propagating the START_DEVICE this way.
         */
        IoCopyCurrentIrpStackLocationToNext(irp);
        KeInitializeEvent(&event,
                          NotificationEvent,
                          FALSE
                          );

        IoSetCompletionRoutine(irp,
                               (PIO_COMPLETION_ROUTINE) PnP_Complete, 
                               &event,
                               TRUE,
                               TRUE,
                               TRUE); // No need for Cancel

		
        status = IoCallDriver(devExt->TopOfStack, irp);
        if (STATUS_PENDING == status) {
            KeWaitForSingleObject(
               &event,
               Executive, // Waiting for reason of a driver
               KernelMode, // Waiting in kernel mode
               FALSE, // No allert
               NULL); // No timeout
			status = irp->IoStatus.Status;
        }

        status = GetDeviceCapabilities(devExt);

		//=========================== Filtro VHID =========================================
		if(NT_SUCCESS(status))
		{
			NTSTATUS st;
			UNICODE_STRING devname;
			RtlInitUnicodeString(&devname,L"\\Device\\XH_VHidF");
			st = IoGetDeviceObjectPointer(&devname,GENERIC_READ|GENERIC_WRITE,&devExt->vfo,&devExt->vdo);
			if(!NT_SUCCESS(st)) { 
				devExt->vfo = NULL;
				devExt->vdo = NULL;
			}
		}
		//================================================================================

		irp->IoStatus.Status = status;
        irp->IoStatus.Information = 0;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
        break;
	
	case IRP_MN_CANCEL_REMOVE_DEVICE:
		{
			KIRQL irql;
			KeAcquireSpinLock(&devExt->slIrpLectura,&irql);
				devExt->cerrando = FALSE;
			KeReleaseSpinLock(&devExt->slIrpLectura,irql);
		}
		IoSkipCurrentIrpStackLocation(irp);
        status = IoCallDriver(devExt->TopOfStack, irp);
		break;
	case IRP_MN_SURPRISE_REMOVAL:
	case IRP_MN_QUERY_REMOVE_DEVICE:
		{
			KIRQL irql;
			PIRP antigua = NULL;
			KeAcquireSpinLock(&devExt->slIrpLectura,&irql);
				devExt->cerrando =TRUE;
				if(devExt->irpLectura != NULL) { antigua = devExt->irpLectura; devExt->irpLectura = NULL; }
			KeReleaseSpinLock(&devExt->slIrpLectura, irql);

			if(antigua != NULL ) {
				if(antigua->Cancel && IoSetCancelRoutine(antigua, NULL)) {
					antigua->IoStatus.Status = STATUS_CANCELLED;
					antigua->IoStatus.Information = 0;
					IoCompleteRequest(antigua,IO_NO_INCREMENT);
				}
			}
		}
		IoSkipCurrentIrpStackLocation(irp);
        status = IoCallDriver(devExt->TopOfStack, irp);

		break;

    case IRP_MN_REMOVE_DEVICE:
		{
		IoReleaseRemoveLockAndWait(&devExt->removeLock, irp);		

        /*
         *  Send the REMOVE IRP down the stack asynchronously.
         *  Do not synchronize sending down the REMOVE_DEVICE
         *  IRP, because the REMOVE_DEVICE IRP must be sent
         *  down and completed all the way back up to the sender
         *  before we continue.
         */
        IoSkipCurrentIrpStackLocation(irp);
        status = IoCallDriver(devExt->TopOfStack, irp);

		// Quitar interfaz de control
		{
			UNICODE_STRING      SymbolicLinkName;
			RtlInitUnicodeString(&SymbolicLinkName, L"\\??\\XUSBInterface");
			IoDeleteSymbolicLink(&SymbolicLinkName);
			IoDeleteDevice(devExt->itfObj);
		}

		//=========================== Filtro VHID =========================================
		if(devExt->vfo != NULL) {
			ObDereferenceObject(devExt->vfo);
			devExt->vfo=NULL;
		}
		//================================================================================

		IoDetachDevice(devExt->TopOfStack);
        IoDeleteDevice(DeviceObject);
		
		return status;
		}
    case IRP_MN_DEVICE_USAGE_NOTIFICATION:

        //
        // On the way down, pagable might become set. Mimic the driver
        // above us. If no one is above us, just set pagable.
        //
        if ((DeviceObject->AttachedDevice == NULL) ||
            (DeviceObject->AttachedDevice->Flags & DO_POWER_PAGABLE)) {

            DeviceObject->Flags |= DO_POWER_PAGABLE;
        }

        IoCopyCurrentIrpStackLocationToNext(irp);

        IoSetCompletionRoutine(
            irp,
            FilterDeviceUsageNotificationCompletionRoutine,
            NULL,
            TRUE,
            TRUE,
            TRUE
            );

        status = IoCallDriver(devExt->TopOfStack, irp);
		break;

	default:
        IoSkipCurrentIrpStackLocation(irp);
        status = IoCallDriver(devExt->TopOfStack, irp);
        break;
    }

	//
    // Pass the IRP down and forget it.
    //
    IoReleaseRemoveLock(&devExt->removeLock, irp); 
    return status;
}

NTSTATUS VA_Power(PDEVICE_EXTENSION devExt, PIRP irp)
/*++

Routine Description:

    Dispatch routine for Power IRPs (MajorFunction == IRP_MJ_Power)


    Note:
        
            This function is left locked down.        

Arguments:

    devExt - device extension for targetted device object
    irp - Io Request Packet

Return Value:

    NT status code

--*/
{
    NTSTATUS status;
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(irp);

	if( (irpSp->MinorFunction==IRP_MN_SET_POWER) && (irpSp->Parameters.Power.Type==DevicePowerState) ) {
		switch (irpSp->Parameters.Power.State.DeviceState) {
			case PowerDeviceD0:
				{
				/*
				 *  Resume from APM Suspend
				 **/
				KIRQL irql;
				KeAcquireSpinLock(&devExt->slIrpLectura,&irql);
					devExt->cerrando = FALSE;
				KeReleaseSpinLock(&devExt->slIrpLectura, irql);
				}
				break;
			case PowerDeviceD1:
			case PowerDeviceD2:
			case PowerDeviceD3:
				{
				/*
				 *  Suspend
				 */
				KIRQL irql;
				PIRP antigua = NULL;
				KeAcquireSpinLock(&devExt->slIrpLectura,&irql);
					if(devExt->irpLectura != NULL) { antigua = devExt->irpLectura; devExt->irpLectura = NULL; }
					devExt->cerrando = TRUE;
				KeReleaseSpinLock(&devExt->slIrpLectura, irql);
				if(antigua != NULL ) {
					if(antigua->Cancel && IoSetCancelRoutine(antigua, NULL)) {
						antigua->IoStatus.Status = STATUS_CANCELLED;
						antigua->IoStatus.Information = 0;
						IoCompleteRequest(antigua,IO_NO_INCREMENT);
					}
				}
				}
		}
	}


	    /*
     *  Send the IRP down the driver stack,
     *  using PoCallDriver (not IoCallDriver, as for non-power irps).
     */

	PoStartNextPowerIrp(irp);
    IoSkipCurrentIrpStackLocation(irp);
    status = PoCallDriver(devExt->TopOfStack, irp);

    return status;
}