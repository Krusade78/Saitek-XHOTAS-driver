/*++

Copyright (c) 2005-2007 Alfredo Costalago
Module Name:

    hidgame.c

Abstract: Human Interface Device (HID) Gameport driver

Environment:

    Kernel mode


--*/

#include <wdm.h>
#include <hidport.h>
#include "hidgame.h"
#include "i386/timing.h"
#include "ioctl.h"
#include "pnp.h"
#include "request.h"

#ifdef ALLOC_PRAGMA
    #pragma alloc_text( INIT, DriverEntry )
    #pragma alloc_text( PAGE, HGM_CreateClose)
    #pragma alloc_text( PAGE, HGM_AddDevice)
    #pragma alloc_text( PAGE, HGM_Unload)
    #pragma alloc_text( PAGE, HGM_SystemControl)
    #pragma alloc_text (PAGE, HGM_Power)
#endif /* ALLOC_PRAGMA */

HIDGAME_GLOBAL Global;

/*****************************************************************************
 *
 *  @func   NTSTATUS | HGM_DriverInit |
 *
 *          Perform global initialization.
 *          <nl>This is called from DriverEntry.  Try to initialize a CPU 
 *          specific timer but if it fails set up default
 *
 *  @rvalue   STATUS_SUCCESS | success
 *  @rvalue   STATUS_UNSUCCESSFUL | not success
 *
 *****************************************************************************/
NTSTATUS HGM_DriverInit()
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if( !HGM_CPUCounterInit() )
    {
        LARGE_INTEGER QPCFrequency;

        KeQueryPerformanceCounter( &QPCFrequency );

        if( ( QPCFrequency.HighPart == 0 )
         && ( QPCFrequency.LowPart <= 10000 ) )
        {
            ntStatus = STATUS_UNSUCCESSFUL;
        }
        else
        {
            Global.CounterScale = CALCULATE_SCALE( QPCFrequency.QuadPart );
            Global.ReadCounter = (COUNTER_FUNCTION)&KeQueryPerformanceCounter;
        }
    }

    return ntStatus;
}
/*****************************************************************************
 *
 *  @func   NTSTATUS  | DriverEntry |
 *
 *          Installable driver initialization entry point.
 *          <nl>This entry point is called directly by the I/O system.
 *
 *  @parm   IN PDRIVER_OBJECT | DriverObject |
 *
 *          Pointer to the driver object
 *
 *  @parm   IN PUNICODE_STRING | RegistryPath |
 *
 *          Pointer to a unicode string representing the path,
 *          to driver-specific key in the registry.
 *
 *  @rvalue   STATUS_SUCCESS | success
 *  @rvalue   ???            | returned HidRegisterMinidriver()
 *
 *****************************************************************************/
NTSTATUS DriverEntry
    (
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
{
    NTSTATUS                        ntStatus;
    HID_MINIDRIVER_REGISTRATION     hidMinidriverRegistration;

    ntStatus = HGM_DriverInit();

    if( NT_SUCCESS(ntStatus) )
    {
        DriverObject->MajorFunction[IRP_MJ_CREATE]    =
            DriverObject->MajorFunction[IRP_MJ_CLOSE] = HGM_CreateClose;
        DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] =
            HGM_InternalIoctl;
        DriverObject->MajorFunction[IRP_MJ_PNP]   = HGM_PnP;
        DriverObject->MajorFunction[IRP_MJ_POWER] = HGM_Power;
        DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = HGM_SystemControl;
        DriverObject->DriverUnload                = HGM_Unload;
        DriverObject->DriverExtension->AddDevice  = HGM_AddDevice;

        /*
         * Register  with HID.SYS module
         */
        RtlZeroMemory(&hidMinidriverRegistration, sizeof(hidMinidriverRegistration));

        hidMinidriverRegistration.Revision            = HID_REVISION;
        hidMinidriverRegistration.DriverObject        = DriverObject;
        hidMinidriverRegistration.RegistryPath        = RegistryPath;
        hidMinidriverRegistration.DeviceExtensionSize = sizeof(DEVICE_EXTENSION);
		hidMinidriverRegistration.DevicesArePolled    = TRUE;


        ntStatus = HidRegisterMinidriver(&hidMinidriverRegistration);

        if( NT_SUCCESS(ntStatus) )
        {
            /*
             *  Protect the list with a Mutex
             */
            ExInitializeFastMutex (&Global.Mutex);

            /*
             *  Initialize the device list head
             */
            InitializeListHead(&Global.DeviceListHead);

            /*
             *  Initialize gameport access spinlock
             */
            KeInitializeSpinLock(&Global.SpinLock);
        }
    }

    return ntStatus;
} /* DriverEntry */


/*****************************************************************************
 *
 *  @func   NTSTATUS  | HGM_AddDevice |
 *
 *          Called by hidclass, allows us to initialize our device extensions.
 *
 *  @parm   IN PDRIVER_OBJECT | DriverObject |
 *
 *          Pointer to the driver object
 *
 *  @parm   IN PDEVICE_OBJECT | FunctionalDeviceObject |
 *
 *          Pointer to a functional device object created by hidclass.
 *
 *  @rvalue   STATUS_SUCCESS | success
 *
 *****************************************************************************/
NTSTATUS HGM_AddDevice
    (
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT FunctionalDeviceObject
    )
{
    NTSTATUS                ntStatus = STATUS_SUCCESS;
    PDEVICE_OBJECT          DeviceObject;
    PDEVICE_EXTENSION       DeviceExtension;
	UCHAR idx;

    PAGED_CODE ();

    DeviceObject = FunctionalDeviceObject;

    /*
     * Initialize the device extension.
     */
    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION (DeviceObject);
	DeviceExtension->wfo=NULL;
	DeviceExtension->wdo=NULL;
	DeviceExtension->vfo=NULL;
	DeviceExtension->vdo=NULL;
	//DeviceExtension->nuevaCalibracion=TRUE;
	RtlZeroMemory(&DeviceExtension->ultimosDatosOK,sizeof(HIDGAME_INPUT_DATA));
	DeviceExtension->descalibrar = FALSE;
	DeviceExtension->limites[0].cal=DeviceExtension->limites[1].cal=DeviceExtension->limites[2].cal=DeviceExtension->limites[3].cal=FALSE;
	DeviceExtension->jitter[0].antiv=DeviceExtension->jitter[1].antiv=DeviceExtension->jitter[2].antiv=DeviceExtension->jitter[3].antiv=FALSE;
	DeviceExtension->RepsAxis[0]=DeviceExtension->RepsAxis[1]=DeviceExtension->RepsAxis[2]=DeviceExtension->RepsAxis[3]=0;
	//DeviceExtension->minAxis[0]=DeviceExtension->minAxis[1]=DeviceExtension->minAxis[2]=DeviceExtension->minAxis[3]=0xffffffff;
	//DeviceExtension->maxAxis[0]=DeviceExtension->maxAxis[1]=DeviceExtension->maxAxis[2]=DeviceExtension->maxAxis[3]=0;
    DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    /*
     * Initialize the list
     */
    InitializeListHead(&DeviceExtension->Link);

    /*
     *  Acquire mutex before modifying the Global Linked list of devices
     */
    ExAcquireFastMutex (&Global.Mutex);

    /*
     * Add this device to the linked list of devices
     */
    InsertTailList(&Global.DeviceListHead, &DeviceExtension->Link);

    /*
     *  Release the mutex
     */
    ExReleaseFastMutex (&Global.Mutex);

    /*
     * Initialize the remove lock 
     */
    DeviceExtension->RequestCount = 1;
    KeInitializeEvent(&DeviceExtension->RemoveEvent,
                      SynchronizationEvent,
                      FALSE);

    return ntStatus;
} /* HGM_AddDevice */


/*****************************************************************************
 *
 *  @func   NTSTATUS  | HGM_CreateClose |
 *
 *          Process the create and close IRPs sent to this device.
 *
 *  @parm   IN PDEVICE_OBJECT | DeviceObject |
 *
 *          Pointer to the device object
 *
 *  @parm   IN PIRP | Irp |
 *
 *          Pointer to an I/O Request Packet.
 *
 *  @rvalue   STATUS_SUCCESS | success
 *  @rvalue   STATUS_INVALID_PARAMETER  | Irp not handled
 *
 *****************************************************************************/
NTSTATUS HGM_CreateClose
    (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PIO_STACK_LOCATION   IrpStack;
    NTSTATUS             ntStatus = STATUS_SUCCESS;

    PAGED_CODE ();

    IrpStack = IoGetCurrentIrpStackLocation(Irp);

    switch(IrpStack->MajorFunction)
    {
        case IRP_MJ_CREATE:
        case IRP_MJ_CLOSE:
            Irp->IoStatus.Information = 0;
            break;

        default:
            ntStatus = STATUS_INVALID_PARAMETER;
            break;
    }

    Irp->IoStatus.Status = ntStatus;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return ntStatus;
} /* HGM_CreateClose */



/*****************************************************************************
 *
 *  @func   NTSTATUS  | HGM_SystemControl |
 *
 *          Process the WMI IRPs sent to this device.
 *
 *  @parm   IN PDEVICE_OBJECT | DeviceObject |
 *
 *          Pointer to the device object
 *
 *  @parm   IN PIRP | Irp |
 *
 *          Pointer to an I/O Request Packet.
 *
 *  @rvalue   STATUS_SUCCESS | success
 *  @rvalue   STATUS_INVALID_PARAMETER  | Irp not handled
 *
 *****************************************************************************/
NTSTATUS HGM_SystemControl
    (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PAGED_CODE ();

    IoSkipCurrentIrpStackLocation(Irp);

    return IoCallDriver(GET_NEXT_DEVICE_OBJECT(DeviceObject), Irp);
} /* HGM_SystemControl */

/*****************************************************************************
 *
 *  @func   NTSTATUS  | HGM_Power |
 *
 *          The power dispatch routine.
 *          <nl>This driver does not recognize power IRPS.  It merely sends them down,
 *          unmodified to the next device on the attachment stack.
 *          As this is a POWER irp, and therefore a special irp, special power irp
 *          handling is required. No completion routine is required.
 *
 *  @parm   IN PDEVICE_OBJECT | DeviceObject |
 *
 *          Pointer to the device object.
 *
 *  @parm   IN PIRP | Irp |
 *
 *          Pointer to an I/O request packet.
 *
 *
 *  @rvalue   STATUS_SUCCESS | success
 *  @rvalue   ???            | Return from PoCallDriver()
 *
 *****************************************************************************/
NTSTATUS HGM_Power
    (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PDEVICE_EXTENSION  DeviceExtension;
    NTSTATUS           ntStatus;

    PAGED_CODE ();

    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION (DeviceObject);

    /*
     * Since we do not know what to do with the IRP, we should pass
     * it on along down the stack.
     */

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
        IoSkipCurrentIrpStackLocation (Irp);

        /*
         * Power IRPS come synchronously; drivers must call
         * PoStartNextPowerIrp, when they are ready for the next power irp.
         * This can be called here, or in the completetion routine.
         */
        PoStartNextPowerIrp (Irp);
        ntStatus =  PoCallDriver (GET_NEXT_DEVICE_OBJECT (DeviceObject), Irp);
        HGM_DecRequestCount( DeviceExtension );
    }


    return ntStatus;
} /* HGM_Power */

/*****************************************************************************
 *
 *  @func   void  | HGM_Unload |
 *
 *          Free all the allocated resources, etc.
 *
 *  @parm   IN PDRIVER_OBJECT | DeviceObject |
 *
 *          Pointer to the driver object
 *
 *
 *****************************************************************************/
VOID HGM_Unload
    (
    IN PDRIVER_OBJECT DriverObject
    )
{
    PAGED_CODE();

    /*
     * All the device objects should be gone
     */

    ASSERT ( NULL == DriverObject->DeviceObject);

    return;
} /* HGM_Unload */
