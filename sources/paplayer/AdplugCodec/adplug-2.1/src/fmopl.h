#ifndef __FMOPL_H_
#define __FMOPL_H_

/* --- select emulation chips --- */
#define BUILD_YM3812 (HAS_YM3812)
//#define BUILD_YM3526 (HAS_YM3526)
//#define BUILD_Y8950  (HAS_Y8950)

/* --- system optimize --- */
/* select bit size of output : 8 or 16 */
#define OPL_OUTPUT_BIT 16

/* compiler dependence */
#ifndef OSD_CPU_H
#define OSD_CPU_H
typedef unsigned char	UINT8;   /* unsigned  8bit */
typedef unsigned short	UINT16;  /* unsigned 16bit */
typedef unsigned int	UINT32;  /* unsigned 32bit */
typedef signed char		INT8;    /* signed  8bit   */
typedef signed short	INT16;   /* signed 16bit   */
typedef signed int		INT32;   /* signed 32bit   */
#endif

#if (OPL_OUTPUT_BIT==16)
typedef INT16 OPLSAMPLE;
#endif
#if (OPL_OUTPUT_BIT==8)
typedef unsigned char  OPLSAMPLE;
#endif


#if BUILD_Y8950
#include "ymdeltat.h"
#endif

typedef void (*OPL_TIMERHANDLER)(int channel,double interval_Sec);
typedef void (*OPL_IRQHANDLER)(int param,int irq);
typedef void (*OPL_UPDATEHANDLER)(int param,int min_interval_us);
typedef void (*OPL_PORTHANDLER_W)(int param,unsigned char data);
typedef unsigned char (*OPL_PORTHANDLER_R)(int param);

/* !!!!! here is private section , do not access there member direct !!!!! */

#define OPL_TYPE_WAVESEL   0x01  /* waveform select    */
#define OPL_TYPE_ADPCM     0x02  /* DELTA-T ADPCM unit */
#define OPL_TYPE_KEYBOARD  0x04  /* keyboard interface */
#define OPL_TYPE_IO        0x08  /* I/O port */

/* Saving is necessary for member of the 'R' mark for suspend/resume */
/* ---------- OPL one of slot  ---------- */
typedef struct fm_opl_slot {
	INT32 TL;		/* total level     :TL << 8            */
	INT32 TLL;		/* adjusted now TL                     */
	UINT8  KSR;		/* key scale rate  :(shift down bit)   */
	INT32 *AR;		/* attack rate     :&AR_TABLE[AR<<2]   */
	INT32 *DR;		/* decay rate      :&DR_TALBE[DR<<2]   */
	INT32 SL;		/* sustin level    :SL_TALBE[SL]       */
	INT32 *RR;		/* release rate    :&DR_TABLE[RR<<2]   */
	UINT8 ksl;		/* keyscale level  :(shift down bits)  */
	UINT8 ksr;		/* key scale rate  :kcode>>KSR         */
	UINT32 mul;		/* multiple        :ML_TABLE[ML]       */
	UINT32 Cnt;		/* frequency count :                   */
	UINT32 Incr;	/* frequency step  :                   */
	/* envelope generator state */
	UINT8 eg_typ;	/* envelope type flag                  */
	UINT8 evm;		/* envelope phase                      */
	INT32 evc;		/* envelope counter                    */
	INT32 eve;		/* envelope counter end point          */
	INT32 evs;		/* envelope counter step               */
	INT32 evsa;	/* envelope step for AR :AR[ksr]           */
	INT32 evsd;	/* envelope step for DR :DR[ksr]           */
	INT32 evsr;	/* envelope step for RR :RR[ksr]           */
	/* LFO */
	UINT8 ams;		/* ams flag                            */
	UINT8 vib;		/* vibrate flag                        */
	/* wave selector */
	INT32 **wavetable;
}OPL_SLOT;

/* ---------- OPL one of channel  ---------- */
typedef struct fm_opl_channel {
	OPL_SLOT SLOT[2];
	UINT8 CON;			/* connection type                     */
	UINT8 FB;			/* feed back       :(shift down bit)   */
	INT32 *connect1;	/* slot1 output pointer                */
	INT32 *connect2;	/* slot2 output pointer                */
	INT32 op1_out[2];	/* slot1 output for selfeedback        */
	/* phase generator state */
	UINT32  block_fnum;	/* block+fnum      :                   */
	UINT8 kcode;		/* key code        : KeyScaleCode      */
	UINT32  fc;			/* Freq. Increment base                */
	UINT32  ksl_base;	/* KeyScaleLevel Base step             */
	UINT8 keyon;		/* key on/off flag                     */
} OPL_CH;

/* OPL state */
typedef struct fm_opl_f {
	UINT8 type;			/* chip type                         */
	int clock;			/* master clock  (Hz)                */
	int rate;			/* sampling rate (Hz)                */
	double freqbase;	/* frequency base                    */
	double TimerBase;	/* Timer base time (==sampling time) */
	UINT8 address;		/* address register                  */
	UINT8 status;		/* status flag                       */
	UINT8 statusmask;	/* status mask                       */
	UINT32 mode;		/* Reg.08 : CSM , notesel,etc.       */
	/* Timer */
	int T[2];			/* timer counter                     */
	UINT8 st[2];		/* timer enable                      */
	/* FM channel slots */
	OPL_CH *P_CH;		/* pointer of CH                     */
	int	max_ch;			/* maximum channel                   */
	/* Rythm sention */
	UINT8 rythm;		/* Rythm mode , key flag */
#if BUILD_Y8950
	/* Delta-T ADPCM unit (Y8950) */
	YM_DELTAT *deltat;			/* DELTA-T ADPCM       */
#endif
	/* Keyboard / I/O interface unit (Y8950) */
	UINT8 portDirection;
	UINT8 portLatch;
	OPL_PORTHANDLER_R porthandler_r;
	OPL_PORTHANDLER_W porthandler_w;
	int port_param;
	OPL_PORTHANDLER_R keyboardhandler_r;
	OPL_PORTHANDLER_W keyboardhandler_w;
	int keyboard_param;
	/* time tables */
	INT32 AR_TABLE[75];	/* atttack rate tables */
	INT32 DR_TABLE[75];	/* decay rate tables   */
	UINT32 FN_TABLE[1024];  /* fnumber -> increment counter */
	/* LFO */
	INT32 *ams_table;
	INT32 *vib_table;
	INT32 amsCnt;
	INT32 amsIncr;
	INT32 vibCnt;
	INT32 vibIncr;
	/* wave selector enable flag */
	UINT8 wavesel;
	/* external event callback handler */
	OPL_TIMERHANDLER  TimerHandler;		/* TIMER handler   */
	int TimerParam;						/* TIMER parameter */
	OPL_IRQHANDLER    IRQHandler;		/* IRQ handler    */
	int IRQParam;						/* IRQ parameter  */
	OPL_UPDATEHANDLER UpdateHandler;	/* stream update handler   */
	int UpdateParam;					/* stream update parameter */
} FM_OPL;

/* ---------- Generic interface section ---------- */
#define OPL_TYPE_YM3526 (0)
#define OPL_TYPE_YM3812 (OPL_TYPE_WAVESEL)
#define OPL_TYPE_Y8950  (OPL_TYPE_ADPCM|OPL_TYPE_KEYBOARD|OPL_TYPE_IO)

FM_OPL *OPLCreate(int type, int clock, int rate);
void OPLDestroy(FM_OPL *OPL);
void OPLSetTimerHandler(FM_OPL *OPL,OPL_TIMERHANDLER TimerHandler,int channelOffset);
void OPLSetIRQHandler(FM_OPL *OPL,OPL_IRQHANDLER IRQHandler,int param);
void OPLSetUpdateHandler(FM_OPL *OPL,OPL_UPDATEHANDLER UpdateHandler,int param);
/* Y8950 port handlers */
void OPLSetPortHandler(FM_OPL *OPL,OPL_PORTHANDLER_W PortHandler_w,OPL_PORTHANDLER_R PortHandler_r,int param);
void OPLSetKeyboardHandler(FM_OPL *OPL,OPL_PORTHANDLER_W KeyboardHandler_w,OPL_PORTHANDLER_R KeyboardHandler_r,int param);

void OPLResetChip(FM_OPL *OPL);
int OPLWrite(FM_OPL *OPL,int a,int v);
unsigned char OPLRead(FM_OPL *OPL,int a);
int OPLTimerOver(FM_OPL *OPL,int c);

/* YM3626/YM3812 local section */
void YM3812UpdateOne(FM_OPL *OPL, INT16 *buffer, int length);

void Y8950UpdateOne(FM_OPL *OPL, INT16 *buffer, int length);

#endif
