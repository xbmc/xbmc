/********************************************************************
 * Copyright(c) 2006-2009 Broadcom Corporation.
 *
 *  Name: libcrystalhd_fwdiag_if.cpp
 *
 *  Description: Firmware diagnostics
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
#include "libcrystalhd_if.h"
#include "libcrystalhd_priv.h"
#include "libcrystalhd_fwdiag_if.h"
#include "bc_defines.h"

/* BOOTLOADER IMPLEMENTATION */
/* Functions */
DRVIFLIB_INT_API BC_STATUS
DtsSendFWDiagCmd(HANDLE hDevice,BC_HOST_CMD_BLOCK_ST hostMsg)
{
	BC_STATUS status=BC_STS_ERROR;

	/* Checksum of Message Block */
	hostMsg.chk_sum = ~(hostMsg.done+hostMsg.cmd+hostMsg.start+hostMsg.size+
						hostMsg.cmdargs[0]+hostMsg.cmdargs[1]+hostMsg.cmdargs[2]);

	/* Loading of Message Block */
	hostMsg.done &= (~BC_HOST_CMD_POSTED);
	status = DtsDevMemWr(hDevice,(uint32_t *)&hostMsg,sizeof(hostMsg),BC_HOST_CMD_ADDR);
	if(BC_STS_ERROR == status)
	{
		DebugLog_Trace(LDIL_DBG,"Writing register failed status:%x\n",status);
		return status;
	}

	/* Issue done */
	hostMsg.done = BC_HOST_CMD_POSTED;
	status = DtsDevMemWr(hDevice,&(hostMsg.done),4,BC_HOST_CMD_ADDR);
	if(BC_STS_ERROR == status)
	{
		DebugLog_Trace(LDIL_DBG,"Writing register failed status:%x\n",status);
		return status;
	}

	return status;
}

DRVIFLIB_INT_API BC_STATUS
DtsClearFWDiagCommBlock(HANDLE hDevice)
{
	BC_STATUS status=BC_STS_ERROR;
	BC_HOST_CMD_BLOCK_ST hostMsg;
	BC_FWDIAG_RES_BLOCK_ST blMsg;

	memset(&hostMsg, 0, sizeof(BC_HOST_CMD_BLOCK_ST));
	memset(&blMsg, 0, sizeof(BC_FWDIAG_RES_BLOCK_ST));

	status = DtsDevMemWr(hDevice,(uint32_t *)&hostMsg,sizeof(BC_HOST_CMD_BLOCK_ST),BC_HOST_CMD_ADDR);
	if(BC_STS_ERROR == status)
	{
		DebugLog_Trace(LDIL_DBG,"Clearing Host Message Block failed, status:%x\n",status);
		return status;
	}

	status = DtsDevMemWr(hDevice,(uint32_t *)&blMsg,sizeof(BC_FWDIAG_RES_BLOCK_ST),BC_FWDIAG_RES_ADDR);
	if(BC_STS_ERROR == status)
	{
		DebugLog_Trace(LDIL_DBG,"Clearing Host Message Block failed, status:%x\n",status);
		return status;
	}

	return status;
}

DRVIFLIB_INT_API BC_STATUS
DtsReceiveFWDiagRes(HANDLE hDevice, PBC_FWDIAG_RES_BLOCK_ST pBlMsg,uint32_t wait)
{
	BC_STATUS status = BC_STS_ERROR;
	uint32_t chkSum = 0;
	uint32_t cnt=1000;

	while(--cnt) {
		status = DtsDevMemRd(hDevice,(uint32_t *)(pBlMsg),sizeof(BC_FWDIAG_RES_BLOCK_ST),BC_FWDIAG_RES_ADDR);
		if(BC_STS_SUCCESS != status){

			DebugLog_Trace(LDIL_DBG,"Command Failure From DIL status:%x\n",status);
			return (status);
		}

		if(pBlMsg->done&BC_FWDIAG_RES_POSTED) {
			chkSum = ~(pBlMsg->done+pBlMsg->status+pBlMsg->detail[0]+\
				pBlMsg->detail[1]+pBlMsg->detail[2]+pBlMsg->detail[3]+pBlMsg->detail[4]);

			if(chkSum != pBlMsg->chk_sum)
			{
				DebugLog_Trace(LDIL_DBG,"Recv. Message Checksum failed\n");
				return BC_STS_ERROR;
			}

			/* Success */
			break;
		}
		/* Wait */
		bc_sleep_ms(wait);
	}

	/* Clear the Message */
	DtsClearFWDiagCommBlock(hDevice);

	if(!cnt){
		DebugLog_Trace(LDIL_DBG,"Message Receive Timed-out\n");
		return BC_STS_TIMEOUT;
	}

	return BC_STS_SUCCESS;
}


DRVIFLIB_INT_API BC_STATUS
DtsDownloadFWDIAGToLINK(HANDLE hDevice,char *FwBinFile)
{
	BC_STATUS status = BC_STS_ERROR;
 uint32_t byesDnld=0;
	//char *fwfile=NULL;
	char fwfile[MAX_PATH+1];
	DTS_LIB_CONTEXT		*Ctx = NULL;
	uint32_t	RegVal =0;

	BC_FWDIAG_RES_BLOCK_ST blMsg;

		DebugLog_Trace(LDIL_DBG,"0. fwfile is %s\n",FwBinFile);
	/* Clear Host Message Area */
	status = DtsClearFWDiagCommBlock(hDevice);
	if(status != BC_STS_SUCCESS) {
		DebugLog_Trace(LDIL_DBG,"DtsDownloadFWDIAGToLINK: Failed to clear the message area\n");
		return status;
	}


	DTS_GET_CTX(hDevice,Ctx);

	/* Get the firmware file to download */
	status = DtsGetDILPath(hDevice, fwfile, sizeof(fwfile));
	if(status != BC_STS_SUCCESS){
		return status;
	}

	if(FwBinFile!=NULL){
		strncat(fwfile,(const char*)FwBinFile,sizeof(fwfile));
		DebugLog_Trace(LDIL_DBG,"1. fwfile is %s\n",FwBinFile);
	}else{
		strncat(fwfile,"/",sizeof(fwfile));
		strncat(fwfile,"bcmFWDiag.bin",sizeof(fwfile));
		DebugLog_Trace(LDIL_DBG,"2. fwfile is %s\n",fwfile);
	}

	//Read OTP_CMD registers to see if Keys are already programmed in OTP
	RegVal =0;
	status = DtsFPGARegisterRead(hDevice, OTP_CMD, &RegVal);
	if(status != BC_STS_SUCCESS)
	{
		DebugLog_Trace(LDIL_DBG,"Error Reading DCI_STATUS register\n");
		return status;
	}




	status = fwbinPushToLINK(hDevice, fwfile, &byesDnld);

	if(status != BC_STS_SUCCESS) {
		DebugLog_Trace(LDIL_DBG,"DtsDownloadAuthFwToLINK: Failed to download firmware\n");
		return status;
	}

	/* Check for firmware authentication result */

	//look for SIGNATURE_MATCHED or SIGNATURE_MISMATCH in DCI status Register.

	RegVal =0;
	status = DtsFPGARegisterRead(hDevice, DCI_STATUS, &RegVal);
	if(status != BC_STS_SUCCESS)
	{
		DebugLog_Trace(LDIL_DBG,"Error Reading DCI_STATUS register\n");
		return status;
	}

	if ((RegVal & DCI_SIGNATURE_MATCHED) == DCI_SIGNATURE_MATCHED)
	{
		//if SIGNATURE_MATCHED Wait for FW_VALIDATED bit to be set.
		DebugLog_Trace(LDIL_DBG,"Signature Matched\n");

		uint32_t cnt = 1000;
		while((RegVal & DCI_FIRMWARE_VALIDATED) != DCI_FIRMWARE_VALIDATED)
		{
			status = DtsFPGARegisterRead(hDevice, DCI_STATUS, &RegVal);
			if(status != BC_STS_SUCCESS)
			{
				DebugLog_Trace(LDIL_DBG,"Error Reading DCI_STATUS register\n");
				return status;
			}
			RegVal &= DCI_FIRMWARE_VALIDATED;
			if(!(--cnt))
				break;
			bc_sleep_ms(1);

	   }

		//uart
		status = DtsDevRegisterWr(hDevice, 0x00100300, 0x03);
		if(status != BC_STS_SUCCESS)
		{
			DebugLog_Trace(LDIL_DBG,"Error Writing UART register\n");

		}
		else
			DebugLog_Trace(LDIL_DBG,"Uart Set Sucessfully\n");

		//START_PROCESSOR bit in DCI_CMD.
		RegVal = 0;
		status = DtsFPGARegisterRead(hDevice, DCI_CMD, &RegVal);
			if(status != BC_STS_SUCCESS)
			{
				DebugLog_Trace(LDIL_DBG,"Error Reading DCI_CMD register\n");
				return status;
			}
		RegVal |= DCI_START_PROCESSOR;
		//DebugLog_Trace(LDIL_DBG,"DCICMD RegVal:%x\n",RegVal);
		status = DtsFPGARegisterWr(hDevice, DCI_CMD, RegVal);
		if(status != BC_STS_SUCCESS)
		{
			DebugLog_Trace(LDIL_DBG,"Error Writing DCI_CMD register\n");
			return status;
		}


	}
	else if ((RegVal & DCI_SIGNATURE_MISMATCH)==DCI_SIGNATURE_MISMATCH)
	{
		DebugLog_Trace(LDIL_DBG,"FW AUthentication failed. Signature Mismatch\n");
		return BC_STS_FW_AUTH_FAILED;
	}

	/* Check bootloader Status */
	status = DtsReceiveFWDiagRes(hDevice, &blMsg, 10);
	if(status != BC_STS_SUCCESS) {
		DebugLog_Trace(LDIL_DBG,"DtsDownloadFWDIAGToLINK: Receive message for FWDiag booting failed, status=%d\n", status);
		return BC_STS_BOOTLOADER_FAILED; //status;
	}

	if(blMsg.status != BC_FWDIAG_BOOTUP_DONE) {
		DebugLog_Trace(LDIL_DBG,"DtsDownloadFWDIAGToLINK: Failed to boot the FWDiag\n");
		return BC_STS_BOOTLOADER_FAILED; //BC_STS_ERROR;
	}

	return BC_STS_SUCCESS;
}





