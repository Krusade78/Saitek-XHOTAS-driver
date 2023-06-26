#include <wdm.h>
#include "hidfilter.h"
#include "io.h"

#define _CONTROL_
#include "control.h"
#undef _CONTROL_ 

#ifdef ALLOC_PRAGMA
    #pragma alloc_text( PAGE, HF_ControlDispatchPassThrough)
    #pragma alloc_text( PAGE, HF_IoDescalibrar)
    #pragma alloc_text( PAGE, HF_IoRecalibrar)
    #pragma alloc_text( PAGE, HF_IoLeerEstado)
#endif /* ALLOC_PRAGMA */

#define IOCTL_WRAPPER_LEERX36	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0100, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_WRAPPER_ESTADO	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0101, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_WRAPPER_CALIBRADO	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0102, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_X36USR_CALIBRADO		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0100, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_X36USR_CALIBRAR		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0101, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_X36USR_DESCALIBRAR	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0102, METHOD_BUFFERED, FILE_READ_ACCESS)

NTSTATUS
HF_ControlDispatchPassThrough(
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
        )
{
	NTSTATUS status=STATUS_SUCCESS;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);

	PAGED_CODE();

    Irp->IoStatus.Information = 0;

	switch(irpStack->MajorFunction) {
        case IRP_MJ_CREATE:
        case IRP_MJ_CLOSE:
        case IRP_MJ_CLEANUP:
	        break;
        case IRP_MJ_INTERNAL_DEVICE_CONTROL:
			switch (irpStack->Parameters.DeviceIoControl.IoControlCode) {
				case IOCTL_WRAPPER_LEERX36:
					return HF_IoLeer(DeviceObject,Irp);
				case IOCTL_WRAPPER_ESTADO:
					return HF_IoLeerEstado(DeviceObject,Irp);
				case IOCTL_WRAPPER_CALIBRADO:
					return HF_IoLeerCalibrado(DeviceObject,Irp);
				default:
					status=STATUS_NOT_SUPPORTED;
			}
			break;
        case IRP_MJ_DEVICE_CONTROL:
			switch (irpStack->Parameters.DeviceIoControl.IoControlCode) {
				case IOCTL_X36USR_CALIBRADO:
					return HF_IoEscribirCalibrado(DeviceObject,Irp);
				case IOCTL_X36USR_CALIBRAR:
					return HF_IoRecalibrar(DeviceObject,Irp);
				case IOCTL_X36USR_DESCALIBRAR:
					return HF_IoDescalibrar(DeviceObject,Irp);
				default:
					status=STATUS_NOT_SUPPORTED;
			}
			break;
        case IRP_MJ_WRITE:
			return HF_IoWrite(DeviceObject,Irp);
		default:
			status=STATUS_NOT_SUPPORTED;
	}

	Irp->IoStatus.Status = status;
	IoCompleteRequest (Irp, IO_NO_INCREMENT);
	return status;
}

#pragma region "usuario"

NTSTATUS
HF_IoDescalibrar(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
	)
{
    PIO_STACK_LOCATION              irpStack;
    PITFDEVICE_EXTENSION            devExt;
    NTSTATUS                        status = STATUS_SUCCESS;
	KIRQL irql;

	PAGED_CODE();

    devExt = (PITFDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    irpStack = IoGetCurrentIrpStackLocation(Irp);

	KeAcquireSpinLock(&devExt->slEstado,&irql);
		devExt->Estado|=4;
	KeReleaseSpinLock(&devExt->slEstado,irql);

	Irp->IoStatus.Information = 0;
	Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return status;
}

NTSTATUS
HF_IoRecalibrar(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
	)
{
    PIO_STACK_LOCATION              irpStack;
    PITFDEVICE_EXTENSION            devExt;
    NTSTATUS                        status = STATUS_SUCCESS;
	KIRQL irql;

	PAGED_CODE();

    devExt = (PITFDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    irpStack = IoGetCurrentIrpStackLocation(Irp);

	KeAcquireSpinLock(&devExt->slEstado,&irql);
		devExt->Estado&=3;
	KeReleaseSpinLock(&devExt->slEstado,irql);

	Irp->IoStatus.Information = 0;
	Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return status;
}

NTSTATUS
HF_IoEscribirCalibrado(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
	)
{
	PIO_STACK_LOCATION              irpStack;
    PITFDEVICE_EXTENSION            devExt;
    NTSTATUS                        status = STATUS_SUCCESS;

	PAGED_CODE();

    devExt = (PITFDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    irpStack = IoGetCurrentIrpStackLocation(Irp);
	if(irpStack->Parameters.DeviceIoControl.InputBufferLength==(sizeof(CALIBRADO)*4)) {
		KIRQL irql;
		KeAcquireSpinLock(&devExt->slCalibrado,&irql);
			RtlCopyMemory(devExt->calibrado,Irp->AssociatedIrp.SystemBuffer,sizeof(CALIBRADO)*4);
		KeReleaseSpinLock(&devExt->slCalibrado,irql);
		KeAcquireSpinLock(&devExt->slEstado,&irql);
			devExt->Estado|=2;
		KeReleaseSpinLock(&devExt->slEstado,irql);
		Irp->IoStatus.Information = sizeof(CALIBRADO)*4;
	} else {
		status=STATUS_BUFFER_TOO_SMALL;
		Irp->IoStatus.Information = 0;
	}

	Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

#pragma endregion

#pragma region "interno"

NTSTATUS
HF_IoLeerEstado(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PIO_STACK_LOCATION              irpStack;
    PITFDEVICE_EXTENSION            devExt;
    NTSTATUS                        status = STATUS_SUCCESS;

	PAGED_CODE();

    devExt = (PITFDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    irpStack = IoGetCurrentIrpStackLocation(Irp);
	if(irpStack->Parameters.DeviceIoControl.OutputBufferLength==1) {
		KIRQL irql;
		KeAcquireSpinLock(&devExt->slEstado,&irql);
			*((PCHAR)Irp->AssociatedIrp.SystemBuffer)=devExt->Estado;
		KeReleaseSpinLock(&devExt->slEstado,irql);
		Irp->IoStatus.Information = 1;
	} else {
		status=STATUS_BUFFER_TOO_SMALL;
		Irp->IoStatus.Information = 0;
	}

	Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS
HF_IoLeerCalibrado(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
	)
{
	PIO_STACK_LOCATION              irpStack;
    PITFDEVICE_EXTENSION            devExt;
    NTSTATUS                        status = STATUS_SUCCESS;

	PAGED_CODE();

    devExt = (PITFDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    irpStack = IoGetCurrentIrpStackLocation(Irp);
	if( irpStack->Parameters.DeviceIoControl.OutputBufferLength==(sizeof(CALIBRADO)*4) ) {
		KIRQL irql;
		KeAcquireSpinLock(&devExt->slCalibrado,&irql);
			RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer,devExt->calibrado,sizeof(CALIBRADO)*4);
		KeReleaseSpinLock(&devExt->slCalibrado,irql);
		KeAcquireSpinLock(&devExt->slEstado,&irql);
			devExt->Estado&=5;
		KeReleaseSpinLock(&devExt->slEstado,irql);

		Irp->IoStatus.Information = sizeof(CALIBRADO)*4;
	} else {
		status=STATUS_BUFFER_TOO_SMALL;
		Irp->IoStatus.Information = 0;
	}

	Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

#pragma endregion