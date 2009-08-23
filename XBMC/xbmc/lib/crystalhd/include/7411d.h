/***************************************************************************
 *     Copyright (c) 2004-2006, Broadcom Corporation
 *     All Rights Reserved
 *     Confidential Property of Broadcom Corporation
 *
 *  THIS SOFTWARE MAY ONLY BE USED SUBJECT TO AN EXECUTED SOFTWARE LICENSE
 *  AGREEMENT  BETWEEN THE USER AND BROADCOM.  YOU HAVE NO RIGHT TO USE OR
 *  EXPLOIT THIS MATERIAL EXCEPT SUBJECT TO THE TERMS OF SUCH AN AGREEMENT.
 *
 * $brcm_Workfile: c011api.h $
 * $brcm_Revision: sw_branch_7411_d0/sw_branch_7412_a0/sw_branch_7412_a0_apple/2 $
 * $brcm_Date: 10/25/06 5:27p $
 *
 * Module Description:
 *   See Module Overview below.
 *
 * Revision History:
 *
 * $brcm_Log: /brickstone/sw/emb/stream/c011api.h $
 * 
 * sw_branch_7411_d0/sw_branch_7412_a0/sw_branch_7412_a0_apple/2   10/25/06 5:27p xshi
 * Code clean: removed sdramInputBuf0/1 in channelOpenRsp; obsoleted
 * sdramInputDmaBufferSize in startVideoCmd.
 * 
 * sw_branch_7411_d0/sw_branch_7412_a0/sw_branch_7412_a0_apple/1   10/19/06 10:16a baginski
 * More Vista modifications
 * 
 * sw_branch_7411_d0/sw_branch_7412_a0/7   9/26/06 1:54p baginski
 * Merge from sw_branch_7411_d0 via sw_branch_settop_firmware_7411d0
 * 
 * sw_branch_7411_d0/66   9/26/06 1:45p baginski
 * Merge from sw_branch_settop_firmware_7411d0 -
 * REL_CANDIDATE_C0_003_005_006_2006_09_26
 * 
 * sw_branch_7411_d0/65   9/18/06 11:50a baginski
 * Merge 7412 Vista Modifications
 * 
 * sw_branch_7411_d0/sw_branch_7412_a0/6   8/24/06 11:38a baginski
 * Merge from FW_7411D_DVD_4_4_25
 * 
 * sw_branch_7411_d0/sw_branch_7412_a0/5   8/3/06 1:53p xshi
 * code optimization:
 * (1) add "dramInputEnabled" in channel context;
 * (2) move the struct "dmaXferCtrlInfo" from c011api.h to stream.h;
 * (3) swap the bit-field packing order of "dmaXferCtrlInfo";
 * (4) always set "IncDst" bit if dstAddr of CIB is zero
 * 
 * sw_branch_7411_d0/sw_branch_7412_a0/4   7/31/06 5:39p baginski
 * Broadcom ECG Mode - Phase 1,2
 * - ts fake header support
 * - sequence number attachment for H.264 bitstreams
 * - chroma upsampling turned off
 * 
 * sw_branch_7411_d0/sw_branch_7412_a0/3   7/27/06 2:32p xshi
 * Obsolete param "sdramInputDmaBufferSize" of channelOpen
 * 
 * sw_branch_7411_d0/sw_branch_7412_a0/2   7/18/06 11:44a xshi
 * add support for sid interrupt.
 * 
 * sw_branch_7411_d0/sw_branch_7412_a0/1   7/13/06 2:18p xshi
 * pci dma API: (1) data struct of Control Info; (2) CIQ; (3) cmd/rsp
 * change; and (4) dma handling on stream arc
 * 
 * sw_branch_7411_d0/62   6/26/06 10:23a xshi
 * SID program space allocation and init. Note that the program space
 * starts at 0x302000 and has a length of about 1 Mbytes, whichn used to
 * be encoder's program space. In future if ENC is brought back we need
 * to re-tune the memory.
 * 
 * sw_branch_7411_d0/61   6/23/06 2:04p xshi
 * SID memory allocation
 * 
 * sw_branch_7411_d0/60   6/21/06 4:24p baginski
 * Set Clipping Command
 * 
 * sw_branch_7411_d0/59   6/15/06 1:45p baginski
 * add picture done payload field to channel status block
 * 
 * sw_branch_7411_d0/58   6/1/06 11:16a xshi
 * (1) Added new DramLogEnable in C011CmdInit; and return dramLogBase(s)
 * and dramLogSize(s) in C011RspInit
 * (2) Removed trickPlayBuf away from channelOpen/startVideo
 * 
 * sw_branch_7411_d0/57   5/25/06 2:03p xshi
 * (1) async event renaming; (2) added cmd respond int to bit-map
 * 
 * sw_branch_7411_d0/56   5/23/06 2:07p baginski
 * Merge settop c011api.h changes - to form a single shared c011api.h
 * 
 * sw_branch_7411_d0/55   5/23/06 10:40a xshi
 * restore the para "reserved" of channelOperationMode
 * 
 * sw_branch_7411_d0/54   5/23/06 10:31a xshi
 * (1) remove picReadyInt and picSetupInt of channelopen/startvideo
 * (2) cleanup the code correspondingly
 *
 ***************************************************************************/
#ifndef __INC_C011API_H__
#define __INC_C011API_H__

#include "vdec_info.h"

// maximum number of host commands and responses
#define C011_MAX_HST_CMDS           (16)
#define C011_MAX_HST_RSPS           (16)
#define C011_MAX_HST_CMDQ_SIZE      (64)

// default success return code
#define C011_RET_SUCCESS            (0x0)

// default failure return code
#define C011_RET_FAILURE            (0xFFFFFFFF)

#define C011_RET_UNKNOWN            (0x1)

// Stream ARC base address
#define STR_BASE                    (0x00000000)

// Stream ARC <- Host (default) address
#define STR_HOSTRCV                 (STR_BASE + 0x100)

// Stream ARC -> Host address
#define STR_HOSTSND                 (STR_BASE + 0x200)

#define eCMD_C011_CMD_BASE          (0x73763000)

/* host commands */
typedef enum
{

    eCMD_TS_GET_NEXT_PIC                = 0x7376F100, // debug get next picture
    eCMD_TS_GET_LAST_PIC                = 0x7376F102, // debug get last pic status
    eCMD_TS_READ_WRITE_MEM              = 0x7376F104, // debug read write memory

    /* New API commands */
    /* General commands */
    eCMD_C011_INIT                      = eCMD_C011_CMD_BASE + 0x01,
    eCMD_C011_RESET                     = eCMD_C011_CMD_BASE + 0x02,
    eCMD_C011_SELF_TEST                 = eCMD_C011_CMD_BASE + 0x03,
    eCMD_C011_GET_VERSION               = eCMD_C011_CMD_BASE + 0x04,
    eCMD_C011_GPIO                      = eCMD_C011_CMD_BASE + 0x05,
    eCMD_C011_DEBUG_SETUP               = eCMD_C011_CMD_BASE + 0x06,

    /* Decoding commands */
    eCMD_C011_DEC_CHAN_OPEN             = eCMD_C011_CMD_BASE + 0x100,
    eCMD_C011_DEC_CHAN_CLOSE            = eCMD_C011_CMD_BASE + 0x101,
    eCMD_C011_DEC_CHAN_ACTIVATE         = eCMD_C011_CMD_BASE + 0x102,
    eCMD_C011_DEC_CHAN_STATUS           = eCMD_C011_CMD_BASE + 0x103,
    eCMD_C011_DEC_CHAN_FLUSH            = eCMD_C011_CMD_BASE + 0x104,
    eCMD_C011_DEC_CHAN_TRICK_PLAY       = eCMD_C011_CMD_BASE + 0x105,
    eCMD_C011_DEC_CHAN_TS_PIDS          = eCMD_C011_CMD_BASE + 0x106,
    eCMD_C011_DEC_CHAN_PS_STREAM_ID     = eCMD_C011_CMD_BASE + 0x107,
    eCMD_C011_DEC_CHAN_INPUT_PARAMS     = eCMD_C011_CMD_BASE + 0x108,
    eCMD_C011_DEC_CHAN_VIDEO_OUTPUT     = eCMD_C011_CMD_BASE + 0x109,
    eCMD_C011_DEC_CHAN_OUTPUT_FORMAT    = eCMD_C011_CMD_BASE + 0x10A,
    eCMD_C011_DEC_CHAN_SCALING_FILTERS  = eCMD_C011_CMD_BASE + 0x10B,
    eCMD_C011_DEC_CHAN_OSD_MODE         = eCMD_C011_CMD_BASE + 0x10D,
    eCMD_C011_DEC_CHAN_DROP             = eCMD_C011_CMD_BASE + 0x10E,
    eCMD_C011_DEC_CHAN_RELEASE          = eCMD_C011_CMD_BASE + 0x10F,
    eCMD_C011_DEC_CHAN_STREAM_SETTINGS  = eCMD_C011_CMD_BASE + 0x110,
    eCMD_C011_DEC_CHAN_PAUSE_OUTPUT     = eCMD_C011_CMD_BASE + 0x111,
    eCMD_C011_DEC_CHAN_CHANGE           = eCMD_C011_CMD_BASE + 0x112,
    eCMD_C011_DEC_CHAN_SET_STC          = eCMD_C011_CMD_BASE + 0x113,
    eCMD_C011_DEC_CHAN_SET_PTS          = eCMD_C011_CMD_BASE + 0x114,
    eCMD_C011_DEC_CHAN_CC_MODE          = eCMD_C011_CMD_BASE + 0x115,
    eCMD_C011_DEC_CREATE_AUDIO_CONTEXT  = eCMD_C011_CMD_BASE + 0x116,
    eCMD_C011_DEC_COPY_AUDIO_CONTEXT    = eCMD_C011_CMD_BASE + 0x117,
    eCMD_C011_DEC_DELETE_AUDIO_CONTEXT  = eCMD_C011_CMD_BASE + 0x118,
    eCMD_C011_DEC_CHAN_SET_DECYPTION    = eCMD_C011_CMD_BASE + 0x119,
    eCMD_C011_DEC_CHAN_START_VIDEO      = eCMD_C011_CMD_BASE + 0x11A,
    eCMD_C011_DEC_CHAN_STOP_VIDEO       = eCMD_C011_CMD_BASE + 0x11B,
    eCMD_C011_DEC_CHAN_PIC_CAPTURE      = eCMD_C011_CMD_BASE + 0x11C,
    eCMD_C011_DEC_CHAN_PAUSE            = eCMD_C011_CMD_BASE + 0x11D,
    eCMD_C011_DEC_CHAN_PAUSE_STATE      = eCMD_C011_CMD_BASE + 0x11E,
    eCMD_C011_DEC_CHAN_SET_SLOWM_RATE   = eCMD_C011_CMD_BASE + 0x11F,
    eCMD_C011_DEC_CHAN_GET_SLOWM_RATE   = eCMD_C011_CMD_BASE + 0x120,
    eCMD_C011_DEC_CHAN_SET_FF_RATE      = eCMD_C011_CMD_BASE + 0x121,
    eCMD_C011_DEC_CHAN_GET_FF_RATE      = eCMD_C011_CMD_BASE + 0x122,
    eCMD_C011_DEC_CHAN_FRAME_ADVANCE    = eCMD_C011_CMD_BASE + 0x123,
    eCMD_C011_DEC_CHAN_SET_SKIP_PIC_MODE        = eCMD_C011_CMD_BASE + 0x124,
    eCMD_C011_DEC_CHAN_GET_SKIP_PIC_MODE        = eCMD_C011_CMD_BASE + 0x125,
    eCMD_C011_DEC_CHAN_FILL_PIC_BUF             = eCMD_C011_CMD_BASE + 0x126,
    eCMD_C011_DEC_CHAN_SET_CONTINUITY_CHECK     = eCMD_C011_CMD_BASE + 0x127,
    eCMD_C011_DEC_CHAN_GET_CONTINUITY_CHECK     = eCMD_C011_CMD_BASE + 0x128,
    eCMD_C011_DEC_CHAN_SET_BRCM_TRICK_MODE    	= eCMD_C011_CMD_BASE + 0x129,
    eCMD_C011_DEC_CHAN_GET_BRCM_TRICK_MODE      = eCMD_C011_CMD_BASE + 0x12A,
    eCMD_C011_DEC_CHAN_REVERSE_FIELD_STATUS     = eCMD_C011_CMD_BASE + 0x12B,
    eCMD_C011_DEC_CHAN_I_PICTURE_FOUND          = eCMD_C011_CMD_BASE + 0x12C,
    eCMD_C011_DEC_CHAN_SET_PARAMETER            = eCMD_C011_CMD_BASE + 0x12D,
    eCMD_C011_DEC_CHAN_SET_USER_DATA_MODE       = eCMD_C011_CMD_BASE + 0x12E,
    eCMD_C011_DEC_CHAN_SET_PAUSE_DISPLAY_MODE   = eCMD_C011_CMD_BASE + 0x12F,
    eCMD_C011_DEC_CHAN_SET_SLOW_DISPLAY_MODE    = eCMD_C011_CMD_BASE + 0x130,
    eCMD_C011_DEC_CHAN_SET_FF_DISPLAY_MODE		= eCMD_C011_CMD_BASE + 0x131,
    eCMD_C011_DEC_CHAN_SET_DISPLAY_TIMING_MODE 	= eCMD_C011_CMD_BASE + 0x132,
    eCMD_C011_DEC_CHAN_SET_DISPLAY_MODE 		   = eCMD_C011_CMD_BASE + 0x133,
    eCMD_C011_DEC_CHAN_GET_DISPLAY_MODE 		   = eCMD_C011_CMD_BASE + 0x134,
    eCMD_C011_DEC_CHAN_SET_REVERSE_FIELD 		   = eCMD_C011_CMD_BASE + 0x135,
    eCMD_C011_DEC_CHAN_STREAM_OPEN              = eCMD_C011_CMD_BASE + 0x136,
    eCMD_C011_DEC_CHAN_SET_PCR_PID              = eCMD_C011_CMD_BASE + 0x137,
    eCMD_C011_DEC_CHAN_SET_VID_PID              = eCMD_C011_CMD_BASE + 0x138,
    eCMD_C011_DEC_CHAN_SET_PAN_SCAN_MODE        = eCMD_C011_CMD_BASE + 0x139,
    eCMD_C011_DEC_CHAN_START_DISPLAY_AT_PTS     = eCMD_C011_CMD_BASE + 0x140,
    eCMD_C011_DEC_CHAN_STOP_DISPLAY_AT_PTS      = eCMD_C011_CMD_BASE + 0x141,
    eCMD_C011_DEC_CHAN_SET_DISPLAY_ORDER        = eCMD_C011_CMD_BASE + 0x142,
    eCMD_C011_DEC_CHAN_GET_DISPLAY_ORDER        = eCMD_C011_CMD_BASE + 0x143,
    eCMD_C011_DEC_CHAN_SET_HOST_TRICK_MODE      = eCMD_C011_CMD_BASE + 0x144,
    eCMD_C011_DEC_CHAN_SET_OPERATION_MODE       = eCMD_C011_CMD_BASE + 0x145,
    eCMD_C011_DEC_CHAN_DISPLAY_PAUSE_UNTO_PTS   = eCMD_C011_CMD_BASE + 0x146,
    eCMD_C011_DEC_CHAN_SET_PTS_STC_DIFF_THRESHOLD = eCMD_C011_CMD_BASE + 0x147,
    eCMD_C011_DEC_CHAN_SEND_COMPRESSED_BUF      = eCMD_C011_CMD_BASE + 0x148,
    eCMD_C011_DEC_CHAN_SET_CLIPPING		        = eCMD_C011_CMD_BASE + 0x149,
    eCMD_C011_DEC_CHAN_SET_PARAMETERS_FOR_HARD_RESET_INTERRUPT_TO_HOST = eCMD_C011_CMD_BASE + 0x150,

    /* Decoder RevD commands */
    eCMD_C011_DEC_CHAN_SET_CSC          = eCMD_C011_CMD_BASE + 0x180,  /* CSC:color space conversion */
    eCMD_C011_DEC_CHAN_SET_RANGE_REMAP  = eCMD_C011_CMD_BASE + 0x181,
    eCMD_C011_DEC_CHAN_SET_FGT			= eCMD_C011_CMD_BASE + 0x182,
    eCMD_C011_DEC_CHAN_SET_LASTPICTURE_PADDING	= eCMD_C011_CMD_BASE + 0x183,  // not implemented yet in Rev D main

    /* Decoder 7412 commands */
    eCMD_C011_DEC_CHAN_SET_CONTENT_KEY	= eCMD_C011_CMD_BASE + 0x190,  // 7412 only
    eCMD_C011_DEC_CHAN_SET_SESSION_KEY	= eCMD_C011_CMD_BASE + 0x191,  // 7412 only
    eCMD_C011_DEC_CHAN_FMT_CHANGE_ACK	= eCMD_C011_CMD_BASE + 0x192,  // 7412 only


    eCMD_C011_DEC_CHAN_CUSTOM_VIDOUT    = eCMD_C011_CMD_BASE + 0x1FF,
    /* Encoding commands */
    eCMD_C011_ENC_CHAN_OPEN             = eCMD_C011_CMD_BASE + 0x200,
    eCMD_C011_ENC_CHAN_CLOSE            = eCMD_C011_CMD_BASE + 0x201,
    eCMD_C011_ENC_CHAN_ACTIVATE         = eCMD_C011_CMD_BASE + 0x202,
    eCMD_C011_ENC_CHAN_CONTROL          = eCMD_C011_CMD_BASE + 0x203,
    eCMD_C011_ENC_CHAN_STATISTICS       = eCMD_C011_CMD_BASE + 0x204,

    eNOTIFY_C011_ENC_CHAN_EVENT         = eCMD_C011_CMD_BASE + 0x210,

} eC011_TS_CMD;

/* ARCs */
/*  - eCMD_C011_INIT */
#define C011_STREAM_ARC             (0x00000001) /* stream ARC */
#define C011_VDEC_ARC               (0x00000002) /* video decoder ARC */
#define C011_SID_ARC                (0x00000004) /* SID ARC */

/* Interrupt Status Register definition for HostControlled mode:
 * 16 bits available for general use, bit-31 dedicated for MailBox Interrupt!
 */
#define RESERVED_FOR_FUTURE_USE_0   (1<<0)  /* BIT 0 */
#define PICTURE_INFO_AVAILABLE      (1<<1)
#define PICTURE_DONE_MARKER         (1<<2)
#define ASYNC_EVENTQ	            (1<<3)
#define INPUT_DMA_DONE              (1<<4)
#define OUTPUT_DMA_DONE             (1<<5)
#define VIDEO_DATA_UNDERFLOW_IN0    (1<<6)
#define CRC_DATA_AVAILABLE_IN0      (1<<7)
#define SID_SERVICE                 (1<<8)
#define USER_DATA_AVAILABLE_IN0     (1<<9)
#define NEW_PCR_OFFSET              (1<<10) /* New PCR Offset received */
#define RESERVED_FOR_FUTURE_USE_2   (1<<11)
#define HOST_DMA_COMPLETE           (1<<12)
#define RAPTOR_SERVICE              (1<<13)
#define INITIAL_PTS                 (1<<14) /* STC Request Interrupt */
#define PTS_DISCONTINUITY           (1<<15) /* PTS Error Interrupt */

#define COMMAND_RESPONSE            (1<<31) /* Command Response Register Interrupt */

/* Asynchronous Events - enabled via ChannelOpen */
#define EVENT_PRESENTATION_START	   (0x00000001)
#define EVENT_PRESENTATION_STOP		(0x00000002)
#define EVENT_ILLEGAL_STREAM        (0x00000003)
#define EVENT_UNDERFLOW             (0x00000004)
#define EVENT_VSYNC_ERROR 	         (0x00000005)
#define INPUT_DMA_BUFFER0_RELEASE  (0x00000006)
#define INPUT_DMA_BUFFER1_RELEASE  (0x00000007)


/* interrupt control */
/*  - eCMD_C011_INIT */
typedef enum
{
    eC011_INT_DISABLE           = 0x00000000,
    eC011_INT_ENABLE            = 0x00000001,
    eC011_INT_ENABLE_RAPTOR     = 0x00000003,
} eC011_INT_CONTROL;


/*mpcDiskformatBD*/
/*  - eCMD_C011_INIT */
#define eC011_DSK_BLURAY (1<<10)
#define eC011_RES_MASK   0x0000F800

/* test id */
/*  - eCMD_C011_SELF_TEST */
typedef enum
{
    eC011_TEST_SHORT_MEMORY     = 0x00000001,
    eC011_TEST_LONG_MEMORY      = 0x00000002,
    eC011_TEST_SHORT_REGISTER   = 0x00000003,
    eC011_TEST_LONG_REGISTER    = 0x00000004,
    eC011_TEST_DECODE_LOOPBACK  = 0x00000005,
    eC011_TEST_ENCODE_LOOPBACK  = 0x00000006,
} eC011_TEST_ID;

/* gpio control */
/*  - eCMD_C011_GPIO */
typedef enum
{
    eC011_GPIO_CONTROL_INTERNAL = 0x00000000,
    eC011_GPIO_CONTROL_HOST     = 0x00000001,
} eC011_GPIO_CONTROL;

/* input port */
/*  - eCMD_C011_DEC_CHAN_OPEN */
typedef enum
{
    eC011_IN_PORT0              = 0x00000000, // input port 0
    eC011_IN_PORT1              = 0x00000001, // input port 1
    eC011_IN_HOST_PORT0         = 0x00000010, // host port (OR this bit to specify which port is host mode)
    eC011_IN_HOST_PORT1         = 0x00000011, // host port (OR this bit to specify which port is host mode)
    eC011_IN_DRAM               = 0x00000100, // SDRAM
} eC011_INPUT_PORT;

/* output port */
/*  - eCMD_C011_DEC_CHAN_OPEN */
typedef enum
{
    eC011_OUT_PORT0             = 0x00000000, // output port 0
    eC011_OUT_PORT1             = 0x00000001, // output port 1
    eC011_OUT_BOTH              = 0x00000002, // output port 0 and 1
    eC011_OUT_HOST              = 0x00000010, // host port (OR this bit to specify which port is host mode)
} eC011_OUTPUT_PORT;

/* stream types */
/*  - eCMD_C011_DEC_CHAN_OPEN */
typedef enum
{
    eC011_STREAM_TYPE_ES          = 0x00000000, // elementary stream
    eC011_STREAM_TYPE_PES         = 0x00000001, // packetized elementary stream
    eC011_STREAM_TYPE_TS          = 0x00000002, // transport stream
    eC011_STREAM_TYPE_TSD_ES      = 0x00000003, // legacy 130-byte transport stream with ES
    eC011_STREAM_TYPE_TSD_PES     = 0x00000004, // legacy 130-byte transport stream with PES
    eC011_STREAM_TYPE_CMS         = 0x00000005, // compressed multistream
    eC011_STREAM_TYPE_ES_W_TSHDR  = 0x00000006, // elementary stream with fixed TS headers

    eC011_STREAM_TYPE_ES_DBG      = 0x80000000, // debug elementary stream
    eC011_STREAM_TYPE_PES_DBG     = 0x80000001, // debug packetized elementary stream
    eC011_STREAM_TYPE_TS_DBG      = 0x80000002, // debug transport stream
    eC011_STREAM_TYPE_TSD_ES_DBG  = 0x80000003, // debug legacy 130-byte transport stream with ES
    eC011_STREAM_TYPE_TSD_PES_DBG = 0x80000004, // debug legacy 130-byte transport stream with PES
    eC011_STREAM_TYPE_CMS_DBG     = 0x80000005, // debug compressed multistream
    eC011_STREAM_TYPE_ES_FIXED_TS_DBG = 0x80000006, // debug elementary stream with fixed TS headers

} eC011_STREAM_TYPE;

/* maximum picture size */
/*  - eCMD_C011_DEC_CHAN_OPEN */
typedef enum
{
    eC011_MAX_PICSIZE_HD        = 0x00000000, // 1920x1088
    eC011_MAX_PICSIZE_SD        = 0x00000001, // 720x576
} eC011_MAX_PICSIZE;

/* output control mode */
/*  - eCMD_C011_DEC_CHAN_OPEN */
typedef enum
{
    eC011_OUTCTRL_VIDEO_TIMING  = 0x00000000,
    eC011_OUTCTRL_HOST_TIMING   = 0x00000001,
} eC011_OUTCTRL_MODE;

/* live/playback */
/*  - eCMD_C011_DEC_CHAN_OPEN */
typedef enum
{
    eC011_CHANNEL_PLAYBACK                   = 0x00000000,
    eC011_CHANNEL_LIVE_DECODE                = 0x00000001,
    eC011_CHANNEL_TRANSPORT_STREAM_CAPTURE   = 0x00000002,
} eC011_CHANNEL_TYPE;

/* video algorithm */
/*  - eCMD_C011_DEC_CHAN_OPEN */
typedef enum
{
    eC011_VIDEO_ALG_H264        = 0x00000000, // H.264
    eC011_VIDEO_ALG_MPEG2       = 0x00000001, // MPEG-2
    eC011_VIDEO_ALG_H261        = 0x00000002, // H.261
    eC011_VIDEO_ALG_H263        = 0x00000003, // H.263
    eC011_VIDEO_ALG_VC1         = 0x00000004, // VC1
    eC011_VIDEO_ALG_MPEG1       = 0x00000005, // MPEG-1
    eC011_VIDEO_ALG_DIVX        = 0x00000006, // divx
#if 0
    eC011_VIDEO_ALG_MPEG4       = 0x00000006, // MPEG-4
#endif

} eC011_VIDEO_ALG;

/* input source */
/*  - eCMD_C011_DEC_CHAN_OPEN */
typedef enum
{
    eC011_VIDSRC_DEFAULT_PROGRESSIVE= 0x00000000, // derive from stream
    eC011_VIDSRC_DEFAULT_INTERLACED = 0x00000001, // derive from stream
    eC011_VIDSRC_FIXED_PROGRESSIVE  = 0x00000002, // progressive frames
    eC011_VIDSRC_FIXED_INTERLACED   = 0x00000003, // interlaced fields

} eC011_VIDEO_SOURCE_MODE;

/* pull-down mode */
/*  - eCMD_C011_DEC_CHAN_OPEN */
typedef enum
{
    eC011_PULLDOWN_DEFAULT_32   = 0x00000000, // derive from PTS inside stream
    eC011_PULLDOWN_DEFAULT_22   = 0x00000001, // derive from PTS inside stream
    eC011_PULLDOWN_DEFAULT_ASAP = 0x00000002, // derive from PTS inside stream
    eC011_PULLDOWN_FIXED_32     = 0x00000003, // fixed 3-2 pulldown
    eC011_PULLDOWN_FIXED_22     = 0x00000004, // fixed 2-2 pulldown
    eC011_PULLDOWN_FIXED_ASAP   = 0x00000005, // fixed as fast as possible

} eC011_PULLDOWN_MODE;

/* display order */
/*  - eCMD_C011_DEC_CHAN_OPEN */
typedef enum
{
    eC011_DISPLAY_ORDER_DISPLAY = 0x00000000, // display in display order
    eC011_DISPLAY_ORDER_DECODE  = 0x00000001, // display in decode order

} eC011_DISPLAY_ORDER;

/* picture information mode */
/*  - eCMD_C011_DEC_CHAN_OPEN */
typedef enum
{
    eC011_PICTURE_INFO_OFF      = 0x00000000, // no picture information
    eC011_PICTURE_INFO_ON       = 0x00000001, // pass picture information to host

} eC011_PICTURE_INFO_MODE;

/* picture ready interrupt mode */
/*  - eCMD_C011_DEC_CHAN_OPEN */
typedef enum
{
    eC011_PIC_INT_NONE          = 0x00000000, // no picture ready interrupts
    eC011_PIC_INT_FIRST_PICTURE = 0x00000001, // interrupt on first picture only
    eC011_PIC_INT_ALL_PICTURES  = 0x00000002, // interrupt on all pictures

} eC011_PIC_INT_MODE;

/* picture setup interrupt mode */
/*  - eCMD_C011_DEC_CHAN_OPEN */
typedef enum
{
    eC011_DISP_INT_NONE          = 0x00000000, // no picture setup/release interrupts
    eC011_DISP_INT_SETUP         = 0x00000001, // interrupt on picture setup only
    eC011_DISP_INT_SETUP_RELEASE = 0x00000002, // interrupt on picture setup and release

} eC011_DISP_INT_MODE;

/* deblocking mode */
/*  - eCMD_C011_DEC_CHAN_OPEN */
typedef enum
{
    eC011_DEBLOCKING_OFF         = 0x00000000, // no deblocking
    eC011_DEBLOCKING_ON          = 0x00000001, // deblocking on

} eC011_DEBLOCKING_MODE;

/* BRCM (HD-DVI) mode */
/*  - eCMD_C011_DEC_CHAN_OPEN */
typedef enum
{
    eC011_BRCM_MODE_OFF         = 0x00000000, // Non BRCM (non HD-DVI) mode
    eC011_BRCM_MODE_ON          = 0x00000001, // BRCM (HD-DVI) mode
    eC011_BRCM_ECG_MODE_ON      = 0x00000002, // BRCM (HD-DVI) ECG mode

} eC011_BRCM_MODE;

/* External VCXO control mode */
/*  - eCMD_C011_DEC_CHAN_OPEN */
typedef enum
{
    eC011_EXTERNAL_VCXO_OFF     = 0x00000000, // No external vcxo control
    eC011_EXTERNAL_VCXO_ON      = 0x00000001, // External vcxo control

} eC011_EXTERNAL_VCXO_MODE;

/* Display timing mode */
/*  - eCMD_C011_DEC_CHAN_OPEN */
typedef enum
{
    eC011_DISPLAY_TIMING_USE_PTS    = 0x00000000, // Use PTS for display timing
    eC011_DISPLAY_TIMING_IGNORE_PTS = 0x00000001, // Ignore PTS and follow pulldown

} eC011_DISPLAY_TIMING_MODE;

/* User data collection mode */
/*  - eCMD_C011_DEC_CHAN_OPEN */
typedef enum
{
    eC011_USER_DATA_MODE_OFF    = 0x00000000, // User data disabled
    eC011_USER_DATA_MODE_ON     = 0x00000001, // User data enabled

} eC011_USER_DATA_MODE;

/*  - eCMD_C011_DEC_CHAN_OPEN */
typedef enum
{
    eC011_PAN_SCAN_MODE_OFF    = 0x00000000, // pan-scan disabled
    eC011_PAN_SCAN_MODE_ON     = 0x00000001, // pan-scan enabled
    eC011_PAN_SCAN_MODE_HOR_ON   = 0x00000002, //Horizontal pan-scan enabled, Vertical pan-scan disabled
    eC011_PAN_SCAN_MODE_HOR_OFF   = 0x00000003, //Horizontal pan-scan disabled, Vertical pan-scan disabled
    eC011_PAN_SCAN_MODE_VER_ON      = 0x00000004, //Horizontal pan-scan disabled, Vertical pan-scan enabled
    eC011_PAN_SCAN_MODE_VER_OFF   = 0x00000005,  //Horizontal pan-scan disabled, Vertical pan-scan disabled 
 
} eC011_PAN_SCAN_MODE;

/*
 * PTS States
 *
 * PTS_VALID: PTS is coded in the picture
 * PTS_INTERPOLATED: PTS has been interpolated from an earlier picture which had a coded PTS
 * PTS_UNKNOWN: Startup condition when PTS is not yet received
 * PTS_HOST: Host has set the PTS to be used for the next pic via the SetPTS API command
 */
typedef enum PTS_STATE {
   PTS_VALID            = 0,
   PTS_INTERPOLATED,
   PTS_UNKNOWN,
   PTS_HOST,

} ePtsState;

/* channel status structure */
/*  - eCMD_C011_DEC_CHAN_OPEN response */
typedef struct {
    eC011_DISPLAY_TIMING_MODE displayTimingMode;   // current display timing mode in effect
    long                      videoDisplayOffset;  // current video display offset in effect
    unsigned long             currentPts;          // current PTS value
    unsigned long             interpolatedPts;     // currentPts of type PTS_STATE
    unsigned long             refCounter;
    unsigned long             pcrOffset;
    unsigned long             stcValue;
    unsigned long             stcWritten;          // 1 -> host updated STC, 0 -> stream ARC ack
    long                      ptsStcOffset;        // PTS - STC
    void                      *pVdecStatusBlk;      /* pointer to vdec status block */
	unsigned long             lastPicture;         // 1 -> decoder last picture indication
    unsigned long             pictureTag;          /* Picture Tag from VDEC */
    unsigned long             tsmLockTime;		   /* Time when the First Picture passed TSM */
    unsigned long             firstPicRcvdTime;	   /* Time when the First Picture was recieved */
    unsigned long             picture_done_payload;/* Payload associated with the picture done marker interrupt */


} sC011_CHAN_STATUS;

/* picture information block (PIB) */
/* used in picInfomode, userdataMode */
typedef struct
{
   unsigned long             bFormatChange;
   unsigned long             resolution;
   unsigned long             channelId;
   unsigned long             ppbPtr;
   long                      ptsStcOffset;
   unsigned long             zeroPanscanValid;
   unsigned long             dramOutBufAddr;
   unsigned long             yComponent;
   PPB                       ppb;

} C011_PIB;

/* size of picture information block */
#define C011_PIB_SIZE               (sizeof(C011_PIB))

/* picture release mode */
/*  - eCMD_C011_DEC_CHAN_CLOSE */
typedef enum
{
    eC011_PIC_REL_HOST          = 0x00000000, // wait for host to release pics
    eC011_PIC_REL_INTERNAL      = 0x00000001, // do not wait for host

} eC011_PIC_REL_MODE;

/* last picture display mode */
/*  - eCMD_C011_DEC_CHAN_CLOSE */
typedef enum
{
    eC011_LASTPIC_DISPLAY_ON    = 0x00000000, // keep displaying last picture after channelClose
    eC011_LASTPIC_DISPLAY_OFF   = 0x00000001, // blank output after channelClose

} eC011_LASTPIC_DISPLAY;

/* channel flush mode */
/*  - eCMD_C011_DEC_CHAN_FLUSH */
typedef enum
{
    eC011_FLUSH_INPUT_POINT     = 0x00000000, // flush at current input point
    eC011_FLUSH_PROC_POINT      = 0x00000001, // flush at current processing
    eC011_FLUSH_PROC_POINT_RESET_TS  = 0x00000002, // flush at current processing, reset TS

} eC011_FLUSH_MODE;

/* direction */
/*  - eCMD_C011_DEC_CHAN_TRICK_PLAY */
typedef enum
{
    eCODEC_DIR_FORWARD          = 0x00000000, // forward
    eCODEC_DIR_REVERSE          = 0x00000001, // reverse

} eC011_DIR;

/* speed */
/*  - eCMD_C011_DEC_CHAN_TRICK_PLAY */
typedef enum
{
    eC011_SPEED_NORMAL          = 0x00000000, // all pictures
    eC011_SPEED_FAST            = 0x00000001, // reference pictures only
    eC011_SPEED_VERYFAST        = 0x00000002, // I-picture only
    eC011_SPEED_SLOW            = 0x00000003, // STC trickplay slow
    eC011_SPEED_PAUSE           = 0x00000004, // STC trickplay pause
    eC011_SPEED_I_ONLY_HOST_MODE = 0x00000100, // I-picture only host mode
    eC011_SPEED_2x_SLOW         = 0xFFFFFFFF, // all pics played 2x frame time
    eC011_SPEED_4x_SLOW         = 0xFFFFFFFE, // all pics played 4x frame time
    eC011_SPEED_8x_SLOW         = 0xFFFFFFFD, // all pics played 8x frame time
    eC011_SPEED_STEP            = 0xFFFFFFFC, // STC trickplay step

} eC011_SPEED;

typedef enum
{
    eC011_DROP_TYPE_DECODER   = 0x00000000,
    eC011_DROP_TYPE_DISPLAY     = 0x00000001,

} eC011_DROP_TYPE;

/* stream input sync mode */
/*  - eCMD_C011_DEC_CHAN_INPUT_PARAMS */
typedef enum
{
    eC011_SYNC_MODE_AUTOMATIC   = 0x00000000, // automatic sync detection
    eC011_SYNC_MODE_SYNCPIN     = 0x00000001, // sync pin mode

} eC011_SYNC_MODE;

/* unmarked discontinuity notification */
/*  - eCMD_C011_DEC_CHAN_INPUT_PARAMS */
typedef enum
{
    eC011_UNMARKED_DISCONTINUITY_OFF   = 0x00000000, // disable unmarked discontinuity
    eC011_UNMARKED_DISCONTINUITY_ON    = 0x00000001, // enable unmarked discontinuity

} eC011_UNMARKED_DISCONTINUITY_MODE;

/* unmarked discontinuity notification trigger threshold */
/*  - eCMD_C011_DEC_CHAN_INPUT_PARAMS */
typedef enum
{
    eC011_UNMARKED_DISCONTINUITY_THRESHOLD_1_PKT   = 0x00000000, // trigger on one packet only
    eC011_UNMARKED_DISCONTINUITY_THRESHOLD_2_PKTS  = 0x00000001, // trigger on 2 consecutive packets only

} eC011_UNMARKED_DISCONTINUITY_THRESHOLD;

/* display resolution */
/*  - eCMD_C011_DEC_CHAN_VIDEO_OUTPUT */
typedef enum
{
    eC011_RESOLUTION_CUSTOM         = 0x00000000, // custom
    eC011_RESOLUTION_480i           = 0x00000001, // 480i
    eC011_RESOLUTION_1080i          = 0x00000002, // 1080i (1920x1080, 60i)
    eC011_RESOLUTION_NTSC           = 0x00000003, // NTSC (720x483, 60i)
    eC011_RESOLUTION_480p           = 0x00000004, // 480p (720x480, 60p)
    eC011_RESOLUTION_720p           = 0x00000005, // 720p (1280x720, 60p)
    eC011_RESOLUTION_PAL1           = 0x00000006, // PAL_1 (720x576, 50i)
    eC011_RESOLUTION_1080i25        = 0x00000007, // 1080i25 (1920x1080, 50i)
    eC011_RESOLUTION_720p50         = 0x00000008, // 720p50 (1280x720, 50p)
    eC011_RESOLUTION_576p           = 0x00000009, // 576p (720x576, 50p)
    eC011_RESOLUTION_1080i29_97     = 0x0000000A, // 1080i (1920x1080, 59.94i)
    eC011_RESOLUTION_720p59_94      = 0x0000000B, // 720p (1280x720, 59.94p)
    eC011_RESOLUTION_SD_DVD         = 0x0000000C, // SD DVD (720x483, 60i)
    eC011_RESOLUTION_480p656        = 0x0000000D, // 480p (720x480, 60p), output bus width 8 bit, clock 74.25MHz.
    eC011_RESOLUTION_1080p23_976    = 0x0000000E, // 1080p23_976 (1920x1080, 23.976p)
    eC011_RESOLUTION_720p23_976     = 0x0000000F, // 720p23_976 (1280x720p, 23.976p)
    eC011_RESOLUTION_240p29_97      = 0x00000010, // 240p (1440x240, 29.97p )
    eC011_RESOLUTION_240p30         = 0x00000011, // 240p (1440x240, 30p)
    eC011_RESOLUTION_288p25         = 0x00000012, // 288p (1440x288p, 25p)
    eC011_RESOLUTION_1080p29_97     = 0x00000013, // 1080p29_97 (1920x1080, 29.97p)
    eC011_RESOLUTION_1080p30        = 0x00000014, // 1080p30 (1920x1080, 30p)
    eC011_RESOLUTION_1080p24        = 0x00000015, // 1080p24 (1920x1080, 24p)
    eC011_RESOLUTION_1080p25        = 0x00000016, // 1080p25 (1920x1080, 25p)
    eC011_RESOLUTION_720p24         = 0x00000017, // 720p24 (1280x720, 25p)
    eC011_RESOLUTION_720p29_97	    = 0x00000018, // 720p29_97 (1280x720, 29.97p)

} eC011_RESOLUTION;

/* output scanning mode */
/*  - eCMD_C011_DEC_CHAN_VIDEO_OUTPUT */
typedef enum
{
    eC011_SCAN_MODE_PROGRESSIVE = 0x00000000, // progressive frames
    eC011_SCAN_MODE_INTERLACED  = 0x00000001, // interlaced fields
} eC011_SCAN_MODE;

/* display option */
/*  - eCMD_C011_DEC_CHAN_VIDEO_OUTPUT */
typedef enum
{
    eC011_DISPLAY_LETTERBOX     = 0x00000000, // letter box
    eC011_DISPLAY_FULLSCREEN    = 0x00000001, // full screen
    eC011_DISPLAY_PILLARBOX     = 0x00000002, // pillar box
} eC011_DISPLAY_OPTION;

/* display formatting */
/*  - eCMD_C011_DEC_CHAN_VIDEO_OUTPUT */
typedef enum
{
    eC011_FORMATTING_AUTO       = 0x00000000, // automatic
    eC011_FORMATTING_CUSTOM     = 0x00000001, // custom
    eC011_FORMATTING_NONE       = 0x00000002, // no formatting
    eC011_FORMATTING_PICTURE    = 0x00000003, // picture level
} eC011_FORMATTING;

/* vsync mode */
/*  - eCMD_C011_DEC_CHAN_VIDEO_OUTPUT */
typedef enum
{
    eC011_VSYNC_MODE_NORMAL     = 0x00000000, // internal video timing
    eC011_VSYNC_MODE_EXTERNAL   = 0x00000001, // use external vsync_in signal
    eC011_VSYNC_MODE_BYPASS     = 0x00000002, // 7411 updates STC from PCR in stream, but external vsync
    eC011_VSYNC_MODE_INTERNAL   = 0x00000003, // User updates STC, but internal vsync

} eC011_VSYNC_MODE;

/* output clipping mode */
/*  - eCMD_C011_DEC_CHAN_VIDEO_OUTPUT */
typedef enum
{
    eC011_OUTPUT_CLIPPING_BT601  = 0x00000000, // Luma pixel is clipped to [16,235]. Chroma pixel is clipped to [16,240]
    eC011_OUTPUT_CLIPPING_BT1120 = 0x00000001, // The pixel is clipped to [1,254]
    eC011_OUTPUT_CLIPPING_NONE   = 0x00000002, // No output clipping

} eC011_OUTPUT_CLIPPING;

/* display mode for pause/slow/fastforward*/
/*  - eCMD_C011_DEC_CHAN_VIDEO_OUTPUT */
typedef enum
{
    eC011_DISPLAY_MODE_AUTO     = 0x00000000,
    eC011_DISPLAY_MODE_FRAME    = 0x00000001,
    eC011_DISPLAY_MODE_TOP      = 0x00000002,
    eC011_DISPLAY_MODE_BOTTOM   = 0x00000003,

} eC011_DISPLAY_MODE;

/*order in timeline where the pauseUntoPts and displayUntoPts occur*/
typedef enum
{
    eC011_PAUSE_UNTO_PTS_ONLY				= 0x00000001,
    eC011_DISPLAY_UNTO_PTS_ONLY				= 0x00000002,    
    eC011_DISPLAY_UNTO_PTS_LESSER_THAN_PAUSE_UNTO_PTS   = 0x00000003,
    eC011_DISPLAY_UNTO_PTS_GREATER_THAN_PAUSE_UNTO_PTS  = 0x00000004,
}
eC011_DISPLAY_PAUSE_STATE;

/* scaling on/off */
/*  - eCMD_C011_DEC_CHAN_OUTPUT_FORMAT */
typedef enum
{
    eC011_SCALING_OFF           = 0x00000000,
    eC011_SCALING_ON            = 0x00000001,

} eC011_SCALING;

/* edge control */
/*  - eCMD_C011_DEC_CHAN_OUTPUT_FORMAT */
typedef enum
{
    eC011_EDGE_CONTROL_NONE     = 0x00000000, // no cropping or padding
    eC011_EDGE_CONTROL_CROP     = 0x00000001, // cropping
    eC011_EDGE_CONTROL_PAD      = 0x00000002, // padding

} eC011_EDGE_CONTROL;

/* deinterlacing on/off */
/*  - eCMD_C011_DEC_CHAN_OUTPUT_FORMAT */
typedef enum
{
    eC011_DEINTERLACING_OFF     = 0x00000000,
    eC011_DEINTERLACING_ON      = 0x00000001,

} eC011_DEINTERLACING;

/* scaling target */
/*  - eCMD_C011_DEC_CHAN_SCALING_FILTERS */
typedef enum
{
    eC011_HORIZONTAL            = 0x00000000,
    eC011_VERTICAL_FRAME        = 0x00000001,
    eC011_VERTICAL_FIELD_TOP    = 0x00000002,
    eC011_VERTICAL_FIELD_BOTTOM = 0x00000003,

} eC011_SCALING_TARGET;

/* normalization */
/*  - eCMD_C011_DEC_CHAN_SCALING_FILTERS */
typedef enum
{
    eC011_NORMALIZATION_128     = 0x00000000, // divide by 128
    eC011_NORMALIZATION_64      = 0x00000001, // divide by 64

} eC011_NORMALIZATION;

/* pause type */
/*  - eCMD_C011_DEC_CHAN_PAUSE_OUTPUT */
typedef enum
{
    eC011_PAUSE_TYPE_RESUME     = 0x00000000, // resume video output
    eC011_PAUSE_TYPE_CURRENT    = 0x00000001, // pause video on current frame
    eC011_PAUSE_TYPE_BLACK      = 0x00000002, // pause video with black screen
    eC011_PAUSE_TYPE_STEP       = 0x00000003, // display next picture

} eC011_PAUSE_TYPE;

/* TSD audio payload type */
/*  - eCMD_C011_DEC_CREATE_AUDIO_CONTEXT */
typedef enum
{
    eC011_TSD_AUDIO_PAYLOAD_MPEG1   = 0x00000000,
    eC011_TSD_AUDIO_PAYLOAD_AC3     = 0x00000001,

} eC011_TSD_AUDIO_PAYLOAD_TYPE;

/* CDB extract bytes for PES */
/*  - eCMD_C011_DEC_CREATE_AUDIO_CONTEXT */
typedef enum
{
    eC011_PES_CDB_EXTRACT_0_BYTES   = 0x00000000,
    eC011_PES_CDB_EXTRACT_1_BYTE    = 0x00000001,
    eC011_PES_CDB_EXTRACT_4_BYTES   = 0x00000002,
    eC011_PES_CDB_EXTRACT_7_BYTES   = 0x00000003,

} eC011_PES_CDB_EXTRACT_BYTES;

/* audio payload info */
/*  - eCMD_C011_DEC_CREATE_AUDIO_CONTEXT */
typedef union DecAudioPayloadInfo {
    eC011_TSD_AUDIO_PAYLOAD_TYPE payloadType;
    eC011_PES_CDB_EXTRACT_BYTES  extractBytes;

} uC011_AUDIO_PAYLOAD_INFO;

/* descrambling mode */
/*  - eCMD_C011_DEC_CHAN_SET_DECYPTION */
typedef enum
{
    eC011_DESCRAMBLING_3DES         = 0x00000000,
    eC011_DESCRAMBLING_DES          = 0x00000001,

} eC011_DESCRAMBLING_MODE;

/* key exchange */
/*  - eCMD_C011_DEC_CHAN_SET_DECYPTION */
typedef enum
{
    eC011_KEY_EXCHANGE_EVEN_0       = 0x00000001,
    eC011_KEY_EXCHANGE_EVEN_1       = 0x00000002,
    eC011_KEY_EXCHANGE_EVEN_2       = 0x00000004,
    eC011_KEY_EXCHANGE_ODD_0        = 0x00000010,
    eC011_KEY_EXCHANGE_ODD_1        = 0x00000020,
    eC011_KEY_EXCHANGE_ODD_2        = 0x00000040,

} eC011_KEY_EXCHANGE_MODE;

/* cipher text stealing */
/*  - eCMD_C011_DEC_CHAN_SET_DECYPTION */
typedef enum
{
    eC011_CT_STEALING_MODE_OFF      = 0x00000000,
    eC011_CT_STEALING_MODE_ON       = 0x00000001,

} eC011_CT_STEALING_MODE;

/* pause mode */
/*  - eCMD_C011_DEC_CHAN_PAUSE */
typedef enum
{
    eC011_PAUSE_MODE_OFF            = 0x00000000,
    eC011_PAUSE_MODE_ON             = 0x00000001,

} eC011_PAUSE_MODE;

/* skip pic mode */
/*  - eCMD_C011_DEC_CHAN_SET_SKIP_PIC_MODE */
typedef enum
{
    eC011_SKIP_PIC_IPB_DECODE       = 0x00000000,
    eC011_SKIP_PIC_IP_DECODE        = 0x00000001,
    eC011_SKIP_PIC_I_DECODE         = 0x00000002,

} eC011_SKIP_PIC_MODE;

/* enum for color space conversion */
typedef enum
{
   eC011_DEC_CSC_CTLBYDEC    = 0x00000000,
   eC011_DEC_CSC_ENABLE      = 0x00000001,
   eC011_DEC_CSC_DISABLE     = 0x00000002,
} eC011_DEC_CSC_SETUP;

/* enum for setting range remap */
typedef enum
{
   eC011_DEC_RANGE_REMAP_VIDCTL  = 0x00000000,
   eC011_DEC_RANGE_REMAP_ENABLE  = 0x00000001,
   eC011_DEC_RANGE_REMAP_DISABLE = 0x00000002,
} eC011_DEC_RANGE_REMAP_SETUP;

typedef enum
{
   eC011_DEC_OPERATION_MODE_GENERIC     = 0x00000000,
   eC011_DEC_OPERATION_MODE_BLURAY      = 0x00000001,
   eC011_DEC_OPERATION_MODE_HDDVD      = 0x00000002,
} eC011_DEC_OPERATION_MODE;

typedef enum
{
   eC011_DEC_RANGE_REMAP_ADVANCED  = 0x00000000,
   eC011_DEC_RANGE_REMAP_MAIN      = 0x00000001,
} eC011_DEC_RANGE_REMAP_VC1PROFILE;

/* encoder sequence paramaters */
/*  - eCMD_C011_ENC_CHAN_OPEN */
/*  - eCMD_C011_ENC_CHAN_CONTROL */
typedef struct
{
  unsigned int                  seqParamId;     // sequence parameter set ID
  unsigned int                  profile;        // profile (E_ENC_PROFILE)
  unsigned int                  level;          // level x 10
  unsigned int                  constraintMap;  // bitmap for constraint sets
  unsigned int                  frameNumMaxLog; // logorithm of frame number max
  unsigned int                  pocLsbMaxLog;   // logorithm of POC LSB max
  unsigned int                  pocType;        // POC type
  unsigned int                  refFrameMax;    // number of reference frames
  unsigned int                  frameMBHeight;  // frame height in MB
  unsigned int                  frameMBWidth;   // frame width in MB
  unsigned int                  frameMBOnly;    // frame MB only flag
  unsigned int                  adaptMB;        // MBAFF flag
  unsigned int                  dir8x8Infer;    // direct 8x8 inference flag

} sC011_ENC_SEQ_PARAM;

/* encoder picture parameters */
/*  - eCMD_C011_ENC_CHAN_OPEN */
/*  - eCMD_C011_ENC_CHAN_CONTROL */
typedef struct
{
  unsigned int                  picParamId;     // picture parameter set ID
  unsigned int                  seqParameterId; // sequence parameter set ID
  unsigned int                  refIdxMaxL0;    // number of active reference indices
  unsigned int                  refIdxMaxL1;    // number of active reference indices
  unsigned int                  entMode;        // entropy mode (E_ENC_ENT_MODE)
  unsigned int                  initQP;         // picture init QP
  unsigned int                  intraConst;     // constrained intra prediction flag

} sC011_ENC_PIC_PARAM;

/* encoder coding parameters */
/*  - eCMD_C011_ENC_CHAN_OPEN */
/*  - eCMD_C011_ENC_CHAN_CONTROL */
typedef struct
{
  unsigned int                  eventNfy;       // event notify generation flag
  unsigned int                  intMode;        // interlace coding mode (E_ENC_INT_MODE)
  unsigned int                  gopSize;        // number of frames per GOP (I frame period)
  unsigned int                  gopStruct;      // number of B-frames between references
  unsigned int                  rateCtrlMode;   // rate control mode (E_ENC_RATE_CTRL_MODE)
  unsigned int                  frameRate;      // frame rate
  unsigned int                  constQP;        // constant QP
  unsigned int                  bitratePeriod;  // VBR bitrate average period (in GOPs)
  unsigned int                  bitRateAvg;     // VBR average bitrate
  unsigned int                  bitRateMax;     // VBR max bitrate
  unsigned int                  bitRateMin;     // VBR min bitrate

} sC011_ENC_CODING_PARAM;

/* encoder video-in parameters */
/*  - eCMD_C011_ENC_CHAN_OPEN */
/*  - eCMD_C011_ENC_CHAN_CONTROL */
typedef struct
{
  unsigned int                  picInfoMode;    // picture info mode (E_ENC_PIC_INFO_MODE)
  unsigned int                  picIdSrc;       // picture ID source (E_ENC_PIC_ID_SRC)
  unsigned int                  picYUVFormat;   // picture YUV format (E_ENC_PIC_YUV_FORMAT)
  unsigned int                  picIntFormat;   // picture interlace format (E_ENC_PIC_INT_FORMAT)

} sC011_ENC_VID_IN_PARAM;

/* encoder code-out parameters */
/*  - eCMD_C011_ENC_CHAN_OPEN */
/*  - eCMD_C011_ENC_CHAN_CONTROL */
typedef struct
{
  unsigned int                  portId;         // port ID
  unsigned int                  codeFormat;     // code format (E_ENC_CODE_FORMAT)
  unsigned int                  delimiter;      // delimiter NAL flag
  unsigned int                  endOfSeq;       // end of sequence NAL flag
  unsigned int                  endOfStream;    // end of stream NAL flag
  unsigned int                  picParamPerPic; // picture parameter set per picture flag
  unsigned int                  seiMasks;       // SEI message (1 << E_ENC_SEI_TYPE)

} sC011_ENC_CODE_OUT_PARAM;

/* encoder picture data */
/*  - eCMD_C011_ENC_CHAN_CONTROL */
typedef struct
{
  unsigned int                  picId;          // picture ID
  unsigned int                  picStruct;      // picture structure (E_ENC_PIC_STRUCT)
  unsigned int                  origBuffIdx;    // original frame buffer index
  unsigned int                  reconBuffIdx;   // reconstructed frame buffer index

} sC011_ENC_PIC_DATA;

/* encoder channel control parameter type */
/*  - eCMD_C011_ENC_CHAN_CONTROL */
typedef enum {
    eC011_ENC_CTRL_SEQ_PARAM    = 0x00000000,
    eC011_ENC_CTRL_PIC_PARAM    = 0x00000001,
    eC011_ENC_CTRL_CODING_PARAM = 0x00000002,
    eC011_ENC_CTRL_PIC_DATA     = 0x00000003,

} eC011_ENC_CTRL_CODE;

/* encoder channel control parameter */
/*  - eCMD_C011_ENC_CHAN_CONTROL */
typedef union EncCtrlParam {
    sC011_ENC_SEQ_PARAM     seqParams;
    sC011_ENC_PIC_PARAM     picParams;
    sC011_ENC_CODING_PARAM  codingParams;
    sC011_ENC_PIC_DATA      picData;

} uC011_ENC_CTRL_PARAM;

/*
 * Data Structures for the API commands above
 */

#define DMA_CIQ_DEPTH 64

/* dsDmaCtrlInfo */
typedef struct {
    unsigned long             dmaSrcAddr;    /* word 0: src addr */
    unsigned long             dmaDstAddr;    /* word 1: dst addr */
    unsigned long             dmaXferCtrol;  /* word 2:
                                              *       bit 31-30. reserved
                                              *       bit 29. Stream number
                                              *       bit 28-27. Interrupt
                                              *           x0: no interrupt;
                                              *           01: interrupt when dma done wo err 
                                              *           11: interrupt only if there is err
                                              *       bit 26. Endidan. 0: big endian; 1: little endian 
                                              *       bit 25. dst inc. 0: addr doesn't change; 1: addr+=4
                                              *       bit 24. src inc. 0: addr doesn't change; 1: addr+=4
                                              *       bit 23-0. number of bytes to be transfered 
                                              */
} dsDmaCtrlInfo;

/* dsDmaCtrlInfoQueue */
typedef struct {
    unsigned long             readIndex;
    unsigned long             writeIndex;
    dsDmaCtrlInfo             dmaCtrlInfo[DMA_CIQ_DEPTH];
} dsDmaCtrlInfoQueue; 

/* Init */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             memSizeMBytes;
    unsigned long             inputClkFreq;
    unsigned long             uartBaudRate;
    unsigned long             initArcs;
    eC011_INT_CONTROL         interrupt;
    unsigned long             audioMemSize;
    eC011_BRCM_MODE           brcmMode;
	unsigned long             fgtEnable;    /* 0 - disable FGT, 1 - enable FGT */
    unsigned long             DramLogEnable; /* 0 - disable DramLog, 1 - enable DramLog */
    unsigned long             sidMemorySize; /* in bytes */
    unsigned long             dmaDataXferEnable; /* 0:disable; 1:enable */
	unsigned long             rsaDecrypt; /* 0:disable; 1:enable */
	unsigned long             openMode;  
	unsigned long             rsvd1; 
	unsigned long             rsvd2; 
	unsigned long             rsvd3; 
		

} C011CmdInit;

typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             status;
    unsigned long             commandBuffer;
    unsigned long             responseBuffer;
    unsigned long             blockPool;
    unsigned long             blockSize; /* in words */
    unsigned long             blockCount;
    unsigned long             audioMemBase;
    unsigned long             watchMemAddr;
    unsigned long             streamDramLogBase;
    unsigned long             streamDramLogSize;
    unsigned long             vdecOuterDramLogBase;
    unsigned long             vdecOuterDramLogSize;
    unsigned long             vdecInnerDramLogBase;
    unsigned long             vdecInnerDramLogSize;
    unsigned long             sidMemoryBaseAddr;
    unsigned long             inputDmaCiqAddr;
    unsigned long             inputDmaCiqReleaseAddr;
    unsigned long             outputDmaCiqAddr;
    unsigned long             outputDmaCiqReleaseAddr;
	unsigned long			  dramX509CertAddr;
	unsigned long			  rsvdAddr; 
} C011RspInit;

/* Reset */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
} C011CmdReset;

/* SelfTest */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    eC011_TEST_ID             testId;
} C011CmdSelfTest;

typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             status;
    unsigned long             errorCode;
} C011RspSelfTest;

/* GetVersion */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
} C011CmdGetVersion;

typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             status;
    unsigned long             streamSwVersion;
    unsigned long             decoderSwVersion;
    unsigned long             chipHwVersion;
    unsigned long             reserved1;
    unsigned long             reserved2;
    unsigned long             reserved3;
    unsigned long             reserved4;
    unsigned long             blockSizePIB;
    unsigned long             blockSizeChannelStatus;
    unsigned long             blockSizePPB;
    unsigned long             blockSizePPBprotocolMpeg;
    unsigned long             blockSizePPBprotocolH264;
    unsigned long             blockSizePPBprotocolRsvd;
} C011RspGetVersion;

/* GPIO */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    eC011_GPIO_CONTROL        gpioControl;
} C011CmdGPIO;

/* DebugSetup */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             paramMask;
    unsigned long             debugARCs;
    unsigned long             debugARCmode;
    unsigned long             outPort;
    unsigned long             clock;
    unsigned long             channelId;
    unsigned long             enableRVCcapture;
    unsigned long             playbackMode;
    unsigned long             enableCRCinterrupt;
    unsigned long             esStartDelay10us;
} C011CmdDebugSetup;

typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             status;
    unsigned long             DQ;
    unsigned long             DRQ;
} C011RspDebugSetup;

/* DecChannelOpen */
typedef struct {
   unsigned long             command;
   unsigned long             sequence;
   eC011_INPUT_PORT          inPort;
   eC011_OUTPUT_PORT         outVidPort;
   eC011_STREAM_TYPE         streamType;
   eC011_MAX_PICSIZE         maxPicSize;
   eC011_OUTCTRL_MODE        outCtrlMode;
   eC011_CHANNEL_TYPE        chanType;
    unsigned long             reservedWord8;
   eC011_VIDEO_ALG           videoAlg;
   eC011_VIDEO_SOURCE_MODE   sourceMode;
   eC011_PULLDOWN_MODE       pulldown;
   eC011_PICTURE_INFO_MODE   picInfo;
   eC011_DISPLAY_ORDER       displayOrder;
    unsigned long             reservedWord14;
    unsigned long             reservedWord15;
   unsigned long             streamId; /* for multi-stream */
   eC011_DEBLOCKING_MODE     deblocking;
   eC011_EXTERNAL_VCXO_MODE  vcxoControl;
   eC011_DISPLAY_TIMING_MODE displayTiming;
   long                      videoDisplayOffset;
   eC011_USER_DATA_MODE      userDataMode;
   unsigned long             enableUserDataInterrupt;
   unsigned long             ptsStcDiffThreshold;
   unsigned long             stcPtsDiffThreshold;
   unsigned long             enableFirstPtsInterrupt;
   unsigned long             enableStcPtsThresholdInterrupt;
   unsigned long             frameRateDefinition;
   unsigned long             hostDmaInterruptEnable;
   unsigned long             asynchEventNotifyEnable;
   unsigned long             enablePtsStcChangeInterrupt;
   unsigned long             enablePtsErrorInterrupt;
   unsigned long             enableFgt;
   unsigned long             enable23_297FrameRateOutput;
   unsigned long             enableVideoDataUnderflowInterrupt;
    unsigned long             reservedWord35;
    unsigned long             pictureInfoInterruptEnable;
} DecCmdChannelOpen;

typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             status;
    unsigned long             channelId;
    unsigned long             picBuf;
    unsigned long             picRelBuf;
    unsigned long             picInfoDeliveryQ;
    unsigned long             picInfoReleaseQ;
    unsigned long             channelStatus;
    unsigned long             userDataDeliveryQ;
    unsigned long             userDataReleaseQ;
    unsigned long             transportStreamCaptureAddr;
    unsigned long             asyncEventQ;
} DecRspChannelOpen;

/* DecChannelClose */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             channelId;
    eC011_PIC_REL_MODE        pictureRelease;
    eC011_LASTPIC_DISPLAY     lastPicDisplay;
} DecCmdChannelClose;

/* DecChannelActivate */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             channelId;
    unsigned long             dbgMode;
} DecCmdChannelActivate;

/* DecChannelStatus */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             channelId;
} DecCmdChannelStatus;

typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             status;
    unsigned long             channelStatus;
    unsigned long             cpbSize;       /* CPB size */
    unsigned long             cpbFullness;   /* CPB fullness */
    unsigned long             binSize;       /* BIN buffer size */
    unsigned long             binFullness;   /* BIN buffer fullness */
    unsigned long             bytesDecoded;  /* Bytes decoded */
    unsigned long             nDelayed;      /* pics with delayed delivery */
} DecRspChannelStatus;

/* DecChannelFlush */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             channelId;
    eC011_FLUSH_MODE          flushMode;
} DecCmdChannelFlush;

/* DecChannelTrickPlay */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             channelId;
    eC011_DIR                 direction;
    eC011_SPEED               speed;
} DecCmdChannelTrickPlay;

/* DecChannelSetTSPIDs */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             channelId;
    unsigned long             pcrPid;
    unsigned long             videoPid;
    unsigned long             videoSubStreamId;
} DecCmdChannelSetTSPIDs;

/* DecChannelSetPcrPID */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             channelId;
    unsigned long             pcrPid;
} DecCmdChannelSetPcrPID;

/* DecChannelSetVideoPID */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             channelId;
    unsigned long             videoPid;
    unsigned long             videoSubStreamId;
} DecCmdChannelSetVideoPID;

/* DecChannelSetPSStreamIDs */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             channelId;
    unsigned long             videoStreamId;
    unsigned long             videoStreamIdExtEnable;
    unsigned long             videoStreamIdExt;
} DecCmdChannelSetPSStreamIDs;

/* DecChannelSetInputParams */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             channelId;
    eC011_SYNC_MODE           syncMode;
    eC011_UNMARKED_DISCONTINUITY_MODE        discontinuityNotify;
    eC011_UNMARKED_DISCONTINUITY_THRESHOLD   discontinuityPktThreshold;
    unsigned long             discontinuityThreshold;
    unsigned long             disableFlowControl;
    unsigned long             disablePCROffset;
} DecCmdChannelSetInputParams;

/* DecChannelSetVideoOutput */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    eC011_OUTPUT_PORT         portId;
    eC011_RESOLUTION          resolution;
    unsigned long             width;
    unsigned long             height;
    eC011_SCAN_MODE           scanMode;
    unsigned long             picRate;
    eC011_DISPLAY_OPTION      option;
    eC011_FORMATTING          formatMode;
    eC011_VSYNC_MODE          vsyncMode;
    unsigned long             numOsdBufs;
    unsigned long             numCcDataBufs;
    unsigned long             memOut;
    eC011_OUTPUT_CLIPPING     outputClipping;
    unsigned long             invertHddviSync;
    eC011_DISPLAY_MODE        pauseMode;
    eC011_DISPLAY_MODE        slowMode;
    eC011_DISPLAY_MODE        ffMode;
    unsigned long             vppPaddingValue; /* bits: 23-16 (Y), 15-8 (U), 7-0 (V) */
    unsigned long             extVideoClock;
    unsigned long             hddviEnable;
    unsigned long             numDramOutBufs;
} DecCmdChannelSetVideoOutput;

typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             status;
    unsigned long             osdBuf1;
    unsigned long             osdBuf2;
    unsigned long             ccDataBuf1;
    unsigned long             ccDataBuf2;
    unsigned long             memOutBuf;
} DecRspChannelSetVideoOutput;

/* DecChannelSetCustomVidOut */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    eC011_OUTPUT_PORT         portId;
    unsigned long             spl;
    unsigned long             spal;
    unsigned long             e2e;
    unsigned long             lpf;
    unsigned long             vlpf;
    unsigned long             vbsf1;
    unsigned long             vbff1;
    unsigned long             vbsf2;
    unsigned long             vbff2;
    unsigned long             f1id;
    unsigned long             f2id;
    unsigned long             gdband;
    unsigned long             vsdf0;
    unsigned long             vsdf1;
    unsigned long             hsyncst;
    unsigned long             hsyncsz;
    unsigned long             vsstf1;
    unsigned long             vsszf1;
    unsigned long             vsstf2;
    unsigned long             vsszf2;
    unsigned long             bkf1;
    unsigned long             bkf2;
    unsigned long             invertsync;
    unsigned long             wordmode;
    unsigned long             hdclock;
} DecCmdChannelSetCustomVidOut;

/* DecChannelSetOutputFormatting */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    eC011_OUTPUT_PORT         portId;
    eC011_SCALING             horizontalScaling;
    eC011_SCALING             verticalScaling;
    unsigned long             horizontalPhases;
    unsigned long             verticalPhases;
    eC011_EDGE_CONTROL        horizontalEdgeControl;
    eC011_EDGE_CONTROL        verticalEdgeControl;
    unsigned long             leftSize;
    unsigned long             rightSize;
    unsigned long             topSize;
    unsigned long             bottomSize;
    unsigned long             horizontalSize;
    unsigned long             verticalSize;
    long                      horizontalOrigin;
    long                      verticalOrigin;
    unsigned long             horizontalCropSize;
    unsigned long             verticalCropSize;
    unsigned long             lumaTopFieldOffset;
    unsigned long             lumaBottomFieldOffset;
    unsigned long             chromaTopFieldOffset;
    unsigned long             chromaBottomFieldOffset;
    eC011_SCAN_MODE           inputScanMode;
    eC011_DEINTERLACING       deinterlacing;
    unsigned long             horizontalDecimation_N;
    unsigned long             horizontalDecimation_M;
    unsigned long             verticalDecimation_N;
    unsigned long             verticalDecimation_M;
    unsigned long             horizontalDecimationVector [4];
    unsigned long             verticalDecimationVector [4];
} DecCmdChannelSetOutputFormatting;

/* DecCmdChannelSetPictureOutputFormatting */
typedef struct {
   unsigned long             command;
   unsigned long             sequence;
   eC011_OUTPUT_PORT         portId;
   eC011_SCALING             horizontalScaling;
   eC011_SCALING             verticalScaling;
   unsigned long             horizontalPhases;
   unsigned long             verticalPhases;
   eC011_EDGE_CONTROL        horizontalEdgeControl;
   eC011_EDGE_CONTROL        verticalEdgeControl;
   unsigned long             leftSize;
   unsigned long             rightSize;   /* 10 */
   unsigned long             topSize;
   unsigned long             bottomSize;
   unsigned long             horizontalSize;
   unsigned long             verticalSize;
   long                      horizontalOrigin;
   long                      verticalOrigin;
   unsigned long             horizontalCropSize;
   unsigned long             verticalCropSize;
   unsigned long             lumaTopFieldOffset;
   unsigned long             lumaBottomFieldOffset; /* 20 */
   unsigned long             chromaTopFieldOffset;
   unsigned long             chromaBottomFieldOffset;
   eC011_SCAN_MODE           inputScanMode;
   eC011_DEINTERLACING       deinterlacing;
   unsigned long             horizontalDecimationFactor; /* bits: 0-7 N; 8-15 M */
   unsigned long             verticalDecimationFactor;   /* bits: 0-7 Np; 8-15 Mp; 16-23 Ni; 24-31 Mi*/
   unsigned long             horizontalDecimationOutputSize;
   unsigned long             verticalDecimationOutputSize;
   unsigned long             horizontalScalingFactor;    /* bits: 0-7 N; 8-15 M */
   unsigned long             verticalScalingFactorProgressive; /* bits: 0-7 Nt; 8-15 Mt; 16-23 Nb; 24-31 Mb*/
   unsigned long             verticalScalingFactorInterlace;   /* bits: 0-7 Nt; 8-15 Mt; 16-23 Nb; 24-31 Mb*/
   unsigned long             Reserved[5];
} DecCmdChannelSetPictureOutputFormatting;

/* DecChannelSetScalingFilters */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    eC011_OUTPUT_PORT         portId;
    eC011_SCALING_TARGET      target;
    unsigned long             pixelPos;
    unsigned long             increment;
    long                      lumaCoeff1;
    long                      lumaCoeff2;
    long                      lumaCoeff3;
    long                      lumaCoeff4;
    long                      lumaCoeff5;
    eC011_NORMALIZATION       lumaNormalization;
    long                      chromaCoeff1;
    long                      chromaCoeff2;
    long                      chromaCoeff3;
    eC011_NORMALIZATION       chromaNormalization;
} DecCmdChannelSetScalingFilters;

/* DecChannelOsdMode */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    eC011_OUTPUT_PORT         portId;
    unsigned long             osdBuffer;
    unsigned long             fullRes;
} DecCmdChannelOsdMode;

/* DecChannelCcMode */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    eC011_OUTPUT_PORT         portId;
    unsigned long             ccBuffer;
} DecCmdChannelCcDataMode;

/* DecChannelDrop */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             channelId;
    unsigned long             numPicDrop;
   eC011_DROP_TYPE            dropType;      
} DecCmdChannelDrop;

/* DecChannelRelease */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             channelId;
    unsigned long             picBuffer;
} DecCmdChannelRelease;

/* DecChannelStreamSettings */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             channelId;
    unsigned long             pcrDelay;
} DecCmdChannelStreamSettings;

/* DecChannelPauseVideoOutput */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    eC011_OUTPUT_PORT         portId;
    eC011_PAUSE_TYPE          action;
} DecCmdChannelPauseVideoOutput;

/* DecChannelChange */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             channelId;
    unsigned long             pcrPid;
    unsigned long             videoPid;
    unsigned long             audio1Pid;
    unsigned long             audio2Pid;
    unsigned long             audio1StreamId;
    unsigned long             audio2StreamId;
} DecCmdChannelChange;

typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             status;
    unsigned long             channelId;
    unsigned long             picBuf;
    unsigned long             picRelBuf;
} DecRspChannelChange;

/* DecChannelSetSTC */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             channelId;
    unsigned long             stcValue0;
    unsigned long             stcValue1;
} DecCmdChannelSetSTC;

/* DecChannelSetPTS */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             channelId;
    unsigned long             ptsValue0;
    unsigned long             ptsValue1;
} DecCmdChannelSetPTS;

/* DecCreateAudioContext */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             contextId;
    unsigned long             inPort;
    unsigned long             streamId;
    unsigned long             subStreamId;
    uC011_AUDIO_PAYLOAD_INFO  payloadInfo;
    unsigned long             cdbBaseAddress;
    unsigned long             cdbEndAddress;
    unsigned long             itbBaseAddress;
    unsigned long             itbEndAddress;
   unsigned long             streamIdExtension;
} DecCmdCreateAudioContext;

/* DecCopyAudioContext */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             cdbBaseAddress;
    unsigned long             cdbEndAddress;
    unsigned long             itbBaseAddress;
    unsigned long             itbEndAddress;
} DecCmdCopyAudioContext;

/* DecDeleteAudioContext */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             contextId;
} DecCmdDeleteAudioContext;

/* DecSetDecryption */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             channelId;
    eC011_DESCRAMBLING_MODE   descramMode;
    unsigned long             evenKey0;
    unsigned long             evenKey1;
    unsigned long             evenKey2;
    unsigned long             evenKey3;
    unsigned long             oddKey0;
    unsigned long             oddKey1;
    unsigned long             oddKey2;
    unsigned long             oddKey3;
    eC011_KEY_EXCHANGE_MODE   keyExchangeMode;
    eC011_CT_STEALING_MODE    cipherTextStealingMode;
} DecCmdSetDecryption;

/* DecChanPicCapture */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             channelId;
} DecCmdChanPicCapture;

typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             status;
    unsigned long             pibAddress;
} DecRspChanPicCapture;

/* DecChanPause */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             channelId;
    eC011_PAUSE_MODE          enableState;
} DecCmdChannelPause;

/* DecChanPauseState */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             channelId;
} DecCmdChannelPauseState;

typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             status;
    eC011_PAUSE_MODE          pauseState;
} DecRspChannelPauseState;

/* DecChanSetSlowMotionRate */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             channelId;
    unsigned long             rate;	// 1 -> 1x (normal speed), 2 -> 2x slower, etc
} DecCmdChannelSetSlowMotionRate;

/* DecChanGetSlowMotionRate */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             channelId;
} DecCmdChannelGetSlowMotionRate;

typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             status;
    unsigned long             rate;	// 1 -> 1x (normal speed), 2 -> 2x slower, etc

} DecRspChannelGetSlowMotionRate;

/* DecChanSetFFRate */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             channelId;
    unsigned long             rate;	// 1 -> 1x (normal speed), 2 -> 2x faster, etc

} DecCmdChannelSetFFRate;

/* DecChanGetFFRate */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             channelId;
} DecCmdChannelGetFFRate;

typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             status;
    unsigned long             rate;	// 1 -> 1x (normal speed), 2 -> 2x faster, etc
} DecRspChannelGetFFRate;

/* DecChanFrameAdvance */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             channelId;
} DecCmdChannelFrameAdvance;

/* DecChanSetSkipPictureMode */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             channelId;
    eC011_SKIP_PIC_MODE       skipMode;
} DecCmdChannelSetSkipPictureMode;

/* DecChanGetSkipPictureMode */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             channelId;
} DecCmdChannelGetSkipPictureMode;

typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             status;
    eC011_SKIP_PIC_MODE       skipMode;
} DecRspChannelGetSkipPictureMode;

/* DecChanFillPictureBuffer */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             channelId;
    unsigned long             yuvValue;
} DecCmdChannelFillPictureBuffer;

/* DecChanSetContinuityCheck */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             channelId;
    unsigned long             enable;
} DecCmdChannelSetContinuityCheck;

/* DecChanGetContinuityCheck */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             channelId;
} DecCmdChannelGetContinuityCheck;

typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             status;
    unsigned long             enable;
} DecRspChannelGetContinuityCheck;

/* DecChanSetBRCMTrickMode */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             channelId;
    unsigned long             enable;
    unsigned long             reverseField;
} DecCmdChannelSetBRCMTrickMode;

/* DecChanGetBRCMTrickMode */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             channelId;
} DecCmdChannelGetBRCMTrickMode;

typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             status;
    unsigned long             brcmTrickMode;
} DecRspChannelGetBRCMTrickMode;

/* DecChanReverseFieldStatus */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             channelId;
} DecCmdChannelReverseFieldStatus;

typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             status;
    unsigned long             reverseField;
} DecRspChannelReverseFieldStatus;

/* DecChanIPictureFound */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             channelId;
} DecCmdChannelIPictureFound;

typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             status;
    unsigned long             iPictureFound;
} DecRspChannelIPictureFound;

/* DecCmdChannelSetParameter */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             channelId;
    long                      videoDisplayOffset;
    long                      ptsStcPhaseThreshold;
    /* add more paras below ... */
} DecCmdChannelSetParameter;

/* DecCmdChannelSetUserDataMode */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             channelId;
    unsigned long             enable;
} DecCmdChannelSetUserDataMode;

/* DecCmdChannelSetPauseDisplayMode */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             channelId;
    eC011_DISPLAY_MODE        displayMode;
} DecCmdChannelSetPauseDisplayMode;

/* DecCmdChannelSetSlowDisplayMode */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             channelId;
    eC011_DISPLAY_MODE        displayMode;
} DecCmdChannelSetSlowDisplayMode;

/* DecCmdChannelSetFastForwardDisplayMode */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             channelId;
    eC011_DISPLAY_MODE        displayMode;
} DecCmdChannelSetFastForwardDisplayMode;

/* DecCmdChannelSetDisplayMode */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             channelId;
    eC011_DISPLAY_MODE        displayMode;
} DecCmdChannelSetDisplayMode;

/* DecCmdChannelGetDisplayMode */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             channelId;
} DecCmdChannelGetDisplayMode;

/* DecRspChannelGetDisplayMode */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             status;
    eC011_DISPLAY_MODE        displayMode;
} DecRspChannelGetDisplayMode;

/* DecCmdChannelSetReverseField */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             channelId;
	unsigned long             enable;
} DecCmdChannelSetReverseField;

/* DecCmdChannelSetDisplayTimingMode */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             channelId;
    eC011_DISPLAY_TIMING_MODE displayTiming;
} DecCmdChannelSetDisplayTimingMode;

/* DecCmdChannelStreamOpen */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    eC011_INPUT_PORT          inPort;
    eC011_STREAM_TYPE         streamType;
} DecCmdChannelStreamOpen;

typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             status;
    unsigned long             channelId;
    unsigned long             channelStatus;
} DecRspChannelStreamOpen;

/* DecChannelStartVideo */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             channelId;
    eC011_OUTPUT_PORT         outVidPort;
    eC011_MAX_PICSIZE         maxPicSize;
    eC011_OUTCTRL_MODE        outCtrlMode;
    eC011_CHANNEL_TYPE        chanType;
    unsigned long             defaultFrameRate;//reservedWord7;
    eC011_VIDEO_ALG           videoAlg;
    eC011_VIDEO_SOURCE_MODE   sourceMode;
    eC011_PULLDOWN_MODE       pulldown;
    eC011_PICTURE_INFO_MODE   picInfo;
    eC011_DISPLAY_ORDER       displayOrder;
    unsigned long             decOperationMode; //reservedWord13;
    unsigned long             MaxFrameRateMode;//reservedWord14;
    unsigned long             streamId;
    eC011_DEBLOCKING_MODE     deblocking;
    eC011_EXTERNAL_VCXO_MODE  vcxoControl;
    eC011_DISPLAY_TIMING_MODE displayTiming;
    long                      videoDisplayOffset;
    eC011_USER_DATA_MODE      userDataMode;
    unsigned long             enableUserDataInterrupt;
    unsigned long             ptsStcDiffThreshold;
    unsigned long             stcPtsDiffThreshold;
    unsigned long             enableFirstPtsInterrupt;
    unsigned long             enableStcPtsThresholdInterrupt;
    unsigned long             frameRateDefinition;
    unsigned long             hostDmaInterruptEnable;
    unsigned long             asynchEventNotifyEnable;
    unsigned long             enablePtsStcChangeInterrupt;
    unsigned long             enablePtsErrorInterrupt;
	unsigned long             enableFgt;
	unsigned long             enable23_297FrameRateOutput;
    unsigned long             enableVideoDataUnderflowInterrupt;
    unsigned long             reservedWord34;
    unsigned long             pictureInfoInterruptEnable;
} DecCmdChannelStartVideo;

typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             status;
    unsigned long             picBuf;
    unsigned long             picRelBuf;
    unsigned long             picInfoDeliveryQ;
    unsigned long             picInfoReleaseQ;
    unsigned long             channelStatus;
    unsigned long             userDataDeliveryQ;
    unsigned long             userDataReleaseQ;
    unsigned long             transportStreamCaptureAddr;
    unsigned long             asyncEventQ;
} DecRspChannelStartVideo;

/* DecChannelStopVideo */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             channelId;
    eC011_PIC_REL_MODE        pictureRelease;
    eC011_LASTPIC_DISPLAY     lastPicDisplay;
} DecCmdChannelStopVideo;

/* DecCmdChannelSetPanScanMode */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             channelId;
   eC011_PAN_SCAN_MODE       ePanScanMode;
} DecCmdChannelSetPanScanMode;

/* DecChannelStartDisplayAtPTS */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             channelId;
    unsigned long             ptsValue0;
    unsigned long             ptsValue1;
} DecCmdChannelStartDisplayAtPTS;

/* DecChannelStopDisplayAtPTS */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             channelId;
    unsigned long             ptsValue0;
    unsigned long             ptsValue1;
} DecCmdChannelStopDisplayAtPTS;

/* DecChannelDisplayPauseUntoPTS */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             channelId;
    unsigned long             pausePtsValue0;
    unsigned long             pausePtsValue1;
    unsigned long             displayPtsValue0;
    unsigned long             displayPtsValue1;    
    long 		      pauseLoopAroundCounter;
    long 	              displayLoopAroundCounter;    
    int 		      pauseUntoPtsValid;
    int 		      displayUntoPtsValid;
} DecCmdChannelDisplayPauseUntoPTS;

/* DecCmdChanSetPtsStcDiffThreshold */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             channelId;
    unsigned long             ptsStcDiffThreshold;
} DecCmdChanSetPtsStcDiffThreshold;

/* DecChanSetDisplayOrder */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             channelId;
    unsigned long             decodeOrder;  // 0: displayOrder, 1: decodeOrder
} DecCmdChannelSetDisplayOrder;

/* DecChanGetDisplayOrder */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             channelId;
} DecCmdChannelGetDisplayOrder;

typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             channelId;
    unsigned long             decodeOrder;  // 0: displayOrder, 1: decodeOrder
} DecRspChannelGetDisplayOrder;

typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             channelId;
    unsigned long 		  CodeInBuffLowerThreshold;
    unsigned long 		  CodeInBuffHigherThreshold;
    int 				  MaxNumVsyncsCodeInEmpty;
    int 				  MaxNumVsyncsCodeInFull;
} DecCmdChanSetParametersForHardResetInterruptToHost;

/* EncChannelOpen */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    sC011_ENC_SEQ_PARAM       seqParam;     // sequence paramaters
    sC011_ENC_PIC_PARAM       picParam;     // picture parameters
    sC011_ENC_CODING_PARAM    codingParam;  // coding parameters
    sC011_ENC_VID_IN_PARAM    vidInParam;   // video-in parameters
    sC011_ENC_CODE_OUT_PARAM  codeOutParam; // code-out parameters
} EncCmdChannelOpen;

/* EncChannelClose */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             channelId;
    unsigned long             condClose;
    unsigned long             lastPicId;
} EncCmdChannelClose;

/* EncChannelActivate */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             channelId;
} EncCmdChannelActivate;

/* EncChannelControl */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             channelId;
    unsigned long             condControl;
    unsigned long             picId;
    eC011_ENC_CTRL_CODE       controlCode;
    uC011_ENC_CTRL_PARAM      controlParam;
} EncCmdChannelControl;

/* EncChannelStatistics */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             channelId;
} EncCmdChannelStatistics;

/* DecChannelColorSpaceConv */
typedef struct {
   unsigned long             command;
   unsigned long             sequence;
   eC011_OUTPUT_PORT         portId;
   eC011_DEC_CSC_SETUP       enable;
   unsigned long             padInput; /* if the padded pixels need to be converted. 1: input; 0:output */
   unsigned long             lumaCoefY;
   unsigned long             lumaCoefU;
   unsigned long             lumaDCOffset;
   unsigned long             lumaOffset;
   unsigned long             lumaCoefChrV;
   unsigned long             chrUCoefY;
   unsigned long             chrUCoefU;
   unsigned long             chrUDCOffset;
   unsigned long             chrUOffset;
   unsigned long             chrUCoefChrV;
   unsigned long             chrVCoefY;
   unsigned long             chrVCoefU;
   unsigned long             chrVDCOffset;
   unsigned long             chrVOffset;
   unsigned long             chrVCoefChrV;
} DecCmdChannelColorSpaceConv;

typedef struct {
   unsigned long                    command;
   unsigned long                    sequence;
   eC011_OUTPUT_PORT                portId;
   eC011_DEC_RANGE_REMAP_SETUP      enable;
   eC011_DEC_RANGE_REMAP_VC1PROFILE vc1Profile;
   unsigned long                    lumaEnable;
   unsigned long                    lumaMultiplier;
   unsigned long                    chromaEnable;
   unsigned long                    chromaMultiplier;
} DecCmdChannelSetRangeRemap;

/* DecChanSetFgt */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             channelId;
    unsigned long             on;  // 0: fgt off, 1: fgt on
} DecCmdChannelSetFgt;

/* DecChanSetLastPicturePadding */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    eC011_OUTPUT_PORT         portId;
    unsigned long             paddingValue;   // bits: 23-16 (Y), 15-8 (U), 7-0 (V)
    unsigned long             padFullScreen;  // 0: fgt off, 1: fgt on
} DecCmdChannelSetLastPicturePadding;

/* DecChanSetHostTrickMode */
typedef struct {
   unsigned long             command;
   unsigned long             sequence;
   unsigned long             channelId;
   unsigned long             enable;  /* 0:disable; 1:enable */
} DecCmdChannelSetHostTrickMode;

/* DecChanSetOperationMode */
typedef struct {
   unsigned long             command;
   unsigned long             sequence;
   unsigned long             reserved;
   eC011_DEC_OPERATION_MODE  mode;
} DecCmdChannelSetOperationMode;

/* DecChanSendCompressedBuffer */
typedef struct {
   unsigned long             command;
   unsigned long             sequence;
   unsigned long             dramInBufAddr;
   unsigned long             dataSizeInBytes;
} DecCmdSendCompressedBuffer;

/* DecChanSetClipping */
typedef struct {
   unsigned long             command;
   unsigned long             sequence;
   eC011_OUTPUT_PORT         portId;
   eC011_OUTPUT_CLIPPING     outputClipping;
} DecCmdSetClipping;

/* DecSetContentKey */
typedef struct {
    unsigned long            command;
    unsigned long            sequence;
    unsigned long            channelId;
	unsigned long            flags;
    unsigned long            inputKey0;  // bits 31:0
    unsigned long            inputKey1;  // bits 33:63
    unsigned long            inputKey2;
    unsigned long            inputKey3;
    unsigned long            inputIv0;  // bits 31:0
    unsigned long            inputIv1;
    unsigned long            inputIv2;
    unsigned long            inputIv3;
    unsigned long            outputKey0;
    unsigned long            outputKey1;
    unsigned long            outputKey2;
    unsigned long            outputKey3;
    unsigned long            outputIv0;
    unsigned long            outputIv1;
    unsigned long            outputIv2;
    unsigned long            outputIv3;
	unsigned long			 outputStripeStart;	  // 0 based stripe encrypt start number, don't use 0
	unsigned long			 outputStripeNumber;  // 0 based number of stripes to encrypt
	unsigned long			 outputStripeLines;   // 0 = 256 lines, otherwise actual number of lines, start on second line
	unsigned long			 outputStripeLineStart;   // 0 based start line number

	unsigned long            outputMode;
	unsigned long            outputyScramLen;
	unsigned long            outputuvLen;
	unsigned long            outputuvOffset;

	unsigned long            outputyLen;
	unsigned long            outputyOffset;
	union {												// Adaptive Output Encryption Percentages
		struct {
			unsigned long	outputClearPercent:8;		// Clear Percentage
			unsigned long	outputEncryptPercent:8;		// Encrypt Percentage
			unsigned long	outputScramPercent:8;		// Scramble Percentage
			unsigned long	output422Mode:8;			// 422 Mode
		} u;
		unsigned long	outputPercentage;
	}adapt;
	unsigned long            outputReserved1;
	
} DecCmdSetContentKey;

/* DecSetSessionKey */
typedef struct {
    unsigned long command;
    unsigned long sequence;
    unsigned long channelId;
    unsigned long flags;
    unsigned long sessionData[32]; // 128 bytes of cipher data.
} DecCmdSetSessionKey;

typedef struct {
   unsigned long        command;
   unsigned long        sequence;
   unsigned long        channelId;
   unsigned long		flags;
   unsigned long		reserved[4];
} DecCmdFormatChangeAck;

/* common response structure */
typedef struct {
    unsigned long             command;
    unsigned long             sequence;
    unsigned long             status;
} C011RspReset,
   C011RspGPIO,
   DecRspChannelClose,
   DecRspChannelActivate,
   DecRspChannelFlush,
   DecRspChannelTrickPlay,
   DecRspChannelSetTSPIDs,
   DecRspChannelSetPcrPID,
   DecRspChannelSetVideoPID,
   DecRspChannelSetPSStreamIDs,
   DecRspChannelSetInputParams,
   DecRspChannelSetOutputFormatting,
   DecRspChannelSetScalingFilters,
   DecRspChannelOsdMode,
   DecRspChannelCcDataMode,
   DecRspChannelDrop,
   DecRspChannelRelease,
   DecRspChannelStreamSettings,
   DecRspChannelPauseVideoOutput,
   DecRspChannelSetSTC,
   DecRspChannelSetPTS,
   DecRspChannelSetCustomVidOut,
   DecRspCreateAudioContext,
   DecRspDeleteAudioContext,
   DecRspCopyAudioContext,
   DecRspSetDecryption,
   DecRspChannelPause,
   DecRspChannelSetSlowMotionRate,
   DecRspChannelSetFFRate,
   DecRspChannelFrameAdvance,
   DecRspChannelSetSkipPictureMode,
   DecRspChannelFillPictureBuffer,
   DecRspChannelSetContinuityCheck,
   DecRspChannelSetBRCMTrickMode,
   DecRspChannelSetDisplayOrder,
   EncRspChannelOpen,
   EncRspChannelClose,
   EncRspChannelActivate,
   EncRspChannelControl,
   EncRspChannelStatistics,
   EncNotifyChannelEvent,
   DecRspChannelSetParameter,
   DecRspChannelSetUserDataMode,
   DecRspChannelSetPauseDisplayMode,
   DecRspChannelSetSlowDisplayMode,
   DecRspChannelSetFastForwardDisplayMode,
   DecRspChannelSetDisplayTimingMode,
   DecRspChannelSetDisplayMode,
   DecRspChannelSetReverseField,
   DecRspChannelStopVideo,
   DecRspChannelSetPanScanMode,
   DecRspChannelStartDisplayAtPTS,
   DecRspChannelStopDisplayAtPTS,
   DecRspChannelDisplayPauseUntoPTS,
   DecRspChannelColorSpaceConv,
   DecRspChannelSetRangeRemap,
   DecRspChannelSetFgt,
   DecRspChannelSetLastPicturePadding,
   DecRspChannelSetHostTrickMode,
   DecRspChannelSetOperationMode,
   DecRspChannelSetPtsStcDiffThreshold,
   DecRspSendCompressedBuffer,
   DecRspSetClipping,
   DecRspChannelSetParametersForHardResetInterruptToHost,
   DecRspSetContentKey,
   DecRspSetSessionKey,
   DecRspFormatChangeAck,

   DecRspChannelUnknownCmd;
#endif // __INC_C011API_H__
