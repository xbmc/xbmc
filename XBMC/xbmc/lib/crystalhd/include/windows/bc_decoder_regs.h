#ifndef _INCLUDE_DECO_REGS_H_
#define _INCLUDE_DECO_REGS_H_

#include "bc_dts_types.h"

// These are SDRAM specific registers 
#define SDRAM_PARAM					0x00040804
#define SDRAM_REF_PARAM				0x00040808
#define SDRAM_REFRESH				0x00040890
#define SDRAM_MODE					0x000408A0
#define SDRAM_EXT_MODE				0x000408A4
#define SDRAM_PRECHARGE				0x000408B0
#define SDRAM_INC					0x00040800

// Registers to access the DRAM
#define TOTAL_DRAM_SIZE				(64 * 1024 * 1024) // We are using 64MB of DRAM
#define	DRAM_ACCESS_GRANUALITY		4				   // We will always access DRAM ULONG by ULONG.
#define DRAM_WINDOW_SIZE			(512 * 1024)	   // 512 K.
#define DRAM_WINDOW_BASE			0x00340020		   // DRAM Address to access 512K Size of data.
#define	DRAM_SHADOW_DATA_START		0x00380000		   // Start of 512 K of window for shadow data.
#define DRAM_SHADOW_DATA_END		0x003FFFFF		   // End of 512 K of window for shadow data.

#define AUD_DSP_MISC_SOFT_RESET		0x00240104
#define AIO_MISC_PLL_RESET			0x0026000C
//
// To Reset the controller
//
#define DEC_HOST_SW_RESET			0x00340000



/* Register Map */
#define TS_Host2CpuSnd				0x00000100
#define Hst2CpuMbx1					0x00100F00
#define Cpu2HstMbx1					0x00100F04
#define MbxStat1					0x00100F08
#define Stream2Host_Intr_Sts		0x00100F24

typedef union _STREAM_TO_HOST_INTR_STS_{

	struct {
		ULONG	Res1:1;							/* Reserved */	
		ULONG	VideoSetupOut0:1;				/* Video Setup Intr Occured at port0. This means that the picture is ready to be displayed */
		ULONG	VideoReleaseOut0:1;				/* Video Release Intr Occured at port0. This means that picture is almost Done */
		ULONG	AsynchEvent:1;					/* Video Setup Intr Occured at port1. This means that the picture is ready to be displayed */
		ULONG	Res2:1;							/* Reserved */
		ULONG	PicAvailIn0:1;					/* PIC Avail Interrupt at port 0*/
		ULONG	PicAvailIn1:1;					/* PIC Avail Interrupt at port 1*/
		ULONG	CRCDataAvail0:1;				/* Intr Occured from Stream Entering at port 0*/
		ULONG	Res3:1;							/* Reserved */
		ULONG	UserDataAvail0:1;				/* User Data Intr occured for stream entring at port 0*/
		ULONG	NewPCROffset:1;					/* New PCR offset Intr Occured*/
		ULONG	ErrNotify:1;					/* An Err Notify Intr Occured*/
		ULONG	HostDMAComplete:1;				/* PCI Host Dma Interrupt occured*/
		ULONG	AudioDecService:1;				/* Audio Decoder Intr Occured*/
		ULONG	InitalPTS:1;					/* First Presentation Time Stamp recieved. Host should then write a new STC when in playback mode*/
		ULONG	PTSDisc:1;						/* A PTS discontinuity has occured*/
		ULONG	Resv4:15;						/* Reserved*/
		ULONG	MailboxIntr:1;					/* A Command Respose Mailbox interrupt occured */
	};

	ULONG	WholeReg;							/* If you want to access whole register without the bitmap*/
}STRTOHOST_INTR_STS,*PSTRTOHOST_INTR_STS;

#define REG_DecCA_RegCinBase	0xa0c
#define REG_DecCA_RegCinEnd		0xa10
#define REG_DecCA_RegCinWrPtr	0xa04
#define REG_DecCA_RegCinRdPtr	0xa08

/* TS case.. */
#define REG_Dec_TsUser0Base		0x100864
#define REG_Dec_TsUser0Rdptr	0x100868
#define REG_Dec_TsUser0Wrptr	0x10086C
#define REG_Dec_TsUser0Len		0x100870
#define REG_Dec_TsUser0End		0x100874
#define REG_Dec_TsUser0Empty	0x100878


/* ASF Case ...*/
#define REG_Dec_TsAudCDB2Base		0x10036c
#define REG_Dec_TsAudCDB2Rdptr	    0x100378
#define REG_Dec_TsAudCDB2Wrptr	    0x100374
#define REG_Dec_TsAudCDB2End		0x100370



// ----------- registers and bits for master mode DMA bursts into 
// ----------- block mode code-in port

// -- code in block addresses
#define BCMPCI_HOST_STREAMA_WINDOW_BASE	0x340200
#define BCMPVI_HOST_STREAMA_WINDOW_END 	0x34023f

// -- DMA registers
#define BCMPCI_DMA_CHAN0_SRC					0x340100
#define BCMPCI_DMA_CHAN0_DEST					0x340104
#define BCMPCI_DMA_CHAN0_CTL					0x340108
#define BCMPCI_ICR								0x0001ec

// -- bits in control register
// read only status
#define BCMPCI_IS_DMA_ACTIVE					0x80000000
#define BCMPCI_IS_DMA_ERROR						0x40000000
#define BCMPCI_IS_DMA_INT						0x20000000

// control bits
#define BCMPCI_IS_LOCAL_SDRAM					0x400000
#define BCMPCI_INC_DST							0x200000
#define BCMPCI_INC_SRC							0x100000


// bit mask denoting area containing number of bytes to transfer
#define BCMPCI_DMA_BYTES_MASK					0xfffff


#define VectorTbl1 (UINT32) 0x00100F0C
#define CpuDbg1    (UINT32) 0x00141010
#define AuxRegs1   (UINT32) 0x00145000


#define INIT_VEC1  (UINT32) 0x00000000


#define UartSelectA		 (UINT32) 0x00100300
#define UartSelectB		 (UINT32) 0x00100304

#define DecHt_HostSwReset	(UINT32) 0x340000


#define TSHostStreamA		0x34002c
#define StrTRA_TsFifoStatus 0x10044c

#define DecHt_PllACtl		0x34000C
#define DecHt_PllBCtl		0x340010
#define DecHt_PllCCtl		0x340014
#define DecHt_PllDCtl		0x340034
#define DecHt_PllECtl		0x340038


#define DQS_CTL_REGISTER			0x00040700
#define DDR_DRIVER_CTL_REGISTER		0x00040704

typedef union _DQS_CTL_REG_
{
	struct{
		ULONG	DQS0_DELAY:4;
		ULONG	DQS1_DELAY:4;
		ULONG	DQS2_DELAY:4;
		ULONG	DQS3_DELAY:4;
		ULONG	PULSE_WIDTH:4;
		ULONG	MHZ:4;
		ULONG	OV:1;
		ULONG	SEN:1;
		ULONG	CL25:1;
		ULONG	RSV:5;
	};
	ULONG	WholeReg;
} DQS_CTL_REG;

typedef union _DDR_DRIVER_CTL_REG_
{
	struct{
		ULONG	DDQ:2;
		ULONG	SDQ:2;
		ULONG	CL2DQ:1;
		ULONG	RSV:3;
		ULONG	DCTL:2;
		ULONG	SCTL:2;
		ULONG	CL2CTL:1;
		ULONG	RSV1:3;
		ULONG	RSV2:16;
	};
	ULONG	WholeReg;
} DDR_DRIVER_CTL_REG;

#define HALF_EMPTY_BIT 0x80
#define FIFO_HALF_EMPTY(a)\
	(a & HALF_EMPTY_BIT )
#define FULL_BIT 0x20
#define FIFO_FULL(a)\
	(a & FULL_BIT)


#endif
