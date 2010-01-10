/********************************************************************
 * Copyright(c) 2006-2009 Broadcom Corporation.
 *
 *  Name: libcrystalhd_int_if.h
 *
 *  Description: Driver Internal functions.
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

#define STREAM_VERSION_ADDR	0x001c5f00

typedef uint32_t	BC_DTS_CFG;

DRVIFLIB_INT_API BC_STATUS
DtsGetHwType(
    HANDLE   hDevice,
    uint32_t *DeviceID,
    uint32_t *VendorID,
    uint32_t *HWRev
    );

DRVIFLIB_INT_API BC_STATUS
DtsSetLinkIn422Mode(HANDLE hDevice);

DRVIFLIB_INT_API BC_STATUS
DtsGetConfig(
    HANDLE hDevice,
    BC_DTS_CFG *cfg
    );

DRVIFLIB_INT_API BC_STATUS
DtsSetCoreClock(
    HANDLE   hDevice,
    uint32_t freq
    );

DRVIFLIB_INT_API BC_STATUS
DtsSetTSMode(
    HANDLE   hDevice,
    uint32_t resrv1
    );

DRVIFLIB_INT_API BC_STATUS
DtsSetProgressive(
    HANDLE   hDevice,
    uint32_t resrv1
	);

BC_STATUS
DtsRstVidClkDLL(
    HANDLE hDevice
	);

DRVIFLIB_INT_API BC_STATUS
DtsSetVideoClock(
    HANDLE   hDevice,
    uint32_t freq
    );

DRVIFLIB_INT_API BC_STATUS
DtsGetPciConfigSpace(
    HANDLE  hDevice,
    uint8_t *info
    );

DRVIFLIB_INT_API BC_STATUS
DtsReadPciConfigSpace(
    HANDLE   hDevice,
    uint32_t offset,
    uint32_t *Value,
    uint32_t Size
    );

DRVIFLIB_INT_API BC_STATUS
DtsWritePciConfigSpace(
    HANDLE   hDevice,
    uint32_t Offset,
    uint32_t Value,
    uint32_t Size
    );

DRVIFLIB_INT_API BC_STATUS
DtsDevRegisterRead(
    HANDLE   hDevice,
    uint32_t offset,
    uint32_t *Value
    );

DRVIFLIB_INT_API BC_STATUS
DtsDevRegisterWr(
    HANDLE   hDevice,
    uint32_t offset,
    uint32_t Value
    );

DRVIFLIB_INT_API BC_STATUS
DtsFPGARegisterRead(
    HANDLE   hDevice,
    uint32_t offset,
    uint32_t *Value
    );

DRVIFLIB_INT_API BC_STATUS
DtsFPGARegisterWr(
    HANDLE   hDevice,
    uint32_t offset,
    uint32_t Value
    );

DRVIFLIB_INT_API BC_STATUS
DtsDevMemRd(
    HANDLE   hDevice,
    uint32_t *Buffer,
    uint32_t BuffSz,
    uint32_t Offset
    );

DRVIFLIB_INT_API BC_STATUS
DtsDevMemWr(
    HANDLE   hDevice,
    uint32_t *Buffer,
    uint32_t BuffSz,
    uint32_t Offset
    );

DRVIFLIB_INT_API BC_STATUS
DtsDownloadFW(
    HANDLE hDevice,
    char   *StreamFName,
    char   *VDecOuter,
    char   *VDecInner
    );


DRVIFLIB_INT_API BC_STATUS
DtsTxDmaText(
    HANDLE   hDevice ,
    uint8_t  *pUserData,
    uint32_t ulSizeInBytes,
    uint32_t *dramOff,
    uint8_t  Encrypted
    );

DRVIFLIB_INT_API BC_STATUS
DtsGetDrvStat(
    HANDLE hDevice,
    BC_DTS_STATS *pDrvStat
    );

DRVIFLIB_INT_API BC_STATUS
DtsRstDrvStat(
    HANDLE hDevice
    );

DRVIFLIB_INT_API BC_STATUS
DtsGetFWFiles(
    HANDLE hDevice,
    char   *StreamFName,
    char   *VDecOuter,
    char   *VDecInner
    );

DRVIFLIB_INT_API BC_STATUS
DtsDownloadFWBin(
    HANDLE   hDevice,
    uint8_t  *binBuff,
    uint32_t buffsize,
    uint32_t sig
    );

DRVIFLIB_INT_API BC_STATUS
DtsCancelProcOutput(
    HANDLE hDevice,
    PVOID  Context
    );

DRVIFLIB_INT_API BC_STATUS
DtsChkYUVSizes(
    struct _DTS_LIB_CONTEXT *Ctx,
    BC_DTS_PROC_OUT *Vout,
    BC_DTS_PROC_OUT *Vin
    );

BC_STATUS
DtsCopyRawDataToOutBuff(
    struct _DTS_LIB_CONTEXT *Ctx,
    BC_DTS_PROC_OUT *Vout,
    BC_DTS_PROC_OUT *Vin
    );

BC_STATUS DtsCopyNV12ToYV12(
    struct _DTS_LIB_CONTEXT *Ctx,
    BC_DTS_PROC_OUT *Vout,
    BC_DTS_PROC_OUT *Vin
    );

BC_STATUS DtsCopyNV12(
    struct _DTS_LIB_CONTEXT *Ctx,
    BC_DTS_PROC_OUT *Vout,
    BC_DTS_PROC_OUT *Vin
    );


DRVIFLIB_INT_API BC_STATUS
DtsPushFwBinToLink(HANDLE hDevice, uint32_t *Buffer, uint32_t BuffSz);

DRVIFLIB_INT_API BC_STATUS
fwbinPushToLINK(HANDLE hDevice, char *FwBinFile, uint32_t *bytesDnld);


DRVIFLIB_INT_API BC_STATUS
DtsPushAuthFwToLink(HANDLE hDevice, char *FwBinFile);

DRVIFLIB_INT_API BC_STATUS dec_write_fw_Sig(HANDLE hndl,uint32_t* Sig);

/*================ Debug/Test Routines ===================*/

DRVIFLIB_INT_API
void DumpDataToFile(
    FILE     *fp,
    char     *header,
    uint32_t off,
    uint8_t  *buff,
    uint32_t dwcount
    );

void DumpInputSampleToFile(uint8_t *buff, uint32_t buffsize);

#ifdef __cplusplus
}
#endif

#endif
