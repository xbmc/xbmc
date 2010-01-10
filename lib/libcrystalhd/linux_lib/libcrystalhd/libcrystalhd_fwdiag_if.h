/********************************************************************
 * Copyright(c) 2006-2009 Broadcom Corporation.
 *
 *  Name: libcrystalhd_fwdiag_if.h
 *
 *  Description: Driver Interface library Internal.
 *
 *  AU
 *
 *  HISTORY:
 *
 ********************************************************************
 *
 * This file is part of libcrystalhd.
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 2.1 of the License.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 *
 *******************************************************************/

#ifndef _libcrystalhd_FWDIAG_IF_
#define _libcrystalhd_FWDIAG_IF_

#define BC_HOST_CMD_ADDR			0x00000100

/* Host-Bootloader Communication Message Block */
typedef struct _BC_HOST_CMD_BLOCK_ST {
	uint32_t	done;
	uint32_t	cmd;
	uint32_t	start;
	uint32_t	size;
	uint32_t	cmdargs[3];
	uint32_t	chk_sum;

} BC_HOST_CMD_BLOCK_ST, *PBC_HOST_CMD_BLOCK_ST;

#define BC_FWDIAG_DONE				0x00000001

/* Bootloader Status */
typedef enum _BC_FWDIAG_STATUS{
	BC_FWDIAG_STS_SUCCESS	= 0,
	BC_FWDIAG_BOOTUP_DONE	= 1,
	BC_FWDIAG_TEST_PASS	= 2,
	BC_FWDIAG_TEST_FAIL	= 3,
	BC_FWDIAG_TEST_NOT_IMPL	= 4,
	BC_FWDIAG_INVALID_ARGS	= 5,

	/* Must be the last one.*/
	BC_FWDIAG_STS_ERROR	= -1

}BC_FWDIAG_STATUS;

#define BC_FWDIAG_RES_ADDR			0x00000140

/* Bootloader-Host Communication Message Block */
typedef struct _BC_FWDIAG_RES_BLOCK_ST {
	uint32_t	done;
	uint32_t	status;
	uint32_t	detail[5];
	uint32_t	chk_sum;

}BC_FWDIAG_RES_BLOCK_ST, *PBC_FWDIAG_RES_BLOCK_ST;

#define BC_HOST_CMD_POSTED			0x00000001
#define BC_FWDIAG_RES_POSTED			0x00000001
#define BC_FWDIAG_PATTERN_ADDR			0x00000200

/* Bootloader Status */
typedef enum _BC_FWDIAG_CMDS{
	BC_FWDIAG_SHORT_MEM_TEST	= 0x1,
	BC_FWDIAG_LONG_MEM_TEST		= 0x2,
	BC_FWDIAG_MEM_READ_TEST		= 0x3,
	BC_FWDIAG_MEM_WRITE_TEST	= 0x4,
	BC_FWDIAG_DMA_READ_TEST		= 0x8,
	BC_FWDIAG_DMA_WRITE_TEST	= 0xC,

}BC_FWDIAG_CMDS;


extern DRVIFLIB_INT_API BC_STATUS
DtsPushFwBinToLink(HANDLE hDevice, char *FwBinFile, uint32_t *bytesDnld);

DRVIFLIB_INT_API BC_STATUS
DtsDownloadFWDIAGToLINK(HANDLE hDevice, char *FwBinFile);

DRVIFLIB_INT_API BC_STATUS
DtsSendFWDiagCmd(HANDLE hDevice, BC_HOST_CMD_BLOCK_ST hostMsg);

DRVIFLIB_INT_API BC_STATUS
DtsReceiveFWDiagRes(HANDLE hDevice, PBC_FWDIAG_RES_BLOCK_ST pBlMsg, uint32_t wait);

DRVIFLIB_INT_API BC_STATUS
DtsClearFWDiagCommBlock(HANDLE hDevice);


#endif
