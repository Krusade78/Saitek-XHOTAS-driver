/*--
Copyright (c) 2005-2007  Alfredo Costalago

Module Name:

    x36.c

--*/
//IO_COMPLETION_ROUTINE LowerCompletionRoutineOut;
NTSTATUS LowerCompletionRoutineOut(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp,
    IN PVOID  Context
    )
{
	IoFreeIrp(Irp);
	ExFreePoolWithTag(Context,(ULONG)'fbKX');
	return STATUS_MORE_PROCESSING_REQUIRED;
}

VOID LLamarDispositivoVirtual
    (
    PX36DEVICE_EXTENSION x36devExt
    )
{
	PIRP pIrpOut;
	PVOID bufOut;
	LARGE_INTEGER	offset;
	IO_STATUS_BLOCK IoStatus;
	PDEVICE_EXTENSION devExt=x36devExt->kDevExt;

	if(devExt->vfo!=NULL)
	{
		bufOut = ExAllocatePoolWithTag(NonPagedPool, 
                                       sizeof(HIDGAME_INPUT_DATA), 
                                       (ULONG)'fbKX'
                                       );
		if(bufOut!=NULL) {
			RtlCopyMemory(bufOut,&x36devExt->hidData,sizeof(HIDGAME_INPUT_DATA));
			offset.HighPart=0;
			offset.LowPart=0;
			pIrpOut =   IoBuildAsynchronousFsdRequest(IRP_MJ_WRITE,
													devExt->vdo,
													bufOut,
													sizeof(HIDGAME_INPUT_DATA),
													&offset,
													&IoStatus);
			if(pIrpOut!=NULL) {
				IoSetCompletionRoutine(pIrpOut,
									LowerCompletionRoutineOut,
									bufOut,
									TRUE,
									TRUE,
									TRUE);
				IoCallDriver(devExt->vdo,pIrpOut);
			}
		}
	}
}

void CambiarEstado(PX36DEVICE_EXTENSION devExt,UCHAR ID,BOOLEAN dato,UCHAR aux)
{
	if(aux>0) {
		devExt->hidData.Botones[ID/8]&=255-(1<<(ID%8)); //Boton ID a 0
		devExt->hidData.Modos=devExt->hidData.Botones[ID/8];
		devExt->hidData.Botones[ID/8]|=1<<(aux%8); // Boton aux a 1
	} else {
		devExt->hidData.Modos=0;
		if(dato) {
			devExt->hidData.Botones[ID/8]|=1<<(ID%8);
		} else {
			devExt->hidData.Botones[ID/8]&=255-(1<<(ID%8));
		}
	}
	LLamarDispositivoVirtual(devExt);
}

void TraducirBoton(PX36DEVICE_EXTENSION devExt,USHORT MakeCode)
{
	switch (MakeCode)
	{
//****************************** X36 F
//------------ Gatillo
		case 13:
            CambiarEstado(devExt,0,TRUE,0);
			break;
		case 77:
            CambiarEstado(devExt,0,FALSE,0);
			break;
//------------ Lanzar
		case 4:
            CambiarEstado(devExt,3,TRUE,0);
			break;
		case 68:
            CambiarEstado(devExt,3,FALSE,0);
			break;

//------------ Boton a
		case 2:
            CambiarEstado(devExt,1,TRUE,0);
			break;
		case 66:
            CambiarEstado(devExt,1,FALSE,0);
			break;
//----------- Boton b
		case 3:
            CambiarEstado(devExt,2,TRUE,0);
			break;
		case 67:
            CambiarEstado(devExt,2,FALSE,0);
			break;
//------------ Boton c
		case 1:
            CambiarEstado(devExt,7,TRUE,0);
			break;
		case 65:
            CambiarEstado(devExt,7,FALSE,0);
			break;

// ------------ Pinkie
		case 14:
            CambiarEstado(devExt,6,TRUE,0);
			break;
		case 78:
            CambiarEstado(devExt,6,FALSE,0);
			break;
//************ Seta 1
		case 5:
			devExt->hidData.Setas[3]|=1;
			LLamarDispositivoVirtual(devExt);
			break;
		case 69:
			devExt->hidData.Setas[3]&=254;
			LLamarDispositivoVirtual(devExt);
			break;
		case 7:
			devExt->hidData.Setas[3]|=4;
			LLamarDispositivoVirtual(devExt);
			break;
		case 71:
			devExt->hidData.Setas[3]&=251;
			LLamarDispositivoVirtual(devExt);
			break;
		case 8:
			devExt->hidData.Setas[3]|=8;
			LLamarDispositivoVirtual(devExt);
			break;
		case 72:
			devExt->hidData.Setas[3]&=247;
			LLamarDispositivoVirtual(devExt);
			break;
		case 6:
			devExt->hidData.Setas[3]|=2;
			LLamarDispositivoVirtual(devExt);
			break;
		case 70:
			devExt->hidData.Setas[3]&=253;
			LLamarDispositivoVirtual(devExt);
			break;
//----------------
//********** Seta 2
		case 9:
			devExt->hidData.Setas[0]|=1;
			LLamarDispositivoVirtual(devExt);
			break;
		case 73:
			devExt->hidData.Setas[0]&=254;
			LLamarDispositivoVirtual(devExt);
			break;
		case 11:
			devExt->hidData.Setas[0]|=4;
			LLamarDispositivoVirtual(devExt);
			break;
		case 75:
			devExt->hidData.Setas[0]&=251;
			LLamarDispositivoVirtual(devExt);
			break;
		case 12:
			devExt->hidData.Setas[0]|=8;
			LLamarDispositivoVirtual(devExt);
			break;
		case 76:
			devExt->hidData.Setas[0]&=247;
			LLamarDispositivoVirtual(devExt);
			break;
		case 10:
			devExt->hidData.Setas[0]|=2;
			LLamarDispositivoVirtual(devExt);
			break;
		case 74:
			devExt->hidData.Setas[0]&=253;
			LLamarDispositivoVirtual(devExt);
			break;
//----------------
//******************** X35 ********************
//------------ Boton d
		case 15:
            CambiarEstado(devExt,4,TRUE,0);
			break;
		case 79:
            CambiarEstado(devExt,4,FALSE,0);
			break;
//----------- Boton Raton
		case 16:
            CambiarEstado(devExt,5,TRUE,0);
			break;
		case 80:
            CambiarEstado(devExt,5,FALSE,0);
			break;
//----------- Modo Izquierda
		case 32:
            CambiarEstado(devExt,9,0,8);
			break;
		case 96:
            CambiarEstado(devExt,8,0,9);
			break;
//----------- Modo Derecha
		case 31:
            CambiarEstado(devExt,9,0,10);
			break;
		case 95:
            CambiarEstado(devExt,10,0,9);
			break;
//----------- Aux Izquierda
		case 30:
            CambiarEstado(devExt,12,0,11);
			break;
		case 94:
            CambiarEstado(devExt,11,0,12);
			break;
//----------- Aux Derecha
		case 29:
            CambiarEstado(devExt,12,0,13);
			break;
		case 93:
            CambiarEstado(devExt,13,0,12);
			break;
//*********** Seta Raton
		case 21:
			devExt->hidData.Setas[2]|=1;
			LLamarDispositivoVirtual(devExt);
			break;
		case 85:
			devExt->hidData.Setas[2]&=254;
			LLamarDispositivoVirtual(devExt);
			break;
		case 23:
			devExt->hidData.Setas[2]|=4;
			LLamarDispositivoVirtual(devExt);
			break;
		case 87:
			devExt->hidData.Setas[2]&=251;
			LLamarDispositivoVirtual(devExt);
			break;
		case 24:
			devExt->hidData.Setas[2]|=8;
			LLamarDispositivoVirtual(devExt);
			break;
		case 88:
			devExt->hidData.Setas[2]&=247;
			LLamarDispositivoVirtual(devExt);
			break;
		case 22:
			devExt->hidData.Setas[2]|=2;
			LLamarDispositivoVirtual(devExt);
			break;
		case 86:
			devExt->hidData.Setas[2]&=253;
			LLamarDispositivoVirtual(devExt);
			break;
//-------------
//********** Seta 3
		case 17:
			devExt->hidData.Setas[1]|=1;
			LLamarDispositivoVirtual(devExt);
			break;
		case 81:
			devExt->hidData.Setas[1]&=254;
			LLamarDispositivoVirtual(devExt);
			break;
		case 19:
			devExt->hidData.Setas[1]|=4;
			LLamarDispositivoVirtual(devExt);
			break;
		case 83:
			devExt->hidData.Setas[1]&=251;
			LLamarDispositivoVirtual(devExt);
			break;
		case 20:
			devExt->hidData.Setas[1]|=8;
			LLamarDispositivoVirtual(devExt);
			break;
		case 84:
			devExt->hidData.Setas[1]&=247;
			LLamarDispositivoVirtual(devExt);
			break;
		case 18:
			devExt->hidData.Setas[1]|=2;
			LLamarDispositivoVirtual(devExt);
			break;
		case 82:
			devExt->hidData.Setas[1]&=253;
			LLamarDispositivoVirtual(devExt);
//------------
	}
}


void TraducirGiratorio(PX36DEVICE_EXTENSION devExt,USHORT MakeCode)
{
	UCHAR girNuevo=((UCHAR)devExt->gPosicion-48)+(((UCHAR)MakeCode-48)*16);

	//if(devExt->gEje<2) {
		//devExt->hidData.Ejes[devExt->gEje]=girNuevo;
	//} else {
	if(devExt->gEje>1) {
		devExt->hidData.Ejes[devExt->gEje-2]=(devExt->gEje==2)?girNuevo:255-girNuevo;
		//devExt->hidData.Rotatorios[devExt->gEje-2]=girNuevo;
		//if(girNuevo==127 || girNuevo==128) {
		//	devExt->hidData.Rotatorios[devExt->gEje-2]=0;
		//} else {
			//devExt->hidData.Rotatorios[devExt->gEje-2]=girNuevo-((girNuevo>128)?128:127);
		//}
	}

	LLamarDispositivoVirtual(devExt);
}
