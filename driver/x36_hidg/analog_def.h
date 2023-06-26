#define INVALID_INDEX               ( 0x80 )
#define NA ( 0x80 )

#define AXIS_X     0x01
#define AXIS_Y     0x02
#define AXIS_R     0x04
#define AXIS_Z     0x08
#define X1 AXIS_X
#define Y1 AXIS_Y
#define X2 AXIS_R
#define Y2 AXIS_Z

#define XMAPBITS    (JOY_HWS_XISJ2Y |   JOY_HWS_XISJ2X |   JOY_HWS_XISJ1Y)
/*
 *                          0                   0                   0           0001
 *                          0                   0                   1           0010
 *                          0                   1                   0           0100
 *                          1                   0                   0           1000
 */
#define XMAPSHFT 7

#define YMAPBITS    (JOY_HWS_YISJ2Y |   JOY_HWS_YISJ2X |   JOY_HWS_YISJ1X)
/*                          0                   0                   0           0010
 *                          0                   0                   1           0001
 *                          0                   1                   0           0100
 *                          1                   0                   0           1000
 */
#define YMAPSHFT 10

#define RMAPBITS    (JOY_HWS_RISJ2Y |   JOY_HWS_RISJ1X |   JOY_HWS_RISJ1Y)
/*                          0                   0                   0           0100
 *                          0                   0                   1           0010
 *                          0                   1                   0           0001
 *                          1                   0                   0           1000
 */
#define RMAPSHFT 20

#define ZMAPBITS    (JOY_HWS_ZISJ2X |   JOY_HWS_ZISJ1X |   JOY_HWS_ZISJ1Y)
/*                          0                   0                   0           1000
 *                          0                   0                   1           0010
 *                          0                   1                   0           0001
 *                          1                   0                   0           0100
 */
#define ZMAPSHFT 13

#define POVMAPBITS  (JOY_HWS_POVISJ2X | JOY_HWS_POVISJ1X | JOY_HWS_POVISJ1Y)
#define POVMAPSHFT 16  // POV is the same as Z but with a larger shift

#define XIS (0)
#define YIS (1)
#define ZIS (3)//(2)
#define RIS (2)//(3)

/* Specific settings for joystick hardware */
#define JOY_HWS_HASZ                ( 0x00000001l )     /* has Z info? */
#define JOY_HWS_HASR                ( 0x00080000l )     /* has R (4th axis) info */
#define JOY_HWS_HASPOV              ( 0x00000002l )     /* point of view hat present */
#define JOY_HWS_POVISBUTTONCOMBOS   ( 0x00000004l )     /* pov done through combo of buttons */
#define JOY_HWS_POVISPOLL           ( 0x00000008l )     /* pov done through polling */

#define JOY_HWS_ISYOKE              ( 0x00000010l )     /* joystick is a flight yoke */
#define JOY_HWS_ISGAMEPAD           ( 0x00000020l )     /* joystick is a game pad */
#define JOY_HWS_ISCARCTRL           ( 0x00000040l )     /* joystick is a car controller */

/* X defaults to J1 X axis */
    #define JOY_HWS_XISJ1Y              ( 0x00000080l )     /* X is on J1 Y axis */
    #define JOY_HWS_XISJ2X              ( 0x00000100l )     /* X is on J2 X axis */
    #define JOY_HWS_XISJ2Y              ( 0x00000200l )     /* X is on J2 Y axis */
/* Y defaults to J1 Y axis */
    #define JOY_HWS_YISJ1X              ( 0x00000400l )     /* Y is on J1 X axis */
    #define JOY_HWS_YISJ2X              ( 0x00000800l )     /* Y is on J2 X axis */
    #define JOY_HWS_YISJ2Y              ( 0x00001000l )     /* Y is on J2 Y axis */
/* Z defaults to J2 Y axis */
    #define JOY_HWS_ZISJ1X              ( 0x00002000l )     /* Z is on J1 X axis */
    #define JOY_HWS_ZISJ1Y              ( 0x00004000l )     /* Z is on J1 Y axis */
    #define JOY_HWS_ZISJ2X              ( 0x00008000l )     /* Z is on J2 X axis */
/* R defaults to J2 X axis */
    #define JOY_HWS_RISJ1X              ( 0x00100000l )     /* R done through J1 X axis */
    #define JOY_HWS_RISJ1Y              ( 0x00200000l )     /* R done through J1 Y axis */
    #define JOY_HWS_RISJ2Y              ( 0x00400000l )     /* R done through J2 X axis */
/* POV defaults to J2 Y axis, if it is not button based */
    #define JOY_HWS_POVISJ1X            ( 0x00010000l )     /* pov done through J1 X axis */
    #define JOY_HWS_POVISJ1Y            ( 0x00020000l )     /* pov done through J1 Y axis */
    #define JOY_HWS_POVISJ2X            ( 0x00040000l )     /* pov done through J2 X axis */

    #define JOY_HWS_HASPOV2             JOY_HWS_POVISJ2X
