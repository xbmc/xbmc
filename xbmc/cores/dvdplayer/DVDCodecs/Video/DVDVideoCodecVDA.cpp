/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif

#if defined(HAVE_LIBVDADECODER)
#include "DynamicDll.h"
#include "settings/GUISettings.h"
#include "DVDClock.h"
#include "DVDStreamInfo.h"
#include "cores/dvdplayer/DVDCodecs/DVDCodecUtils.h"
#include "DVDVideoCodecVDA.h"
#include "DllAvFormat.h"
#include "DllSwScale.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"
#include "osx/CocoaInterface.h"
#include <CoreFoundation/CoreFoundation.h>

/*
 * if extradata size is greater than 7, then have a valid quicktime 
 * avcC atom header.
 *
 *      -: avcC atom header :-
 *  -----------------------------------
 *  1 byte  - version
 *  1 byte  - h.264 stream profile
 *  1 byte  - h.264 compatible profiles
 *  1 byte  - h.264 stream level
 *  6 bits  - reserved set to 63
 *  2 bits  - NAL length 
 *            ( 0 - 1 byte; 1 - 2 bytes; 3 - 4 bytes)
 *  3 bit   - reserved
 *  5 bits  - number of SPS 
 *  for (i=0; i < number of SPS; i++) {
 *      2 bytes - SPS length
 *      SPS length bytes - SPS NAL unit
 *  }
 *  1 byte  - number of PPS
 *  for (i=0; i < number of PPS; i++) {
 *      2 bytes - PPS length 
 *      PPS length bytes - PPS NAL unit 
 *  }
 
 how to detect the interlacing used on an existing stream:
- progressive is signalled by setting
   frame_mbs_only_flag: 1 in the SPS.
- interlaced is signalled by setting
   frame_mbs_only_flag: 0 in the SPS and
   field_pic_flag: 1 on all frames.
- paff is signalled by setting
   frame_mbs_only_flag: 0 in the SPS and
   field_pic_flag: 1 on all frames that get interlaced and
   field_pic_flag: 0 on all frames that get progressive.
- mbaff is signalled by setting
   frame_mbs_only_flag: 0 in the SPS and
   mb_adaptive_frame_field_flag: 1 in the SPS and
   field_pic_flag: 0 on the frames,
   (field_pic_flag: 1 would indicate a normal interlaced frame).
*/

// missing in 10.4/10.5 SDKs.
#if (MAC_OS_X_VERSION_MAX_ALLOWED < 1060)
#include "dlfcn.h"
enum {
  // component Y'CbCr 8-bit 4:2:2, ordered Cb Y'0 Cr Y'1 .
  kCVPixelFormatType_422YpCbCr8 = FourCharCode('2vuy'),
  kCVPixelFormatType_32BGRA = FourCharCode('BGRA')
};
#endif

////////////////////////////////////////////////////////////////////////////////////////////
// http://developer.apple.com/mac/library/technotes/tn2010/tn2267.html
// VDADecoder API (keep this until VDADecoder.h is public).
// #include <VideoDecodeAcceleration/VDADecoder.h>
enum {
  kVDADecoderNoErr = 0,
  kVDADecoderHardwareNotSupportedErr = -12470,
  kVDADecoderFormatNotSupportedErr = -12471,
  kVDADecoderConfigurationError = -12472,
  kVDADecoderDecoderFailedErr = -12473,
};
enum {
  kVDADecodeInfo_Asynchronous = 1UL << 0,
  kVDADecodeInfo_FrameDropped = 1UL << 1
};
enum {
  // tells the decoder not to bother returning a CVPixelBuffer
  // in the outputCallback. The output callback will still be called.
  kVDADecoderDecodeFlags_DontEmitFrame = 1 << 0
};
enum {
  // decode and return buffers for all frames currently in flight.
  kVDADecoderFlush_EmitFrames = 1 << 0		
};

typedef struct OpaqueVDADecoder* VDADecoder;

typedef void (*VDADecoderOutputCallback)(
  void *decompressionOutputRefCon,
  CFDictionaryRef frameInfo,
  OSStatus status,
  uint32_t infoFlags,
  CVImageBufferRef imageBuffer);

////////////////////////////////////////////////////////////////////////////////////////////
class DllLibVDADecoderInterface
{
public:
  virtual ~DllLibVDADecoderInterface() {}
  virtual OSStatus VDADecoderCreate(
    CFDictionaryRef decoderConfiguration, CFDictionaryRef destinationImageBufferAttributes,
    VDADecoderOutputCallback *outputCallback, void *decoderOutputCallbackRefcon, VDADecoder *decoderOut) = 0;
  virtual OSStatus VDADecoderDecode(
    VDADecoder decoder, uint32_t decodeFlags, CFTypeRef compressedBuffer, CFDictionaryRef frameInfo) = 0;
  virtual OSStatus VDADecoderFlush(VDADecoder decoder, uint32_t flushFlags) = 0;
  virtual OSStatus VDADecoderDestroy(VDADecoder decoder) = 0;
  virtual CFStringRef Get_kVDADecoderConfiguration_Height() = 0;
  virtual CFStringRef Get_kVDADecoderConfiguration_Width() = 0;
  virtual CFStringRef Get_kVDADecoderConfiguration_SourceFormat() = 0;
  virtual CFStringRef Get_kVDADecoderConfiguration_avcCData() = 0;
};

class DllLibVDADecoder : public DllDynamic, DllLibVDADecoderInterface
{
  DECLARE_DLL_WRAPPER(DllLibVDADecoder, DLL_PATH_LIBVDADECODER)

  DEFINE_METHOD5(OSStatus, VDADecoderCreate, (CFDictionaryRef p1, CFDictionaryRef p2, VDADecoderOutputCallback* p3, void* p4, VDADecoder* p5))
  DEFINE_METHOD4(OSStatus, VDADecoderDecode, (VDADecoder p1, uint32_t p2, CFTypeRef p3, CFDictionaryRef p4))
  DEFINE_METHOD2(OSStatus, VDADecoderFlush, (VDADecoder p1, uint32_t p2))
  DEFINE_METHOD1(OSStatus, VDADecoderDestroy, (VDADecoder p1))
  DEFINE_GLOBAL(CFStringRef, kVDADecoderConfiguration_Height)
  DEFINE_GLOBAL(CFStringRef, kVDADecoderConfiguration_Width)
  DEFINE_GLOBAL(CFStringRef, kVDADecoderConfiguration_SourceFormat)
  DEFINE_GLOBAL(CFStringRef, kVDADecoderConfiguration_avcCData)
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(VDADecoderCreate)
    RESOLVE_METHOD(VDADecoderDecode)
    RESOLVE_METHOD(VDADecoderFlush)
    RESOLVE_METHOD(VDADecoderDestroy)
    RESOLVE_METHOD(kVDADecoderConfiguration_Height)
    RESOLVE_METHOD(kVDADecoderConfiguration_Width)
    RESOLVE_METHOD(kVDADecoderConfiguration_SourceFormat)
    RESOLVE_METHOD(kVDADecoderConfiguration_avcCData)
  END_METHOD_RESOLVE()
};

////////////////////////////////////////////////////////////////////////////////////////////
// helper function that wraps dts/pts into a dictionary
static CFDictionaryRef CreateDictionaryWithDisplayTime(double time, double dts, double pts)
{
  CFStringRef key[3] = {
    CFSTR("VideoDisplay_TIME"),
    CFSTR("VideoDisplay_DTS"),
    CFSTR("VideoDisplay_PTS")};
  CFNumberRef value[3];
  CFDictionaryRef display_time;

  value[0] = CFNumberCreate(kCFAllocatorDefault, kCFNumberDoubleType, &time);
  value[1] = CFNumberCreate(kCFAllocatorDefault, kCFNumberDoubleType, &dts);
  value[2] = CFNumberCreate(kCFAllocatorDefault, kCFNumberDoubleType, &pts);

  display_time = CFDictionaryCreate(
    kCFAllocatorDefault, (const void **)&key, (const void **)&value, 3,
    &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
  
  CFRelease(value[0]);
  CFRelease(value[1]);
  CFRelease(value[2]);

  return display_time;
}

// helper function to extract dts/pts from a dictionary
static void GetFrameDisplayTimeFromDictionary(
  CFDictionaryRef inFrameInfoDictionary, frame_queue *frame)
{
  // default to DVD_NOPTS_VALUE
  frame->sort_time = -1.0;
  frame->dts = DVD_NOPTS_VALUE;
  frame->pts = DVD_NOPTS_VALUE;
  if (inFrameInfoDictionary == NULL)
    return;

  CFNumberRef value[3];
  //
  value[0] = (CFNumberRef)CFDictionaryGetValue(inFrameInfoDictionary, CFSTR("VideoDisplay_TIME"));
  if (value[0])
    CFNumberGetValue(value[0], kCFNumberDoubleType, &frame->sort_time);
  value[1] = (CFNumberRef)CFDictionaryGetValue(inFrameInfoDictionary, CFSTR("VideoDisplay_DTS"));
  if (value[1])
    CFNumberGetValue(value[1], kCFNumberDoubleType, &frame->dts);
  value[2] = (CFNumberRef)CFDictionaryGetValue(inFrameInfoDictionary, CFSTR("VideoDisplay_PTS"));
  if (value[2])
    CFNumberGetValue(value[2], kCFNumberDoubleType, &frame->pts);

  return;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
// GStreamer h264 parser
// Copyright (C) 2005 Michal Benes <michal.benes@itonis.tv>
//           (C) 2008 Wim Taymans <wim.taymans@gmail.com>
// gsth264parse.c:
//  * License as published by the Free Software Foundation; either
//  * version 2.1 of the License, or (at your option) any later version.
typedef struct
{
  const uint8_t *data;
  const uint8_t *end;
  int head;
  uint64_t cache;
} nal_bitstream;

static void
nal_bs_init(nal_bitstream *bs, const uint8_t *data, size_t size)
{
  bs->data = data;
  bs->end  = data + size;
  bs->head = 0;
  // fill with something other than 0 to detect
  //  emulation prevention bytes
  bs->cache = 0xffffffff;
}

static uint32_t
nal_bs_read(nal_bitstream *bs, int n)
{
  uint32_t res = 0;
  int shift;

  if (n == 0)
    return res;

  // fill up the cache if we need to
  while (bs->head < n)
  {
    uint8_t a_byte;
    bool check_three_byte;

    check_three_byte = TRUE;
next_byte:
    if (bs->data >= bs->end)
    {
      // we're at the end, can't produce more than head number of bits
      n = bs->head;
      break;
    }
    // get the byte, this can be an emulation_prevention_three_byte that we need
    // to ignore.
    a_byte = *bs->data++;
    if (check_three_byte && a_byte == 0x03 && ((bs->cache & 0xffff) == 0))
    {
      // next byte goes unconditionally to the cache, even if it's 0x03
      check_three_byte = FALSE;
      goto next_byte;
    }
    // shift bytes in cache, moving the head bits of the cache left
    bs->cache = (bs->cache << 8) | a_byte;
    bs->head += 8;
  }

  // bring the required bits down and truncate
  if ((shift = bs->head - n) > 0)
    res = bs->cache >> shift;
  else
    res = bs->cache;

  // mask out required bits
  if (n < 32)
    res &= (1 << n) - 1;

  bs->head = shift;

  return res;
}

static bool
nal_bs_eos(nal_bitstream *bs)
{
  return (bs->data >= bs->end) && (bs->head == 0);
}

// read unsigned Exp-Golomb code
static int
nal_bs_read_ue(nal_bitstream *bs)
{
  int i = 0;

  while (nal_bs_read(bs, 1) == 0 && !nal_bs_eos(bs) && i < 32)
    i++;

  return ((1 << i) - 1 + nal_bs_read(bs, i));
}

typedef struct
{
  int profile_idc;
  int level_idc;
  int sps_id;

  int chroma_format_idc;
  int separate_colour_plane_flag;
  int bit_depth_luma_minus8;
  int bit_depth_chroma_minus8;
  int qpprime_y_zero_transform_bypass_flag;
  int seq_scaling_matrix_present_flag;

  int log2_max_frame_num_minus4;
  int pic_order_cnt_type;
  int log2_max_pic_order_cnt_lsb_minus4;

  int max_num_ref_frames;
  int gaps_in_frame_num_value_allowed_flag;
  int pic_width_in_mbs_minus1;
  int pic_height_in_map_units_minus1;

  int frame_mbs_only_flag;
  int mb_adaptive_frame_field_flag;

  int direct_8x8_inference_flag;

  int frame_cropping_flag;
  int frame_crop_left_offset;
  int frame_crop_right_offset;
  int frame_crop_top_offset;
  int frame_crop_bottom_offset;
} sps_info_struct;

static void
parseh264_sps(uint8_t *sps, uint32_t sps_size, bool *interlaced, int32_t *max_ref_frames)
{
  nal_bitstream bs;
  sps_info_struct sps_info;

  nal_bs_init(&bs, sps, sps_size);

  sps_info.profile_idc  = nal_bs_read(&bs, 8);
  nal_bs_read(&bs, 1);  // constraint_set0_flag
  nal_bs_read(&bs, 1);  // constraint_set1_flag
  nal_bs_read(&bs, 1);  // constraint_set2_flag
  nal_bs_read(&bs, 1);  // constraint_set3_flag
  nal_bs_read(&bs, 4);  // reserved
  sps_info.level_idc    = nal_bs_read(&bs, 8);
  sps_info.sps_id       = nal_bs_read_ue(&bs);

  if (sps_info.profile_idc == 100 ||
      sps_info.profile_idc == 110 ||
      sps_info.profile_idc == 122 ||
      sps_info.profile_idc == 244 ||
      sps_info.profile_idc == 44  ||
      sps_info.profile_idc == 83  ||
      sps_info.profile_idc == 86)
  {
    sps_info.chroma_format_idc                    = nal_bs_read_ue(&bs);
    if (sps_info.chroma_format_idc == 3)
      sps_info.separate_colour_plane_flag         = nal_bs_read(&bs, 1);
    sps_info.bit_depth_luma_minus8                = nal_bs_read_ue(&bs);
    sps_info.bit_depth_chroma_minus8              = nal_bs_read_ue(&bs);
    sps_info.qpprime_y_zero_transform_bypass_flag = nal_bs_read(&bs, 1);

    sps_info.seq_scaling_matrix_present_flag = nal_bs_read (&bs, 1);
    if (sps_info.seq_scaling_matrix_present_flag)
    {
      /* TODO: unfinished */
    }
  }
  sps_info.log2_max_frame_num_minus4 = nal_bs_read_ue(&bs);
  if (sps_info.log2_max_frame_num_minus4 > 12)
  { // must be between 0 and 12
    return;
  }

  sps_info.pic_order_cnt_type = nal_bs_read_ue(&bs);
  if (sps_info.pic_order_cnt_type == 0)
  {
    sps_info.log2_max_pic_order_cnt_lsb_minus4 = nal_bs_read_ue(&bs);
  }
  else if (sps_info.pic_order_cnt_type == 1)
  { // TODO: unfinished
    /*
    delta_pic_order_always_zero_flag = gst_nal_bs_read (bs, 1);
    offset_for_non_ref_pic = gst_nal_bs_read_se (bs);
    offset_for_top_to_bottom_field = gst_nal_bs_read_se (bs);

    num_ref_frames_in_pic_order_cnt_cycle = gst_nal_bs_read_ue (bs);
    for( i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++ )
    offset_for_ref_frame[i] = gst_nal_bs_read_se (bs);
    */
  }

  sps_info.max_num_ref_frames             = nal_bs_read_ue(&bs);
  sps_info.gaps_in_frame_num_value_allowed_flag = nal_bs_read(&bs, 1);
  sps_info.pic_width_in_mbs_minus1        = nal_bs_read_ue(&bs);
  sps_info.pic_height_in_map_units_minus1 = nal_bs_read_ue(&bs);

  sps_info.frame_mbs_only_flag            = nal_bs_read(&bs, 1);
  if (!sps_info.frame_mbs_only_flag)
    sps_info.mb_adaptive_frame_field_flag = nal_bs_read(&bs, 1);

  sps_info.direct_8x8_inference_flag      = nal_bs_read(&bs, 1);

  sps_info.frame_cropping_flag            = nal_bs_read(&bs, 1);
  if (sps_info.frame_cropping_flag)
  {
    sps_info.frame_crop_left_offset       = nal_bs_read_ue(&bs);
    sps_info.frame_crop_right_offset      = nal_bs_read_ue(&bs);
    sps_info.frame_crop_top_offset        = nal_bs_read_ue(&bs);
    sps_info.frame_crop_bottom_offset     = nal_bs_read_ue(&bs);
  }

  *interlaced = !sps_info.frame_mbs_only_flag;
  *max_ref_frames = sps_info.max_num_ref_frames;
}

////////////////////////////////////////////////////////////////////////////////////////////
// TODO: refactor this so as not to need these ffmpeg routines.
// These are not exposed in ffmpeg's API so we dupe them here.
// AVC helper functions for muxers,
//  * Copyright (c) 2006 Baptiste Coudurier <baptiste.coudurier@smartjog.com>
// This is part of FFmpeg
//  * License as published by the Free Software Foundation; either
//  * version 2.1 of the License, or (at your option) any later version.
#define VDA_RB16(x)                          \
  ((((const uint8_t*)(x))[0] <<  8) |        \
   ((const uint8_t*)(x)) [1])

#define VDA_RB24(x)                          \
  ((((const uint8_t*)(x))[0] << 16) |        \
   (((const uint8_t*)(x))[1] <<  8) |        \
   ((const uint8_t*)(x))[2])

#define VDA_RB32(x)                          \
  ((((const uint8_t*)(x))[0] << 24) |        \
   (((const uint8_t*)(x))[1] << 16) |        \
   (((const uint8_t*)(x))[2] <<  8) |        \
   ((const uint8_t*)(x))[3])

#define VDA_WB32(p, d) { \
  ((uint8_t*)(p))[3] = (d); \
  ((uint8_t*)(p))[2] = (d) >> 8; \
  ((uint8_t*)(p))[1] = (d) >> 16; \
  ((uint8_t*)(p))[0] = (d) >> 24; }

static const uint8_t *avc_find_startcode_internal(const uint8_t *p, const uint8_t *end)
{
  const uint8_t *a = p + 4 - ((intptr_t)p & 3);

  for (end -= 3; p < a && p < end; p++)
  {
    if (p[0] == 0 && p[1] == 0 && p[2] == 1)
      return p;
  }

  for (end -= 3; p < end; p += 4)
  {
    uint32_t x = *(const uint32_t*)p;
    if ((x - 0x01010101) & (~x) & 0x80808080) // generic
    {
      if (p[1] == 0)
      {
        if (p[0] == 0 && p[2] == 1)
          return p;
        if (p[2] == 0 && p[3] == 1)
          return p+1;
      }
      if (p[3] == 0)
      {
        if (p[2] == 0 && p[4] == 1)
          return p+2;
        if (p[4] == 0 && p[5] == 1)
          return p+3;
      }
    }
  }

  for (end += 3; p < end; p++)
  {
    if (p[0] == 0 && p[1] == 0 && p[2] == 1)
      return p;
  }

  return end + 3;
}

const uint8_t *avc_find_startcode(const uint8_t *p, const uint8_t *end)
{
  const uint8_t *out= avc_find_startcode_internal(p, end);
  if (p<out && out<end && !out[-1])
    out--;
  return out;
}

const int avc_parse_nal_units(DllAvFormat *av_format_ctx,
  ByteIOContext *pb, const uint8_t *buf_in, int size)
{
  const uint8_t *p = buf_in;
  const uint8_t *end = p + size;
  const uint8_t *nal_start, *nal_end;

  size = 0;
  nal_start = avc_find_startcode(p, end);
  while (nal_start < end)
  {
    while (!*(nal_start++));
    nal_end = avc_find_startcode(nal_start, end);
    av_format_ctx->put_be32(pb, nal_end - nal_start);
    av_format_ctx->put_buffer(pb, nal_start, nal_end - nal_start);
    size += 4 + nal_end - nal_start;
    nal_start = nal_end;
  }
  return size;
}

const int avc_parse_nal_units_buf(DllAvUtil *av_util_ctx, DllAvFormat *av_format_ctx,
  const uint8_t *buf_in, uint8_t **buf, int *size)
{
  ByteIOContext *pb;
  int ret = av_format_ctx->url_open_dyn_buf(&pb);
  if (ret < 0)
    return ret;

  avc_parse_nal_units(av_format_ctx, pb, buf_in, *size);

  av_util_ctx->av_freep(buf);
  *size = av_format_ctx->url_close_dyn_buf(pb, buf);
  return 0;
}

const int isom_write_avcc(DllAvUtil *av_util_ctx, DllAvFormat *av_format_ctx,
  ByteIOContext *pb, const uint8_t *data, int len)
{
  // extradata from bytestream h264, convert to avcC atom data for bitstream
  if (len > 6)
  {
    /* check for h264 start code */
    if (VDA_RB32(data) == 0x00000001 || VDA_RB24(data) == 0x000001)
    {
      uint8_t *buf=NULL, *end, *start;
      uint32_t sps_size=0, pps_size=0;
      uint8_t *sps=0, *pps=0;

      int ret = avc_parse_nal_units_buf(av_util_ctx, av_format_ctx, data, &buf, &len);
      if (ret < 0)
        return ret;
      start = buf;
      end = buf + len;

      /* look for sps and pps */
      while (buf < end)
      {
        unsigned int size;
        uint8_t nal_type;
        size = VDA_RB32(buf);
        nal_type = buf[4] & 0x1f;
        if (nal_type == 7) /* SPS */
        {
          sps = buf + 4;
          sps_size = size;
          
          //parse_sps(sps+1, sps_size-1);
        }
        else if (nal_type == 8) /* PPS */
        {
          pps = buf + 4;
          pps_size = size;
        }
        buf += size + 4;
      }
      assert(sps);

      av_format_ctx->put_byte(pb, 1); /* version */
      av_format_ctx->put_byte(pb, sps[1]); /* profile */
      av_format_ctx->put_byte(pb, sps[2]); /* profile compat */
      av_format_ctx->put_byte(pb, sps[3]); /* level */
      av_format_ctx->put_byte(pb, 0xff); /* 6 bits reserved (111111) + 2 bits nal size length - 1 (11) */
      av_format_ctx->put_byte(pb, 0xe1); /* 3 bits reserved (111) + 5 bits number of sps (00001) */

      av_format_ctx->put_be16(pb, sps_size);
      av_format_ctx->put_buffer(pb, sps, sps_size);
      if (pps)
      {
        av_format_ctx->put_byte(pb, 1); /* number of pps */
        av_format_ctx->put_be16(pb, pps_size);
        av_format_ctx->put_buffer(pb, pps, pps_size);
      }
      av_util_ctx->av_free(start);
    }
    else
    {
      av_format_ctx->put_buffer(pb, data, len);
    }
  }
  return 0;
}


static DllLibVDADecoder *g_DllLibVDADecoder = (DllLibVDADecoder*)-1;
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
CDVDVideoCodecVDA::CDVDVideoCodecVDA() : CDVDVideoCodec()
{
  if (g_DllLibVDADecoder == (DllLibVDADecoder*)-1)
  {
    m_dll = new DllLibVDADecoder;
    m_dll->Load();
  }
  else
    m_dll = g_DllLibVDADecoder;

  m_vda_decoder = NULL;
  m_pFormatName = "vda-";

  m_queue_depth = 0;
  m_display_queue = NULL;
  pthread_mutex_init(&m_queue_mutex, NULL);

  m_convert_bytestream = false;
  m_convert_3byteTo4byteNALSize = false;
  m_dllAvUtil = NULL;
  m_dllAvFormat = NULL;
  m_dllSwScale = NULL;
  memset(&m_videobuffer, 0, sizeof(DVDVideoPicture));
}

CDVDVideoCodecVDA::~CDVDVideoCodecVDA()
{
  Dispose();
  pthread_mutex_destroy(&m_queue_mutex);
  //delete m_dll;
}

bool CDVDVideoCodecVDA::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  if (g_guiSettings.GetBool("videoplayer.usevda") && !hints.software)
  {
    CCocoaAutoPool pool;
    int32_t width, height, profile, level;
    CFDataRef avcCData;
    uint8_t *extradata; // extra data for codec to use
    unsigned int extrasize; // size of extra data

    //
    width  = hints.width;
    height = hints.height;
    level  = hints.level;
    profile = hints.profile;
    extrasize = hints.extrasize;
    extradata = (uint8_t*)hints.extradata;
    
    if (width <= 0 || height <= 0 || profile <= 0 || level <= 0)
    {
      CLog::Log(LOGNOTICE, "%s - bailing with bogus hints, width(%d), height(%d), profile(%d), level(%d)",
        __FUNCTION__, width, height, profile, level);
      return false;
    }
    
    if (Cocoa_GPUForDisplayIsNvidiaPureVideo3() && !CDVDCodecUtils::IsVP3CompatibleWidth(width))
    {
      CLog::Log(LOGNOTICE, "%s - Nvidia 9400 GPU hardware limitation, cannot decode a width of %d", __FUNCTION__, width);
      return false;
    }

    switch (hints.codec)
    {
      case CODEC_ID_H264:
        // source must be H.264 with valid avcC atom data in extradata
        if (extrasize < 7 || extradata == NULL)
        {
          CLog::Log(LOGNOTICE, "%s - avcC atom too data small or missing", __FUNCTION__);
          return false;
        }
        // valid avcC atom data always starts with the value 1 (version)
        if ( *extradata != 1 )
        {
          if (extradata[0] == 0 && extradata[1] == 0 && extradata[2] == 0 && extradata[3] == 1)
          {
            // video content is from x264 or from bytestream h264 (AnnexB format)
            // NAL reformating to bitstream format needed
            m_dllAvUtil = new DllAvUtil;
            m_dllAvFormat = new DllAvFormat;
            if (!m_dllAvUtil->Load() || !m_dllAvFormat->Load())
            {
              return false;
            }

            ByteIOContext *pb;
            if (m_dllAvFormat->url_open_dyn_buf(&pb) < 0)
            {
              return false;
            }

            m_convert_bytestream = true;
            // create a valid avcC atom data from ffmpeg's extradata
            isom_write_avcc(m_dllAvUtil, m_dllAvFormat, pb, extradata, extrasize);
            // unhook from ffmpeg's extradata
            extradata = NULL;
            // extract the avcC atom data into extradata then write it into avcCData for VDADecoder
            extrasize = m_dllAvFormat->url_close_dyn_buf(pb, &extradata);
            // CFDataCreate makes a copy of extradata contents
            avcCData = CFDataCreate(kCFAllocatorDefault, (const uint8_t*)extradata, extrasize);
            // done with the converted extradata, we MUST free using av_free
            m_dllAvUtil->av_free(extradata);
          }
          else
          {
            CLog::Log(LOGNOTICE, "%s - invalid avcC atom data", __FUNCTION__);
            return false;
          }
        }
        else
        {
          if (extradata[4] == 0xFE)
          {
            // video content is from so silly encoder that think 3 byte NAL sizes
            // are valid, setup to convert 3 byte NAL sizes to 4 byte.
            m_dllAvUtil = new DllAvUtil;
            m_dllAvFormat = new DllAvFormat;
            if (!m_dllAvUtil->Load() || !m_dllAvFormat->Load())
              return false;

            extradata[4] = 0xFF;
            m_convert_3byteTo4byteNALSize = true;
          }
          // CFDataCreate makes a copy of extradata contents
          avcCData = CFDataCreate(kCFAllocatorDefault, (const uint8_t*)extradata, extrasize);
        }

        m_format = 'avc1';
        m_pFormatName = "vda-h264";
      break;
      default:
        return false;
      break;
    }

    // check the avcC atom's sps for number of reference frames and
    // bail if interlaced, VDA does not handle interlaced h264.
    uint32_t avcc_len = CFDataGetLength(avcCData);
    if (avcc_len < 8)
    {
      // avcc atoms with length less than 8 are borked.
      CFRelease(avcCData);
      return false;
    }
    else
    {
      bool interlaced = true;
      uint8_t *spc = (uint8_t*)CFDataGetBytePtr(avcCData) + 6;
      uint32_t sps_size = VDA_RB16(spc);
      if (sps_size)
        parseh264_sps(spc+3, sps_size-1, &interlaced, &m_max_ref_frames);
      if (interlaced)
      {
        CLog::Log(LOGNOTICE, "%s - possible interlaced content.", __FUNCTION__);
        CFRelease(avcCData);
        return false;
      }
      if (m_max_ref_frames == 0)
        m_max_ref_frames = 2;
    }

    if (hints.profile == 77 && hints.level == 32 && (m_max_ref_frames > 4))
    {
      // Main@L3.2, VDA cannot handle greater than 4 reference frames
      CLog::Log(LOGNOTICE, "%s - Main@L3.2 detected, VDA cannot decode.", __FUNCTION__);
      return false;
    }
 
    // input stream is qualified, now we can load dlls.
    m_dllSwScale = new DllSwScale;
    if (!m_dllSwScale->Load())
    {
      CFRelease(avcCData);
      return false;
    }

    // setup the decoder configuration dict
    CFMutableDictionaryRef decoderConfiguration = CFDictionaryCreateMutable(
      kCFAllocatorDefault, 4, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

    CFNumberRef avcWidth  = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &width);
    CFNumberRef avcHeight = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &height);
    CFNumberRef avcFormat = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &m_format);

    CFDictionarySetValue(decoderConfiguration, m_dll->Get_kVDADecoderConfiguration_Height(), avcHeight);
    CFDictionarySetValue(decoderConfiguration, m_dll->Get_kVDADecoderConfiguration_Width(),  avcWidth);
    CFDictionarySetValue(decoderConfiguration, m_dll->Get_kVDADecoderConfiguration_SourceFormat(), avcFormat);
    CFDictionarySetValue(decoderConfiguration, m_dll->Get_kVDADecoderConfiguration_avcCData(), avcCData);

    // release the retained object refs, decoderConfiguration owns them now
    CFRelease(avcWidth);
    CFRelease(avcHeight);
    CFRelease(avcFormat);
    CFRelease(avcCData);

    // setup the destination image buffer dict, vda will output this pict format
    CFMutableDictionaryRef destinationImageBufferAttributes = CFDictionaryCreateMutable(
      kCFAllocatorDefault, 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

    OSType cvPixelFormatType = kCVPixelFormatType_422YpCbCr8;
    CFNumberRef pixelFormat  = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &cvPixelFormatType);
    CFDictionarySetValue(destinationImageBufferAttributes, kCVPixelBufferPixelFormatTypeKey, pixelFormat);
    // release the retained object refs, destinationImageBufferAttributes owns it now
    CFRelease(pixelFormat);

    // create the VDADecoder object
    OSStatus status;
    try
    {
      status = m_dll->VDADecoderCreate(decoderConfiguration, destinationImageBufferAttributes,
        (VDADecoderOutputCallback *)VDADecoderCallback, this, (VDADecoder*)&m_vda_decoder);
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "%s - exception",__FUNCTION__);
      status = kVDADecoderDecoderFailedErr;
    }
    CFRelease(decoderConfiguration);
    CFRelease(destinationImageBufferAttributes);
    if (status != kVDADecoderNoErr)
    {
	  if (status == kVDADecoderDecoderFailedErr)
        CLog::Log(LOGNOTICE, "%s - VDADecoder Codec failed to open, currently in use by another process",
          __FUNCTION__);
	  else
        CLog::Log(LOGNOTICE, "%s - VDADecoder Codec failed to open, status(%d), profile(%d), level(%d)",
          __FUNCTION__, (int)status, profile, level);
      return false;
    }

    // allocate a YV12 DVDVideoPicture buffer.
    // first make sure all properties are reset.
    memset(&m_videobuffer, 0, sizeof(DVDVideoPicture));
    unsigned int iPixels = width * height;
    unsigned int iChromaPixels = iPixels/4;

    m_videobuffer.pts = DVD_NOPTS_VALUE;
    m_videobuffer.iFlags = DVP_FLAG_ALLOCATED;
    m_videobuffer.format = DVDVideoPicture::FMT_YUV420P;
    m_videobuffer.color_range  = 0;
    m_videobuffer.color_matrix = 4;
    m_videobuffer.iWidth  = width;
    m_videobuffer.iHeight = height;
    m_videobuffer.iDisplayWidth  = width;
    m_videobuffer.iDisplayHeight = height;

    m_videobuffer.iLineSize[0] = width;   //Y
    m_videobuffer.iLineSize[1] = width/2; //U
    m_videobuffer.iLineSize[2] = width/2; //V
    m_videobuffer.iLineSize[3] = 0;

    m_videobuffer.data[0] = (BYTE*)malloc(iPixels);       //Y
    m_videobuffer.data[1] = (BYTE*)malloc(iChromaPixels); //U
    m_videobuffer.data[2] = (BYTE*)malloc(iChromaPixels); //V
    m_videobuffer.data[3] = NULL;

    // set all data to 0 for less artifacts.. hmm.. what is black in YUV??
    memset(m_videobuffer.data[0], 0, iPixels);
    memset(m_videobuffer.data[1], 0, iChromaPixels);
    memset(m_videobuffer.data[2], 0, iChromaPixels);

    m_DropPictures = false;
    m_max_ref_frames = std::min(m_max_ref_frames, 5);
    m_sort_time_offset = (CurrentHostCounter() * 1000.0) / CurrentHostFrequency();

    return true;
  }

  return false;
}

void CDVDVideoCodecVDA::Dispose()
{
  CCocoaAutoPool pool;
  if (m_vda_decoder)
  {
    m_dll->VDADecoderDestroy((VDADecoder)m_vda_decoder);
    m_vda_decoder = NULL;
  }
  if (m_videobuffer.iFlags & DVP_FLAG_ALLOCATED)
  {
    free(m_videobuffer.data[0]);
    free(m_videobuffer.data[1]);
    free(m_videobuffer.data[2]);
    m_videobuffer.iFlags = 0;
  }
  if (m_dllAvUtil)
  {
    delete m_dllAvUtil;
    m_dllAvUtil = NULL;
  }
  if (m_dllAvFormat)
  {
    delete m_dllAvFormat;
    m_dllAvFormat = NULL;
  }
  if (m_dllSwScale)
  {
    delete m_dllSwScale;
    m_dllSwScale = NULL;
  }
}

void CDVDVideoCodecVDA::SetDropState(bool bDrop)
{
  m_DropPictures = bDrop;
}

int CDVDVideoCodecVDA::Decode(BYTE* pData, int iSize, double dts, double pts)
{
  CCocoaAutoPool pool;
  //
  if (pData)
  {
    OSStatus status;
    double sort_time;
    uint32_t avc_flags = 0;
    CFDataRef avc_demux;
    CFDictionaryRef avc_time;

    if (m_convert_bytestream)
    {
      // convert demuxer packet from bytestream (AnnexB) to bitstream
      ByteIOContext *pb;
      int demuxer_bytes;
      uint8_t *demuxer_content;

      if(m_dllAvFormat->url_open_dyn_buf(&pb) < 0)
      {
        return VC_ERROR;
      }
      demuxer_bytes = avc_parse_nal_units(m_dllAvFormat, pb, pData, iSize);
      demuxer_bytes = m_dllAvFormat->url_close_dyn_buf(pb, &demuxer_content);
      avc_demux = CFDataCreate(kCFAllocatorDefault, demuxer_content, demuxer_bytes);
      m_dllAvUtil->av_free(demuxer_content);
    }
    else if (m_convert_3byteTo4byteNALSize)
    {
      // convert demuxer packet from 3 byte NAL sizes to 4 byte
      ByteIOContext *pb;
      if (m_dllAvFormat->url_open_dyn_buf(&pb) < 0)
        return VC_ERROR;

      uint32_t nal_size;
      uint8_t *end = pData + iSize;
      uint8_t *nal_start = pData;
      while (nal_start < end)
      {
        nal_size = VDA_RB24(nal_start);
        m_dllAvFormat->put_be32(pb, nal_size);
        nal_start += 3;
        m_dllAvFormat->put_buffer(pb, nal_start, nal_size);
        nal_start += nal_size;
      }

      uint8_t *demuxer_content;
      int demuxer_bytes = m_dllAvFormat->url_close_dyn_buf(pb, &demuxer_content);
      avc_demux = CFDataCreate(kCFAllocatorDefault, demuxer_content, demuxer_bytes);
      m_dllAvUtil->av_free(demuxer_content);
    }
    else
    {
      avc_demux = CFDataCreate(kCFAllocatorDefault, pData, iSize);
    }
    sort_time = (CurrentHostCounter() * 1000.0) / CurrentHostFrequency();
    avc_time = CreateDictionaryWithDisplayTime(sort_time - m_sort_time_offset, dts, pts);

    if (m_DropPictures)
      avc_flags = kVDADecoderDecodeFlags_DontEmitFrame;

    status = m_dll->VDADecoderDecode((VDADecoder)m_vda_decoder, avc_flags, avc_demux, avc_time);
    CFRelease(avc_time);
    CFRelease(avc_demux);
    if (status != kVDADecoderNoErr)
    {
      CLog::Log(LOGNOTICE, "%s - VDADecoderDecode failed, status(%d)", __FUNCTION__, (int)status);
      return VC_ERROR;
    }
  }

  if (!m_queue_depth || m_queue_depth < m_max_ref_frames)
  {
    return VC_BUFFER;
  }

  return VC_PICTURE | VC_BUFFER;
}

void CDVDVideoCodecVDA::Reset(void)
{
  CCocoaAutoPool pool;
  m_dll->VDADecoderFlush((VDADecoder)m_vda_decoder, 0);

  while (m_queue_depth)
    DisplayQueuePop();

  m_sort_time_offset = (CurrentHostCounter() * 1000.0) / CurrentHostFrequency();
}

bool CDVDVideoCodecVDA::GetPicture(DVDVideoPicture* pDvdVideoPicture)
{
  CCocoaAutoPool pool;
  FourCharCode pixel_buffer_format;
  CVPixelBufferRef picture_buffer_ref;

  // clone the video picture buffer settings.
  *pDvdVideoPicture = m_videobuffer;

  // get the top yuv frame, we risk getting the wrong frame if the frame queue
  // depth is less than the number of encoded reference frames. If queue depth
  // is greater than the number of encoded reference frames, then the top frame
  // will never change and we can just grab a ref to the top frame. This way
  // we don't lockout the vdadecoder while doing color format convert.
  pthread_mutex_lock(&m_queue_mutex);
  picture_buffer_ref = m_display_queue->pixel_buffer_ref;
  pixel_buffer_format = m_display_queue->pixel_buffer_format;
  pDvdVideoPicture->dts = m_display_queue->dts;
  pDvdVideoPicture->pts = m_display_queue->pts;
  pthread_mutex_unlock(&m_queue_mutex);

  // lock the CVPixelBuffer down
  CVPixelBufferLockBaseAddress(picture_buffer_ref, 0);
  int row_stride = CVPixelBufferGetBytesPerRowOfPlane(picture_buffer_ref, 0);
  uint8_t *base_ptr = (uint8_t*)CVPixelBufferGetBaseAddressOfPlane(picture_buffer_ref, 0);
  if (base_ptr)
  {
    if (pixel_buffer_format == kCVPixelFormatType_422YpCbCr8)
      UYVY422_to_YUV420P(base_ptr, row_stride, pDvdVideoPicture);
    else if (pixel_buffer_format == kCVPixelFormatType_32BGRA)
      BGRA_to_YUV420P(base_ptr, row_stride, pDvdVideoPicture);
  }
  // unlock the CVPixelBuffer
  CVPixelBufferUnlockBaseAddress(picture_buffer_ref, 0);

  // now we can pop the top frame.
  DisplayQueuePop();

  //CLog::Log(LOGNOTICE, "%s - VDADecoderDecode dts(%f), pts(%f)", __FUNCTION__,
  //  pDvdVideoPicture->dts, pDvdVideoPicture->pts);

  return VC_PICTURE | VC_BUFFER;
}

void CDVDVideoCodecVDA::UYVY422_to_YUV420P(uint8_t *yuv422_ptr, int yuv422_stride, DVDVideoPicture *picture)
{
  // convert PIX_FMT_UYVY422 to PIX_FMT_YUV420P.
  struct SwsContext *swcontext = m_dllSwScale->sws_getContext(
    m_videobuffer.iWidth, m_videobuffer.iHeight, PIX_FMT_UYVY422, 
    m_videobuffer.iWidth, m_videobuffer.iHeight, PIX_FMT_YUV420P, 
    SWS_FAST_BILINEAR | SwScaleCPUFlags(), NULL, NULL, NULL);
  if (swcontext)
  {
    uint8_t  *src[] = { yuv422_ptr, 0, 0, 0 };
    int srcStride[] = { yuv422_stride, 0, 0, 0 };

    uint8_t  *dst[] = { picture->data[0], picture->data[1], picture->data[2], 0 };
    int dstStride[] = { picture->iLineSize[0], picture->iLineSize[1], picture->iLineSize[2], 0 };

    m_dllSwScale->sws_scale(swcontext, src, srcStride, 0, picture->iHeight, dst, dstStride);
    m_dllSwScale->sws_freeContext(swcontext);
  }
}

void CDVDVideoCodecVDA::BGRA_to_YUV420P(uint8_t *bgra_ptr, int bgra_stride, DVDVideoPicture *picture)
{
  // convert PIX_FMT_BGRA to PIX_FMT_YUV420P.
  struct SwsContext *swcontext = m_dllSwScale->sws_getContext(
    m_videobuffer.iWidth, m_videobuffer.iHeight, PIX_FMT_BGRA, 
    m_videobuffer.iWidth, m_videobuffer.iHeight, PIX_FMT_YUV420P, 
    SWS_FAST_BILINEAR | SwScaleCPUFlags(), NULL, NULL, NULL);
  if (swcontext)
  {
    uint8_t  *src[] = { bgra_ptr, 0, 0, 0 };
    int srcStride[] = { bgra_stride, 0, 0, 0 };

    uint8_t  *dst[] = { picture->data[0], picture->data[1], picture->data[2], 0 };
    int dstStride[] = { picture->iLineSize[0], picture->iLineSize[1], picture->iLineSize[2], 0 };

    m_dllSwScale->sws_scale(swcontext, src, srcStride, 0, picture->iHeight, dst, dstStride);
    m_dllSwScale->sws_freeContext(swcontext);
  }
}

void CDVDVideoCodecVDA::DisplayQueuePop(void)
{
  CCocoaAutoPool pool;
  if (!m_display_queue || m_queue_depth == 0)
    return;

  // pop the top frame off the queue
  pthread_mutex_lock(&m_queue_mutex);
  frame_queue *top_frame = m_display_queue;
  m_display_queue = m_display_queue->nextframe;
  m_queue_depth--;
  pthread_mutex_unlock(&m_queue_mutex);

  // and release it
  CVPixelBufferRelease(top_frame->pixel_buffer_ref);
  free(top_frame);
}

void CDVDVideoCodecVDA::VDADecoderCallback(
  void                *decompressionOutputRefCon,
   CFDictionaryRef    frameInfo,
   OSStatus           status,
   uint32_t           infoFlags,
   CVImageBufferRef   imageBuffer)
{
  CCocoaAutoPool pool;
  // Warning, this is an async callback. There can be multiple frames in flight.
  CDVDVideoCodecVDA *ctx = (CDVDVideoCodecVDA*)decompressionOutputRefCon;

  if (imageBuffer == NULL)
  {
    //CLog::Log(LOGDEBUG, "%s - imageBuffer is NULL", __FUNCTION__);
    return;
  }
  OSType format_type = CVPixelBufferGetPixelFormatType(imageBuffer);
  if ((format_type != kCVPixelFormatType_422YpCbCr8) && (format_type != kCVPixelFormatType_32BGRA) )
  {
    CLog::Log(LOGERROR, "%s - imageBuffer format is not '2vuy' or 'BGRA',is reporting 0x%x",
      __FUNCTION__, (unsigned int)format_type);
    return;
  }
  if (kVDADecodeInfo_FrameDropped & infoFlags)
  {
    CLog::Log(LOGDEBUG, "%s - frame dropped", __FUNCTION__);
    return;
  }

  // allocate a new frame and populate it with some information.
  // this pointer to a frame_queue type keeps track of the newest decompressed frame
  // and is then inserted into a linked list of frame pointers depending on the display time
  // parsed out of the bitstream and stored in the frameInfo dictionary by the client
  frame_queue *newFrame = (frame_queue*)calloc(sizeof(frame_queue), 1);
  newFrame->nextframe = NULL;
  newFrame->pixel_buffer_format = format_type;
  newFrame->pixel_buffer_ref = CVPixelBufferRetain(imageBuffer);
  GetFrameDisplayTimeFromDictionary(frameInfo, newFrame);

  // if both dts or pts are good we use those, else use decoder insert time for frame sort
  if ((newFrame->pts != DVD_NOPTS_VALUE) || (newFrame->dts != DVD_NOPTS_VALUE))
  {
    // if pts is borked (stupid avi's), use dts for frame sort
    if (newFrame->pts == DVD_NOPTS_VALUE)
      newFrame->sort_time = newFrame->dts;
    else
      newFrame->sort_time = newFrame->pts;
  }

  // since the frames we get may be in decode order rather than presentation order
  // our hypothetical callback places them in a queue of frames which will
  // hold them in display order for display on another thread
  pthread_mutex_lock(&ctx->m_queue_mutex);
  //
  frame_queue *queueWalker = ctx->m_display_queue;
  if (!queueWalker || (newFrame->sort_time < queueWalker->sort_time))
  {
    // we have an empty queue, or this frame earlier than the current queue head.
    newFrame->nextframe = queueWalker;
    ctx->m_display_queue = newFrame;
  } else {
    // walk the queue and insert this frame where it belongs in display order.
    bool frameInserted = false;
    frame_queue *nextFrame = NULL;
    //
    while (!frameInserted)
    {
      nextFrame = queueWalker->nextframe;
      if (!nextFrame || (newFrame->sort_time < nextFrame->sort_time))
      {
        // if the next frame is the tail of the queue, or our new frame is earlier.
        newFrame->nextframe = nextFrame;
        queueWalker->nextframe = newFrame;
        frameInserted = true;
      }
      queueWalker = nextFrame;
    }
  }
  ctx->m_queue_depth++;
  //
  pthread_mutex_unlock(&ctx->m_queue_mutex);	
}

#endif
