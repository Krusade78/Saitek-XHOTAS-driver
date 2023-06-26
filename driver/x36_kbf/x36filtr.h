/*++
Copyright (c) 2005-2007 Alfredo Costalago

Module Name:

    x36filter.h

Abstract:

    Este modulo contiene las declaraciones de funciones especificamente diseñadas
	para controlar el X36
--*/
#define IOCTL_X36_JOYINICIAR	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0100, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _HIDGAME_INPUT_DATA
{
    UCHAR   Ejes[2];
    UCHAR   Setas[4];
    UCHAR   Modos;
	UCHAR	Botones[2];
} HIDGAME_INPUT_DATA, *PHIDGAME_INPUT_DATA;

typedef struct _X36DEVICE_EXTENSION
{
	PDEVICE_EXTENSION kDevExt;

	// Variables de estado
	BOOLEAN faseIni;
	UCHAR bytesIni;
	UCHAR fase;
	//UCHAR acelPed;

	//UCHAR stSeta1;
	//UCHAR stSeta2;
	USHORT gPosicion;
	UCHAR gEje;

	// Datos HID
	HIDGAME_INPUT_DATA hidData;

}X36DEVICE_EXTENSION,*PX36DEVICE_EXTENSION;


VOID
X36Filter_Iniciar(
    IN PX36DEVICE_EXTENSION x36DevExt
	);

VOID X36Filter_IniciarJoy(
    IN PX36DEVICE_EXTENSION x36DevExt//,
	//UCHAR ace_ped
	);

NTSTATUS
X36Filter_InterfaceDispatch (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
X36Filter_ServiceCallback(
    IN PDEVICE_OBJECT DeviceObject,
    IN PKEYBOARD_INPUT_DATA InputDataStart,
    IN PKEYBOARD_INPUT_DATA InputDataEnd,
    IN OUT PULONG InputDataConsumed
    );
