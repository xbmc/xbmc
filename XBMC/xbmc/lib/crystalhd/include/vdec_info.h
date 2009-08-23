/***************************************************************************
 *     Copyright (c) 2004,2005 Broadcom Corporation
 *     All Rights Reserved
 *     Confidential Property of Broadcom Corporation
 *
 *  THIS SOFTWARE MAY ONLY BE USED SUBJECT TO AN EXECUTED SOFTWARE LICENSE
 *  AGREEMENT  BETWEEN THE USER AND BROADCOM.  YOU HAVE NO RIGHT TO USE OR
 *  EXPLOIT THIS MATERIAL EXCEPT SUBJECT TO THE TERMS OF SUCH AN AGREEMENT.
 *
 ***************************************************************************
 *  Common Video Decoder Information
 *  $Id: //galaxy/main/include/vdec_info.h#2 $
 ***************************************************************************/

#ifndef __INC_VDEC_INFO_H__
#define __INC_VDEC_INFO_H__

#pragma pack (1)

/* User Data Header */
typedef struct user_data
{
   struct user_data* POINTER_32 next;
   unsigned long      type;
   unsigned long      size;
} UD_HDR;

/*------------------------------------------------------*
 *    MPEG Extension to the PPB                         *
 *------------------------------------------------------*/
#define  MPEG_VALID_PANSCAN             (1)
#define  MPEG_VALID_USER_DATA           (2)
#define  MPEG_USER_DATA_OVERFLOW        (4)

#define  MPEG_USER_DATA_TYPE_SEQ        (1)
#define  MPEG_USER_DATA_TYPE_GOP        (2)
#define  MPEG_USER_DATA_TYPE_PIC        (4)
#define  MPEG_USER_DATA_TYPE_TOP        (8)
#define  MPEG_USER_DATA_TYPE_BTM        (16)
#define  MPEG_USER_DATA_TYPE_I          (32)
#define  MPEG_USER_DATA_TYPE_P          (64)
#define  MPEG_USER_DATA_TYPE_B          (128)

typedef struct
{
   unsigned           to_be_defined;
   unsigned long      valid;

   /* Always valid,  defaults to picture size if no
      sequence display extension in the stream. */
   unsigned long      display_horizontal_size;
   unsigned long      display_vertical_size;

   /* MPEG_VALID_PANSCAN
      Offsets are a copy values from the MPEG stream. */
   unsigned long      offset_count;
   long               horizontal_offset[3];
   long               vertical_offset[3];

   /* MPEG_VALID_USERDATA
      User data is in the form of a linked list. */
   long               userDataSize;
   UD_HDR* POINTER_32 userData;

} PPB_MPEG;



/*------------------------------------------------------*
 *    VC1 Extension to the PPB                          *
 *------------------------------------------------------*/
#define  VC1_VALID_PANSCAN             (1)
#define  VC1_VALID_USER_DATA           (2)
#define  VC1_USER_DATA_OVERFLOW        (4)
#define  VC1_USER_DATA_TYPE_SEQ        (1)
#define  VC1_USER_DATA_TYPE_ENTRYPOINT (2)
#define  VC1_USER_DATA_TYPE_FRM        (4)
#define  VC1_USER_DATA_TYPE_FLD        (8)
#define  VC1_USER_DATA_TYPE_SLICE      (16)

typedef struct
{
   unsigned           to_be_defined;
   unsigned long      valid;

   /* Always valid,  defaults to picture size if no
      sequence display extension in the stream. */
   unsigned long      display_horizontal_size;
   unsigned long      display_vertical_size;

  /* VC1 pan scan windows
   */
   unsigned long      num_panscan_windows;
   long               ps_horiz_offset[4];
   long               ps_vert_offset[4];
   long               ps_width[4];
   long               ps_height[4];

   /* VC1_VALID_USERDATA
      User data is in the form of a linked list. */
   long               userDataSize;
   UD_HDR* POINTER_32 userData;

} PPB_VC1;



/*------------------------------------------------------*
 *    H.264 Extension to the PPB                        *
 *------------------------------------------------------*/
/**
 * @brief Film grain SEI message.
 *
 * Content of the film grain SEI message.
 */
//maximum number of model-values as for Thomson spec(standard says 5)
#define MAX_FGT_MODEL_VALUE         (3)
//maximum number of intervals(as many as 256 intervals?)
#define MAX_FGT_VALUE_INTERVAL      (256)

typedef struct FGT_SEI
{
    struct FGT_SEI* POINTER_32 next;
    unsigned char model_values[3][MAX_FGT_VALUE_INTERVAL][MAX_FGT_MODEL_VALUE];
    unsigned char upper_bound[3][MAX_FGT_VALUE_INTERVAL];
    unsigned char lower_bound[3][MAX_FGT_VALUE_INTERVAL];

    unsigned char cancel_flag;               /**< Cancel flag: 1 no film grain. */
    unsigned char model_id;                  /**< Model id. */

    //+unused SE based on Thomson spec
    unsigned char color_desc_flag;           /**< Separate color descrition flag. */
    unsigned char bit_depth_luma;            /**< Bit depth luma minus 8. */
    unsigned char bit_depth_chroma;          /**< Bit depth chroma minus 8. */
    unsigned char full_range_flag;           /**< Full range flag. */
    unsigned char color_primaries;           /**< Color primaries. */
    unsigned char transfer_charact;          /**< Transfer characteristics. */
    unsigned char matrix_coeff;              /**< Matrix coefficients. */
    //-unused SE based on Thomson spec

    unsigned char blending_mode_id;          /**< Blending mode. */
    unsigned char log2_scale_factor;         /**< Log2 scale factor (2-7). */
    unsigned char comp_flag[3];              /**< Components [0,2] parameters present flag. */
    unsigned char num_intervals_minus1[3];    /**< Number of intensity level intervals. */
    unsigned char num_model_values[3]; /**< Number of model values. */
    unsigned short repetition_period;        /**< Repetition period (0-16384) */
}FGT_SEI;

/* Bit definitions for H.264 user data type field
 */
#define  AVC_USERDATA_TYPE_REGISTERED   4
#define  AVC_USERDATA_TYPE_TOP          8
#define  AVC_USERDATA_TYPE_BOT          16


/* Bit definitions for 'other.h264.valid' field */
#define  H264_VALID_PANSCAN             (1)
#define  H264_VALID_SPS_CROP            (2)
#define  H264_VALID_VUI                 (4)
#define  H264_VALID_USER                (8)
#define  H264_VALID_CT_TYPE             (16)
#define  H264_USER_OVERFLOW             (32)
#define  H264_FILM_GRAIN_MSG            (64)

typedef struct
{
   /* 'valid' specifies which fields (or sets of
    * fields) below are valid.  If the corresponding
    * bit in 'valid' is NOT set then that field(s)
    * is (are) not initialized. */
   unsigned long      valid;

   long               poc_top;          /* POC for Top Field/Frame */
   long               poc_bottom;       /* POC for Bottom Field    */
   unsigned long      idr_pic_id;

   /* H264_VALID_PANSCAN */
   unsigned long      pan_scan_count;
   long               pan_scan_left  [3];
   long               pan_scan_right [3];
   long               pan_scan_top   [3];
   long               pan_scan_bottom[3];

   /* H264_VALID_CT_TYPE */
   unsigned long      ct_type_count;
   unsigned long      ct_type[3];

   /* H264_VALID_SPS_CROP */
   long               sps_crop_left;
   long               sps_crop_right;
   long               sps_crop_top;
   long               sps_crop_bottom;

   /* H264_VALID_VUI */
   unsigned long      chroma_top;
   unsigned long      chroma_bottom;

   /* H264_VALID_USER */
   unsigned long      user_data_size;
   UD_HDR* POINTER_32 user_data;

   /* H264 VALID FGT */
   FGT_SEI* POINTER_32 pfgt;

} PPB_H264;


/*------------------------------------------------------*
 *    Picture Parameter Block                           *
 *------------------------------------------------------*/

/* Bit definitions for 'flags' field */
#define  VDEC_FLAG_PTS_PRESENT          (0x0001)
#define  VDEC_FLAG_PTS_MSB              (0x0002)
#define  VDEC_FLAG_EOS                  (0x0004)

#define  VDEC_FLAG_FRAME                (0x0000)
#define  VDEC_FLAG_FIELDPAIR            (0x0008)
#define  VDEC_FLAG_TOPFIELD             (0x0010)
#define  VDEC_FLAG_BOTTOMFIELD          (0x0018)

#define  VDEC_FLAG_PROGRESSIVE_SRC      (0x0000)
#define  VDEC_FLAG_INTERLACED_SRC       (0x0020)
#define  VDEC_FLAG_UNKNOWN_SRC          (0x0040)

#define  VDEC_FLAG_BOTTOM_FIRST         (0x0080)
#define  VDEC_FLAG_LAST_PICTURE         (0x0100)

#define  VDEC_FLAG_STC_IS_CRAP          (0x0200)
#define  VDEC_FLAG_PCR_OFFSET_PRESENT   (0x0400)
#define  VDEC_FLAG_REF_CNTR_TYPE_PCR    (0x0800)
#define  VDEC_FLAG_DISCONT_PCR_OFFSET   (0x1000)
#define  VDEC_FLAG_PICTURE_TAG_VALID    (0x2000)

#define  VDEC_FLAG_PICTURE_DONE_MARKER_PRESENT    (0x20000)

#define  VDEC_FLAG_PICTURE_META_DATA_PRESENT      (0x40000)

#define  VDEC_FLAG_RESOLUTION_CHANGE	(0x80000)

/* Values for the 'chroma_format' field. */
enum
{
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
enum
{
   vdecNoPulldownInfo  = 0,
   vdecTop             = 1,
   vdecBottom          = 2,
   vdecTopBottom       = 3,
   vdecBottomTop       = 4,
   vdecTopBottomTop    = 5,
   vdecBottomTopBottom = 6,
   vdecFrame_X2        = 7,
   vdecFrame_X3        = 8,
   vdecFrame_X1        = 9,
   vdecFrame_X4        = 10,
};

/* Values for 'protocol' field. */
enum
{
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
enum
{
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
enum
{
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
enum
{
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
enum
{
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
enum
{
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

typedef struct
{
   /* Common fields. */
   unsigned long      picture_number;   /* Ordinal display number  */
   unsigned long      video_buffer;     /* Video (picbuf) number   */
   unsigned long      video_address;    /* Address of picbuf Y     */
   unsigned long      video_address_uv; /* Address of picbuf UV    */
   unsigned long      video_stripe;     /* Picbuf stripe     */
   unsigned long      video_width;      /* Picbuf width      */
   unsigned long      video_height;     /* Picbuf height     */

   unsigned long      channel_id;       /* Decoder channel ID      */
   unsigned long      status;           /* reserved          */
   unsigned long      width;            /* pixels            */
   unsigned long      height;           /* pixels            */
   unsigned long      chroma_format;    /* see above         */
   unsigned long      pulldown;         /* see above         */
   unsigned long      flags;            /* see above         */
   unsigned long      pts;              /* 32 LSBs of PTS    */
   unsigned long      protocol;         /* protocolXXX (above)     */

   unsigned long      frame_rate;       /* see above         */
   unsigned long      matrix_coeff;     /* see above         */
   unsigned long      aspect_ratio;     /* see above         */
   unsigned long      colour_primaries; /* see above         */
   unsigned long      transfer_char;    /* see above         */
   unsigned long      pcr_offset;       /* 45kHz if PCR type; else 27MHz */
   unsigned long      n_drop;           /* Number of pictures to be dropped */

   unsigned long      custom_aspect_ratio_width_height; /* upper 16-bits is Y and lower 16-bits is X */

   unsigned long      picture_tag;      /* Indexing tag from BUD packets */
   unsigned long      picture_done_payload;
   unsigned long      picture_meta_payload;
   unsigned long      reserved[1];

   /* Protocol-specific extensions. */
   union
   {
      PPB_H264        h264;
      PPB_MPEG        mpeg;
      PPB_VC1         vc1;
   } other;
} PPB;


typedef struct
{
    /* Approximate number of pictures in
    * the CPB */
    unsigned long     codein_picture_count;

    /* GOP time code */
    unsigned long    time_code_hours;
    unsigned long    time_code_minutes;
    unsigned long    time_code_seconds;
    unsigned long    time_code_pictures;

    unsigned long    pic_done_marker_btp_payload;
    
    unsigned long    start_decode_time;
    unsigned long    chan_open_time;
    unsigned long    chan_close_start_time;
    unsigned long    chan_close_end_time;
    unsigned long    first_pic_delivery_time;
    unsigned long    first_code_in_packet_time;
    unsigned long    ts_fifo_byte_non_zero_time;
    unsigned long    ts_video_packet_count_non_zero_time;
} VdecStatusBlock;


#if defined(CHIP_7401A)

/* Display Manager: STC and Parity Information in DRAM for host to use */
typedef struct
{
    unsigned long stc_snapshot;
    unsigned long vsync_parity;
    unsigned long vsync_count; /* used only for debug */
} DisplayInfo;

/* Parameters passed from Display Manager (host) to DMS */
typedef struct
{
   unsigned long write_offset;
   unsigned long drop_count;
} DMS_Info;

/* Picture Delivery parameters required by Display Manager */
typedef struct
{
    unsigned long queue_read_offset; /* offset is w.r.t base of this data struct so value of 0-1 prohibited */
    unsigned long queue_write_offset; /* offset is w.r.t base of this data struct so value of 0-1 prohibited */
    /* queue if full if (write_offset+1 == read_offset) */
    /* write_offset modified by firmware and read_offset modified by Display Manager in host */
    PPB* POINTER_32 display_elements[62];
} PictureDeliveryQueue;

/* Picture Release parameters returned by Display Manager to firmware */
typedef struct
{
    unsigned long queue_read_offset; /* offset is w.r.t base of this data struct so value of 0-1 prohibited */
    unsigned long queue_write_offset; /* offset is w.r.t base of this data struct so value of 0-1 prohibited */
    /* queue if full if (write_offset+1 == read_offset) */
    /* read_offset modified by firmware and write_offset modified by Display Manager in host */
    PPB* POINTER_32 display_elements[62];
} PictureReleaseQueue;
#endif

#pragma pack ()

#endif // __INC_VDEC_INFO_H__
