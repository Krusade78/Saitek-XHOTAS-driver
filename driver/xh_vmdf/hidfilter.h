//#include <gameport.h>
#include "cola.h"

typedef struct _HID_INPUT_DATA
{
    UCHAR   Ejes[14];
	UCHAR	Setas[4];
	UCHAR	Botones[7];
    UCHAR   MiniStick;
} HID_INPUT_DATA, *PHID_INPUT_DATA;

typedef struct _DEVICE_EXTENSION
{
	ULONG id;
    //
    // The top of the stack before this filter was added.  AKA the location
    // to which all IRPS should be directed.
    //
    PDEVICE_OBJECT  TopOfStack;
	PDEVICE_OBJECT  self;
	IO_REMOVE_LOCK removeLock;

	PDEVICE_OBJECT itfObj;

	UCHAR	stRaton[4];
	UCHAR	stTeclado[29];
	HID_INPUT_DATA stHidData;
	KEVENT  evAccion;

	KSPIN_LOCK slGameData;
	KSPIN_LOCK slRaton;

	KSPIN_LOCK	slListaAcciones;
	COLA		ListaAcciones;

	KSPIN_LOCK	slIrpLectura;
	PIRP		IrpLectura;
	UCHAR TickRaton;
	BOOLEAN TickRatonTimer;

	// llamadas al usb
	KEVENT mutexUSB;
	//PDEVICE_OBJECT udo;
 //   PFILE_OBJECT ufo;
	BOOLEAN finUSB;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

typedef struct _ST_COMANDO
{
	UCHAR tam;
	UINT16 *datos;
} COMANDO, *PCOMANDO;

typedef struct _ITFDEVICE_EXTENSION
{
	ULONG id;
	PDEVICE_EXTENSION vDevExt;

	KSPIN_LOCK slCalibrado;
	struct {
		BOOLEAN cal;
		UINT16 i;
		UINT16 c;
		UINT16 d;
		UCHAR n;
	} limites[4];
	struct {
		BOOLEAN antiv;
		UINT16 PosElegida;
		UCHAR PosRepetida;
		UCHAR Margen;
		UCHAR Resistencia;
	} jitter[4];
	BOOLEAN descalibrar;

	struct {
		UCHAR Estado;	// 4 bit idc 4 bit total posiciones
		UINT16 Indices[15];
	}MapaBotones[2][3][3][26]; // el ultimo es la rueda
	struct {
		UCHAR Estado;
		UINT16 Indices[15];
	} MapaSetas[2][3][3][32];
	struct {
		UCHAR Mouse;
		UCHAR nEje;				//Mapeado 0:nada <20 normal >20 invertido
		UCHAR Sensibilidad[10];
		UCHAR Bandas[15];
		UINT16 Indices[16];
	}MapaEjes[2][3][3][4];
	struct {
		UCHAR Mouse;
		UCHAR nEje;
		UCHAR Bandas[16];
		UINT16 Indices[16];
	}MapaEjesPeque[2][3][3][3];
	struct {
		UCHAR Mouse;
		UCHAR nEje;
	}MapaEjesMini[2][3][3][2];

	UCHAR tipoJoy;
	UCHAR stPinkie;
	UCHAR stModo;
	UCHAR stAux;
	USHORT posVieja[2][3][3][7];

	KSPIN_LOCK slMapas;

	COMANDO *Comandos;
	UINT16 nComandos;
	KSPIN_LOCK slComandos;

	HID_INPUT_DATA DeltaHidData;

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