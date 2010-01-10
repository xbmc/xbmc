/********************************************************************
 * Copyright(c) 2006-2009 Broadcom Corporation.
 *
 *  Name: libcrystalhd_priv.h
 *
 *  Description: Driver Interface library Interanl.
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

#ifndef _BCM_DRV_IF_PRIV_
#define _BCM_DRV_IF_PRIV_

#include "bc_dts_glob_lnx.h"
#include "7411d.h"
#include <semaphore.h>

#ifdef __cplusplus
extern "C" {
#endif

enum _bc_ldil_log_level{
        LDIL_ERR            = 0x80000000,   /* Don't disable this option */

        /* Following are allowed only in debug mode */
        LDIL_INFO             = 0x00000001,   /* Generic informational */
        LDIL_DBG              = 0x00000002,   /* First level Debug info */
};

#define LDIL_PRINTS_ON 1
#if LDIL_PRINTS_ON
#define DebugLog_Trace(_tl, fmt, args...)	printf(fmt, ##args);
#else
#define DebugLog_Trace(_tl, fmt, args...)
#endif
#define bc_sleep_ms(x)	usleep(1000*x)

/* Some of the globals from bc_dts_glob.h. are defined
 * here that are applicable only to ldil.
 */
enum _crystalhd_ldil_globals {
	BC_EOS_PIC_COUNT	= 16,			/* EOS check counter..*/
	BC_INPUT_MDATA_POOL_SZ  = 4560,			/* Input Meta Data Pool size */
	BC_MAX_SW_VOUT_BUFFS    = BC_RX_LIST_CNT,	/* MAX - pre allocated buffers..*/
	RX_START_DELIVERY_THRESHOLD = 0,
	PAUSE_DECODER_THRESHOLD = 12,
	RESUME_DECODER_THRESHOLD = 5,
};

enum _BC_PCI_DEV_IDS {
	BC_PCI_DEVID_INVALID	= 0,
	BC_PCI_DEVID_DOZER	= 0x1610,
	BC_PCI_DEVID_TANK	= 0x1620,
	BC_PCI_DEVID_LINK	= 0x1612,
	BC_PCI_DEVID_LOCKE	= 0x1613,
	BC_PCI_DEVID_DEMOBRD	= 0x7411,
	BC_PCI_DEVID_MORPHEUS	= 0x7412,
};

enum _DtsRunState {
	BC_DEC_STATE_INVALID	= 0x00,
	BC_DEC_STATE_OPEN	= 0x01,
	BC_DEC_STATE_START	= 0x02,
	BC_DEC_STATE_PAUSE	= 0x03,
	BC_DEC_STATE_STOP	= 0x04
};

/* Bit fields */
enum _BCMemTypeFlags {
        BC_MEM_DEC_YUVBUFF             = 0x1,
	BC_MEM_USER_MODE_ALLOC	= 0x80000000,
};

/* Application specific run-time configurations */
enum _DtsAppSpecificCfgFlags {
	BC_MPOOL_FLAGS_DEF	= 0x000,
	BC_MPOOL_INCL_YUV_BUFFS	= 0x001,	/* Include YUV Buffs allocation */
//	BC_DEC_EN_DUART		= 0x002,	/* Enable DUART for FW log */
	BC_DEC_INIT_MEM		= 0x004,	/* Initialize Entire DRAM takes about a min */
	BC_DEC_VCLK_74MHZ	= 0x008,	/* Enable Vidoe clock to 75 MHZ */
	BC_EN_DIAG_MODE		= 0x010,	/* Enable Diag Mode application */
	BC_PIX_WID_1080		= 0x020,	/* FIX_ME: deprecate this after PIB work */
//	BC_ADDBUFF_MOVE		= 0x040,	/* FIX_ME: Deleteme after testing.. */
	BC_DEC_VCLK_77MHZ	= 0x080		/* Enable Vidoe clock to 77 MHZ */

};

#define BC_DTS_DEF_CFG		(BC_MPOOL_INCL_YUV_BUFFS | BC_EN_DIAG_MODE |  BC_DEC_VCLK_74MHZ)

/* !!!!Note!!!!
 * Don't forget to change this value
 * while changing the file names
 */
#define BC_MAX_FW_FNAME_SIZE	32
#define MAX_PATH		256

#define TSHEXFILE		"stream.hex"
#define DECOHEXFILE		"vdec_outer.hex"
#define DECIHEXFILE		"vdec_inner.hex"
#define FWBINFILE		"bcmDecFw.bin"
//#define FWBINFILE_LNK	"link_fw.bin"
#define FWBINFILE_LNK	"bcmFilePlayFw.bin"
#define FWBIN_FILE_PLAY_LNK	"bcmFilePlayFw.bin"

#define BC_DTS_DEF_OPTIONS	0x0D
#define BC_DTS_DEF_OPTIONS_LINK	0xB0000005

typedef struct _DTS_MPOOL_TYPE {
	uint32_t	type;
	uint32_t	sz;
	uint8_t		*buff;
} DTS_MPOOL_TYPE;

#define LIB_CTX_SIG	0x11223344

typedef struct _DTS_VIDEO_PARAMS {
	uint32_t	VideoAlgo;
	uint32_t	WidthInPixels;
	uint32_t	HeightInPixels;
	BOOL		FGTEnable;
	BOOL		MetaDataEnable;
	BOOL		Progressive;
	BOOL		FrameRate; //currently not used, frame rate is passed in the 1st byte of the OptFlags member
	uint32_t	OptFlags; //currently has the DEc_operation_mode in bits 4 and 5, bits 0:3 have the default framerate
} DTS_VIDEO_PARAMS;

/* Input MetaData handling.. */
typedef struct _BC_SEQ_HDR_FORMAT{
	uint8_t		StartCode[4];
	uint8_t		PacketLen;
	uint8_t		StartCodeEnd;
	uint8_t		SeqNum[2];
	uint8_t		Command;
	uint8_t		PicLength[2];
	uint8_t		Reserved;
}BC_SEQ_HDR_FORMAT;

typedef struct _BC_PES_HDR_FORMAT{
	uint8_t		StartCode[4];
	uint8_t		PacketLen[2];
	uint8_t		OptPesHdr[2];
	uint8_t		optPesHdrLen;
}BC_PES_HDR_FORMAT;

typedef struct _DTS_INPUT_MDATA{
	struct _DTS_INPUT_MDATA		*flink;
	struct _DTS_INPUT_MDATA		*blink;
	uint32_t							IntTag;
	uint32_t							Reserved;
	uint64_t							appTimeStamp;
	BC_SEQ_HDR_FORMAT			Spes;
}DTS_INPUT_MDATA;


#define DTS_MDATA_PEND_LINK(_c)	( (DTS_INPUT_MDATA *)&_c->MDPendHead )

#define DTS_MDATA_TAG_MASK		(0x00010000)
#define DTS_MDATA_MAX_TAG		(0x0000FFFF)

typedef struct _DTS_LIB_CONTEXT{
	uint32_t				Sig;			/* Mazic number */
	uint32_t				State;			/* DIL's Run State */
	int				DevHandle;		/* Driver handle */
	BC_IOCTL_DATA	*pIoDataFreeHd;	/* IOCTL data pool head */
	DTS_MPOOL_TYPE	*Mpools;		/* List of memory pools created */
	uint32_t				MpoolCnt;		/* Number of entries */
	uint32_t				CfgFlags;		/* Application specifi flags */
	uint32_t				OpMode;			/* Mode of operation playback etc..*/
	uint32_t				DevId;			/* HW Device ID */
	uint32_t				hwRevId;		/* HW revision ID */
	uint32_t				fwcmdseq;		/* FW Cmd Sequence number */
	uint32_t				FixFlags;		/* Flags for conditionally enabling fixes */

	pthread_mutex_t  thLock;



	DTS_VIDEO_PARAMS VidParams;		/* App specific Video Params */

	DecRspChannelStreamOpen OpenRsp;/* Channel Open Response */
	DecRspChannelStartVideo	sVidRsp;/* Start Video Response */

	/* Proc Output Related */
	BOOL			ProcOutPending;	/* To avoid muliple ProcOuts */
	BOOL			CancelWaiting;	/* Notify FetchOut to signal */
	sem_t			CancelProcOut;	/* Cancel outstanding ProcOut Request */

	/* pOutData is dedicated for ProcOut() use only. Every other
	 * Interface should use the memory from IocData pool. This
	 * is to provide priority for ProcOut() interface due to its
	 * performance criticality.
	 *
	 * !!!!!NOTE!!!!
	 *
	 * Using this data structures for other interfaces, will cause
	 * thread related race conditions.
	 */
	BC_IOCTL_DATA	*pOutData;		/* Current Active Proc Out Buffer */

	/* Place Holder for FW file paths */
	char				StreamFile[MAX_PATH+1];
	char				VidInner[MAX_PATH+1];
	char				VidOuter[MAX_PATH+1];

	uint32_t				InMdataTag;				/* InMetaData Tag for TimeStamp */
	void			*MdataPoolPtr;			/* allocated memory PoolPointer */

	struct _DTS_INPUT_MDATA	*MDFreeHead;	/* MetaData Free List Head */

	struct _DTS_INPUT_MDATA	*MDPendHead;	/* MetaData Pending List Head */
	struct _DTS_INPUT_MDATA	*MDPendTail;	/* MetaData Pending List Tail */
	uint32_t			MDPendCount;

	/* End Of Stream detection */
	BOOL			FlushIssued;			/* Flag to start EOS detection */
	uint32_t				eosCnt;					/* Last picture repetition count */
	uint32_t				LastPicNum;				/* Last picture number */

	/* Statistics Related */
	uint32_t				prevPicNum;				/* Previous received frame */
	uint32_t				CapState;				/* 0 = Not started, 1 = Interlaced, 2 = progressive */
	uint32_t				PibIntToggle;			/* Toggle flag to detect PIB miss in Interlaced mode.*/
	uint32_t				prevFrameRate;			/* Previous frame rate */

	BC_REG_CONFIG	RegCfg;					/* Registry Configurable options.*/

	char				FwBinFile[MAX_PATH+1];	/* Firmware Bin file place holder */

	uint8_t				b422Mode;				/* 422 Mode Identifier for Link */
	uint32_t				picWidth;
	uint32_t				picHeight;

	char				DilPath[MAX_PATH+1];	/* DIL runtime Location.. */

	/* Power management and dynamic clock frequency changes related */
	uint8_t				totalPicsCounted;
	uint8_t				rptPicsCounted;
	uint8_t				nrptPicsCounted;
	uint8_t				numTimesClockChanged;
	uint8_t				minClk;
	uint8_t				maxClk;
	uint8_t				curClk;

}DTS_LIB_CONTEXT;

/* Helper macro to get lib context from user handle */
#define DTS_GET_CTX(_uh, _c)											\
{																		\
	if( !(_c = DtsGetContext(_uh)) ){									\
		return BC_STS_INV_ARG;											\
	}																	\
}


extern DTS_LIB_CONTEXT	*	DtsGetContext(HANDLE userHnd);
BOOL DtsIsPend(DTS_LIB_CONTEXT	*Ctx);
BOOL DtsDrvIoctl
(
	  HANDLE	hDevice,
	  DWORD		dwIoControlCode,
	  LPVOID	lpInBuffer,
	  DWORD		nInBufferSize,
	  LPVOID	lpOutBuffer,
	  DWORD		nOutBufferSize,
	  LPDWORD	lpBytesReturned,
	  BOOL		Async
);
BC_STATUS DtsDrvCmd(DTS_LIB_CONTEXT	*Ctx, DWORD Code, BOOL Async, BC_IOCTL_DATA *pIoData, BOOL Rel);
void DtsRelIoctlData(DTS_LIB_CONTEXT *Ctx, BC_IOCTL_DATA *pIoData);
BC_IOCTL_DATA *DtsAllocIoctlData(DTS_LIB_CONTEXT *Ctx);
BC_STATUS DtsAllocMemPools(DTS_LIB_CONTEXT *Ctx);
void DtsReleaseMemPools(DTS_LIB_CONTEXT *Ctx);
BC_STATUS DtsAddOutBuff(DTS_LIB_CONTEXT *Ctx, PVOID buff, uint32_t flags);
BC_STATUS DtsRelRxBuff(DTS_LIB_CONTEXT *Ctx, BC_DEC_YUV_BUFFS *buff,BOOL SkipAddBuff);
BC_STATUS DtsFetchOutInterruptible(DTS_LIB_CONTEXT *Ctx, BC_DTS_PROC_OUT *DecOut, uint32_t dwTimeout);
BC_STATUS DtsCancelFetchOutInt(DTS_LIB_CONTEXT *Ctx);
BC_STATUS DtsMapYUVBuffs(DTS_LIB_CONTEXT *Ctx);
BC_STATUS DtsInitInterface(int hDevice,HANDLE *RetCtx, uint32_t mode);
BC_STATUS DtsSetupConfig(DTS_LIB_CONTEXT *Ctx, uint32_t did, uint32_t rid, uint32_t FixFlags);
BC_STATUS DtsReleaseInterface(DTS_LIB_CONTEXT *Ctx);
BC_STATUS DtsGetBCRegConfig(DTS_LIB_CONTEXT	*Ctx);
BC_STATUS DtsGetFirmwareFiles(DTS_LIB_CONTEXT	*Ctx);
DTS_INPUT_MDATA	*DtsAllocMdata(DTS_LIB_CONTEXT *Ctx);
BC_STATUS DtsFreeMdata(DTS_LIB_CONTEXT *Ctx, DTS_INPUT_MDATA	*Mdata, BOOL sync);
BC_STATUS DtsClrPendMdataList(DTS_LIB_CONTEXT *Ctx);
BC_STATUS DtsInsertMdata(DTS_LIB_CONTEXT *Ctx, DTS_INPUT_MDATA	*Mdata);
BC_STATUS DtsRemoveMdata(DTS_LIB_CONTEXT *Ctx, DTS_INPUT_MDATA	*Mdata, BOOL sync);
BC_STATUS DtsFetchMdata(DTS_LIB_CONTEXT *Ctx, uint16_t snum, BC_DTS_PROC_OUT *pout);
BC_STATUS DtsPrepareMdata(DTS_LIB_CONTEXT *Ctx, uint64_t timeStamp, DTS_INPUT_MDATA **mData);
BC_STATUS DtsPrepareMdataASFHdr(DTS_LIB_CONTEXT *Ctx, DTS_INPUT_MDATA *mData, uint8_t* buf);
BC_STATUS DtsNotifyOperatingMode(HANDLE hDevice,uint32_t Mode);
/*====================== Performance Counter Routines ============================*/
void DtsUpdateInStats(DTS_LIB_CONTEXT	*Ctx, uint32_t	size);
void DtsUpdateOutStats(DTS_LIB_CONTEXT	*Ctx, BC_DTS_PROC_OUT *pOut);

/*====================== Debug Routines ========================================*/
void DtsTestMdata(DTS_LIB_CONTEXT	*gCtx);
BOOL DtsDbgCheckPointers(DTS_LIB_CONTEXT *Ctx,BC_IOCTL_DATA *pIo);

/*============== Global shared area usage ======================*/

#define BC_DIL_HWINIT_IN_PROGRESS 1
#ifdef _USE_SHMEM_
#define BC_DIL_SHMEM_KEY 0xBABEFACE

typedef struct _bc_dil_glob_s{
	uint32_t gDilOpMode;
	uint32_t gHwInitSts;
	BC_DTS_STATS stats;
} bc_dil_glob_s;


BC_STATUS DtsGetDilShMem(uint32_t shmid);
BC_STATUS DtsDelDilShMem(void);
BC_STATUS DtsCreateShMem(int *shmem_id);
#endif
uint32_t DtsGetOPMode( void );
void DtsSetOPMode( uint32_t value );
uint32_t DtsGetHwInitSts( void );
void DtsSetHwInitSts( uint32_t value );
void DtsRstStats( void ) ;
BC_DTS_STATS * DtsGetgStats ( void );

#ifdef __cplusplus
}
#endif

#endif

