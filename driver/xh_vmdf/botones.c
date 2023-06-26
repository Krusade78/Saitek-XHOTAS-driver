#include <wdm.h>
#include "hidfilter.h"
#include "acciones.h"
#include "botones.h"


VOID AccionPulsarBoton
(	PDEVICE_EXTENSION devExt,
	UCHAR idx)
{
	PITFDEVICE_EXTENSION idevExt = (PITFDEVICE_EXTENSION)devExt->itfObj->DeviceExtension;
	UINT16 accionId;
	KIRQL irql;

	KeAcquireSpinLock(&idevExt->slMapas,&irql);

	if((idevExt->MapaBotones[idevExt->stPinkie][idevExt->stModo][idevExt->stAux][idx].Estado&0xf)>0) {
#ifdef PRO
		accionId=idevExt->MapaBotones[idevExt->stPinkie][idevExt->stModo][idevExt->stAux][idx].Indices[idevExt->MapaBotones[idevExt->stPinkie][idevExt->stModo][idevExt->stAux][idx].Estado>>4];
		idevExt->MapaBotones[idevExt->stPinkie][idevExt->stModo][idevExt->stAux][idx].Estado+=16;
		if((idevExt->MapaBotones[idevExt->stPinkie][idevExt->stModo][idevExt->stAux][idx].Estado>>4)==(idevExt->MapaBotones[idevExt->stPinkie][idevExt->stModo][idevExt->stAux][idx].Estado&0xf)) idevExt->MapaBotones[idevExt->stPinkie][idevExt->stModo][idevExt->stAux][idx].Estado&=0xf;
#endif
	} else {
		accionId=idevExt->MapaBotones[idevExt->stPinkie][idevExt->stModo][idevExt->stAux][idx].Indices[0];
	}

	KeReleaseSpinLock(&idevExt->slMapas,irql);

	Accionar(devExt,accionId,idx);
}

VOID AccionSoltarBoton
(	PDEVICE_EXTENSION devExt,
	UCHAR idx)
{
#ifdef PRO
	PITFDEVICE_EXTENSION idevExt = (PITFDEVICE_EXTENSION)devExt->itfObj->DeviceExtension;
	UINT16 accionId;
	KIRQL irql;

	KeAcquireSpinLock(&idevExt->slMapas,&irql);

	if((idevExt->MapaBotones[idevExt->stPinkie][idevExt->stModo][idevExt->stAux][idx].Estado&0xf)==0) {
	//	accionId=idevExt->MapaBotones[(UCHAR)idevExt->stPinkie][idevExt->stModo][idevExt->stAux][idx].Indices[idevExt->MapaBotones[(UCHAR)idevExt->stPinkie][idevExt->stModo][idevExt->stAux][0][idx].Estado>>4];
	//	idevExt->MapaBotones[(UCHAR)idevExt->stPinkie][idevExt->stModo][idevExt->stAux][idx].Estado+=16;
	//	if((idevExt->MapaBotones[(UCHAR)idevExt->stPinkie][idevExt->stModo][idevExt->stAux][idx].Estado>>4)>(idevExt->MapaBotones[(UCHAR)idevExt->stPinkie][idevExt->stModo][idevExt->stAux][0][idx].Estado&0xf)) idevExt->MapaBotones[(UCHAR)idevExt->stPinkie][idevExt->stModos][idevExt->stAux][0][idx].Estado&=0xf;
	//} else {
		accionId=idevExt->MapaBotones[idevExt->stPinkie][idevExt->stModo][idevExt->stAux][idx].Indices[1];
		KeReleaseSpinLock(&idevExt->slMapas,irql);

		Accionar(devExt,accionId,idx);
	} else {
		KeReleaseSpinLock(&idevExt->slMapas,irql);
	}
#endif
}

VOID AccionPulsarSeta
(	PDEVICE_EXTENSION devExt,
	UCHAR idx)
{
	PITFDEVICE_EXTENSION idevExt = (PITFDEVICE_EXTENSION)devExt->itfObj->DeviceExtension;
	UINT16 accionId;
	KIRQL irql;

	KeAcquireSpinLock(&idevExt->slMapas,&irql);

	if((idevExt->MapaSetas[idevExt->stPinkie][idevExt->stModo][idevExt->stAux][idx].Estado&0xf)>0) {
#ifdef PRO
		accionId=idevExt->MapaSetas[idevExt->stPinkie][idevExt->stModo][idevExt->stAux][idx].Indices[idevExt->MapaSetas[idevExt->stPinkie][idevExt->stModo][idevExt->stAux][idx].Estado>>4];
		idevExt->MapaSetas[idevExt->stPinkie][idevExt->stModo][idevExt->stAux][idx].Estado+=16;
		if((idevExt->MapaSetas[idevExt->stPinkie][idevExt->stModo][idevExt->stAux][idx].Estado>>4)==(idevExt->MapaSetas[idevExt->stPinkie][idevExt->stModo][idevExt->stAux][idx].Estado&0xf)) idevExt->MapaSetas[idevExt->stPinkie][idevExt->stModo][idevExt->stAux][idx].Estado&=0xf;
#endif
	} else {
		accionId=idevExt->MapaSetas[idevExt->stPinkie][idevExt->stModo][idevExt->stAux][idx].Indices[0];
	}

	KeReleaseSpinLock(&idevExt->slMapas,irql);

	Accionar(devExt,accionId,idx+101);
}

VOID AccionSoltarSeta
(	PDEVICE_EXTENSION devExt,
	UCHAR idx)
{
#ifdef PRO
	PITFDEVICE_EXTENSION idevExt = (PITFDEVICE_EXTENSION)devExt->itfObj->DeviceExtension;
	UINT16 accionId;
	KIRQL irql;

	KeAcquireSpinLock(&idevExt->slMapas,&irql);

	if((idevExt->MapaSetas[idevExt->stPinkie][idevExt->stModo][idevExt->stAux][idx].Estado&0xf)==0) {
	//	accionId=idevExt->MapaSetas[(UCHAR)idevExt->stPinkie][idevExt->stModo][idevExt->stAux][idx].Indices[idevExt->MapaSetas[(UCHAR)idevExt->stPinkie][idevExt->stModo][idevExt->stAux][idx].Estado>>4];
	//	idevExt->MapaSetas[(UCHAR)idevExt->stPinkie][idevExt->stModo][idevExt->stAux][idx].Estado+=16;
	//	if((idevExt->MapaSetas[(UCHAR)idevExt->stPinkie][idevExt->stModo][idevExt->stAux][idx].Estado>>4)>(idevExt->MapaSetas[(UCHAR)idevExt->stPinkie][idevExt->stModo][idevExt->stAux][idx].Estado&0xf)) idevExt->MapaSetas[(UCHAR)idevExt->stPinkie][idevExt->stModos][idevExt->stAux][idx].Estado&=0xf;
	//} else {
		accionId=idevExt->MapaSetas[idevExt->stPinkie][idevExt->stModo][idevExt->stAux][idx].Indices[1];
		KeReleaseSpinLock(&idevExt->slMapas,irql);

		Accionar(devExt,accionId,idx+101);
	} else {
		KeReleaseSpinLock(&idevExt->slMapas,irql);
	}
#endif
}