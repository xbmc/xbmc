/*****************************************************************************
 * Copyright(c) 2006 Broadcom Corporation, all rights reserved.
 * Contains proprietary and confidential information.
 *
 * This source file is the property of Broadcom Corporation, and
 * may not be copied or distributed in any isomorphic form without
 * the prior written consent of Broadcom Corporation.
 *
 *
 *  Name: bc_drv_if.h
 *
 *  Description: Driver Interface API.
 *
 *  AU
 *
 *  HISTORY:
 *
 ****************************************************************************/
#ifndef _BCM_DETECT_H_
#define _BCM_DETECT_H_

#include "bc_dts_defs.h"

extern U32	_g_rnd_table[8];
extern U32 g_did; 

__inline BOOL bc_poll_ready(U32 *table)
{
	U32 *reg = &table[0];
	U8	*buff = (U8*)&_g_rnd_table[0];

	reg[5] = GetTickCount();

	reg[0] = ( (buff[0] << 24) | (buff[1] << 16) | (buff[2] << 8) | (buff[3]) );
	reg[1] = ( (buff[4] << 24) | (buff[5] << 16) | (buff[6] << 8) | (buff[7]) );
	reg[2] = ( (buff[8] << 24) | (buff[9] << 16) | (buff[10] << 8) | (buff[11]) );
	reg[3] = ( (buff[12] << 24) | (buff[13] << 16) | (buff[14] << 8) | (buff[15]) );

	reg[7] = reg[0] ^ reg[1] ^ reg[2] ^ reg[3];

	if(_g_rnd_table[6] != 840426719){
		reg[6] = GetTickCount();
	}else if(_g_rnd_table[7] == 199018230){
		reg[6] = GetTickCount();
		reg[4] = (reg[7] ^ (reg[5] | reg[6]));
	}else{
		reg[4] = (reg[7] ^ reg[5] ^ reg[6]);
	}

	return TRUE;
}

__inline BOOL bc_poll_done(U32 *table)
{
	U32 *reg = &table[0];
	U8	*buff = (U8*)&_g_rnd_table[0];

	reg[5] = GetTickCount();

	reg[0] = ( (buff[0] << 24) | (buff[1] << 16) | (buff[2] << 8) | (buff[3]) );
	reg[1] = ( (buff[4] << 24) | (buff[5] << 16) | (buff[6] << 8) | (buff[7]) );
	
	reg[7] = reg[0] ^ reg[1] ^ reg[2] ^ reg[3];

	reg[6] = GetTickCount();

	reg[2] = ( (buff[11] << 24) | (buff[10] << 16) | (buff[9] << 8) | (buff[8]) );
	reg[3] = ( (buff[15] << 24) | (buff[14] << 16) | (buff[13] << 8) | (buff[11]) );

	reg[4] = (reg[7] ^ (reg[5] | reg[6]));

	_g_rnd_table[1] = GetTickCount();

	_g_rnd_table[6] =  ( (buff[4] << 24) | (buff[5] << 16) | (buff[6] << 8) | (buff[7]) );

	reg[5] = GetTickCount();

	return TRUE;
}

#define bc_chk_table(reg) ( (reg[0] ^ reg[1] ^ reg[2] ^ reg[3])^ (reg[5] | reg[6]) )

__inline BOOL bc_chk_w32_dbg(U32 *table)
{
	BOOL var = IsDebuggerPresent();
	if(var){
		bc_poll_done(table); //mismatch
	}else{
		bc_poll_ready(table);
	}
	return var;
}

__inline BOOL bc_chk_hw_present(U32 *table)
{
	HANDLE chk_hnd = NULL;
	BC_STATUS sts = BC_STS_SUCCESS;
	U32	vid=0, rid = 0;
	
	sts = DtsDeviceOpen(&chk_hnd,DTS_DIAG_MODE);
	if(sts == BC_STS_SUCCESS) {
		DtsGetHwType(chk_hnd,&g_did,&vid,&rid);
		DtsDeviceClose(chk_hnd);
		return TRUE;
	}

	return FALSE;
}

extern bool CDebugDetect();

#endif
