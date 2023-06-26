/*++

Copyright (c) 2007 Alfredo Costalago
Module Name:

    vhidmini.c

Environment:

    Kernel mode

--*/

#include <wdm.h>
#include <hidport.h>
#include "ioctl.h"

#define _VHIDMINI_
#include "VHidMini.h"
#undef _VHIDMINI_

#ifdef ALLOC_PRAGMA
    #pragma alloc_text( INIT, DriverEntry )
    #pragma alloc_text( PAGE, AddDevice)
    #pragma alloc_text( PAGE, Unload)
    #pragma alloc_text( PAGE, PnP)
#endif 


NTSTATUS 
DriverEntry (
    IN PDRIVER_OBJECT  DriverObject,
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
    STATUS_UNSUCCESSFUL otherwise.

--*/
{
    NTSTATUS                      ntStatus;
    HID_MINIDRIVER_REGISTRATION   hidMinidriverRegistration;

    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] =
                                                InternalIoctl;
    DriverObject->MajorFunction[IRP_MJ_PNP]   = PnP;
    DriverObject->MajorFunction[IRP_MJ_POWER] = Power;
    DriverObject->DriverUnload                = Unload;
    DriverObject->DriverExtension->AddDevice  = AddDevice;

    RtlZeroMemory(&hidMinidriverRegistration, 
                  sizeof(hidMinidriverRegistration));

    //
    // Revision must be set to HID_REVISION by the minidriver
    //
    
    hidMinidriverRegistration.Revision            = HID_REVISION;
    hidMinidriverRegistration.DriverObject        = DriverObject;
    hidMinidriverRegistration.RegistryPath        = RegistryPath;
    hidMinidriverRegistration.DeviceExtensionSize = sizeof(DEVICE_EXTENSION); 
    hidMinidriverRegistration.DevicesArePolled    = FALSE; 

    //
    //Register with hidclass
    //

    ntStatus = HidRegisterMinidriver(&hidMinidriverRegistration);

    return ntStatus;   
}


NTSTATUS  
AddDevice (
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT FunctionalDeviceObject
    )
/*++

Routine Description:

    HidClass Driver calls our AddDevice routine after creating an FDO for us.
    We do not need to create a device object or attach it to the PDO. 
    Hidclass driver will do it for us.
   
Arguments:

    DriverObject - pointer to the driver object.

    FunctionalDeviceObject -  pointer to the FDO created by the
                            Hidclass driver for us.

Return Value:

    NT status code.

--*/
{
    NTSTATUS            ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceInfo;

    PAGED_CODE ();

    deviceInfo = GET_MINIDRIVER_DEVICE_EXTENSION (FunctionalDeviceObject);

    //
    // Initialize all the members of device extension
    //

    RtlZeroMemory((PVOID)deviceInfo, sizeof(DEVICE_EXTENSION));

    //
    // Set the initial state of the FDO
    //

    deviceInfo->DevicePnPState =  NotStarted;
    deviceInfo->PreviousPnPState = NotStarted;
    FunctionalDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
    
    return ntStatus;
    
}


NTSTATUS  
PnP (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++
Routine Description:

    Handles PnP Irps sent to FDO .

Arguments:

    DeviceObject - Pointer to deviceobject
    Irp          - Pointer to a PnP Irp.
    
Return Value:

    NT Status is returned.
--*/
{
    NTSTATUS            ntStatus, queryStatus;
    PDEVICE_EXTENSION   deviceInfo;
    KEVENT              startEvent;
    PIO_STACK_LOCATION  IrpStack, previousSp;
    PWCHAR              buffer;

    PAGED_CODE();

    deviceInfo = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);
    IrpStack = IoGetCurrentIrpStackLocation (Irp);

    switch(IrpStack->MinorFunction)
    {
    case IRP_MN_START_DEVICE:

        KeInitializeEvent(&startEvent, NotificationEvent, FALSE);

        IoCopyCurrentIrpStackLocationToNext (Irp);

        IoSetCompletionRoutine (Irp, 
                                PnPComplete, 
                                &startEvent, 
                                TRUE, 
                                TRUE, 
                                TRUE);

        ntStatus = IoCallDriver (GET_NEXT_DEVICE_OBJECT(DeviceObject), Irp);

        if (STATUS_PENDING == ntStatus) {
            KeWaitForSingleObject(
                    		&startEvent,
                    		Executive, 
                    		KernelMode, 
                    		FALSE, // No allert
                    		NULL); // No timeout
            ntStatus = Irp->IoStatus.Status;
        }

        if (NT_SUCCESS(ntStatus)) {
            //
            //Set new PnP state
            //
            if(NT_SUCCESS(ntStatus)){
				deviceInfo->PreviousPnPState =  deviceInfo->DevicePnPState;
				deviceInfo->DevicePnPState = (Started);
            }
        }

        Irp->IoStatus.Status = ntStatus;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        return ntStatus;

    case IRP_MN_STOP_DEVICE:
		deviceInfo->PreviousPnPState =  deviceInfo->DevicePnPState;
		deviceInfo->DevicePnPState = (Stopped);
        ntStatus = STATUS_SUCCESS;
        break;

    case IRP_MN_CANCEL_STOP_DEVICE:
		deviceInfo->DevicePnPState = deviceInfo->PreviousPnPState;
        ntStatus = STATUS_SUCCESS;         
        break;

    case IRP_MN_QUERY_STOP_DEVICE:    
		deviceInfo->PreviousPnPState =  deviceInfo->DevicePnPState;
		deviceInfo->DevicePnPState = (StopPending);
        ntStatus = STATUS_SUCCESS;
        break;

    case IRP_MN_QUERY_REMOVE_DEVICE:
		deviceInfo->PreviousPnPState =  deviceInfo->DevicePnPState;
		deviceInfo->DevicePnPState = (RemovePending);
        ntStatus = STATUS_SUCCESS;
        break;

    case IRP_MN_CANCEL_REMOVE_DEVICE:
		deviceInfo->DevicePnPState = deviceInfo->PreviousPnPState;
        ntStatus = STATUS_SUCCESS;
        break;

    case IRP_MN_SURPRISE_REMOVAL:
		deviceInfo->PreviousPnPState =  deviceInfo->DevicePnPState;
		deviceInfo->DevicePnPState = (SurpriseRemovePending);
        ntStatus = STATUS_SUCCESS;
        break;

    case IRP_MN_REMOVE_DEVICE:
		deviceInfo->PreviousPnPState =  deviceInfo->DevicePnPState;
		deviceInfo->DevicePnPState = (Deleted);    
        ntStatus = STATUS_SUCCESS;           
        break;
        
    case IRP_MN_QUERY_ID:

        //
        // This check is required to filter out QUERY_IDs forwarded
        // by the HIDCLASS for the parent FDO. These IDs are sent
        // by PNP manager for the parent FDO if you root-enumerate this driver.
        //
        previousSp = ((PIO_STACK_LOCATION) ((UCHAR *) (IrpStack) + 
                            sizeof(IO_STACK_LOCATION)));
        
        if(previousSp->DeviceObject == DeviceObject) {
            //
            // Filtering out this basically prevents the Found New Hardware 
            // popup for the root-enumerated VHIDMINI on reboot.
            // 
            ntStatus = Irp->IoStatus.Status;            
            break;
        }
       
        switch (IrpStack->Parameters.QueryId.IdType) 
        {
        case BusQueryDeviceID:
        case BusQueryHardwareIDs:
#define VHID_HARDWARE_IDS    L"HID\\NullX36WrapperHIDDevice\0\0"
#define VHID_HARDWARE_IDS_LENGTH sizeof (VHID_HARDWARE_IDS)
			//
            // HIDClass is asking for child deviceid & hardwareids.
            // Let us just make up some  id for our child device.
            //
            buffer = (PWCHAR)ExAllocatePoolWithTag(
                          NonPagedPool, 
                          VHID_HARDWARE_IDS_LENGTH, 
                          (ULONG)'wiHV'
                          );

            if (buffer) {
                //
                // Do the copy, store the buffer in the Irp
                //
                RtlCopyMemory(buffer, 
                              VHID_HARDWARE_IDS, 
                              VHID_HARDWARE_IDS_LENGTH
                              );
                
                Irp->IoStatus.Information = (ULONG_PTR)buffer;
                ntStatus = STATUS_SUCCESS;
            } 
            else {
                //
                //  No memory
                //
                ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            }

            Irp->IoStatus.Status = ntStatus;
            //
            // We don't need to forward this to our bus. This query 
            // is for our child so we should complete it right here. 
            // fallthru.
            //
            IoCompleteRequest (Irp, IO_NO_INCREMENT);     
            
            return ntStatus;           
#undef VHID_HARDWARE_IDS
#undef VHID_HARDWARE_IDS_LENGTH
                 
        default:            
            ntStatus = Irp->IoStatus.Status;
            IoCompleteRequest (Irp, IO_NO_INCREMENT);          
            return ntStatus;
        }

    default:         
        ntStatus = Irp->IoStatus.Status;
        break;
    }
    
    Irp->IoStatus.Status = ntStatus;
    IoSkipCurrentIrpStackLocation (Irp);
    return IoCallDriver(GET_NEXT_DEVICE_OBJECT(DeviceObject), Irp);
    
}


NTSTATUS 
PnPComplete (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    )
{
    UNREFERENCED_PARAMETER (DeviceObject);

    if (Irp->PendingReturned) 
    {
        KeSetEvent ((PKEVENT) Context, 0, FALSE);
    }
    return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
Power(
    IN PDEVICE_OBJECT    DeviceObject,
    IN PIRP              Irp
    )
/*++

Routine Description:

    This routine is the dispatch routine for power irps.
    Does nothing except forwarding the IRP to the next device
    in the stack.

Arguments:

    DeviceObject - Pointer to the device object.

    Irp - Pointer to the request packet.

Return Value:

    NT Status code
--*/
{
    PDEVICE_EXTENSION   deviceInfo;
   
    deviceInfo = GET_MINIDRIVER_DEVICE_EXTENSION (DeviceObject);

    PoStartNextPowerIrp(Irp);     
    IoSkipCurrentIrpStackLocation(Irp);

    return PoCallDriver(GET_NEXT_DEVICE_OBJECT (DeviceObject), Irp);
}


VOID
Unload(
    IN PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:

    Free all the allocated resources, etc.

Arguments:

    DriverObject - pointer to a driver object.

Return Value:

    VOID.

--*/
{
    PAGED_CODE ();

    ASSERT(DriverObject->DeviceObject == NULL);

    return;
}
