/********************************************************************
 * Copyright (c) 2004-2009 Broadcom Corporation.
 *
 *  Common Video Decoder Information
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

#ifndef __INC_VDEC_INFO_H__
#define __INC_VDEC_INFO_H__

#pragma pack (1)

/* User Data Header */
typedef struct user_data {
   struct user_data* POINTER_32 next;
   uint32_t      type;
   uint32_t      size;
} UD_HDR;

/*------------------------------------------------------*
 *    MPEG Extension to the PPB				*
 *------------------------------------------------------*/
#define  MPEG_VALID_PANSCAN		(1)
#define  MPEG_VALID_USER_DATA		(2)
#define  MPEG_USER_DATA_OVERFLOW	(4)

#define  MPEG_USER_DATA_TYPE_SEQ	(1)
#define  MPEG_USER_DATA_TYPE_GOP	(2)
#define  MPEG_USER_DATA_TYPE_PIC	(4)
#define  MPEG_USER_DATA_TYPE_TOP	(8)
#define  MPEG_USER_DATA_TYPE_BTM	(16)
#define  MPEG_USER_DATA_TYPE_I		(32)
#define  MPEG_USER_DATA_TYPE_P		(64)
#define  MPEG_USER_DATA_TYPE_B		(128)

typedef struct {
   uint32_t	to_be_defined;
   uint32_t     valid;

   /* Always valid,  defaults to picture size if no
      sequence display extension in the stream. */
   uint32_t     display_horizontal_size;
   uint32_t     display_vertical_size;

   /* MPEG_VALID_PANSCAN
      Offsets are a copy values from the MPEG stream. */
   uint32_t     offset_count;
   int32_t	horizontal_offset[3];
   int32_t	vertical_offset[3];

   /* MPEG_VALID_USERDATA
      User data is in the form of a linked list. */
   int32_t	userDataSize;
   UD_HDR* POINTER_32 userData;

} PPB_MPEG;



/*------------------------------------------------------*
 *    VC1 Extension to the PPB				*
 *------------------------------------------------------*/
#define  VC1_VALID_PANSCAN		(1)
#define  VC1_VALID_USER_DATA		(2)
#define  VC1_USER_DATA_OVERFLOW		(4)
#define  VC1_USER_DATA_TYPE_SEQ		(1)
#define  VC1_USER_DATA_TYPE_ENTRYPOINT	(2)
#define  VC1_USER_DATA_TYPE_FRM		(4)
#define  VC1_USER_DATA_TYPE_FLD		(8)
#define  VC1_USER_DATA_TYPE_SLICE	(16)

typedef struct {
   uint32_t	to_be_defined;
   uint32_t	valid;

   /* Always valid,  defaults to picture size if no
      sequence display extension in the stream. */
   uint32_t	display_horizontal_size;
   uint32_t	display_vertical_size;

  /* VC1 pan scan windows
   */
   uint32_t	num_panscan_windows;
   int32_t	ps_horiz_offset[4];
   int32_t	ps_vert_offset[4];
   int32_t	ps_width[4];
   int32_t	ps_height[4];

   /* VC1_VALID_USERDATA
      User data is in the form of a linked list. */
   int32_t	userDataSize;
   UD_HDR* POINTER_32 userData;

} PPB_VC1;



/*------------------------------------------------------*
 *    H.264 Extension to the PPB			*
 *------------------------------------------------------*/
/**
 * @brief Film grain SEI message.
 *
 * Content of the film grain SEI message.
 */
//maximum number of model-values as for Thomson spec(standard says 5)
#define MAX_FGT_MODEL_VALUE	 (3)
//maximum number of intervals(as many as 256 intervals?)
#define MAX_FGT_VALUE_INTERVAL      (256)

typedef struct FGT_SEI {
    struct FGT_SEI* POINTER_32 next;
    unsigned char model_values[3][MAX_FGT_VALUE_INTERVAL][MAX_FGT_MODEL_VALUE];
    unsigned char upper_bound[3][MAX_FGT_VALUE_INTERVAL];
    unsigned char lower_bound[3][MAX_FGT_VALUE_INTERVAL];

    unsigned char cancel_flag;		/**< Cancel flag: 1 no film grain. */
    unsigned char model_id;		/**< Model id. */

    //+unused SE based on Thomson spec
    unsigned char color_desc_flag;	/**< Separate color descrition flag. */
    unsigned char bit_depth_luma;	/**< Bit depth luma minus 8. */
    unsigned char bit_depth_chroma;	/**< Bit depth chroma minus 8. */
    unsigned char full_range_flag;	/**< Full range flag. */
    unsigned char color_primaries;	/**< Color primaries. */
    unsigned char transfer_charact;	/**< Transfer characteristics. */
    unsigned char matrix_coeff;		/**< Matrix coefficients. */
    //-unused SE based on Thomson spec

    unsigned char blending_mode_id;	/**< Blending mode. */
    unsigned char log2_scale_factor;	/**< Log2 scale factor (2-7). */
    unsigned char comp_flag[3];		/**< Components [0,2] parameters present flag. */
    unsigned char num_intervals_minus1[3]; /**< Number of intensity level intervals. */
    unsigned char num_model_values[3];	/**< Number of model values. */
    uint16_t repetition_period;		/**< Repetition period (0-16384) */
} FGT_SEI;

/* Bit definitions for H.264 user data type field
 */
#define  AVC_USERDATA_TYPE_REGISTERED	4
#define  AVC_USERDATA_TYPE_TOP		8
#define  AVC_USERDATA_TYPE_BOT		16


/* Bit definitions for 'other.h264.valid' field */
#define  H264_VALID_PANSCAN		(1)
#define  H264_VALID_SPS_CROP		(2)
#define  H264_VALID_VUI			(4)
#define  H264_VALID_USER		(8)
#define  H264_VALID_CT_TYPE		(16)
#define  H264_USER_OVERFLOW		(32)
#define  H264_FILM_GRAIN_MSG		(64)

typedef struct {
   /* 'valid' specifies which fields (or sets of
    * fields) below are valid.  If the corresponding
    * bit in 'valid' is NOT set then that field(s)
    * is (are) not initialized. */
   uint32_t	valid;

   int32_t	poc_top;	  /* POC for Top Field/Frame */
   int32_t	poc_bottom;       /* POC for Bottom Field    */
   uint32_t	idr_pic_id;

   /* H264_VALID_PANSCAN */
   uint32_t	pan_scan_count;
   int32_t	pan_scan_left  [3];
   int32_t	pan_scan_right [3];
   int32_t	pan_scan_top   [3];
   int32_t	pan_scan_bottom[3];

   /* H264_VALID_CT_TYPE */
   uint32_t	ct_type_count;
   uint32_t	ct_type[3];

   /* H264_VALID_SPS_CROP */
   int32_t	sps_crop_left;
   int32_t	sps_crop_right;
   int32_t	sps_crop_top;
   int32_t	sps_crop_bottom;

   /* H264_VALID_VUI */
   uint32_t	chroma_top;
   uint32_t	chroma_bottom;

   /* H264_VALID_USER */
   uint32_t	user_data_size;
   UD_HDR* POINTER_32 user_data;

   /* H264 VALID FGT */
   FGT_SEI* POINTER_32 pfgt;

} PPB_H264;


/*------------------------------------------------------*
 *    Picture Parameter Block			   *
 *------------------------------------------------------*/

/* Bit definitions for 'flags' field */
#define  VDEC_FLAG_PTS_PRESENT			(0x0001)
#define  VDEC_FLAG_PTS_MSB			(0x0002)
#define  VDEC_FLAG_EOS				(0x0004)

#define  VDEC_FLAG_FRAME			(0x0000)
#define  VDEC_FLAG_FIELDPAIR			(0x0008)
#define  VDEC_FLAG_TOPFIELD			(0x0010)
#define  VDEC_FLAG_BOTTOMFIELD			(0x0018)

#define  VDEC_FLAG_PROGRESSIVE_SRC		(0x0000)
#define  VDEC_FLAG_INTERLACED_SRC		(0x0020)
#define  VDEC_FLAG_UNKNOWN_SRC			(0x0040)

#define  VDEC_FLAG_BOTTOM_FIRST			(0x0080)
#define  VDEC_FLAG_LAST_PICTURE			(0x0100)

#define  VDEC_FLAG_STC_IS_CRAP			(0x0200)
#define  VDEC_FLAG_PCR_OFFSET_PRESENT		(0x0400)
#define  VDEC_FLAG_REF_CNTR_TYPE_PCR		(0x0800)
#define  VDEC_FLAG_DISCONT_PCR_OFFSET		(0x1000)
#define  VDEC_FLAG_PICTURE_TAG_VALID		(0x2000)

#define  VDEC_FLAG_PICTURE_DONE_MARKER_PRESENT	(0x20000)

#define  VDEC_FLAG_PICTURE_META_DATA_PRESENT	(0x40000)

#define  VDEC_FLAG_RESOLUTION_CHANGE		(0x80000)

/* Values for the 'chroma_format' field. */
enum {
   vdecChroma420 = 0x420,
   vdecChroma422 = 0x422,
   vdecChroma444 = 0x444,
};

#if !defined(_WIN32) && !defined(_WIN64) && !defined(__LINUX_USER__)

//////////////////////////////////////////////////////////////////////
/////// Moved to bc_dts_defs.h file for external Exports /////////////
//////////////////////////////////////////////////////////////////////

/* Values for 'pulldown' field.  '0' means no pulldown information
 * was present for this picture. */
enum {
   vdecNoPulldownInfo	= 0,
   vdecTop		= 1,
   vdecBottom		= 2,
   vdecTopBottom	= 3,
   vdecBottomTop	= 4,
   vdecTopBottomTop	= 5,
   vdecBottomTopBottom	= 6,
   vdecFrame_X2		= 7,
   vdecFrame_X3		= 8,
   vdecFrame_X1		= 9,
   vdecFrame_X4		= 10,
};

/* Values for 'protocol' field. */
enum {
   protocolH264 = 0,
   protocolMPEG2,
   protocolH261,
   protocolH263,
   protocolVC1,
   protocolMPEG1,
   protocolMPEG2DTV,
   protocolVC1ASF,
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
};

/* Values for the 'matrix_coeff' field. */
enum {
   vdecMatrixCoeffUnknown   = 0,
   vdecMatrixCoeffBT709,
   vdecMatrixCoeffUnspecified,
   vdecMatrixCoeffReserved,
   vdecMatrixCoeffFCC       = 4,
   vdecMatrixCoeffBT740_2BG,
   vdecMatrixCoeffSMPTE170M,
   vdecMatrixCoeffSMPTE240M,
   vdecMatrixCoeffSMPTE293M,
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

/* Values for the 'transfer_char' field. */
enum {
   vdecTransferCharUnknown = 0,
   vdecTransferCharBT709,
   vdecTransferCharUnspecified,
   vdecTransferCharReserved,
   vdecTransferCharBT479_2M = 4,
   vdecTransferCharBT479_2BG,
   vdecTransferCharSMPTE170M,
   vdecTransferCharSMPTE240M,
   vdecTransferCharLinear,
   vdecTransferCharLog100_1,
   vdecTransferCharLog31622777_1,
   vdecColourPrimariesBT1361,
};

#endif // _WIN32

typedef struct {
   /* Common fields. */
   uint32_t	picture_number;   /* Ordinal display number */
   uint32_t	video_buffer;     /* Video (picbuf) number */
   uint32_t	video_address;    /* Address of picbuf Y */
   uint32_t	video_address_uv; /* Address of picbuf UV */
   uint32_t	video_stripe;     /* Picbuf stripe */
   uint32_t	video_width;      /* Picbuf width */
   uint32_t	video_height;     /* Picbuf height */

   uint32_t	channel_id;	  /* Decoder channel ID */
   uint32_t	status;		  /* reserved */
   uint32_t	width;		  /* pixels */
   uint32_t	height;		  /* pixels */
   uint32_t	chroma_format;	  /* see above */
   uint32_t	pulldown;	  /* see above */
   uint32_t	flags;		  /* see above */
   uint32_t	pts;		  /* 32 LSBs of PTS */
   uint32_t	protocol;	  /* protocolXXX (above) */

   uint32_t	frame_rate;       /* see above */
   uint32_t	matrix_coeff;     /* see above */
   uint32_t	aspect_ratio;     /* see above */
   uint32_t	colour_primaries; /* see above */
   uint32_t	transfer_char;    /* see above */
   uint32_t	pcr_offset;       /* 45kHz if PCR type; else 27MHz */
   uint32_t	n_drop;		  /* Number of pictures to be dropped */

   uint32_t	custom_aspect_ratio_width_height; /* upper 16-bits is Y and lower 16-bits is X */

   uint32_t	picture_tag;      /* Indexing tag from BUD packets */
   uint32_t	picture_done_payload;
   uint32_t	picture_meta_payload;
   uint32_t	reserved[1];

   /* Protocol-specific extensions. */
   union
   {
      PPB_H264	h264;
      PPB_MPEG	mpeg;
      PPB_VC1	 vc1;
   } other;
} PPB;


typedef struct {
    /* Approximate number of pictures in
    * the CPB */
    uint32_t	codein_picture_count;

    /* GOP time code */
    uint32_t	time_code_hours;
    uint32_t	time_code_minutes;
    uint32_t	time_code_seconds;
    uint32_t	time_code_pictures;

    uint32_t	pic_done_marker_btp_payload;

    uint32_t	start_decode_time;
    uint32_t	chan_open_time;
    uint32_t	chan_close_start_time;
    uint32_t	chan_close_end_time;
    uint32_t	first_pic_delivery_time;
    uint32_t	first_code_in_packet_time;
    uint32_t	ts_fifo_byte_non_zero_time;
    uint32_t	ts_video_packet_count_non_zero_time;
} VdecStatusBlock;


#if defined(CHIP_7401A)

/* Display Manager: STC and Parity Information in DRAM for host to use */
typedef struct {
    uint32_t stc_snapshot;
    uint32_t vsync_parity;
    uint32_t vsync_count; /* used only for debug */
} DisplayInfo;

/* Parameters passed from Display Manager (host) to DMS */
typedef struct {
   uint32_t write_offset;
   uint32_t drop_count;
} DMS_Info;

/* Picture Delivery parameters required by Display Manager */
typedef struct {
    uint32_t queue_read_offset; /* offset is w.r.t base of this data struct so value of 0-1 prohibited */
    uint32_t queue_write_offset; /* offset is w.r.t base of this data struct so value of 0-1 prohibited */
    /* queue if full if (write_offset+1 == read_offset) */
    /* write_offset modified by firmware and read_offset modified by Display Manager in host */
    PPB* POINTER_32 display_elements[62];
} PictureDeliveryQueue;

/* Picture Release parameters returned by Display Manager to firmware */
typedef struct {
    uint32_t queue_read_offset; /* offset is w.r.t base of this data struct so value of 0-1 prohibited */
    uint32_t queue_write_offset; /* offset is w.r.t base of this data struct so value of 0-1 prohibited */
    /* queue if full if (write_offset+1 == read_offset) */
    /* read_offset modified by firmware and write_offset modified by Display Manager in host */
    PPB* POINTER_32 display_elements[62];
} PictureReleaseQueue;
#endif

#pragma pack ()

#endif // __INC_VDEC_INFO_H__
