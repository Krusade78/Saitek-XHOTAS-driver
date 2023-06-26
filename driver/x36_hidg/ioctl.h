/*
 *  ioctl.c
 */

// Funciones públicas

//DRIVER_DISPATCH HGM_InternalIoctl;
NTSTATUS HGM_InternalIoctl
    (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

// Funciones privadas

#ifdef _IOCTLC_

#define HIDGAME_VERSION_NUMBER  ((USHORT) 1)

NTSTATUS HGM_GetDeviceDescriptor
    (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );


NTSTATUS HGM_GetReportDescriptor
    (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS HGM_ReadReport
    (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS HGM_GetAttributes
    (
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp
    );

#endif