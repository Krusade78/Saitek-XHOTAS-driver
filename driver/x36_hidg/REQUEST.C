#include <wdm.h>
#include "hidgame.h"
#include "request.h"

/*****************************************************************************
 *
 *  @func   NTSTATUS  | HGM_IncRequestCount |
 *
 *          Try to increment the request count but fail if the device is 
 *          being removed.
 *
 *  @parm   IN PDEVICE_EXTENSION | DeviceExtension |
 *
 *          Pointer to the device extension.
 *
 *  @rvalue STATUS_SUCCESS | success
 *  @rvalue STATUS_DELETE_PENDING | PnP IRP received after device was removed
 *
 *****************************************************************************/
NTSTATUS HGM_IncRequestCount
    (
    IN PDEVICE_EXTENSION DeviceExtension
    )
{
    NTSTATUS    ntStatus;

    InterlockedIncrement( &DeviceExtension->RequestCount );
    ASSERT( DeviceExtension->RequestCount > 0 );
    
    if( DeviceExtension->fRemoved )
    {
        /*
         *  PnP has already told us to remove the device so fail and make 
         *  sure that the event has been set.
         */
        if( 0 == InterlockedDecrement( &DeviceExtension->RequestCount ) ) 
        {
            KeSetEvent( &DeviceExtension->RemoveEvent, IO_NO_INCREMENT, FALSE );
        }
        ntStatus = STATUS_DELETE_PENDING;
    }
    else
    {
        ntStatus = STATUS_SUCCESS;
    }

    return ntStatus;
}
/*****************************************************************************
 *
 *  @func   VOID  | HGM_DecRequestCount |
 *
 *          Decrement the request count and set event if this is the last.
 *
 *  @parm   IN PDEVICE_EXTENSION | DeviceExtension |
 *
 *          Pointer to the device extension.
 *
 *****************************************************************************/
VOID HGM_DecRequestCount
    (
    IN PDEVICE_EXTENSION DeviceExtension
    )
{
    LONG        LocalCount;

    LocalCount = InterlockedDecrement( &DeviceExtension->RequestCount );

    ASSERT( DeviceExtension->RequestCount >= 0 );
    
    if( LocalCount == 0 )
    {
        /*
         *  PnP has already told us to remove the device so the PnP remove 
         *  code should have set device as removed and should be waiting on
         *  the event.
         */
        ASSERT( DeviceExtension->fRemoved );
        KeSetEvent( &DeviceExtension->RemoveEvent, IO_NO_INCREMENT, FALSE );
    }

    return;
}