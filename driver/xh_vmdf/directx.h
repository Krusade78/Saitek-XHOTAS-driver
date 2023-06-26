HF_IoWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );


#ifdef _DIRECTX_

typedef struct _HIDX36_INPUT_DATA
{
    UCHAR   EjesXY[3];
	UCHAR	Ejes[4];
	UCHAR	Botones[2];
	UCHAR	Setas[2];
} HIDX36_INPUT_DATA, *PHIDX36_INPUT_DATA;
typedef struct _HIDX52_INPUT_DATA
{
    UCHAR   EjesXYR[4];
	UCHAR	Ejes[4];
	UCHAR	Botones[4];
	UCHAR	Seta; // 2bits wheel + 2 blanco + 4 bits seta
	UCHAR	Ministick;
} HIDX52_INPUT_DATA, *PHIDX52_INPUT_DATA;

VOID 
ProcesarWrite(   
    PDEVICE_EXTENSION devExt,
	PVOID inputData,
	PHID_INPUT_DATA outputData,
	BOOLEAN esX52
    );

#endif