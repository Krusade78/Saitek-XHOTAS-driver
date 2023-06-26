typedef	struct _CALIBRADO {
	ULONG i;
	ULONG c;
	ULONG d;
	UCHAR n;
	UCHAR Margen;
	UCHAR Resistencia;
	BOOLEAN cal;
	BOOLEAN antiv;
} CALIBRADO, *PCALIBRADO;

typedef struct _DEVICE_EXTENSION
{
	ULONG id;
    //
    // The top of the stack before this filter was added.  AKA the location
    // to which all IRPS should be directed.
    //
    PDEVICE_OBJECT  TopOfStack;
	IO_REMOVE_LOCK removeLock;

	PDEVICE_OBJECT itfObj;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

typedef struct _HIDGAME_INPUT_DATA{
			UCHAR   Ejes[2];
			UCHAR	Botones[2]; // El ultimo byte se comparte con setas (2 bits)
			UCHAR   Setas[2];
} HIDGAME_INPUT_DATA, *PHIDGAME_INPUT_DATA;

typedef struct _ITFDEVICE_EXTENSION
{
	ULONG id;
	PDEVICE_EXTENSION vDevExt;

	CALIBRADO calibrado[4];
	KSPIN_LOCK slCalibrado;
	KSPIN_LOCK slEstado;
	UCHAR Estado;

	KSPIN_LOCK slGameData;
	HIDGAME_INPUT_DATA stX36;
	HIDGAME_INPUT_DATA stAux;
	BOOLEAN reportLeido;
	BOOLEAN auxLibre;

}ITFDEVICE_EXTENSION,*PITFDEVICE_EXTENSION;

#ifdef _HIDFILTER_

NTSTATUS DriverEntry
    (
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

//DRIVER_ADD_DEVICE HF_AddDevice;
NTSTATUS
HF_AddDevice(
    IN PDRIVER_OBJECT   Driver,
    IN PDEVICE_OBJECT   PDO
    );

VOID
IniciarExtensiones(
	PDEVICE_EXTENSION devExt
	);

//DRIVER_UNLOAD HF_Unload;
VOID
HF_Unload(
   IN PDRIVER_OBJECT Driver
   );

//DRIVER_DISPATCH HF_DispatchPassThrough;
NTSTATUS
HF_DispatchPassThrough(
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
        );

NTSTATUS
HF_InternIoCtl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
HF_PnP(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
HF_Power(
    IN PDEVICE_OBJECT    DeviceObject,
    IN PIRP              Irp
    );

#endif