/********************************************************************
 * Copyright(c) 2006 Broadcom Corporation, all rights reserved.
 * Contains proprietary and confidential information.
 *
 * This source file is the property of Broadcom Corporation, and
 * may not be copied or distributed in any isomorphic form without
 * the prior written consent of Broadcom Corporation.
 *
 *
 *  Name: DtsIfPriv.h
 *
 *  Description: Driver Interface library Interanl.
 *
 *  AU
 *
 *  HISTORY:
 *
 *******************************************************************/

#ifndef _BC_FWDIAG_IF_
#define _BC_FWDIAG_IF_
#include "bc_dts_types.h"
//#include "bc_dts_defs.h"


#define BC_HOST_CMD_ADDR					0x00000100

/* Host-Bootloader Communication Message Block */
typedef struct _BC_HOST_CMD_BLOCK_ST {

	U32	done;
	U32	cmd;
	U32	start;
	U32	size;
	U32	cmdargs[3];
	U32	chk_sum;

} BC_HOST_CMD_BLOCK_ST, *PBC_HOST_CMD_BLOCK_ST;

#define BC_FWDIAG_DONE						0x00000001

/* Bootloader Status */
typedef enum _BC_FWDIAG_STATUS{
	BC_FWDIAG_STS_SUCCESS      = 0,
	BC_FWDIAG_BOOTUP_DONE		= 1,
	BC_FWDIAG_TEST_PASS     = 2,
	BC_FWDIAG_TEST_FAIL     = 3,
	BC_FWDIAG_TEST_NOT_IMPL = 4,
	BC_FWDIAG_INVALID_ARGS  = 5, 

	/* Must be the last one.*/
	BC_FWDIAG_STS_ERROR 		= -1
}BC_FWDIAG_STATUS;

#define BC_FWDIAG_RES_ADDR			0x00000140

/* Bootloader-Host Communication Message Block */
typedef struct _BC_FWDIAG_RES_BLOCK_ST {

	U32	done;
	U32	status;
	U32	detail[5];
	U32	chk_sum;

}BC_FWDIAG_RES_BLOCK_ST, *PBC_FWDIAG_RES_BLOCK_ST;

#define BC_HOST_CMD_POSTED	0x00000001
#define BC_FWDIAG_RES_POSTED	0x00000001
#define BC_FWDIAG_PATTERN_ADDR	0x00000200

/* Bootloader Status */
typedef enum _BC_FWDIAG_CMDS{
	BC_FWDIAG_SHORT_MEM_TEST      = 0x1,
	BC_FWDIAG_LONG_MEM_TEST		  = 0x2,
	BC_FWDIAG_MEM_READ_TEST		  = 0x3,
	BC_FWDIAG_MEM_WRITE_TEST      = 0x4, 
	BC_FWDIAG_DMA_READ_TEST       = 0x8,
	BC_FWDIAG_DMA_WRITE_TEST      = 0xC,
	
}BC_FWDIAG_CMDS;





#endif
