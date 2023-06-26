/*++

Copyright (c) 2005-2007 Alfredo Costalago

Module Name:

    pnp.c

Abstract: This module contains PnP Start, Stop, Remove,
          Power dispatch routines and IRP cancel routine.

Environment:

    Kernel mode

--*/

#include <wdm.h>
#include <hidport.h>
#include "hidgame.h"
#include "pnp_joyinit.h"
#include "request.h"

#define _PNPC_
#include "pnp.h"

#ifdef ALLOC_PRAGMA
    #pragma alloc_text (PAGE, HGM_RemoveDevice)
    #pragma alloc_text (PAGE, HGM_PnP)
    #pragma alloc_text (PAGE, HGM_InitDevice)
    #pragma alloc_text (PAGE, HGM_GetResources)
#endif

/*****************************************************************************
 *
 *  @func   NTSTATUS  | HGM_PnP |
 *
 *          Plug and Play dispatch routine for this driver.
 *
 *  @parm   IN PDEVICE_OBJECT | DeviceObject |
 *
 *          Pointer to the device object.
 *
 *  @parm   IN PIRP | Irp |
 *
 *          Pointer to an I/O request packet.
 *
 *  @rvalue   STATUS_SUCCESS | success
 *  @rvalue   STATUS_DELETE_PENDING | PnP IRP received after device was removed
 *  @rvalue   ???   | Return from IoCallDriver() or HGM_InitDevice()
 *
 *****************************************************************************/
NTSTATUS HGM_PnP
    (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    NTSTATUS ntStatus;
    PDEVICE_EXTENSION DeviceExtension;
    KEVENT            StartEvent;

    PAGED_CODE();

    /*
     * Get a pointer to the device extension
     */
    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);

    ntStatus = HGM_IncRequestCount( DeviceExtension );
    if (!NT_SUCCESS (ntStatus))
    {
        /*
         * Someone sent us another plug and play IRP after removed
         */

        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = ntStatus;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
    } else
    {
        PIO_STACK_LOCATION IrpStack;

        /*
         * Get a pointer to the current location in the Irp
         */
        IrpStack = IoGetCurrentIrpStackLocation (Irp);

        switch(IrpStack->MinorFunction)
        {
            case IRP_MN_START_DEVICE:

                /*
                 * We cannot touch the device (send it any non pnp irps) until a
                 * start device has been passed down to the lower drivers.
                 */
                KeInitializeEvent(&StartEvent, NotificationEvent, FALSE);

                IoCopyCurrentIrpStackLocationToNext (Irp);
                IoSetCompletionRoutine (Irp, HGM_PnPComplete, &StartEvent, TRUE, TRUE, TRUE);
                ntStatus = IoCallDriver (GET_NEXT_DEVICE_OBJECT(DeviceObject), Irp);

                if( NT_SUCCESS(ntStatus ) )
                {
                    ntStatus = KeWaitForSingleObject
                               (
                               &StartEvent,
                               Executive,   /* Waiting for reason of a driver */
                               KernelMode,  /* Waiting in kernel mode         */
                               FALSE,       /* No allert                      */
                               NULL         /* No timeout                     */
                               );
                }

                if(NT_SUCCESS(ntStatus))
                {
                    ntStatus = Irp->IoStatus.Status;
                }

                if(NT_SUCCESS (ntStatus))
                {
                    /*
                     * As we are now back from our start device we can do work.
                     */
	 				ntStatus = HGM_InitDevice (DeviceObject, Irp);
                }


                DeviceExtension->fStarted = TRUE;

                /*
                 *      Return Status
                 */
                Irp->IoStatus.Information = 0;
                Irp->IoStatus.Status = ntStatus;
                IoCompleteRequest (Irp, IO_NO_INCREMENT);

                break;

            case IRP_MN_STOP_DEVICE:

                /*
                 * After the start IRP has been sent to the lower driver object, the bus may
                 * NOT send any more IRPS down ``touch'' until another START has occured.
                 * Whatever access is required must be done before Irp passed on.
                 */

                DeviceExtension->fStarted = FALSE;

                IoSkipCurrentIrpStackLocation (Irp);
                ntStatus = IoCallDriver (GET_NEXT_DEVICE_OBJECT(DeviceObject), Irp);
                break;

            case IRP_MN_SURPRISE_REMOVAL:

                HGM_RemoveDevice(DeviceExtension);

                Irp->IoStatus.Status = STATUS_SUCCESS;
                IoSkipCurrentIrpStackLocation(Irp);
                ntStatus = IoCallDriver (GET_NEXT_DEVICE_OBJECT(DeviceObject), Irp);

                break;

            case IRP_MN_REMOVE_DEVICE:

                DeviceExtension->fRemoved = TRUE;
                HGM_RemoveDevice(DeviceExtension);

                /*
                 * Send on the remove IRP
                 */
                IoSkipCurrentIrpStackLocation (Irp);
                ntStatus = IoCallDriver (GET_NEXT_DEVICE_OBJECT(DeviceObject), Irp);

                /*
                 *  Remove this IRPs hold which should leave the initial 1 plus 
                 *  any other IRP holds.
                 */
                {
                    LONG RequestCount = InterlockedDecrement( &DeviceExtension->RequestCount );
                    ASSERT( RequestCount > 0 );
                }

                /*
                 *  If someone has already started, wait for them to finish
                 */
                if( InterlockedDecrement( &DeviceExtension->RequestCount ) > 0 )
                {
                    KeWaitForSingleObject( &DeviceExtension->RemoveEvent,
                        Executive, KernelMode, FALSE, NULL );
                }

                ntStatus = STATUS_SUCCESS;

                return ntStatus;

            default:

                IoSkipCurrentIrpStackLocation (Irp);

                ntStatus = IoCallDriver(GET_NEXT_DEVICE_OBJECT(DeviceObject), Irp);
                break;
        }

        HGM_DecRequestCount( DeviceExtension );
    }

    return ntStatus;
} /* HGM_PnP */

/*****************************************************************************
 *
 *  @func   VOID  | HGM_RemoveDevice |
 *
 *          FDO Remove routine 
 *
 *  @parm   IN PDEVICE_EXTENSION | DeviceExtension |
 *
 *          Pointer to the device extension.
 *
 *****************************************************************************/
VOID HGM_RemoveDevice
    (
    PDEVICE_EXTENSION DeviceExtension
    )
{
    if (DeviceExtension->fSurpriseRemoved) {
        return;
    }

    DeviceExtension->fSurpriseRemoved = TRUE;

//=========================== Filtro Wrapper =========================================
		if(DeviceExtension->wfo != NULL) {
			ObDereferenceObject(DeviceExtension->wfo);
			DeviceExtension->wfo=NULL;
		}
//================================================================================
//=========================== Filtro VHID =========================================
		if(DeviceExtension->vfo != NULL) {
			ObDereferenceObject(DeviceExtension->vfo);
			DeviceExtension->vfo=NULL;
		}
//================================================================================

    /*
     *  Acquire mutex before modifying the Global Linked list of devices
     */
    ExAcquireFastMutex (&Global.Mutex);
    
    /*
     * Remove this device from the linked list of devices
     */
    RemoveEntryList(&DeviceExtension->Link);
    
    /*
     *  Release the mutex
     */
    ExReleaseFastMutex (&Global.Mutex);
} /* HGM_RemoveDevice */

/*****************************************************************************
 *
 *  @func   NTSTATUS  | HGM_InitDevice |
 *
 *          Get the device information and attempt to initialize a configuration
 *          for a device.  If we cannot identify this as a valid HID device or
 *          configure the device, our start device function is failed.
 *
 *  @parm   IN PDEVICE_OBJECT | DeviceObject |
 *
 *          Pointer to the device object.
 *
 *  @parm   IN PIRP | Irp |
 *
 *          Pointer to an I/O request packet.
 *
 *  @rvalue   STATUS_SUCCESS | success
 *  @rvalue   STATUS_DEVICE_CONFIGURATION_ERROR | Resources overlap
 *  @rvalue   ???            | Return from HGM_GetResources() or HGM_JoystickConfig()
 *
 *
 *****************************************************************************/
NTSTATUS HGM_InitDevice
    (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    NTSTATUS ntStatus;
    PDEVICE_EXTENSION   DeviceExtension;
    ULONG  DescriptorLength;

    PAGED_CODE();

    /*
     * Get a pointer to the device extension
     */
    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);

    /*
     * Get resource information from GameEnum and store it in the device extension
     */
    ntStatus = HGM_GetResources(DeviceObject,Irp);
    if( NT_SUCCESS(ntStatus) )
    {
		ntStatus = HGM_InitAnalog(DeviceObject);
    }

//=========================== Filtro Wrapper =========================================
	if(NT_SUCCESS(ntStatus))
	{
		NTSTATUS st;
		UNICODE_STRING devname;
		RtlInitUnicodeString(&devname,L"\\Device\\X36_KbWF");
		st = IoGetDeviceObjectPointer(&devname,GENERIC_READ|GENERIC_WRITE,&DeviceExtension->wfo,&DeviceExtension->wdo);
		if(!NT_SUCCESS(st)) { 
			DeviceExtension->wfo = NULL;
			DeviceExtension->wdo = NULL;
		}
	}
//================================================================================
//=========================== Filtro VHID =========================================
	if(NT_SUCCESS(ntStatus))
	{
		NTSTATUS st;
		UNICODE_STRING devname;
		RtlInitUnicodeString(&devname,L"\\Device\\XH_VHidF");
		st = IoGetDeviceObjectPointer(&devname,GENERIC_READ|GENERIC_WRITE,&DeviceExtension->vfo,&DeviceExtension->vdo);
		if(!NT_SUCCESS(st)) { 
			DeviceExtension->vfo = NULL;
			DeviceExtension->vdo = NULL;
		}
	}
//================================================================================

    if( !NT_SUCCESS(ntStatus) )
    {
        /*
         *  Acquire mutex before modifying the Global Linked list of devices
         */
        ExAcquireFastMutex (&Global.Mutex);

        /*
         * Remove this device from the linked list of devices
         */
        RemoveEntryList(&DeviceExtension->Link);

        /*
         *  Release the mutex
         */
        ExReleaseFastMutex (&Global.Mutex);
    }

    return ntStatus;
} /* HGM_InitDevice */



/*****************************************************************************
 *
 *  @func   NTSTATUS  | HGM_GetResources |
 *
 *          Gets gameport resource information from the GameEnum driver
 *
 *  @parm   IN PDEVICE_OBJECT | DeviceObject |
 *
 *          Pointer to the device object.
 *
 *  @parm   IN PIRP | Irp |
 *
 *          Pointer to an I/O request packet.
 *
 *  @rvalue   STATUS_SUCCESS | success
 *  @rvalue   ???            | Return from IoCallDriver()
 *
 *****************************************************************************/
NTSTATUS HGM_GetResources
    (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    NTSTATUS            ntStatus = STATUS_SUCCESS;
    GAMEENUM_PORT_PARAMETERS    PortInfo;
    PDEVICE_EXTENSION   DeviceExtension;
    KEVENT              IoctlCompleteEvent;
    IO_STATUS_BLOCK     IoStatus;
    PIO_STACK_LOCATION  irpStack, nextStack;

    PAGED_CODE ();

    /*
     * Get a pointer to the device extension
     */

    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);


    /*
     * issue a synchronous request to get the resources info from GameEnum
     */

    KeInitializeEvent(&IoctlCompleteEvent, NotificationEvent, FALSE);

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    nextStack = IoGetNextIrpStackLocation(Irp);

    /*
     * pass the Portinfo buffer of the DeviceExtension
     */

    nextStack->MajorFunction                                    =
        IRP_MJ_INTERNAL_DEVICE_CONTROL;

    nextStack->Parameters.DeviceIoControl.IoControlCode         =
        IOCTL_GAMEENUM_PORT_PARAMETERS;

    PortInfo.Size                                                   =
        nextStack->Parameters.DeviceIoControl.InputBufferLength     =
        nextStack->Parameters.DeviceIoControl.OutputBufferLength    =
        sizeof (PortInfo);

    Irp->UserBuffer =   &PortInfo;

    IoSetCompletionRoutine (Irp, HGM_PnPComplete,
                            &IoctlCompleteEvent, TRUE, TRUE, TRUE);

    ntStatus = IoCallDriver(GET_NEXT_DEVICE_OBJECT (DeviceObject), Irp);

    if( NT_SUCCESS(ntStatus) )
    {
        ntStatus = KeWaitForSingleObject(
                                        &IoctlCompleteEvent,
                                        Executive,
                                        KernelMode,
                                        FALSE,
                                        NULL);

    }

    if( NT_SUCCESS(ntStatus) )
    {
        ntStatus = Irp->IoStatus.Status;
    }
    DeviceExtension->GameContext        = PortInfo.GameContext;
    DeviceExtension->ReadAccessor       = PortInfo.ReadAccessor;
    DeviceExtension->WriteAccessor      = PortInfo.WriteAccessor;
    DeviceExtension->AcquirePort        = PortInfo.AcquirePort;
    DeviceExtension->ReleasePort        = PortInfo.ReleasePort;
    DeviceExtension->PortContext        = PortInfo.PortContext;
    DeviceExtension->nAxes		        = PortInfo.NumberAxis;

    RtlCopyMemory(DeviceExtension->HidGameOemData.Game_Oem_Data, PortInfo.OemData, sizeof(PortInfo.OemData));

    return Irp->IoStatus.Status;
} /* HGM_GetResources */


/*****************************************************************************
 *
 *  @func   NTSTATUS  | HGM_PnPComplete |
 *
 *          Completion routine for PnP IRPs.  
 *          Not pageable because it is a completion routine.
 *
 *  @parm   IN PDEVICE_OBJECT | DeviceObject |
 *
 *          Pointer to the device object.
 *
 *  @parm   IN PIRP | Irp |
 *
 *          Pointer to an I/O request packet.
 *
 *  @rvalue STATUS_MORE_PROCESSING_REQUIRED | We want the IRP back
 *
 *****************************************************************************/
NTSTATUS HGM_PnPComplete
    (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    )
{
    NTSTATUS ntStatus = STATUS_MORE_PROCESSING_REQUIRED;

    UNREFERENCED_PARAMETER (DeviceObject);
    KeSetEvent ((PKEVENT) Context, 0, FALSE);

    return ntStatus;
}


