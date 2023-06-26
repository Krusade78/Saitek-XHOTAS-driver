//
// These are the states FDO transition to upon
// receiving a specific PnP Irp. Refer to the PnP Device States
// diagram in DDK documentation for better understanding.
//

typedef enum _DEVICE_PNP_STATE {

    NotStarted = 0,         // Not started yet
    Started,                // Device has received the START_DEVICE IRP
    StopPending,            // Device has received the QUERY_STOP IRP
    Stopped,                // Device has received the STOP_DEVICE IRP
    RemovePending,          // Device has received the QUERY_REMOVE IRP
    SurpriseRemovePending,  // Device has received the SURPRISE_REMOVE IRP
    Deleted                 // Device has received the REMOVE_DEVICE IRP

} DEVICE_PNP_STATE;

typedef struct _DEVICE_EXTENSION
{
    DEVICE_PNP_STATE    DevicePnPState;   // Track the state of the device
    DEVICE_PNP_STATE    PreviousPnPState; // Remembers the previous pnp state
} DEVICE_EXTENSION, * PDEVICE_EXTENSION;

#define GET_MINIDRIVER_DEVICE_EXTENSION(DO) \
    ((PDEVICE_EXTENSION) \
    (((PHID_DEVICE_EXTENSION)(DO)->DeviceExtension)->MiniDeviceExtension))

#define GET_NEXT_DEVICE_OBJECT(DO) \
    (((PHID_DEVICE_EXTENSION)(DO)->DeviceExtension)->NextDeviceObject)

//#define GET_PHYSICAL_DEVICE_OBJECT(DO) \
//    (((PHID_DEVICE_EXTENSION)(DO)->DeviceExtension)->PhysicalDeviceObject)


#ifdef _VHIDMINI_

//
// driver routines
//

NTSTATUS 
DriverEntry(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING registryPath
    );

//DRIVER_DISPATCH PnP;
NTSTATUS 
PnP(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

//IO_COMPLETION_ROUTINE PnPComplete;
NTSTATUS 
PnPComplete(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    );

//DRIVER_DISPATCH Power;
NTSTATUS 
Power(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

//DRIVER_ADD_DEVICE AddDevice;
NTSTATUS  
AddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT FunctionalDeviceObject
    );

//DRIVER_UNLOAD Unload;
VOID 
Unload(
    IN PDRIVER_OBJECT DriverObject
    );

#endif