/***************************************************************************
 * Copyright (c) 2004-2009, Broadcom Corporation.
 *
 *  Name: 7411d.h
 *
 *  Description: Decoder register, status and parameter definitions
 *
 * Revision History:
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
 ********************************************************************
 * This header is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 2.1 of the License.
 *
 * This header is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * You should have received a copy of the GNU Lesser General Public License
 * along with this header.  If not, see <http://www.gnu.org/licenses/>.
 ***************************************************************************/

#ifndef __INC_C011API_H__
#define __INC_C011API_H__

#include "vdec_info.h"

// maximum number of host commands and responses
#define C011_MAX_HST_CMDS	   (16)
#define C011_MAX_HST_RSPS	   (16)
#define C011_MAX_HST_CMDQ_SIZE      (64)

// default success return code
#define C011_RET_SUCCESS	    (0x0)

// default failure return code
#define C011_RET_FAILURE	    (0xFFFFFFFF)

#define C011_RET_UNKNOWN	    (0x1)

// Stream ARC base address
#define STR_BASE		    (0x00000000)

// Stream ARC <- Host (default) address
#define STR_HOSTRCV		 (STR_BASE + 0x100)

// Stream ARC -> Host address
#define STR_HOSTSND		 (STR_BASE + 0x200)

#define eCMD_C011_CMD_BASE	  (0x73763000)

/* host commands */
typedef enum
{

    eCMD_TS_GET_NEXT_PIC		= 0x7376F100, // debug get next picture
    eCMD_TS_GET_LAST_PIC		= 0x7376F102, // debug get last pic status
    eCMD_TS_READ_WRITE_MEM	      = 0x7376F104, // debug read write memory

    /* New API commands */
    /* General commands */
    eCMD_C011_INIT		      = eCMD_C011_CMD_BASE + 0x01,
    eCMD_C011_RESET		     = eCMD_C011_CMD_BASE + 0x02,
    eCMD_C011_SELF_TEST		 = eCMD_C011_CMD_BASE + 0x03,
    eCMD_C011_GET_VERSION	       = eCMD_C011_CMD_BASE + 0x04,
    eCMD_C011_GPIO		      = eCMD_C011_CMD_BASE + 0x05,
    eCMD_C011_DEBUG_SETUP	       = eCMD_C011_CMD_BASE + 0x06,

    /* Decoding commands */
    eCMD_C011_DEC_CHAN_OPEN	     = eCMD_C011_CMD_BASE + 0x100,
    eCMD_C011_DEC_CHAN_CLOSE	    = eCMD_C011_CMD_BASE + 0x101,
    eCMD_C011_DEC_CHAN_ACTIVATE	 = eCMD_C011_CMD_BASE + 0x102,
    eCMD_C011_DEC_CHAN_STATUS	   = eCMD_C011_CMD_BASE + 0x103,
    eCMD_C011_DEC_CHAN_FLUSH	    = eCMD_C011_CMD_BASE + 0x104,
    eCMD_C011_DEC_CHAN_TRICK_PLAY       = eCMD_C011_CMD_BASE + 0x105,
    eCMD_C011_DEC_CHAN_TS_PIDS	  = eCMD_C011_CMD_BASE + 0x106,
    eCMD_C011_DEC_CHAN_PS_STREAM_ID     = eCMD_C011_CMD_BASE + 0x107,
    eCMD_C011_DEC_CHAN_INPUT_PARAMS     = eCMD_C011_CMD_BASE + 0x108,
    eCMD_C011_DEC_CHAN_VIDEO_OUTPUT     = eCMD_C011_CMD_BASE + 0x109,
    eCMD_C011_DEC_CHAN_OUTPUT_FORMAT    = eCMD_C011_CMD_BASE + 0x10A,
    eCMD_C011_DEC_CHAN_SCALING_FILTERS  = eCMD_C011_CMD_BASE + 0x10B,
    eCMD_C011_DEC_CHAN_OSD_MODE	 = eCMD_C011_CMD_BASE + 0x10D,
    eCMD_C011_DEC_CHAN_DROP	     = eCMD_C011_CMD_BASE + 0x10E,
    eCMD_C011_DEC_CHAN_RELEASE	  = eCMD_C011_CMD_BASE + 0x10F,
    eCMD_C011_DEC_CHAN_STREAM_SETTINGS  = eCMD_C011_CMD_BASE + 0x110,
    eCMD_C011_DEC_CHAN_PAUSE_OUTPUT     = eCMD_C011_CMD_BASE + 0x111,
    eCMD_C011_DEC_CHAN_CHANGE	   = eCMD_C011_CMD_BASE + 0x112,
    eCMD_C011_DEC_CHAN_SET_STC	  = eCMD_C011_CMD_BASE + 0x113,
    eCMD_C011_DEC_CHAN_SET_PTS	  = eCMD_C011_CMD_BASE + 0x114,
    eCMD_C011_DEC_CHAN_CC_MODE	  = eCMD_C011_CMD_BASE + 0x115,
    eCMD_C011_DEC_CREATE_AUDIO_CONTEXT  = eCMD_C011_CMD_BASE + 0x116,
    eCMD_C011_DEC_COPY_AUDIO_CONTEXT    = eCMD_C011_CMD_BASE + 0x117,
    eCMD_C011_DEC_DELETE_AUDIO_CONTEXT  = eCMD_C011_CMD_BASE + 0x118,
    eCMD_C011_DEC_CHAN_SET_DECYPTION    = eCMD_C011_CMD_BASE + 0x119,
    eCMD_C011_DEC_CHAN_START_VIDEO      = eCMD_C011_CMD_BASE + 0x11A,
    eCMD_C011_DEC_CHAN_STOP_VIDEO       = eCMD_C011_CMD_BASE + 0x11B,
    eCMD_C011_DEC_CHAN_PIC_CAPTURE      = eCMD_C011_CMD_BASE + 0x11C,
    eCMD_C011_DEC_CHAN_PAUSE	    = eCMD_C011_CMD_BASE + 0x11D,
    eCMD_C011_DEC_CHAN_PAUSE_STATE      = eCMD_C011_CMD_BASE + 0x11E,
    eCMD_C011_DEC_CHAN_SET_SLOWM_RATE   = eCMD_C011_CMD_BASE + 0x11F,
    eCMD_C011_DEC_CHAN_GET_SLOWM_RATE   = eCMD_C011_CMD_BASE + 0x120,
    eCMD_C011_DEC_CHAN_SET_FF_RATE      = eCMD_C011_CMD_BASE + 0x121,
    eCMD_C011_DEC_CHAN_GET_FF_RATE      = eCMD_C011_CMD_BASE + 0x122,
    eCMD_C011_DEC_CHAN_FRAME_ADVANCE    = eCMD_C011_CMD_BASE + 0x123,
    eCMD_C011_DEC_CHAN_SET_SKIP_PIC_MODE	= eCMD_C011_CMD_BASE + 0x124,
    eCMD_C011_DEC_CHAN_GET_SKIP_PIC_MODE	= eCMD_C011_CMD_BASE + 0x125,
    eCMD_C011_DEC_CHAN_FILL_PIC_BUF	     = eCMD_C011_CMD_BASE + 0x126,
    eCMD_C011_DEC_CHAN_SET_CONTINUITY_CHECK     = eCMD_C011_CMD_BASE + 0x127,
    eCMD_C011_DEC_CHAN_GET_CONTINUITY_CHECK     = eCMD_C011_CMD_BASE + 0x128,
    eCMD_C011_DEC_CHAN_SET_BRCM_TRICK_MODE	= eCMD_C011_CMD_BASE + 0x129,
    eCMD_C011_DEC_CHAN_GET_BRCM_TRICK_MODE      = eCMD_C011_CMD_BASE + 0x12A,
    eCMD_C011_DEC_CHAN_REVERSE_FIELD_STATUS     = eCMD_C011_CMD_BASE + 0x12B,
    eCMD_C011_DEC_CHAN_I_PICTURE_FOUND	  = eCMD_C011_CMD_BASE + 0x12C,
    eCMD_C011_DEC_CHAN_SET_PARAMETER	    = eCMD_C011_CMD_BASE + 0x12D,
    eCMD_C011_DEC_CHAN_SET_USER_DATA_MODE       = eCMD_C011_CMD_BASE + 0x12E,
    eCMD_C011_DEC_CHAN_SET_PAUSE_DISPLAY_MODE   = eCMD_C011_CMD_BASE + 0x12F,
    eCMD_C011_DEC_CHAN_SET_SLOW_DISPLAY_MODE    = eCMD_C011_CMD_BASE + 0x130,
    eCMD_C011_DEC_CHAN_SET_FF_DISPLAY_MODE	= eCMD_C011_CMD_BASE + 0x131,
    eCMD_C011_DEC_CHAN_SET_DISPLAY_TIMING_MODE	= eCMD_C011_CMD_BASE + 0x132,
    eCMD_C011_DEC_CHAN_SET_DISPLAY_MODE		= eCMD_C011_CMD_BASE + 0x133,
    eCMD_C011_DEC_CHAN_GET_DISPLAY_MODE		= eCMD_C011_CMD_BASE + 0x134,
    eCMD_C011_DEC_CHAN_SET_REVERSE_FIELD	= eCMD_C011_CMD_BASE + 0x135,
    eCMD_C011_DEC_CHAN_STREAM_OPEN	      = eCMD_C011_CMD_BASE + 0x136,
    eCMD_C011_DEC_CHAN_SET_PCR_PID	      = eCMD_C011_CMD_BASE + 0x137,
    eCMD_C011_DEC_CHAN_SET_VID_PID	      = eCMD_C011_CMD_BASE + 0x138,
    eCMD_C011_DEC_CHAN_SET_PAN_SCAN_MODE	= eCMD_C011_CMD_BASE + 0x139,
    eCMD_C011_DEC_CHAN_START_DISPLAY_AT_PTS     = eCMD_C011_CMD_BASE + 0x140,
    eCMD_C011_DEC_CHAN_STOP_DISPLAY_AT_PTS      = eCMD_C011_CMD_BASE + 0x141,
    eCMD_C011_DEC_CHAN_SET_DISPLAY_ORDER	= eCMD_C011_CMD_BASE + 0x142,
    eCMD_C011_DEC_CHAN_GET_DISPLAY_ORDER	= eCMD_C011_CMD_BASE + 0x143,
    eCMD_C011_DEC_CHAN_SET_HOST_TRICK_MODE      = eCMD_C011_CMD_BASE + 0x144,
    eCMD_C011_DEC_CHAN_SET_OPERATION_MODE       = eCMD_C011_CMD_BASE + 0x145,
    eCMD_C011_DEC_CHAN_DISPLAY_PAUSE_UNTO_PTS   = eCMD_C011_CMD_BASE + 0x146,
    eCMD_C011_DEC_CHAN_SET_PTS_STC_DIFF_THRESHOLD = eCMD_C011_CMD_BASE + 0x147,
    eCMD_C011_DEC_CHAN_SEND_COMPRESSED_BUF      = eCMD_C011_CMD_BASE + 0x148,
    eCMD_C011_DEC_CHAN_SET_CLIPPING			= eCMD_C011_CMD_BASE + 0x149,
    eCMD_C011_DEC_CHAN_SET_PARAMETERS_FOR_HARD_RESET_INTERRUPT_TO_HOST = eCMD_C011_CMD_BASE + 0x150,

    /* Decoder RevD commands */
    eCMD_C011_DEC_CHAN_SET_CSC	  = eCMD_C011_CMD_BASE + 0x180,  /* CSC:color space conversion */
    eCMD_C011_DEC_CHAN_SET_RANGE_REMAP  = eCMD_C011_CMD_BASE + 0x181,
    eCMD_C011_DEC_CHAN_SET_FGT			= eCMD_C011_CMD_BASE + 0x182,
    eCMD_C011_DEC_CHAN_SET_LASTPICTURE_PADDING	= eCMD_C011_CMD_BASE + 0x183,  // not implemented yet in Rev D main

    /* Decoder 7412 commands */
    eCMD_C011_DEC_CHAN_SET_CONTENT_KEY	= eCMD_C011_CMD_BASE + 0x190,  // 7412 only
    eCMD_C011_DEC_CHAN_SET_SESSION_KEY	= eCMD_C011_CMD_BASE + 0x191,  // 7412 only
    eCMD_C011_DEC_CHAN_FMT_CHANGE_ACK	= eCMD_C011_CMD_BASE + 0x192,  // 7412 only


    eCMD_C011_DEC_CHAN_CUSTOM_VIDOUT    = eCMD_C011_CMD_BASE + 0x1FF,
    /* Encoding commands */
    eCMD_C011_ENC_CHAN_OPEN	     = eCMD_C011_CMD_BASE + 0x200,
    eCMD_C011_ENC_CHAN_CLOSE	    = eCMD_C011_CMD_BASE + 0x201,
    eCMD_C011_ENC_CHAN_ACTIVATE	 = eCMD_C011_CMD_BASE + 0x202,
    eCMD_C011_ENC_CHAN_CONTROL	  = eCMD_C011_CMD_BASE + 0x203,
    eCMD_C011_ENC_CHAN_STATISTICS       = eCMD_C011_CMD_BASE + 0x204,

    eNOTIFY_C011_ENC_CHAN_EVENT	 = eCMD_C011_CMD_BASE + 0x210,

} eC011_TS_CMD;

/* ARCs */
/*  - eCMD_C011_INIT */
#define C011_STREAM_ARC	     (0x00000001) /* stream ARC */
#define C011_VDEC_ARC	       (0x00000002) /* video decoder ARC */
#define C011_SID_ARC		(0x00000004) /* SID ARC */

/* Interrupt Status Register definition for HostControlled mode:
 * 16 bits available for general use, bit-31 dedicated for MailBox Interrupt!
 */
#define RESERVED_FOR_FUTURE_USE_0   (1<<0)  /* BIT 0 */
#define PICTURE_INFO_AVAILABLE      (1<<1)
#define PICTURE_DONE_MARKER	 (1<<2)
#define ASYNC_EVENTQ		    (1<<3)
#define INPUT_DMA_DONE	      (1<<4)
#define OUTPUT_DMA_DONE	     (1<<5)
#define VIDEO_DATA_UNDERFLOW_IN0    (1<<6)
#define CRC_DATA_AVAILABLE_IN0      (1<<7)
#define SID_SERVICE		 (1<<8)
#define USER_DATA_AVAILABLE_IN0     (1<<9)
#define NEW_PCR_OFFSET	      (1<<10) /* New PCR Offset received */
#define RESERVED_FOR_FUTURE_USE_2   (1<<11)
#define HOST_DMA_COMPLETE	   (1<<12)
#define RAPTOR_SERVICE	      (1<<13)
#define INITIAL_PTS		 (1<<14) /* STC Request Interrupt */
#define PTS_DISCONTINUITY	   (1<<15) /* PTS Error Interrupt */

#define COMMAND_RESPONSE	    (1<<31) /* Command Response Register Interrupt */

/* Asynchronous Events - enabled via ChannelOpen */
#define EVENT_PRESENTATION_START	(0x00000001)
#define EVENT_PRESENTATION_STOP		(0x00000002)
#define EVENT_ILLEGAL_STREAM		(0x00000003)
#define EVENT_UNDERFLOW			(0x00000004)
#define EVENT_VSYNC_ERROR		(0x00000005)
#define INPUT_DMA_BUFFER0_RELEASE	(0x00000006)
#define INPUT_DMA_BUFFER1_RELEASE	(0x00000007)


/* interrupt control */
/*  - eCMD_C011_INIT */
typedef enum
{
    eC011_INT_DISABLE		= 0x00000000,
    eC011_INT_ENABLE		= 0x00000001,
    eC011_INT_ENABLE_RAPTOR	= 0x00000003,
} eC011_INT_CONTROL;


/*chdDiskformatBD*/
/*  - eCMD_C011_INIT */
#define eC011_DSK_BLURAY (1<<10)
#define eC011_RES_MASK   0x0000F800

/* test id */
/*  - eCMD_C011_SELF_TEST */
typedef enum
{
    eC011_TEST_SHORT_MEMORY	= 0x00000001,
    eC011_TEST_LONG_MEMORY	= 0x00000002,
    eC011_TEST_SHORT_REGISTER	= 0x00000003,
    eC011_TEST_LONG_REGISTER	= 0x00000004,
    eC011_TEST_DECODE_LOOPBACK	= 0x00000005,
    eC011_TEST_ENCODE_LOOPBACK	= 0x00000006,
} eC011_TEST_ID;

/* gpio control */
/*  - eCMD_C011_GPIO */
typedef enum
{
    eC011_GPIO_CONTROL_INTERNAL	= 0x00000000,
    eC011_GPIO_CONTROL_HOST	= 0x00000001,
} eC011_GPIO_CONTROL;

/* input port */
/*  - eCMD_C011_DEC_CHAN_OPEN */
typedef enum
{
    eC011_IN_PORT0	= 0x00000000, // input port 0
    eC011_IN_PORT1	= 0x00000001, // input port 1
    eC011_IN_HOST_PORT0	= 0x00000010, // host port (OR this bit to specify which port is host mode)
    eC011_IN_HOST_PORT1	= 0x00000011, // host port (OR this bit to specify which port is host mode)
    eC011_IN_DRAM	= 0x00000100, // SDRAM
} eC011_INPUT_PORT;

/* output port */
/*  - eCMD_C011_DEC_CHAN_OPEN */
typedef enum
{
    eC011_OUT_PORT0	= 0x00000000, // output port 0
    eC011_OUT_PORT1	= 0x00000001, // output port 1
    eC011_OUT_BOTH	= 0x00000002, // output port 0 and 1
    eC011_OUT_HOST	= 0x00000010, // host port (OR this bit to specify which port is host mode)
} eC011_OUTPUT_PORT;

/* stream types */
/*  - eCMD_C011_DEC_CHAN_OPEN */
typedef enum
{
    eC011_STREAM_TYPE_ES	= 0x00000000, // elementary stream
    eC011_STREAM_TYPE_PES	= 0x00000001, // packetized elementary stream
    eC011_STREAM_TYPE_TS	= 0x00000002, // transport stream
    eC011_STREAM_TYPE_TSD_ES	= 0x00000003, // legacy 130-byte transport stream with ES
    eC011_STREAM_TYPE_TSD_PES	= 0x00000004, // legacy 130-byte transport stream with PES
    eC011_STREAM_TYPE_CMS	= 0x00000005, // compressed multistream
    eC011_STREAM_TYPE_ES_W_TSHDR = 0x00000006, // elementary stream with fixed TS headers

    eC011_STREAM_TYPE_ES_DBG	= 0x80000000, // debug elementary stream
    eC011_STREAM_TYPE_PES_DBG	= 0x80000001, // debug packetized elementary stream
    eC011_STREAM_TYPE_TS_DBG	= 0x80000002, // debug transport stream
    eC011_STREAM_TYPE_TSD_ES_DBG  = 0x80000003, // debug legacy 130-byte transport stream with ES
    eC011_STREAM_TYPE_TSD_PES_DBG = 0x80000004, // debug legacy 130-byte transport stream with PES
    eC011_STREAM_TYPE_CMS_DBG	= 0x80000005, // debug compressed multistream
    eC011_STREAM_TYPE_ES_FIXED_TS_DBG = 0x80000006, // debug elementary stream with fixed TS headers

} eC011_STREAM_TYPE;

/* maximum picture size */
/*  - eCMD_C011_DEC_CHAN_OPEN */
typedef enum
{
    eC011_MAX_PICSIZE_HD	= 0x00000000, // 1920x1088
    eC011_MAX_PICSIZE_SD	= 0x00000001, // 720x576
} eC011_MAX_PICSIZE;

/* output control mode */
/*  - eCMD_C011_DEC_CHAN_OPEN */
typedef enum
{
    eC011_OUTCTRL_VIDEO_TIMING	= 0x00000000,
    eC011_OUTCTRL_HOST_TIMING	= 0x00000001,
} eC011_OUTCTRL_MODE;

/* live/playback */
/*  - eCMD_C011_DEC_CHAN_OPEN */
typedef enum
{
    eC011_CHANNEL_PLAYBACK			= 0x00000000,
    eC011_CHANNEL_LIVE_DECODE			= 0x00000001,
    eC011_CHANNEL_TRANSPORT_STREAM_CAPTURE	= 0x00000002,
} eC011_CHANNEL_TYPE;

/* video algorithm */
/*  - eCMD_C011_DEC_CHAN_OPEN */
typedef enum
{
    eC011_VIDEO_ALG_H264	= 0x00000000, // H.264
    eC011_VIDEO_ALG_MPEG2	= 0x00000001, // MPEG-2
    eC011_VIDEO_ALG_H261	= 0x00000002, // H.261
    eC011_VIDEO_ALG_H263	= 0x00000003, // H.263
    eC011_VIDEO_ALG_VC1		= 0x00000004, // VC1
    eC011_VIDEO_ALG_MPEG1	= 0x00000005, // MPEG-1
    eC011_VIDEO_ALG_DIVX	= 0x00000006, // divx
#if 0
    eC011_VIDEO_ALG_MPEG4       = 0x00000006, // MPEG-4
#endif

} eC011_VIDEO_ALG;

/* input source */
/*  - eCMD_C011_DEC_CHAN_OPEN */
typedef enum
{
    eC011_VIDSRC_DEFAULT_PROGRESSIVE	= 0x00000000, // derive from stream
    eC011_VIDSRC_DEFAULT_INTERLACED	= 0x00000001, // derive from stream
    eC011_VIDSRC_FIXED_PROGRESSIVE	= 0x00000002, // progressive frames
    eC011_VIDSRC_FIXED_INTERLACED	= 0x00000003, // interlaced fields

} eC011_VIDEO_SOURCE_MODE;

/* pull-down mode */
/*  - eCMD_C011_DEC_CHAN_OPEN */
typedef enum
{
    eC011_PULLDOWN_DEFAULT_32	= 0x00000000, // derive from PTS inside stream
    eC011_PULLDOWN_DEFAULT_22	= 0x00000001, // derive from PTS inside stream
    eC011_PULLDOWN_DEFAULT_ASAP	= 0x00000002, // derive from PTS inside stream
    eC011_PULLDOWN_FIXED_32	= 0x00000003, // fixed 3-2 pulldown
    eC011_PULLDOWN_FIXED_22	= 0x00000004, // fixed 2-2 pulldown
    eC011_PULLDOWN_FIXED_ASAP	= 0x00000005, // fixed as fast as possible

} eC011_PULLDOWN_MODE;

/* display order */
/*  - eCMD_C011_DEC_CHAN_OPEN */
typedef enum
{
    eC011_DISPLAY_ORDER_DISPLAY	= 0x00000000, // display in display order
    eC011_DISPLAY_ORDER_DECODE	= 0x00000001, // display in decode order

} eC011_DISPLAY_ORDER;

/* picture information mode */
/*  - eCMD_C011_DEC_CHAN_OPEN */
typedef enum
{
    eC011_PICTURE_INFO_OFF	= 0x00000000, // no picture information
    eC011_PICTURE_INFO_ON	= 0x00000001, // pass picture information to host

} eC011_PICTURE_INFO_MODE;

/* picture ready interrupt mode */
/*  - eCMD_C011_DEC_CHAN_OPEN */
typedef enum
{
    eC011_PIC_INT_NONE		= 0x00000000, // no picture ready interrupts
    eC011_PIC_INT_FIRST_PICTURE	= 0x00000001, // interrupt on first picture only
    eC011_PIC_INT_ALL_PICTURES	= 0x00000002, // interrupt on all pictures

} eC011_PIC_INT_MODE;

/* picture setup interrupt mode */
/*  - eCMD_C011_DEC_CHAN_OPEN */
typedef enum
{
    eC011_DISP_INT_NONE		 = 0x00000000, // no picture setup/release interrupts
    eC011_DISP_INT_SETUP	 = 0x00000001, // interrupt on picture setup only
    eC011_DISP_INT_SETUP_RELEASE = 0x00000002, // interrupt on picture setup and release

} eC011_DISP_INT_MODE;

/* deblocking mode */
/*  - eCMD_C011_DEC_CHAN_OPEN */
typedef enum
{
    eC011_DEBLOCKING_OFF	= 0x00000000, // no deblocking
    eC011_DEBLOCKING_ON		= 0x00000001, // deblocking on

} eC011_DEBLOCKING_MODE;

/* BRCM (HD-DVI) mode */
/*  - eCMD_C011_DEC_CHAN_OPEN */
typedef enum
{
    eC011_BRCM_MODE_OFF		= 0x00000000, // Non BRCM (non HD-DVI) mode
    eC011_BRCM_MODE_ON		= 0x00000001, // BRCM (HD-DVI) mode
    eC011_BRCM_ECG_MODE_ON	= 0x00000002, // BRCM (HD-DVI) ECG mode

} eC011_BRCM_MODE;

/* External VCXO control mode */
/*  - eCMD_C011_DEC_CHAN_OPEN */
typedef enum
{
    eC011_EXTERNAL_VCXO_OFF	= 0x00000000, // No external vcxo control
    eC011_EXTERNAL_VCXO_ON	= 0x00000001, // External vcxo control

} eC011_EXTERNAL_VCXO_MODE;

/* Display timing mode */
/*  - eCMD_C011_DEC_CHAN_OPEN */
typedef enum
{
    eC011_DISPLAY_TIMING_USE_PTS	= 0x00000000, // Use PTS for display timing
    eC011_DISPLAY_TIMING_IGNORE_PTS	= 0x00000001, // Ignore PTS and follow pulldown

} eC011_DISPLAY_TIMING_MODE;

/* User data collection mode */
/*  - eCMD_C011_DEC_CHAN_OPEN */
typedef enum
{
    eC011_USER_DATA_MODE_OFF	= 0x00000000, // User data disabled
    eC011_USER_DATA_MODE_ON	= 0x00000001, // User data enabled

} eC011_USER_DATA_MODE;

/*  - eCMD_C011_DEC_CHAN_OPEN */
typedef enum
{
    eC011_PAN_SCAN_MODE_OFF	= 0x00000000, // pan-scan disabled
    eC011_PAN_SCAN_MODE_ON	= 0x00000001, // pan-scan enabled
    eC011_PAN_SCAN_MODE_HOR_ON	= 0x00000002, //Horizontal pan-scan enabled, Vertical pan-scan disabled
    eC011_PAN_SCAN_MODE_HOR_OFF	= 0x00000003, //Horizontal pan-scan disabled, Vertical pan-scan disabled
    eC011_PAN_SCAN_MODE_VER_ON	= 0x00000004, //Horizontal pan-scan disabled, Vertical pan-scan enabled
    eC011_PAN_SCAN_MODE_VER_OFF	= 0x00000005,  //Horizontal pan-scan disabled, Vertical pan-scan disabled

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
   PTS_VALID	    = 0,
   PTS_INTERPOLATED,
   PTS_UNKNOWN,
   PTS_HOST,

} ePtsState;

/* channel status structure */
/*  - eCMD_C011_DEC_CHAN_OPEN response */
typedef struct {
    eC011_DISPLAY_TIMING_MODE displayTimingMode;   // current display timing mode in effect
    int32_t	videoDisplayOffset;  // current video display offset in effect
    uint32_t	currentPts;	  // current PTS value
    uint32_t	interpolatedPts;     // currentPts of type PTS_STATE
    uint32_t	refCounter;
    uint32_t	pcrOffset;
    uint32_t	stcValue;
    uint32_t	stcWritten;	  // 1 -> host updated STC, 0 -> stream ARC ack
    int32_t	ptsStcOffset;	// PTS - STC
    void	*pVdecStatusBlk;     /* pointer to vdec status block */
    uint32_t	lastPicture;	 // 1 -> decoder last picture indication
    uint32_t	pictureTag;	  /* Picture Tag from VDEC */
    uint32_t	tsmLockTime;	     /* Time when the First Picture passed TSM */
    uint32_t	firstPicRcvdTime;    /* Time when the First Picture was recieved */
    uint32_t	picture_done_payload;/* Payload associated with the picture done marker interrupt */

} sC011_CHAN_STATUS;

/* picture information block (PIB) */
/* used in picInfomode, userdataMode */
typedef struct
{
   uint32_t	bFormatChange;
   uint32_t	resolution;
   uint32_t	channelId;
   uint32_t	ppbPtr;
   int32_t	ptsStcOffset;
   uint32_t	zeroPanscanValid;
   uint32_t	dramOutBufAddr;
   uint32_t	yComponent;
   PPB		ppb;

} C011_PIB;

/* size of picture information block */
#define C011_PIB_SIZE	       (sizeof(C011_PIB))

/* picture release mode */
/*  - eCMD_C011_DEC_CHAN_CLOSE */
typedef enum
{
    eC011_PIC_REL_HOST		= 0x00000000, // wait for host to release pics
    eC011_PIC_REL_INTERNAL	= 0x00000001, // do not wait for host

} eC011_PIC_REL_MODE;

/* last picture display mode */
/*  - eCMD_C011_DEC_CHAN_CLOSE */
typedef enum
{
    eC011_LASTPIC_DISPLAY_ON	= 0x00000000, // keep displaying last picture after channelClose
    eC011_LASTPIC_DISPLAY_OFF	= 0x00000001, // blank output after channelClose

} eC011_LASTPIC_DISPLAY;

/* channel flush mode */
/*  - eCMD_C011_DEC_CHAN_FLUSH */
typedef enum
{
    eC011_FLUSH_INPUT_POINT	= 0x00000000, // flush at current input point
    eC011_FLUSH_PROC_POINT	= 0x00000001, // flush at current processing
    eC011_FLUSH_PROC_POINT_RESET_TS = 0x00000002, // flush at current processing, reset TS

} eC011_FLUSH_MODE;

/* direction */
/*  - eCMD_C011_DEC_CHAN_TRICK_PLAY */
typedef enum
{
    eCODEC_DIR_FORWARD		= 0x00000000, // forward
    eCODEC_DIR_REVERSE		= 0x00000001, // reverse

} eC011_DIR;

/* speed */
/*  - eCMD_C011_DEC_CHAN_TRICK_PLAY */
typedef enum
{
    eC011_SPEED_NORMAL		= 0x00000000, // all pictures
    eC011_SPEED_FAST		= 0x00000001, // reference pictures only
    eC011_SPEED_VERYFAST	= 0x00000002, // I-picture only
    eC011_SPEED_SLOW		= 0x00000003, // STC trickplay slow
    eC011_SPEED_PAUSE		= 0x00000004, // STC trickplay pause
    eC011_SPEED_I_ONLY_HOST_MODE = 0x00000100, // I-picture only host mode
    eC011_SPEED_2x_SLOW		= 0xFFFFFFFF, // all pics played 2x frame time
    eC011_SPEED_4x_SLOW		= 0xFFFFFFFE, // all pics played 4x frame time
    eC011_SPEED_8x_SLOW		= 0xFFFFFFFD, // all pics played 8x frame time
    eC011_SPEED_STEP		= 0xFFFFFFFC, // STC trickplay step

} eC011_SPEED;

typedef enum
{
    eC011_DROP_TYPE_DECODER	= 0x00000000,
    eC011_DROP_TYPE_DISPLAY	= 0x00000001,

} eC011_DROP_TYPE;

/* stream input sync mode */
/*  - eCMD_C011_DEC_CHAN_INPUT_PARAMS */
typedef enum
{
    eC011_SYNC_MODE_AUTOMATIC	= 0x00000000, // automatic sync detection
    eC011_SYNC_MODE_SYNCPIN	= 0x00000001, // sync pin mode

} eC011_SYNC_MODE;

/* unmarked discontinuity notification */
/*  - eCMD_C011_DEC_CHAN_INPUT_PARAMS */
typedef enum
{
    eC011_UNMARKED_DISCONTINUITY_OFF	= 0x00000000, // disable unmarked discontinuity
    eC011_UNMARKED_DISCONTINUITY_ON	= 0x00000001, // enable unmarked discontinuity

} eC011_UNMARKED_DISCONTINUITY_MODE;

/* unmarked discontinuity notification trigger threshold */
/*  - eCMD_C011_DEC_CHAN_INPUT_PARAMS */
typedef enum
{
    eC011_UNMARKED_DISCONTINUITY_THRESHOLD_1_PKT	= 0x00000000, // trigger on one packet only
    eC011_UNMARKED_DISCONTINUITY_THRESHOLD_2_PKTS	= 0x00000001, // trigger on 2 consecutive packets only

} eC011_UNMARKED_DISCONTINUITY_THRESHOLD;

/* display resolution */
/*  - eCMD_C011_DEC_CHAN_VIDEO_OUTPUT */
typedef enum
{
    eC011_RESOLUTION_CUSTOM	= 0x00000000, // custom
    eC011_RESOLUTION_480i	= 0x00000001, // 480i
    eC011_RESOLUTION_1080i	= 0x00000002, // 1080i (1920x1080, 60i)
    eC011_RESOLUTION_NTSC	= 0x00000003, // NTSC (720x483, 60i)
    eC011_RESOLUTION_480p	= 0x00000004, // 480p (720x480, 60p)
    eC011_RESOLUTION_720p	= 0x00000005, // 720p (1280x720, 60p)
    eC011_RESOLUTION_PAL1	= 0x00000006, // PAL_1 (720x576, 50i)
    eC011_RESOLUTION_1080i25	= 0x00000007, // 1080i25 (1920x1080, 50i)
    eC011_RESOLUTION_720p50	= 0x00000008, // 720p50 (1280x720, 50p)
    eC011_RESOLUTION_576p	= 0x00000009, // 576p (720x576, 50p)
    eC011_RESOLUTION_1080i29_97	= 0x0000000A, // 1080i (1920x1080, 59.94i)
    eC011_RESOLUTION_720p59_94	= 0x0000000B, // 720p (1280x720, 59.94p)
    eC011_RESOLUTION_SD_DVD	= 0x0000000C, // SD DVD (720x483, 60i)
    eC011_RESOLUTION_480p656	= 0x0000000D, // 480p (720x480, 60p), output bus width 8 bit, clock 74.25MHz.
    eC011_RESOLUTION_1080p23_976 = 0x0000000E, // 1080p23_976 (1920x1080, 23.976p)
    eC011_RESOLUTION_720p23_976	= 0x0000000F, // 720p23_976 (1280x720p, 23.976p)
    eC011_RESOLUTION_240p29_97	= 0x00000010, // 240p (1440x240, 29.97p )
    eC011_RESOLUTION_240p30	= 0x00000011, // 240p (1440x240, 30p)
    eC011_RESOLUTION_288p25	= 0x00000012, // 288p (1440x288p, 25p)
    eC011_RESOLUTION_1080p29_97	= 0x00000013, // 1080p29_97 (1920x1080, 29.97p)
    eC011_RESOLUTION_1080p30	= 0x00000014, // 1080p30 (1920x1080, 30p)
    eC011_RESOLUTION_1080p24	= 0x00000015, // 1080p24 (1920x1080, 24p)
    eC011_RESOLUTION_1080p25	= 0x00000016, // 1080p25 (1920x1080, 25p)
    eC011_RESOLUTION_720p24	= 0x00000017, // 720p24 (1280x720, 25p)
    eC011_RESOLUTION_720p29_97	= 0x00000018, // 720p29_97 (1280x720, 29.97p)

} eC011_RESOLUTION;

/* output scanning mode */
/*  - eCMD_C011_DEC_CHAN_VIDEO_OUTPUT */
typedef enum
{
    eC011_SCAN_MODE_PROGRESSIVE	= 0x00000000, // progressive frames
    eC011_SCAN_MODE_INTERLACED	= 0x00000001, // interlaced fields
} eC011_SCAN_MODE;

/* display option */
/*  - eCMD_C011_DEC_CHAN_VIDEO_OUTPUT */
typedef enum
{
    eC011_DISPLAY_LETTERBOX	= 0x00000000, // letter box
    eC011_DISPLAY_FULLSCREEN	= 0x00000001, // full screen
    eC011_DISPLAY_PILLARBOX	= 0x00000002, // pillar box
} eC011_DISPLAY_OPTION;

/* display formatting */
/*  - eCMD_C011_DEC_CHAN_VIDEO_OUTPUT */
typedef enum
{
    eC011_FORMATTING_AUTO	= 0x00000000, // automatic
    eC011_FORMATTING_CUSTOM	= 0x00000001, // custom
    eC011_FORMATTING_NONE	= 0x00000002, // no formatting
    eC011_FORMATTING_PICTURE	= 0x00000003, // picture level
} eC011_FORMATTING;

/* vsync mode */
/*  - eCMD_C011_DEC_CHAN_VIDEO_OUTPUT */
typedef enum
{
    eC011_VSYNC_MODE_NORMAL	= 0x00000000, // internal video timing
    eC011_VSYNC_MODE_EXTERNAL	= 0x00000001, // use external vsync_in signal
    eC011_VSYNC_MODE_BYPASS	= 0x00000002, // 7411 updates STC from PCR in stream, but external vsync
    eC011_VSYNC_MODE_INTERNAL	= 0x00000003, // User updates STC, but internal vsync

} eC011_VSYNC_MODE;

/* output clipping mode */
/*  - eCMD_C011_DEC_CHAN_VIDEO_OUTPUT */
typedef enum
{
    eC011_OUTPUT_CLIPPING_BT601	 = 0x00000000, // Luma pixel is clipped to [16,235]. Chroma pixel is clipped to [16,240]
    eC011_OUTPUT_CLIPPING_BT1120 = 0x00000001, // The pixel is clipped to [1,254]
    eC011_OUTPUT_CLIPPING_NONE	 = 0x00000002, // No output clipping

} eC011_OUTPUT_CLIPPING;

/* display mode for pause/slow/fastforward*/
/*  - eCMD_C011_DEC_CHAN_VIDEO_OUTPUT */
typedef enum
{
    eC011_DISPLAY_MODE_AUTO	= 0x00000000,
    eC011_DISPLAY_MODE_FRAME	= 0x00000001,
    eC011_DISPLAY_MODE_TOP	= 0x00000002,
    eC011_DISPLAY_MODE_BOTTOM	= 0x00000003,

} eC011_DISPLAY_MODE;

/*order in timeline where the pauseUntoPts and displayUntoPts occur*/
typedef enum
{
    eC011_PAUSE_UNTO_PTS_ONLY				= 0x00000001,
    eC011_DISPLAY_UNTO_PTS_ONLY				= 0x00000002,
    eC011_DISPLAY_UNTO_PTS_LESSER_THAN_PAUSE_UNTO_PTS	= 0x00000003,
    eC011_DISPLAY_UNTO_PTS_GREATER_THAN_PAUSE_UNTO_PTS	= 0x00000004,
}
eC011_DISPLAY_PAUSE_STATE;

/* scaling on/off */
/*  - eCMD_C011_DEC_CHAN_OUTPUT_FORMAT */
typedef enum
{
    eC011_SCALING_OFF		= 0x00000000,
    eC011_SCALING_ON		= 0x00000001,

} eC011_SCALING;

/* edge control */
/*  - eCMD_C011_DEC_CHAN_OUTPUT_FORMAT */
typedef enum
{
    eC011_EDGE_CONTROL_NONE	= 0x00000000, // no cropping or padding
    eC011_EDGE_CONTROL_CROP	= 0x00000001, // cropping
    eC011_EDGE_CONTROL_PAD	= 0x00000002, // padding

} eC011_EDGE_CONTROL;

/* deinterlacing on/off */
/*  - eCMD_C011_DEC_CHAN_OUTPUT_FORMAT */
typedef enum
{
    eC011_DEINTERLACING_OFF	= 0x00000000,
    eC011_DEINTERLACING_ON	= 0x00000001,

} eC011_DEINTERLACING;

/* scaling target */
/*  - eCMD_C011_DEC_CHAN_SCALING_FILTERS */
typedef enum
{
    eC011_HORIZONTAL		= 0x00000000,
    eC011_VERTICAL_FRAME	= 0x00000001,
    eC011_VERTICAL_FIELD_TOP	= 0x00000002,
    eC011_VERTICAL_FIELD_BOTTOM	= 0x00000003,

} eC011_SCALING_TARGET;

/* normalization */
/*  - eCMD_C011_DEC_CHAN_SCALING_FILTERS */
typedef enum
{
    eC011_NORMALIZATION_128	= 0x00000000, // divide by 128
    eC011_NORMALIZATION_64	= 0x00000001, // divide by 64

} eC011_NORMALIZATION;

/* pause type */
/*  - eCMD_C011_DEC_CHAN_PAUSE_OUTPUT */
typedef enum
{
    eC011_PAUSE_TYPE_RESUME	= 0x00000000, // resume video output
    eC011_PAUSE_TYPE_CURRENT	= 0x00000001, // pause video on current frame
    eC011_PAUSE_TYPE_BLACK	= 0x00000002, // pause video with black screen
    eC011_PAUSE_TYPE_STEP	= 0x00000003, // display next picture

} eC011_PAUSE_TYPE;

/* TSD audio payload type */
/*  - eCMD_C011_DEC_CREATE_AUDIO_CONTEXT */
typedef enum
{
    eC011_TSD_AUDIO_PAYLOAD_MPEG1	= 0x00000000,
    eC011_TSD_AUDIO_PAYLOAD_AC3		= 0x00000001,

} eC011_TSD_AUDIO_PAYLOAD_TYPE;

/* CDB extract bytes for PES */
/*  - eCMD_C011_DEC_CREATE_AUDIO_CONTEXT */
typedef enum
{
    eC011_PES_CDB_EXTRACT_0_BYTES	= 0x00000000,
    eC011_PES_CDB_EXTRACT_1_BYTE	= 0x00000001,
    eC011_PES_CDB_EXTRACT_4_BYTES	= 0x00000002,
    eC011_PES_CDB_EXTRACT_7_BYTES	= 0x00000003,

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
    eC011_DESCRAMBLING_3DES	= 0x00000000,
    eC011_DESCRAMBLING_DES	= 0x00000001,

} eC011_DESCRAMBLING_MODE;

/* key exchange */
/*  - eCMD_C011_DEC_CHAN_SET_DECYPTION */
typedef enum
{
    eC011_KEY_EXCHANGE_EVEN_0	= 0x00000001,
    eC011_KEY_EXCHANGE_EVEN_1	= 0x00000002,
    eC011_KEY_EXCHANGE_EVEN_2	= 0x00000004,
    eC011_KEY_EXCHANGE_ODD_0	= 0x00000010,
    eC011_KEY_EXCHANGE_ODD_1	= 0x00000020,
    eC011_KEY_EXCHANGE_ODD_2	= 0x00000040,

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
    eC011_PAUSE_MODE_OFF	= 0x00000000,
    eC011_PAUSE_MODE_ON		= 0x00000001,

} eC011_PAUSE_MODE;

/* skip pic mode */
/*  - eCMD_C011_DEC_CHAN_SET_SKIP_PIC_MODE */
typedef enum
{
    eC011_SKIP_PIC_IPB_DECODE	= 0x00000000,
    eC011_SKIP_PIC_IP_DECODE	= 0x00000001,
    eC011_SKIP_PIC_I_DECODE	= 0x00000002,

} eC011_SKIP_PIC_MODE;

/* enum for color space conversion */
typedef enum
{
   eC011_DEC_CSC_CTLBYDEC	= 0x00000000,
   eC011_DEC_CSC_ENABLE		= 0x00000001,
   eC011_DEC_CSC_DISABLE	= 0x00000002,
} eC011_DEC_CSC_SETUP;

/* enum for setting range remap */
typedef enum
{
   eC011_DEC_RANGE_REMAP_VIDCTL	 = 0x00000000,
   eC011_DEC_RANGE_REMAP_ENABLE	 = 0x00000001,
   eC011_DEC_RANGE_REMAP_DISABLE = 0x00000002,
} eC011_DEC_RANGE_REMAP_SETUP;

typedef enum
{
   eC011_DEC_OPERATION_MODE_GENERIC	= 0x00000000,
   eC011_DEC_OPERATION_MODE_BLURAY	= 0x00000001,
   eC011_DEC_OPERATION_MODE_HDDVD	= 0x00000002,
} eC011_DEC_OPERATION_MODE;

typedef enum
{
   eC011_DEC_RANGE_REMAP_ADVANCED	= 0x00000000,
   eC011_DEC_RANGE_REMAP_MAIN		= 0x00000001,
} eC011_DEC_RANGE_REMAP_VC1PROFILE;

/* encoder sequence paramaters */
/*  - eCMD_C011_ENC_CHAN_OPEN */
/*  - eCMD_C011_ENC_CHAN_CONTROL */
typedef struct
{
  uint32_t	seqParamId;     // sequence parameter set ID
  uint32_t	profile;	// profile (E_ENC_PROFILE)
  uint32_t	level;		// level x 10
  uint32_t	constraintMap;  // bitmap for constraint sets
  uint32_t	frameNumMaxLog; // logorithm of frame number max
  uint32_t	pocLsbMaxLog;   // logorithm of POC LSB max
  uint32_t	pocType;	// POC type
  uint32_t	refFrameMax;    // number of reference frames
  uint32_t	frameMBHeight;  // frame height in MB
  uint32_t	frameMBWidth;   // frame width in MB
  uint32_t	frameMBOnly;    // frame MB only flag
  uint32_t	adaptMB;	// MBAFF flag
  uint32_t	dir8x8Infer;    // direct 8x8 inference flag

} sC011_ENC_SEQ_PARAM;

/* encoder picture parameters */
/*  - eCMD_C011_ENC_CHAN_OPEN */
/*  - eCMD_C011_ENC_CHAN_CONTROL */
typedef struct
{
  uint32_t	picParamId;     // picture parameter set ID
  uint32_t	seqParameterId; // sequence parameter set ID
  uint32_t	refIdxMaxL0;    // number of active reference indices
  uint32_t	refIdxMaxL1;    // number of active reference indices
  uint32_t	entMode;	// entropy mode (E_ENC_ENT_MODE)
  uint32_t	initQP;	 // picture init QP
  uint32_t	intraConst;     // constrained intra prediction flag

} sC011_ENC_PIC_PARAM;

/* encoder coding parameters */
/*  - eCMD_C011_ENC_CHAN_OPEN */
/*  - eCMD_C011_ENC_CHAN_CONTROL */
typedef struct
{
  uint32_t	eventNfy;       // event notify generation flag
  uint32_t	intMode;	// interlace coding mode (E_ENC_INT_MODE)
  uint32_t	gopSize;	// number of frames per GOP (I frame period)
  uint32_t	gopStruct;      // number of B-frames between references
  uint32_t	rateCtrlMode;   // rate control mode (E_ENC_RATE_CTRL_MODE)
  uint32_t	frameRate;      // frame rate
  uint32_t	constQP;	// constant QP
  uint32_t	bitratePeriod;  // VBR bitrate average period (in GOPs)
  uint32_t	bitRateAvg;     // VBR average bitrate
  uint32_t	bitRateMax;     // VBR max bitrate
  uint32_t	bitRateMin;     // VBR min bitrate

} sC011_ENC_CODING_PARAM;

/* encoder video-in parameters */
/*  - eCMD_C011_ENC_CHAN_OPEN */
/*  - eCMD_C011_ENC_CHAN_CONTROL */
typedef struct
{
  uint32_t	picInfoMode;    // picture info mode (E_ENC_PIC_INFO_MODE)
  uint32_t	picIdSrc;       // picture ID source (E_ENC_PIC_ID_SRC)
  uint32_t	picYUVFormat;   // picture YUV format (E_ENC_PIC_YUV_FORMAT)
  uint32_t	picIntFormat;   // picture interlace format (E_ENC_PIC_INT_FORMAT)

} sC011_ENC_VID_IN_PARAM;

/* encoder code-out parameters */
/*  - eCMD_C011_ENC_CHAN_OPEN */
/*  - eCMD_C011_ENC_CHAN_CONTROL */
typedef struct
{
  uint32_t	portId;		// port ID
  uint32_t	codeFormat;     // code format (E_ENC_CODE_FORMAT)
  uint32_t	delimiter;      // delimiter NAL flag
  uint32_t	endOfSeq;       // end of sequence NAL flag
  uint32_t	endOfStream;    // end of stream NAL flag
  uint32_t	picParamPerPic; // picture parameter set per picture flag
  uint32_t	seiMasks;       // SEI message (1 << E_ENC_SEI_TYPE)

} sC011_ENC_CODE_OUT_PARAM;

/* encoder picture data */
/*  - eCMD_C011_ENC_CHAN_CONTROL */
typedef struct
{
  uint32_t	picId;		// picture ID
  uint32_t	picStruct;      // picture structure (E_ENC_PIC_STRUCT)
  uint32_t	origBuffIdx;    // original frame buffer index
  uint32_t	reconBuffIdx;   // reconstructed frame buffer index

} sC011_ENC_PIC_DATA;

/* encoder channel control parameter type */
/*  - eCMD_C011_ENC_CHAN_CONTROL */
typedef enum {
    eC011_ENC_CTRL_SEQ_PARAM	= 0x00000000,
    eC011_ENC_CTRL_PIC_PARAM	= 0x00000001,
    eC011_ENC_CTRL_CODING_PARAM	= 0x00000002,
    eC011_ENC_CTRL_PIC_DATA	= 0x00000003,

} eC011_ENC_CTRL_CODE;

/* encoder channel control parameter */
/*  - eCMD_C011_ENC_CHAN_CONTROL */
typedef union EncCtrlParam {
    sC011_ENC_SEQ_PARAM		seqParams;
    sC011_ENC_PIC_PARAM		picParams;
    sC011_ENC_CODING_PARAM	codingParams;
    sC011_ENC_PIC_DATA		picData;

} uC011_ENC_CTRL_PARAM;

/*
 * Data Structures for the API commands above
 */

#define DMA_CIQ_DEPTH 64

/* dsDmaCtrlInfo */
typedef struct {
    uint32_t	dmaSrcAddr;    /* word 0: src addr */
    uint32_t	dmaDstAddr;    /* word 1: dst addr */
    uint32_t	dmaXferCtrol;  /* word 2:
				*   bit 31-30. reserved
				*   bit 29. Stream number
				*   bit 28-27. Interrupt
				*	x0: no interrupt;
				*	01: interrupt when dma done wo err
				*	11: interrupt only if there is err
				*   bit 26. Endidan. 0: big endian; 1: little endian
				*   bit 25. dst inc. 0: addr doesn't change; 1: addr+=4
				*   bit 24. src inc. 0: addr doesn't change; 1: addr+=4
				*   bit 23-0. number of bytes to be transfered
				*/
} dsDmaCtrlInfo;

/* dsDmaCtrlInfoQueue */
typedef struct {
    uint32_t		readIndex;
    uint32_t		writeIndex;
    dsDmaCtrlInfo	dmaCtrlInfo[DMA_CIQ_DEPTH];
} dsDmaCtrlInfoQueue;

/* Init */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	memSizeMBytes;
    uint32_t	inputClkFreq;
    uint32_t	uartBaudRate;
    uint32_t	initArcs;
    eC011_INT_CONTROL interrupt;
    uint32_t	audioMemSize;
    eC011_BRCM_MODE brcmMode;
    uint32_t	fgtEnable;		/* 0 - disable FGT, 1 - enable FGT */
    uint32_t	DramLogEnable;		/* 0 - disable DramLog, 1 - enable DramLog */
    uint32_t	sidMemorySize;		/* in bytes */
    uint32_t	dmaDataXferEnable;	/* 0:disable; 1:enable */
    uint32_t	rsaDecrypt;		/* 0:disable; 1:enable */
    uint32_t	openMode;
    uint32_t	rsvd1;
    uint32_t	rsvd2;
    uint32_t	rsvd3;

} C011CmdInit;

typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	status;
    uint32_t	commandBuffer;
    uint32_t	responseBuffer;
    uint32_t	blockPool;
    uint32_t	blockSize; /* in words */
    uint32_t	blockCount;
    uint32_t	audioMemBase;
    uint32_t	watchMemAddr;
    uint32_t	streamDramLogBase;
    uint32_t	streamDramLogSize;
    uint32_t	vdecOuterDramLogBase;
    uint32_t	vdecOuterDramLogSize;
    uint32_t	vdecInnerDramLogBase;
    uint32_t	vdecInnerDramLogSize;
    uint32_t	sidMemoryBaseAddr;
    uint32_t	inputDmaCiqAddr;
    uint32_t	inputDmaCiqReleaseAddr;
    uint32_t	outputDmaCiqAddr;
    uint32_t	outputDmaCiqReleaseAddr;
    uint32_t	dramX509CertAddr;
    uint32_t	rsvdAddr;
} C011RspInit;

/* Reset */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
} C011CmdReset;

/* SelfTest */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    eC011_TEST_ID testId;
} C011CmdSelfTest;

typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	status;
    uint32_t	errorCode;
} C011RspSelfTest;

/* GetVersion */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
} C011CmdGetVersion;

typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	status;
    uint32_t	streamSwVersion;
    uint32_t	decoderSwVersion;
    uint32_t	chipHwVersion;
    uint32_t	reserved1;
    uint32_t	reserved2;
    uint32_t	reserved3;
    uint32_t	reserved4;
    uint32_t	blockSizePIB;
    uint32_t	blockSizeChannelStatus;
    uint32_t	blockSizePPB;
    uint32_t	blockSizePPBprotocolMpeg;
    uint32_t	blockSizePPBprotocolH264;
    uint32_t	blockSizePPBprotocolRsvd;
} C011RspGetVersion;

/* GPIO */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    eC011_GPIO_CONTROL	gpioControl;
} C011CmdGPIO;

/* DebugSetup */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	paramMask;
    uint32_t	debugARCs;
    uint32_t	debugARCmode;
    uint32_t	outPort;
    uint32_t	clock;
    uint32_t	channelId;
    uint32_t	enableRVCcapture;
    uint32_t	playbackMode;
    uint32_t	enableCRCinterrupt;
    uint32_t	esStartDelay10us;
} C011CmdDebugSetup;

typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	status;
    uint32_t	DQ;
    uint32_t	DRQ;
} C011RspDebugSetup;

/* DecChannelOpen */
typedef struct {
   uint32_t	command;
   uint32_t	sequence;
   eC011_INPUT_PORT	inPort;
   eC011_OUTPUT_PORT	outVidPort;
   eC011_STREAM_TYPE	streamType;
   eC011_MAX_PICSIZE	maxPicSize;
   eC011_OUTCTRL_MODE	outCtrlMode;
   eC011_CHANNEL_TYPE	chanType;
   uint32_t	reservedWord8;
   eC011_VIDEO_ALG	videoAlg;
   eC011_VIDEO_SOURCE_MODE   sourceMode;
   eC011_PULLDOWN_MODE  pulldown;
   eC011_PICTURE_INFO_MODE   picInfo;
   eC011_DISPLAY_ORDER       displayOrder;
   uint32_t	reservedWord14;
   uint32_t	reservedWord15;
   uint32_t	streamId; /* for multi-stream */
   eC011_DEBLOCKING_MODE     deblocking;
   eC011_EXTERNAL_VCXO_MODE  vcxoControl;
   eC011_DISPLAY_TIMING_MODE displayTiming;
   int32_t	videoDisplayOffset;
   eC011_USER_DATA_MODE      userDataMode;
   uint32_t	enableUserDataInterrupt;
   uint32_t	ptsStcDiffThreshold;
   uint32_t	stcPtsDiffThreshold;
   uint32_t	enableFirstPtsInterrupt;
   uint32_t	enableStcPtsThresholdInterrupt;
   uint32_t	frameRateDefinition;
   uint32_t	hostDmaInterruptEnable;
   uint32_t	asynchEventNotifyEnable;
   uint32_t	enablePtsStcChangeInterrupt;
   uint32_t	enablePtsErrorInterrupt;
   uint32_t	enableFgt;
   uint32_t	enable23_297FrameRateOutput;
   uint32_t	enableVideoDataUnderflowInterrupt;
   uint32_t	reservedWord35;
   uint32_t	pictureInfoInterruptEnable;
} DecCmdChannelOpen;

typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	status;
    uint32_t	channelId;
    uint32_t	picBuf;
    uint32_t	picRelBuf;
    uint32_t	picInfoDeliveryQ;
    uint32_t	picInfoReleaseQ;
    uint32_t	channelStatus;
    uint32_t	userDataDeliveryQ;
    uint32_t	userDataReleaseQ;
    uint32_t	transportStreamCaptureAddr;
    uint32_t	asyncEventQ;
} DecRspChannelOpen;

/* DecChannelClose */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	channelId;
    eC011_PIC_REL_MODE	pictureRelease;
    eC011_LASTPIC_DISPLAY     lastPicDisplay;
} DecCmdChannelClose;

/* DecChannelActivate */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	channelId;
    uint32_t	dbgMode;
} DecCmdChannelActivate;

/* DecChannelStatus */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	channelId;
} DecCmdChannelStatus;

typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	status;
    uint32_t	channelStatus;
    uint32_t	cpbSize;       /* CPB size */
    uint32_t	cpbFullness;   /* CPB fullness */
    uint32_t	binSize;       /* BIN buffer size */
    uint32_t	binFullness;   /* BIN buffer fullness */
    uint32_t	bytesDecoded;  /* Bytes decoded */
    uint32_t	nDelayed;      /* pics with delayed delivery */
} DecRspChannelStatus;

/* DecChannelFlush */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	channelId;
    eC011_FLUSH_MODE	  flushMode;
} DecCmdChannelFlush;

/* DecChannelTrickPlay */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	channelId;
    eC011_DIR		 direction;
    eC011_SPEED	       speed;
} DecCmdChannelTrickPlay;

/* DecChannelSetTSPIDs */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	channelId;
    uint32_t	pcrPid;
    uint32_t	videoPid;
    uint32_t	videoSubStreamId;
} DecCmdChannelSetTSPIDs;

/* DecChannelSetPcrPID */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	channelId;
    uint32_t	pcrPid;
} DecCmdChannelSetPcrPID;

/* DecChannelSetVideoPID */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	channelId;
    uint32_t	videoPid;
    uint32_t	videoSubStreamId;
} DecCmdChannelSetVideoPID;

/* DecChannelSetPSStreamIDs */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	channelId;
    uint32_t	videoStreamId;
    uint32_t	videoStreamIdExtEnable;
    uint32_t	videoStreamIdExt;
} DecCmdChannelSetPSStreamIDs;

/* DecChannelSetInputParams */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	channelId;
    eC011_SYNC_MODE	   syncMode;
    eC011_UNMARKED_DISCONTINUITY_MODE	discontinuityNotify;
    eC011_UNMARKED_DISCONTINUITY_THRESHOLD   discontinuityPktThreshold;
    uint32_t	discontinuityThreshold;
    uint32_t	disableFlowControl;
    uint32_t	disablePCROffset;
} DecCmdChannelSetInputParams;

/* DecChannelSetVideoOutput */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    eC011_OUTPUT_PORT	 portId;
    eC011_RESOLUTION	  resolution;
    uint32_t	width;
    uint32_t	height;
    eC011_SCAN_MODE	   scanMode;
    uint32_t	picRate;
    eC011_DISPLAY_OPTION      option;
    eC011_FORMATTING	  formatMode;
    eC011_VSYNC_MODE	  vsyncMode;
    uint32_t	numOsdBufs;
    uint32_t	numCcDataBufs;
    uint32_t	memOut;
    eC011_OUTPUT_CLIPPING     outputClipping;
    uint32_t	invertHddviSync;
    eC011_DISPLAY_MODE	pauseMode;
    eC011_DISPLAY_MODE	slowMode;
    eC011_DISPLAY_MODE	ffMode;
    uint32_t	vppPaddingValue; /* bits: 23-16 (Y), 15-8 (U), 7-0 (V) */
    uint32_t	extVideoClock;
    uint32_t	hddviEnable;
    uint32_t	numDramOutBufs;
} DecCmdChannelSetVideoOutput;

typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	status;
    uint32_t	osdBuf1;
    uint32_t	osdBuf2;
    uint32_t	ccDataBuf1;
    uint32_t	ccDataBuf2;
    uint32_t	memOutBuf;
} DecRspChannelSetVideoOutput;

/* DecChannelSetCustomVidOut */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    eC011_OUTPUT_PORT	 portId;
    uint32_t	spl;
    uint32_t	spal;
    uint32_t	e2e;
    uint32_t	lpf;
    uint32_t	vlpf;
    uint32_t	vbsf1;
    uint32_t	vbff1;
    uint32_t	vbsf2;
    uint32_t	vbff2;
    uint32_t	f1id;
    uint32_t	f2id;
    uint32_t	gdband;
    uint32_t	vsdf0;
    uint32_t	vsdf1;
    uint32_t	hsyncst;
    uint32_t	hsyncsz;
    uint32_t	vsstf1;
    uint32_t	vsszf1;
    uint32_t	vsstf2;
    uint32_t	vsszf2;
    uint32_t	bkf1;
    uint32_t	bkf2;
    uint32_t	invertsync;
    uint32_t	wordmode;
    uint32_t	hdclock;
} DecCmdChannelSetCustomVidOut;

/* DecChannelSetOutputFormatting */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    eC011_OUTPUT_PORT	 portId;
    eC011_SCALING	     horizontalScaling;
    eC011_SCALING	     verticalScaling;
    uint32_t	horizontalPhases;
    uint32_t	verticalPhases;
    eC011_EDGE_CONTROL	horizontalEdgeControl;
    eC011_EDGE_CONTROL	verticalEdgeControl;
    uint32_t	leftSize;
    uint32_t	rightSize;
    uint32_t	topSize;
    uint32_t	bottomSize;
    uint32_t	horizontalSize;
    uint32_t	verticalSize;
    int32_t	horizontalOrigin;
    int32_t	verticalOrigin;
    uint32_t	horizontalCropSize;
    uint32_t	verticalCropSize;
    uint32_t	lumaTopFieldOffset;
    uint32_t	lumaBottomFieldOffset;
    uint32_t	chromaTopFieldOffset;
    uint32_t	chromaBottomFieldOffset;
    eC011_SCAN_MODE	   inputScanMode;
    eC011_DEINTERLACING       deinterlacing;
    uint32_t	horizontalDecimation_N;
    uint32_t	horizontalDecimation_M;
    uint32_t	verticalDecimation_N;
    uint32_t	verticalDecimation_M;
    uint32_t	horizontalDecimationVector [4];
    uint32_t	verticalDecimationVector [4];
} DecCmdChannelSetOutputFormatting;

/* DecCmdChannelSetPictureOutputFormatting */
typedef struct {
   uint32_t	command;
   uint32_t	sequence;
   eC011_OUTPUT_PORT	 portId;
   eC011_SCALING	     horizontalScaling;
   eC011_SCALING	     verticalScaling;
   uint32_t	horizontalPhases;
   uint32_t	verticalPhases;
   eC011_EDGE_CONTROL	horizontalEdgeControl;
   eC011_EDGE_CONTROL	verticalEdgeControl;
   uint32_t	leftSize;
   uint32_t	rightSize;   /* 10 */
   uint32_t	topSize;
   uint32_t	bottomSize;
   uint32_t	horizontalSize;
   uint32_t	verticalSize;
   int32_t	horizontalOrigin;
   int32_t	verticalOrigin;
   uint32_t	horizontalCropSize;
   uint32_t	verticalCropSize;
   uint32_t	lumaTopFieldOffset;
   uint32_t	lumaBottomFieldOffset; /* 20 */
   uint32_t	chromaTopFieldOffset;
   uint32_t	chromaBottomFieldOffset;
   eC011_SCAN_MODE	   inputScanMode;
   eC011_DEINTERLACING       deinterlacing;
   uint32_t	horizontalDecimationFactor; /* bits: 0-7 N; 8-15 M */
   uint32_t	verticalDecimationFactor;   /* bits: 0-7 Np; 8-15 Mp; 16-23 Ni; 24-31 Mi*/
   uint32_t	horizontalDecimationOutputSize;
   uint32_t	verticalDecimationOutputSize;
   uint32_t	horizontalScalingFactor;    /* bits: 0-7 N; 8-15 M */
   uint32_t	verticalScalingFactorProgressive; /* bits: 0-7 Nt; 8-15 Mt; 16-23 Nb; 24-31 Mb*/
   uint32_t	verticalScalingFactorInterlace;   /* bits: 0-7 Nt; 8-15 Mt; 16-23 Nb; 24-31 Mb*/
   uint32_t	Reserved[5];
} DecCmdChannelSetPictureOutputFormatting;

/* DecChannelSetScalingFilters */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    eC011_OUTPUT_PORT	 portId;
    eC011_SCALING_TARGET      target;
    uint32_t	pixelPos;
    uint32_t	increment;
    int32_t	lumaCoeff1;
    int32_t	lumaCoeff2;
    int32_t	lumaCoeff3;
    int32_t	lumaCoeff4;
    int32_t	lumaCoeff5;
    eC011_NORMALIZATION       lumaNormalization;
    int32_t	chromaCoeff1;
    int32_t	chromaCoeff2;
    int32_t	chromaCoeff3;
    eC011_NORMALIZATION       chromaNormalization;
} DecCmdChannelSetScalingFilters;

/* DecChannelOsdMode */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    eC011_OUTPUT_PORT	 portId;
    uint32_t	osdBuffer;
    uint32_t	fullRes;
} DecCmdChannelOsdMode;

/* DecChannelCcMode */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    eC011_OUTPUT_PORT	 portId;
    uint32_t	ccBuffer;
} DecCmdChannelCcDataMode;

/* DecChannelDrop */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	channelId;
    uint32_t	numPicDrop;
   eC011_DROP_TYPE	    dropType;
} DecCmdChannelDrop;

/* DecChannelRelease */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	channelId;
    uint32_t	picBuffer;
} DecCmdChannelRelease;

/* DecChannelStreamSettings */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	channelId;
    uint32_t	pcrDelay;
} DecCmdChannelStreamSettings;

/* DecChannelPauseVideoOutput */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    eC011_OUTPUT_PORT	 portId;
    eC011_PAUSE_TYPE	 action;
} DecCmdChannelPauseVideoOutput;

/* DecChannelChange */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	channelId;
    uint32_t	pcrPid;
    uint32_t	videoPid;
    uint32_t	audio1Pid;
    uint32_t	audio2Pid;
    uint32_t	audio1StreamId;
    uint32_t	audio2StreamId;
} DecCmdChannelChange;

typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	status;
    uint32_t	channelId;
    uint32_t	picBuf;
    uint32_t	picRelBuf;
} DecRspChannelChange;

/* DecChannelSetSTC */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	channelId;
    uint32_t	stcValue0;
    uint32_t	stcValue1;
} DecCmdChannelSetSTC;

/* DecChannelSetPTS */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	channelId;
    uint32_t	ptsValue0;
    uint32_t	ptsValue1;
} DecCmdChannelSetPTS;

/* DecCreateAudioContext */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	contextId;
    uint32_t	inPort;
    uint32_t	streamId;
    uint32_t	subStreamId;
    uC011_AUDIO_PAYLOAD_INFO  payloadInfo;
    uint32_t	cdbBaseAddress;
    uint32_t	cdbEndAddress;
    uint32_t	     itbBaseAddress;
    uint32_t	itbEndAddress;
    uint32_t	streamIdExtension;
} DecCmdCreateAudioContext;

/* DecCopyAudioContext */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	cdbBaseAddress;
    uint32_t	cdbEndAddress;
    uint32_t	itbBaseAddress;
    uint32_t	itbEndAddress;
} DecCmdCopyAudioContext;

/* DecDeleteAudioContext */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	contextId;
} DecCmdDeleteAudioContext;

/* DecSetDecryption */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	channelId;
    eC011_DESCRAMBLING_MODE   descramMode;
    uint32_t	evenKey0;
    uint32_t	evenKey1;
    uint32_t	evenKey2;
    uint32_t	evenKey3;
    uint32_t	oddKey0;
    uint32_t	oddKey1;
    uint32_t	oddKey2;
    uint32_t	oddKey3;
    eC011_KEY_EXCHANGE_MODE   keyExchangeMode;
    eC011_CT_STEALING_MODE    cipherTextStealingMode;
} DecCmdSetDecryption;

/* DecChanPicCapture */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	channelId;
} DecCmdChanPicCapture;

typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	status;
    uint32_t	pibAddress;
} DecRspChanPicCapture;

/* DecChanPause */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	channelId;
    eC011_PAUSE_MODE	  enableState;
} DecCmdChannelPause;

/* DecChanPauseState */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	channelId;
} DecCmdChannelPauseState;

typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	status;
    eC011_PAUSE_MODE	  pauseState;
} DecRspChannelPauseState;

/* DecChanSetSlowMotionRate */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	channelId;
    uint32_t	rate;	// 1 -> 1x (normal speed), 2 -> 2x slower, etc
} DecCmdChannelSetSlowMotionRate;

/* DecChanGetSlowMotionRate */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	channelId;
} DecCmdChannelGetSlowMotionRate;

typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	status;
    uint32_t	rate;	// 1 -> 1x (normal speed), 2 -> 2x slower, etc

} DecRspChannelGetSlowMotionRate;

/* DecChanSetFFRate */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	channelId;
    uint32_t	rate;	// 1 -> 1x (normal speed), 2 -> 2x faster, etc

} DecCmdChannelSetFFRate;

/* DecChanGetFFRate */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	channelId;
} DecCmdChannelGetFFRate;

typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	status;
    uint32_t	rate;	// 1 -> 1x (normal speed), 2 -> 2x faster, etc
} DecRspChannelGetFFRate;

/* DecChanFrameAdvance */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	channelId;
} DecCmdChannelFrameAdvance;

/* DecChanSetSkipPictureMode */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	channelId;
    eC011_SKIP_PIC_MODE       skipMode;
} DecCmdChannelSetSkipPictureMode;

/* DecChanGetSkipPictureMode */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	channelId;
} DecCmdChannelGetSkipPictureMode;

typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	status;
    eC011_SKIP_PIC_MODE       skipMode;
} DecRspChannelGetSkipPictureMode;

/* DecChanFillPictureBuffer */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	channelId;
    uint32_t	yuvValue;
} DecCmdChannelFillPictureBuffer;

/* DecChanSetContinuityCheck */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	channelId;
    uint32_t	enable;
} DecCmdChannelSetContinuityCheck;

/* DecChanGetContinuityCheck */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	channelId;
} DecCmdChannelGetContinuityCheck;

typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	status;
    uint32_t	enable;
} DecRspChannelGetContinuityCheck;

/* DecChanSetBRCMTrickMode */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	channelId;
    uint32_t	enable;
    uint32_t	reverseField;
} DecCmdChannelSetBRCMTrickMode;

/* DecChanGetBRCMTrickMode */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	channelId;
} DecCmdChannelGetBRCMTrickMode;

typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	status;
    uint32_t	brcmTrickMode;
} DecRspChannelGetBRCMTrickMode;

/* DecChanReverseFieldStatus */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	channelId;
} DecCmdChannelReverseFieldStatus;

typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	status;
    uint32_t	reverseField;
} DecRspChannelReverseFieldStatus;

/* DecChanIPictureFound */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	channelId;
} DecCmdChannelIPictureFound;

typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	status;
    uint32_t	iPictureFound;
} DecRspChannelIPictureFound;

/* DecCmdChannelSetParameter */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	channelId;
    int32_t	videoDisplayOffset;
    int32_t	ptsStcPhaseThreshold;
    /* add more paras below ... */
} DecCmdChannelSetParameter;

/* DecCmdChannelSetUserDataMode */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	channelId;
    uint32_t	enable;
} DecCmdChannelSetUserDataMode;

/* DecCmdChannelSetPauseDisplayMode */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	channelId;
    eC011_DISPLAY_MODE	displayMode;
} DecCmdChannelSetPauseDisplayMode;

/* DecCmdChannelSetSlowDisplayMode */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	channelId;
    eC011_DISPLAY_MODE	displayMode;
} DecCmdChannelSetSlowDisplayMode;

/* DecCmdChannelSetFastForwardDisplayMode */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	channelId;
    eC011_DISPLAY_MODE	displayMode;
} DecCmdChannelSetFastForwardDisplayMode;

/* DecCmdChannelSetDisplayMode */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	channelId;
    eC011_DISPLAY_MODE	displayMode;
} DecCmdChannelSetDisplayMode;

/* DecCmdChannelGetDisplayMode */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	channelId;
} DecCmdChannelGetDisplayMode;

/* DecRspChannelGetDisplayMode */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	status;
    eC011_DISPLAY_MODE	displayMode;
} DecRspChannelGetDisplayMode;

/* DecCmdChannelSetReverseField */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	channelId;
    uint32_t	enable;
} DecCmdChannelSetReverseField;

/* DecCmdChannelSetDisplayTimingMode */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	channelId;
    eC011_DISPLAY_TIMING_MODE displayTiming;
} DecCmdChannelSetDisplayTimingMode;

/* DecCmdChannelStreamOpen */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    eC011_INPUT_PORT	 inPort;
    eC011_STREAM_TYPE	 streamType;
} DecCmdChannelStreamOpen;

typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	status;
    uint32_t	channelId;
    uint32_t	channelStatus;
} DecRspChannelStreamOpen;

/* DecChannelStartVideo */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	channelId;
    eC011_OUTPUT_PORT	 outVidPort;
    eC011_MAX_PICSIZE	 maxPicSize;
    eC011_OUTCTRL_MODE	outCtrlMode;
    eC011_CHANNEL_TYPE	chanType;
    uint32_t	defaultFrameRate;//reservedWord7;
    eC011_VIDEO_ALG	   videoAlg;
    eC011_VIDEO_SOURCE_MODE   sourceMode;
    eC011_PULLDOWN_MODE       pulldown;
    eC011_PICTURE_INFO_MODE   picInfo;
    eC011_DISPLAY_ORDER       displayOrder;
    uint32_t	decOperationMode; //reservedWord13;
    uint32_t	MaxFrameRateMode;//reservedWord14;
    uint32_t	streamId;
    eC011_DEBLOCKING_MODE     deblocking;
    eC011_EXTERNAL_VCXO_MODE  vcxoControl;
    eC011_DISPLAY_TIMING_MODE displayTiming;
    int32_t	videoDisplayOffset;
    eC011_USER_DATA_MODE      userDataMode;
    uint32_t	enableUserDataInterrupt;
    uint32_t	ptsStcDiffThreshold;
    uint32_t	stcPtsDiffThreshold;
    uint32_t	enableFirstPtsInterrupt;
    uint32_t	enableStcPtsThresholdInterrupt;
    uint32_t	frameRateDefinition;
    uint32_t	hostDmaInterruptEnable;
    uint32_t	asynchEventNotifyEnable;
    uint32_t	enablePtsStcChangeInterrupt;
    uint32_t	enablePtsErrorInterrupt;
    uint32_t	enableFgt;
    uint32_t	enable23_297FrameRateOutput;
    uint32_t	enableVideoDataUnderflowInterrupt;
    uint32_t	reservedWord34;
    uint32_t	pictureInfoInterruptEnable;
} DecCmdChannelStartVideo;

typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	status;
    uint32_t	picBuf;
    uint32_t	picRelBuf;
    uint32_t	picInfoDeliveryQ;
    uint32_t	picInfoReleaseQ;
    uint32_t	channelStatus;
    uint32_t	userDataDeliveryQ;
    uint32_t	userDataReleaseQ;
    uint32_t	transportStreamCaptureAddr;
    uint32_t	asyncEventQ;
} DecRspChannelStartVideo;

/* DecChannelStopVideo */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	channelId;
    eC011_PIC_REL_MODE	pictureRelease;
    eC011_LASTPIC_DISPLAY     lastPicDisplay;
} DecCmdChannelStopVideo;

/* DecCmdChannelSetPanScanMode */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	channelId;
   eC011_PAN_SCAN_MODE       ePanScanMode;
} DecCmdChannelSetPanScanMode;

/* DecChannelStartDisplayAtPTS */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	channelId;
    uint32_t	ptsValue0;
    uint32_t	ptsValue1;
} DecCmdChannelStartDisplayAtPTS;

/* DecChannelStopDisplayAtPTS */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	channelId;
    uint32_t	ptsValue0;
    uint32_t	ptsValue1;
} DecCmdChannelStopDisplayAtPTS;

/* DecChannelDisplayPauseUntoPTS */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	channelId;
    uint32_t	pausePtsValue0;
    uint32_t	pausePtsValue1;
    uint32_t	displayPtsValue0;
    uint32_t	displayPtsValue1;
    int32_t	pauseLoopAroundCounter;
    int32_t	displayLoopAroundCounter;
    int32_t	pauseUntoPtsValid;
    int32_t	displayUntoPtsValid;
} DecCmdChannelDisplayPauseUntoPTS;

/* DecCmdChanSetPtsStcDiffThreshold */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	channelId;
    uint32_t	ptsStcDiffThreshold;
} DecCmdChanSetPtsStcDiffThreshold;

/* DecChanSetDisplayOrder */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	channelId;
    uint32_t	decodeOrder;  // 0: displayOrder, 1: decodeOrder
} DecCmdChannelSetDisplayOrder;

/* DecChanGetDisplayOrder */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	channelId;
} DecCmdChannelGetDisplayOrder;

typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	channelId;
    uint32_t	decodeOrder;  // 0: displayOrder, 1: decodeOrder
} DecRspChannelGetDisplayOrder;

typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	channelId;
    uint32_t	CodeInBuffLowerThreshold;
    uint32_t	CodeInBuffHigherThreshold;
    int32_t	MaxNumVsyncsCodeInEmpty;
    int32_t	MaxNumVsyncsCodeInFull;
} DecCmdChanSetParametersForHardResetInterruptToHost;

/* EncChannelOpen */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    sC011_ENC_SEQ_PARAM       seqParam;     // sequence paramaters
    sC011_ENC_PIC_PARAM       picParam;     // picture parameters
    sC011_ENC_CODING_PARAM    codingParam;  // coding parameters
    sC011_ENC_VID_IN_PARAM    vidInParam;   // video-in parameters
    sC011_ENC_CODE_OUT_PARAM  codeOutParam; // code-out parameters
} EncCmdChannelOpen;

/* EncChannelClose */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	channelId;
    uint32_t	condClose;
    uint32_t	lastPicId;
} EncCmdChannelClose;

/* EncChannelActivate */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	channelId;
} EncCmdChannelActivate;

/* EncChannelControl */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	channelId;
    uint32_t	condControl;
    uint32_t	picId;
    eC011_ENC_CTRL_CODE       controlCode;
    uC011_ENC_CTRL_PARAM      controlParam;
} EncCmdChannelControl;

/* EncChannelStatistics */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	channelId;
} EncCmdChannelStatistics;

/* DecChannelColorSpaceConv */
typedef struct {
   uint32_t	command;
   uint32_t	sequence;
   eC011_OUTPUT_PORT	 portId;
   eC011_DEC_CSC_SETUP       enable;
   uint32_t	padInput; /* if the padded pixels need to be converted. 1: input; 0:output */
   uint32_t	lumaCoefY;
   uint32_t	lumaCoefU;
   uint32_t	lumaDCOffset;
   uint32_t	lumaOffset;
   uint32_t	lumaCoefChrV;
   uint32_t	chrUCoefY;
   uint32_t	chrUCoefU;
   uint32_t	chrUDCOffset;
   uint32_t	chrUOffset;
   uint32_t	chrUCoefChrV;
   uint32_t	chrVCoefY;
   uint32_t	chrVCoefU;
   uint32_t	chrVDCOffset;
   uint32_t	chrVOffset;
   uint32_t	chrVCoefChrV;
} DecCmdChannelColorSpaceConv;

typedef struct {
   uint32_t	command;
   uint32_t	sequence;
   eC011_OUTPUT_PORT		portId;
   eC011_DEC_RANGE_REMAP_SETUP      enable;
   eC011_DEC_RANGE_REMAP_VC1PROFILE vc1Profile;
   uint32_t	lumaEnable;
   uint32_t	lumaMultiplier;
   uint32_t	chromaEnable;
   uint32_t	chromaMultiplier;
} DecCmdChannelSetRangeRemap;

/* DecChanSetFgt */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	channelId;
    uint32_t	on;  // 0: fgt off, 1: fgt on
} DecCmdChannelSetFgt;

/* DecChanSetLastPicturePadding */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    eC011_OUTPUT_PORT	 portId;
    uint32_t	paddingValue;   // bits: 23-16 (Y), 15-8 (U), 7-0 (V)
    uint32_t	padFullScreen;  // 0: fgt off, 1: fgt on
} DecCmdChannelSetLastPicturePadding;

/* DecChanSetHostTrickMode */
typedef struct {
   uint32_t	command;
   uint32_t	sequence;
   uint32_t	channelId;
   uint32_t	enable;  /* 0:disable; 1:enable */
} DecCmdChannelSetHostTrickMode;

/* DecChanSetOperationMode */
typedef struct {
   uint32_t	command;
   uint32_t	sequence;
   uint32_t	reserved;
   eC011_DEC_OPERATION_MODE  mode;
} DecCmdChannelSetOperationMode;

/* DecChanSendCompressedBuffer */
typedef struct {
   uint32_t	command;
   uint32_t	sequence;
   uint32_t	dramInBufAddr;
   uint32_t	dataSizeInBytes;
} DecCmdSendCompressedBuffer;

/* DecChanSetClipping */
typedef struct {
   uint32_t	command;
   uint32_t	sequence;
   eC011_OUTPUT_PORT	 portId;
   eC011_OUTPUT_CLIPPING     outputClipping;
} DecCmdSetClipping;

/* DecSetContentKey */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	channelId;
    uint32_t	flags;
    uint32_t	inputKey0;  // bits 31:0
    uint32_t	inputKey1;  // bits 33:63
    uint32_t	inputKey2;
    uint32_t	inputKey3;
    uint32_t	inputIv0;  // bits 31:0
    uint32_t	inputIv1;
    uint32_t	inputIv2;
    uint32_t	inputIv3;
    uint32_t	outputKey0;
    uint32_t	outputKey1;
    uint32_t	outputKey2;
    uint32_t	outputKey3;
    uint32_t	outputIv0;
    uint32_t	outputIv1;
    uint32_t	outputIv2;
    uint32_t	outputIv3;
    uint32_t	outputStripeStart;	  // 0 based stripe encrypt start number, don't use 0
    uint32_t	outputStripeNumber;  // 0 based number of stripes to encrypt
    uint32_t	outputStripeLines;   // 0 = 256 lines, otherwise actual number of lines, start on second line
    uint32_t	outputStripeLineStart;   // 0 based start line number

    uint32_t	outputMode;
    uint32_t	outputyScramLen;
    uint32_t	outputuvLen;
    uint32_t	outputuvOffset;

    uint32_t	outputyLen;
    uint32_t	outputyOffset;
    union {												// Adaptive Output Encryption Percentages
	struct {
	    uint32_t	outputClearPercent:8;		// Clear Percentage
	    uint32_t	outputEncryptPercent:8;		// Encrypt Percentage
	    uint32_t	outputScramPercent:8;		// Scramble Percentage
	    uint32_t	output422Mode:8;			// 422 Mode
	} u;
	uint32_t	outputPercentage;
    } adapt;
    uint32_t	outputReserved1;

} DecCmdSetContentKey;

/* DecSetSessionKey */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	channelId;
    uint32_t	flags;
    uint32_t	sessionData[32]; // 128 bytes of cipher data.
} DecCmdSetSessionKey;

typedef struct {
   uint32_t	command;
   uint32_t	sequence;
   uint32_t	channelId;
   uint32_t	flags;
   uint32_t	reserved[4];
} DecCmdFormatChangeAck;

/* common response structure */
typedef struct {
    uint32_t	command;
    uint32_t	sequence;
    uint32_t	status;
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
