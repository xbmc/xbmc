/********************************************************************
 * Copyright(c) 2006-2009 Broadcom Corporation.
 *
 *  Name: libcrystalhd_int_if.cpp
 *
 *  Description: Driver Internal Interfaces
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

#include "libcrystalhd_int_if.h"
#include "libcrystalhd_fwcmds.h"
#include "libcrystalhd_priv.h"

#define SV_MAX_LINE_SZ 128


//===================================Externs ============================================
DRVIFLIB_INT_API BC_STATUS 
DtsGetHwType(
    HANDLE  hDevice,
    uint32_t     *DeviceID,
    uint32_t     *VendorID,
    uint32_t     *HWRev
    )
{
	BC_HW_TYPE *pHWInfo;
	BC_IOCTL_DATA *pIocData = NULL;
	DTS_LIB_CONTEXT		*Ctx = NULL;
	BC_STATUS	sts = BC_STS_SUCCESS;

	DTS_GET_CTX(hDevice,Ctx);

	if(!(pIocData = DtsAllocIoctlData(Ctx)))
		return BC_STS_INSUFF_RES;

	pHWInfo = 	&pIocData->u.hwType;

	pHWInfo->PciDevId = 0xffff;
	pHWInfo->PciVenId =  0xffff;
	pHWInfo->HwRev = 0xff;

	if( (sts=DtsDrvCmd(Ctx,BCM_IOC_GET_HWTYPE,0,pIocData,FALSE)) != BC_STS_SUCCESS){
		DtsRelIoctlData(Ctx,pIocData);
		DebugLog_Trace(LDIL_DBG,"DtsGetHwType: Ioctl failed: %d\n",sts);
		return sts;
	}

	*DeviceID	= 	pHWInfo->PciDevId;
	*VendorID	=	pHWInfo->PciVenId;
	*HWRev		=	pHWInfo->HwRev;

	DtsRelIoctlData(Ctx,pIocData);

	return BC_STS_SUCCESS;
}

DRVIFLIB_INT_API BC_STATUS
DtsSetLinkIn422Mode(HANDLE hDevice)
{
	uint32_t					Val = 0;
	DTS_LIB_CONTEXT		*Ctx;
	uint32_t					ModeSelect;

	DTS_GET_CTX(hDevice,Ctx);
	ModeSelect = Ctx->b422Mode;

	/* 
	 * EN_WRITE_ALL BIT -Bit 20
	 * This bit dictates that weather the data will be xferred in 
	 * 1 -  UYVY Mode.
	 * 0 - YUY2 Mode.
 	 */
	DtsFPGARegisterRead(hDevice,MISC2_GLOBAL_CTRL,&Val);

	if (ModeSelect == MODE420) {
        Val &= 0xffeeffff;
	} else {	
		Val |= BC_BIT(16);
		if(ModeSelect == MODE422_UYVY) {
			Val |= BC_BIT(20);
		} else {
			Val &= ~BC_BIT(20);
		}
	}

	DtsFPGARegisterWr(hDevice,MISC2_GLOBAL_CTRL,Val);
	return BC_STS_SUCCESS;
}

DRVIFLIB_INT_API BC_STATUS 
DtsGetConfig(
    HANDLE hDevice,
	BC_DTS_CFG *cfg
    )
{
	DTS_LIB_CONTEXT		*Ctx;

	DTS_GET_CTX(hDevice,Ctx);

	if(!cfg){
		return BC_STS_INV_ARG;
	}
	*cfg = Ctx->CfgFlags;

	return BC_STS_SUCCESS;
}



DRVIFLIB_INT_API BC_STATUS
DtsSetTSMode(
	HANDLE hDevice,
	uint32_t	resv1
	)
{
	uint32_t RegVal = 0;
	DTS_LIB_CONTEXT		*Ctx;
	BOOL TsMode = TRUE;

	DTS_GET_CTX(hDevice,Ctx);

	//if(Ctx->FixFlags & DTS_LOAD_FILE_PLAY_FW)
		TsMode = FALSE;

	if(TsMode){
		DtsFPGARegisterRead(hDevice,MISC2_GLOBAL_CTRL,&RegVal);
		RegVal &= 0xFFFFFFFE;		//Reset Bit 0
		DtsFPGARegisterWr(hDevice,MISC2_GLOBAL_CTRL,RegVal);
	}else{
		// Set the FPGA up in non TS mode
		DtsFPGARegisterRead(hDevice,MISC2_GLOBAL_CTRL,&RegVal);
		RegVal |= 0x01;		//Set Bit 0
		DtsFPGARegisterWr(hDevice,MISC2_GLOBAL_CTRL,RegVal);
	}

	return BC_STS_SUCCESS;
}

DRVIFLIB_INT_API BC_STATUS
DtsSetProgressive(
	HANDLE hDevice,
	uint32_t	resv1
	)
{
	uint32_t RegVal;
	// Set the FPGA up in Progressive mode - i.e. 1 vsync/frame
	DtsFPGARegisterRead(hDevice,MISC2_GLOBAL_CTRL,&RegVal);
	RegVal |= 0x10;		//Set Bit 4
	DtsFPGARegisterWr(hDevice,MISC2_GLOBAL_CTRL,RegVal);

	return BC_STS_SUCCESS;
}


DRVIFLIB_INT_API BC_STATUS 
DtsSetVideoClock(
    HANDLE hDevice,
	uint32_t		freq
    )
{
	
	DTS_LIB_CONTEXT		*Ctx;
	uint32_t	DevID,VendorID,Revision;

	DTS_GET_CTX(hDevice,Ctx);

	if(freq){
		DebugLog_Trace(LDIL_DBG,"DtsSetVideoClock: Custom pll settings not implemented yet.\n");
		return BC_STS_NOT_IMPL;
	}

	if(BC_STS_SUCCESS != DtsGetHwType(hDevice,&DevID,&VendorID,&Revision)) {
		DebugLog_Trace(LDIL_DBG,"Get Hardware Type Failed\n");
		return BC_STS_INV_ARG;
	}
	if(DevID == BC_PCI_DEVID_LINK) {
		// Don't set the video clock
		return BC_STS_SUCCESS;
	}

	return BC_STS_SUCCESS;
}


DRVIFLIB_INT_API BC_STATUS 
DtsGetPciConfigSpace(
    HANDLE 	hDevice,
    uint8_t          *info
    )
{
	BC_IOCTL_DATA *pIocData = NULL;
	BC_PCI_CFG *pciInfo;
	DTS_LIB_CONTEXT		*Ctx = NULL;
	BC_STATUS	sts = BC_STS_SUCCESS;


	DTS_GET_CTX(hDevice,Ctx);

	if(!info)
	{
		DebugLog_Trace(LDIL_DBG,"DtsGetPciConfigSpace: Invlid Arguments\n");
		return BC_STS_ERROR;
	}

	if(!(pIocData = DtsAllocIoctlData(Ctx)))
		return BC_STS_INSUFF_RES;

	pciInfo = (BC_PCI_CFG *)&pIocData->u.pciCfg;

	pciInfo->Size = PCI_CFG_SIZE;
	pciInfo->Offset = 0;
	memset(info,0,	PCI_CFG_SIZE);

	if( (sts=DtsDrvCmd(Ctx,BCM_IOC_RD_PCI_CFG,0,pIocData,FALSE)) != BC_STS_SUCCESS){
		DtsRelIoctlData(Ctx,pIocData);
		DebugLog_Trace(LDIL_DBG,"DtsGetPciConfigSpace: Ioctl failed: %d\n",sts);
		return sts;
	}

	memcpy(info,pciInfo->pci_cfg_space,PCI_CFG_SIZE);

	DtsRelIoctlData(Ctx,pIocData);

	return BC_STS_SUCCESS;
}

/****/

DRVIFLIB_INT_API BC_STATUS 
DtsReadPciConfigSpace(
    HANDLE		hDevice,
    uint32_t			Offset,
    uint32_t			*Value,
	uint32_t			Size
    )
{
	BC_IOCTL_DATA *pIocData = NULL;
	BC_PCI_CFG *pciInfo;
	DTS_LIB_CONTEXT		*Ctx = NULL;
	BC_STATUS	sts = BC_STS_SUCCESS;

	DTS_GET_CTX(hDevice,Ctx);

	if(!Value)
	{
		DebugLog_Trace(LDIL_DBG,"DtsGetPciConfigSpace: Invlid Arguments\n");
		return BC_STS_ERROR;
	}

	if(!(pIocData = DtsAllocIoctlData(Ctx)))
		return BC_STS_INSUFF_RES;

	pciInfo = (BC_PCI_CFG *)&pIocData->u.pciCfg;

	pciInfo->Size = Size;
	pciInfo->Offset = Offset;
	if( (sts=DtsDrvCmd(Ctx,BCM_IOC_RD_PCI_CFG,0,pIocData,FALSE)) != BC_STS_SUCCESS){
		DtsRelIoctlData(Ctx,pIocData);
		DebugLog_Trace(LDIL_DBG,"DtsGetPciConfigSpace: Ioctl failed: %d\n",sts);
		return sts;
	}

	*Value = *(uint32_t *)(pciInfo->pci_cfg_space);

	DtsRelIoctlData(Ctx,pIocData);

	return BC_STS_SUCCESS;
}

DRVIFLIB_INT_API BC_STATUS 
DtsWritePciConfigSpace(
    HANDLE		hDevice,
    uint32_t			Offset,
    uint32_t			Value,
	uint32_t			Size
    )
{
	BC_IOCTL_DATA *pIocData = NULL;
	BC_PCI_CFG *pciInfo;
	DTS_LIB_CONTEXT		*Ctx = NULL;
	BC_STATUS	sts = BC_STS_SUCCESS;

	DTS_GET_CTX(hDevice,Ctx);

	if(!(pIocData = DtsAllocIoctlData(Ctx)))
		return BC_STS_INSUFF_RES;

	pciInfo = (BC_PCI_CFG *)&pIocData->u.pciCfg;

	pciInfo->Size = Size;
	pciInfo->Offset = Offset;
	*((uint32_t *)(pciInfo->pci_cfg_space)) = Value;
	if( (sts=DtsDrvCmd(Ctx,BCM_IOC_WR_PCI_CFG,0,pIocData,FALSE)) != BC_STS_SUCCESS){
		DtsRelIoctlData(Ctx,pIocData);
		DebugLog_Trace(LDIL_DBG,"DtsGetPciConfigSpace: Ioctl failed: %d\n",sts);
		return sts;
	}

	DtsRelIoctlData(Ctx,pIocData);

	return BC_STS_SUCCESS;
}

/****/

DRVIFLIB_INT_API BC_STATUS 
DtsDevRegisterRead(
    HANDLE		hDevice,
    uint32_t			offset,
    uint32_t			*Value
    )
{
	BC_IOCTL_DATA *pIocData = NULL;	
	BC_CMD_REG_ACC	*reg_acc_read;
	DTS_LIB_CONTEXT		*Ctx = NULL;
	BC_STATUS	sts = BC_STS_SUCCESS;


	DTS_GET_CTX(hDevice,Ctx);
	
	if(!(pIocData = DtsAllocIoctlData(Ctx)))
		return BC_STS_INSUFF_RES;


	reg_acc_read = (BC_CMD_REG_ACC *) &pIocData->u.regAcc;

	//
	// Prepare the command here.
	//
	reg_acc_read->Offset	= offset;
	reg_acc_read->Value		= 0;

	if( (sts=DtsDrvCmd(Ctx,BCM_IOC_REG_RD,0,pIocData,FALSE)) != BC_STS_SUCCESS){
		DtsRelIoctlData(Ctx,pIocData);
		DebugLog_Trace(LDIL_DBG,"DtsDevRegisterRead: Ioctl failed: %d\n",sts);
		return sts;
	}

	*Value = reg_acc_read->Value;
	
	DtsRelIoctlData(Ctx,pIocData);

	return BC_STS_SUCCESS;
}


DRVIFLIB_INT_API BC_STATUS 
DtsDevRegisterWr(
    HANDLE	hDevice,
    uint32_t		offset,
    uint32_t		Value
    )
{
	BC_CMD_REG_ACC	*reg_acc_wr;
	BC_IOCTL_DATA *pIocData = NULL;	
	DTS_LIB_CONTEXT		*Ctx = NULL;
	BC_STATUS	sts = BC_STS_SUCCESS;


	DTS_GET_CTX(hDevice,Ctx);
	
	if(!(pIocData = DtsAllocIoctlData(Ctx)))
		return BC_STS_INSUFF_RES;

	reg_acc_wr = (BC_CMD_REG_ACC *) &pIocData->u.regAcc;

	//
	// Prepare the command here.
	//
	reg_acc_wr->Offset		= offset;
	reg_acc_wr->Value		= Value;

	if( (sts=DtsDrvCmd(Ctx,BCM_IOC_REG_WR,0,pIocData,FALSE)) != BC_STS_SUCCESS){
		DtsRelIoctlData(Ctx,pIocData);
		DebugLog_Trace(LDIL_DBG,"DtsDevRegisterWr: Ioctl failed: %d\n",sts);
		return sts;
	}

	DtsRelIoctlData(Ctx,pIocData);

	return BC_STS_SUCCESS;
}

DRVIFLIB_INT_API BC_STATUS 
DtsFPGARegisterRead(
    HANDLE		hDevice,
    uint32_t			offset,
    uint32_t			*Value
    )
{
	BC_IOCTL_DATA *pIocData = NULL;	
	BC_CMD_REG_ACC	*reg_acc_read;
	DTS_LIB_CONTEXT		*Ctx = NULL;
	BC_STATUS	sts = BC_STS_SUCCESS;


	DTS_GET_CTX(hDevice,Ctx);
	
	if(!(pIocData = DtsAllocIoctlData(Ctx)))
		return BC_STS_INSUFF_RES;


	reg_acc_read = (BC_CMD_REG_ACC *) &pIocData->u.regAcc;

	//
	// Prepare the command here.
	//
	reg_acc_read->Offset	= offset;
	reg_acc_read->Value		= 0;

	if( (sts=DtsDrvCmd(Ctx,BCM_IOC_FPGA_RD,0,pIocData,FALSE)) != BC_STS_SUCCESS){
		DtsRelIoctlData(Ctx,pIocData);
		DebugLog_Trace(LDIL_DBG,"DtsFPGARegisterRead: Ioctl failed: %d\n",sts);
		return sts;
	}

	*Value = reg_acc_read->Value;
	
	DtsRelIoctlData(Ctx,pIocData);

	return BC_STS_SUCCESS;
}

DRVIFLIB_INT_API BC_STATUS 
DtsFPGARegisterWr(
    HANDLE	hDevice,
    uint32_t		offset,
    uint32_t		Value
    )
{
	BC_CMD_REG_ACC	*reg_acc_wr;
	BC_IOCTL_DATA *pIocData = NULL;	
	DTS_LIB_CONTEXT		*Ctx = NULL;
	BC_STATUS	sts = BC_STS_SUCCESS;


	DTS_GET_CTX(hDevice,Ctx);
	
	if(!(pIocData = DtsAllocIoctlData(Ctx)))
		return BC_STS_INSUFF_RES;

	reg_acc_wr = (BC_CMD_REG_ACC *) &pIocData->u.regAcc;

	//
	// Prepare the command here.
	//
	reg_acc_wr->Offset		= offset;
	reg_acc_wr->Value		= Value;

	if( (sts=DtsDrvCmd(Ctx,BCM_IOC_FPGA_WR,0,pIocData,FALSE)) != BC_STS_SUCCESS){
		DtsRelIoctlData(Ctx,pIocData);
		DebugLog_Trace(LDIL_DBG,"DtsFPGARegisterWr: Ioctl failed: %d\n",sts);
		return sts;
	}

	DtsRelIoctlData(Ctx,pIocData);

	return BC_STS_SUCCESS;
}

DRVIFLIB_INT_API BC_STATUS 
DtsDevMemRd(
    HANDLE	hDevice,
    uint32_t		*Buffer,
    uint32_t		BuffSz,
    uint32_t		Offset
    )
{
	uint8_t					*pXferBuff;
	uint32_t					IOcCode,size_in_dword;
	BC_IOCTL_DATA		*pIoctlData;	
	BC_CMD_DEV_MEM		*pMemAccessRd;
	uint32_t					BytesReturned,AllocSz;
	
	if(!hDevice)
	{
		DebugLog_Trace(LDIL_DBG,"DtsDevMemRd: Invalid Handle\n");
		return BC_STS_INV_ARG;
	}

	if(!Buffer)
	{
		DebugLog_Trace(LDIL_DBG,"DtsDevMemRd: Null Buffer\n");
		return BC_STS_INV_ARG;
	}

	if(BuffSz % 4)
	{
		DebugLog_Trace(LDIL_DBG,"DtsDevMemRd: Buff Size is not a multiple of DWORD\n");
		return BC_STS_ERROR;
	}


	AllocSz = sizeof(BC_IOCTL_DATA) + (BuffSz);

	pIoctlData = (BC_IOCTL_DATA *) malloc(AllocSz);

	if(!pIoctlData)
	{
		DebugLog_Trace(LDIL_DBG,"DtsDevMemRd: Memory Allocation Failed\n");
		return BC_STS_ERROR;
	}

	pXferBuff = ( ((PUCHAR)pIoctlData) + sizeof(BC_IOCTL_DATA));

	pMemAccessRd =  &pIoctlData->u.devMem;
	size_in_dword = BuffSz / 4;
	pIoctlData->RetSts = BC_STS_ERROR;
	pIoctlData->IoctlDataSz = sizeof(BC_IOCTL_DATA);
	pMemAccessRd->StartOff = Offset;
	memset(pXferBuff,'a',BuffSz);
	/* The size is passed in Bytes*/
	pMemAccessRd->NumDwords = size_in_dword;
	IOcCode = BCM_IOC_MEM_RD;
	if(!DtsDrvIoctl(hDevice,
					BCM_IOC_MEM_RD,
					pIoctlData,
					AllocSz,
					pIoctlData,
					AllocSz,
					(LPDWORD)&BytesReturned,
					NULL))
	{
		DebugLog_Trace(LDIL_DBG,"DtsDevMemRd: DeviceIoControl Failed\n");
		return BC_STS_ERROR;
	}

	if(BC_STS_ERROR == pIoctlData->RetSts)
	{
		DebugLog_Trace(LDIL_DBG,"DtsDevMemRd: IOCTL Cmd Failed By Driver\n");
		return pIoctlData->RetSts;
	}
	
	memcpy(Buffer,pXferBuff,BuffSz);
	
	if(pIoctlData)
		free(pIoctlData);

	return BC_STS_SUCCESS;
}



DRVIFLIB_INT_API BC_STATUS 
DtsDevMemWr(
    HANDLE	hDevice,
    uint32_t		*Buffer,
    uint32_t		BuffSz,
    uint32_t		Offset
    )
{
	uint8_t					*pXferBuff;
	uint32_t					IOcCode,size_in_dword;
	BC_IOCTL_DATA		*pIoctlData;	
	BC_CMD_DEV_MEM		*pMemAccessRd;
	uint32_t					BytesReturned,AllocSz;
	
	//pIoctlData = (BC_IOCTL_DATA *)malloc(sizeof(BC_IOCTL_DATA));


	if(!hDevice)
	{
		DebugLog_Trace(LDIL_DBG,"DtsDevMemRd: Invalid Handle\n");
		return BC_STS_INV_ARG;
	}

	if(!Buffer)
	{
		DebugLog_Trace(LDIL_DBG,"DtsDevMemRd: Null Buffer\n");
		return BC_STS_INV_ARG;
	}

	if(BuffSz % 4)
	{
		DebugLog_Trace(LDIL_DBG,"DtsDevMemWr: Buff Size is not a multiple of DWORD\n");
		return BC_STS_ERROR;
	}


	AllocSz = sizeof(BC_IOCTL_DATA) + (BuffSz);

	pIoctlData = (BC_IOCTL_DATA *) malloc(AllocSz);

	if(!pIoctlData)
	{
		DebugLog_Trace(LDIL_DBG,"DtsDevMemRd: Memory Allocation Failed\n");
		return BC_STS_ERROR;
	}

	pXferBuff = ( ((PUCHAR)pIoctlData) + sizeof(BC_IOCTL_DATA));

	pMemAccessRd =  &pIoctlData->u.devMem;
	size_in_dword = BuffSz / 4;
	pIoctlData->RetSts = BC_STS_ERROR;
	pIoctlData->IoctlDataSz = sizeof(BC_IOCTL_DATA);
	pMemAccessRd->StartOff = Offset;
	memcpy(pXferBuff,Buffer,BuffSz);
	/* The size is passed in Bytes*/
	pMemAccessRd->NumDwords = size_in_dword;
	IOcCode = BCM_IOC_MEM_WR;
	if(!DtsDrvIoctl(hDevice,
					BCM_IOC_MEM_WR,
					pIoctlData,
					AllocSz,
					pIoctlData,
					AllocSz,
					(LPDWORD)&BytesReturned,
					NULL))
	{
		DebugLog_Trace(LDIL_DBG,"DtsDevMemRd: DeviceIoControl Failed\n");
		return BC_STS_ERROR;
	}

	if(BC_STS_ERROR == pIoctlData->RetSts)
	{
		DebugLog_Trace(LDIL_DBG,"DtsDevMemRd: IOCTL Cmd Failed By Driver\n");
		return pIoctlData->RetSts;
	}
	
	if(pIoctlData)
		free(pIoctlData);

	return BC_STS_SUCCESS;

}

DRVIFLIB_INT_API BC_STATUS 
DtsTxDmaText( HANDLE  hDevice ,
				 uint8_t *pUserData,
				 uint32_t ulSizeInBytes,
				 uint32_t *dramOff,
				 uint8_t Encrypted)
{
	BC_STATUS status = BC_STS_SUCCESS;
	uint32_t		ulDmaSz; 
	uint8_t		*pDmaBuff;
	
	DTS_LIB_CONTEXT		*Ctx = NULL;
	BC_IOCTL_DATA		*pIocData = NULL;

	DTS_GET_CTX(hDevice,Ctx);

	if( (!pUserData) || (!ulSizeInBytes) || !dramOff)
	{
		return BC_STS_INV_ARG;
	}

	pDmaBuff = pUserData;
	ulDmaSz = ulSizeInBytes;
	
	if(!(pIocData = DtsAllocIoctlData(Ctx)))
		return BC_STS_INSUFF_RES;

	pIocData->RetSts = BC_STS_ERROR;
	pIocData->IoctlDataSz = sizeof(BC_IOCTL_DATA);
	pIocData->u.ProcInput.DramOffset =0;
	pIocData->u.ProcInput.pDmaBuff = pDmaBuff;
	pIocData->u.ProcInput.BuffSz = ulDmaSz;
	pIocData->u.ProcInput.Mapped = FALSE;
	pIocData->u.ProcInput.Encrypted = Encrypted;


	status = DtsDrvCmd(Ctx,BCM_IOC_PROC_INPUT,1,pIocData,FALSE);

	*dramOff = pIocData->u.ProcInput.DramOffset;

	if( BC_STS_SUCCESS != status)
	{
		DebugLog_Trace(LDIL_DBG,"DtsDevMemRd: DeviceIoControl Failed\n");
	}

	DtsRelIoctlData(Ctx,pIocData);
	
	DumpInputSampleToFile(pUserData,ulSizeInBytes);
	
	return status;
}

DRVIFLIB_INT_API BC_STATUS 
DtsCancelProcOutput(
    HANDLE  hDevice,
	PVOID	Context)
{
	DTS_LIB_CONTEXT		*Ctx = NULL;
	
	DTS_GET_CTX(hDevice,Ctx);

	return DtsCancelFetchOutInt(Ctx);
}


//------------------------------------------------------------------------
// Name: DtsChkYUVSizes
// Description: Check Src/Dst buffer sizes with configured resolution.
//
// Vin:  Strtucture received from HW
// Vout: Structure got from App (Where data need to be copied)
//
//------------------------------------------------------------------------
DRVIFLIB_INT_API BC_STATUS DtsChkYUVSizes(
	DTS_LIB_CONTEXT *Ctx, 
	BC_DTS_PROC_OUT *Vout, 
	BC_DTS_PROC_OUT *Vin)
{
	if (!Vout || !Vout->Ybuff || !Vin || !Vin->Ybuff){
		return BC_STS_INV_ARG;
	}
	if((!Ctx->b422Mode) && (!Vout->UVbuff || !Vin->UVbuff)){
		return BC_STS_INV_ARG;
	}

	/* Pass on size info irrespective of the status (for debug) */
	Vout->YBuffDoneSz = Vin->YBuffDoneSz;
	Vout->UVBuffDoneSz = Vin->UVBuffDoneSz;

	/* Does Driver qualifies this condition before setting _PIB_VALID flag??..*/
	if( !( Vin->YBuffDoneSz) || ((!Ctx->b422Mode) && (!Vin->UVBuffDoneSz)) ){
		DebugLog_Trace(LDIL_DBG,"DtsChkYUVSizes: Incomplete Transfer\n");
		return BC_STS_IO_XFR_ERROR;
	}
/*
	Let the upper layer take care of this
	if(!(Vin->PoutFlags & BC_POUT_FLAGS_PIB_VALID)){
		DebugLog_Trace(LDIL_DBG,"DtsChkYUVSizes: PIB not Valid\n");
		return BC_STS_IO_XFR_ERROR;
	}*/

	return BC_STS_SUCCESS;
}

/***/

DRVIFLIB_INT_API BC_STATUS 
DtsGetDrvStat(
    HANDLE		hDevice,
	BC_DTS_STATS *pDrvStat
    )
{
	BC_IOCTL_DATA *pIocData = NULL;
	BC_DTS_STATS *pIntDrvStat;
	DTS_LIB_CONTEXT		*Ctx = NULL;
	BC_STATUS	sts = BC_STS_SUCCESS;

	DTS_GET_CTX(hDevice,Ctx);

	if(!pDrvStat)
	{
		DebugLog_Trace(LDIL_DBG,"DtsGetDrvStat: Invlid Arguments\n");
		return BC_STS_ERROR;
	}

	if(!(pIocData = DtsAllocIoctlData(Ctx)))
		return BC_STS_INSUFF_RES;

	if( (sts=DtsDrvCmd(Ctx,BCM_IOC_GET_DRV_STAT,0,pIocData,FALSE)) != BC_STS_SUCCESS){
		DtsRelIoctlData(Ctx,pIocData);
		DebugLog_Trace(LDIL_DBG,"DtsGetDriveStats: Ioctl failed: %d\n",sts);
		return sts;
	}

	/* DIL counters */
	pIntDrvStat = DtsGetgStats ( );

//	memcpy_s(pDrvStat, 128, pIntDrvStat, 128);
	memcpy(pDrvStat,pIntDrvStat, 128);


	/* Driver counters */
	pIntDrvStat = (BC_DTS_STATS *)&pIocData->u.drvStat;
	pDrvStat->drvRLL = pIntDrvStat->drvRLL;
	pDrvStat->drvFLL = pIntDrvStat->drvFLL;
	pDrvStat->intCount = pIntDrvStat->intCount;
	pDrvStat->pauseCount = pIntDrvStat->pauseCount;
	pDrvStat->DrvIgnIntrCnt = pIntDrvStat->DrvIgnIntrCnt;
	pDrvStat->DrvTotalFrmDropped = pIntDrvStat->DrvTotalFrmDropped;
	pDrvStat->DrvTotalHWErrs = pIntDrvStat->DrvTotalHWErrs;
	pDrvStat->DrvTotalPIBFlushCnt = pIntDrvStat->DrvTotalPIBFlushCnt;
	pDrvStat->DrvTotalFrmCaptured = pIntDrvStat->DrvTotalFrmCaptured;		
	pDrvStat->DrvPIBMisses = pIntDrvStat->DrvPIBMisses;
	pDrvStat->DrvPauseTime = pIntDrvStat->DrvPauseTime;
	pDrvStat->DrvRepeatedFrms = pIntDrvStat->DrvRepeatedFrms;	
	pDrvStat->TxFifoBsyCnt = pIntDrvStat->TxFifoBsyCnt;
	pDrvStat->pwr_state_change = pIntDrvStat->pwr_state_change;

	DtsRelIoctlData(Ctx,pIocData);

	return BC_STS_SUCCESS;
}

DRVIFLIB_INT_API BC_STATUS 
DtsRstDrvStat(
    HANDLE		hDevice
    )
{
	BC_IOCTL_DATA *pIocData = NULL;
	DTS_LIB_CONTEXT		*Ctx = NULL;
	BC_STATUS	sts = BC_STS_SUCCESS;

	DTS_GET_CTX(hDevice,Ctx);

	/* CHECK WHETHER NULL pIocData CAN BE PASSED */
	if(!(pIocData = DtsAllocIoctlData(Ctx)))
		return BC_STS_INSUFF_RES;

	/* Driver related counters */
	if( (sts=DtsDrvCmd(Ctx,BCM_IOC_RST_DRV_STAT,0,pIocData,FALSE)) != BC_STS_SUCCESS){
		DtsRelIoctlData(Ctx,pIocData);
		DebugLog_Trace(LDIL_DBG,"DtsRstDrvStats: Ioctl failed: %d\n",sts);
		return sts;
	}

	/* DIL related counters */
	DtsRstStats( );

	DtsRelIoctlData(Ctx,pIocData);

	return BC_STS_SUCCESS;
}

/**/
/* Get firmware files */
DRVIFLIB_INT_API BC_STATUS 
DtsGetFWFiles(
	HANDLE hDevice,
	char *StreamFName,
	char *VDecOuter,
	char *VDecInner
	)
{
	DTS_LIB_CONTEXT		*Ctx = NULL;
	BC_STATUS	sts = BC_STS_SUCCESS;
	
	DTS_GET_CTX(hDevice,Ctx);

	sts = DtsGetFirmwareFiles(Ctx);
	if(sts == BC_STS_SUCCESS){
		strncpy(StreamFName,  Ctx->StreamFile, MAX_PATH);
		strncpy(VDecOuter,  Ctx->VidOuter, MAX_PATH);
		strncpy(VDecInner, Ctx->VidInner, MAX_PATH );
	}else{
		return sts;
	}

	return sts;
}
/**/

DRVIFLIB_INT_API BC_STATUS 
DtsDownloadFWBin(HANDLE	hDevice, uint8_t *binBuff, uint32_t buffsize, uint32_t dramOffset)
{
	BC_STATUS rstatus = BC_STS_SUCCESS;

	/* Write bootloader vector table section */
	rstatus = DtsDevMemWr(hDevice,(uint32_t *)binBuff,buffsize,dramOffset);
	if (BC_STS_SUCCESS != rstatus)
	{
		DebugLog_Trace(LDIL_DBG,"DtsDownloadFWBin: Fw Download Failed\n");
	}

	return rstatus;
}

BC_STATUS
DtsCopyRawDataToOutBuff(DTS_LIB_CONTEXT	*Ctx, 
						BC_DTS_PROC_OUT *Vout, 
						BC_DTS_PROC_OUT *Vin)
{
	uint32_t	y,lDestStride=0;
	uint8_t	*pSrc = NULL, *pDest=NULL;
	uint32_t	dstWidthInPixels, dstHeightInPixels;
	uint32_t srcWidthInPixels, srcHeightInPixels;
	BC_STATUS	Sts = BC_STS_SUCCESS;

	if ( (Sts = DtsChkYUVSizes(Ctx,Vout,Vin)) != BC_STS_SUCCESS)
		return Sts;

	if(Vout->PoutFlags & BC_POUT_FLAGS_STRIDE) 
		lDestStride = Vout->StrideSz;
	
	if(Vout->PoutFlags & BC_POUT_FLAGS_SIZE) {
		// Use DShow provided size for now
		dstWidthInPixels = Vout->PicInfo.width;
		if(Vout->PoutFlags & BC_POUT_FLAGS_INTERLACED)
			dstHeightInPixels = Vout->PicInfo.height/2;
		else
			dstHeightInPixels = Vout->PicInfo.height;
		/* Check for Valid data based on the filter information */
		if(Vout->YBuffDoneSz < (dstWidthInPixels * dstHeightInPixels / 2))
			return BC_STS_IO_XFR_ERROR;
		/* Calculate the appropriate source width and height */
		if(dstWidthInPixels > 1280) {
			srcWidthInPixels = 1920;
			srcHeightInPixels = dstHeightInPixels;
		} else if(dstWidthInPixels > 720) {
			srcWidthInPixels = 1280;
			srcHeightInPixels = dstHeightInPixels;
		} else {
			srcWidthInPixels = 720;
			srcHeightInPixels = dstHeightInPixels;
		}
	} else {
		dstWidthInPixels = Vin->PicInfo.width;
		dstHeightInPixels = Vin->PicInfo.height;
	}

	lDestStride = lDestStride*2;
	dstWidthInPixels = dstWidthInPixels*2;
	srcWidthInPixels = srcWidthInPixels*2;
	// Do a strided copy only if the stride is non-zero
	if( (lDestStride != 0)|| (srcWidthInPixels != dstWidthInPixels) ) {
		// Y plane
		pDest = Vout->Ybuff;
		pSrc = Vin->Ybuff;
		for (y = 0; y < dstHeightInPixels; y++){
			memcpy(pDest,pSrc,dstWidthInPixels);
			//memcpy_fast(pDest,pSrc,dstWidthInPixels*2);
			pDest += dstWidthInPixels + lDestStride;
			pSrc += (srcWidthInPixels);
		}
	} else {
		// Y Plane
		memcpy(Vout->Ybuff, Vin->Ybuff, dstHeightInPixels * dstWidthInPixels);
		//memcpy_fast(Vout->Ybuff, Vin->Ybuff, dstHeightInPixels * dstWidthInPixels * 2);
	}

	return BC_STS_SUCCESS;
}
/***/
//FIX_ME:: This routine assumes, Y & UV buffs are contiguous.. 
BC_STATUS DtsCopyNV12ToYV12(DTS_LIB_CONTEXT	*Ctx, BC_DTS_PROC_OUT *Vout, BC_DTS_PROC_OUT *Vin)
{

	uint8_t	*buff=NULL;
	uint8_t	*yv12buff = NULL;
	uint32_t uvbase=0;
	BC_STATUS	Sts = BC_STS_SUCCESS;
	uint32_t	x,y,lDestStride=0;
	uint8_t	*pSrc = NULL, *pDest=NULL;
	uint32_t	dstWidthInPixels, dstHeightInPixels;
	uint32_t srcWidthInPixels, srcHeightInPixels;	


	if ( (Sts = DtsChkYUVSizes(Ctx,Vout,Vin)) != BC_STS_SUCCESS)
		return Sts;

	if(Vout->PoutFlags & BC_POUT_FLAGS_SIZE)// needs to be optimized.
	{
		if(Vout->PoutFlags & BC_POUT_FLAGS_STRIDE) 
			lDestStride = Vout->StrideSz;
		
		// Use DShow provided size for now
		dstWidthInPixels = Vout->PicInfo.width;
		if(Vout->PoutFlags & BC_POUT_FLAGS_INTERLACED)
			dstHeightInPixels = Vout->PicInfo.height/2;
		else
			dstHeightInPixels = Vout->PicInfo.height;

		/* Check for Valid data based on the filter information */
		if((Vout->YBuffDoneSz < (dstWidthInPixels * dstHeightInPixels / 4)) ||
			(Vout->UVBuffDoneSz < (dstWidthInPixels * dstHeightInPixels/2 / 4)))
		{
			DebugLog_Trace(LDIL_DBG,"DtsCopyYV12[%d x %d]: XFER ERROR %x %x \n",
				dstWidthInPixels, dstHeightInPixels,
				Vout->YBuffDoneSz, Vout->UVBuffDoneSz);
			return BC_STS_IO_XFR_ERROR;
		}

		/* Calculate the appropriate source width and height */
		if(dstWidthInPixels > 1280) 
		{
			srcWidthInPixels = 1920;
			srcHeightInPixels = dstHeightInPixels;
		} 
		else if(dstWidthInPixels > 720) 
		{
			srcWidthInPixels = 1280;
			srcHeightInPixels = dstHeightInPixels;
		} 
		else 
		{
			srcWidthInPixels = 720;
			srcHeightInPixels = dstHeightInPixels;
		}

		//copy luma
		pDest = Vout->Ybuff;
		pSrc = Vin->Ybuff;
		for (y = 0; y < dstHeightInPixels; y++)
		{
			memcpy(pDest,pSrc,dstWidthInPixels);
			pDest += dstWidthInPixels + lDestStride;
			pSrc += srcWidthInPixels;
		}
		//copy chroma
		pDest = Vout->UVbuff;
		pSrc = Vin->UVbuff;
		uvbase = (dstWidthInPixels + lDestStride) * dstHeightInPixels/4 ;//(Vin->UVBuffDoneSz * 4/2);
		for (y = 0; y < dstHeightInPixels/2; y++)
		{
			for(x = 0; x < dstWidthInPixels; x += 2)
			{
				pDest[x/2] = pSrc[x+1];	
				pDest[uvbase + x/2] = pSrc[x];
			}		
			pDest += (dstWidthInPixels + lDestStride) / 2;
			pSrc += srcWidthInPixels;
		}
	}
	else
	{
		/* Y-Buff loop */
		buff = Vin->Ybuff;
		yv12buff = Vout->Ybuff;
		for(uint32_t i = 0; i < Vin->YBuffDoneSz*4; i += 2) {
			yv12buff[i] = buff[i];
			yv12buff[i+1] = buff[i+1];
		}
		
		/* UV-Buff loop */
		buff = Vin->UVbuff;
		yv12buff = Vout->UVbuff;
		uvbase = (Vin->UVBuffDoneSz * 4/2);
		for(uint32_t i = 0; i < Vin->UVBuffDoneSz*4; i += 2) {
			yv12buff[i/2] = buff[i+1];
			yv12buff[uvbase + (i/2)] = buff[i];
		}
	}

	return BC_STS_SUCCESS;
}

	
BC_STATUS DtsCopyNV12(DTS_LIB_CONTEXT *Ctx, BC_DTS_PROC_OUT *Vout, BC_DTS_PROC_OUT *Vin)
{
	uint32_t	y,lDestStride=0;
	uint8_t	*pSrc = NULL, *pDest=NULL;
	uint32_t	dstWidthInPixels, dstHeightInPixels;
	uint32_t srcWidthInPixels, srcHeightInPixels;
	
	BC_STATUS	Sts = BC_STS_SUCCESS;

	if ( (Sts = DtsChkYUVSizes(Ctx,Vout,Vin)) != BC_STS_SUCCESS)
		return Sts;

	if(Vout->PoutFlags & BC_POUT_FLAGS_STRIDE) 
		lDestStride = Vout->StrideSz;
	
	if(Vout->PoutFlags & BC_POUT_FLAGS_SIZE) {
		// Use DShow provided size for now
		dstWidthInPixels = Vout->PicInfo.width;
		if(Vout->PoutFlags & BC_POUT_FLAGS_INTERLACED)
			dstHeightInPixels = Vout->PicInfo.height/2;
		else
			dstHeightInPixels = Vout->PicInfo.height;
		/* Check for Valid data based on the filter information */
		if((Vout->YBuffDoneSz < (dstWidthInPixels * dstHeightInPixels / 4)) ||
			(Vout->UVBuffDoneSz < (dstWidthInPixels * dstHeightInPixels/2 / 4)))
			return BC_STS_IO_XFR_ERROR;
		/* Calculate the appropriate source width and height */
		if(dstWidthInPixels > 1280) {
			srcWidthInPixels = 1920;
			srcHeightInPixels = dstHeightInPixels;
		} else if(dstWidthInPixels > 720) {
			srcWidthInPixels = 1280;
			srcHeightInPixels = dstHeightInPixels;
		} else {
			srcWidthInPixels = 720;
			srcHeightInPixels = dstHeightInPixels;
		}
	} else {
		dstWidthInPixels = Vin->PicInfo.width;
		dstHeightInPixels = Vin->PicInfo.height;
	}

	
	
	//WidthInPixels = 1280;
	//HeightInPixels = 720;
    // NV12 is planar: Y plane, followed by packed U-V plane.

	// Do a strided copy only if the stride is non-zero
	if( (lDestStride != 0) || (srcWidthInPixels != dstWidthInPixels) ) {
		// Y plane
		pDest = Vout->Ybuff;
		pSrc = Vin->Ybuff;
		for (y = 0; y < dstHeightInPixels; y++){
			memcpy(pDest,pSrc,dstWidthInPixels);
			pDest += dstWidthInPixels + lDestStride;
			pSrc += srcWidthInPixels;
		}
	// U-V plane
		pDest = Vout->UVbuff;
		pSrc = Vin->UVbuff;
		for (y = 0; y < dstHeightInPixels/2; y++){
			memcpy(pDest,pSrc,dstWidthInPixels);
			pDest += dstWidthInPixels + lDestStride;
			pSrc += srcWidthInPixels;
		}
	} else {
		// Y Plane
		memcpy(Vout->Ybuff, Vin->Ybuff, dstHeightInPixels * dstWidthInPixels);
		// UV Plane
		memcpy(Vout->UVbuff, Vin->UVbuff, dstHeightInPixels/2 * dstWidthInPixels);

	}

	return BC_STS_SUCCESS;
}

DRVIFLIB_INT_API BC_STATUS
DtsPushFwBinToLink(
	HANDLE	hDevice,
    uint32_t		*Buffer,
    uint32_t		BuffSz)
{
	uint8_t				*pXferBuff;
	BC_IOCTL_DATA	*pIoctlData;	
	BC_CMD_DEV_MEM	*pMemAccess;
	uint32_t				BytesReturned, AllocSz;

	if (!hDevice) {
		DebugLog_Trace(LDIL_DBG,"DtsPushFwBinToLink: Invalid Handle\n");
		return BC_STS_INV_ARG;
	}

	if (!Buffer) {
		DebugLog_Trace(LDIL_DBG,"DtsPushFwBinToLink: Null Buffer\n");
		return BC_STS_INV_ARG;
	}

	if (BuffSz % 4) {
		DebugLog_Trace(LDIL_DBG,"DtsPushFwBinToLink: Buff Size is not a multiple of DWORD\n");
		return BC_STS_ERROR;
	}

	AllocSz = sizeof(BC_IOCTL_DATA) + (BuffSz);
	pIoctlData = (BC_IOCTL_DATA *) malloc(AllocSz);
	if(!pIoctlData) {
		DebugLog_Trace(LDIL_DBG,"DtsPushFwBinToLink: Memory Allocation Failed\n");
		return BC_STS_ERROR;
	}

	pXferBuff = ((PUCHAR)pIoctlData) + sizeof(BC_IOCTL_DATA);
	pMemAccess = &pIoctlData->u.devMem;
	pIoctlData->RetSts = BC_STS_ERROR;
	pIoctlData->IoctlDataSz = sizeof(BC_IOCTL_DATA);
	pMemAccess->StartOff = 0;
	pMemAccess->NumDwords = BuffSz/4;
	memcpy(pXferBuff, Buffer, BuffSz);

	if (!DtsDrvIoctl(hDevice, BCM_IOC_FW_DOWNLOAD, pIoctlData, AllocSz, pIoctlData, AllocSz, (LPDWORD)&BytesReturned, NULL)) {
		DebugLog_Trace(LDIL_DBG,"DtsPushFwBinToLink: DeviceIoControl Failed\n");
		return BC_STS_ERROR;
	}

	if (BC_STS_ERROR == pIoctlData->RetSts) {
		DebugLog_Trace(LDIL_DBG,"DtsPushFwBinToLink: IOCTL Cmd Failed By Driver\n");
		return pIoctlData->RetSts;
	}
	
	if(pIoctlData) {
		free(pIoctlData);
	}

	return BC_STS_SUCCESS;
}


DRVIFLIB_INT_API BC_STATUS
DtsPushAuthFwToLink(HANDLE hDevice, char *FwBinFile)
{
	BC_STATUS		status=BC_STS_ERROR;
	uint32_t				byesDnld=0;
	char			*fwfile=NULL;
	DTS_LIB_CONTEXT *Ctx=NULL;
	
	DTS_GET_CTX(hDevice,Ctx);

	if(Ctx->OpMode == DTS_DIAG_MODE){
		/* In command line case, we don't get a close
		 * between successive devinit commands.
		 */
		Ctx->FixFlags &= ~DTS_LOAD_FILE_PLAY_FW;

		if(FwBinFile){
			if(!strncmp(FwBinFile,"FILE_PLAY_BACK",14)){
				Ctx->FixFlags |=DTS_LOAD_FILE_PLAY_FW;
				FwBinFile=NULL;
			}
		}
	}

	/* Get the firmware file to download */
	if (!FwBinFile) 
	{
		status = DtsGetFirmwareFiles(Ctx);
		if (status == BC_STS_SUCCESS) fwfile = Ctx->FwBinFile;
		else return status;

	} else {
		fwfile = FwBinFile;
	}

	//DebugLog_Trace(LDIL_DBG,"Firmware File is :%s\n",fwfile);

	/* Push the F/W bin file to the driver */	
	status = fwbinPushToLINK(hDevice, fwfile, &byesDnld);
	
	if (status != BC_STS_SUCCESS) {
		DebugLog_Trace(LDIL_DBG,"DtsDownloadAuthFwToLINK: Failed to download firmware\n");
		return status;
	}
	


	
	return BC_STS_SUCCESS;
}



DRVIFLIB_INT_API BC_STATUS 
fwbinPushToLINK(HANDLE hDevice, char *FwBinFile, uint32_t *bytesDnld)
{
	BC_STATUS	status=BC_STS_ERROR;
	uint32_t			FileSz=0;
	uint8_t			*buff=NULL;
	FILE 		*fp=NULL;

	if( (!FwBinFile) || (!hDevice) || (!bytesDnld))
	{
		DebugLog_Trace(LDIL_DBG,"Invalid Arguments\n");
		return BC_STS_INV_ARG;
	}
	
	fp = fopen(FwBinFile,"rb");
	if(!fp)
	{
		DebugLog_Trace(LDIL_DBG,"Failed to Open FW file\n");
		return BC_STS_ERROR;
	}

	fseek(fp,0,SEEK_END);
	FileSz = ftell(fp);
	fseek(fp,0,SEEK_SET);

	buff = (uint8_t *)malloc(FileSz);
	if (!buff) {
		DebugLog_Trace(LDIL_DBG,"Failed to allocate memory\n");
		return BC_STS_INSUFF_RES;
	}

	*bytesDnld = fread(buff,1,FileSz,fp);
	if(0 == *bytesDnld)
	{
		DebugLog_Trace(LDIL_DBG,"Failed to Read The File\n");
		return BC_STS_IO_ERROR;
	}

	status = DtsPushFwBinToLink(hDevice, (uint32_t *)buff, *bytesDnld);
	if(buff) free(buff);
	if(fp) fclose(fp);
	return status;
}


#define rotr32_1(x,n)   (((x) >> n) | ((x) << (32 - n)))
#define bswap_32_1(x) ((rotr32_1((x), 24) & 0x00ff00ff) | (rotr32_1((x), 8) & 0xff00ff00))

BC_STATUS dec_write_fw_Sig(HANDLE hndl,uint32_t* Sig)
{
		unsigned int *ptr = Sig;
		unsigned int DciSigDataReg = (unsigned int)DCI_SIGNATURE_DATA_7;
		BC_STATUS sts = BC_STS_ERROR;

		for (int reg_cnt=0;reg_cnt<8;reg_cnt++)
		{
			sts = DtsFPGARegisterWr(hndl, DciSigDataReg, bswap_32_1(*ptr));
			
			if(sts != BC_STS_SUCCESS)
			{	
				DebugLog_Trace(LDIL_DBG,"Error Wrinting Fw Sig data register\n");
				return sts;
			}
			DciSigDataReg-=4;
			ptr++;
		}
		return sts;

}
/*====================== Debug Routines ========================================*/
void DumpDataToFile(FILE *fp, char *header, uint32_t off, uint8_t *buff, uint32_t dwcount)
{
	uint32_t i, k=1;

#ifndef  _LIB_EN_FWDUMP_
	// Skip FW Download dumping.. 
	return ;
#endif

	if(!fp)
		return;

	if(header){
		fprintf(fp,"%s\n",header);
	}

	for(i = 0; i < dwcount; i++){
		if (k == 1) 
			fprintf(fp, "0x%08X : ", off);
		
		fprintf(fp," 0x%08X ", *((uint32_t *)buff));
		
		buff	+= sizeof(uint32_t);
		off		+= sizeof(uint32_t);
		k++;
		if ((i == dwcount - 1) || (k > 4)){
			fprintf(fp,"\n");
			k = 1;
		}
	}
	//fprintf(fp,"\n");

	fflush(fp);
}

void DumpInputSampleToFile(uint8_t *buff, uint32_t buffsize)
{
#ifndef  _LIB_EN_INDUMP_
	return ;
#endif

	static FILE	*pOutputFile=NULL;

	if(!buff || !buffsize){
		if(pOutputFile){
			fclose(pOutputFile);
			pOutputFile = NULL;
		}
		return;
	}

	if(!pOutputFile){
		if(!(pOutputFile = fopen("hdfile_dump.ts","wb")) )
			return;
	}

	fwrite(buff, sizeof(uint8_t), buffsize, pOutputFile);

	fflush(pOutputFile);
}

