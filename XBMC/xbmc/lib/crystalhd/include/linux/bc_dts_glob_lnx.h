
/********************************************************************
 * Copyright(c) 2006 Broadcom Corporation, all rights reserved.
 * Contains proprietary and confidential information.
 *
 * This source file is the property of Broadcom Corporation, and
 * may not be copied or distributed in any isomorphic form without
 * the prior written consent of Broadcom Corporation.
 *
 *
 *  Name: bc_dts_glob_lnx.h
 *
 *  Description: Wrapper to Windows dts_glob.h for Link-Linux usage.
 *  		 The idea is to define additional Linux related defs
 *  		 in this file to avoid changes to existing Windows
 *  		 glob file.
 *
 *  AU
 *
 *  HISTORY:
 *
 *******************************************************************/
#ifndef _BC_DTS_GLOB_LNX_H_
#define _BC_DTS_GLOB_LNX_H_

#ifdef __LINUX_USER__
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <stropts.h>
#include <errno.h>
#include <netdb.h>
#include <sys/time.h>
#include <time.h>
#include <arpa/inet.h>
#include <asm/param.h>
#include <linux/ioctl.h>
#include <sys/select.h>

#define DRVIFLIB_INT_API

#endif

#include "bc_dts_defs.h"
#include "bcm_70012_regs.h"		/* Link Register defs..*/

#define MPC_LINK_API_NAME                  "mpclink"
#define MPC_LINK_API_DEV_NAME              "/dev/mpclink"

/*
 * These are SW stack tunable parameters shared 
 * between the driver and the application. 
 */
enum _BC_DTS_GLOBALS{
	BC_MAX_FW_CMD_BUFF_SZ		= 0x40,		/* FW passthrough cmd/rsp buffer size */
	PCI_CFG_SIZE			= 256,		/* PCI config size buffer */
	BC_IOCTL_DATA_POOL_SIZE		= 8,		/* BC_IOCTL_DATA Pool size */ 
	BC_LINK_MAX_OPENS       	= 3,		/* Maximum simultaneous opens*/
	BC_LINK_MAX_SGLS        	= 1024,		/* Maximum SG elements 4M/4K */
	BC_TX_LIST_CNT			= 2,		/* Max Tx DMA Rings */		
	BC_RX_LIST_CNT			= 8,		/* Max Rx DMA Rings*/		
	BC_PROC_OUTPUT_TIMEOUT		= 3000,	/* Milliseconds */
	BC_INFIFO_THRESHOLD            	= 0x10000,
};

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
enum _bc_fw_cmd_flags{
	BC_FW_CMD_FLAGS_NONE   	= 0,
	BC_FW_CMD_PIB_QS 	= 0x01,
};

typedef struct _BC_FW_CMD{
	U32		cmd[BC_MAX_FW_CMD_BUFF_SZ];
	U32		rsp[BC_MAX_FW_CMD_BUFF_SZ];
	U32		flags;
	U32		add_data;
}BC_FW_CMD,*PBC_FW_CMD;

typedef struct _BC_HW_TYPE{
	U16		PciDevId;
	U16		PciVenId;
	U8		HwRev;
	U8		Align[3];
}BC_HW_TYPE;

typedef struct _BC_PCI_CFG{
	U32		Size;
	U32		Offset;	
	U8		pci_cfg_space[PCI_CFG_SIZE];
}BC_PCI_CFG;

typedef struct _BC_VERSION_INFO_{
	U8			DriverMajor;
	U8			DriverMinor;
	U16			DriverRevision;
}BC_VERSION_INFO;

typedef struct _BC_START_RX_CAP_{
	U32			Rsrd;		
	U32			StartDeliveryThsh;
	U32			PauseThsh;
	U32			ResumeThsh;	
}BC_START_RX_CAP;

typedef struct _BC_FLUSH_RX_CAP_{
	U32			Rsrd;
	U32			bDiscardOnly;
}BC_FLUSH_RX_CAP;

typedef struct _BC_DTS_STATS {
	U8			drvRLL;
	U8			drvFLL;
	U8			eosDetected;
	U8			pwr_state_change;

	/* Stats from App */
	U32			opFrameDropped;
	U32			opFrameCaptured;
	U32			ipSampleCnt;
	U64			ipTotalSize;
	U32			reptdFrames;
	U32			pauseCount;
	U32			pibMisses;
	U32			discCounter;

	/* Stats from Driver */
	U32			TxFifoBsyCnt;
	U32			intCount;
	U32			DrvIgnIntrCnt;
	U32			DrvTotalFrmDropped;
	U32			DrvTotalHWErrs;
	U32			DrvTotalPIBFlushCnt;
	U32			DrvTotalFrmCaptured;		
	U32			DrvPIBMisses;
	U32			DrvPauseTime;
	U32			DrvRepeatedFrms;
	U32			res1[13];

}BC_DTS_STATS;

typedef struct _BC_PROC_INPUT_{
	U8			*pDmaBuff;
	U32			BuffSz;
	U8			Mapped;
	U8			Encrypted;
	U8			Rsrd[2];
	U32			DramOffset;			/* For debug use only */
}BC_PROC_INPUT,*PBC_PROC_INPUT;

typedef struct _BC_DEC_YUV_BUFFS{
	U32			b422Mode;		
	U8			*YuvBuff;
	U32			YuvBuffSz;
	U32			UVbuffOffset;
	U32			YBuffDoneSz;
	U32			UVBuffDoneSz;
	U32			RefCnt;
}BC_DEC_YUV_BUFFS;

enum _DECOUT_COMPLETION_FLAGS{
	COMP_FLAG_NO_INFO		= 0x00, 
	COMP_FLAG_FMT_CHANGE		= 0x01,
	COMP_FLAG_PIB_VALID		= 0x02,
	COMP_FLAG_DATA_VALID		= 0x04,
	COMP_FLAG_DATA_ENC		= 0x08,
	COMP_FLAG_DATA_BOT		= 0x10,
};

typedef struct _BC_DEC_OUT_BUFF{
	BC_DEC_YUV_BUFFS	OutPutBuffs;
	BC_PIC_INFO_BLOCK	PibInfo;
	U32			Flags;	
	U32			BadFrCnt;
}BC_DEC_OUT_BUFF;

typedef struct _BC_NOTIFY_MODE {
	U32		Mode;
	U32		Rsvr[3];
}BC_NOTIFY_MODE;

typedef struct _BC_CLOCK {
	U32		clk;
	U32		Rsvr[3];
} BC_CLOCK;

typedef struct _BC_IOCTL_DATA{
	BC_STATUS	RetSts;		
	U32		IoctlDataSz;
	U32		Timeout;
	union {
		BC_CMD_REG_ACC		regAcc;
		BC_CMD_DEV_MEM		devMem;
		BC_FW_CMD		fwCmd;
		BC_HW_TYPE		hwType;
		BC_PCI_CFG		pciCfg;
		BC_VERSION_INFO		VerInfo;
		BC_PROC_INPUT		ProcInput;
		BC_DEC_YUV_BUFFS	RxBuffs;
		BC_DEC_OUT_BUFF		DecOutData;
		BC_START_RX_CAP		RxCap;
		BC_FLUSH_RX_CAP		FlushRxCap;
		BC_DTS_STATS		drvStat;
		BC_NOTIFY_MODE		NotifyMode;
		BC_CLOCK			clockValue;
	}u;
	struct _BC_IOCTL_DATA	*next;
}BC_IOCTL_DATA;

typedef enum _BC_DRV_CMD{
	DRV_CMD_VERSION = 0,		/* Get SW version */
	DRV_CMD_GET_HWTYPE,		/* Get HW version and type Dozer/Tank */
	DRV_CMD_REG_RD,			/* Read Device Register */
	DRV_CMD_REG_WR,			/* Write Device Register */
	DRV_CMD_FPGA_RD,		/* Read FPGA Register */
	DRV_CMD_FPGA_WR,		/* Wrtie FPGA Reister */
	DRV_CMD_MEM_RD,			/* Read Device Memory */
	DRV_CMD_MEM_WR,			/* Write Device Memory */
	DRV_CMD_RD_PCI_CFG,		/* Read PCI Config Space */
	DRV_CMD_WR_PCI_CFG,		/* Write the PCI Configuration Space*/
	DRV_CMD_FW_DOWNLOAD,		/* Download Firmware */
	DRV_ISSUE_FW_CMD,		/* Issue FW Cmd (pass through mode) */
	DRV_CMD_PROC_INPUT,		/* Process Input Sample */
	DRV_CMD_ADD_RXBUFFS,		/* Add Rx side buffers to driver pool */
	DRV_CMD_FETCH_RXBUFF,		/* Get Rx DMAed buffer */
	DRV_CMD_START_RX_CAP,		/* Start Rx Buffer Capture */
	DRV_CMD_FLUSH_RX_CAP,		/* Stop the capture for now...we will enhance this later*/
	DRV_CMD_GET_DRV_STAT,		/* Get Driver Internal Statistics */
	DRV_CMD_RST_DRV_STAT,		/* Reset Driver Internal Statistics */
	DRV_CMD_NOTIFY_MODE,		/* Notify the Mode to driver in which the application is Operating*/
	DRV_CMD_CHANGE_CLOCK,	/* Change the core clock to either save power or improve performance */
	
	/* MUST be the last one.. */
	DRV_CMD_END,			/* End of the List.. */
}BC_DRV_CMD;

#define BC_IOC_BASE			'b'
#define BC_IOC_VOID			_IOC_NONE
#define BC_IOC_IOWR(nr,type)		_IOWR(BC_IOC_BASE,nr,type)
#define BC_IOCTL_MB			BC_IOCTL_DATA	


#define	BCM_IOC_GET_VERSION		BC_IOC_IOWR(DRV_CMD_VERSION, BC_IOCTL_MB) 
#define	BCM_IOC_GET_HWTYPE		BC_IOC_IOWR(DRV_CMD_GET_HWTYPE, BC_IOCTL_MB) 
#define	BCM_IOC_REG_RD			BC_IOC_IOWR(DRV_CMD_REG_RD, BC_IOCTL_MB) 
#define	BCM_IOC_REG_WR			BC_IOC_IOWR(DRV_CMD_REG_WR, BC_IOCTL_MB) 
#define	BCM_IOC_MEM_RD			BC_IOC_IOWR(DRV_CMD_MEM_RD, BC_IOCTL_MB) 
#define	BCM_IOC_MEM_WR			BC_IOC_IOWR(DRV_CMD_MEM_WR, BC_IOCTL_MB) 
#define BCM_IOC_FPGA_RD			BC_IOC_IOWR(DRV_CMD_FPGA_RD, BC_IOCTL_MB)
#define BCM_IOC_FPGA_WR			BC_IOC_IOWR(DRV_CMD_FPGA_WR, BC_IOCTL_MB)
#define	BCM_IOC_RD_PCI_CFG		BC_IOC_IOWR(DRV_CMD_RD_PCI_CFG, BC_IOCTL_MB) 
#define	BCM_IOC_WR_PCI_CFG		BC_IOC_IOWR(DRV_CMD_WR_PCI_CFG, BC_IOCTL_MB) 
#define BCM_IOC_PROC_INPUT		BC_IOC_IOWR(DRV_CMD_PROC_INPUT, BC_IOCTL_MB)
#define BCM_IOC_ADD_RXBUFFS		BC_IOC_IOWR(DRV_CMD_ADD_RXBUFFS, BC_IOCTL_MB)
#define BCM_IOC_FETCH_RXBUFF		BC_IOC_IOWR(DRV_CMD_FETCH_RXBUFF, BC_IOCTL_MB)
#define	BCM_IOC_FW_CMD			BC_IOC_IOWR(DRV_ISSUE_FW_CMD, BC_IOCTL_MB) 
#define	BCM_IOC_START_RX_CAP		BC_IOC_IOWR(DRV_CMD_START_RX_CAP, BC_IOCTL_MB) 
#define BCM_IOC_FLUSH_RX_CAP		BC_IOC_IOWR(DRV_CMD_FLUSH_RX_CAP, BC_IOCTL_MB) 
#define BCM_IOC_GET_DRV_STAT		BC_IOC_IOWR(DRV_CMD_GET_DRV_STAT, BC_IOCTL_MB) 
#define BCM_IOC_RST_DRV_STAT		BC_IOC_IOWR(DRV_CMD_RST_DRV_STAT, BC_IOCTL_MB) 
#define BCM_IOC_NOTIFY_MODE		BC_IOC_IOWR(DRV_CMD_NOTIFY_MODE, BC_IOCTL_MB)
#define	BCM_IOC_FW_DOWNLOAD		BC_IOC_IOWR(DRV_CMD_FW_DOWNLOAD, BC_IOCTL_MB) 
#define BCM_IOC_CHG_CLK			BC_IOC_IOWR(DRV_CMD_CHANGE_CLOCK, BC_IOCTL_MB)
#define	BCM_IOC_END			BC_IOC_VOID

/* Wrapper for  main IOCTL data */
typedef struct _bc_link_ioctl_data{
	BC_IOCTL_DATA			udata;		/* IOCTL from App..*/
	uint32_t			u_id;		/* Driver specific user ID */
	uint32_t			cmd;		/* Cmd ID for driver's use. */
	void				*add_cdata;	/* Additional command specific data..*/
	uint32_t			add_cdata_sz;	/* Additional command specific data size */
	struct _bc_link_ioctl_data	*next;		/* List/Fifo management */
}bc_link_ioctl_data;


enum _mpc_link_kmod_ver{
	mpc_link_kmod_major	= 0,
	mpc_link_kmod_minor	= 9,
	mpc_link_kmod_rev	= 26,
};

#endif


