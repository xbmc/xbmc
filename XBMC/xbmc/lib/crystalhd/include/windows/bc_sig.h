/********************************************************************
 * Copyright(c) 2006 Broadcom Corporation, all rights reserved.
 * Contains proprietary and confidential information.
 *
 * This source file is the property of Broadcom Corporation, and
 * may not be copied or distributed in any isomorphic form without
 * the prior written consent of Broadcom Corporation.
 *
 *
 *  Name: bc_sig.h
 *
 *  Description: Structures for Host-Bootloader communication
 *              
 *
 *  AU
 *
 *  HISTORY:
 *
 *******************************************************************/
#ifndef _BC_SIG_H_
#define _BC_SIG_H_

#define INITIATE_FW_DOWNLOAD	0x1 //bit 0
#define DOWNLOAD_READY	0x10 //bit 4
#define DOWNLOAD_COMPLETE 0x2  //bit 2
#define FIRMWARE_VALIDATED	0x1 //bit 0
#define SIGNATURE_MATCHED 0x200 //bit 9
#define SIGNATURE_MISMATCH  0x100 //bit 8
#define START_PROCESSOR 0x10 //bit 4

BC_STATUS CheckFpgaVersion(HANDLE handle, U32 minVer, U32 maxVer);

#define BC_FPGA_MINVER	0x2EA
#define BC_FPGA_MAXVER	0xFFF

#define CHK_FPGA_VER(handle, minVer, maxVer)								\
{																		\
	if(BC_STS_SUCCESS != CheckFpgaVersion(handle, minVer, maxVer)){				\
		if(maxVer == 0xFFF) {											\
			printf("Expected Version(s): 0x%X or greater\n",minVer);		\
		} else {														\
			printf("Expected Version(s): 0x%X to 0x%X\n",minVer,maxVer);	\
		}																\
		return BC_STS_VER_MISMATCH;										\
	}																	\
}


BC_STATUS dts_sec_readFile(U8* buf,int *buflen,char *filename);
BC_STATUS dts_sec_writeToFile(U8*buf,int len,char *filename);
BC_STATUS DtsProgKeyInOTP(HANDLE hndl,U8* buff);
BC_STATUS dec_burn_bootloader(HANDLE dil_handle, char* binFile);
BC_STATUS dec_burn_key(HANDLE dil_handle, char* keyfile);
void get_help();
void swapbuffer(U8*buf1,U8*buf2,int bufsize);
BC_STATUS bc_gen_sig(HANDLE dil_handle);

#endif
