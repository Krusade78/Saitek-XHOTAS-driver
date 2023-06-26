#include <wdm.h>
#include "hidfilter.h"
#include "ejes.h"
#include "botones.h"

#define _DIRECTX_
#include "directx.h"
#undef _DIRECTX_

UCHAR Switch4To8(UCHAR in);

#ifdef ALLOC_PRAGMA
    #pragma alloc_text( PAGE, HF_IoWrite)
    #pragma alloc_text( PAGE, Switch4To8)
    #pragma alloc_text( PAGE, ProcesarWrite)
#endif /* ALLOC_PRAGMA */

HF_IoWrite(
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
    Irp->IoStatus.Information = 0;

	if(irpStack->Parameters.DeviceIoControl.InputBufferLength==sizeof(HIDX36_INPUT_DATA) && irpStack->Parameters.DeviceIoControl.OutputBufferLength==sizeof(HID_INPUT_DATA) )
	{
		ProcesarWrite(devExt->vDevExt,Irp->AssociatedIrp.SystemBuffer,Irp->AssociatedIrp.SystemBuffer,FALSE);
	    Irp->IoStatus.Information = sizeof(HID_INPUT_DATA);
		Irp->IoStatus.Status = status;
	    IoCompleteRequest(Irp, IO_NO_INCREMENT);
	} else if(irpStack->Parameters.DeviceIoControl.InputBufferLength==sizeof(HIDX52_INPUT_DATA) && irpStack->Parameters.DeviceIoControl.OutputBufferLength==sizeof(HID_INPUT_DATA) ) {
		ProcesarWrite(devExt->vDevExt,Irp->AssociatedIrp.SystemBuffer,Irp->AssociatedIrp.SystemBuffer,TRUE);
	    Irp->IoStatus.Information = sizeof(HID_INPUT_DATA);
		Irp->IoStatus.Status = status;
	    IoCompleteRequest(Irp, IO_NO_INCREMENT);
	} else {
		status=STATUS_INVALID_BUFFER_SIZE;
		Irp->IoStatus.Status = status;
	    IoCompleteRequest(Irp, IO_NO_INCREMENT);
	}

	return status;
}

UCHAR Switch4To8(UCHAR in)
{
	PAGED_CODE();

	switch(in)
	{
		case 0: return 0;
		case 1: return 1;
		case 2: return 3;
		case 3: return 2;
		case 4: return 5;
		case 6: return 4;
		case 8: return 7;
		case 9: return 8;
		case 12: return 6;
		default: return 0;
	}
}
VOID 
ProcesarWrite(   
    PDEVICE_EXTENSION devExt,
	PVOID inputData,
	PHID_INPUT_DATA outputData,
	BOOLEAN esX52
    )
{
	PITFDEVICE_EXTENSION idevExt = (PITFDEVICE_EXTENSION)devExt->itfObj->DeviceExtension;
	KIRQL irql;
	UCHAR idx;
	UCHAR cambios;

	HID_INPUT_DATA viejohidData;
	HID_INPUT_DATA hidData;

	PAGED_CODE();

	RtlZeroMemory(&hidData,sizeof(HID_INPUT_DATA));

	// Traducir a formato común
	if(!esX52) {
		HIDX36_INPUT_DATA hidGameData;
		RtlCopyMemory(&hidGameData,inputData,sizeof(HIDX36_INPUT_DATA));
		hidData.Ejes[0]=hidGameData.EjesXY[0];
		hidData.Ejes[1]=hidGameData.EjesXY[1]&0x0f;
		hidData.Ejes[2]=(hidGameData.EjesXY[1]>>4)|((hidGameData.EjesXY[2]&0x0f)<<4);
		hidData.Ejes[3]=hidGameData.EjesXY[2]>>4;
		hidData.Ejes[4]=hidGameData.Ejes[0];
		hidData.Ejes[6]=255-hidGameData.Ejes[1];
		hidData.Ejes[8]=hidGameData.Ejes[2];
		hidData.Ejes[10]=hidGameData.Ejes[3];
		hidData.Botones[0]=hidGameData.Botones[0];
		hidData.Botones[1]=hidGameData.Botones[1]&0x3f;
		hidData.Setas[0]=hidGameData.Setas[1]>>4;
		hidData.Setas[1]=Switch4To8( (hidGameData.Botones[1]>>6)|((hidGameData.Setas[0]&0x3)<<2) );
		hidData.Setas[2]=Switch4To8( (hidGameData.Setas[0]&0x3c)>>2 );
		hidData.Setas[3]=Switch4To8( (hidGameData.Setas[0]>>6)|((hidGameData.Setas[1]&0x3)<<2) );
		switch(hidData.Setas[3]) {
			case 0:
				hidData.MiniStick=0x88; break;
			case 1:
				hidData.MiniStick=0x08; break;
			case 2:
				hidData.MiniStick=0x0f; break;
			case 3:
				hidData.MiniStick=0x8f; break;
			case 4:
				hidData.MiniStick=0xff; break;
			case 5:
				hidData.MiniStick=0xf8; break;
			case 6:
				hidData.MiniStick=0xf0; break;
			case 7:
				hidData.MiniStick=0x80; break;
			case 8:
				hidData.MiniStick=0; break;
		}
	} else {
		HIDX52_INPUT_DATA hidGameData;
		RtlCopyMemory(&hidGameData,inputData,sizeof(HIDX52_INPUT_DATA));
		hidData.Ejes[0]=hidGameData.EjesXYR[0];
		hidData.Ejes[1]=hidGameData.EjesXYR[1]&0x7;
		hidData.Ejes[2]=(hidGameData.EjesXYR[1]>>3)|((hidGameData.EjesXYR[2]&0x7)<<5);
		hidData.Ejes[3]=(hidGameData.EjesXYR[2]>>3)&0x7;
		hidData.Ejes[4]=(hidGameData.EjesXYR[2]>>6)|((hidGameData.EjesXYR[3]&0x3f)<<2);
		hidData.Ejes[5]=hidGameData.EjesXYR[3]>>6;
		hidData.Ejes[6]=255-hidGameData.Ejes[0];
		hidData.Ejes[8]=hidGameData.Ejes[2];
		hidData.Ejes[10]=hidGameData.Ejes[1];
		hidData.Ejes[12]=hidGameData.Ejes[3];
		hidData.Botones[0]=((hidGameData.Botones[1]>>6)&1) | ((hidGameData.Botones[0]>>1)&6) | ((hidGameData.Botones[0]<<2)&8) | ((hidGameData.Botones[0]>>2)&16) | ((hidGameData.Botones[3]>>1)&32) | ((hidGameData.Botones[0]<<1)&64) | ((hidGameData.Botones[0]<<3)&128);
		hidData.Botones[1]=((hidGameData.Botones[2]&0x80)>>7) | ((hidGameData.Botones[3]<<1)&126) | (hidGameData.Botones[0]&128);
		hidData.Botones[2]=(hidGameData.Botones[1]&0x3f) | ((hidGameData.Botones[0]&1)<<6) | (hidGameData.Botones[3]&0x80);
		hidData.Botones[3]=hidGameData.Seta&0x3;
		hidData.Setas[0]=hidGameData.Seta>>4;
		hidData.Setas[1]=Switch4To8( (hidGameData.Botones[1]>>7)+((hidGameData.Botones[2]<<1)&0xf) );
		hidData.Setas[2]=Switch4To8( (hidGameData.Botones[2]>>3)&0xf );
		switch(hidGameData.Ministick&0xf) {
			case 0:
				hidData.Setas[3]=8;
				break;
			case 0xf:
				hidData.Setas[3]=2;
				break;
			default: hidData.Setas[3]=0;
		}
		switch(hidGameData.Ministick>>4) {
			case 0:
				hidData.Setas[3]|=1;
				break;
			case 0xf:
				hidData.Setas[3]|=4;
				break;
		}
		hidData.Setas[3]=Switch4To8( hidData.Setas[3] );
		hidData.MiniStick=hidGameData.Ministick;
	}

	// Calibrar
	RtlCopyMemory(&viejohidData,&idevExt->DeltaHidData,sizeof(HID_INPUT_DATA));
	if(!idevExt->descalibrar) Calibrado(idevExt, &hidData, esX52);
	RtlCopyMemory(&idevExt->DeltaHidData,&hidData,sizeof(HID_INPUT_DATA));

	if(!idevExt->descalibrar) {
		// Botones

		for(idx=0;idx<4;idx++) {
			cambios=hidData.Botones[idx]^viejohidData.Botones[idx];
			if(cambios!=0) {
				UCHAR exp;
				for(exp=0;exp<8;exp++) {
					if((cambios>>exp)&1) { // Si ha cambiado
						if((hidData.Botones[idx]>>exp)&1) {
							AccionPulsarBoton(devExt,(idx*8)+exp);
						} else {
							AccionSoltarBoton(devExt,(idx*8)+exp);
						}
					}
				}
			}
		}

		// Setas

		for(idx=0;idx<4;idx++) {
			if(hidData.Setas[idx]!=viejohidData.Setas[idx]) {
				if(viejohidData.Setas[idx]!=0) AccionSoltarSeta(devExt,(idx*8)+viejohidData.Setas[idx]-1);
				if(hidData.Setas[idx]!=0) AccionPulsarSeta(devExt,(idx*8)+hidData.Setas[idx]-1);
			}
		}

		// Ejes

		for(idx=0;idx<7;idx++) {
			if( *((USHORT*)&hidData.Ejes[idx*2])!= *((USHORT*)&viejohidData.Ejes[idx*2]) ) {
				AccionEje(devExt, esX52, idx, *((USHORT*)&hidData.Ejes[idx*2]) );
			}
		}

		// Sensibilidad y mapeado
		RtlZeroMemory(outputData,sizeof(HID_INPUT_DATA));
		SensibilidadYMapeado(idevExt,esX52,&hidData,outputData);

		KeAcquireSpinLock(&devExt->slGameData,&irql);
			RtlCopyMemory(outputData->Setas,devExt->stHidData.Setas,11);
			RtlCopyMemory(&devExt->stHidData,outputData,sizeof(HID_INPUT_DATA)); // ejes
		KeReleaseSpinLock(&devExt->slGameData,irql);

	} else {
		RtlCopyMemory(outputData,&hidData,sizeof(HID_INPUT_DATA));
	}

}


