NTSTATUS
HF_IoLeer(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

HF_IoWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );


#ifdef _IO_

typedef struct _X36KB_INPUT_DATA
{
    UCHAR   Ejes[2];
    UCHAR   Setas[4];
    UCHAR   Modos;
	UCHAR	Botones[2];
} X36KB_INPUT_DATA, *PX36KB_INPUT_DATA;

VOID 
ProcesarWrite(   
    PDEVICE_EXTENSION devExt,
	PX36KB_INPUT_DATA hidGameData
    );

UCHAR TraducirGiratorio(
	PITFDEVICE_EXTENSION idevExt,
	UCHAR eje,
	UCHAR nueva
	);

#endif