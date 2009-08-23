/********************************************************************
 * Copyright(c) 2006 Broadcom Corporation, all rights reserved.
 * Contains proprietary and confidential information.
 *
 * This source file is the property of Broadcom Corporation, and
 * may not be copied or distributed in any isomorphic form without
 * the prior written consent of Broadcom Corporation.
 *
 *
 *  Name: bc_dts_glob.h
 *
 *  Description: Common Data structure definitions shared between
 *               DIL, driver and Applications
 *
 *  AU
 *
 *  HISTORY:
 *
 *******************************************************************/
#ifndef _BCM_DTS_GLOB_H_
#define _BCM_DTS_GLOB_H_

#include "bc_dts_defs.h"
#include "7411d.h"

/* =========== Work in progress flags =================*/
#define _LIB_FIX_ME_	1
#define _DRV_FIX_ME_	1

/* =========== Global Compile Time Flags =================*/
/* _LINK_COMPATIBLE_FPGA_ Information
 * ======================================
 * Define this flag and recompile the driver and DIL 
 * to switch between the old and new offsets.
 * On Driver Side:
 * ===============
 * 1. This flag will enable the Reset Handling for FPGA.
 *     a. On Startup bring out of reset.
 *	   b. On Shutdown Put the controller beack in Reset.
 * 2. When this flag is defined the the driver will use the NEW Offsets.
 *	  If this flag is not defined the driver will use the OLD Offsets
 *
 * On DIl Side:
 *=============
 * 1. The DIL Also uses different register definitions. Defining this flag 
 *    Will switch it to the new offset.
 */

#define _LINK_COMPATIBLE_FPGA_

#ifdef __cplusplus
extern "C" {
#endif

/* API exports for internal component Usage */
#if (defined(_WIN32) || defined(_WIN64)) && defined(_BC_EXPORT_INT_API)
	#include <windows.h>
	#include <stdio.h>
	#define DRVIFLIB_INT_API __declspec(dllexport)
#else
    #define DRVIFLIB_INT_API
#endif

#pragma pack (1)

enum _BC_PCI_DEV_IDS{
	BC_PCI_DEVID_INVALID = 0,
	BC_PCI_DEVID_DOZER   = 0x1610,
	BC_PCI_DEVID_TANK    = 0x1620,
	BC_PCI_DEVID_LINK	 = 0x1612,
	BC_PCI_DEVID_LOCKE	 = 0x1613,
	BC_PCI_DEVID_DEMOBRD = 0x7411,
	BC_PCI_DEVID_MORPHEUS = 0x7412,
};

/* 
 * NOTE OF CAUTION FOR SETTING THRESHOLD VALUES:
 * ==============================================
 * The Threshold values should be set so that the RX_START_DELIVERY_THRESHOLD 
 * should lie between RESUME_DECODER_THRESHOLD and PAUSE_DECODER_THRESHOLD.
 * Else we might never reach the Start Delivery Threshold and will pause 
 * the decoder even before that.
 */

/* Global tunable parameters */
enum _BC_DTS_GLOBALS{
	BC_MAX_FW_CMD_BUFF_SZ			= 0x40,		/* FW passthrough cmd/rsp buffer size */
	PCI_CFG_SIZE					= 256,		/* PCI config size buffer */
	BC_MAX_APPS_ALLOWED				= 2,		/* Number of simultaneous apps */
	BC_MAX_SW_VOUT_BUFFS			= 16,		/* Maximum number of pre-allocate SW video out buffers */
	BC_IOCTL_ASYNC_TIMEOUT			= 20000,	/* Milliseconds */
	BC_PROC_OUTPUT_TIMEOUT			= 20000,	/* Milliseconds */
	BC_IOCTL_DATA_POOL_SIZE			= 8,		/* BC_IOCTL_DATA Pool size */ 
	BC_FW_CMD_POLL_DELAY			= 1000,		/* Microseconds */
	SV_TIMEOUT_COUNT				= 10000,	/* Timeout Counter For FW Commands*/
	BC_MAX_DEVICE_OPEN			    = 3,		/* Max Number of open Handles for Device*/
	BC_PIB_CNT_FACTOR				= 2,		/* Multiplication factor.PIB buffer Cnt = [BC_MAX_SW_VOUT_BUFFS * BC_PIB_CNT_FACTOR] */
	BC_INPUT_MDATA_POOL_SZ			= 256,		/* Input Meta Data Pool size*/
	BC_DRIVER_DEFAULT_TIMEOUT		= 3000,		/* 3 sec in ms */
	BC_EOS_PIC_COUNT				= 16,		/* Count 16 frames with same Pic Number */
	RX_START_DELIVERY_THRESHOLD		= 0,		/* Start the delivery of Rx Frames if the Ready List Length goes to this value*/
	HARDWARE_INIT_RETRY_CNT			= 10,
	HARDWARE_INIT_RETRY_LINK_CNT	= 1,
};

// this is duplicated as a define elsewhere in avstream
#ifndef BC_INFIFO_THRESHOLD
enum _BC_DTS_GLOBALS2{
    BC_INFIFO_THRESHOLD				= 0x10000,	/* Input FIFO FULL/EMPTY qualifier*/
    PAUSE_DECODER_THRESHOLD			= 12,		/* Pause the decoder and discard the repeated frames if we reach this threshold */
    RESUME_DECODER_THRESHOLD		= 5	,		/* Resume the decoder and start collecting frames if we reach this threshold */
};
#endif

typedef struct _BC_PHY_ADDR{
	U32	AddrLow;
	U32	AddrHigh;
}BC_PHY_ADDR;

/* Bit fields */
enum _BCMemTypeFlags{
	BC_MEM_DEC_YUVBUFF		= 0x1,
	BC_MEM_USER_MODE_ALLOC	= 0x80000000, 
};

enum _STCapParams{
	NO_PARAM				= 0,
	ST_CAP_IMMIDIATE		= 0x01, 
};

typedef struct _BC_MEM_INFO{
	U8		*UserVa;
	U8		*kernVa;
	BC_PHY_ADDR	PhyAddr;
	U32		Size;
	U32		Flags;
}BC_MEM_INFO;

#ifdef _WIN64
/* Fixed Precision Structure */
typedef struct _BC_MEM_INFO_32_{
	U8* POINTER_32	UserVa;
	U8* POINTER_32	kernVa;
	BC_PHY_ADDR		PhyAddr;
	U32				Size;
	U32				Flags;
}BC_MEM_INFO_32;
#endif

typedef struct _BC_CMD_REG_ACC{
	U32			Offset;
	U32			Value;
}BC_CMD_REG_ACC;

typedef struct _BC_CMD_DEV_MEM{
	U32		StartOff;
	U32		NumDwords;
	U32		Rsrd;
}BC_CMD_DEV_MEM;

/* FW Passthrough command structure */
typedef struct _BC_FW_CMD{
	U32		cmd[BC_MAX_FW_CMD_BUFF_SZ];
	U32		rsp[BC_MAX_FW_CMD_BUFF_SZ];
}BC_FW_CMD,*PBC_FW_CMD;

typedef struct _BC_HW_TYPE{
	U16		PciDevId;
	U16		PciVenId;
	U8		HwRev;
	U8		Align[3];
}BC_HW_TYPE;

typedef struct _BC_PCI_CFG{
	U32		Size;							/* Size for Read/Write Operation*/
	U32		Offset;							/* Offset for Write Operation*/
	U8		pci_cfg_space[PCI_CFG_SIZE];	/* Buffer */
}BC_PCI_CFG;

typedef struct _BC_VERSION_INFO_{
	U8			DriverMajor;
	U8			DriverMinor;
	U16			DriverRevision;
}BC_VERSION_INFO;

typedef struct _BC_START_RX_CAP_{
	U32			Rsrd;						/* Used as Bit Map For Parameter Passing. see _STCapParams definition*/
	U32			StartDeliveryThsh;
	U32			PauseThsh;
	U32			ResumeThsh;	
	DecRspChannelStartVideo	SVidRsp;
}BC_START_RX_CAP;

typedef struct _BC_FLUSH_RX_CAP_{
	U32			Rsrd;
	U32			bDiscardOnly;				/* Dont do a full Flush Just Discard the Ready buffers */
}BC_FLUSH_RX_CAP;
typedef struct _BC_INIT_DRAM_MEM_{
	U32		Pattern;
	U32		offset;
	U32		ulSizeInDwords;
}BC_INIT_DRAM_MEM,*PBC_INIT_DRAM_MEM;
/**/

typedef struct _BC_DTS_STATS {
	U8			drvRLL;
	U8			drvFLL;
	U8			eosDetected;
	U8			res[1];

	/* Stats from App */
	U32			opFrameDropped;
	U32			opFrameCaptured;
	U32			ipSampleCnt;
	U64			ipTotalSize;
	U32			reptdFrames;
	U32			pauseCount;
	U32			pibMisses;
	U32			discCounter;
	U32			TxFifoBsyCnt;

	/* Stats from Driver */
	U32			intCount;
	U32			DrvIgnIntrCnt;
	U32			DrvTotalFrmDropped;
	U32			DrvTotalHWErrs;
	U32			DrvTotalPIBFlushCnt;
	U32			DrvTotalFrmCaptured;		
	U32			DrvPIBMisses;
	U32			DrvPauseTime;
	U32			DrvRepeatedFrms;

	/* 
	 * BIT-31 MEANS READ Next PIB Info.
	 * Width will be in bit 0-16.
	 */
	U32			DrvNextMDataPLD;	
	U32			res1[12];

}BC_DTS_STATS;

/**/

typedef struct _BC_PROC_INPUT_{
	U8			*pDmaBuff;
	U32			BuffSz;
	U8			Mapped;
	U8			Encrypted;
	U8			Rsrd[2];
	U32			DramOffset;			/* For debug use only */
}BC_PROC_INPUT,*PBC_PROC_INPUT;

#ifdef _WIN64
/* Fixed Precision Structure */
typedef struct _BC_PROC_INPUT_32_{
	U8*	POINTER_32	pDmaBuff;
	U32				BuffSz;
	U8				Mapped;
	U8				Encrypted;
	U8				Rsrd[2];
	U32				DramOffset;		/* For debug use only */
}BC_PROC_INPUT_32, *PBC_PROC_INPUT_32;
#endif

/* This structure is for internal Rx DMA buffer management 
 * DrivPriv & PibInfo are place holders for driver internal
 * house keeping and management.
 * Since we need to recycle the buffers we have done size
 * seperate so that the driver can fill in these values
 */
typedef struct _BC_DEC_YUV_BUFFS{
	U8			b422Mode;		
	BC_MEM_INFO	Ybuff;
	BC_MEM_INFO	UVbuff;
	U32			YBuffDoneSz;
	U32			UVBuffDoneSz;
	U32			RefCnt;
}BC_DEC_YUV_BUFFS;

#ifdef _WIN64
/* Fixed Precision Structure */
typedef struct _BC_DEC_YUV_BUFFS_32_{
	U8			b422Mode;
	BC_MEM_INFO_32	Ybuff;
	BC_MEM_INFO_32	UVbuff;
	U32			YBuffDoneSz;
	U32			UVBuffDoneSz;
	U32			RefCnt;
}BC_DEC_YUV_BUFFS_32;
#endif

enum _DECOUT_COMPLETION_FLAGS{
	COMP_FLAG_NO_INFO		= 0x00, 
	COMP_FLAG_FMT_CHANGE	= 0x01,
	COMP_FLAG_PIB_VALID		= 0x02,
	COMP_FLAG_DATA_VALID	= 0x04,
	COMP_FLAG_DATA_ENC		= 0x08,
	COMP_FLAG_DATA_BOT		= 0x10,
};

/* This structure will be used to fetch YUV and associated PIB
 * data from driver to user mode application. Driver will copy
 * PIB information after matching the sequence information with
 * appropriate YUV dmaed buffer.
 */
typedef struct _BC_DEC_OUT_BUFF{
	BC_DEC_YUV_BUFFS	OutPutBuffs;
	C011_PIB			PibInfo;
	U32					Flags;	
	U32					BadFrCnt;
}BC_DEC_OUT_BUFF;

#ifdef _WIN64
/* Fixed Precision Structure */
typedef struct _BC_DEC_OUT_BUFF_32_{
	BC_DEC_YUV_BUFFS_32	OutPutBuffs;
	C011_PIB			PibInfo;
	U32					Flags;	
	U32					BadFrCnt;
}BC_DEC_OUT_BUFF_32;
#endif

typedef struct _BC_NOTIFY_MODE {
	U32		Mode;
	U32		Rsvr[3];
}BC_NOTIFY_MODE;

enum _CANCEL_IO_TYPES {
	TX_REQUEST		= 0,
	RX_REQUEST		= 1,
	IOCTL_REQUEST	= 2,
};

typedef struct _BC_CANCEL_IO {
	U32		Op;
	U32		Rsvr[3];
	U32		Rsvr1[16];
}BC_CANCEL_IO;


#define	FRAME_720P_SZ		(1280 * 720)
#define PIXEL_422_SZ		(2)
#define PIXEL_444_SZ		(3)
#define LCD_444_FRAME_SZ	(FRAME_720P_SZ * PIXEL_444_SZ)

#define MAX_DESC_LIST				(1024)

typedef struct _BC_IOCTL_DATA{
	BC_STATUS	RetSts;		
	U32			IoctlDataSz;
	U32			Timeout;
	union {
		BC_MEM_INFO			mInfo;
		BC_CMD_REG_ACC		regAcc;
		BC_CMD_DEV_MEM		devMem;
		BC_FW_CMD			fwCmd;
		BC_HW_TYPE			hwType;
		BC_PCI_CFG			pciCfg;
		BC_VERSION_INFO		VerInfo;
		BC_INIT_DRAM_MEM	InitDramMem;
		BC_PROC_INPUT		ProcInput;
		BC_DEC_YUV_BUFFS	RxBuffs;
		BC_DEC_OUT_BUFF		DecOutData;
		BC_START_RX_CAP		RxCap;
		BC_FLUSH_RX_CAP		FlushRxCap;
		BC_DTS_STATS		drvStat;
		BC_NOTIFY_MODE		NotifyMode;
		BC_CANCEL_IO		CancelIo;
	}u;
	struct _BC_IOCTL_DATA	*next;
}BC_IOCTL_DATA;

#ifdef _WIN64
/* Fixed Precision Structure */
typedef struct _BC_IOCTL_DATA_32_ {
	BC_STATUS	RetSts;	
	U32			IoctlDataSz;
	U32			Timeout;
	union {
		BC_CMD_REG_ACC		regAcc;
		BC_CMD_DEV_MEM		devMem;
		BC_FW_CMD			fwCmd;
		BC_HW_TYPE			hwType;
		BC_PCI_CFG			pciCfg;
		BC_VERSION_INFO		VerInfo;
		BC_INIT_DRAM_MEM	InitDramMem;
		BC_START_RX_CAP		RxCap;
		BC_FLUSH_RX_CAP		FlushRxCap;
		BC_MEM_INFO_32		mInfo32;
		BC_PROC_INPUT_32	ProcInput32;
		BC_DEC_YUV_BUFFS_32	RxBuffs32;
		BC_DEC_OUT_BUFF_32	DecOutData32;
		BC_DTS_STATS		drvStat;
		BC_NOTIFY_MODE		NotifyMode;
		BC_CANCEL_IO		CancelIo;
	}u;
	struct _BC_IOCTL_DATA* POINTER_32	next;
}BC_IOCTL_DATA_32;
#endif

#pragma pack ()

typedef enum _BC_DRV_CMD{
	DRV_CMD_VERSION = 0,			/* Get SW version */
	DRV_CMD_GET_HWTYPE,				/* Get HW version and type Dozer/Tank */
	DRV_CMD_REG_RD,					/* Read Device Register */
	DRV_CMD_REG_WR,					/* Write Device Register */
	DRV_CMD_FPGA_RD,				/* Read FPGA Register */
	DRV_CMD_FPGA_WR,				/* Wrtie FPGA Reister */
	DRV_CMD_DRAM_INIT,				/* Initialize the DRAM */	
	DRV_CMD_MEM_RD,					/* Read Device Memory */
	DRV_CMD_MEM_WR,					/* Write Device Memory */
	DRV_CMD_MEM_INIT,				/* Initialize memory Pools */
	DRV_CMD_NOTIFY_FW_DNLD_ST,		/* NOTIFY FW DNLD Start to driver*/
	DRV_CMD_NOTIFY_FW_DNLD_DONE,	/* NOTIFY FW DNLD COMPLETED to driver*/
	DRV_CMD_RD_PCI_CFG,				/* Read PCI Config Space */
	DRV_CMD_WR_PCI_CFG,				/* Write the PCI Configuration Space*/
	DRV_CMD_EPROM_RD,				/* EEPROM Read */
	DRV_CMD_EPROM_WR,				/* EEPROM Write */

	DRV_CMD_FW_DOWNLOAD,			/* Download Firmware */
	DRV_ISSUE_FW_CMD,				/* Issue FW Cmd (pass through mode) */
	DRV_CMD_INIT_HW,				/* Initialize Hardware */

	DRV_CMD_PROC_INPUT,				/* Process Input Sample */
	
	DRV_CMD_ADD_RXBUFFS,			/* Add Rx side buffers to driver pool */
	DRV_CMD_FETCH_RXBUFF,			/* Get Rx DMAed buffer */
	DRV_CMD_REL_RXBUFFS,			/* Release all user allocated RX buffs */

	DRV_CMD_START_RX_CAP,			/* Start Rx Buffer Capture */
	DRV_CMD_FLUSH_RX_CAP,			/* Stop the capture for now...we will enhance this later*/
	DRV_CMD_GET_DRV_STAT,			/* Get Driver Internal Statistics */
	DRV_CMD_RST_DRV_STAT,			/* Reset Driver Internal Statistics */
	DRV_CMD_CANCEL_IO,				/* Cancel this IRP */
	DRV_CMD_NOTIFY_MODE,			/* Notify the Mode to driver in which the application is Operating*/

	/* MUST be the last one.. */
	DRV_CMD_END,			/* End of the List.. */
}BC_DRV_CMD;

typedef struct _BC_DEC_AES_CFG_INFO {
	U16		Mode;
	U16		yScramLen;
	U16		uvLen;
	U16		uvOffset;
	U16		yLen;
	U16		yOffset;
	U8		key[16];
	U8		initVector[16];
}BC_DEC_AES_CFG_INFO;

#define	 BC_AES_MODE_CBC		0x1
#define	 BC_AES_MODE_ECB		0x0
#define  BC_DRAM_AES_CFG_ADDR	0x001c2000

#define BC_AES_START_KEY_LOAD	0x1
#define BC_AES_KEY_LOAD_DONE	0x1

#define BE2LE(nLongNumber) (((nLongNumber & 0x000000FF) <<24 )+((nLongNumber & 0x0000FF00) <<8 )+ \
			((nLongNumber & 0x00FF0000) >> 8)+((nLongNumber & 0xFF000000) >> 24))

#ifdef __cplusplus
}
#endif

#endif
