/********************************************************************
 * Copyright(c) 2006 Broadcom Corporation, all rights reserved.
 * Contains proprietary and confidential information.
 *
 * This source file is the property of Broadcom Corporation, and
 * may not be copied or distributed in any isomorphic form without
 * the prior written consent of Broadcom Corporation.
 *
 *
 *  Name: bc_drv_int.h
 *
 *  Description: Driver Internal functions.
 *
 *  AU
 *
 *  HISTORY:
 *
 *******************************************************************/
#ifndef _BCM_DRV_INT_H_
#define _BCM_DRV_INT_H_


#ifndef __LINUX_USER__
#include <windows.h>
#include "bc_dts_glob.h"
#else
#include "bc_dts_glob_lnx.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif
	// DLL Export

#define BSVS_UART_DEC_NONE  0x00
#define BSVS_UART_DEC_OUTER 0x01
#define BSVS_UART_DEC_INNER 0x02
#define BSVS_UART_STREAM    0x03

#define STREAM_VERSION_ADDR	0x001c5f00

typedef U32	BC_DTS_CFG;

BC_STATUS 
DtsFwDnldNotifyStart(HANDLE	hDevice);

BC_STATUS 
DtsFwDnldNotifyDone(HANDLE	hDevice);

DRVIFLIB_INT_API BC_STATUS 
DtsGetHwType(
    HANDLE  hDevice,
    U32     *DeviceID,
    U32     *VendorID,
    U32     *HWRev
    );

DRVIFLIB_INT_API VOID 
DtsHwReset(
    HANDLE hDevice
    );

DRVIFLIB_INT_API BC_STATUS
DtsSetLinkIn422Mode(HANDLE hDevice);

DRVIFLIB_INT_API VOID 
DtsSoftReset(
    HANDLE hDevice
    );

DRVIFLIB_INT_API BC_STATUS 
DtsGetConfig(
    HANDLE hDevice,
	BC_DTS_CFG *cfg
    );

DRVIFLIB_INT_API BC_STATUS 
DtsSetConfig(
    HANDLE hDevice,
	BC_DTS_CFG *cfg
    );

DRVIFLIB_INT_API BC_STATUS 
DtsSetCoreClock(
    HANDLE hDevice,
	U32		freq
    );

DRVIFLIB_INT_API BC_STATUS
DtsSetTSMode(
	HANDLE hDevice,
	U32		resrv1
	);

DRVIFLIB_INT_API BC_STATUS
DtsSetProgressive(
	HANDLE hDevice,
	U32		resrv1
	);

BC_STATUS 
DtsRstVidClkDLL(
	HANDLE hDevice
	);

DRVIFLIB_INT_API BC_STATUS 
DtsSetVideoClock(
    HANDLE hDevice,
	U32		freq
    );

DRVIFLIB_INT_API BOOL 
DtsIsVideoClockSet(HANDLE hDevice);

/* Deprecated, functionality moved to driver */
DRVIFLIB_INT_API VOID		
DtsSetUart(
    HANDLE hDevice,
	U32		uarta,
	U32		uartb
    );

DRVIFLIB_INT_API BC_STATUS 
DtsInitMemory(
    HANDLE      hDevice,
    PVOID	*pMemoryInfo
    );

DRVIFLIB_INT_API BC_STATUS 
DtsGetPciConfigSpace(
    HANDLE 	    hDevice,
    U8          *info
    );

DRVIFLIB_INT_API BC_STATUS 
DtsReadPciConfigSpace(
    HANDLE		hDevice,
    U32			offset,
    U32			*Value,
	U32			Size
    );

DRVIFLIB_INT_API BC_STATUS 
DtsWritePciConfigSpace(
    HANDLE		hDevice,
    U32			Offset,
    U32			Value,
	U32			Size
    );

DRVIFLIB_INT_API BC_STATUS 
DtsDevRegisterRead(
    HANDLE	hDevice,
    U32			offset,
    U32			*Value
    );

DRVIFLIB_INT_API BC_STATUS 
DtsDevRegisterWr(
    HANDLE	hDevice,
    U32		offset,
    U32		Value
    );

DRVIFLIB_INT_API BC_STATUS 
DtsFPGARegisterRead(
    HANDLE		hDevice,
    U32			offset,
    U32			*Value
    );

DRVIFLIB_INT_API BC_STATUS 
DtsFPGARegisterWr(
    HANDLE	hDevice,
    U32		offset,
    U32		Value
    );

DRVIFLIB_INT_API BC_STATUS 
DtsDevMemRd(
    HANDLE	hDevice,
    U32		*Buffer,
    U32		BuffSz,
    U32		Offset
    );

DRVIFLIB_INT_API BC_STATUS 
DtsDevMemWr(
    HANDLE	hDevice,
    U32		*Buffer,
    U32		BuffSz,
    U32		Offset
    );

DRVIFLIB_INT_API BC_STATUS 
DtsInitDRAM(HANDLE	hDevice, 
			U32 patt, 
			U32 off, 
			U32 dwsize);

DRVIFLIB_INT_API BC_STATUS 
DtsDownloadFW(HANDLE	hDevice,
			  char *StreamFName,
			  char *VDecOuter,
			  char *VDecInner);


DRVIFLIB_INT_API BC_STATUS 
DtsTxDmaText( HANDLE  hDevice ,
				 U8 *pUserData,
				 U32 ulSizeInBytes,
				 U32 *dramOff,
				 U8 Encrypted);

DRVIFLIB_INT_API BC_STATUS 
DtsGetDrvStat(
    HANDLE		hDevice,
	BC_DTS_STATS *pDrvStat
    );

DRVIFLIB_INT_API BC_STATUS 
DtsRstDrvStat(
    HANDLE		hDevice
    );

DRVIFLIB_INT_API BC_STATUS 
DtsEpromRead(
    HANDLE	hDevice,
    U8		*Buffer,
    U32		BuffSz,
    U32		Offset
    );

DRVIFLIB_INT_API BC_STATUS 
DtsEpromWrite(
    HANDLE	hDevice,
    U8		*Buffer,
    U32		BuffSz,
    U32		Offset
    );

DRVIFLIB_INT_API BC_STATUS 
DtsAesEpromWrite(
    HANDLE	hDevice,
    U8		*Buffer,
    U32		BuffSz,
    U32		Offset
    );

DRVIFLIB_INT_API BC_STATUS 
DtsGetFWFiles(
	HANDLE hDevice,
	char *StreamFName,
	char *VDecOuter,
	char *VDecInner
	);

DRVIFLIB_INT_API BC_STATUS 
DtsDownloadFWBin(
	HANDLE	hDevice,
	U8 *binBuff,
	U32 buffsize,
	U32 sig
	);

DRVIFLIB_INT_API BC_STATUS 
DtsCancelProcOutput(
    HANDLE  hDevice,
	PVOID	Context);

DRVIFLIB_INT_API BC_STATUS 
DtsChkYUVSizes(
	struct _DTS_LIB_CONTEXT	*Ctx,
	BC_DTS_PROC_OUT *Vout, 
	BC_DTS_PROC_OUT *Vin);

BC_STATUS
DtsCopyRawDataToOutBuff(struct _DTS_LIB_CONTEXT	*Ctx, 
						BC_DTS_PROC_OUT *Vout, 
						BC_DTS_PROC_OUT *Vin);

BC_STATUS DtsCopyNV12ToYV12(
	struct _DTS_LIB_CONTEXT	*Ctx,
	BC_DTS_PROC_OUT *Vout, 
	BC_DTS_PROC_OUT *Vin);

BC_STATUS DtsCopyNV12(
	struct _DTS_LIB_CONTEXT	*Ctx,
	BC_DTS_PROC_OUT *Vout, 
	BC_DTS_PROC_OUT *Vin);


/*================ Debug/Test Routines ===================*/

DRVIFLIB_INT_API
void DumpDataToFile(FILE *fp, 
					char *header, 
					U32 off, 
					U8 *buff, 
					U32 dwcount);

void DumpInputSampleToFile(U8 *buff, U32 buffsize);

#ifdef __cplusplus
}
#endif

#endif
