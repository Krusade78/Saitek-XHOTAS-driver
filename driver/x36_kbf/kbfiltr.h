/*++
Copyright (c) 2003-2007 Alfredo Costalago

Module Name:

    kbfilter.h

Abstract:

    This module contains the common private declarations for the keyboard
    packet filter

Notes:

	Original file by Microsoft Corporation in Windows XP DDK 2.6
	Copyright (c) 1998. 1999  Microsoft Corporation

License:

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.


--*/

#include "ntddk.h"
#include "kbdmou.h"
#include <ntddkbd.h>
#include <ntdd8042.h>

typedef struct _DEVICE_EXTENSION
{
    //
    // A backpointer to the device object for which this is the extension
    //
    PDEVICE_OBJECT  Self;

    //
    // "THE PDO"  (ejected by the root bus or ACPI)
    //
    PDEVICE_OBJECT  PDO;

    //
    // The top of the stack before this filter was added.  AKA the location
    // to which all IRPS should be directed.
    //
    PDEVICE_OBJECT  TopOfStack;

    //
    // The real connect data that this driver reports to
    //
    CONNECT_DATA UpperConnectData;

    //
    // Write function from within KbFilter_IsrHook
    //
    IN PI8042_ISR_WRITE_PORT IsrWritePort;

	// SpinLock para el callback
    KSPIN_LOCK SpinLock;

    //
    // Context for IsrWritePort, QueueKeyboardPacket
    //
    IN PVOID CallContext;

	// X36 Device
	PDEVICE_OBJECT x36Obj;

	// Punteros del VHID Filter
	PDEVICE_OBJECT vdo;
    PFILE_OBJECT vfo;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;


//
// Prototypes
//

//DRIVER_ADD_DEVICE KbFilter_AddDevice;
NTSTATUS
KbFilter_AddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT BusDeviceObject
    );

//DRIVER_UNLOAD KbFilter_Unload;
VOID
KbFilter_Unload (
    IN PDRIVER_OBJECT DriverObject
    );

//DRIVER_DISPATCH KbFilter_DispatchPassThrough;
NTSTATUS
KbFilter_DispatchPassThrough(
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
        );

NTSTATUS
KbFilter_InternIoCtl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
KbFilter_PnP (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

//IO_COMPLETION_ROUTINE KbFilter_Complete;
NTSTATUS
KbFilter_Complete(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    );

NTSTATUS
KbFilter_Power (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );
