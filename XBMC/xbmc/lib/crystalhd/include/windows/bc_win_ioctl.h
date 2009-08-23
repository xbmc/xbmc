/********************************************************************
 * Copyright(c) 2006 Broadcom Corporation, all rights reserved.
 * Contains proprietary and confidential information.
 *
 * This source file is the property of Broadcom Corporation, and
 * may not be copied or distributed in any isomorphic form without
 * the prior written consent of Broadcom Corporation.
 *
 *
 *  Name: bc_win_ioctl.h
 *
 *  Description: Windows IOCTL control code definitions
 *              
 *
 *  AU
 *
 *  HISTORY:
 *
 *******************************************************************/

#include "bc_dts_glob.h"


#ifdef __cplusplus
extern "C" {
#endif
    /**********************************************************
     * Note: Codes 0-2047 (0-7FFh) are reserved by Microsoft
     *       Coded 2048-4095 (800h-FFFh) are reserved for OEMs
     *********************************************************/

#define BCM_IOCTL_CODE_BASE      0x800

#define BC_IOCTL_MB		METHOD_BUFFERED
#define BC_IOCTL_MN		METHOD_NEITHER

#ifdef _WIN64
#define CLIENT_64BIT   0x800
#define IOCTL_CODE( _code, _meth )		CTL_CODE(                	\
                                         	FILE_DEVICE_UNKNOWN, 	\
                                         	(CLIENT_64BIT  | _code),\
                                         	_meth,     				\
                                         	FILE_ANY_ACCESS      	\
                                         	)
//
//	In 64bit environment, a seperate set of Ioctls is defined for 32bit callers
//
#define IOCTL_CODE_32( _code, _meth )	CTL_CODE(                	\
                                         	FILE_DEVICE_UNKNOWN, 	\
                                         	_code,                	\
                                         	_meth,     				\
                                         	FILE_ANY_ACCESS      	\
                                         	)

//
// Macro to check for the "64bit" 
//
#define IS_64BIT(_code)		(_code & (1<<14))

#else

#define IOCTL_CODE( _code, _meth )     CTL_CODE(                	\
                                         	FILE_DEVICE_UNKNOWN, 	\
                                         	_code,                	\
                                         	_meth,     				\
                                         	FILE_ANY_ACCESS      	\
                                         	)
#endif

/* IOCTLs with method buffered */
#define	BC_IOCTL_GET_VERSION		IOCTL_CODE(DRV_CMD_VERSION, BC_IOCTL_MB) 
#define	BC_IOCTL_GET_HWTYPE			IOCTL_CODE(DRV_CMD_GET_HWTYPE, BC_IOCTL_MB) 
#define	BC_IOCTL_REG_RD				IOCTL_CODE(DRV_CMD_REG_RD, BC_IOCTL_MB) 
#define	BC_IOCTL_REG_WR				IOCTL_CODE(DRV_CMD_REG_WR, BC_IOCTL_MB) 
#define	BC_IOCTL_DRAM_INIT			IOCTL_CODE(DRV_CMD_DRAM_INIT, BC_IOCTL_MB)
#define	BC_IOCTL_MEM_RD				IOCTL_CODE(DRV_CMD_MEM_RD, BC_IOCTL_MB) 
#define	BC_IOCTL_MEM_WR				IOCTL_CODE(DRV_CMD_MEM_WR, BC_IOCTL_MB) 
#define BC_IOCTL_FPGA_RD			IOCTL_CODE(DRV_CMD_FPGA_RD, BC_IOCTL_MB)
#define BC_IOCTL_FPGA_WR			IOCTL_CODE(DRV_CMD_FPGA_WR, BC_IOCTL_MB)
#define	BC_IOCTL_MEM_INIT			IOCTL_CODE(DRV_CMD_MEM_INIT, BC_IOCTL_MB) 
#define	BC_IOCTL_RD_PCI_CFG			IOCTL_CODE(DRV_CMD_RD_PCI_CFG, BC_IOCTL_MB) 
#define	BC_IOCTL_WR_PCI_CFG			IOCTL_CODE(DRV_CMD_WR_PCI_CFG, BC_IOCTL_MB) 
#define BC_FW_DNLD_NOTIFY_START		IOCTL_CODE(DRV_CMD_NOTIFY_FW_DNLD_ST,BC_IOCTL_MB)
#define BC_FW_DNLD_NOTIFY_DONE		IOCTL_CODE(DRV_CMD_NOTIFY_FW_DNLD_DONE,BC_IOCTL_MB)
#define	BC_IOCTL_EPROM_RD			IOCTL_CODE(DRV_CMD_EPROM_RD, BC_IOCTL_MB) 
#define	BC_IOCTL_EPROM_WR			IOCTL_CODE(DRV_CMD_EPROM_WR, BC_IOCTL_MB) 
#define	BC_IOCTL_INIT_HW			IOCTL_CODE(DRV_CMD_INIT_HW, BC_IOCTL_MB) 
#define BC_IOCTL_PROC_INPUT			IOCTL_CODE(DRV_CMD_PROC_INPUT, BC_IOCTL_MB)
#define BC_IOCTL_ADD_RXBUFFS		IOCTL_CODE(DRV_CMD_ADD_RXBUFFS, BC_IOCTL_MB)
#define BC_IOCTL_FETCH_RXBUFF		IOCTL_CODE(DRV_CMD_FETCH_RXBUFF, BC_IOCTL_MB)
#define BC_IOCTL_REL_RXBUFFS		IOCTL_CODE(DRV_CMD_REL_RXBUFFS, BC_IOCTL_MB)
#define	BC_IOCTL_FW_CMD				IOCTL_CODE(DRV_ISSUE_FW_CMD, BC_IOCTL_MB) 
#define	BC_IOCTL_START_RX_CAP		IOCTL_CODE(DRV_CMD_START_RX_CAP, BC_IOCTL_MB) 
#define BC_IOCTL_FLUSH_RX_CAP		IOCTL_CODE(DRV_CMD_FLUSH_RX_CAP, BC_IOCTL_MB) 
#define BC_IOCTL_GET_DRV_STAT		IOCTL_CODE(DRV_CMD_GET_DRV_STAT, BC_IOCTL_MB) 
#define BC_IOCTL_RST_DRV_STAT		IOCTL_CODE(DRV_CMD_RST_DRV_STAT, BC_IOCTL_MB) 
#define BC_IOCTL_NOTIFY_MODE		IOCTL_CODE(DRV_CMD_NOTIFY_MODE, BC_IOCTL_MB)
#define BC_IOCTL_CANCEL_IO			IOCTL_CODE(DRV_CMD_CANCEL_IO, BC_IOCTL_MB) 
#define	BC_IOCTL_FW_DOWNLOAD		IOCTL_CODE(DRV_CMD_FW_DOWNLOAD, BC_IOCTL_MB) 

//
//	In 64bit environment, a seperate set of Ioctls is defined for 32bit callers
//	For fixed precision ioctls, the same ioctl code can be reused
//	For pointer precision ioctls, a new ioctl code need to be defined
//
#ifdef _WIN64
#define BC_IOCTL_PROC_INPUT_32		IOCTL_CODE_32(DRV_CMD_PROC_INPUT, BC_IOCTL_MB)
#define BC_IOCTL_ADD_RXBUFFS_32		IOCTL_CODE_32(DRV_CMD_ADD_RXBUFFS, BC_IOCTL_MB)
#define BC_IOCTL_FETCH_RXBUFF_32	IOCTL_CODE_32(DRV_CMD_FETCH_RXBUFF, BC_IOCTL_MB)

//
//	The following Ioctls don't change for 64bit
//
#define	BC_IOCTL_GET_VERSION_32		IOCTL_CODE_32(DRV_CMD_VERSION, BC_IOCTL_MB) 
#define	BC_IOCTL_GET_HWTYPE_32		IOCTL_CODE_32(DRV_CMD_GET_HWTYPE, BC_IOCTL_MB) 
#define	BC_IOCTL_REG_RD_32			IOCTL_CODE_32(DRV_CMD_REG_RD, BC_IOCTL_MB) 
#define	BC_IOCTL_REG_WR_32			IOCTL_CODE_32(DRV_CMD_REG_WR, BC_IOCTL_MB) 
#define	BC_IOCTL_DRAM_INIT_32		IOCTL_CODE_32(DRV_CMD_DRAM_INIT, BC_IOCTL_MB)
#define	BC_IOCTL_MEM_RD_32			IOCTL_CODE_32(DRV_CMD_MEM_RD, BC_IOCTL_MB) 
#define	BC_IOCTL_MEM_WR_32			IOCTL_CODE_32(DRV_CMD_MEM_WR, BC_IOCTL_MB) 
#define BC_IOCTL_FPGA_RD_32			IOCTL_CODE_32(DRV_CMD_FPGA_RD, BC_IOCTL_MB)
#define BC_IOCTL_FPGA_WR_32			IOCTL_CODE_32(DRV_CMD_FPGA_WR, BC_IOCTL_MB)
#define	BC_IOCTL_MEM_INIT_32		IOCTL_CODE_32(DRV_CMD_MEM_INIT, BC_IOCTL_MB) 
#define	BC_IOCTL_RD_PCI_CFG_32		IOCTL_CODE_32(DRV_CMD_RD_PCI_CFG, BC_IOCTL_MB) 
#define	BC_IOCTL_WR_PCI_CFG_32		IOCTL_CODE_32(DRV_CMD_WR_PCI_CFG, BC_IOCTL_MB) 
#define BC_FW_DNLD_NOTIFY_START_32	IOCTL_CODE_32(DRV_CMD_NOTIFY_FW_DNLD_ST,BC_IOCTL_MB)
#define BC_FW_DNLD_NOTIFY_DONE_32	IOCTL_CODE_32(DRV_CMD_NOTIFY_FW_DNLD_DONE,BC_IOCTL_MB)
#define	BC_IOCTL_EPROM_RD_32		IOCTL_CODE_32(DRV_CMD_EPROM_RD, BC_IOCTL_MB) 
#define	BC_IOCTL_EPROM_WR_32		IOCTL_CODE_32(DRV_CMD_EPROM_WR, BC_IOCTL_MB) 
#define	BC_IOCTL_INIT_HW_32			IOCTL_CODE_32(DRV_CMD_INIT_HW, BC_IOCTL_MB) 
#define BC_IOCTL_REL_RXBUFFS_32		IOCTL_CODE_32(DRV_CMD_REL_RXBUFFS, BC_IOCTL_MB)
#define	BC_IOCTL_FW_CMD_32			IOCTL_CODE_32(DRV_ISSUE_FW_CMD, BC_IOCTL_MB) 
#define	BC_IOCTL_START_RX_CAP_32	IOCTL_CODE_32(DRV_CMD_START_RX_CAP, BC_IOCTL_MB) 
#define BC_IOCTL_FLUSH_RX_CAP_32	IOCTL_CODE_32(DRV_CMD_FLUSH_RX_CAP, BC_IOCTL_MB) 
#define BC_IOCTL_GET_DRV_STAT_32	IOCTL_CODE_32(DRV_CMD_GET_DRV_STAT, BC_IOCTL_MB) 
#define BC_IOCTL_RST_DRV_STAT_32	IOCTL_CODE_32(DRV_CMD_RST_DRV_STAT, BC_IOCTL_MB) 
#define BC_IOCTL_NOTIFY_MODE_32		IOCTL_CODE_32(DRV_CMD_NOTIFY_MODE, BC_IOCTL_MB)
#define BC_IOCTL_CANCEL_IO_32		IOCTL_CODE(DRV_CMD_CANCEL_IO, BC_IOCTL_MB) 
#define	BC_IOCTL_FW_DOWNLOAD_32		IOCTL_CODE_32(DRV_CMD_FW_DOWNLOAD, BC_IOCTL_MB) 

#endif

#ifdef __cplusplus
}
#endif