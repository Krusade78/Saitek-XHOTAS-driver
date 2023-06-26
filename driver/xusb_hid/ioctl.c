/*++

Copyright (c) 2007 Alfredo Costalago
Module Name:

    hidfilter.c

Abstract: Filtro para - Human Interface Device (HID) USB driver

Environment:

    Kernel mode

--*/

#include <wdm.h>
//#include <usbdi.h>
//#include <usbdlib.h>
#include <hidport.h>
#include "hidfilter.h"

#define _IOCTL_
#include "ioctl.h"
#undef _IOCTL_

#ifdef ALLOC_PRAGMA
        #pragma alloc_text(PAGE, PreReadReport)
        #pragma alloc_text(PAGE, ReadReport)
#endif

VOID CancelarIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
	KIRQL irql;
    PDEVICE_EXTENSION devExt = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

	KeAcquireSpinLock(&devExt->slIrpLectura,&irql);
		devExt->irpLectura = NULL;
	KeReleaseSpinLock(&devExt->slIrpLectura, irql);
	IoReleaseCancelSpinLock(Irp->CancelIrql);
	Irp->IoStatus.Status = STATUS_CANCELLED;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp,IO_NO_INCREMENT);
}
NTSTATUS
HF_InternIoCtl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp	,
    BOOLEAN* unlock
    )
{
    PIO_STACK_LOCATION              irpStack;
    NTSTATUS                        status = STATUS_SUCCESS;
    PDEVICE_EXTENSION devExt;
	PURB pUrb;

	devExt = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    irpStack = IoGetCurrentIrpStackLocation(Irp);
	pUrb = (PURB)irpStack->Parameters.Others.Argument1;
   
	if(irpStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_INTERNAL_USB_SUBMIT_URB) {
		switch (pUrb->UrbHeader.Function) {
			case URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER:
				{
					KIRQL irql;
					KeAcquireSpinLock(&devExt->slIrpLectura,&irql);
					if(devExt->cerrando) {
						KeReleaseSpinLock(&devExt->slIrpLectura, irql);
						IoSkipCurrentIrpStackLocation(Irp);
						status = IoCallDriver(devExt->TopOfStack, Irp);
					} else {
						PURB pUrb = (PURB)IoGetCurrentIrpStackLocation(Irp)->Parameters.Others.Argument1;
						struct _URB_BULK_OR_INTERRUPT_TRANSFER *pBulkOrInterruptTransfer = (struct _URB_BULK_OR_INTERRUPT_TRANSFER *) pUrb;
						if( (devExt->irpLectura == NULL) && (pBulkOrInterruptTransfer->TransferBufferLength==sizeof(HID_INPUT_DATA)) && (pBulkOrInterruptTransfer->TransferFlags==(USBD_TRANSFER_DIRECTION_IN|USBD_SHORT_TRANSFER_OK))) {
							IoSetCancelRoutine(Irp, CancelarIrp);
							if(Irp->Cancel && IoSetCancelRoutine(Irp, NULL)) {
								KeReleaseSpinLock(&devExt->slIrpLectura, irql);
								status = STATUS_CANCELLED;
								Irp->IoStatus.Status = STATUS_CANCELLED;
								Irp->IoStatus.Information = 0;
								IoCompleteRequest(Irp,IO_NO_INCREMENT);
							} else {
								devExt->irpLectura = Irp;
								IoMarkIrpPending(Irp);
								KeReleaseSpinLock(&devExt->slIrpLectura, irql);
								status = STATUS_PENDING;
							}
						} else { // por si acaso
							KeReleaseSpinLock(&devExt->slIrpLectura, irql);
							IoCopyCurrentIrpStackLocationToNext(Irp);
							IoSetCompletionRoutine(Irp, (PIO_COMPLETION_ROUTINE)ReportCompletion, NULL, TRUE, TRUE, TRUE);    
							status = IoCallDriver(devExt->TopOfStack, Irp);
							if(status==STATUS_SUCCESS) *unlock = FALSE;
						}
					}
				}
				break;
			case URB_FUNCTION_RESET_PIPE:
				{
					KIRQL irql;
					PIRP antigua = NULL;
					KeAcquireSpinLock(&devExt->slIrpLectura,&irql);
						if ( devExt->irpLectura != NULL ) { antigua = devExt->irpLectura; devExt->irpLectura = NULL; }
					KeReleaseSpinLock(&devExt->slIrpLectura, irql);
					if( antigua != NULL ) {
						if(antigua->Cancel && IoSetCancelRoutine(antigua, NULL)) {
							antigua->IoStatus.Status = STATUS_CANCELLED;
							antigua->IoStatus.Information = 0;
							IoCompleteRequest(antigua, IO_NO_INCREMENT);
						}
					}
					IoSkipCurrentIrpStackLocation(Irp);
					status = IoCallDriver(devExt->TopOfStack, Irp);
				}
				break ;
			case URB_FUNCTION_ABORT_PIPE:
				{
					KIRQL irql;
					PIRP antigua = NULL;
					KeAcquireSpinLock(&devExt->slIrpLectura,&irql);
						if ( devExt->irpLectura != NULL ) { antigua = devExt->irpLectura; devExt->irpLectura = NULL; }
					KeReleaseSpinLock(&devExt->slIrpLectura, irql);
					if( antigua != NULL ) {
						if(antigua->Cancel && IoSetCancelRoutine(antigua, NULL)) {
							antigua->IoStatus.Status = STATUS_CANCELLED;
							antigua->IoStatus.Information =0;
							IoCompleteRequest(antigua, IO_NO_INCREMENT);  
						}
					}
					IoSkipCurrentIrpStackLocation(Irp);
					status = IoCallDriver(devExt->TopOfStack, Irp);
				}
				break;
			case URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE:
				IoCopyCurrentIrpStackLocationToNext(Irp) ;
				IoSetCompletionRoutine(Irp,(PIO_COMPLETION_ROUTINE)ConfigDescriptorCompletion, NULL, TRUE, TRUE,TRUE) ;
				status = IoCallDriver(devExt->TopOfStack, Irp) ;
				if(status==STATUS_SUCCESS) *unlock = FALSE;
				break;
			case URB_FUNCTION_GET_DESCRIPTOR_FROM_INTERFACE:
		        IoCopyCurrentIrpStackLocationToNext(Irp) ;
				IoSetCompletionRoutine(Irp,(PIO_COMPLETION_ROUTINE)ReportDescriptorCompletion, NULL, TRUE,TRUE, TRUE) ;
				status = IoCallDriver(devExt->TopOfStack, Irp) ;
				if(status==STATUS_SUCCESS) *unlock = FALSE;
				break;
			default:
				IoSkipCurrentIrpStackLocation(Irp);
				status = IoCallDriver(devExt->TopOfStack, Irp);
		}
	} else {
		IoSkipCurrentIrpStackLocation(Irp);
		status = IoCallDriver(devExt->TopOfStack, Irp);
	}

	return status;
}

NTSTATUS ConfigDescriptorCompletion( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context )
{
	NTSTATUS ntStatus = Irp->IoStatus.Status;
    PDEVICE_EXTENSION  DeviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
    PURB pUrb = (PURB)IoGetCurrentIrpStackLocation(Irp)->Parameters.Others.Argument1;
    struct _URB_CONTROL_TRANSFER   *pControlTransfer = (struct _URB_CONTROL_TRANSFER *) pUrb;
    PUCHAR BufPtr=NULL;
   
	if (!NT_SUCCESS(ntStatus))
    {
        ntStatus = STATUS_SUCCESS;
	} else {    
		if (pUrb != NULL)
		{  
			if (pControlTransfer->TransferBuffer) {
			  BufPtr=(PUCHAR)pControlTransfer->TransferBuffer;
			} else if(pControlTransfer->TransferBufferMDL) {
			  BufPtr = (PUCHAR)MmGetSystemAddressForMdlSafe(pControlTransfer->TransferBufferMDL,HighPagePriority);
			}
			if (BufPtr!=NULL) {                
				// si es CONFIGURATION && (longitud USHOR, [6] y [7], >0)
				if ((pControlTransfer->SetupPacket[3]==0x02)&&((pControlTransfer->SetupPacket[6]>0)||(pControlTransfer->SetupPacket[7]>0)))
				{
					PHID_DESCRIPTOR pHD = NULL;
					USHORT pos;
					for(pos=0; pos<pControlTransfer->TransferBufferLength; pos=pos+BufPtr[pos]) {
						if(BufPtr[pos+1]==HID_HID_DESCRIPTOR_TYPE) {
							pHD = (PHID_DESCRIPTOR)&BufPtr[pos];
							break;
						}
					}
				  
				  if(pHD == NULL ) return STATUS_SUCCESS; // no encontrado
				  if( (ULONG)(pHD->bLength+pos) > pControlTransfer->TransferBufferLength) return STATUS_BUFFER_TOO_SMALL; //buffer pequeño
				     
				  pHD->DescriptorList[0].wReportLength = sizeof(reportDescriptor);                                      
				}
			}		
		}
	}
	if (ntStatus != STATUS_MORE_PROCESSING_REQUIRED)
    {
        if (Irp->PendingReturned)
        {
            IoMarkIrpPending(Irp);
        }
        IoReleaseRemoveLock(&DeviceExtension->removeLock, Irp);
    }

	return ntStatus;
}


NTSTATUS ReportDescriptorCompletion( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context )
{
	NTSTATUS ntStatus = Irp->IoStatus.Status;
    PDEVICE_EXTENSION DeviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
    PURB pUrb = (PURB)IoGetCurrentIrpStackLocation(Irp)->Parameters.Others.Argument1;
    struct _URB_CONTROL_TRANSFER *pControlTransfer = (struct _URB_CONTROL_TRANSFER *) pUrb;
    PVOID BufPtr=NULL;
    
    
	if (!NT_SUCCESS(ntStatus))
    {
        ntStatus = STATUS_SUCCESS;
	} else {
		if (pUrb != NULL)
		{
			if (pControlTransfer->TransferBuffer) {
			  BufPtr = pControlTransfer->TransferBuffer;
			} else if(pControlTransfer->TransferBufferMDL) {
			  BufPtr = MmGetSystemAddressForMdlSafe(pControlTransfer->TransferBufferMDL,HighPagePriority);
			}
			if (BufPtr!=NULL) {          
				if (pControlTransfer->SetupPacket[3]==HID_REPORT_DESCRIPTOR_TYPE) {      
				  // copy over modified report descriptor
				  RtlCopyMemory(BufPtr,reportDescriptor, sizeof(reportDescriptor));
				  pControlTransfer->TransferBufferLength=sizeof(reportDescriptor);     
				}
			}
		}
	}
	if (ntStatus != STATUS_MORE_PROCESSING_REQUIRED)
    {
        if (Irp->PendingReturned)
        {
            IoMarkIrpPending(Irp);
        }
        IoReleaseRemoveLock(&DeviceExtension->removeLock, Irp);
    }

	return ntStatus;
}

#define IOCTL_HID_PROCESAR		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0100, METHOD_BUFFERED, FILE_ANY_ACCESS)
NTSTATUS ReportCompletion( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context )
{
    NTSTATUS ntStatus = Irp->IoStatus.Status;
    PDEVICE_EXTENSION devExt = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
    PURB pUrb = (PURB)IoGetCurrentIrpStackLocation(Irp)->Parameters.Others.Argument1;
	struct _URB_BULK_OR_INTERRUPT_TRANSFER *pBulkOrInterruptTransfer = (struct _URB_BULK_OR_INTERRUPT_TRANSFER *) pUrb;
    UCHAR *bufPtr=NULL;

	if (!NT_SUCCESS(ntStatus))
    {
        ntStatus = STATUS_SUCCESS;
	} else {
		if (pUrb != NULL) {
			if (pBulkOrInterruptTransfer->TransferBuffer) {
			  bufPtr = (UCHAR*)pBulkOrInterruptTransfer->TransferBuffer;
			} else if(pBulkOrInterruptTransfer->TransferBufferMDL) {
			  bufPtr = (UCHAR*)MmGetSystemAddressForMdlSafe(pBulkOrInterruptTransfer->TransferBufferMDL,HighPagePriority);
			}
			if (bufPtr!=NULL) {
				if(pBulkOrInterruptTransfer->TransferBufferLength == 0x0e  || pBulkOrInterruptTransfer->TransferBufferLength == 0x0b) {
	//RtlCopyMemory(bufPtr,&devExt->ultimosDatosOK,sizeof(HID_INPUT_DATA));
	//pBulkOrInterruptTransfer->TransferBufferLength=sizeof(HID_INPUT_DATA);

					PREAD_WORKITEM rwi = ExAllocatePoolWithTag(NonPagedPool, sizeof(READ_WORKITEM),'iwFU');
					if(rwi!=NULL) {
						rwi->WorkItem = IoAllocateWorkItem(devExt->self);
						if(rwi->WorkItem!=NULL) {
							rwi->DeviceExtension = devExt;
							rwi->Irp = Irp;
							IoQueueWorkItem(rwi->WorkItem,PreReadReport,DelayedWorkQueue,rwi);
							ntStatus = STATUS_MORE_PROCESSING_REQUIRED;
						} else {
							ExFreePool(rwi);
						}
					} 
				}
			}
		}
	}
	if (ntStatus != STATUS_MORE_PROCESSING_REQUIRED)
    {
        if (Irp->PendingReturned)
        {
            IoMarkIrpPending(Irp);
        }
        IoReleaseRemoveLock(&devExt->removeLock, Irp);
    }

	return ntStatus;
}

VOID PreReadReport(IN PDEVICE_OBJECT DevObj,IN PVOID Context)
{
	PIO_WORKITEM wi = ((PREAD_WORKITEM)Context)->WorkItem;
	PIRP Irp =((PREAD_WORKITEM)Context)->Irp;
	PDEVICE_EXTENSION devExt = ((PREAD_WORKITEM)Context)->DeviceExtension;
	HANDLE hHilo = NULL;
	OBJECT_ATTRIBUTES oa;
	PREAD_CONTEXT ctx = NULL;

	ExFreePool(Context);

	PAGED_CODE();

	ctx = ExAllocatePoolWithTag(PagedPool, sizeof(READ_CONTEXT),'rrFU');
	if(ctx!=NULL) {
		KeInitializeEvent(&ctx->Ev, NotificationEvent, FALSE);
		ctx->DeviceExtension = devExt;
		ctx->Irp = Irp;
		InitializeObjectAttributes(&oa, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);

		if(PsCreateSystemThread(&hHilo,THREAD_ALL_ACCESS,&oa,NULL,NULL,ReadReport,ctx)==STATUS_SUCCESS) {
			ObReferenceObjectByHandle(hHilo,THREAD_ALL_ACCESS,NULL,KernelMode,&ctx->Hilo,NULL);
			KeSetEvent(&ctx->Ev,0,FALSE);
			ZwClose(hHilo);
		} else {
			ExFreePool(ctx);
	        IoReleaseRemoveLock(&devExt->removeLock, Irp);
		}
	} else {
        IoReleaseRemoveLock(&devExt->removeLock, Irp);
	}

	IoFreeWorkItem(wi);
}

VOID ReadReport(IN PVOID Context)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	PIRP Irp = ((PREAD_CONTEXT)Context)->Irp;
    PDEVICE_EXTENSION devExt = ((PREAD_CONTEXT)Context)->DeviceExtension;

	PURB pUrb = (PURB)IoGetCurrentIrpStackLocation(Irp)->Parameters.Others.Argument1;
	struct _URB_BULK_OR_INTERRUPT_TRANSFER *pBulkOrInterruptTransfer = (struct _URB_BULK_OR_INTERRUPT_TRANSFER *) pUrb;
    UCHAR *bufPtr=NULL;
	PETHREAD pHilo;

	PAGED_CODE();

	KeWaitForSingleObject(&((PREAD_CONTEXT)Context)->Ev,Executive,KernelMode,FALSE,NULL);
	pHilo = ((PREAD_CONTEXT)Context)->Hilo;

	ExFreePool(Context);
	KeSetPriorityThread(KeGetCurrentThread(), LOW_REALTIME_PRIORITY );

	if (pBulkOrInterruptTransfer->TransferBuffer) {
		bufPtr = (UCHAR*)pBulkOrInterruptTransfer->TransferBuffer;
	} else {
		bufPtr = (UCHAR*)MmGetSystemAddressForMdlSafe(pBulkOrInterruptTransfer->TransferBufferMDL,HighPagePriority);
	}

	Procesar(devExt, bufPtr, pBulkOrInterruptTransfer->TransferBufferLength);

	RtlCopyMemory(bufPtr,&devExt->ultimosDatosOK,sizeof(HID_INPUT_DATA));
	pBulkOrInterruptTransfer->TransferBufferLength=sizeof(HID_INPUT_DATA);
	IoCompleteRequest(Irp,IO_NO_INCREMENT);
	IoReleaseRemoveLock(&devExt->removeLock, Irp);
	ObDereferenceObject(pHilo);
	PsTerminateSystemThread(0);
}



VOID Procesar(IN PDEVICE_EXTENSION devExt, IN PUCHAR entrada, IN ULONG tamBuffer)
{
	HID_INPUT_DATA salida;

	PAGED_CODE();

	if(devExt->vfo!=NULL && devExt->vdo!=NULL) {
		RtlZeroMemory(&salida,sizeof(HID_INPUT_DATA));
		if(tamBuffer == 0x0b) { //X36,X45 requieren manipular los modos a 3 botones
			BOOLEAN cambio=FALSE;
			UCHAR buf[0x0b];
			RtlCopyMemory(buf,entrada,0x0b);
			if( devExt->stModos!=(entrada[8]&0x7) ) { buf[8]&=0x7c; cambio=TRUE; devExt->stModos=entrada[8]&0x7;}
			if( devExt->stAux!=((entrada[8]>>3)&0x7) ) { buf[8]&=0xc7; cambio=TRUE; devExt->stAux=(entrada[8]>>3)&0x7;}
			if(cambio) {
				KEVENT                  IoctlCompleteEvent;
				IO_STATUS_BLOCK         IoStatus;
				PIRP					pIrp = NULL;
				// Pasar al vmdf
				KeInitializeEvent(&IoctlCompleteEvent, NotificationEvent, FALSE);
				pIrp=IoBuildDeviceIoControlRequest(IOCTL_HID_PROCESAR,devExt->vdo,buf,0x0b,&salida,sizeof(HID_INPUT_DATA),TRUE,&IoctlCompleteEvent,&IoStatus);
				if(pIrp!=NULL) IoCallDriver(devExt->vdo, pIrp);
			}
		} else { //x52 asegurar que primero se suelta el modo y luego se pulsa
			BOOLEAN cambio=FALSE;
			UCHAR buf[0x0e];
			RtlCopyMemory(buf,entrada,0x0e);
			if( devExt->stModos!=(((entrada[11]&3)<<1)|(entrada[10]>>7)) ) {
				buf[10]&=0x7f; buf[11]&=0xfc;
				cambio=TRUE;
				devExt->stModos=(((entrada[11]&3)<<1)|(entrada[10]>>7));
			}
			if(cambio) {
				KEVENT                  IoctlCompleteEvent;
				IO_STATUS_BLOCK         IoStatus;
				PIRP					pIrp = NULL;
				// Pasar al vmdf
				KeInitializeEvent(&IoctlCompleteEvent, NotificationEvent, FALSE);
				pIrp=IoBuildDeviceIoControlRequest(IOCTL_HID_PROCESAR,devExt->vdo,buf,0x0e,&salida,sizeof(HID_INPUT_DATA),TRUE,&IoctlCompleteEvent,&IoStatus);
				if(pIrp!=NULL) IoCallDriver(devExt->vdo, pIrp);
			}
		}
		{
			KEVENT                  IoctlCompleteEvent;
			IO_STATUS_BLOCK         IoStatus;
			PIRP					pIrp = NULL;
			// Pasar al vmdf
			KeInitializeEvent(&IoctlCompleteEvent, NotificationEvent, FALSE);
			pIrp=IoBuildDeviceIoControlRequest(IOCTL_HID_PROCESAR,devExt->vdo,entrada,tamBuffer,&salida,sizeof(HID_INPUT_DATA),TRUE,&IoctlCompleteEvent,&IoStatus);
			if(pIrp!=NULL) {
				IoCallDriver(devExt->vdo, pIrp);
				RtlCopyMemory(&devExt->ultimosDatosOK,&salida,sizeof(HID_INPUT_DATA));
			}
		}
	}
}
