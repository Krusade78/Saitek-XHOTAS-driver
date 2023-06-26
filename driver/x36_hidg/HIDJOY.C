/*++

Copyright (c) 2005-2207 Alfredo Costalago

Module Name:

    pnp.c

Abstract: This module contains routines Generate the HID report and
    configure the joystick.

Environment:

    Kernel mode


--*/

#include <wdm.h>
#include <hidport.h>
#include "hidtoken.h"
#include <hidusage.h>
#include "hidgame.h"
#include "i386\timing.h"
#define _HIDJOYC_
#include "hidjoy.h"


#ifdef ALLOC_PRAGMA
    #pragma alloc_text (PAGE, HGM_MapAxesFromDevExt)
    #pragma alloc_text (PAGE, HGM_GenerateReport)
#endif

/*****************************************************************************
 *
 *  @func   NTSTATUS  | HGM_MapAxesFromDevExt |
 *
 *          Use the flags in the DeviceExtension to generate mappings for each 
 *          axis.  
 *          <nl>This is called both from HGM_JoystickConfig to validate the 
 *          configuration and HGM_GenerateReport to use the axis maps.
 *          
 *
 *  @parm   IN OUT PDEVICE_EXTENSION | DeviceExtension |
 *
 *          Pointer to the minidriver device extension
 *
 *  @rvalue   STATUS_SUCCESS | success
 *  @rvalue   STATUS_DEVICE_CONFIGURATION_ERROR | The configuration is invalid
 *
 *****************************************************************************/
NTSTATUS HGM_MapAxesFromDevExt
    (
    IN OUT PDEVICE_EXTENSION DeviceExtension 
    )
{
#include "analog_def.h"
//=====================Definiciones==============================

const unsigned char XLU[8] = { X1,Y1,X2,NA,Y2,NA,NA,NA };
const unsigned char YLU[8] = { Y1,X1,X2,NA,Y2,NA,NA,NA };
const unsigned char RLU[8] = { X2,Y1,X1,NA,Y2,NA,NA,NA };
const unsigned char ZLU[8] = { Y2,Y1,X1,NA,X2,NA,NA,NA };
const unsigned char cAxisIndexTable[9] = { 0, 0, 1, 0, 2, 0, 0, 0, 3 };

//==============================================================
    NTSTATUS    ntStatus;
    ULONG       dwFlags;
    int         nAxis;
    UCHAR       AxisMask;

    ntStatus = STATUS_SUCCESS;

    dwFlags = DeviceExtension->HidGameOemData.OemData[(DeviceExtension->fSiblingFound!=0)].joy_hws_dwFlags;  

    /* 
     *  Check X and Y last as Z, R and POV must not overlap
     *  The are no flags to indicate the presence of X or Y so if they 
     *  overlap, this indicates that they are not used,
     */

    DeviceExtension->resistiveInputMask = 0;
    for( nAxis=0; nAxis< 4; nAxis++ )
    {
        DeviceExtension->AxisMap[nAxis] = INVALID_INDEX;
    }
    nAxis = 0;

    if( dwFlags & JOY_HWS_HASZ )
    {
        AxisMask = ZLU[(dwFlags & ZMAPBITS) >> ZMAPSHFT];
        if( AxisMask >= NA )
        {
            ntStatus = STATUS_DEVICE_CONFIGURATION_ERROR; 
        }
        else
        {
            nAxis = 1;
            DeviceExtension->resistiveInputMask = AxisMask;
            DeviceExtension->AxisMap[ZIS] = cAxisIndexTable[AxisMask];
        }
    }


    if( dwFlags & JOY_HWS_HASR )
    {
        AxisMask = RLU[(dwFlags & RMAPBITS) >> RMAPSHFT];
        if( AxisMask >= NA )
        {
            ntStatus = STATUS_DEVICE_CONFIGURATION_ERROR; 
        }
        else
        {
            if( DeviceExtension->resistiveInputMask & AxisMask )
            {
                ntStatus = STATUS_DEVICE_CONFIGURATION_ERROR ; 
            }
            else
            {
                nAxis++;
                DeviceExtension->resistiveInputMask |= AxisMask;
                DeviceExtension->AxisMap[RIS] = cAxisIndexTable[AxisMask];
            }
        }
    }

    AxisMask = XLU[( dwFlags & XMAPBITS ) >> XMAPSHFT];
    if( AxisMask >= NA )
    {
        ntStatus = STATUS_DEVICE_CONFIGURATION_ERROR ; 
    }
    else
    {
        if( DeviceExtension->resistiveInputMask & AxisMask )
        {
            //("HGM_MapAxesFromDevExt: X axis mapped to same as another axis") );
        }
        else
        {
            nAxis++;
            DeviceExtension->resistiveInputMask |= AxisMask;
            DeviceExtension->AxisMap[XIS] = cAxisIndexTable[AxisMask];
        }
    }

    AxisMask = YLU[( dwFlags & YMAPBITS ) >> YMAPSHFT];
    if( AxisMask >= NA )
    {
        ntStatus = STATUS_DEVICE_CONFIGURATION_ERROR ; 
    }
    else
    {
        if( DeviceExtension->resistiveInputMask & AxisMask )
        {
            //("HGM_MapAxesFromDevExt: Y axis mapped to same as another axis") );
        }
        else
        {
            nAxis++;
            DeviceExtension->resistiveInputMask |= AxisMask;
            DeviceExtension->AxisMap[YIS] = cAxisIndexTable[AxisMask];
        }
    }

    if( nAxis != DeviceExtension->nAxes )
    {
        ntStatus = STATUS_DEVICE_CONFIGURATION_ERROR ;
    }

#include "analog_udef.h"

    return( ntStatus );
} /* HGM_MapAxesFromDevExt */










/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   NTSTATUS  | HGM_GenerateReport |
 *
 *          Generates a hid report descriptor for a n-axis, m-button joystick,
 *          depending on number of buttons and joy_hws_flags field.
 *
 *  @parm   IN PDEVICE_OBJECT | DeviceObject |
 *
 *          Pointer to the device object
 *
 *  @parm   IN OUT UCHAR * | rgGameReport[MAXBYTES_GAME_REPORT] |
 *
 *          Array that receives the HID report descriptor
 *
 *  @parm   OUT PUSHORT | pCbReport |
 *          
 *          Address of a short integer that receives size of 
 *          HID report descriptor. 
 *
 *  @rvalue   STATUS_SUCCESS  | success
 *  @rvalue   STATUS_BUFFER_TOO_SMALL  | Need more memory for HID descriptor
 *
 *****************************************************************************/
NTSTATUS HGM_GenerateReport
    (
    IN PDEVICE_OBJECT       DeviceObject, 
    OUT UCHAR               rgGameReport[MAXBYTES_GAME_REPORT],
	  OUT PUSHORT             pCbReport
    )
{
    NTSTATUS    ntStatus;
    PDEVICE_EXTENSION DeviceExtension;
    int Idx = 0;
	int UsageIdx;
    int InitialAxisMappings[4];
	UCHAR reportDescriptor[] = {
		0x05, 0x01,
		0x09, 0x04,
		0xa1, 0x01,
		0x15, 0x00,
		0x27, 0xff, 0xff, 0xff, 0xff,
		0x95, 0x01,
		0x75, 0x20,
		0x09, 0x30,  //X
		0x81, 0x02,
		0x09, 0x31,  //Y
		0x81, 0x02,
		0x09, 0x35,	//Rudder
		0x81, 0x02,
		0x09, 0x32,  //throttle
		0x81, 0x02,
		//0x15, 0x00,
		0x26, 0xff,0x0f,
		0x75, 0x10,
		0x09, 0x33,  //Rx
		0x81, 0x02,
		0x09, 0x34,  //Ry
		0x81, 0x02,
		0x09, 0x36,  //Slider
		0x81, 0x02,

		0x15, 0x01,
		0x25, 0x08,
		0x75, 0x08,
		0x35, 0x00,
		0x46, 0x3b, 0x01,
		0x65, 0x14,
		0x09, 0x39, //Hat1
		0x81, 0x42,
		0x09, 0x39, //Hat2
		0x81, 0x42,
		0x09, 0x39, //Hat3
		0x81, 0x42,
		0x09, 0x39, //Hat4
		0x81, 0x42,

		0x05,0x09,			//botones
		0x19,0x01,
		0x29,0x38,
		0x15,0x00,
		0x25,0x01,
		0x75,0x01,
		0x95,0x38,
		0x81,0x02,

		//0x05,0x01,0x09,0x05, //ministick
		//0xa1,0x00,
		//	0x09,0x30,0x09,0x31, //X Y
		//	0x15,0x00,
		//	0x25,0x0f,
		//	0x75,0x04,
		//	0x95,0x02, 
		//	0x81,0x02,
		//0xc0,
		0x05,0x01, //ministick
			0x15,0x00,
			0x25,0x0f,
			0x35,0x00,
			0x45,0x0f,
			0x95,0x01,
			0x75,0x04,
			0x66,0x11,0xf0, //unit velicity
			0x09,0x30, //vx
			0x81,0x02,
			0x09,0x31, //vy
			0x81,0x02,

		0xc0
	};

    PAGED_CODE();

    /*
     *  Get a pointer to the device extension
     */

    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);

    /*
     *  Although the axes have already been validated and mapped in 
     *  HGM_JoystickConfig this function destroys the mapping when it compacts 
     *  the axes towards the start of the descriptor report.  Since this 
     *  function will be called once to find the descriptor length and then 
     *  again to read the report, the mappings are regenerated again each 
     *  time through.  Although this results in the parameters being 
     *  interpreted three times (for validation, descriptor size and 
     *  descriptor content) it avoids the possibility of a discrepancy in 
     *  implementation of separate functions.
     */

    /*ntStatus =*/ HGM_MapAxesFromDevExt( DeviceExtension );

    /* 
     *  Do the axis 
     *  Although HID could cope with the "active" axes being mixed with the 
     *  dummy ones, it makes life simpler to move them to the start.
     *  Pass through all the axis maps generated by HGM_JoystickConfig 
     *  and map all the active ones into the descriptor, copying the usages 
     *  appropriate for the type of device.
     *  Since a polled POV is nothing more than a different interpretation 
     *  of axis data, this is added after any axes.
     */
    RtlCopyMemory( InitialAxisMappings, DeviceExtension->AxisMap, sizeof( InitialAxisMappings ) );

    for( UsageIdx = 0; UsageIdx < 4; UsageIdx++ )
    {
        if( InitialAxisMappings[UsageIdx] >= INVALID_INDEX )
        {
            continue;
        }

        DeviceExtension->AxisMap[Idx] = InitialAxisMappings[UsageIdx];
		Idx++;
	}

	RtlCopyMemory(rgGameReport, reportDescriptor, sizeof(reportDescriptor));
	*pCbReport = (USHORT)sizeof(reportDescriptor);

	ntStatus = STATUS_SUCCESS;

    return ( ntStatus );
} /* HGM_GenerateReport */





/*****************************************************************************
 *
 *  @func   VOID  | HGM_Game2HID |
 *
 *          Process the data returned from polling the gameport into values 
 *          and buttons for returning to HID.
 *          <nl>The meaning of the data is interpreted according to the 
 *          characteristics of the device described in the hardware settings
 *          flags.
 *
 *  @parm   IN PDEVICE_OBJECT | DeviceObject |
 *
 *          Pointer to the device object
 *
 *  @parm   IN      PDEVICE_EXTENSION | DeviceExtension | 
 *
 *          Pointer to the mini-driver device extension.
 *
 *  @parm   IN  OUT PUHIDGAME_INPUT_DATA | pHIDData | 
 *
 *          Pointer to the buffer into which the HID report should be written.
 *          This buffer must be assumed to be unaligned.
 *
 *****************************************************************************/
VOID HGM_Game2HID
    (
    IN PDEVICE_OBJECT      DeviceObject,
    IN  OUT PUHIDGAME_INPUT_DATA    pHIDData
    )
{
	PDEVICE_EXTENSION DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);

    /*
     *  Use a local buffer to assemble the report as the real buffer may not 
     *  be aligned.
     */
	HIDGAME_INPUT_DATA aux;

	RtlCopyMemory(&aux,&DeviceExtension->ultimosDatosOK,sizeof(HIDGAME_INPUT_DATA));

	if(!DeviceExtension->descalibrar) {	
		HGM_Coger_FiltrarEjes(DeviceObject,&aux);
	} else {
		UCHAR idx;
		ULONG pollEje[4];
		for(idx=0;idx<4;idx++) {
			if(DeviceExtension->LastGoodAxis[DeviceExtension->AxisMap[idx]]<ANALOG_POLL_TIMEOUT_MAX) pollEje[idx]=DeviceExtension->LastGoodAxis[DeviceExtension->AxisMap[idx]];
		}
		RtlCopyMemory(aux.EjesXYZR,pollEje,16);
	}
	HGM_CogerDatosX36(DeviceObject,&aux);

	RtlCopyMemory(pHIDData,&aux,sizeof(HIDGAME_INPUT_DATA));
	RtlCopyMemory(&DeviceExtension->ultimosDatosOK,&aux,sizeof(HIDGAME_INPUT_DATA));

	HGM_ComprobarEstado(DeviceObject);

} /* HGM_Game2HID */


VOID HGM_Coger_FiltrarEjes
    (
    IN      PDEVICE_OBJECT       DeviceObject,
	IN  OUT PHIDGAME_INPUT_DATA    pHIDData
    )
{
	PDEVICE_EXTENSION devExt = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);
	UCHAR idx;
	ULONG pollEje[4];

	// Filtrado de ejes
	for(idx=0;idx<4;idx++) {
		pollEje[idx]=devExt->LastGoodAxis[devExt->AxisMap[idx]];
#ifdef PRO
		if(devExt->jitter[idx].antiv) {
			// Antivibraciones
			if( (pollEje[idx]<(devExt->jitter[idx].PosElegida-devExt->jitter[idx].Margen)) || (pollEje[idx]>(devExt->jitter[idx].PosElegida+devExt->jitter[idx].Margen)) ) {
				devExt->jitter[idx].PosRepetida=0;
				devExt->jitter[idx].PosElegida=pollEje[idx];
			} else {
				if(devExt->jitter[idx].PosRepetida<devExt->jitter[idx].Resistencia) {
					devExt->jitter[idx].PosRepetida++;
					pollEje[idx]=devExt->jitter[idx].PosElegida;
				} else {
					devExt->jitter[idx].PosRepetida=0;
					devExt->jitter[idx].PosElegida=pollEje[idx];
				}
			}
		}
#endif
		if(devExt->limites[idx].cal) {
			// Calibrado
			ULONG ancho1,ancho2;
			ancho1=(devExt->limites[idx].c-devExt->limites[idx].n)-devExt->limites[idx].i;
			ancho2=devExt->limites[idx].d-(devExt->limites[idx].c+devExt->limites[idx].n);
			if(pollEje[idx]==0 ||((pollEje[idx]>=(devExt->limites[idx].c-devExt->limites[idx].n)) && (pollEje[idx]<=(devExt->limites[idx].c+devExt->limites[idx].n)) ))
			{
				if(idx==0 || idx==1) {
					pollEje[idx]=2047; continue;
				} else {
					pollEje[idx]=127; continue;
				}
			} else {
		        if(pollEje[idx]<devExt->limites[idx].i)
				{
					pollEje[idx]=devExt->limites[idx].i;
				} else {
					if(pollEje[idx]>devExt->limites[idx].d)
					{
						pollEje[idx]=devExt->limites[idx].d;
					}
				}
				if(pollEje[idx]<devExt->limites[idx].c)
				{
					pollEje[idx]=((devExt->limites[idx].c-devExt->limites[idx].n)-pollEje[idx]);
					if(ancho1>ancho2)
					{
						pollEje[idx]=(pollEje[idx]*ancho2)/ancho1;
					}
					pollEje[idx]=0-pollEje[idx];
				} else {
					pollEje[idx]-=(devExt->limites[idx].c+devExt->limites[idx].n);
					if(ancho2>ancho1)
					{
						pollEje[idx]=(pollEje[idx]*ancho1)/ancho2;
					}
				}
			}
			if(ancho1>ancho2) {
				pollEje[idx]+=ancho2;
				if(idx==0 || idx==1) {// Convertir X e Y en rango (0-4095)
					unsigned __int64 prePosicion;
					prePosicion=pollEje[idx]*4095;
					pollEje[idx] = (USHORT) ( prePosicion/(ancho2*2) );
				} else { // Convertir R y Z en rango (0-255)
					unsigned __int64 prePosicion;
					prePosicion=pollEje[idx]*255;
					pollEje[idx] = (USHORT) ( prePosicion/(ancho2*2) );
				}
			} else {
				pollEje[idx]+=ancho1;
				if(idx==0 || idx==1) {// Convertir X e Y en rango (0-4095)
					unsigned __int64 prePosicion;
					prePosicion=pollEje[idx]*4095;
					pollEje[idx] = (USHORT) ( prePosicion/(ancho1*2) );
				} else { // Convertir R y Z en rango (0-255)
					unsigned __int64 prePosicion;
					prePosicion=pollEje[idx]*255;
					pollEje[idx] = (USHORT) ( prePosicion/(ancho1*2) );
				}
			}
		} else {
			if(idx==0 || idx==1) {// Convertir X e Y en rango (0-4095)
				pollEje[idx] = 2047;
			} else { // Convertir R y Z en rango (0-255)
				pollEje[idx] = 127;
			}
		}

	}

	//if(devExt->limites[0].cal && devExt->limites[1].cal) {// Convertir X e Y en rango (0-4095) y 12 bits
		//SHORT pollEje[2];
		//unsigned __int64 prePosicion[2];

		//prePosicion[0]=*((ULONG*)pHIDData->EjesXYZR);
		//prePosicion[1]=*((ULONG*)&pHIDData->EjesXYZR[4]);

		//if(prePosicion[0]>devExt->maxAxis[0]) devExt->maxAxis[0]=(ULONG)prePosicion[0];
		//if(prePosicion[0]<devExt->minAxis[0]) devExt->minAxis[0]=(ULONG)prePosicion[0];

		//if(prePosicion[1]<devExt->minAxis[1]) devExt->minAxis[1]=(ULONG)prePosicion[1];
		//if(prePosicion[1]>devExt->maxAxis[1]) devExt->maxAxis[1]=(ULONG)prePosicion[1];

		//prePosicion[0]=(prePosicion[0]-devExt->minAxis[0])*255; 
		//prePosicion[1]=(prePosicion[1]-devExt->minAxis[1])*255;
		//if((devExt->maxAxis[0]-devExt->minAxis[0])<1) { pollEje[0]=0; } else {
			//pollEje[0] = (USHORT) (prePosicion[0]/(devExt->maxAxis[0]-devExt->minAxis[0]));
		//}
		//if((devExt->maxAxis[1]-devExt->minAxis[1])<1) { pollEje[1]=0; } else {
			//pollEje[1] = (USHORT) (prePosicion[1]/(devExt->maxAxis[1]-devExt->minAxis[1]));
		//}
	//}
	//if(devExt->nAxes==4) { // Convertir R y Z en rango (0-255)
	//	UCHAR pollEje[2];
	//	unsigned __int64 prePosicion[2];

	//	prePosicion[0]=*((ULONG*)pHIDData->EjesXYZR[8]);
	//	prePosicion[1]=*((ULONG*)&pHIDData->EjesXYZR[12]);

	//	if(prePosicion[0]>devExt->maxAxis[2]) devExt->maxAxis[2]=(ULONG)prePosicion[0];
	//	if(prePosicion[0]<devExt->minAxis[2]) devExt->minAxis[2]=(ULONG)prePosicion[0];

	//	if(prePosicion[1]<devExt->minAxis[3]) devExt->minAxis[3]=(ULONG)prePosicion[1];
	//	if(prePosicion[1]>devExt->maxAxis[3]) devExt->maxAxis[3]=(ULONG)prePosicion[1];

	//	prePosicion[0]=(prePosicion[0]-devExt->minAxis[2])*255; 
	//	prePosicion[1]=(prePosicion[1]-devExt->minAxis[3])*255;
	//	if((devExt->maxAxis[2]-devExt->minAxis[2])<1) { hidExterno.Ejes[0]=0; } else {
	//		hidExterno.Ejes[0] = (UCHAR) (prePosicion[0]/(devExt->maxAxis[2]-devExt->minAxis[2]));
	//	}
	//	if((devExt->maxAxis[3]-devExt->minAxis[3])<1) { hidExterno.Ejes[1]=0; } else {
	//		hidExterno.Ejes[1] = (UCHAR) (prePosicion[1]/(devExt->maxAxis[3]-devExt->minAxis[3]));
	//	}
	//} else {
	//	hidExterno.Ejes[0]=hidExterno.Ejes[1]=0;
	//}

	RtlCopyMemory(pHIDData->EjesXYZR,pollEje,16);
}

/************************************************************+
				LLamadas externas
*************************************************************/

#define IOCTL_WRAPPER_LEERX36	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0100, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_HID_PROCESAR		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0100, METHOD_BUFFERED, FILE_ANY_ACCESS)
VOID HGM_CogerDatosX36
    (
    IN      PDEVICE_OBJECT       DeviceObject,
    IN  OUT PHIDGAME_INPUT_DATA    pHIDData
    )
{
	PDEVICE_EXTENSION devExt = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);
	if(devExt->wfo!=NULL && devExt->wdo!=NULL && KeGetCurrentIrql()<=APC_LEVEL)
	{
		NTSTATUS                ntStatus = STATUS_SUCCESS;
		KEVENT                  IoctlCompleteEvent;
		IO_STATUS_BLOCK         IoStatus;
		PIRP					pIrp = NULL;
		struct {
			UCHAR   EjesXY[3];
			UCHAR   Ejes[4];
			UCHAR	Botones[2]; // El ultimo byte se comparte con setas (2 bits)
			UCHAR	Setas[2];
		} hidExterno;
		struct {
			UCHAR   Ejes[2];
			UCHAR	Botones[2]; // El ultimo byte se comparte con setas (2 bits)
			UCHAR   Setas[2];
		} buffer1;
		struct {
			UCHAR   Ejes[14];
			UCHAR	Setas[4];
			UCHAR	Botones[7];
			UCHAR   MiniStick;
		} buffer2;

		KeInitializeEvent(&IoctlCompleteEvent, NotificationEvent, FALSE);
		pIrp=IoBuildDeviceIoControlRequest(IOCTL_WRAPPER_LEERX36,devExt->wdo,NULL,0,&buffer1,sizeof(buffer1),TRUE,&IoctlCompleteEvent,&IoStatus);
		if(pIrp!=NULL)
		{
			ntStatus = IoCallDriver(devExt->wdo, pIrp);
			//if (ntStatus == STATUS_PENDING) {
			//	KeWaitForSingleObject (&IoctlCompleteEvent,Executive,KernelMode,FALSE,NULL);
			//}
			RtlCopyMemory(hidExterno.Setas,buffer1.Setas,2);
			RtlCopyMemory(&hidExterno.Ejes[2],buffer1.Ejes,2);
			RtlCopyMemory(hidExterno.Botones,buffer1.Botones,2);

			if(!devExt->descalibrar) { // Convertir a 12 bits
				USHORT pollEje;
				pollEje=(USHORT)(*((ULONG*)pHIDData->EjesXYZR));
				hidExterno.EjesXY[0] =(UCHAR) (pollEje&0x00ff);
				hidExterno.EjesXY[1] =(UCHAR) ((pollEje>>8)&0x0f);
			} else {
				hidExterno.EjesXY[0] = hidExterno.EjesXY[1] = 0;
			}
			if(!devExt->descalibrar) { // Convertir a 12 bits
				USHORT pollEje;
				pollEje=(USHORT)(*((ULONG*)&pHIDData->EjesXYZR[4]));
				hidExterno.EjesXY[1] |=(UCHAR) ((pollEje&0x0F)<<4);
				hidExterno.EjesXY[2] = (UCHAR) ((pollEje>>4)&0x00ff);	
			} else {
				hidExterno.EjesXY[2] = 0;	
			}
			if(devExt->nAxes==4 && !devExt->descalibrar) { // Convertir R y Z a 1 byte
				hidExterno.Ejes[0]=pHIDData->EjesXYZR[8];
				hidExterno.Ejes[1]=pHIDData->EjesXYZR[12];
			} else {
				hidExterno.Ejes[0]=hidExterno.Ejes[1]=0;
			}

			if(devExt->vfo!=NULL && devExt->vdo!=NULL) {
				// Pasar al vmdf
				KeClearEvent(&IoctlCompleteEvent);
				pIrp=IoBuildDeviceIoControlRequest(IOCTL_HID_PROCESAR,devExt->vdo,&hidExterno,sizeof(hidExterno),&buffer2,sizeof(buffer2),TRUE,&IoctlCompleteEvent,&IoStatus);
				if(pIrp!=NULL)
				{
					ntStatus = IoCallDriver(devExt->vdo, pIrp);
					//if (ntStatus == STATUS_PENDING) {
					//	KeWaitForSingleObject (&IoctlCompleteEvent,Executive,KernelMode,FALSE,NULL);
					//}
					if(!devExt->descalibrar) {
						RtlCopyMemory(&pHIDData->EjesXYZR[0],buffer2.Ejes,2); pHIDData->EjesXYZR[2]=pHIDData->EjesXYZR[3]=0;
						RtlCopyMemory(&pHIDData->EjesXYZR[4],&buffer2.Ejes[2],2); pHIDData->EjesXYZR[6]=pHIDData->EjesXYZR[7]=0;
						RtlCopyMemory(&pHIDData->EjesXYZR[8],&buffer2.Ejes[4],2); pHIDData->EjesXYZR[10]=pHIDData->EjesXYZR[11]=0;
						RtlCopyMemory(&pHIDData->EjesXYZR[12],&buffer2.Ejes[6],2); pHIDData->EjesXYZR[14]=pHIDData->EjesXYZR[15]=0;
					}
					RtlCopyMemory(pHIDData->Ejes,&buffer2.Ejes[8],18); //el resto
				}
			}
		}
	}
}


#define IOCTL_WRAPPER_ESTADO	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0101, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_WRAPPER_CALIBRADO CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0102, METHOD_BUFFERED, FILE_READ_ACCESS)
VOID HGM_ComprobarEstado
(
    IN      PDEVICE_OBJECT       DeviceObject
)
{
	PDEVICE_EXTENSION devExt = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);

    if(devExt->wfo!=NULL && devExt->wdo!=NULL && KeGetCurrentIrql()<=APC_LEVEL)
	{
		NTSTATUS                ntStatus = STATUS_SUCCESS;
		KEVENT                  IoctlCompleteEvent;
		IO_STATUS_BLOCK         IoStatus;
		PIRP                    pIrp;
		CHAR					buffer=0;

		KeInitializeEvent(&IoctlCompleteEvent, NotificationEvent, FALSE);
		pIrp=IoBuildDeviceIoControlRequest(IOCTL_WRAPPER_ESTADO,devExt->wdo,NULL,0,&buffer,1,TRUE,&IoctlCompleteEvent,&IoStatus);
		if(pIrp!=NULL)
		{
			ntStatus = IoCallDriver(devExt->wdo, pIrp);
			//if (ntStatus == STATUS_PENDING) {
			//	KeWaitForSingleObject (&IoctlCompleteEvent,Executive,KernelMode,FALSE,NULL);
			//}
			if((buffer&2)==2) {
				//devExt->nuevaCalibracion=TRUE;
				struct {
					ULONG i;
					ULONG c;
					ULONG d;
					UCHAR n;
					UCHAR Margen;
					UCHAR Resistencia;
					BOOLEAN cal;
					BOOLEAN antiv;
				} bufCal[4];
				pIrp=IoBuildDeviceIoControlRequest(IOCTL_WRAPPER_CALIBRADO,devExt->wdo,NULL,0,&bufCal,sizeof(bufCal),TRUE,&IoctlCompleteEvent,&IoStatus);
				if(pIrp!=NULL) {
					UCHAR i;
					ntStatus = IoCallDriver(devExt->wdo, pIrp);
					for(i=0; i<4; i++) {
						devExt->limites[i].c=bufCal[i].c;
						devExt->limites[i].i=bufCal[i].i;
						devExt->limites[i].d=bufCal[i].d;
						devExt->limites[i].n=bufCal[i].n;
						devExt->jitter[i].Margen=bufCal[i].Margen;
						devExt->jitter[i].Resistencia=bufCal[i].Resistencia;
						devExt->limites[i].cal=bufCal[i].cal;
						devExt->jitter[i].antiv=bufCal[i].antiv;
					}
				}
			}		
			if((buffer&4)==4) {
				devExt->descalibrar = TRUE;
			} else {
				devExt->descalibrar = FALSE;
			}
		}
	}
}