/********************************************************************
 * Copyright(c) 2006-2009 Broadcom Corporation.
 *
 *  Name: bc_dts_defs.h
 *
 *  Description: Common definitions for all components. Only types
 *		 is allowed to be included from this file.
 *
 *  AU
 *
 *  HISTORY:
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
 *******************************************************************/

#ifndef _BC_DTS_DEFS_H_
#define _BC_DTS_DEFS_H_

#include "bc_dts_types.h"

/* BIT Mask */
#define BC_BIT(_x)		(1 << (_x))

typedef enum _BC_STATUS {
	BC_STS_SUCCESS		= 0,
	BC_STS_INV_ARG		= 1,
	BC_STS_BUSY		= 2,
	BC_STS_NOT_IMPL		= 3,
	BC_STS_PGM_QUIT		= 4,
	BC_STS_NO_ACCESS	= 5,
	BC_STS_INSUFF_RES	= 6,
	BC_STS_IO_ERROR		= 7,
	BC_STS_NO_DATA		= 8,
	BC_STS_VER_MISMATCH	= 9,
	BC_STS_TIMEOUT		= 10,
	BC_STS_FW_CMD_ERR	= 11,
	BC_STS_DEC_NOT_OPEN	= 12,
	BC_STS_ERR_USAGE	= 13,
	BC_STS_IO_USER_ABORT	= 14,
	BC_STS_IO_XFR_ERROR	= 15,
	BC_STS_DEC_NOT_STARTED	= 16,
	BC_STS_FWHEX_NOT_FOUND	= 17,
	BC_STS_FMT_CHANGE	= 18,
	BC_STS_HIF_ACCESS	= 19,
	BC_STS_CMD_CANCELLED	= 20,
	BC_STS_FW_AUTH_FAILED	= 21,
	BC_STS_BOOTLOADER_FAILED = 22,
	BC_STS_CERT_VERIFY_ERROR = 23,
	BC_STS_DEC_EXIST_OPEN	= 24,
	BC_STS_PENDING		= 25,
	BC_STS_CLK_NOCHG	= 26,

	/* Must be the last one.*/
	BC_STS_ERROR		= -1
} BC_STATUS;

/*------------------------------------------------------*
 *    Registry Key Definitions				*
 *------------------------------------------------------*/
#define BC_REG_KEY_MAIN_PATH	"Software\\Broadcom\\MediaPC\\CrystalHD"
#define BC_REG_KEY_FWPATH		"FirmwareFilePath"
#define BC_REG_KEY_SEC_OPT		"DbgOptions"

/*
 * Options:
 *
 *  b[5] = Enable RSA KEY in EEPROM Support
 *  b[6] = Enable Old PIB scheme. (0 = Use PIB with video scheme)
 *
 *  b[12] = Enable send message to NotifyIcon
 *
 */

typedef enum _BC_SW_OPTIONS {
	BC_OPT_DOSER_OUT_ENCRYPT	= BC_BIT(3),
	BC_OPT_LINK_OUT_ENCRYPT		= BC_BIT(29),
} BC_SW_OPTIONS;

typedef struct _BC_REG_CONFIG{
	uint32_t		DbgOptions;
} BC_REG_CONFIG;

#if defined(__KERNEL__) || defined(__LINUX_USER__)
#else
/* Align data structures */
#define ALIGN(x)	__declspec(align(x))
#endif

/* mode
 * b[0]..b[7]	= _DtsDeviceOpenMode
 * b[8]		=  Load new FW
 * b[9]		=  Load file play back FW
 * b[10]	=  Disk format (0 for HD DVD and 1 for BLU ray)
 * b[11]-b[15]	=  default output resolution
 * b[16]	=  Skip TX CPB Buffer Check
 * b[17]	=  Adaptive Output Encrypt/Scramble Scheme
 * b[18]-b[31]	=  reserved for future use
 */

/* To allow multiple apps to open the device. */
enum _DtsDeviceOpenMode {
	DTS_PLAYBACK_MODE = 0,
	DTS_DIAG_MODE,
	DTS_MONITOR_MODE,
	DTS_HWINIT_MODE
};

/* To enable the filter to selectively enable/disable fixes or erratas */
enum _DtsDeviceFixMode {
	DTS_LOAD_NEW_FW		= BC_BIT(8),
	DTS_LOAD_FILE_PLAY_FW	= BC_BIT(9),
	DTS_DISK_FMT_BD		= BC_BIT(10),
	/* b[11]-b[15] : Default output resolution */
	DTS_SKIP_TX_CHK_CPB	= BC_BIT(16),
	DTS_ADAPTIVE_OUTPUT_PER	= BC_BIT(17),
	DTS_INTELLIMAP		= BC_BIT(18),
	/* b[19]-b[21] : select clock frequency */
	DTS_PLAYBACK_DROP_RPT_MODE = BC_BIT(22),
	DTS_DIAG_TEST_MODE = BC_BIT(23),
	DTS_SINGLE_THREADED_MODE = BC_BIT(24),
	DTS_FILTER_MODE = BC_BIT(25),
	DTS_MFT_MODE = BC_BIT(26)
};

#define DTS_DFLT_RESOLUTION(x)	(x<<11)

#define DTS_DFLT_CLOCK(x) (x<<19)

/* F/W File Version corresponding to S/W Releases */
enum _FW_FILE_VER {
	/* S/W release: 02.04.02	F/W release 2.12.2.0 */
	BC_FW_VER_020402 = ((12<<16) | (2<<8) | (0))
};

/*------------------------------------------------------*
 *    Stream Types for DtsOpenDecoder()			*
 *------------------------------------------------------*/
enum _DtsOpenDecStreamTypes {
	BC_STREAM_TYPE_ES		= 0,
	BC_STREAM_TYPE_PES		= 1,
	BC_STREAM_TYPE_TS		= 2,
	BC_STREAM_TYPE_ES_TSTAMP	= 6,
};

/*------------------------------------------------------*
 *    Video Algorithms for DtsSetVideoParams()		*
 *------------------------------------------------------*/
enum _DtsSetVideoParamsAlgo {
	BC_VID_ALGO_H264		= 0,
	BC_VID_ALGO_MPEG2		= 1,
	BC_VID_ALGO_VC1			= 4,
	BC_VID_ALGO_DIVX		= 6,
	BC_VID_ALGO_VC1MP		= 7,
};

/*------------------------------------------------------*
 *    MPEG Extension to the PPB				*
 *------------------------------------------------------*/
#define BC_MPEG_VALID_PANSCAN		(1)

typedef struct _BC_PIB_EXT_MPEG {
	uint32_t	valid;
	/* Always valid,  defaults to picture size if no
	 * sequence display extension in the stream. */
	uint32_t	display_horizontal_size;
	uint32_t	display_vertical_size;

	/* MPEG_VALID_PANSCAN
	 * Offsets are a copy values from the MPEG stream. */
	uint32_t	offset_count;
	int32_t		horizontal_offset[3];
	int32_t		vertical_offset[3];

} BC_PIB_EXT_MPEG;

/*------------------------------------------------------*
 *    H.264 Extension to the PPB			*
 *------------------------------------------------------*/
/* Bit definitions for 'other.h264.valid' field */
#define H264_VALID_PANSCAN		(1)
#define H264_VALID_SPS_CROP		(2)
#define H264_VALID_VUI			(4)

typedef struct _BC_PIB_EXT_H264 {
	/* 'valid' specifies which fields (or sets of
	 * fields) below are valid.  If the corresponding
	 * bit in 'valid' is NOT set then that field(s)
	 * is (are) not initialized. */
	uint32_t	valid;

	/* H264_VALID_PANSCAN */
	uint32_t	pan_scan_count;
	int32_t		pan_scan_left[3];
	int32_t		pan_scan_right[3];
	int32_t		pan_scan_top[3];
	int32_t		pan_scan_bottom[3];

	/* H264_VALID_SPS_CROP */
	int32_t		sps_crop_left;
	int32_t		sps_crop_right;
	int32_t		sps_crop_top;
	int32_t		sps_crop_bottom;

	/* H264_VALID_VUI */
	uint32_t	chroma_top;
	uint32_t	chroma_bottom;

} BC_PIB_EXT_H264;

/*------------------------------------------------------*
 *    VC1 Extension to the PPB				*
 *------------------------------------------------------*/
#define VC1_VALID_PANSCAN		(1)

typedef struct _BC_PIB_EXT_VC1 {
	uint32_t	valid;

	/* Always valid, defaults to picture size if no
	 * sequence display extension in the stream. */
	uint32_t	display_horizontal_size;
	uint32_t	display_vertical_size;

	/* VC1 pan scan windows */
	uint32_t	num_panscan_windows;
	int32_t		ps_horiz_offset[4];
	int32_t		ps_vert_offset[4];
	int32_t		ps_width[4];
	int32_t		ps_height[4];

} BC_PIB_EXT_VC1;


/*------------------------------------------------------*
 *    Picture Information Block				*
 *------------------------------------------------------*/
#if defined(__LINUX_USER__) || defined(_WIN32)
/* Values for 'pulldown' field.  '0' means no pulldown information
 * was present for this picture. */
enum {
	vdecNoPulldownInfo	= 0,
	vdecTop			= 1,
	vdecBottom		= 2,
	vdecTopBottom		= 3,
	vdecBottomTop		= 4,
	vdecTopBottomTop	= 5,
	vdecBottomTopBottom	= 6,
	vdecFrame_X2		= 7,
	vdecFrame_X3		= 8,
	vdecFrame_X1		= 9,
	vdecFrame_X4		= 10,
};

/* Values for the 'frame_rate' field. */
enum {
	vdecFrameRateUnknown = 0,
	vdecFrameRate23_97,
	vdecFrameRate24,
	vdecFrameRate25,
	vdecFrameRate29_97,
	vdecFrameRate30,
	vdecFrameRate50,
	vdecFrameRate59_94,
	vdecFrameRate60,
	vdecFrameRate14_985,
	vdecFrameRate7_496,
};

/* Values for the 'aspect_ratio' field. */
enum {
	vdecAspectRatioUnknown = 0,
	vdecAspectRatioSquare,
	vdecAspectRatio12_11,
	vdecAspectRatio10_11,
	vdecAspectRatio16_11,
	vdecAspectRatio40_33,
	vdecAspectRatio24_11,
	vdecAspectRatio20_11,
	vdecAspectRatio32_11,
	vdecAspectRatio80_33,
	vdecAspectRatio18_11,
	vdecAspectRatio15_11,
	vdecAspectRatio64_33,
	vdecAspectRatio160_99,
	vdecAspectRatio4_3,
	vdecAspectRatio16_9,
	vdecAspectRatio221_1,
	vdecAspectRatioOther = 255,
};

/* Values for the 'colour_primaries' field. */
enum {
	vdecColourPrimariesUnknown = 0,
	vdecColourPrimariesBT709,
	vdecColourPrimariesUnspecified,
	vdecColourPrimariesReserved,
	vdecColourPrimariesBT470_2M = 4,
	vdecColourPrimariesBT470_2BG,
	vdecColourPrimariesSMPTE170M,
	vdecColourPrimariesSMPTE240M,
	vdecColourPrimariesGenericFilm,
};

enum {
	vdecRESOLUTION_CUSTOM	= 0x00000000, /* custom */
	vdecRESOLUTION_480i	= 0x00000001, /* 480i */
	vdecRESOLUTION_1080i	= 0x00000002, /* 1080i (1920x1080, 60i) */
	vdecRESOLUTION_NTSC	= 0x00000003, /* NTSC (720x483, 60i) */
	vdecRESOLUTION_480p	= 0x00000004, /* 480p (720x480, 60p) */
	vdecRESOLUTION_720p	= 0x00000005, /* 720p (1280x720, 60p) */
	vdecRESOLUTION_PAL1	= 0x00000006, /* PAL_1 (720x576, 50i) */
	vdecRESOLUTION_1080i25	= 0x00000007, /* 1080i25 (1920x1080, 50i) */
	vdecRESOLUTION_720p50	= 0x00000008, /* 720p50 (1280x720, 50p) */
	vdecRESOLUTION_576p	= 0x00000009, /* 576p (720x576, 50p) */
	vdecRESOLUTION_1080i29_97 = 0x0000000A, /* 1080i (1920x1080, 59.94i) */
	vdecRESOLUTION_720p59_94  = 0x0000000B, /* 720p (1280x720, 59.94p) */
	vdecRESOLUTION_SD_DVD	= 0x0000000C, /* SD DVD (720x483, 60i) */
	vdecRESOLUTION_480p656	= 0x0000000D, /* 480p (720x480, 60p), output bus width 8 bit, clock 74.25MHz */
	vdecRESOLUTION_1080p23_976 = 0x0000000E, /* 1080p23_976 (1920x1080, 23.976p) */
	vdecRESOLUTION_720p23_976  = 0x0000000F, /* 720p23_976 (1280x720p, 23.976p) */
	vdecRESOLUTION_240p29_97   = 0x00000010, /* 240p (1440x240, 29.97p ) */
	vdecRESOLUTION_240p30	= 0x00000011, /* 240p (1440x240, 30p) */
	vdecRESOLUTION_288p25	= 0x00000012, /* 288p (1440x288p, 25p) */
	vdecRESOLUTION_1080p29_97 = 0x00000013, /* 1080p29_97 (1920x1080, 29.97p) */
	vdecRESOLUTION_1080p30	= 0x00000014, /* 1080p30 (1920x1080, 30p) */
	vdecRESOLUTION_1080p24	= 0x00000015, /* 1080p24 (1920x1080, 24p) */
	vdecRESOLUTION_1080p25	= 0x00000016, /* 1080p25 (1920x1080, 25p) */
	vdecRESOLUTION_720p24	= 0x00000017, /* 720p24 (1280x720, 25p) */
	vdecRESOLUTION_720p29_97  = 0x00000018, /* 720p29.97 (1280x720, 29.97p) */
	vdecRESOLUTION_480p23_976 = 0x00000019, /* 480p23.976 (720*480, 23.976) */
	vdecRESOLUTION_480p29_97  = 0x0000001A, /* 480p29.976 (720*480, 29.97p) */
	vdecRESOLUTION_576p25	= 0x0000001B, /* 576p25 (720*576, 25p) */
	/* For Zero Frame Rate */
	vdecRESOLUTION_480p0	= 0x0000001C, /* 480p (720x480, 0p) */
	vdecRESOLUTION_480i0	= 0x0000001D, /* 480i (720x480, 0i) */
	vdecRESOLUTION_576p0	= 0x0000001E, /* 576p (720x576, 0p) */
	vdecRESOLUTION_720p0	= 0x0000001F, /* 720p (1280x720, 0p) */
	vdecRESOLUTION_1080p0	= 0x00000020, /* 1080p (1920x1080, 0p) */
	vdecRESOLUTION_1080i0	= 0x00000021, /* 1080i (1920x1080, 0i) */
};

/* Bit definitions for 'flags' field */
#define VDEC_FLAG_EOS				(0x0004)

#define VDEC_FLAG_FRAME				(0x0000)
#define VDEC_FLAG_FIELDPAIR			(0x0008)
#define VDEC_FLAG_TOPFIELD			(0x0010)
#define VDEC_FLAG_BOTTOMFIELD			(0x0018)

#define VDEC_FLAG_PROGRESSIVE_SRC		(0x0000)
#define VDEC_FLAG_INTERLACED_SRC		(0x0020)
#define VDEC_FLAG_UNKNOWN_SRC			(0x0040)

#define VDEC_FLAG_BOTTOM_FIRST			(0x0080)
#define VDEC_FLAG_LAST_PICTURE			(0x0100)

#define VDEC_FLAG_PICTURE_META_DATA_PRESENT	(0x40000)

#endif /* __LINUX_USER__ */

typedef struct _BC_PIC_INFO_BLOCK {
	/* Common fields. */
	uint64_t	timeStamp;	/* Timestamp */
	uint32_t	picture_number;	/* Ordinal display number  */
	uint32_t	width;		/* pixels	    */
	uint32_t	height;		/* pixels	    */
	uint32_t	chroma_format;	/* 0x420, 0x422 or 0x444 */
	uint32_t	pulldown;
	uint32_t	flags;
	uint32_t	frame_rate;
	uint32_t	aspect_ratio;
	uint32_t	colour_primaries;
	uint32_t	picture_meta_payload;
	uint32_t	sess_num;
	uint32_t	ycom;
	uint32_t	custom_aspect_ratio_width_height;
	uint32_t	n_drop;	/* number of non-reference frames remaining to be dropped */

	/* Protocol-specific extensions. */
	union {
		BC_PIB_EXT_H264	h264;
		BC_PIB_EXT_MPEG	mpeg;
		BC_PIB_EXT_VC1	 vc1;
	} other;

} BC_PIC_INFO_BLOCK, *PBC_PIC_INFO_BLOCK;

/*------------------------------------------------------*
 *    ProcOut Info					*
 *------------------------------------------------------*/
/* Optional flags for ProcOut Interface.*/
enum _POUT_OPTIONAL_IN_FLAGS_{
	/* Flags from App to Device */
	BC_POUT_FLAGS_YV12	  = 0x01,	/* Copy Data in YV12 format */
	BC_POUT_FLAGS_STRIDE	  = 0x02,	/* Stride size is valid. */
	BC_POUT_FLAGS_SIZE	  = 0x04,	/* Take size information from Application */
	BC_POUT_FLAGS_INTERLACED  = 0x08,	/* copy only half the bytes */
	BC_POUT_FLAGS_INTERLEAVED = 0x10,	/* interleaved frame */
	BC_POUT_FLAGS_STRIDE_UV	  = 0x20,	/* Stride size is valid (for UV buffers). */
	BC_POUT_FLAGS_MODE	  = 0x40,	/* Take output mode from Application, overrides YV12 flag if on */

	/* Flags from Device to APP */
	BC_POUT_FLAGS_FMT_CHANGE  = 0x10000,	/* Data is not VALID when this flag is set */
	BC_POUT_FLAGS_PIB_VALID	  = 0x20000,	/* PIB Information valid */
	BC_POUT_FLAGS_ENCRYPTED	  = 0x40000,	/* Data is encrypted. */
	BC_POUT_FLAGS_FLD_BOT	  = 0x80000,	/* Bottom Field data */
};

//Decoder Capability
enum DECODER_CAP_FLAGS
{
	BC_DEC_FLAGS_H264		= 0x01,
	BC_DEC_FLAGS_MPEG2		= 0x02,
	BC_DEC_FLAGS_VC1		= 0x04,
	BC_DEC_FLAGS_M4P2		= 0x08,	//MPEG-4 Part 2: Divx, Xvid etc.
};

#if defined(__KERNEL__) || defined(__LINUX_USER__)
typedef BC_STATUS(*dts_pout_callback)(void  *shnd, uint32_t width, uint32_t height, uint32_t stride, void *pOut);
#else
typedef BC_STATUS(*dts_pout_callback)(void  *shnd, uint32_t width, uint32_t height, uint32_t stride, struct _BC_DTS_PROC_OUT *pOut);
#endif

/* Line 21 Closed Caption */
/* User Data */
#define MAX_UD_SIZE		1792	/* 1920 - 128 */

typedef struct _BC_DTS_PROC_OUT {
	uint8_t		*Ybuff;			/* Caller Supplied buffer for Y data */
	uint32_t	YbuffSz;		/* Caller Supplied Y buffer size */
	uint32_t	YBuffDoneSz;		/* Transferred Y datasize */

	uint8_t		*UVbuff;		/* Caller Supplied buffer for UV data */
	uint32_t	UVbuffSz;		/* Caller Supplied UV buffer size */
	uint32_t	UVBuffDoneSz;		/* Transferred UV data size */

	uint32_t	StrideSz;		/* Caller supplied Stride Size */
	uint32_t	PoutFlags;		/* Call IN Flags */

	uint32_t	discCnt;		/* Picture discontinuity count */

	BC_PIC_INFO_BLOCK PicInfo;		/* Picture Information Block Data */

	/* Line 21 Closed Caption */
	/* User Data */
	uint32_t	UserDataSz;
	uint8_t		UserData[MAX_UD_SIZE];

	void		*hnd;
	dts_pout_callback AppCallBack;
	uint8_t		DropFrames;
	uint8_t		b422Mode;		/* Picture output Mode */
	uint8_t		bPibEnc;		/* PIB encrypted */
	uint8_t		bRevertScramble;
	uint32_t	StrideSzUV;		/* Caller supplied Stride Size */

} BC_DTS_PROC_OUT;

typedef struct _BC_DTS_STATUS {
	uint8_t		ReadyListCount;	/* Number of frames in ready list (reported by driver) */
	uint8_t		FreeListCount;	/* Number of frame buffers free.  (reported by driver) */
	uint8_t		PowerStateChange; /* Number of active state power transitions (reported by driver) */
	uint8_t		reserved_[1];

	uint32_t	FramesDropped;	/* Number of frames dropped.  (reported by DIL) */
	uint32_t	FramesCaptured;	/* Number of frames captured. (reported by DIL) */
	uint32_t	FramesRepeated;	/* Number of frames repeated. (reported by DIL) */

	uint32_t	InputCount;	/* Times compressed video has been sent to the HW.
					 * i.e. Successful DtsProcInput() calls (reported by DIL) */
	uint64_t	InputTotalSize;	/* Amount of compressed video that has been sent to the HW.
					 * (reported by DIL) */
	uint32_t	InputBusyCount;	/* Times compressed video has attempted to be sent to the HW
					 * but the input FIFO was full. (reported by DIL) */

	uint32_t	PIBMissCount;	/* Amount of times a PIB is invalid. (reported by DIL) */

	uint32_t	cpbEmptySize;	/* supported only for H.264, specifically changed for
					 * SingleThreadedAppMode. Report size of CPB buffer available.
					 * Reported by DIL */
	uint64_t	NextTimeStamp;	/* TimeStamp of the next picture that will be returned
					 * by a call to ProcOutput. Added for SingleThreadedAppMode.
					 * Reported back from the driver */
	uint8_t		TxBufData;

	uint8_t		reserved__[3];

	uint32_t	picNumFlags; /* Picture number and flags of the next picture to be delivered from the driver */

	uint8_t		reserved___[8];

} BC_DTS_STATUS;

#define BC_SWAP32(_v)			\
	((((_v) & 0xFF000000)>>24)|	\
	  (((_v) & 0x00FF0000)>>8)|	\
	  (((_v) & 0x0000FF00)<<8)|	\
	  (((_v) & 0x000000FF)<<24))

#define WM_AGENT_TRAYICON_DECODER_OPEN	10001
#define WM_AGENT_TRAYICON_DECODER_CLOSE	10002
#define WM_AGENT_TRAYICON_DECODER_START	10003
#define WM_AGENT_TRAYICON_DECODER_STOP	10004
#define WM_AGENT_TRAYICON_DECODER_RUN	10005
#define WM_AGENT_TRAYICON_DECODER_PAUSE	10006

#define MAX_COLOR_SPACES	3

typedef enum _BC_OUTPUT_FORMAT {
	MODE420			= 0x0,
	MODE422_YUY2		= 0x1,
	MODE422_UYVY		= 0x2,
	OUTPUT_MODE420		= 0x0,
	OUTPUT_MODE422_YUY2	= 0x1,
	OUTPUT_MODE422_UYVY	= 0x2,
	OUTPUT_MODE420_NV12	= 0x0,
	OUTPUT_MODE_INVALID	= 0xFF,
} BC_OUTPUT_FORMAT;

typedef struct _BC_COLOR_SPACES_ {
	BC_OUTPUT_FORMAT	OutFmt[MAX_COLOR_SPACES];
	uint16_t		Count;
} BC_COLOR_SPACES;


typedef enum _BC_CAPS_FLAGS_ {
	PES_CONV_SUPPORT	= 1,	/*Support PES Conversion*/
	MULTIPLE_DECODE_SUPPORT	= 2	/*Support multiple stream decode*/
} BC_CAPS_FLAGS;

typedef struct _BC_HW_CAPABILITY_ {
	BC_CAPS_FLAGS		flags;
	BC_COLOR_SPACES		ColorCaps;
	void*			Reserved1;	/* Expansion Of API */

	//Decoder Capability
	uint32_t		DecCaps;	//DECODER_CAP_FLAGS
} BC_HW_CAPS, *PBC_HW_CAPS;

typedef struct _BC_SCALING_PARAMS_ {
	uint32_t	sWidth;
	uint32_t	sHeight;
	uint32_t	DNR;
	uint32_t	Reserved1;	/*Expansion Of API*/
	uint8_t		*Reserved2;	/*Expansion OF API*/
	uint32_t	Reserved3;	/*Expansion Of API*/
	uint8_t		*Reserved4;	/*Expansion Of API*/

} BC_SCALING_PARAMS, *PBC_SCALING_PARAMS;

typedef enum _BC_MEDIA_SUBTYPE_ {
	BC_MSUBTYPE_INVALID = 0,
	BC_MSUBTYPE_MPEG1VIDEO,
	BC_MSUBTYPE_MPEG2VIDEO,
	BC_MSUBTYPE_H264,
	BC_MSUBTYPE_WVC1,
	BC_MSUBTYPE_WMV3,
	BC_MSUBTYPE_AVC1,
	BC_MSUBTYPE_WMVA,
	BC_MSUBTYPE_VC1,
	BC_MSUBTYPE_DIVX,
	BC_MSUBTYPE_DIVX311,
	BC_MSUBTYPE_OTHERS	/*Types to facilitate PES conversion*/
} BC_MEDIA_SUBTYPE;

typedef struct _BC_INPUT_FORMAT_ {
	BOOL        FGTEnable;      /*Enable processing of FGT SEI*/
	BOOL        MetaDataEnable; /*Enable retrieval of picture metadata to be sent to video pipeline.*/
	BOOL        Progressive;    /*Instruct decoder to always try to send back progressive
				     frames. If input content is 1080p, the decoder will
				     ignore pull-down flags and always give 1080p output.
				     If 1080i content is processed, the decoder will return
				     1080i data. When this flag is not set, the decoder will
				     use pull-down information in the input stream to decide
				     the decoded data format.*/
	uint32_t    OptFlags;       /*In this field bits 0:3 are used pass default frame rate, bits 4:5 are for operation mode
				     (used to indicate Blu-ray mode to the decoder) and bit 6 is for the flag mpcOutPutMaxFRate
				     which when set tells the FW to output at the max rate for the resolution and ignore the
				     frame rate determined from the stream. Bit 7 is set to indicate that this is single threaded
				     mode and the driver will be peeked to get timestamps ahead of time*/
	BC_MEDIA_SUBTYPE mSubtype;  /* Video Media Type*/
	uint32_t    width;
	uint32_t    height;
	uint32_t    startCodeSz;    /*Start code size for H264 clips*/
	uint8_t     *pMetaData;     /*Metadata buffer that is used to pass sequence header*/
	uint32_t    metaDataSz;     /*Metadata size*/
	uint8_t     bEnableScaling;
	BC_SCALING_PARAMS ScalingParams;
} BC_INPUT_FORMAT;

typedef struct _BC_INFO_CRYSTAL_ {
	uint8_t device;
	union {
		struct {
			uint32_t dilRelease:8;
			uint32_t dilMajor:8;
			uint32_t dilMinor:16;
		};
		uint32_t version;
	} dilVersion;

	union {
		struct {
			uint32_t drvRelease:4;
			uint32_t drvMajor:8;
			uint32_t drvMinor:12;
			uint32_t drvBuild:8;
		};
		uint32_t version;
	} drvVersion;

	union {
		struct {
			uint32_t fwRelease:4;
			uint32_t fwMajor:8;
			uint32_t fwMinor:12;
			uint32_t fwBuild:8;
		};
		uint32_t version;
	} fwVersion;

	uint32_t Reserved1; // For future expansion
	uint32_t Reserved2; // For future expansion
} BC_INFO_CRYSTAL, *PBC_INFO_CRYSTAL;

#endif	/* _BC_DTS_DEFS_H_ */
