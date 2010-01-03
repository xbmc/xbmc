/********************************************************************
 * Copyright(c) 2006-2009 Broadcom Corporation.
 *
 *  Name: libcrystalhd_if.cpp
 *
 *  Description: Driver Interface API.
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

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "libcrystalhd_if.h"
#include "libcrystalhd_int_if.h"
#include "version_lnx.h"
#include "libcrystalhd_fwcmds.h"
#include "libcrystalhd_priv.h"

static BC_STATUS DtsSetupHardware(HANDLE hDevice, BOOL IgnClkChk)
{
	BC_STATUS	sts = BC_STS_SUCCESS;
	DTS_LIB_CONTEXT		*Ctx;

	DTS_GET_CTX(hDevice,Ctx);

	if(!IgnClkChk)
	{
		if(Ctx->DevId == BC_PCI_DEVID_LINK)
		{
			if((DtsGetOPMode() & 0x08) && 
			   (DtsGetHwInitSts() != BC_DIL_HWINIT_IN_PROGRESS))
			{
				return BC_STS_SUCCESS;
			}
		}
		
	}
	 
	if(Ctx->DevId == BC_PCI_DEVID_LINK)
	{
		DebugLog_Trace(LDIL_ERR,"DtsSetupHardware: DtsPushAuthFwToLink\n");
		sts = DtsPushAuthFwToLink(hDevice,NULL);
	}

	if(sts != BC_STS_SUCCESS){
		return sts;
	}
	
	/* Initialize Firmware interface */
	sts = DtsFWInitialize(hDevice,0);
	if(sts != BC_STS_SUCCESS ){
		return sts;
	}

	return sts;
}


static BC_STATUS DtsReleaseChannel(HANDLE  hDevice,uint32_t ChannelID, BOOL Stop)
{
	BC_STATUS	sts = BC_STS_SUCCESS;


	if(Stop){
		sts = DtsFWStopVideo(hDevice, ChannelID, TRUE);
		if(sts != BC_STS_SUCCESS){
			DebugLog_Trace(LDIL_DBG,"DtsReleaseChannel: StopVideoFailed Ignoring error\n");
		}
	}
	sts = DtsFWCloseChannel(hDevice, ChannelID);
	if(sts != BC_STS_SUCCESS){
		DebugLog_Trace(LDIL_DBG,"DtsReleaseChannel: DtsFWCloseChannel Failed\n");
	}

	return sts;
}

static BC_STATUS DtsRecoverableDecOpen(HANDLE  hDevice,uint32_t StreamType)
{
	BC_STATUS	sts = BC_STS_SUCCESS;
	uint32_t			retry = 3;

	DTS_LIB_CONTEXT		*Ctx = NULL;

	DTS_GET_CTX(hDevice,Ctx);
	
	while(retry--){

		sts = DtsFWOpenChannel(hDevice, StreamType, 0);
		if(sts == BC_STS_SUCCESS){
			if(Ctx->OpenRsp.channelId == 0){
				break;
			}else{
				DebugLog_Trace(LDIL_DBG,"DtsFWOpenChannel: ChannelID leakage..\n");
				/* First Release the Current Channel.*/
				DtsReleaseChannel(hDevice,Ctx->OpenRsp.channelId, FALSE);
				/* Fall through to release the previous Channel */
				sts = BC_STS_FW_CMD_ERR;
			}
		}
	
		if((sts == BC_STS_TIMEOUT) || (retry == 1) ){
			/* Do Full initialization */
			sts = DtsSetupHardware(hDevice,TRUE);
			if(sts != BC_STS_SUCCESS)
				break;
			/* Setup The Clock Again */
			sts = DtsSetVideoClock(hDevice,0);
			if(sts != BC_STS_SUCCESS )
				break;

		}else{
			sts = DtsReleaseChannel(hDevice,0,TRUE);
			if(sts != BC_STS_SUCCESS)
				break;
		}
	}

	return sts;

}

//===================================Externs ============================================
#ifdef _USE_SHMEM_
extern BOOL glob_mode_valid;
#endif

DRVIFLIB_API BC_STATUS 
DtsDeviceOpen(
    HANDLE	*hDevice,
	uint32_t		mode
    )
{
	int 		drvHandle=1;
	BC_STATUS	Sts=BC_STS_SUCCESS;
	uint32_t globMode = 0, cnt = 100;
	uint8_t	nTry=1;
	uint32_t		VendorID, DeviceID, RevID, FixFlags, drvMode;
	uint32_t		drvVer, dilVer;
	uint32_t		fwVer, decVer, hwVer;
#ifdef _USE_SHMEM_
	int shmid=0;
#endif
	
	DebugLog_Trace(LDIL_DBG,"Running DIL (%d.%d.%d) Version\n",
		DIL_MAJOR_VERSION,DIL_MINOR_VERSION,DIL_REVISION );

	FixFlags = mode;
	mode &= 0xFF;

	/* For External API case, we support only Plyaback mode. */
	if( !(BC_DTS_DEF_CFG & BC_EN_DIAG_MODE) && (mode != DTS_PLAYBACK_MODE) ){
		DebugLog_Trace(LDIL_ERR,"DtsDeviceOpen: mode %d not supported\n",mode);
		return BC_STS_INV_ARG;
	}
#ifdef _USE_SHMEM_
	Sts = DtsCreateShMem(&shmid);	
	if(BC_STS_SUCCESS !=Sts)
		return Sts;	


/*	Sts = DtsGetDilShMem(shmid);
	if(BC_STS_SUCCESS !=Sts)
		return Sts;*/

	if(!glob_mode_valid) {
		globMode = DtsGetOPMode();
		if(globMode&4) {
			globMode&=4;
		}
		DebugLog_Trace(LDIL_DBG,"DtsDeviceOpen:New  globmode is %d \n",globMode);
	}
	else{
		globMode = DtsGetOPMode();
	}
#else
	globMode = DtsGetOPMode();
#endif


	if(((globMode & 0x3) && (mode != DTS_MONITOR_MODE)) ||
	   ((globMode & 0x4) && (mode == DTS_MONITOR_MODE)) ||
	   ((globMode & 0x8) && (mode == DTS_HWINIT_MODE))){
		DebugLog_Trace(LDIL_DBG,"DtsDeviceOpen: mode %d already opened\n",mode);
#ifdef _USE_SHMEM_
		DtsDelDilShMem();
#endif
		return BC_STS_DEC_EXIST_OPEN;
	}

	if(mode == DTS_PLAYBACK_MODE){
		while(cnt--){
			if(DtsGetHwInitSts() != BC_DIL_HWINIT_IN_PROGRESS)
				break;
			bc_sleep_ms(100);
		}
		if(!cnt){
			return BC_STS_TIMEOUT;
		}
	}else if(mode == DTS_HWINIT_MODE){
		DtsSetHwInitSts(BC_DIL_HWINIT_IN_PROGRESS);
	}

	drvHandle =open(CRYSTALHD_API_DEV_NAME, O_RDWR);
	if(drvHandle < 0)
	{
		DebugLog_Trace(LDIL_ERR,"DtsDeviceOpen: Create File Failed\n");
		return BC_STS_ERROR;
	}


	/* Initialize Internal Driver interfaces.. */
	if( (Sts = DtsInitInterface(drvHandle,hDevice, mode)) != BC_STS_SUCCESS){
		DebugLog_Trace(LDIL_ERR,"DtsDeviceOpen: Interface Init Failed:%x\n",Sts);
		return Sts;
	}
	
	/* 
	 * We have to specify the mode to the driver.
	 * So the driver can cleanup only in case of 
	 * playback/Diag mode application close.
	 */
	if ((Sts = DtsGetVersion(*hDevice, &drvVer, &dilVer)) != BC_STS_SUCCESS) {
		DebugLog_Trace(LDIL_DBG,"Get drv ver failed\n");
		return Sts;
	}

	/* If driver minor version is more than 13, enable DTS_SKIP_TX_CHK_CPB feature */
	if (FixFlags & DTS_SKIP_TX_CHK_CPB) {
		if (((drvVer >> 16) & 0xFF) > 13) 
			FixFlags |= DTS_SKIP_TX_CHK_CPB;
	}

	if (FixFlags & DTS_ADAPTIVE_OUTPUT_PER) {

		if (!DtsGetFWVersion(*hDevice, &fwVer, &decVer, &hwVer, (char*)FWBINFILE_LNK, 0)) {
			if (fwVer >= ((14 << 16) | (8 << 8) | (1)))		// 2.14.8.1 (ignore 2)
				FixFlags |= DTS_ADAPTIVE_OUTPUT_PER;
			else
				FixFlags &= (~DTS_ADAPTIVE_OUTPUT_PER);
		}
	}

	/* If driver minor version is newer, send the fullMode */
	if (((drvVer >> 16) & 0xFF) > 10) drvMode = FixFlags; else drvMode = mode;

	if( (Sts = DtsNotifyOperatingMode(*hDevice,drvMode)) != BC_STS_SUCCESS){
		DebugLog_Trace(LDIL_DBG,"Notify Operating Mode Failed\n");
		return Sts;
	}

	if( (Sts = DtsGetHwType(*hDevice,&DeviceID,&VendorID,&RevID))!=BC_STS_SUCCESS){
		DebugLog_Trace(LDIL_DBG,"Get Hardware Type Failed\n");
		return Sts;
	}

	/* Setup Hardware Specific Configuration */
	DtsSetupConfig(DtsGetContext(*hDevice), DeviceID, RevID, FixFlags);

	if(mode == DTS_PLAYBACK_MODE){
		globMode |= 0x1;
	} else if(mode == DTS_DIAG_MODE){
		globMode |= 0x2;
	} else if(mode == DTS_MONITOR_MODE) {
		globMode |= 0x4;
	} else if(mode == DTS_HWINIT_MODE) {
		globMode |= 0x8;
	} else {
		globMode = 0;
	}

	DtsSetOPMode(globMode);
 
	if((mode == DTS_PLAYBACK_MODE)||(mode == DTS_HWINIT_MODE))
	{	
		while(nTry--)
		{
			Sts = 	DtsSetupHardware(*hDevice, FALSE);	
			if(Sts == BC_STS_SUCCESS)
			{	
				DebugLog_Trace(LDIL_DBG,"DtsSetupHardware: Success\n");
				break;
			}
			else 
			{	
				DebugLog_Trace(LDIL_DBG,"DtsSetupHardware: Failed\n");
				bc_sleep_ms(100);				
			}
		}
		if(Sts != BC_STS_SUCCESS )
		{
			DtsReleaseInterface(DtsGetContext(*hDevice));
		}		
	}
	
	if(mode == DTS_HWINIT_MODE){
		DtsSetHwInitSts(0);
	}

	return Sts;
}

DRVIFLIB_API BC_STATUS 
DtsDeviceClose(
    HANDLE hDevice
    )
{
	DTS_LIB_CONTEXT		*Ctx;
	uint32_t globMode = 0;

	DTS_GET_CTX(hDevice,Ctx);

	if(Ctx->State != BC_DEC_STATE_INVALID){
		DebugLog_Trace(LDIL_DBG,"DtsDeviceClose: closing decoder ....\n");
		DtsCloseDecoder(hDevice);

	}

	DtsCancelFetchOutInt(Ctx);
	/* Unmask the mode */
	globMode = DtsGetOPMode( );
	if(Ctx->OpMode == DTS_PLAYBACK_MODE){
		globMode &= (~0x1);
	} else if(Ctx->OpMode == DTS_DIAG_MODE){
		globMode &= (~0x2);
	} else if(Ctx->OpMode == DTS_MONITOR_MODE) {
		globMode &= (~0x4);
	} else if(Ctx->OpMode == DTS_HWINIT_MODE) {
		globMode &= (~0x8);
	} else {
		globMode = 0;
	}
	DtsSetOPMode(globMode);
	


	return DtsReleaseInterface(Ctx);
	
}

DRVIFLIB_API BC_STATUS 
DtsGetVersion(
    HANDLE  hDevice,
    uint32_t     *DrVer,
    uint32_t     *DilVer
	)
{
	BC_VERSION_INFO *pVerInfo;
	BC_IOCTL_DATA *pIocData = NULL;
	DTS_LIB_CONTEXT		*Ctx = NULL;
	BC_STATUS	sts = BC_STS_SUCCESS;


	DTS_GET_CTX(hDevice,Ctx);
	
	if(!(pIocData = DtsAllocIoctlData(Ctx)))
		return BC_STS_INSUFF_RES;

	pVerInfo = 	&pIocData->u.VerInfo;

	if( (sts=DtsDrvCmd(Ctx,BCM_IOC_GET_VERSION,0,pIocData,FALSE)) != BC_STS_SUCCESS){
		DtsRelIoctlData(Ctx,pIocData);
		DebugLog_Trace(LDIL_DBG,"DtsGetVersion: Ioctl failed: %d\n",sts);
		return sts;
	}

	*DrVer  = (pVerInfo->DriverMajor << 24) | (pVerInfo->DriverMinor<<16) | pVerInfo->DriverRevision;
	*DilVer = (DIL_MAJOR_VERSION <<24)| (DIL_MINOR_VERSION<<16) | DIL_REVISION;

	DtsRelIoctlData(Ctx,pIocData);

	return BC_STS_SUCCESS;
}

#define MAX_BIN_FILE_SZ 	0x300000

DRVIFLIB_API BC_STATUS 
DtsGetFWVersionFromFile(
    HANDLE  hDevice,
    uint32_t     *StreamVer,
	uint32_t		*DecVer,
	char	*fname
	)
{

	
	BC_STATUS sts = BC_STS_SUCCESS;
	uint8_t *buf;
	//uint32_t buflen=0;
	uint32_t sizeRead=0;
	uint32_t err=0;
	FILE *fhnd =NULL;
	char fwfile[MAX_PATH+1];

	DTS_LIB_CONTEXT		*Ctx = NULL;
	
	DTS_GET_CTX(hDevice,Ctx);	
	
	sts = DtsGetDILPath(hDevice, fwfile, sizeof(fwfile));

	if(sts != BC_STS_SUCCESS){
		return sts;
	}

	if(fname){
		strncat(fwfile,(const char*)fname,sizeof(fwfile));
	}else{
		//strncat(fwfile,"/",1);
		strncat(fwfile,FWBINFILE_LNK,sizeof(FWBINFILE_LNK));
	}
	



	
	if(!StreamVer){
		DebugLog_Trace(LDIL_DBG,"\nDtsGetFWVersionFromFile: Null Pointer argument");
		return BC_STS_INSUFF_RES;
	}

	fhnd = fopen((const char *)fwfile, "rb");
	
 	

	if(fhnd == NULL ){
		DebugLog_Trace(LDIL_DBG,"DtsGetFWVersionFromFile:Failed to Open file Err\n");
		return BC_STS_INSUFF_RES;
	}
	

	buf=(uint8_t *)malloc(MAX_BIN_FILE_SZ);
	if(buf==NULL) {
		DebugLog_Trace(LDIL_DBG,"DtsGetFWVersionFromFile:Failed to allocate memory\n");
		return BC_STS_INSUFF_RES;
	}

	err = fread(buf, sizeof(uint8_t), MAX_BIN_FILE_SZ, fhnd);
	if(!err)
		 {
			sizeRead = err;
	}

	if((err==0)&&(errno!=0)) 
	{ 
		DebugLog_Trace(LDIL_DBG,"DtsGetFWVersionFromFile:Failed to read bin file %d\n",errno);

		if(buf)free(buf);

		fclose(fhnd);
		return BC_STS_ERROR;
	}
	

	
	
	uint8_t *pSearchStr = &buf[0x4000];
	*StreamVer =0;
	for(uint32_t i=0; i <(sizeRead-0x4000);i++){
		if(NULL != strstr((char *)pSearchStr,(const char *)"Media_PC_FW_Rev")){
			//The actual FW versions are at searchstring - 4 bytes.	
			*StreamVer = ((*(pSearchStr-4)) << 16) | 
						 ((*(pSearchStr-3))<<8) | 
						 (*(pSearchStr-2));
			break;
		}
		pSearchStr++;
	}
	

	if(buf)
		free(buf);
	if(fhnd)
		fhnd=NULL;
	if(*StreamVer ==0)
		return BC_STS_ERROR;
	else
		return BC_STS_SUCCESS;

	
}

DRVIFLIB_API BC_STATUS 
DtsGetFWVersion(
    HANDLE  hDevice,
    uint32_t     *StreamVer,
	uint32_t		*DecVer,
    uint32_t     *HwVer,
	char	*fname,
	uint32_t	     flag
	)
{
	BC_STATUS sts;
	if(flag) //get runtime version by issuing a FWcmd
	{
		sts = DtsFWVersion(hDevice,StreamVer,DecVer,HwVer);
			if(sts == BC_STS_SUCCESS)
			{
				DebugLog_Trace(LDIL_DBG,"FW Version: Stream: %x, Dec: %x, HW:%x\n",*StreamVer,*DecVer,*HwVer);
			}
			else
			{
				DebugLog_Trace(LDIL_DBG,"DtsGetFWVersion: failed to get version fromFW at runtime: %d\n",sts);			
				return sts;
			}
	
	}
	else //read the stream arc version from binary
	{
		sts = DtsGetFWVersionFromFile(hDevice,StreamVer,DecVer,fname);
		if(sts == BC_STS_SUCCESS)
		{
			DebugLog_Trace(LDIL_DBG,"FW Version: Stream: %x",*StreamVer);
			if(DecVer !=NULL)
				DebugLog_Trace(LDIL_DBG," Dec: %x\n",*DecVer);
		}
		else
		{
			DebugLog_Trace(LDIL_DBG,"DtsGetFWVersion: failed to get version from FW bin file: %d\n",sts);			
			return sts;
		}

	}
	
	return BC_STS_SUCCESS;

}


DRVIFLIB_API BC_STATUS 
DtsOpenDecoder(HANDLE  hDevice, uint32_t StreamType)
{
	BC_STATUS sts = BC_STS_SUCCESS;
	DTS_LIB_CONTEXT	*Ctx = NULL;

	DTS_GET_CTX(hDevice,Ctx);

	if (Ctx->State != BC_DEC_STATE_INVALID) {
		DebugLog_Trace(LDIL_DBG, "DtsOpenDecoder: Channel Already Open\n");
		return BC_STS_BUSY;
	}

	Ctx->LastPicNum = 0;
	Ctx->eosCnt = 0;
	Ctx->FlushIssued = FALSE;
	Ctx->CapState = 0;
	Ctx->picWidth = 0;
	Ctx->picHeight = 0;
	
	sts = DtsSetVideoClock(hDevice,0);
	if (sts != BC_STS_SUCCESS)
	{
		return sts;
	}

	// FIX_ME to support other stream types.
	sts = DtsSetTSMode(hDevice,0);
	if(sts != BC_STS_SUCCESS )
	{
		return sts;
	}

	sts = DtsRecoverableDecOpen(hDevice,StreamType);
	if(sts != BC_STS_SUCCESS )
		return sts;

	sts = DtsFWSetVideoInput(hDevice);
	if(sts != BC_STS_SUCCESS )
	{
		return sts;
	}

	Ctx->State = BC_DEC_STATE_OPEN;


	return BC_STS_SUCCESS;
}

DRVIFLIB_API BC_STATUS 
DtsStartDecoder(
    HANDLE  hDevice
    )
{
	BC_STATUS	sts = BC_STS_SUCCESS;

	DTS_LIB_CONTEXT		*Ctx = NULL;
	DTS_GET_CTX(hDevice,Ctx);

	if( (Ctx->State == BC_DEC_STATE_START) || (Ctx->State == BC_DEC_STATE_INVALID) )
	{
		DebugLog_Trace(LDIL_DBG,"DtsStartDecoder: Decoder Not in correct State\n");
		return BC_STS_ERR_USAGE;
	}

	if(Ctx->VidParams.Progressive){
		sts = DtsSetProgressive(hDevice,0);
		if(sts != BC_STS_SUCCESS )
			return sts;
	}

	sts = DtsFWActivateDecoder(hDevice);
	if(sts != BC_STS_SUCCESS )
	{
		return sts;
	}
	sts = DtsFWStartVideo(hDevice,
							Ctx->VidParams.VideoAlgo,
							Ctx->VidParams.FGTEnable,
							Ctx->VidParams.MetaDataEnable,
							Ctx->VidParams.Progressive,
							Ctx->VidParams.OptFlags);
	if(sts != BC_STS_SUCCESS )
		return sts;
	
	Ctx->State = BC_DEC_STATE_START;

	return sts;
}

DRVIFLIB_API BC_STATUS 
DtsCloseDecoder(
    HANDLE  hDevice
    )
{
	BC_STATUS	sts = BC_STS_SUCCESS;

	DTS_LIB_CONTEXT		*Ctx = NULL;
	DTS_GET_CTX(hDevice,Ctx);

	if( Ctx->State == BC_DEC_STATE_INVALID){
		DebugLog_Trace(LDIL_DBG,"DtsCloseDecoder: Decoder Not Opened Ignoring..\n");
		return BC_STS_SUCCESS;
	}

	sts = DtsFWCloseChannel(hDevice,Ctx->OpenRsp.channelId);
	
	/*if(sts != BC_STS_SUCCESS )
	{
		return sts;
	}*/

	Ctx->State = BC_DEC_STATE_INVALID;
	
	Ctx->LastPicNum = 0;
	Ctx->eosCnt = 0;
	Ctx->FlushIssued = FALSE;

	/* Clear all pending lists.. */
	DtsClrPendMdataList(Ctx);

	/* Close the Input dump File */
	DumpInputSampleToFile(NULL,0);

	

	return sts;
}

DRVIFLIB_API BC_STATUS 
DtsSetVideoParams(
    HANDLE  hDevice,
	uint32_t		videoAlg,
	BOOL	FGTEnable,
	BOOL	MetaDataEnable,
	BOOL	Progressive,
    uint32_t     OptFlags
	)
{
	DTS_LIB_CONTEXT		*Ctx = NULL;
	
	DTS_GET_CTX(hDevice,Ctx);

	if(Ctx->State != BC_DEC_STATE_OPEN){
		DebugLog_Trace(LDIL_DBG,"DtsOpenDecoder: Channel Already Open\n");
		return BC_STS_ERR_USAGE;
	}
	Ctx->VidParams.VideoAlgo = videoAlg;
	Ctx->VidParams.FGTEnable = FGTEnable;
	Ctx->VidParams.MetaDataEnable = MetaDataEnable;
	Ctx->VidParams.Progressive = Progressive;
	//Ctx->VidParams.Reserved = rsrv;
	//Ctx->VidParams.FrameRate = FrameRate;
	Ctx->VidParams.OptFlags = OptFlags;

	return BC_STS_SUCCESS;
}

DRVIFLIB_API BC_STATUS 
DtsGetVideoParams(
    HANDLE  hDevice,
	uint32_t		*videoAlg,
	BOOL	*FGTEnable,
	BOOL	*MetaDataEnable,
	BOOL	*Progressive,
    uint32_t     rsrv
	)
{
	DTS_LIB_CONTEXT		*Ctx = NULL;

	DTS_GET_CTX(hDevice,Ctx);

	if(!videoAlg || !FGTEnable || !MetaDataEnable || !Progressive)
		return BC_STS_INV_ARG;
	
	if(Ctx->State != BC_DEC_STATE_OPEN){
		DebugLog_Trace(LDIL_DBG,"DtsOpenDecoder: Channel Already Open\n");
		return BC_STS_ERR_USAGE;
	}

	 *videoAlg = Ctx->VidParams.VideoAlgo;
	 *FGTEnable = Ctx->VidParams.FGTEnable;
	 *MetaDataEnable = Ctx->VidParams.MetaDataEnable;
	 *Progressive = Ctx->VidParams.Progressive;

	 return BC_STS_SUCCESS;

}


DRVIFLIB_API BC_STATUS 
DtsStopDecoder(
    HANDLE  hDevice
	)
{
	BC_STATUS	sts = BC_STS_SUCCESS;

	DTS_LIB_CONTEXT		*Ctx = NULL;
	DTS_GET_CTX(hDevice,Ctx);


	if( (Ctx->State != BC_DEC_STATE_START) && (Ctx->State != BC_DEC_STATE_PAUSE) ) {
		DebugLog_Trace(LDIL_DBG,"DtsStopDecoder: Decoder Not Started\n");
		return BC_STS_ERR_USAGE;
	}

	DtsCancelFetchOutInt(Ctx);

	sts = DtsFWStopVideo(hDevice,Ctx->OpenRsp.channelId, FALSE);
	if(sts != BC_STS_SUCCESS )
	{
		return sts;
	}

	Ctx->State = BC_DEC_STATE_STOP;

	return sts;
}

DRVIFLIB_API BC_STATUS 
	DtsPauseDecoder(
    HANDLE  hDevice	
    )
{
	BC_STATUS	sts = BC_STS_SUCCESS;

	DTS_LIB_CONTEXT		*Ctx = NULL;
	DTS_GET_CTX(hDevice,Ctx);

	if( Ctx->State != BC_DEC_STATE_START ) {
		DebugLog_Trace(LDIL_DBG,"DtsPauseDecoder: Decoder Not Started\n");
		return BC_STS_ERR_USAGE;
	}

	sts = DtsFWPauseVideo(hDevice,eC011_PAUSE_MODE_ON);	
	if(sts != BC_STS_SUCCESS )
	{
		DebugLog_Trace(LDIL_DBG,"DtsPauseDecoder: Failed\n");
		return sts;
	}

	Ctx->State = BC_DEC_STATE_PAUSE;


	return sts;
}

DRVIFLIB_API BC_STATUS 
	DtsResumeDecoder(
    HANDLE  hDevice
    )
{
	BC_STATUS	sts = BC_STS_SUCCESS;

	DTS_LIB_CONTEXT		*Ctx = NULL;
	DTS_GET_CTX(hDevice,Ctx);

	if( Ctx->State != BC_DEC_STATE_PAUSE ) {		
		return BC_STS_ERR_USAGE;
	}

	sts = DtsFWPauseVideo(hDevice,eC011_PAUSE_MODE_OFF);	
	if(sts != BC_STS_SUCCESS )
	{
		return sts;
	}

	Ctx->State = BC_DEC_STATE_START;

	Ctx->totalPicsCounted = 0;
	Ctx->rptPicsCounted = 0;
	Ctx->nrptPicsCounted = 0;

	return sts;
}


DRVIFLIB_API BC_STATUS 
DtsStartCapture(HANDLE  hDevice)
{
	DTS_LIB_CONTEXT		*Ctx = NULL;
	BC_IOCTL_DATA *pIocData = NULL;
	BC_STATUS	sts = BC_STS_SUCCESS;

	DTS_GET_CTX(hDevice,Ctx);

	if(!(pIocData = DtsAllocIoctlData(Ctx)))
		return BC_STS_INSUFF_RES;

	sts = DtsMapYUVBuffs(Ctx);
	if(sts != BC_STS_SUCCESS){
		DebugLog_Trace(LDIL_DBG,"DtsMapYUVBuffs failed Sts:%d\n",sts);
		return sts;
	}

	DebugLog_Trace(LDIL_DBG,"DbgOptions=%x\n", Ctx->RegCfg.DbgOptions);
	pIocData->u.RxCap.Rsrd = 0;
	pIocData->u.RxCap.StartDeliveryThsh = RX_START_DELIVERY_THRESHOLD;
	pIocData->u.RxCap.PauseThsh = PAUSE_DECODER_THRESHOLD;
	pIocData->u.RxCap.ResumeThsh = 	RESUME_DECODER_THRESHOLD;
	if( (sts=DtsDrvCmd(Ctx,BCM_IOC_START_RX_CAP,0,pIocData,FALSE)) != BC_STS_SUCCESS){
		DebugLog_Trace(LDIL_DBG,"DtsStartCapture: Failed: %d\n",sts);
	}

	DtsRelIoctlData(Ctx,pIocData);	

	return sts;
}

DRVIFLIB_API BC_STATUS 
DtsFlushRxCapture(
				HANDLE  hDevice,	
				BOOL	bDiscardOnly)
{
	DTS_LIB_CONTEXT		*Ctx = NULL;
	BC_STATUS			Sts;
	BC_IOCTL_DATA *pIocData = NULL;

	DTS_GET_CTX(hDevice,Ctx);

	if(!(pIocData = DtsAllocIoctlData(Ctx)))
		return BC_STS_INSUFF_RES;

	pIocData->u.FlushRxCap.bDiscardOnly = bDiscardOnly;
	Sts = DtsDrvCmd(Ctx,BCM_IOC_FLUSH_RX_CAP, 0, pIocData, TRUE);
	return Sts;
}


DRVIFLIB_INT_API BC_STATUS
DtsCancelTxRequest(
	HANDLE	hDevice,
	uint32_t		Operation)
{
	return BC_STS_NOT_IMPL;
}


DRVIFLIB_API BC_STATUS 
DtsProcOutput(
    HANDLE  hDevice,
	uint32_t		milliSecWait,
	BC_DTS_PROC_OUT *pOut
)

{

	BC_STATUS	stRel,sts = BC_STS_SUCCESS;
#if 0
	BC_STATUS	stspwr = BC_STS_SUCCESS;
	BC_CLOCK *dynClock;
	BC_IOCTL_DATA *pIocData = NULL;
#endif
	BC_DTS_PROC_OUT OutBuffs;
	uint32_t	width=0, savFlags=0;
	
	

	DTS_LIB_CONTEXT		*Ctx = NULL;
	
	DTS_GET_CTX(hDevice,Ctx);
	

	if(!pOut){
		DebugLog_Trace(LDIL_DBG,"DtsProcOutput: Invalid Arg!!\n");
		return BC_STS_INV_ARG;
	}

	savFlags = pOut->PoutFlags;
	pOut->discCnt = 0;
	pOut->b422Mode = Ctx->b422Mode;

	do
	{
		memset(&OutBuffs,0,sizeof(OutBuffs));

		sts = DtsFetchOutInterruptible(Ctx,&OutBuffs,milliSecWait);
		if(sts != BC_STS_SUCCESS)
		{
			/* In case of a peek..*/
			if((sts == BC_STS_TIMEOUT) && !(milliSecWait) )
				sts = BC_STS_NO_DATA;
			return sts;
		}

		/* Copying the discontinuity count */
		if(OutBuffs.discCnt)
			pOut->discCnt = OutBuffs.discCnt;

		pOut->PoutFlags |= OutBuffs.PoutFlags;

		if(pOut->PoutFlags & BC_POUT_FLAGS_FMT_CHANGE){
			if(pOut->PoutFlags & BC_POUT_FLAGS_PIB_VALID){
				pOut->PicInfo = OutBuffs.PicInfo;
#if 0
// Disable clock control for Linux for now.
				if(OutBuffs.PicInfo.width > 1280) {
					Ctx->minClk = 135;
					Ctx->maxClk = 165;
				} else if (OutBuffs.PicInfo.width > 720) {
					Ctx->minClk = 105;
					Ctx->maxClk = 135;
				} else {
					Ctx->minClk = 75;
					Ctx->maxClk = 100;
				}
				if(!(pIocData = DtsAllocIoctlData(Ctx)))
					/* don't do anything */
					DebugLog_Trace(LDIL_DBG,"Could not allocate IOCTL for power ctl\n");
				else {
					dynClock = 	&pIocData->u.clockValue;
					dynClock->clk = Ctx->minClk;
					DtsDrvCmd(Ctx, BCM_IOC_CHG_CLK, 0, pIocData, FALSE);
				}
				if(pIocData) DtsRelIoctlData(Ctx,pIocData);
				
				Ctx->curClk = dynClock->clk;	
				Ctx->totalPicsCounted = 0;
				Ctx->rptPicsCounted = 0;
				Ctx->nrptPicsCounted = 0;
				Ctx->numTimesClockChanged = 0;
#endif
			}

			/* Update Counters */
			DebugLog_Trace(LDIL_DBG,"Dtsprocout:update stats\n");
			DtsUpdateOutStats(Ctx,pOut);

			DtsRelRxBuff(Ctx,&Ctx->pOutData->u.RxBuffs,TRUE);
			
		
			return BC_STS_FMT_CHANGE;
		}

		if(pOut->DropFrames)
		{
			/* We need to release the buffers even if we fail to copy..*/
			stRel = DtsRelRxBuff(Ctx,&Ctx->pOutData->u.RxBuffs,FALSE);

			if(sts != BC_STS_SUCCESS)
			{
				DebugLog_Trace(LDIL_DBG,"DtsProcOutput: Failed to copy out buffs.. %x\n", sts);
				return sts;
			}			
			pOut->DropFrames--;
			DebugLog_Trace(LDIL_DBG,"DtsProcOutput: Drop count.. %d\n", pOut->DropFrames);

			/* Get back the original flags */
			pOut->PoutFlags = savFlags;
		}
		else
			break;

	}while((pOut->DropFrames >= 0));

	if(pOut->AppCallBack && pOut->hnd && (OutBuffs.PoutFlags & BC_POUT_FLAGS_PIB_VALID))
	{
		/* Merge in and out flags */
		OutBuffs.PoutFlags |= pOut->PoutFlags;
		if(OutBuffs.PicInfo.width > 1280){
			width = 1920;
		}else if(OutBuffs.PicInfo.width > 720){
			width = 1280;
		}else{
			width = 720;
		}

		OutBuffs.b422Mode = Ctx->b422Mode;
		pOut->AppCallBack(	pOut->hnd,
							width,
							OutBuffs.PicInfo.height,
							0,
							&OutBuffs);
	}

	if(Ctx->b422Mode) {
		sts = DtsCopyRawDataToOutBuff(Ctx,pOut,&OutBuffs);	
	}else{
		if(pOut->PoutFlags & BC_POUT_FLAGS_YV12){
			sts = DtsCopyNV12ToYV12(Ctx,pOut,&OutBuffs);
		}else {
			sts = DtsCopyNV12(Ctx,pOut,&OutBuffs);
		}
	}

	if(pOut->PoutFlags & BC_POUT_FLAGS_PIB_VALID){
		pOut->PicInfo = OutBuffs.PicInfo;
	}
	
	if(Ctx->FlushIssued){
		if(Ctx->LastPicNum == pOut->PicInfo.picture_number){
			Ctx->eosCnt++;
		}else{
			Ctx->LastPicNum = pOut->PicInfo.picture_number;
			Ctx->eosCnt = 0;
			pOut->PicInfo.flags &= ~VDEC_FLAG_LAST_PICTURE;
		}
		if(Ctx->eosCnt >= BC_EOS_PIC_COUNT){
			/* Mark this picture as end of stream..*/
			pOut->PicInfo.flags |= VDEC_FLAG_LAST_PICTURE;
			Ctx->FlushIssued = 0;
			DebugLog_Trace(LDIL_DBG,"DtsProcOutput: Last Picture Set\n");
		}
		else {
			pOut->PicInfo.flags &= ~VDEC_FLAG_LAST_PICTURE;
		}
	}

#if 0
	/* Dynamic power control. If we are not repeating pictures then slow down the clock. 
	If we are repeating pictures, speed up the clock */
	if(Ctx->FlushIssued) {
		Ctx->totalPicsCounted = 0;
		Ctx->rptPicsCounted = 0;
		Ctx->nrptPicsCounted = 0;
		Ctx->numTimesClockChanged = 0;
	}
	else {
		Ctx->totalPicsCounted++;
		
		if(pOut->PicInfo.picture_number != Ctx->LastPicNum)
			Ctx->nrptPicsCounted++;
		else
			Ctx->rptPicsCounted++;
		
		if (Ctx->totalPicsCounted > 100) {
			Ctx->totalPicsCounted = 0;
			Ctx->rptPicsCounted = 0;
			Ctx->nrptPicsCounted = 0;
		}
	}
	
	/* start dynamic power mangement only after the first 10 pictures are decoded */
	/* algorithm used is to check if for every 20 pictures decoded greater than 6 are repeating. 
		Then we are too slow. If we are not repeating > 80% of the pictures, then slow down the clock.
		Slow down the clock only 3 times for the clip. This is an arbitrary number until we figure out the long 
		run problem. */
	if(pOut->PicInfo.picture_number >= 10) {			
		if((Ctx->totalPicsCounted >= 20) && (Ctx->totalPicsCounted % 20) == 0) {
			if(!(pIocData = DtsAllocIoctlData(Ctx)))
				/* don't do anything */
				DebugLog_Trace(LDIL_DBG,"Could not allocate IOCTL for power ctl\n");
			else {
				dynClock = 	&pIocData->u.clockValue;
				if (Ctx->rptPicsCounted >= 6) {
					dynClock->clk = Ctx->curClk + 5;
					if (dynClock->clk >= Ctx->maxClk)
						dynClock->clk = Ctx->curClk;
					Ctx->rptPicsCounted = 0;
				}
				else if (Ctx->nrptPicsCounted > 80) {
					dynClock->clk = Ctx->curClk - 5;
					if (dynClock->clk <= Ctx->minClk)
						dynClock->clk = Ctx->curClk;
					else
						Ctx->numTimesClockChanged++;
					if(Ctx->numTimesClockChanged > 3)
						dynClock->clk = Ctx->curClk;
				}
				else
					dynClock->clk = Ctx->curClk;
				
				if(dynClock->clk != Ctx->curClk)
					stspwr = DtsDrvCmd(Ctx, BCM_IOC_CHG_CLK, 0, pIocData, FALSE);
			}
		
			if(pIocData) DtsRelIoctlData(Ctx,pIocData);
			
			Ctx->curClk = dynClock->clk;	
			if (stspwr != BC_STS_SUCCESS)
				DebugLog_Trace(LDIL_DBG,"Could not change frequency for power ctl\n");
		}
	}
#endif

	Ctx->LastPicNum = pOut->PicInfo.picture_number;

	/* Update Counters */
	DtsUpdateOutStats(Ctx,pOut);

	/* We need to release the buffers even if we fail to copy..*/
	stRel = DtsRelRxBuff(Ctx,&Ctx->pOutData->u.RxBuffs,FALSE);

	if(sts != BC_STS_SUCCESS){
		DebugLog_Trace(LDIL_DBG,"DtsProcOutput: Failed to copy out buffs.. %x\n", sts);
		return sts;
	}

	return stRel;
}

#ifdef _ENABLE_CODE_INSTRUMENTATION_
static void dts_swap_buffer(uint32_t *dst, uint32_t *src, uint32_t cnt)
{
	uint32_t i=0;

	for (i=0; i < cnt; i++){
		dst[i] = BC_SWAP32(src[i]);
	}

}
#endif

DRVIFLIB_API BC_STATUS 
DtsProcOutputNoCopy(
    HANDLE  hDevice,
	uint32_t		milliSecWait,
	BC_DTS_PROC_OUT *pOut
)
{

	BC_STATUS	sts = BC_STS_SUCCESS;

	DTS_LIB_CONTEXT		*Ctx = NULL;
	
	DTS_GET_CTX(hDevice,Ctx);

	if(!pOut){
		return BC_STS_INV_ARG;
	}
	/* Init device params */
	if(Ctx->DevId == BC_PCI_DEVID_LINK){
		pOut->bPibEnc = TRUE;
	}else{
		pOut->bPibEnc = FALSE;
	}
	pOut->b422Mode = Ctx->b422Mode;

	while(1){

		if( (sts = DtsFetchOutInterruptible(Ctx,pOut,milliSecWait)) != BC_STS_SUCCESS){
			DebugLog_Trace(LDIL_DBG,"DtsProcOutput: No Active Channels\n");
				/* In case of a peek..*/
			if((sts == BC_STS_TIMEOUT) && !(milliSecWait) ){
				sts = BC_STS_NO_DATA;
				break;
			}	
		}

		/*
		 * If the PIB is not encrypted then we can direclty go
		 * to the PIB and get the information weather the Picture 
		 * is encrypted or not.
		 */
		
#ifdef _ENABLE_CODE_INSTRUMENTATION_

		if( (sts == BC_STS_SUCCESS) && 
			(!(pOut->PoutFlags & BC_POUT_FLAGS_FMT_CHANGE)) && 
			(FALSE == pOut->b422Mode))
		{
			uint32_t						Width=0,Height=0,PicInfoLineNum=0,CntrlFlags = 0; 
			uint8_t*						pPicInfoLine;

			Height = Ctx->picHeight;
			Width = Ctx->picWidth;
			
			PicInfoLineNum = (ULONG)(*(pOut->Ybuff + 3)) & 0xff
			| ((ULONG)(*(pOut->Ybuff + 2)) << 8) & 0x0000ff00
			| ((ULONG)(*(pOut->Ybuff + 1)) << 16) & 0x00ff0000
			| ((ULONG)(*(pOut->Ybuff + 0)) << 24) & 0xff000000;

			//DebugLog_Trace(LDIL_DBG,"PicInfoLineNum:%x, Width: %d, Height: %d\n",PicInfoLineNum, Width, Height);
			pOut->PoutFlags &= ~BC_POUT_FLAGS_PIB_VALID;
			if( (PicInfoLineNum == Height) || (PicInfoLineNum == Height/2))
			{
				pOut->PoutFlags |= BC_POUT_FLAGS_PIB_VALID;
				pPicInfoLine = pOut->Ybuff + PicInfoLineNum * Width;
				CntrlFlags  = (ULONG)(*(pPicInfoLine + 3)) & 0xff
							| ((ULONG)(*(pPicInfoLine + 2)) << 8) & 0x0000ff00
							| ((ULONG)(*(pPicInfoLine + 1)) << 16) & 0x00ff0000
							| ((ULONG)(*(pPicInfoLine + 0)) << 24) & 0xff000000;


				pOut->PicInfo.picture_number = 0xc0000000 & CntrlFlags;
				DebugLog_Trace(LDIL_DBG," =");
				if(CntrlFlags & BC_BIT(30)){
					pOut->PoutFlags |= BC_POUT_FLAGS_FLD_BOT;
					DebugLog_Trace(LDIL_DBG,"B+");
				}else{
					pOut->PoutFlags &= ~BC_POUT_FLAGS_FLD_BOT;
					DebugLog_Trace(LDIL_DBG,"T+");
				}

				if(CntrlFlags & BC_BIT(31)) {
					DebugLog_Trace(LDIL_DBG,"E");
				} else {
					DebugLog_Trace(LDIL_DBG,"Ne");
				}
				DebugLog_Trace(LDIL_DBG,"=");

				dts_swap_buffer((uint32_t*)&pOut->PicInfo,(uint32_t*)(pPicInfoLine + 4), 32);
			}
		}

#endif
		/* Update Counters.. */
		DtsUpdateOutStats(Ctx,pOut);

		if( (sts == BC_STS_SUCCESS) && (pOut->PoutFlags & BC_POUT_FLAGS_FMT_CHANGE) ){	
			DtsRelRxBuff(Ctx,&Ctx->pOutData->u.RxBuffs,TRUE);	
			sts = BC_STS_FMT_CHANGE;
			break;
		}

		if(pOut->DropFrames){
			/* We need to release the buffers even if we fail to copy..*/
			sts = DtsRelRxBuff(Ctx,&Ctx->pOutData->u.RxBuffs,FALSE);

			if(sts != BC_STS_SUCCESS)
			{
				DebugLog_Trace(LDIL_DBG,"DtsProcOutput: Failed to release buffs.. %x\n", sts);
				return sts;
			}			
			pOut->DropFrames--;
			DebugLog_Trace(LDIL_DBG,"DtsProcOutput: Drop count.. %d\n", pOut->DropFrames);

		}
		else
			break;

	}


	return sts;
}

DRVIFLIB_API BC_STATUS 
DtsReleaseOutputBuffs(
    HANDLE  hDevice,
	PVOID   Reserved,
	BOOL	fChange)
{
	DTS_LIB_CONTEXT		*Ctx = NULL;
	
	DTS_GET_CTX(hDevice,Ctx);

	if(fChange)
		return BC_STS_SUCCESS;

	return DtsRelRxBuff(Ctx, &Ctx->pOutData->u.RxBuffs, FALSE);
}


DRVIFLIB_API BC_STATUS 
DtsProcInput( HANDLE  hDevice ,
				 uint8_t *pUserData,
				 uint32_t ulSizeInBytes,
				 uint64_t timeStamp,
				 BOOL encrypted)
{
	uint32_t					DramOff;
	BC_STATUS			sts = BC_STS_SUCCESS;
	DTS_LIB_CONTEXT		*Ctx = NULL;
	DTS_INPUT_MDATA		*im = NULL;
	uint8_t					BDPktflag=1;

	DTS_GET_CTX(hDevice,Ctx);

	if(Ctx->VidParams.VideoAlgo == BC_VID_ALGO_VC1MP){
		uint32_t start_code=0x0;
		start_code = *(uint32_t *)(pUserData+pUserData[8]+9);
		
		/*In WMV ASF the PICTURE PAYLOAD can come in multiple PES PKTS as each payload pkt
		needs to be less than 64K. Filter sets NO_DISPLAY_PTS with the continuation pkts
		also, but only one SPES PKT needs to be inserted per picture. Only the first picture
		payload PES pkt will have the ASF header and rest of the continuation pkts do not
		have the ASF hdr. so, inser the SPES PKT only before the First picture payload pkt
		which has teh ASF hdr. The ASF HDr starts with teh magic number 0x5a5a5a5a.
		*/
		
		if(start_code !=0x5a5a5a5a){
			BDPktflag=0;
		}
	}

	if(timeStamp && BDPktflag){
		sts = DtsPrepareMdata(Ctx,timeStamp,&im);
		if(sts != BC_STS_SUCCESS){
			DebugLog_Trace(LDIL_ERR,"DtsProcInput: Failed to Prepare Spes hdr:%x\n",sts);
			return sts;
		}

        uint8_t *temp=NULL;
		uint32_t sz=0;
		uint8_t AsfHdr[32+sizeof(BC_PES_HDR_FORMAT) + 4];

		if(Ctx->VidParams.VideoAlgo == BC_VID_ALGO_VC1MP){
			temp = (uint8_t*) &AsfHdr;
			temp = (uint8_t*) ((uintptr_t)temp & ~0x3); /*Take care of alignment*/

			if(temp==NULL){
				DebugLog_Trace(TEXT("DtsProcInput: Failed to alloc mem for  ASFHdr for SPES:%x\n"),(char *)sts);
				return BC_STS_INSUFF_RES;
			}

			sts =DtsPrepareMdataASFHdr(Ctx, im, temp);
			if(sts != BC_STS_SUCCESS){
				DebugLog_Trace(TEXT("DtsProcInput: Failed to Prepare ASFHdr for SPES:%x\n"),(char *)sts);
				return sts;
			}
			
			sz = 32+sizeof(BC_PES_HDR_FORMAT);
		}else{
			temp = (uint8_t*)&im->Spes;
			sz = sizeof(im->Spes);
		}
	
		//sts = DtsTxDmaText(hDevice,(uint8_t*)&im->Spes,sizeof(im->Spes),&DramOff, encrypted);
        sts = DtsTxDmaText(hDevice,temp,sz,&DramOff, encrypted);
		if(sts != BC_STS_SUCCESS){
			DebugLog_Trace(LDIL_DBG,"DtsProcInput: Failed to send Spes hdr:%x\n",sts);
			DtsFreeMdata(Ctx,im,TRUE);
			return sts;
		}

		sts = DtsInsertMdata(Ctx,im);
		if(sts != BC_STS_SUCCESS){
			DebugLog_Trace(TEXT("DtsProcInput: DtsInsertMdata failed\n"),(char *)sts);
		}
	}
	
	sts = DtsTxDmaText(hDevice,pUserData,ulSizeInBytes,&DramOff, encrypted);

	if(sts == BC_STS_SUCCESS)
		DtsUpdateInStats(Ctx,ulSizeInBytes);

	return sts;
}

DRVIFLIB_API BC_STATUS 
DtsFlushInput( HANDLE  hDevice ,
				 uint32_t Op )
{
	BC_STATUS			sts = BC_STS_SUCCESS;
	DTS_LIB_CONTEXT		*Ctx = NULL;

	DTS_GET_CTX(hDevice,Ctx);

	if (Op == 3) {
		return DtsCancelTxRequest(hDevice, Op);
	}

	if(Op == 0)
		bc_sleep_ms(100);
	sts = DtsFWDecFlushChannel(hDevice,Op);

	if(sts != BC_STS_SUCCESS)
		return sts;

	Ctx->LastPicNum = 0;
	Ctx->eosCnt = 0;

	if(Op == 0)//If mode is drain.
		Ctx->FlushIssued = TRUE;
	else
		DtsClrPendMdataList(Ctx);

	return BC_STS_SUCCESS;
}
DRVIFLIB_API BC_STATUS 
DtsSetRateChange(HANDLE  hDevice ,
				 uint32_t rate,
				 uint8_t direction )
{
	BC_STATUS			sts = BC_STS_SUCCESS;
	DTS_LIB_CONTEXT		*Ctx = NULL;

	DTS_GET_CTX(hDevice,Ctx);

	//For Rate Change
	uint32_t mode = 0;
	uint32_t HostTrickModeEnable = 0;

	//Change Rate Value for Version 1.1
	//Rate: Specifies the new rate x 10000
	//Rate is the inverse of speed. For example, if the playback speed is 2x, the rate is 1/2, so the Rate member is set to 5000.

	float fRate = float(1) / ((float)rate / (float)10000);

	//Mode Decision
	if(fRate < 1)
	{
		//Slow
		LONG Rate = LONG(float(1) / fRate);

		mode = eC011_SKIP_PIC_IPB_DECODE;
		HostTrickModeEnable = 1;

		sts = DtsFWSetHostTrickMode(hDevice,HostTrickModeEnable);
		if(sts != BC_STS_SUCCESS)
		{
			DebugLog_Trace(LDIL_DBG,"DtsSetRateChange:DtsFWSetHostTrickMode Failed\n");
			return sts;
		}

		sts = DtsFWSetSkipPictureMode(hDevice,mode);
		if(sts != BC_STS_SUCCESS)
		{
			DebugLog_Trace(LDIL_DBG,"DtsSetRateChange: DtsFWSetSkipPictureMode Failed\n");
			return sts;
		}

		sts = DtsFWSetSlowMotionRate(hDevice, Rate);

		if(sts != BC_STS_SUCCESS)
		{
			DebugLog_Trace(LDIL_DBG,"DtsSetRateChange: Set Slow Forward Failed\n");

			return sts;
		}
	}
	else
	{
		//Fast
		LONG Rate = (LONG)fRate;

		//For I-Frame Only Trick Mode
		//Direction: 0: Forward, 1: Backward
		if((Rate == 1) && (direction == 0))
		{
			DebugLog_Trace(LDIL_DBG,"DtsSetRateChange: Set Normal Speed\n");
			mode = eC011_SKIP_PIC_IPB_DECODE;
			HostTrickModeEnable = 0;			
			if(fRate > 1 && fRate < 2)
			{
				//Nav is giving I instead of IBP for 1.4x or 1.6x
				DebugLog_Trace(LDIL_DBG,"DtsSetRateChange: Set 1.x I only\n");
				mode = eC011_SKIP_PIC_I_DECODE;
				HostTrickModeEnable = 1;		
			}
		}
		else
		{
			//I-Frame Only for Fast Forward and All Speed Backward
			DebugLog_Trace(LDIL_DBG,"DtsSetRateChange: Set Very Fast Speed for I-Frame Only\n");
			mode = eC011_SKIP_PIC_I_DECODE;
			HostTrickModeEnable = 1;			
		}

		sts = DtsFWSetHostTrickMode(hDevice,HostTrickModeEnable);
		if(sts != BC_STS_SUCCESS)
		{
			DebugLog_Trace(LDIL_DBG,"DtsSetRateChange:DtsFWSetHostTrickMode Failed\n");
			return sts;
		}

		sts = DtsFWSetSkipPictureMode(hDevice,mode);
		if(sts != BC_STS_SUCCESS)
		{
			DebugLog_Trace(LDIL_DBG,"DtsSetRateChange: DtsFWSetSkipPictureMode Failed\n");
			return sts;
		}

		sts = DtsFWSetFFRate(hDevice, Rate);
		if(sts != BC_STS_SUCCESS)
		{
			DebugLog_Trace(LDIL_DBG,"DtsSetRateChange: Set Fast Forward Failed\n");
			return sts;
		}
	}

	return BC_STS_SUCCESS;
}

//Set FF Rate for Catching Up
DRVIFLIB_API BC_STATUS 
DtsSetFFRate(HANDLE  hDevice ,
				 uint32_t Rate)
{
	BC_STATUS			sts = BC_STS_SUCCESS;
	DTS_LIB_CONTEXT		*Ctx = NULL;

	DTS_GET_CTX(hDevice,Ctx);

	//For Rate Change
	uint32_t mode = 0;
	uint32_t HostTrickModeEnable = 0;
	

	//IPB Mode for Catching Up
	mode = eC011_SKIP_PIC_IPB_DECODE;

	if(Rate == 1)
	{
		//Normal Mode
		DebugLog_Trace(LDIL_DBG,"DtsSetFFRate: Set Normal Speed\n");
		
		HostTrickModeEnable = 0;
	}
	else
	{
		//Fast Forward
		DebugLog_Trace(LDIL_DBG,"DtsSetFFRate: Set Fast Forward\n");

		HostTrickModeEnable = 1;			
	}

	sts = DtsFWSetHostTrickMode(hDevice, HostTrickModeEnable);
	if(sts != BC_STS_SUCCESS)
	{
		DebugLog_Trace(LDIL_DBG,"DtsSetFFRate:DtsFWSetHostTrickMode Failed\n");
		return sts;
	}

	sts = DtsFWSetSkipPictureMode(hDevice, mode);
	if(sts != BC_STS_SUCCESS)
	{
		DebugLog_Trace(LDIL_DBG,"DtsSetFFRate: DtsFWSetSkipPictureMode Failed\n");
		return sts;
	}

	sts = DtsFWSetFFRate(hDevice, Rate);
	if(sts != BC_STS_SUCCESS)
	{
		DebugLog_Trace(LDIL_DBG,"DtsSetFFRate: Set Fast Forward Failed\n");
		return sts;
	}


	return BC_STS_SUCCESS;
}

DRVIFLIB_API BC_STATUS 
DtsSetSkipPictureMode( HANDLE  hDevice ,
						uint32_t SkipMode )
{
	BC_STATUS			sts = BC_STS_SUCCESS;
	DTS_LIB_CONTEXT		*Ctx = NULL;

	DTS_GET_CTX(hDevice,Ctx);

	sts = DtsFWSetSkipPictureMode(hDevice,SkipMode);

	if(sts != BC_STS_SUCCESS)
	{
		DebugLog_Trace(LDIL_DBG,"DtsSetSkipPictureMode: Set Picture Mode Failed, %d\n",SkipMode);	
		return sts;
	}

	return BC_STS_SUCCESS;
}

DRVIFLIB_API BC_STATUS 
	DtsSetIFrameTrickMode(
	HANDLE  hDevice
	)
{
	BC_STATUS			sts = BC_STS_SUCCESS;
	DTS_LIB_CONTEXT		*Ctx = NULL;

	DTS_GET_CTX(hDevice,Ctx);

	sts = DtsFWSetHostTrickMode(hDevice,1);
	if(sts != BC_STS_SUCCESS)
	{
		DebugLog_Trace(LDIL_DBG,"DtsSetIFrameTrickMode: DtsFWSetHostTrickMode Failed\n");
		return sts;
	}

	sts = DtsFWSetSkipPictureMode(hDevice,eC011_SKIP_PIC_I_DECODE);
	if(sts != BC_STS_SUCCESS)
	{
		DebugLog_Trace(LDIL_DBG,"DtsSetIFrameTrickMode: DtsFWSetSkipPictureMode Failed\n");
		return sts;
	}
	return BC_STS_SUCCESS;
}


DRVIFLIB_API BC_STATUS 
	DtsStepDecoder(
    HANDLE  hDevice	
    )
{
	BC_STATUS	sts = BC_STS_SUCCESS;

	DTS_LIB_CONTEXT		*Ctx = NULL;
	DTS_GET_CTX(hDevice,Ctx);

	if( Ctx->State != BC_DEC_STATE_PAUSE ) {
		DebugLog_Trace(LDIL_DBG,"DtsStepDecoder: Failed because Decoder is Not Paused\n");
		return BC_STS_ERR_USAGE;
	}

	sts = DtsFWFrameAdvance(hDevice);	
	if(sts != BC_STS_SUCCESS )
	{
		DebugLog_Trace(LDIL_DBG,"DtsStepDecoder: Failed \n");
		return sts;
	}

	return sts;
}


DRVIFLIB_API BC_STATUS 
	DtsIs422Supported(
    HANDLE  hDevice,
	uint8_t*		bSupported
    )
{
	BC_STATUS	sts = BC_STS_SUCCESS;

	DTS_LIB_CONTEXT		*Ctx = NULL;
	DTS_GET_CTX(hDevice,Ctx);

	if(Ctx->DevId == BC_PCI_DEVID_LINK)
	{
		*bSupported = 1;
	}
	else
	{
		*bSupported = 0;
	}
	return sts;
}

DRVIFLIB_API BC_STATUS 
	DtsSet422Mode(
    HANDLE  hDevice,
	uint8_t	Mode422
    )
{
	BC_STATUS	sts = BC_STS_SUCCESS;
	DTS_LIB_CONTEXT		*Ctx = NULL;

	DTS_GET_CTX(hDevice,Ctx);

	if(Ctx->DevId != BC_PCI_DEVID_LINK)
	{
		return sts;
	}

	Ctx->b422Mode = Mode422;

	sts = DtsSetLinkIn422Mode(hDevice);

	return sts;
}

DRVIFLIB_API BC_STATUS
DtsGetDILPath(
    HANDLE  hDevice,
	char		*DilPath,
	uint32_t		size
    )
{
	DTS_LIB_CONTEXT		*Ctx = NULL;
	uint32_t *ptemp=NULL;
	DTS_GET_CTX(hDevice,Ctx);

	if(!DilPath || (size < sizeof(Ctx->DilPath)) ){
		return BC_STS_INV_ARG;
	}

	/* if the first 4 bytes are zero, then the dil path in the context
	is not yet set. Hence go ahead and look at registry and update the
	context. and then just copy the dil path from context.
	*/
	ptemp = (uint32_t*)&Ctx->DilPath[0];
	if(!(*ptemp))
		DtsGetFirmwareFiles(Ctx);

	strncpy(DilPath, Ctx->DilPath, sizeof(Ctx->DilPath));


	return BC_STS_SUCCESS;
}

DRVIFLIB_API BC_STATUS 
DtsDropPictures( HANDLE  hDevice ,
                uint32_t Pictures )
{
	BC_STATUS			sts = BC_STS_SUCCESS;
	DTS_LIB_CONTEXT		*Ctx = NULL;

	DTS_GET_CTX(hDevice,Ctx);

	sts = DtsFWDrop(hDevice,Pictures);

	if(sts != BC_STS_SUCCESS)
	{
		DebugLog_Trace(LDIL_DBG,"DtsDropPictures: Set Picture Mode Failed, %d\n",Pictures);	
		return sts;
	}

	return BC_STS_SUCCESS;
}

DRVIFLIB_API BC_STATUS 
DtsGetDriverStatus( HANDLE  hDevice,
	                BC_DTS_STATUS *pStatus)
{
    BC_DTS_STATS temp;
    BC_STATUS ret;

    ret = DtsGetDrvStat(hDevice, &temp);

    pStatus->FreeListCount      = temp.drvFLL;
    pStatus->ReadyListCount     = temp.drvRLL;
    pStatus->FramesCaptured     = temp.opFrameCaptured;
    pStatus->FramesDropped      = temp.opFrameDropped;
    pStatus->FramesRepeated     = temp.reptdFrames;
    pStatus->PIBMissCount       = temp.pibMisses;
    pStatus->InputCount         = temp.ipSampleCnt;
    pStatus->InputBusyCount     = temp.TxFifoBsyCnt;
    pStatus->InputTotalSize     = temp.ipTotalSize;

    pStatus->PowerStateChange   = temp.pwr_state_change;

	return ret;
}
