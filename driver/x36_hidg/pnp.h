/*
 *  pnp.c
 */

// Funciones publicas

//DRIVER_DISPATCH HGM_PnP;
NTSTATUS HGM_PnP
    (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

// Funciones privadas

#ifdef _PNPC_

VOID HGM_RemoveDevice
    (
    PDEVICE_EXTENSION DeviceExtension
    );

NTSTATUS HGM_InitDevice
    (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

NTSTATUS HGM_GetResources
    (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

//IO_COMPLETION_ROUTINE HGM_PnPComplete;
NTSTATUS HGM_PnPComplete
    (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

#endif