/*++

Copyright (c) 2007 Alfredo Costalago
Module Name:

    hidfilter.c

Abstract: Filtro para - Human Interface Device (HID) USB driver

Environment:

    Kernel mode

--*/

#include <wdm.h>
#include <usbdi.h>
#include <usbdlib.h>
#include "hidfilter.h"

#define _CONTROL_
#include "control.h"
#undef _CONTROL_

#ifdef ALLOC_PRAGMA
    #pragma alloc_text( PAGE, HF_ControlDispatchPassThrough)
#endif /* ALLOC_PRAGMA */

#define IOCTL_MFD_LUZ		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0100, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_GLOBAL_LUZ	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0101, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_INFO_LUZ		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0102, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_PINKIE		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0103, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_TEXTO			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0104, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_HORA			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0105, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_HORA24		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0106, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_FECHA			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0107, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_ESTADO_USB	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0108, METHOD_BUFFERED, FILE_WRITE_ACCESS)
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
        case IRP_MJ_DEVICE_CONTROL:
			switch (irpStack->Parameters.DeviceIoControl.IoControlCode) {
				case IOCTL_MFD_LUZ:
					{
						UCHAR params[3];
						params[0] = *((UCHAR*)Irp->AssociatedIrp.SystemBuffer); params[1] = 0;
						params[2] = 0xb1;
						status = EnviarOrden(DeviceObject,params);
						if(NT_SUCCESS(status)) Irp->IoStatus.Information = 1;
						break;
					}
				case IOCTL_GLOBAL_LUZ:
					{
						UCHAR params[3];
						params[0] = *((UCHAR*)Irp->AssociatedIrp.SystemBuffer); params[1] = 0;
						params[2] = 0xb2;
						status = EnviarOrden(DeviceObject,params);
						if(NT_SUCCESS(status)) Irp->IoStatus.Information = 1;
						break;
					}
				case IOCTL_INFO_LUZ:
					{
						UCHAR params[3];
						params[0] = *((UCHAR*)Irp->AssociatedIrp.SystemBuffer)+0x50; params[1] = 0;
						params[2] = 0xb4;
						status = EnviarOrden(DeviceObject,params);
						if(NT_SUCCESS(status)) Irp->IoStatus.Information = 1;
						break;
					}
				case IOCTL_PINKIE:
					{
						UCHAR params[3];
						params[0] = *((UCHAR*)Irp->AssociatedIrp.SystemBuffer)+0x50; params[1] = 0;
						params[2] = 0xfd;
						status = EnviarOrden(DeviceObject,params);
						if(NT_SUCCESS(status)) Irp->IoStatus.Information = 1;
						break;
					}
				case IOCTL_TEXTO:
					{
						UCHAR texto[17];
						RtlZeroMemory(texto,17);
						if((irpStack->Parameters.DeviceIoControl.InputBufferLength-1)<=16) {
							RtlCopyMemory(texto,&((UCHAR*)Irp->AssociatedIrp.SystemBuffer)[1],irpStack->Parameters.DeviceIoControl.InputBufferLength-1);
						} else {
							RtlCopyMemory(texto,&((UCHAR*)Irp->AssociatedIrp.SystemBuffer)[1],16);
						}
						status = SetLinea(DeviceObject,*((UCHAR*)Irp->AssociatedIrp.SystemBuffer),texto);
						if(NT_SUCCESS(status)) Irp->IoStatus.Information = irpStack->Parameters.DeviceIoControl.InputBufferLength;
						break;
					}
				case IOCTL_HORA:
					{
						UCHAR params[3];
						params[0] = ((UCHAR*)Irp->AssociatedIrp.SystemBuffer)[2];
						params[1] = ((UCHAR*)Irp->AssociatedIrp.SystemBuffer)[1];
						params[2] = *((UCHAR*)Irp->AssociatedIrp.SystemBuffer)+0xbf;
						status = EnviarOrden(DeviceObject,params);
						if(NT_SUCCESS(status)) Irp->IoStatus.Information = 1;
						break;
					}
				case IOCTL_HORA24:
					{
						UCHAR params[3];
						params[0] = ((UCHAR*)Irp->AssociatedIrp.SystemBuffer)[2];
						params[1] = ((UCHAR*)Irp->AssociatedIrp.SystemBuffer)[1]+0x80;
						params[2] = *((UCHAR*)Irp->AssociatedIrp.SystemBuffer)+0xbf;
						status = EnviarOrden(DeviceObject,params);
						if(NT_SUCCESS(status)) Irp->IoStatus.Information = 1;
						break;
					}
				case IOCTL_FECHA:
					{
						status = SetFecha(DeviceObject,Irp->AssociatedIrp.SystemBuffer);
						if(NT_SUCCESS(status)) Irp->IoStatus.Information = 2;
						break;
					}
				default:
					status=STATUS_NOT_SUPPORTED;
			}
			break;
        case IRP_MJ_INTERNAL_DEVICE_CONTROL:
			if( irpStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_ESTADO_USB ) {
				PDEVICE_EXTENSION devExt = ((PITFDEVICE_EXTENSION)DeviceObject->DeviceExtension)->vDevExt;
				KIRQL irql;
				PIRP antigua = NULL;
				KeAcquireSpinLock(&devExt->slIrpLectura,&irql);
					if(devExt->irpLectura != NULL) { antigua = devExt->irpLectura; devExt->irpLectura = NULL; }
				KeReleaseSpinLock(&devExt->slIrpLectura, irql);
				if(antigua != NULL ) {
					PURB pUrb = (PURB)IoGetCurrentIrpStackLocation(antigua)->Parameters.Others.Argument1;
					struct _URB_BULK_OR_INTERRUPT_TRANSFER *pBulkOrInterruptTransfer = (struct _URB_BULK_OR_INTERRUPT_TRANSFER *) pUrb;
					UCHAR* bufPtr;
					if (pBulkOrInterruptTransfer->TransferBuffer) {
						bufPtr = (UCHAR*)pBulkOrInterruptTransfer->TransferBuffer;
					} else {
						bufPtr = (UCHAR*)MmGetSystemAddressForMdlSafe(pBulkOrInterruptTransfer->TransferBufferMDL,HighPagePriority);
					}
					RtlCopyMemory(bufPtr, Irp->AssociatedIrp.SystemBuffer,sizeof(HID_INPUT_DATA));
					antigua->IoStatus.Status = STATUS_SUCCESS;
					antigua->IoStatus.Information = sizeof(HID_INPUT_DATA);
					IoCompleteRequest(antigua,IO_NO_INCREMENT);
				}

				Irp->IoStatus.Information = sizeof(HID_INPUT_DATA);
			} else {
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

NTSTATUS SetFecha( IN PDEVICE_OBJECT DeviceObject, IN UCHAR* datos)
{
	PITFDEVICE_EXTENSION itfdevExt=(PITFDEVICE_EXTENSION) DeviceObject->DeviceExtension;
	UCHAR params[3];

	switch(datos[0]) {
		case 1:
			params[2]=0xc4;
			params[1]=(UCHAR)(itfdevExt->fecha>>8);
			params[0]=datos[1];
			itfdevExt->fecha=*((USHORT*)params);
			break;
		case 2:
			params[2]=0xc4;
			params[1]=datos[1];
			params[0]=(UCHAR)(itfdevExt->fecha&0xff);
			itfdevExt->fecha=*((USHORT*)params);
			break;
		case 3:
			params[2]=0xc8;
			params[1]=0;
			params[0]=datos[1];
	}

	return EnviarOrden(DeviceObject,params);
}

NTSTATUS SetLinea( 
	IN PDEVICE_OBJECT DeviceObject,
	IN CHAR linea,
	IN UCHAR* texto)
{
    NTSTATUS status = STATUS_SUCCESS;
	CHAR i = 0;
	UCHAR params[3];

	params[0]=0x0; params[1]=0;
	switch(linea) {
		case 1:
			params[2]=0xd9;
			break;
		case 2:
			params[2]=0xda;
			break;
		case 3:
			params[2]=0xdc;
	}
	status =EnviarOrden(DeviceObject,params);

	if(NT_SUCCESS(status)) {
		switch(linea) {
			case 1:
				params[2]=0xd1;
				break;
			case 2:
				params[2]=0xd2;
				break;
			case 3:
				params[2]=0xd4;
		}
		for(i=0;i<17;i+=2) {
			if(texto[i]==0) break;
			params[0]=texto[i];
			params[1]=texto[i+1];
			status =EnviarOrden(DeviceObject,params);
			if(!NT_SUCCESS(status)) break;
		}
	}

	return status;
}


NTSTATUS EnviarOrden(
    IN PDEVICE_OBJECT DeviceObject,
	UCHAR* params
    )
{
    NTSTATUS                        status = STATUS_SUCCESS;
	PITFDEVICE_EXTENSION			itfdevExt=(PITFDEVICE_EXTENSION) DeviceObject->DeviceExtension;

	if(KeGetCurrentIrql()>APC_LEVEL) {
		status = STATUS_SUCCESS;
	} else {
		PVOID urb = ExAllocatePoolWithTag(PagedPool,sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST),'USUX');
		if (urb==NULL) {
			status = STATUS_INSUFFICIENT_RESOURCES;
		} else {
			PIRP pIrp = NULL;
			KEVENT                  IoctlCompleteEvent;
			IO_STATUS_BLOCK         IoStatus;

			UsbBuildVendorRequest((PURB)urb,URB_FUNCTION_VENDOR_DEVICE,sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST),
					0,0, 
					0x91,
					*(USHORT*)params, // value
					params[2], // index
					NULL,NULL,0,NULL);
			KeInitializeEvent(&IoctlCompleteEvent, NotificationEvent, FALSE);
			pIrp = IoBuildDeviceIoControlRequest(IOCTL_INTERNAL_USB_SUBMIT_URB,itfdevExt->vDevExt->TopOfStack,NULL,0,NULL,0,TRUE,&IoctlCompleteEvent,&IoStatus);
			if(pIrp!=NULL)
			{
				PIO_STACK_LOCATION stack = IoGetNextIrpStackLocation(pIrp);
				stack->Parameters.Others.Argument1 = urb;
				status = IoCallDriver(itfdevExt->vDevExt->TopOfStack, pIrp);
				if (status == STATUS_PENDING) {
					KeWaitForSingleObject(&IoctlCompleteEvent,Executive,KernelMode,FALSE,NULL);
					status = STATUS_SUCCESS;
				}
			}
			ExFreePool(urb);
		}
	}

	return status;
}