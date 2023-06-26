/*++

Copyright (c) 2005-2007 Alfredo Costalago

Module Name:

    hidgame.h

Abstract:  Contains definitions of all constants and
           data types for the joystick driver.


Environment:

    Kernel mode


--*/

#include "gameport.h"

/*
 *  Typedef the structs we need
 */

/*****************************************************************************
 *
 *  @struct HIDGAME_GLOBAL |
 *
 *          Global struct to store driver wide data.
 *          Stuff we need to share across multiple instances of this driver.
 *
 *  @field  FAST_MUTEX | Mutex |
 *
 *          Mutex to synchronize access to the following list entry
 *
 *  @field  LIST_ENTRY | DeviceListHead |
 *
 *          Keeps a list of all devices.
 *
 *  @field  KSPIN_LOCK | SpinLock | 
 *
 *          Spinlock used to stop multiple processors polling gameports at 
 *          once.  It would be better to keep a list of spinlocks, one for 
 *          each gameport but then processors could contend for IO access 
 *          and we'd have to maintain another list.
 *
 *  @field  COUNTER_FUNCTION | ReadCounter | 
 *
 *          Function to retrieve clock time
 *
 *  @field  ULONG | CounterScale | 
 *
 *          The scale to be used.
 *
 *****************************************************************************/
/*
 *  Function type used for timing.  
 *  MUST be compatible with KeQueryPerformanceCounter 
 */
typedef
LARGE_INTEGER
(*COUNTER_FUNCTION) (
    PLARGE_INTEGER  pDummy
    );

typedef struct _HIDGAME_GLOBAL
{
    FAST_MUTEX          Mutex;          /* A syncronization for access to list */
    LIST_ENTRY          DeviceListHead; /* Keeps list of all the devices */
    KSPIN_LOCK          SpinLock;       /* Lock so that only one port is accessed */
    COUNTER_FUNCTION    ReadCounter;    /* Function to retrieve clock time */
    ULONG               CounterScale;   /* Clock scale factor */
} HIDGAME_GLOBAL;

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @struct OEMDATA |
 *
 *          OEMData is send to gameEnum as a parameter to IOCTL_EXPOSE_HARDWARE.
 *          Defined as 8 DWORDS. We interpert them here
 *
 *  @field  USHORT | VID |
 *
 *          Vendor ID
 *
 *  @field  USHORT | PID |
 *
 *          Product ID
 *
 *  @field  ULONG | joy_hws_dwFlags |
 *
 *          The dwFlags fields for the device ( Usually read from the registry )
 *
 *  @field  ULONG   | Timeout |
 *
 *          Global timeout for device polling in micro seconds
 *
 *  @field  ULONG   | Reserved |
 *
 *          Reserved for future use.
 *
 *
 *****************************************************************************/
typedef struct _OEMDATA
{
    USHORT  VID;
    USHORT  PID;
    ULONG   joy_hws_dwFlags;
    ULONG   Timeout;
    ULONG   Reserved;

} OEMDATA, *POEMDATA;


typedef struct _HIDGAME_OEM_DATA
{
    union
    {
        OEMDATA OemData[2];
        GAMEENUM_OEM_DATA   Game_Oem_Data;
    };
} HIDGAME_OEM_DATA, *PHIDGAME_OEM_DATA;

#include <pshpack1.h>
typedef struct _HIDGAME_INPUT_DATA
{
    UCHAR   EjesXYZR[16];
	UCHAR	Ejes[6];
	UCHAR	Setas[4];
	UCHAR	Botones[7];
    UCHAR   MiniStick;
} HIDGAME_INPUT_DATA, *PHIDGAME_INPUT_DATA;
typedef struct _HIDGAME_INPUT_DATA UNALIGNED *PUHIDGAME_INPUT_DATA;
#include <poppack.h>


/*****************************************************************************
 *
 *  @struct DEVICE_EXTENSION |
 *
 *          Device specific data.
 *
 *  @field  PGAMEENUM_READPORT | ReadAccessor |
 *
 *          Read Accessor function for the gameport. Obtained in the return from
 *          IOCTL to the gameport.
 *
 *  @field  PGAMEENUM_WRITEPORT | WriteAccessor |
 *
 *          Write Accessor function for the gameport. Obtained in the return from
 *          IOCTL to the gameport.
 *
 *  @field  PGAMEENUM_READPORT_DIGITAL | ReadAccessorDigital |
 *
 *          Digital read accessor for the gameport. Obtained as part of return from
 *          IOCTL to the gameport
 *
 *  @field  PGAMEENUM_ACQUIRE_PORT | AcquirePort |
 *
 *          Function to call before reading/writing to the port. Obtained as 
 *          part of return from IOCTL to the gameport
 *
 *  @field  PGAMEENUM_RELEASE_PORT | ReleasePort |
 *
 *          Function to call when done reading/writing to the port. Obtained as 
 *          part of return from IOCTL to the gameport
 *
 *  @field  PVOID    | GameContext |
 *
 *          Token to read this game port. Obtained as part of the return from
 *          IOCTL to the gameport.
 *
 *  @field  PVOID    | PortContext |
 *
 *          Context to pass to AcquirePort and ReleasePort. Obtained as part 
 *          of the return from IOCTL to the gameport.
 *
 *  @field  LIST_ENTRY  | Link |
 *
 *          Link to other hidgame devices on the system.
 *
 *  @field  KEVENT | RemoveEvent |
 *
 *          The remove plugplay request must use this event to make sure all 
 *          other requests have completed before it deletes the device object.
 *
 *  @field  LONG | RequestCount |
 *
 *          Number of IRPs underway.
 *
 *  @field  ANALOG_DEVICE | unnamed structure see ANALOG_DEVICE |
 *          
 *          Structure containing analog device specific information.
 *
 *          NOTE: this structure is placed after the DWORD aligned elements.
 *
 *  @xref   <t ANALOG_DEVICE>.
 *
 *  @field  BOOLEAN | fRemoved |
 *
 *          Set to true if the device has been removed => all requests should be failed
 *
 *  @field  BOOLEAN | fStarted |
 *
 *          Set to true is device has started.
 *
 *  @field  BOOLEAN | fSurpriseRemoved |
 *
 *          Set to true if the device has been surprise removed by PnPs device has started.
 *
 *****************************************************************************/
typedef struct _DEVICE_EXTENSION
{
#pragma region "Resources"
    /*
     *  read accessor for the game port
     */
    PGAMEENUM_READPORT          ReadAccessor;

    /*
     *  write the game port
     */
    PGAMEENUM_WRITEPORT         WriteAccessor;

    /*
     * Function to call before reading/writing to the port
     */
    PGAMEENUM_ACQUIRE_PORT      AcquirePort;

    /*
     * Function to call when done reading/writing to the port
     */
    PGAMEENUM_RELEASE_PORT      ReleasePort;
    
    /*
     *  token to read this game port
     */
    PVOID                       GameContext;

    /* 
     * Context to pass to AcquirePort and ReleasePort
     */
    PVOID                       PortContext;
#pragma endregion
#pragma region "PNP"
    /*
     *  List of other joystick devices
     */
    LIST_ENTRY                  Link;

    /*
     *  The remove plugplay request must use this event to make sure all 
     *  other requests have completed before it deletes the device object.
     */
    KEVENT                      RemoveEvent;

    /*
     *  Number of IRPs underway.
     */
    LONG                        RequestCount;

    BOOLEAN                     fRemoved;
    BOOLEAN                     fStarted;
    BOOLEAN                     fSurpriseRemoved;
#pragma endregion
#pragma region "Datos analogicos"
	/*
     *  Structure containing analog device specific information.
     */
    HIDGAME_OEM_DATA            HidGameOemData;
    BOOLEAN                     fSiblingFound;
    UCHAR                       resistiveInputMask;
    ULONG                       ScaledTimeout; //The value at which an axis in considered not present.
	USHORT						nAxes;
    /*
     *  The minimum resolution of a polling cycle.
     *  This is used to detect if we've been pre-empted or interrupted 
     *  during a polling loop.
     */
    ULONG                       ScaledThreshold;
	ULONG                       LastGoodAxis[4];
	ULONG						PrevGoodAxis[4];
	UCHAR						RepsAxis[4];

    int                         AxisMap[4]; //MAX_AXES
#pragma endregion

	// Calibrado y antivibraciones
	//ULONG minAxis[4];
	//ULONG maxAxis[4];
	//BOOLEAN nuevaCalibracion;
	HIDGAME_INPUT_DATA ultimosDatosOK;

	BOOLEAN descalibrar;
	struct {
		BOOLEAN cal;
		ULONG i;
		ULONG c;
		ULONG d;
		UCHAR n;
	} limites[4];
	struct {
		BOOLEAN antiv;
		ULONG PosElegida;
		UCHAR PosRepetida;
		UCHAR Margen;
		UCHAR Resistencia;
	} jitter[4];

	// Punteros del VHID wrapper Filter
	PDEVICE_OBJECT wdo;
    PFILE_OBJECT wfo;
	PDEVICE_OBJECT vdo;
    PFILE_OBJECT vfo;

}  DEVICE_EXTENSION, *PDEVICE_EXTENSION;

    #define GET_MINIDRIVER_DEVICE_EXTENSION(DO)  \
    ((PDEVICE_EXTENSION) (((PHID_DEVICE_EXTENSION)(DO)->DeviceExtension)->MiniDeviceExtension))

    #define GET_NEXT_DEVICE_OBJECT(DO) \
    (((PHID_DEVICE_EXTENSION)(DO)->DeviceExtension)->NextDeviceObject)



/*
 *  Globals
 */
extern HIDGAME_GLOBAL Global;

/*
 * Function prototypes
 */

NTSTATUS DriverEntry
    (
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING registryPath
    );

//DRIVER_DISPATCH HGM_CreateClose;
NTSTATUS HGM_CreateClose
    (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

//DRIVER_DISPATCH HGM_SystemControl;
NTSTATUS HGM_SystemControl
    (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

//DRIVER_DISPATCH HGM_Power;
NTSTATUS HGM_Power
    (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP pIrp
    );

//DRIVER_ADD_DEVICE HGM_AddDevice;
NTSTATUS HGM_AddDevice
    (
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT FunctionalDeviceObject
    );

//DRIVER_UNLOAD HGM_Unload;
VOID HGM_Unload
    (
    IN PDRIVER_OBJECT DriverObject
    );
