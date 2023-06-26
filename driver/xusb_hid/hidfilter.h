#pragma once
#include <usbdi.h>
#include <usbdlib.h>


typedef struct _HID_INPUT_DATA
{
    UCHAR   Ejes[14];
	UCHAR	Setas[4];
	UCHAR	Botones[7];
    UCHAR   MiniStick;
} HID_INPUT_DATA, *PHID_INPUT_DATA;

typedef struct _DEVICE_EXTENSION
{
	ULONG id;
	PDEVICE_OBJECT  self;
    //
    // The top of the stack before this filter was added.  AKA the location
    // to which all IRPS should be directed.
    //
    PDEVICE_OBJECT  TopOfStack;

    /*
     *  deviceCapabilities includes a
     *  table mapping system power states to device power states.
     */
    DEVICE_CAPABILITIES deviceCapabilities;
    /*
     *  pendingActionCount is used to keep track of outstanding actions.
     *  removeEvent is used to wait until all pending actions are
     *  completed before complete the REMOVE_DEVICE IRP and let the
     *  driver get unloaded.
     */
    IO_REMOVE_LOCK removeLock;

	PDEVICE_OBJECT itfObj;

	PDEVICE_OBJECT vdo;
    PFILE_OBJECT vfo;
	UCHAR stModos;
	UCHAR stAux;
	BOOLEAN cerrando;
	HID_INPUT_DATA ultimosDatosOK;
	KSPIN_LOCK slIrpLectura;
	PIRP irpLectura;


} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

typedef struct _ITFDEVICE_EXTENSION
{
	ULONG id;
	PDEVICE_EXTENSION vDevExt;
	USHORT fecha;

}ITFDEVICE_EXTENSION,*PITFDEVICE_EXTENSION;

#ifdef _HIDFILTER_

NTSTATUS DriverEntry
    (
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

//DRIVER_ADD_DEVICE HF_AddDevice;
NTSTATUS
HF_AddDevice(
    IN PDRIVER_OBJECT   Driver,
    IN PDEVICE_OBJECT   PDO
    );

//DRIVER_UNLOAD HF_Unload;
VOID
HF_Unload(
   IN PDRIVER_OBJECT Driver
   );

//DRIVER_DISPATCH HF_DispatchPassThrough;
NTSTATUS
HF_DispatchPassThrough(
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
        );

NTSTATUS
HF_PnP(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

//NTSTATUS
//HF_Power(
//    IN PDEVICE_OBJECT    DeviceObject,
//    IN PIRP              Irp
//    );
NTSTATUS VA_Power(PDEVICE_EXTENSION devExt, PIRP irp);
#endif