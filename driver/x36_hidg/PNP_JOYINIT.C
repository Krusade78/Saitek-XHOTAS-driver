/*++

Copyright (c) 2007 Alfredo Costalago

Module Name:

    pnp.c

Abstract: Funciones inicio analogicas.

Environment:

    Kernel mode


--*/

#include <wdm.h>
#include <hidport.h>
#include "hidgame.h"
#include "hidjoy.h"
#include "i386\timing.h"
#include "pnp_joyinit.h"


#ifdef ALLOC_PRAGMA
    #pragma alloc_text (PAGE, HGM_JoystickConfig)
    #pragma alloc_text (PAGE, HGM_InitAnalog)
#endif

/*****************************************************************************
 *
 *  @func   NTSTATUS  | HGM_InitAnalog |
 *
 *          Check that the configuration is valid whilst there is still time 
 *          to refuse it.  
 *          <nl>Detect and validate sibling relationships and call 
 *          HGM_JoystickConfig for the rest of the work.
 *
 *  @parm   IN PDEVICE_OBJECT | DeviceObject |
 *
 *          Pointer to the device object
 *
 *  @rvalue   STATUS_SUCCESS  | success
 *  @rvalue   STATUS_DEVICE_CONFIGURATION_ERROR  | Invalid configuration specified
 *
 *****************************************************************************/
/*
 *  Disable warning for variable used before set as it is hard for a compiler 
 *  to see that the use of DeviceExtension_Sibling is gated by a flag which 
 *  can only be set after DeviceExtension_Sibling is initialized.
 */
#pragma warning( disable:4701 )
NTSTATUS HGM_InitAnalog
    (
    IN PDEVICE_OBJECT         DeviceObject
    )
{
    NTSTATUS            ntStatus;
    PDEVICE_EXTENSION   DeviceExtension;
    PDEVICE_EXTENSION   DeviceExtension_Sibling;
    PLIST_ENTRY         pEntry;

#define ARE_WE_RELATED(_x_, _y_)                                \
    (                                                           \
        (_x_)->GameContext     == (_y_)->GameContext      &&    \
        (_x_)->WriteAccessor   == (_y_)->WriteAccessor    &&    \
        (_x_)->ReadAccessor    == (_y_)->ReadAccessor           \
    )

    PAGED_CODE ();
    
    /*
     * Get a pointer to the device extension
     */
    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);
    

    /*
     *  No modifications to the Global List while we are looking at it
     */
    ExAcquireFastMutex (&Global.Mutex);

    /*
     *  For two joysticks interface two fdos are created to service them 
     *  but physically they both share the same port.
     *  For the second sibling certain extra rules must be applied so we 
     *  search our list of devices for another device using the same port 
     *  and if we find one mark this one as a sibling.
     */
    for(pEntry = Global.DeviceListHead.Flink;
       pEntry != &Global.DeviceListHead;
       pEntry = pEntry->Flink)
    {

        /*
         * Obtain the device Extension of the Sibling
         */
        DeviceExtension_Sibling = CONTAINING_RECORD(pEntry, DEVICE_EXTENSION, Link);

        if(       DeviceExtension_Sibling != DeviceExtension
               && TRUE == ARE_WE_RELATED(DeviceExtension, DeviceExtension_Sibling)
               && TRUE == DeviceExtension_Sibling->fStarted )
        {

                DeviceExtension->fSiblingFound = TRUE;
           break;
        }
    }

    /*
     *  We are done, release the Mutex
     */
    ExReleaseFastMutex (&Global.Mutex);

    /*
     * check the axis and button configuration for the joystick
     */
    ntStatus = HGM_JoystickConfig(DeviceObject);

    if( NT_SUCCESS( ntStatus ) )
    {
        /*
         *  Make sure that sibling axes are not overlapped
         */
        if(  DeviceExtension->fSiblingFound &&
             (DeviceExtension_Sibling->resistiveInputMask & DeviceExtension->resistiveInputMask) != 0x0 )
        {

            ntStatus = STATUS_DEVICE_CONFIGURATION_ERROR;

        }
    }

    return( ntStatus );

} /* HGM_InitAnalog */

/*****************************************************************************
 *
 *  @func   NTSTATUS  | HGM_JoystickConfig |
 *
 *          Check that the configuration is valid whilst there is still time 
 *          to refuse it.
 *          <nl>HGM_GenerateReport uses the results generated here if the 
 *          settings are OK.
 *
 *  @parm   IN PDEVICE_OBJECT | DeviceObject |
 *
 *          Pointer to the device object
 *
 *  @rvalue   STATUS_SUCCESS  | success
 *  @rvalue   STATUS_DEVICE_CONFIGURATION_ERROR  | Invalid configuration specified
 *
 *****************************************************************************/
NTSTATUS HGM_JoystickConfig 
    (
    IN PDEVICE_OBJECT         DeviceObject
    )
{
    PDEVICE_EXTENSION   DeviceExtension;
    POEMDATA            OemData;
    NTSTATUS            ntStatus;
    int                 Idx;

    PAGED_CODE();


    /*
     * Get a pointer to the device extension
     */

    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);

    ntStatus = HGM_MapAxesFromDevExt( DeviceExtension );

    {
        /*
         * Calculate time thresholds for analog device
         */
        if( ( DeviceExtension->HidGameOemData.OemData[0].Timeout < ANALOG_POLL_TIMEOUT_MIN )
          ||( DeviceExtension->HidGameOemData.OemData[0].Timeout > ANALOG_POLL_TIMEOUT_MAX ) )
        {
            DeviceExtension->ScaledTimeout = (ULONG)( ( (ULONGLONG)ANALOG_POLL_TIMEOUT_DFT
                                                      * (ULONGLONG)(AXIS_FULL_SCALE<<SCALE_SHIFT) )
                                                    / (ULONGLONG)ANALOG_POLL_TIMEOUT_MAX );
        }
        else
        {
            DeviceExtension->ScaledTimeout = (ULONG)( ( (ULONGLONG)DeviceExtension->HidGameOemData.OemData[0].Timeout
                                                      * (ULONGLONG)(AXIS_FULL_SCALE<<SCALE_SHIFT) )
                                                    / (ULONGLONG)ANALOG_POLL_TIMEOUT_MAX );
        }

        /*
         *  Use one quarter of the minimum poll timeout as a starting value 
         *  for the time between two polls which will be considered to have 
         *  been interrupted.
         */
        DeviceExtension->ScaledThreshold = (ULONG)( ( (ULONGLONG)ANALOG_POLL_TIMEOUT_MIN
                                                    * (ULONGLONG)AXIS_FULL_SCALE )
                                                  / (ULONGLONG)ANALOG_POLL_TIMEOUT_MAX )>>2;
    }


    /*
     *  Set initial values of LastGoodAxis so that the device will not show
     *  up as present until we get at least one valid poll.
     */
    for( Idx = 0; Idx < 4; Idx++ )
    {
        DeviceExtension->LastGoodAxis[Idx] = AXIS_TIMEOUT;
    }

    return ntStatus;
} /* HGM_JoystickConfig */




