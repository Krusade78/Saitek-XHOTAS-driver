#include <wdm.h>
#include "hidfilter.h"
#include "directx.h"
#include "ejes.h"
//#include "raton.h"
#include "mapa.h"

#include "control.h"

#ifdef ALLOC_PRAGMA
    #pragma alloc_text( PAGE, HF_ControlDispatchPassThrough)
#endif /* ALLOC_PRAGMA */

#define IOCTL_HID_PROCESAR		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0100, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_USR_CALIBRADO		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0100, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_USR_CALIBRAR		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0101, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_USR_DESCALIBRAR	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0102, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_USR_MAPA			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0103, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_USR_COMANDOS		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0104, METHOD_BUFFERED, FILE_WRITE_ACCESS)

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
			if(irpStack->Parameters.DeviceIoControl.IoControlCode==IOCTL_HID_PROCESAR) {
				return HF_IoWrite(DeviceObject,Irp);
			} else {
				status=STATUS_NOT_SUPPORTED;
			}
			break;
        case IRP_MJ_DEVICE_CONTROL:
			switch (irpStack->Parameters.DeviceIoControl.IoControlCode) {
				case IOCTL_USR_COMANDOS:
					return HF_IoEscribirComandos(DeviceObject,Irp);
				case IOCTL_USR_MAPA:
					return HF_IoEscribirMapa(DeviceObject,Irp);
				case IOCTL_USR_CALIBRADO:
					return HF_IoEscribirCalibrado(DeviceObject,Irp);
				case IOCTL_USR_CALIBRAR:
					return HF_IoRecalibrar(DeviceObject,Irp);
				case IOCTL_USR_DESCALIBRAR:
					return HF_IoDescalibrar(DeviceObject,Irp);
				default:
					status=STATUS_NOT_SUPPORTED;
			}
			break;
		default:
			status=STATUS_NOT_SUPPORTED;
	}

	Irp->IoStatus.Status = status;
	IoCompleteRequest (Irp, IO_NO_INCREMENT);
	return status;
}