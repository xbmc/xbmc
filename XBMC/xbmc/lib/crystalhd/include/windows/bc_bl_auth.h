/********************************************************************
 * Copyright(c) 2006 Broadcom Corporation, all rights reserved.
 * Contains proprietary and confidential information.
 *
 * This source file is the property of Broadcom Corporation, and
 * may not be copied or distributed in any isomorphic form without
 * the prior written consent of Broadcom Corporation.
 *
 *
 *  Name: bc_bl_auth.h
 *
 *  Description: Structures for Host-Bootloader communication
 *              
 *
 *  AU
 *
 *  HISTORY:
 *
 *******************************************************************/

#ifndef _BL_AUTH_H_
#define _BL_AUTH_H_

#include "bc_dts_types.h"


/* Bootloader-Host defines */
#define BC_TOTAL_BL_SIZE					0x2800
#define BC_EPROM_BL_ST_ADDR					0x1800
#define BC_EPROM_BL_VECTOR_SZ				0x100
#define BC_DRAM_FW_VECTOR_SZ				0x100
#define BC_EPROM_BL_VECTOR_OFFSET			BC_BL_ST_ADDR
#define BC_VECTOR_BIN_BUFF_OFFSET			0
#define BC_FW_VECTOR_BUFF_OFFSET			0x00300100
#define BC_EPROM_BL_CODE_SZ					(BC_TOTAL_BL_SIZE-BC_EPROM_BL_VECTOR_SZ)
#define BC_EPROM_BL_CODE_OFFSET				(BC_EPROM_BL_VECTOR_OFFSET+BC_EPROM_BL_VECTOR_SZ)
#define BC_DRAM_BL_CODE_OFFSET				(0x300800)	

#define DRAM_WINDOW_BASE					0x00340020		   // DRAM Address to access 512K Size of data.

#define BC_EP_AUTH_KEY_SIG_ADDR				0x0000

/* Key+Signature, Max of 1024 bytes */
#define BC_EP_AUTH_KEY_SIG_SIZE				0x0100

#define BC_HOST_DONE						0x00000001
#define BC_HBL_FW_LOAD_SUCCESS_FLAG			0x00000001
#define BC_HBL_FW_DISABLE_VERIFY_FLAG		0x80000000
#define BC_HOST_MSG_ADDR					0x00300000

#define HMAC_DEFAULT_KEY_LEN	32
#define HMAC_KEY_LEN	16
#define HMAC_KEY_LEN2	32
#define HMAC_SIG_LEN	20
#define HMAC_SIG_LEN2	32
#define HMAC_COMBINED_SIG_LEN	100 //88



/* Host-Bootloader Communication Message Block */
typedef struct _BC_HOST_MSG_BLOCK_ST {

	U32	done;
	U32	status;
	U32	img_start;
	U32	img_size;
	U32	res[3];
	U32	chk_sum;

} BC_HOST_MSG_BLOCK_ST, *PBC_HOST_MSG_BLOCK_ST;

#define BC_BL_DONE						0x00000001

/* Bootloader Status */
typedef enum _BC_BL_STATUS{
	BC_BLH_BOOTUP_DONE		= 1,
	BC_BLH_FW_AUTH_PASS		= 2,
	BC_BLH_FW_AUTH_FAIL		= 3,

	/* Must be the last one.*/
	BC_BLH_STS_ERROR 		= -1
}BC_BL_STATUS;

#define BC_BL_MSG_ADDR					0x00300040

/* Bootloader-Host Communication Message Block */
typedef struct _BC_BL_MES_BLOCK_ST {

	U32	done;
	U32	status;
	U32	retry_count;
	U32	res[4];
	U32	chk_sum;

}BC_BL_MES_BLOCK_ST, *PBC_BL_MES_BLOCK_ST;

/* KEY + SIGNATURE */
#define BC_BL_HMAC_KEY_SIG_ADDR			0x00300200
#define BC_DRAM_AUTH_KEY_SIG_ADDR		BC_BL_HMAC_KEY_SIG_ADDR
#define BC_BL_HMAC_SIG_ADDR				0x00300300
#define BC_BL_MAX_KEYSIG_SIZE			252

/* Key+Signature exchange structure */
typedef struct _BC_HMAC_SHA1_KEY_SIG_ST {

	U32	key_len;
	U8	key[BC_BL_MAX_KEYSIG_SIZE];
	U32	sig_len;
	U8	sig[BC_BL_MAX_KEYSIG_SIZE];

}BC_HMAC_SHA1_KEY_SIG_ST, *PBC_HMAC_SHA1_KEY_SIG_ST;

#define BC_DRAM_CERT_ADDR					0x400
#define BC_MPC_EP_x509_CERT_ADDR			0x400
#define BC_MPC_EP_CIPHER_DRAM_ADDR			0x1c2100


#endif /* _BL_AUTH_H_ */

