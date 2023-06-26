VOID ProcesarAccionRaton(PDEVICE_EXTENSION devExt);
VOID ProcesarAccionTeclado(PDEVICE_EXTENSION devExt);

VOID Accionar
(
	PDEVICE_EXTENSION devExt,
	UINT16 accionId,
	UCHAR boton
	);

#ifdef _ACCIONES_

//#include "cola.h"

typedef struct _DELAY_CONTEXT{
    
    KDPC			DelayTimerDpc;
    KTIMER			DelayTimer;
    PNODO			NodoIni;
    PNODO			NodoFin;
	PDEVICE_EXTENSION DeviceExtension;
  
} DELAY_CONTEXT, *PDELAY_CONTEXT;

VOID 
DpcRoutineDelay(   
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

typedef struct _ORDEN_WORKITEM
{
    PIO_WORKITEM      WorkItem;
    PDEVICE_EXTENSION DeviceExtension;
    PVOID             Params;
	UCHAR			  Tam;
	UCHAR			  Tipo;
} ORDEN_WORKITEM, *PORDEN_WORKITEM;
typedef struct _ESTADO_WORKITEM
{
	PIO_WORKITEM			WorkItem;
	PDEVICE_EXTENSION	DeviceExtension;
	HID_INPUT_DATA		Estado;
} ESTADO_WORKITEM, *PESTADO_WORKITEM;

typedef struct _ORDENX52_CONTEXT
{
    PDEVICE_EXTENSION	DeviceExtension;
	PETHREAD			Hilo;
	KEVENT				Ev;
    PVOID				Params;
	UCHAR				Tam;
	UCHAR				Tipo;
} ORDENX52_CONTEXT, *PORDENX52_CONTEXT;
typedef struct _ESTADOX52_CONTEXT
{
	PDEVICE_EXTENSION	DeviceExtension;
	PETHREAD					Hilo;
	KEVENT						Ev;
	HID_INPUT_DATA		Estado;

} ESTADOX52_CONTEXT, *PESTADOX52_CONTEXT;

VOID EnviarEstadoX52
(
	PDEVICE_EXTENSION	devExt,
	PHID_INPUT_DATA		sthid
	);
VOID EnviarOrdenX52
(
	PDEVICE_EXTENSION devExt,
	UCHAR tipo,
	PVOID params,
	UCHAR tam
	);
VOID PreEstadoX52
(
	IN PDEVICE_OBJECT DevObj,
	IN PVOID Context
	);
VOID PreEnviarX52
(
	IN PDEVICE_OBJECT DevObj,
	IN PVOID Context
	);
VOID EstadoX52
(
   IN PVOID Context
   );
VOID EnviarX52
(
   IN PVOID Context
   );


#endif