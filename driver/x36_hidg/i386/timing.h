/*
 *  A value guaranteed to be considered a timeout
 */
    #define AXIS_TIMEOUT            ( 0xffffffffL )

/*
 *  If you change any of the scaling or timeout values you have to check
 *  that overflows are still avoided under reasonable circumstances.
 */

/*
 *  The timeout value should be passed in DEVICE_EXTENSION.oemData.Timeout
 *  so these limits are a sanity check and default value
 *  The largest expected value is 8mS, use 10 for safety
 */
    #define ANALOG_POLL_TIMEOUT_MIN     (   100L )
    #define ANALOG_POLL_TIMEOUT_DFT     ( 10000L )
    #define ANALOG_POLL_TIMEOUT_MAX     ( 20000L )

/*
 *  Valid axis values are scaled such that a poll of exactly 
 *  ANALOG_POLL_TIMEOUT_MAX mS should return this value
 *  Any analog value greater than this is a timeout
 */
    #define AXIS_FULL_SCALE         ( ANALOG_POLL_TIMEOUT_MAX )

/*
 *  Slowest CPU frequency considered when calibrating the CPU timer 
 *  against the performance counter.
 */

    #define HIDGAME_SLOWEST_X86_HZ      ( 45000000 )

/*
 *  Number of bits to shift left to get a scaled value
 *  This is used so that we can always use an integer multiply of the 
 *  number of counter ticks the poll took to scale the value.
 */
    #define SCALE_SHIFT             16

/*
 *  Macro to calculate a scaling factor from a (ULONGLONG)frequency
 */
#if AXIS_FULL_SCALE == ANALOG_POLL_TIMEOUT_MAX
    #define CALCULATE_SCALE( _Freq_ ) \
        (ULONG)( ( (ULONGLONG)( 1000000 ) << SCALE_SHIFT ) \
               / _Freq_ )
#else
    #define CALCULATE_SCALE( _Freq_ ) \
        (ULONG)( ( (ULONGLONG)AXIS_FULL_SCALE \
                 * ( (ULONGLONG)( 1000000 ) << SCALE_SHIFT ) ) \
                   / ANALOG_POLL_TIMEOUT_MAX ) \
               / _Freq_ )
#endif


/*
 * <CPU>\timing.c (or macro equivalents for external functions)
 */


#ifdef _X86_

BOOLEAN HGM_x86IsClockAvailable
    (
    VOID
    );

LARGE_INTEGER HGM_x86ReadCounter
    (
    IN      PLARGE_INTEGER      Dummy
    );

VOID HGM_x86SampleClocks
    (
    OUT PULONGLONG  pTSC,
    OUT PULONGLONG  pQPC
    );

BOOLEAN HGM_x86CounterInit();
#define HGM_CPUCounterInit HGM_x86CounterInit

#else

/*
 *  For all other processors a value to cause the default timing to be used
 */

#define HGM_CPUCounterInit() FALSE

#endif /* _X86_ */
