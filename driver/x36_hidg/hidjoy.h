/*
 *  hidjoy.c
 */

// Funciones públicas

#define MAXBYTES_GAME_REPORT    ( 256 )
//#define REPORT_DESCRIPTOR_SIZE  134

NTSTATUS HGM_GenerateReport
    (
    //IN PDEVICE_OBJECT   DeviceObject,
    //OUT UCHAR           rgGameReport[MAXBYTES_GAME_REPORT]
	IN PDEVICE_OBJECT       DeviceObject, 
    OUT UCHAR               rgGameReport[MAXBYTES_GAME_REPORT],
    OUT PUSHORT             pCbReport
    );

VOID HGM_Game2HID
    (
    IN PDEVICE_OBJECT			DeviceObject,
	IN OUT PUHIDGAME_INPUT_DATA	pHIDData
    );

NTSTATUS HGM_MapAxesFromDevExt
    (
    IN OUT PDEVICE_EXTENSION DeviceExtension 
    );

// Funciones privadas

#ifdef _HIDJOYC_

VOID HGM_CogerDatosX36
    (
    IN      PDEVICE_OBJECT       DeviceObject,
    IN  OUT PHIDGAME_INPUT_DATA    pHIDData
    );

VOID HGM_ComprobarEstado
	(
    IN      PDEVICE_OBJECT       DeviceObject
	);

VOID HGM_Coger_FiltrarEjes
    (
    IN      PDEVICE_OBJECT       DeviceObject,
    IN  OUT PHIDGAME_INPUT_DATA    pHIDData
    );

#endif