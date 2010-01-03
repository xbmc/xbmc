/********************************************************************
 * Copyright(c) 2006-2009 Broadcom Corporation.
 *
 *  Name: libcrystalhd_fwcmds.cpp
 *
 *  Description: FW commands.
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

#include "libcrystalhd_fwcmds.h"

DRVIFLIB_INT_API BC_STATUS 
DtsFWInitialize(
    HANDLE  hDevice,
	uint32_t		resrv1
    )
{
	BC_STATUS	sts = BC_STS_SUCCESS;
	C011CmdInit	*vi=NULL;

	DTS_LIB_CONTEXT		*Ctx = NULL;
	BC_IOCTL_DATA *pIocData = NULL;
	
	DTS_GET_CTX(hDevice,Ctx);

	if(!(pIocData = DtsAllocIoctlData(Ctx)))
		return BC_STS_INSUFF_RES;

	vi = (C011CmdInit*)&pIocData->u.fwCmd.cmd;

	vi->command			= eCMD_C011_INIT;
	vi->sequence		= ++Ctx->fwcmdseq;
	vi->memSizeMBytes	= 0x00000040; // 64MB
//	vi->inputClkFreq	= 200000000; // 200 MHz
	vi->inputClkFreq	= 200000000; // 20 MHz
	vi->uartBaudRate	= 38400;
	vi->initArcs		= C011_STREAM_ARC|C011_VDEC_ARC;
	vi->interrupt		= eC011_INT_ENABLE;
	vi->brcmMode		= eC011_BRCM_ECG_MODE_ON;
	vi->fgtEnable		= 0x00000001;
	vi->openMode        = Ctx->FixFlags;
	
	if(Ctx->DevId == BC_PCI_DEVID_LINK)
		vi->rsaDecrypt = 0x00000001;
	
	if( (sts=DtsDrvCmd(Ctx,BCM_IOC_FW_CMD,1,pIocData,FALSE)) != BC_STS_SUCCESS){
		DtsRelIoctlData(Ctx,pIocData);
		DebugLog_Trace(LDIL_DBG,"DtsInitialize: Ioctl failed: %d\n",sts);
		return sts;
	}

	if(pIocData->u.fwCmd.rsp[2]){
		DtsRelIoctlData(Ctx,pIocData);
		DebugLog_Trace(LDIL_DBG,"DtsInitialize: Failed %d\n",pIocData->u.fwCmd.rsp[2]);
		return BC_STS_FW_CMD_ERR;
	}
	
	DtsRelIoctlData(Ctx,pIocData);

	return BC_STS_SUCCESS;
}

DRVIFLIB_INT_API BC_STATUS
DtsFWOpenChannel(HANDLE hDevice, uint32_t StreamType, uint32_t reserved)
{
	BC_STATUS sts = BC_STS_SUCCESS;
	DecCmdChannelStreamOpen *pOpen;
	DecRspChannelStreamOpen	*pRsp;

	DTS_LIB_CONTEXT	*Ctx = NULL;
	BC_IOCTL_DATA *pIocData = NULL;

	DTS_GET_CTX(hDevice,Ctx);

	if (!(pIocData = DtsAllocIoctlData(Ctx)))
		return BC_STS_INSUFF_RES;

	pOpen = (DecCmdChannelStreamOpen *)&pIocData->u.fwCmd.cmd;

	pOpen->command	  = eCMD_C011_DEC_CHAN_STREAM_OPEN;
	pOpen->sequence	  = ++Ctx->fwcmdseq;
	pOpen->inPort	  = eC011_IN_PORT0;		//CSI
	pOpen->streamType = (eC011_STREAM_TYPE)StreamType;

	sts = DtsDrvCmd(Ctx, BCM_IOC_FW_CMD, 1, pIocData, FALSE);
	if (sts != BC_STS_SUCCESS) {
		DtsRelIoctlData(Ctx, pIocData);
		DebugLog_Trace(LDIL_DBG, "DtsOpenDecoder: Ioctl failed: %d\n", sts);
		return sts;
	}

	pRsp = (DecRspChannelStreamOpen*)&pIocData->u.fwCmd.rsp;

	if (pRsp->status) {
		DebugLog_Trace(LDIL_DBG, "DtsOpenDecoder: Failed %u\n", pRsp->status);
		DtsRelIoctlData(Ctx, pIocData);
		return BC_STS_FW_CMD_ERR;
	}

	memcpy(&Ctx->OpenRsp, pRsp, sizeof(Ctx->OpenRsp));

	// For deconf backward compatibility only...
	Ctx->State = BC_DEC_STATE_OPEN;

	DtsRelIoctlData(Ctx, pIocData);

	return BC_STS_SUCCESS;
}


DRVIFLIB_INT_API BC_STATUS 
DtsFWActivateDecoder(
    HANDLE  hDevice
    )
{
	BC_STATUS	sts = BC_STS_SUCCESS;
	DecCmdChannelActivate	*pAct;
	DecRspChannelActivate	*pActRsp;

	DTS_LIB_CONTEXT		*Ctx = NULL;
	BC_IOCTL_DATA *pIocData = NULL;
	
	DTS_GET_CTX(hDevice,Ctx);

	if(Ctx->State != BC_DEC_STATE_OPEN){
		DebugLog_Trace(LDIL_DBG,"DtsActChannel: Channel Not Opened\n");
		return BC_STS_DEC_NOT_OPEN;
	}

	if(!(pIocData = DtsAllocIoctlData(Ctx)))
		return BC_STS_INSUFF_RES;


	pAct = (DecCmdChannelActivate *)&pIocData->u.fwCmd.cmd;

	pAct->command = eCMD_C011_DEC_CHAN_ACTIVATE;
	pAct->sequence = ++Ctx->fwcmdseq;
	pAct->channelId = Ctx->OpenRsp.channelId;

	if( (sts=DtsDrvCmd(Ctx,BCM_IOC_FW_CMD,1,pIocData,FALSE)) != BC_STS_SUCCESS){
		DebugLog_Trace(LDIL_DBG,"DtsActChannel: Ioctl failed: %d\n",sts);
		DtsRelIoctlData(Ctx,pIocData);
		return sts;
	}
	
	pActRsp = (DecRspChannelActivate*)&pIocData->u.fwCmd.rsp;

	if(pActRsp->status){
		DebugLog_Trace(LDIL_DBG,"DtsActChannel: ChannelActivate Failed %u\n",pActRsp->status);
		DtsRelIoctlData(Ctx,pIocData);
		return BC_STS_FW_CMD_ERR;
	}
	
	DtsRelIoctlData(Ctx,pIocData);

	return BC_STS_SUCCESS;
}

DRVIFLIB_INT_API BC_STATUS 
DtsFWHwSelfTest(
    HANDLE  hDevice,
    uint32_t     testID
    )
{
	BC_STATUS	sts = BC_STS_SUCCESS;
	C011CmdSelfTest	*stest=NULL;
	C011RspSelfTest *pRsp = NULL;

	DTS_LIB_CONTEXT		*Ctx = NULL;
	BC_IOCTL_DATA *pIocData = NULL;

	
	DTS_GET_CTX(hDevice,Ctx);

	if(!(pIocData = DtsAllocIoctlData(Ctx)))
		return BC_STS_INSUFF_RES;

	stest = (C011CmdSelfTest *)&pIocData->u.fwCmd.cmd;
	stest->command = eCMD_C011_SELF_TEST;
	stest->sequence = ++Ctx->fwcmdseq;
	stest->testId = (eC011_TEST_ID)testID;

	if( (sts=DtsDrvCmd(Ctx,BCM_IOC_FW_CMD,1,pIocData,FALSE)) != BC_STS_SUCCESS){
		DebugLog_Trace(LDIL_DBG,"DtsHwSelfTest: Ioctl failed: %d\n",sts);
		DtsRelIoctlData(Ctx,pIocData);
		return sts;
	}

	pRsp = (C011RspSelfTest*)&pIocData->u.fwCmd.rsp;

	if(pRsp->status){
		DebugLog_Trace(LDIL_DBG,"DtsHwSelfTest: SetVideoOut Failed %u\n",pRsp->status);
		DtsRelIoctlData(Ctx,pIocData);
		return BC_STS_FW_CMD_ERR;
	}

	DtsRelIoctlData(Ctx,pIocData);
	return BC_STS_SUCCESS;
}


DRVIFLIB_INT_API BC_STATUS 
DtsFWVersion(
    HANDLE  hDevice,
    uint32_t     *Stream,
    uint32_t     *DecCore,
	uint32_t		*HwNumber
    )
{
	BC_STATUS	sts = BC_STS_SUCCESS;
	C011CmdGetVersion *ver=NULL;
	C011RspGetVersion *pRsp = NULL;

	DTS_LIB_CONTEXT		*Ctx = NULL;
	BC_IOCTL_DATA *pIocData = NULL;

	DTS_GET_CTX(hDevice,Ctx);

	if(!Stream || !DecCore || !HwNumber)
	{
		DebugLog_Trace(LDIL_DBG,"DtsFWVersion: Invalid Handle\n");
		return BC_STS_INV_ARG;
	}
	
	if(!(pIocData = DtsAllocIoctlData(Ctx)))
		return BC_STS_INSUFF_RES;

	ver = (C011CmdGetVersion *)&pIocData->u.fwCmd.cmd;
	ver->command = eCMD_C011_GET_VERSION;
	ver->sequence = ++Ctx->fwcmdseq;

	if( (sts=DtsDrvCmd(Ctx,BCM_IOC_FW_CMD,1,pIocData,FALSE)) != BC_STS_SUCCESS){
		DebugLog_Trace(LDIL_DBG,"DtsFWVersion: Ioctl failed: %d\n",sts);
		DtsRelIoctlData(Ctx,pIocData);
		return sts;
	}

	pRsp = (C011RspGetVersion*)&pIocData->u.fwCmd.rsp;

	if(pRsp->status){
		DebugLog_Trace(LDIL_DBG,"DtsHwSelfTest: SetVideoOut Failed %u\n",pRsp->status);
		DtsRelIoctlData(Ctx,pIocData);
		return BC_STS_FW_CMD_ERR;
	}
	
	*Stream  = pRsp->streamSwVersion;
	*DecCore = pRsp->decoderSwVersion;
	*HwNumber = pRsp->chipHwVersion;

	DtsRelIoctlData(Ctx,pIocData);

	return BC_STS_SUCCESS;
}

DRVIFLIB_INT_API BC_STATUS 
DtsFWFifoStatus(
    HANDLE  hDevice,
    uint32_t     *CpbSize,
	uint32_t		*CpbFullness
    )
{
	BC_STATUS	sts = BC_STS_SUCCESS;
	DecCmdChannelStatus *pCmd=NULL;
	DecRspChannelStatus *pRsp = NULL;

	DTS_LIB_CONTEXT		*Ctx = NULL;
	BC_IOCTL_DATA *pIocData = NULL;

	DTS_GET_CTX(hDevice,Ctx);

	if(Ctx->State == BC_DEC_STATE_INVALID){
		DebugLog_Trace(LDIL_DBG,"DtsFifoStatus: No Active Decoder\n");
		return BC_STS_DEC_NOT_OPEN;
	}

	if(!CpbSize || !CpbFullness)
	{
		DebugLog_Trace(LDIL_DBG,"DtsFifoStatus: Invalid Args\n");
		return BC_STS_INV_ARG;
	}

	if(!(pIocData = DtsAllocIoctlData(Ctx)))
		return BC_STS_INSUFF_RES;

	pCmd = (DecCmdChannelStatus *)&pIocData->u.fwCmd.cmd;
	pCmd->command = eCMD_C011_DEC_CHAN_STATUS;
	pCmd->sequence = ++Ctx->fwcmdseq;
	pCmd->channelId = Ctx->OpenRsp.channelId;

	if( (sts=DtsDrvCmd(Ctx,BCM_IOC_FW_CMD,1, pIocData, FALSE)) != BC_STS_SUCCESS){
		DebugLog_Trace(LDIL_DBG,"DtsChannelStatus: Ioctl failed: %d\n",sts);
		DtsRelIoctlData(Ctx,pIocData);
		return sts;
	}

	pRsp = (DecRspChannelStatus*)&pIocData->u.fwCmd.rsp;

	if(pRsp->status){
		DebugLog_Trace(LDIL_DBG,"DtsChannelStatus: Failed %u\n",pRsp->status);
		DtsRelIoctlData(Ctx,pIocData);
		return BC_STS_FW_CMD_ERR;
	}
	
	*CpbSize = pRsp->cpbSize;
	*CpbFullness = pRsp->cpbFullness;

	DtsRelIoctlData(Ctx,pIocData);
	
	return BC_STS_SUCCESS;
}

DRVIFLIB_INT_API BC_STATUS 
DtsFWCloseChannel(
    HANDLE  hDevice,
	uint32_t		ChannelID
    )
{
	BC_STATUS	sts = BC_STS_SUCCESS;
	DecCmdChannelClose *pCmd;
	DecRspChannelClose *pRsp;
	DTS_LIB_CONTEXT		*Ctx = NULL;
	BC_IOCTL_DATA *pIocData = NULL;

	DTS_GET_CTX(hDevice,Ctx);

	if(Ctx->State == BC_DEC_STATE_INVALID){
		DebugLog_Trace(LDIL_DBG,"DtsCloseDecoder: Channel is not Open\n");
	}

	if(!(pIocData = DtsAllocIoctlData(Ctx)))
		return BC_STS_INSUFF_RES;

	pCmd  = ((DecCmdChannelClose*)&pIocData->u.fwCmd.cmd);

	pCmd->command          = eCMD_C011_DEC_CHAN_CLOSE;
	pCmd->sequence         = ++Ctx->fwcmdseq;
	//pCmd->channelId        = Ctx->OpenRsp.channelId;
	pCmd->channelId        = ChannelID;
	pCmd->pictureRelease   = eC011_PIC_REL_INTERNAL;
	pCmd->lastPicDisplay   = eC011_LASTPIC_DISPLAY_ON;
	
	if( (sts=DtsDrvCmd(Ctx,BCM_IOC_FW_CMD,1,pIocData,FALSE)) != BC_STS_SUCCESS){
		DebugLog_Trace(LDIL_DBG,"DtsCloseDecoder: Ioctl failed: %d\n",sts);
		DtsRelIoctlData(Ctx,pIocData);
		return sts;
	}

	pRsp = (DecRspChannelClose*)&pIocData->u.fwCmd.rsp;

	if(pRsp->status){
		DebugLog_Trace(LDIL_DBG,"DtsCloseDecoder: Failed %u\n",pRsp->status);
		DtsRelIoctlData(Ctx,pIocData);
		return BC_STS_FW_CMD_ERR;
	}

	memset(&Ctx->OpenRsp,0,sizeof(Ctx->OpenRsp));

	DtsRelIoctlData(Ctx,pIocData);

	return BC_STS_SUCCESS;
}

DRVIFLIB_INT_API BC_STATUS 
DtsFWSetVideoInput(
    HANDLE  hDevice
    )
{
	BC_STATUS	sts = BC_STS_SUCCESS;
	DecCmdChannelSetInputParams	*vi=NULL;

	DTS_LIB_CONTEXT		*Ctx = NULL;
	BC_IOCTL_DATA *pIocData = NULL;

	
	DTS_GET_CTX(hDevice,Ctx);

	if((Ctx->State != BC_DEC_STATE_OPEN)){
		DebugLog_Trace(LDIL_DBG,"DtsSetVideoInput: Channel not opened\n");
		return BC_STS_DEC_NOT_OPEN;
	}

	if(!(pIocData = DtsAllocIoctlData(Ctx)))
		return BC_STS_INSUFF_RES;

	vi = (DecCmdChannelSetInputParams*)&pIocData->u.fwCmd.cmd;

	vi->command			= eCMD_C011_DEC_CHAN_INPUT_PARAMS;
	vi->sequence		= ++Ctx->fwcmdseq;
	vi->syncMode		= eC011_SYNC_MODE_SYNCPIN;
	
	if( (sts=DtsDrvCmd(Ctx,BCM_IOC_FW_CMD,1,pIocData,FALSE)) != BC_STS_SUCCESS){
		DebugLog_Trace(LDIL_DBG,"DtsSetVideoInput: Ioctl failed: %d\n",sts);
		DtsRelIoctlData(Ctx,pIocData);
		return sts;
	}

	if(pIocData->u.fwCmd.rsp[2]){
		DebugLog_Trace(LDIL_DBG,"DtsSetVideoInput: SetInputParameters Failed %d\n",pIocData->u.fwCmd.rsp[2]);
		sts = BC_STS_FW_CMD_ERR;
	}
	DtsRelIoctlData(Ctx,pIocData);

	return sts;
}

DRVIFLIB_INT_API BC_STATUS 
DtsFWSetVideoPID(
    HANDLE  hDevice,
	uint32_t		pid
    )
{
	BC_STATUS	sts = BC_STS_SUCCESS;
	DecCmdChannelSetTSPIDs	*spid=NULL;

	DTS_LIB_CONTEXT		*Ctx = NULL;
	BC_IOCTL_DATA *pIocData = NULL;

	DTS_GET_CTX(hDevice,Ctx);

	if((Ctx->State != BC_DEC_STATE_OPEN)){
		DebugLog_Trace(LDIL_DBG,"DtsSetTSPIDs: Channel not opened\n");
		return BC_STS_DEC_NOT_OPEN;
	}

	if(!(pIocData = DtsAllocIoctlData(Ctx)))
		return BC_STS_INSUFF_RES;

	spid = (DecCmdChannelSetTSPIDs*)&pIocData->u.fwCmd.cmd;

	spid->command		= eCMD_C011_DEC_CHAN_TS_PIDS;
	spid->sequence		= ++Ctx->fwcmdseq;
	spid->channelId		= Ctx->OpenRsp.channelId;
	spid->videoPid		= pid;
	
	if( (sts=DtsDrvCmd(Ctx,BCM_IOC_FW_CMD,1,pIocData,FALSE)) != BC_STS_SUCCESS){
		DebugLog_Trace(LDIL_DBG,"DtsSetTSPIDS: Ioctl failed: %d\n",sts);
		DtsRelIoctlData(Ctx,pIocData);
		return sts;
	}

	if(pIocData->u.fwCmd.rsp[2]){
		DebugLog_Trace(LDIL_DBG,"DtsSetTSPIDs:  Failed %d\n",pIocData->u.fwCmd.rsp[2]);
		DtsRelIoctlData(Ctx,pIocData);
		return BC_STS_FW_CMD_ERR;
	}
	
	DtsRelIoctlData(Ctx,pIocData);

	return BC_STS_SUCCESS;
}

DRVIFLIB_INT_API BC_STATUS 
DtsFWFlushDecoder(
    HANDLE  hDevice,
    uint32_t     rsrv
	)
{
	BC_STATUS	sts = BC_STS_SUCCESS;
	DecCmdChannelFlush	*fl;
	DecRspChannelFlush	*flRsp;

	DTS_LIB_CONTEXT		*Ctx = NULL;
	BC_IOCTL_DATA *pIocData = NULL;

	DTS_GET_CTX(hDevice,Ctx);

	if( (Ctx->State != BC_DEC_STATE_OPEN) && ((Ctx->State != BC_DEC_STATE_START)) ){
		DebugLog_Trace(LDIL_DBG,"DtsFlushDecoder: Channel Not Opened\n");
		return BC_STS_DEC_NOT_OPEN;
	}

	if(!(pIocData = DtsAllocIoctlData(Ctx)))
		return BC_STS_INSUFF_RES;

	fl = (DecCmdChannelFlush *)&pIocData->u.fwCmd.cmd;

	fl->command = eCMD_C011_DEC_CHAN_FLUSH;
	fl->sequence = ++Ctx->fwcmdseq;
	fl->channelId = Ctx->OpenRsp.channelId;
	fl->flushMode = eC011_FLUSH_PROC_POINT_RESET_TS;

	if( (sts=DtsDrvCmd(Ctx,BCM_IOC_FW_CMD,1,pIocData,FALSE)) != BC_STS_SUCCESS){
		DebugLog_Trace(LDIL_DBG,"DtsFlushDecoder: Ioctl failed: %d\n",sts);
		DtsRelIoctlData(Ctx,pIocData);
		return sts;
	}
	flRsp = (DecRspChannelFlush*)&pIocData->u.fwCmd.rsp;
	if(flRsp->status){
		DebugLog_Trace(LDIL_DBG,"DtsFlushDecoder: Flush Decoder Failed %u\n",flRsp->status);
		DtsRelIoctlData(Ctx,pIocData);
		return BC_STS_FW_CMD_ERR;
	}
	
	DtsRelIoctlData(Ctx,pIocData);

	return BC_STS_SUCCESS;
}

DRVIFLIB_INT_API BC_STATUS 
DtsFWStartVideo(
    HANDLE  hDevice,
	uint32_t		videoAlg,
	uint32_t		FGTEnable,
	uint32_t		MetaDataEnable,
	uint32_t		Progressive,
    uint32_t     OptFlags
	)
{
	BC_STATUS	sts = BC_STS_SUCCESS;
	DecCmdChannelStartVideo	*sVid;
	DecRspChannelStartVideo	*sVidRsp;

	DTS_LIB_CONTEXT		*Ctx = NULL;
	BC_IOCTL_DATA *pIocData = NULL;

	DTS_GET_CTX(hDevice,Ctx);

	if((Ctx->State != BC_DEC_STATE_OPEN) && (Ctx->State != BC_DEC_STATE_STOP) ){
		DebugLog_Trace(LDIL_DBG,"DtsStartVideo: Channel Not Opened\n");
		return BC_STS_DEC_NOT_OPEN;
	}

	if(!(pIocData = DtsAllocIoctlData(Ctx)))
		return BC_STS_INSUFF_RES;

	/* Let Driver know the Response offset for PIB Delivery and Response Qs.*/
	pIocData->u.fwCmd.flags = BC_FW_CMD_PIB_QS;
	pIocData->u.fwCmd.add_data = ((long) &((DecRspChannelStartVideo *)0)->picInfoDeliveryQ);

	sVid = (DecCmdChannelStartVideo *)&pIocData->u.fwCmd.cmd;

	sVid->command		= eCMD_C011_DEC_CHAN_START_VIDEO;
	sVid->sequence		= ++Ctx->fwcmdseq;
	sVid->channelId		= Ctx->OpenRsp.channelId;
	sVid->outVidPort	= eC011_OUT_PORT0;
	sVid->maxPicSize	= eC011_MAX_PICSIZE_HD;
	sVid->outCtrlMode	= eC011_OUTCTRL_VIDEO_TIMING;
	sVid->chanType		= eC011_CHANNEL_PLAYBACK;
	sVid->videoAlg		= (eC011_VIDEO_ALG)videoAlg;
	sVid->sourceMode	= eC011_VIDSRC_DEFAULT_PROGRESSIVE;
	sVid->pulldown		= eC011_PULLDOWN_DEFAULT_32;
	if(Ctx->RegCfg.DbgOptions & BC_BIT(6) ){
		/* PIB in regular DEL/RELQ scheme. */
		sVid->picInfo		= eC011_PICTURE_INFO_ON;
	}else {
		/* PIB embedded with in the frame */
		sVid->picInfo		= eC011_PICTURE_INFO_OFF;
	}
	sVid->displayOrder	= eC011_DISPLAY_ORDER_DISPLAY;
	sVid->streamId		= 0;
	sVid->vcxoControl	= eC011_EXTERNAL_VCXO_OFF;
	sVid->enableFgt		= FGTEnable;
	sVid->enable23_297FrameRateOutput = Progressive;

	sVid->displayTiming = eC011_DISPLAY_TIMING_IGNORE_PTS;

	//Line 21 Closed Caption
	sVid->userDataMode = eC011_USER_DATA_MODE_ON;

	sVid->defaultFrameRate = (OptFlags&0x0f);
	sVid->decOperationMode = (OptFlags&0x30)>>4;

	if( (sts=DtsDrvCmd(Ctx,BCM_IOC_FW_CMD,1,pIocData,FALSE)) != BC_STS_SUCCESS){
		DebugLog_Trace(LDIL_DBG,"DtsStartVideo: Ioctl failed: %d\n",sts);
		DtsRelIoctlData(Ctx,pIocData);
		return sts;
	}


	sVidRsp = (DecRspChannelStartVideo*)&pIocData->u.fwCmd.rsp;
	if(sVidRsp->status){
		DebugLog_Trace(LDIL_DBG,"DtsStartVideo: StartVideo Failed %u\n",sVidRsp->status);
		DtsRelIoctlData(Ctx,pIocData);
		return BC_STS_FW_CMD_ERR;
	}

	memcpy(&Ctx->sVidRsp,sVidRsp,sizeof(Ctx->sVidRsp));

	Ctx->State = BC_DEC_STATE_START;

	DtsRelIoctlData(Ctx,pIocData);

	return BC_STS_SUCCESS;
}

DRVIFLIB_INT_API BC_STATUS 
DtsFWStopVideo(
    HANDLE  hDevice,
	uint32_t     ChannelId,
	BOOL	ForceStop
	)
{
	BC_STATUS	sts = BC_STS_SUCCESS;
	DecCmdChannelStopVideo	*sVid;
	DecRspChannelStopVideo	*sVidRsp;

	DTS_LIB_CONTEXT		*Ctx = NULL;
	BC_IOCTL_DATA *pIocData = NULL;

	DTS_GET_CTX(hDevice,Ctx);

	if(!(ForceStop) && (Ctx->State != BC_DEC_STATE_START)
					&& (Ctx->State != BC_DEC_STATE_PAUSE)){
		DebugLog_Trace(LDIL_DBG,"DtsStopVideo: Channel Not Opened\n");
		return BC_STS_DEC_NOT_OPEN;
	}

	if(!(pIocData = DtsAllocIoctlData(Ctx)))
		return BC_STS_INSUFF_RES;

	sVid = (DecCmdChannelStopVideo *)&pIocData->u.fwCmd.cmd;

	sVid->command		= eCMD_C011_DEC_CHAN_STOP_VIDEO;
	sVid->sequence		= ++Ctx->fwcmdseq;
	//sVid->channelId		= Ctx->OpenRsp.channelId;
	sVid->channelId		= ChannelId;
	sVid->pictureRelease= eC011_PIC_REL_INTERNAL;
	sVid->lastPicDisplay= eC011_LASTPIC_DISPLAY_ON;

	if( (sts=DtsDrvCmd(Ctx,BCM_IOC_FW_CMD,1,pIocData,FALSE)) != BC_STS_SUCCESS){
		DebugLog_Trace(LDIL_DBG,"DtsStopVideo: Ioctl failed: %d\n",sts);
		DtsRelIoctlData(Ctx,pIocData);
		return sts;
	}

	sVidRsp = (DecRspChannelStopVideo*)&pIocData->u.fwCmd.rsp;
	if(sVidRsp->status){
		DebugLog_Trace(LDIL_DBG,"DtsStopVideo: StopVideo Failed %u\n",sVidRsp->status);
		DtsRelIoctlData(Ctx,pIocData);
		return BC_STS_FW_CMD_ERR;
	}

	Ctx->State = BC_DEC_STATE_STOP;

	DtsRelIoctlData(Ctx,pIocData);

	return BC_STS_SUCCESS;
}

DRVIFLIB_INT_API BC_STATUS 
DtsFWDecFlushChannel(
    HANDLE  hDevice,
	uint32_t		Operation
	)
{
	BC_STATUS	sts = BC_STS_SUCCESS;

	DecCmdChannelFlush	*cFlush;
	DecRspChannelFlush	*rspFlush;

	DTS_LIB_CONTEXT		*Ctx = NULL;
	BC_IOCTL_DATA *pIocData = NULL;

	DTS_GET_CTX(hDevice,Ctx);

	if( (Operation <0) || (Operation > 2) )
		return BC_STS_INV_ARG;

	if(!(pIocData = DtsAllocIoctlData(Ctx)))
		return BC_STS_INSUFF_RES;

	cFlush = (DecCmdChannelFlush *)&pIocData->u.fwCmd.cmd;
	cFlush->command		= eCMD_C011_DEC_CHAN_FLUSH;
	cFlush->sequence	= ++Ctx->fwcmdseq;
	cFlush->channelId	= Ctx->OpenRsp.channelId;
	cFlush->flushMode   = (eC011_FLUSH_MODE)Operation;
	
	
	if( (sts=DtsDrvCmd(Ctx,BCM_IOC_FW_CMD,1,pIocData,FALSE)) != BC_STS_SUCCESS){
		DebugLog_Trace(LDIL_DBG,"DtsFWDecFlushChannel: Ioctl failed: %d\n",sts);
		DtsRelIoctlData(Ctx,pIocData);
		return sts;
	}

	rspFlush = (DecRspChannelFlush*)&pIocData->u.fwCmd.rsp;
	if(rspFlush->status){
		DebugLog_Trace(LDIL_DBG,"DtsFWDecFlushChannel: FlushChannel Failed %u\n",rspFlush->status);
		DtsRelIoctlData(Ctx,pIocData);
		return BC_STS_FW_CMD_ERR;
	}

	DtsRelIoctlData(Ctx,pIocData);

	return BC_STS_SUCCESS;
}

DRVIFLIB_INT_API BC_STATUS 
DtsFWPauseVideo(
    HANDLE  hDevice,
	uint32_t	Operation
	)
{
	BC_STATUS	sts = BC_STS_SUCCESS;

	DecCmdChannelPause	*cPause;

	DecRspChannelPause	*rspPause;

	DTS_LIB_CONTEXT		*Ctx = NULL;
	BC_IOCTL_DATA *pIocData = NULL;

	DTS_GET_CTX(hDevice,Ctx);

	if(!(pIocData = DtsAllocIoctlData(Ctx)))
		return BC_STS_INSUFF_RES;

	cPause = (DecCmdChannelPause *)&pIocData->u.fwCmd.cmd;
	cPause->command		= eCMD_C011_DEC_CHAN_PAUSE;
	cPause->sequence	= ++Ctx->fwcmdseq;
	cPause->channelId	= Ctx->OpenRsp.channelId;
	cPause->enableState   = (eC011_PAUSE_MODE)Operation;
	
	
	if( (sts=DtsDrvCmd(Ctx,BCM_IOC_FW_CMD,1,pIocData,FALSE)) != BC_STS_SUCCESS){
		DebugLog_Trace(LDIL_DBG,"DtsFWPauseVideo: Ioctl failed: %d\n",sts);
		DtsRelIoctlData(Ctx,pIocData);
		return sts;
	}

	rspPause = (DecRspChannelPause*)&pIocData->u.fwCmd.rsp;
	if(rspPause->status){
		DebugLog_Trace(LDIL_DBG,"DtsFWPauseVideo: PauseChannel Failed %u\n",rspPause->status);
		DtsRelIoctlData(Ctx,pIocData);
		return BC_STS_FW_CMD_ERR;
	}

	DtsRelIoctlData(Ctx,pIocData);

	return BC_STS_SUCCESS;
}

DRVIFLIB_INT_API BC_STATUS 
DtsFWSetTrickPlay(
	HANDLE hDevice,
	uint32_t	trickMode,
	uint8_t direction
	)
{
	BC_STATUS	sts = BC_STS_SUCCESS;

	DecCmdChannelTrickPlay	*cTrickPlay;

	DecRspChannelTrickPlay	*rspTrickPlay;

	DTS_LIB_CONTEXT		*Ctx = NULL;
	BC_IOCTL_DATA *pIocData = NULL;

	DTS_GET_CTX(hDevice,Ctx);

	if(!(pIocData = DtsAllocIoctlData(Ctx)))
		return BC_STS_INSUFF_RES;

	cTrickPlay = (DecCmdChannelTrickPlay *)&pIocData->u.fwCmd.cmd;
	cTrickPlay->command		= eCMD_C011_DEC_CHAN_TRICK_PLAY;
	cTrickPlay->sequence	= ++Ctx->fwcmdseq;
	cTrickPlay->channelId	= Ctx->OpenRsp.channelId;
	cTrickPlay->direction   = (eC011_DIR)direction;
	cTrickPlay->speed = (eC011_SPEED)trickMode;
	
	
	if( (sts=DtsDrvCmd(Ctx,BCM_IOC_FW_CMD,1,pIocData,FALSE)) != BC_STS_SUCCESS){
		DebugLog_Trace(LDIL_DBG,"DtsFWSetTrickPlay: Ioctl failed: %d\n",sts);
		DtsRelIoctlData(Ctx,pIocData);
		return sts;
	}

	rspTrickPlay = (DecRspChannelTrickPlay*)&pIocData->u.fwCmd.rsp;
	if(rspTrickPlay->status){
		DebugLog_Trace(LDIL_DBG,"DtsFWSetTrickPlay: TrickPlay Failed %u\n",rspTrickPlay->status);
		DtsRelIoctlData(Ctx,pIocData);
		return BC_STS_FW_CMD_ERR;
	}

	DtsRelIoctlData(Ctx,pIocData);

	return BC_STS_SUCCESS;

}

DRVIFLIB_INT_API BC_STATUS 
DtsFWSetHostTrickMode(
	HANDLE hDevice,
	uint32_t	enable	
	)
{
	BC_STATUS	sts = BC_STS_SUCCESS;

	DecCmdChannelSetHostTrickMode	*cHostTrickMode;

	DecRspChannelSetHostTrickMode	*rspHostTrickMode;

	DTS_LIB_CONTEXT		*Ctx = NULL;
	BC_IOCTL_DATA *pIocData = NULL;

	DTS_GET_CTX(hDevice,Ctx);

	if(!(pIocData = DtsAllocIoctlData(Ctx)))
		return BC_STS_INSUFF_RES;

	cHostTrickMode = (DecCmdChannelSetHostTrickMode *)&pIocData->u.fwCmd.cmd;
	cHostTrickMode->command		= eCMD_C011_DEC_CHAN_SET_HOST_TRICK_MODE;
	cHostTrickMode->sequence	= ++Ctx->fwcmdseq;
	cHostTrickMode->channelId	= Ctx->OpenRsp.channelId;
	cHostTrickMode->enable   = enable;	
	
	if( (sts=DtsDrvCmd(Ctx,BCM_IOC_FW_CMD,1,pIocData,FALSE)) != BC_STS_SUCCESS){
		DebugLog_Trace(LDIL_DBG,"DtsFWSetHostTrickMode: Ioctl failed: %d\n",sts);
		DtsRelIoctlData(Ctx,pIocData);
		return sts;
	}

	rspHostTrickMode = (DecRspChannelSetHostTrickMode*)&pIocData->u.fwCmd.rsp;
	if(rspHostTrickMode->status){
		DebugLog_Trace(LDIL_DBG,"DtsFWSetHostTrickMode:  Failed %u\n",rspHostTrickMode->status);
		DtsRelIoctlData(Ctx,pIocData);
		return BC_STS_FW_CMD_ERR;
	}

	DtsRelIoctlData(Ctx,pIocData);

	return BC_STS_SUCCESS;

}

DRVIFLIB_INT_API BC_STATUS 
DtsFWSetFFRate(
	HANDLE hDevice,
	uint32_t	Rate	
	)
{
	BC_STATUS	sts = BC_STS_SUCCESS;

	DecCmdChannelSetFFRate	*cFFRate;

	DecRspChannelSetFFRate	*rspFFRate;

	DTS_LIB_CONTEXT		*Ctx = NULL;
	BC_IOCTL_DATA *pIocData = NULL;

	DTS_GET_CTX(hDevice,Ctx);

	if(!(pIocData = DtsAllocIoctlData(Ctx)))
		return BC_STS_INSUFF_RES;

	cFFRate = (DecCmdChannelSetFFRate *)&pIocData->u.fwCmd.cmd;
	cFFRate->command		= eCMD_C011_DEC_CHAN_SET_FF_RATE;
	cFFRate->sequence	= ++Ctx->fwcmdseq;
	cFFRate->channelId	= Ctx->OpenRsp.channelId;
	cFFRate->rate   = Rate;	
	
	if( (sts=DtsDrvCmd(Ctx,BCM_IOC_FW_CMD,1,pIocData,FALSE)) != BC_STS_SUCCESS){
		DebugLog_Trace(LDIL_DBG,"DtsFWSetFFRate: Ioctl failed: %d\n",sts);
		DtsRelIoctlData(Ctx,pIocData);
		return sts;
	}

	rspFFRate = (DecRspChannelSetFFRate*)&pIocData->u.fwCmd.rsp;
	if(rspFFRate->status){
		DebugLog_Trace(LDIL_DBG,"DtsFWSetFFRate: SetFFRate Failed %u\n",rspFFRate->status);
		DtsRelIoctlData(Ctx,pIocData);
		return BC_STS_FW_CMD_ERR;
	}

	DtsRelIoctlData(Ctx,pIocData);

	return BC_STS_SUCCESS;

}

DRVIFLIB_INT_API BC_STATUS 
DtsFWSetSlowMotionRate(
	HANDLE hDevice,
	uint32_t	Rate	
	)
{
	BC_STATUS	sts = BC_STS_SUCCESS;

	DecCmdChannelSetSlowMotionRate	*cSMRate;

	DecRspChannelSetSlowMotionRate	*rspSMRate;

	DTS_LIB_CONTEXT		*Ctx = NULL;
	BC_IOCTL_DATA *pIocData = NULL;

	DTS_GET_CTX(hDevice,Ctx);

	if(!(pIocData = DtsAllocIoctlData(Ctx)))
		return BC_STS_INSUFF_RES;

	cSMRate = (DecCmdChannelSetSlowMotionRate *)&pIocData->u.fwCmd.cmd;
	cSMRate->command = eCMD_C011_DEC_CHAN_SET_SLOWM_RATE;
	cSMRate->sequence = ++Ctx->fwcmdseq;
	cSMRate->channelId = Ctx->OpenRsp.channelId;
	cSMRate->rate = Rate;	
	
	if( (sts=DtsDrvCmd(Ctx,BCM_IOC_FW_CMD,1,pIocData,FALSE)) != BC_STS_SUCCESS){
		DebugLog_Trace(LDIL_DBG,"DtsFWSetFFRate: Ioctl failed: %d\n",sts);
		DtsRelIoctlData(Ctx,pIocData);
		return sts;
	}

	rspSMRate = (DecRspChannelSetSlowMotionRate*)&pIocData->u.fwCmd.rsp;
	if(rspSMRate->status){
		DebugLog_Trace(LDIL_DBG,"DtsFWSetFFRate: SetSMRate Failed %u\n",rspSMRate->status);
		DtsRelIoctlData(Ctx,pIocData);
		return BC_STS_FW_CMD_ERR;
	}

	DtsRelIoctlData(Ctx,pIocData);

	return BC_STS_SUCCESS;

}

DRVIFLIB_INT_API BC_STATUS 
DtsFWSetSkipPictureMode(
	HANDLE hDevice,
	uint32_t	SkipMode	
	)
{
	BC_STATUS	sts = BC_STS_SUCCESS;

	DecCmdChannelSetSkipPictureMode	*cSkipPictureMode;

	DecRspChannelSetSkipPictureMode	*rspSkipPictureMode;

	DTS_LIB_CONTEXT		*Ctx = NULL;
	BC_IOCTL_DATA *pIocData = NULL;

	DTS_GET_CTX(hDevice,Ctx);

	if(!(pIocData = DtsAllocIoctlData(Ctx)))
		return BC_STS_INSUFF_RES;

	cSkipPictureMode = (DecCmdChannelSetSkipPictureMode *)&pIocData->u.fwCmd.cmd;
	cSkipPictureMode->command		= eCMD_C011_DEC_CHAN_SET_SKIP_PIC_MODE;
	cSkipPictureMode->sequence	= ++Ctx->fwcmdseq;
	cSkipPictureMode->channelId	= Ctx->OpenRsp.channelId;
	cSkipPictureMode->skipMode   = (eC011_SKIP_PIC_MODE)SkipMode;	
	
	if( (sts=DtsDrvCmd(Ctx,BCM_IOC_FW_CMD,1,pIocData,FALSE)) != BC_STS_SUCCESS){
		DebugLog_Trace(LDIL_DBG,"DtsFWSetSkipPictureMode: Ioctl failed: %d\n",sts);
		DtsRelIoctlData(Ctx,pIocData);
		return sts;
	}

	rspSkipPictureMode = (DecRspChannelSetSkipPictureMode*)&pIocData->u.fwCmd.rsp;
	if(rspSkipPictureMode->status){
		DebugLog_Trace(LDIL_DBG,"DtsFWSetSkipPictureMode: SkipPictureMode Failed %u\n",rspSkipPictureMode->status);
		DtsRelIoctlData(Ctx,pIocData);
		return BC_STS_FW_CMD_ERR;
	}

	DtsRelIoctlData(Ctx,pIocData);

	return BC_STS_SUCCESS;

}

DRVIFLIB_INT_API BC_STATUS 
DtsFWFrameAdvance(
    HANDLE  hDevice	
	)
{
	BC_STATUS	sts = BC_STS_SUCCESS;

	DecCmdChannelFrameAdvance	*cFrameAdvance;

	DecRspChannelFrameAdvance	*rspFrameAdvance;

	DTS_LIB_CONTEXT		*Ctx = NULL;
	BC_IOCTL_DATA *pIocData = NULL;

	DTS_GET_CTX(hDevice,Ctx);

	if(!(pIocData = DtsAllocIoctlData(Ctx)))
		return BC_STS_INSUFF_RES;

	cFrameAdvance = (DecCmdChannelFrameAdvance *)&pIocData->u.fwCmd.cmd;
	cFrameAdvance->command		= eCMD_C011_DEC_CHAN_FRAME_ADVANCE;
	cFrameAdvance->sequence	= ++Ctx->fwcmdseq;
	cFrameAdvance->channelId	= Ctx->OpenRsp.channelId;	
	
	if( (sts=DtsDrvCmd(Ctx,BCM_IOC_FW_CMD,1,pIocData,FALSE)) != BC_STS_SUCCESS){
		DebugLog_Trace(LDIL_DBG,"DtsFWFrameAdvance: Ioctl failed: %d\n",sts);
		DtsRelIoctlData(Ctx,pIocData);
		return sts;
	}

	rspFrameAdvance = (DecRspChannelFrameAdvance*)&pIocData->u.fwCmd.rsp;
	if(rspFrameAdvance->status){
		DebugLog_Trace(LDIL_DBG,"DtsFWFrameAdvance: Failed %u\n",rspFrameAdvance->status);
		DtsRelIoctlData(Ctx,pIocData);
		return BC_STS_FW_CMD_ERR;
	}

	DtsRelIoctlData(Ctx,pIocData);

	return BC_STS_SUCCESS;
}

DRVIFLIB_INT_API BC_STATUS 
DtsFWSetContentKeys(
	HANDLE hDevice,
	uint8_t	*buffer,
	uint32_t	Length,
	uint32_t	flags
	)
{
	BC_STATUS	sts = BC_STS_SUCCESS;

	DecCmdSetContentKey	*cKeys;
	DecRspSetContentKey	*rspcKeys;
	uint8_t	*temp=NULL;

	DTS_LIB_CONTEXT		*Ctx = NULL;
	BC_IOCTL_DATA *pIocData = NULL;

	DTS_GET_CTX(hDevice,Ctx);

	if(!buffer || (Length > ((BC_MAX_FW_CMD_BUFF_SZ*sizeof(uint32_t)) - 16) ) ){
		return BC_STS_INV_ARG;
	}

	if(!(pIocData = DtsAllocIoctlData(Ctx)))
		return BC_STS_INSUFF_RES;

	cKeys = (DecCmdSetContentKey *)&pIocData->u.fwCmd.cmd;
	cKeys->command = eCMD_C011_DEC_CHAN_SET_CONTENT_KEY;
	cKeys->sequence = ++Ctx->fwcmdseq;
	cKeys->channelId = Ctx->OpenRsp.channelId;
	cKeys->flags = flags;
	if(Ctx->FixFlags & DTS_ADAPTIVE_OUTPUT_PER)
		cKeys->flags |= BC_BIT(17);
	temp = ((uint8_t *)cKeys) + 16;
	memcpy(temp, buffer, Length);
	
	if( (sts=DtsDrvCmd(Ctx,BCM_IOC_FW_CMD,1,pIocData,FALSE)) != BC_STS_SUCCESS){
		DebugLog_Trace(LDIL_DBG,"DtsFWSetFFRate: Ioctl failed: %d\n",sts);
		DtsRelIoctlData(Ctx,pIocData);
		return sts;
	}

	rspcKeys = (DecRspSetContentKey*)&pIocData->u.fwCmd.rsp;
	if(rspcKeys->status){
		DebugLog_Trace(LDIL_DBG,"DtsFWSetContentKeys: Failed %u\n",rspcKeys->status);
		DtsRelIoctlData(Ctx,pIocData);
		return BC_STS_FW_CMD_ERR;
	}

	DtsRelIoctlData(Ctx,pIocData);

	return BC_STS_SUCCESS;
}

DRVIFLIB_INT_API BC_STATUS 
DtsFWSetSessionKeys(
	HANDLE hDevice,
	uint8_t	*buffer,
	uint32_t	Length,
	uint32_t	flags
	)
{
	BC_STATUS	sts = BC_STS_SUCCESS;

	DecCmdSetSessionKey	*sKey;
	DecRspSetSessionKey	*rspsKey;
	uint8_t	*temp=NULL;

	DTS_LIB_CONTEXT		*Ctx = NULL;
	BC_IOCTL_DATA *pIocData = NULL;

	DTS_GET_CTX(hDevice,Ctx);

	if(!buffer || (Length > ((BC_MAX_FW_CMD_BUFF_SZ*sizeof(uint32_t)) - 16) ) ){
		return BC_STS_INV_ARG;
	}

	if(!(pIocData = DtsAllocIoctlData(Ctx)))
		return BC_STS_INSUFF_RES;

	sKey = (DecCmdSetSessionKey *)&pIocData->u.fwCmd.cmd;
	sKey->command = eCMD_C011_DEC_CHAN_SET_SESSION_KEY;
	sKey->sequence = ++Ctx->fwcmdseq;
	sKey->channelId = Ctx->OpenRsp.channelId;
	sKey->flags = flags;
	temp = ((uint8_t *)sKey) + 16;
	memcpy(temp, buffer, Length);
	
	if( (sts=DtsDrvCmd(Ctx,BCM_IOC_FW_CMD,1,pIocData,FALSE)) != BC_STS_SUCCESS){
		DebugLog_Trace(LDIL_DBG,"DtsFWSetFFRate: Ioctl failed: %d\n",sts);
		DtsRelIoctlData(Ctx,pIocData);
		return sts;
	}

	rspsKey = (DecRspSetSessionKey*)&pIocData->u.fwCmd.rsp;
	if(rspsKey->status){
		DebugLog_Trace(LDIL_DBG,"DtsFWSetSessionKey: Failed %u\n",rspsKey->status);
		DtsRelIoctlData(Ctx,pIocData);
		return BC_STS_FW_CMD_ERR;
	}

	DtsRelIoctlData(Ctx,pIocData);

	return BC_STS_SUCCESS;
}

BC_STATUS
DtsFormatChangeAck(HANDLE hDevice, uint32_t flags)
{
	BC_STATUS	sts = BC_STS_SUCCESS;

	DecCmdFormatChangeAck		*Ack;
	DecRspFormatChangeAck		*rspAck;
	DTS_LIB_CONTEXT				*Ctx = NULL;
	BC_IOCTL_DATA				*pIocData = NULL;

	DTS_GET_CTX(hDevice,Ctx);
	if (!(pIocData = DtsAllocIoctlData(Ctx)))
		return BC_STS_INSUFF_RES;

	Ack = (DecCmdFormatChangeAck *) &pIocData->u.fwCmd.cmd;
	Ack->command = eCMD_C011_DEC_CHAN_FMT_CHANGE_ACK;
	Ack->sequence = ++Ctx->fwcmdseq;
	Ack->channelId = Ctx->OpenRsp.channelId;
	Ack->flags = flags;
	
	if( (sts=DtsDrvCmd(Ctx,BCM_IOC_FW_CMD,1,pIocData,FALSE)) != BC_STS_SUCCESS){
		DebugLog_Trace(LDIL_DBG,"DtsFormatChangeAck: Ioctl failed: %d\n",sts);
		DtsRelIoctlData(Ctx,pIocData);
		return sts;
	}

	rspAck = (DecRspFormatChangeAck*) &pIocData->u.fwCmd.rsp;
	if(rspAck->status){
		DebugLog_Trace(LDIL_DBG,"DtsFormatChangeAck: Failed %u\n",rspAck->status);
		DtsRelIoctlData(Ctx,pIocData);
		return BC_STS_FW_CMD_ERR;
	}

	DtsRelIoctlData(Ctx,pIocData);
	return BC_STS_SUCCESS;
}

DRVIFLIB_INT_API BC_STATUS 
DtsFWDrop(
	HANDLE hDevice,
	uint32_t	Pictures
	)
{
	BC_STATUS	sts = BC_STS_SUCCESS;

	DecCmdChannelDrop	*cDrop;

	DecRspChannelDrop	*rspDrop;

	DTS_LIB_CONTEXT		*Ctx = NULL;
	BC_IOCTL_DATA *pIocData = NULL;

	DTS_GET_CTX(hDevice,Ctx);

	if(!(pIocData = DtsAllocIoctlData(Ctx)))
		return BC_STS_INSUFF_RES;

	cDrop = (DecCmdChannelDrop *)&pIocData->u.fwCmd.cmd;
	cDrop->command      = eCMD_C011_DEC_CHAN_DROP;
	cDrop->sequence	    = ++Ctx->fwcmdseq;
	cDrop->channelId    = Ctx->OpenRsp.channelId;
    cDrop->numPicDrop   = Pictures;
	cDrop->dropType     = eC011_DROP_TYPE_DECODER; // Do not skip reference types.
	
	if( (sts=DtsDrvCmd(Ctx,BCM_IOC_FW_CMD,1,pIocData,FALSE)) != BC_STS_SUCCESS){
		DebugLog_Trace(LDIL_DBG,"DtsFWDrop: Ioctl failed: %d\n",sts);
		DtsRelIoctlData(Ctx,pIocData);
		return sts;
	}

	rspDrop = (DecRspChannelDrop*)&pIocData->u.fwCmd.rsp;
	if(rspDrop->status){
		DebugLog_Trace(LDIL_DBG,"DtsFWDrop: Drop Failed %u\n",rspDrop->status);
		DtsRelIoctlData(Ctx,pIocData);
		return BC_STS_FW_CMD_ERR;
	}

	DtsRelIoctlData(Ctx,pIocData);

	return BC_STS_SUCCESS;
}
