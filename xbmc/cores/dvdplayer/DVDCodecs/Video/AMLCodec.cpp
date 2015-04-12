/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "system.h"

#include "AMLCodec.h"
#include "DynamicDll.h"

#include "cores/dvdplayer/DVDClock.h"
#include "cores/VideoRenderers/RenderFlags.h"
#include "cores/VideoRenderers/RenderManager.h"
#include "guilib/GraphicContext.h"
#include "settings/DisplaySettings.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "utils/AMLUtils.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/SysfsUtils.h"
#include "utils/TimeUtils.h"

#if defined(TARGET_ANDROID)
#include "android/activity/AndroidFeatures.h"
#include "utils/BitstreamConverter.h"
#endif

#include <unistd.h>
#include <queue>
#include <vector>
#include <signal.h>
#include <semaphore.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>

// amcodec include
extern "C" {
#include <amcodec/codec.h>
}  // extern "C"

typedef struct {
  bool          noblock;
  int           video_pid;
  int           video_type;
  stream_type_t stream_type;
  unsigned int  format;
  unsigned int  width;
  unsigned int  height;
  unsigned int  rate;
  unsigned int  extra;
  unsigned int  status;
  unsigned int  ratio;
  unsigned long long ratio64;
  void *param;
} aml_generic_param;

class DllLibamCodecInterface
{
public:
  virtual ~DllLibamCodecInterface() {};

  virtual int codec_init(codec_para_t *pcodec)=0;
  virtual int codec_close(codec_para_t *pcodec)=0;
  virtual int codec_reset(codec_para_t *pcodec)=0;
  virtual int codec_pause(codec_para_t *pcodec)=0;
  virtual int codec_resume(codec_para_t *pcodec)=0;
  virtual int codec_write(codec_para_t *pcodec, void *buffer, int len)=0;
  virtual int codec_checkin_pts(codec_para_t *pcodec, unsigned long pts)=0;
  virtual int codec_get_vbuf_state(codec_para_t *pcodec, struct buf_status *buf)=0;
  virtual int codec_get_vdec_state(codec_para_t *pcodec, struct vdec_status *vdec)=0;

  virtual int codec_init_cntl(codec_para_t *pcodec)=0;
  virtual int codec_poll_cntl(codec_para_t *pcodec)=0;
  virtual int codec_set_cntl_mode(codec_para_t *pcodec, unsigned int mode)=0;
  virtual int codec_set_cntl_avthresh(codec_para_t *pcodec, unsigned int avthresh)=0;
  virtual int codec_set_cntl_syncthresh(codec_para_t *pcodec, unsigned int syncthresh)=0;

  // grab these from libamplayer
  virtual int h263vld(unsigned char *inbuf, unsigned char *outbuf, int inbuf_len, int s263)=0;
  virtual int decodeble_h263(unsigned char *buf)=0;

  // grab this from amffmpeg so we do not have to load DllAvUtil
  virtual AVRational av_d2q(double d, int max)=0;
};

class DllLibAmCodec : public DllDynamic, DllLibamCodecInterface
{
  // libamcodec is static linked into libamplayer.so
  DECLARE_DLL_WRAPPER(DllLibAmCodec, "libamplayer.so")

  DEFINE_METHOD1(int, codec_init,               (codec_para_t *p1))
  DEFINE_METHOD1(int, codec_close,              (codec_para_t *p1))
  DEFINE_METHOD1(int, codec_reset,              (codec_para_t *p1))
  DEFINE_METHOD1(int, codec_pause,              (codec_para_t *p1))
  DEFINE_METHOD1(int, codec_resume,             (codec_para_t *p1))
  DEFINE_METHOD3(int, codec_write,              (codec_para_t *p1, void *p2, int p3))
  DEFINE_METHOD2(int, codec_checkin_pts,        (codec_para_t *p1, unsigned long p2))
  DEFINE_METHOD2(int, codec_get_vbuf_state,     (codec_para_t *p1, struct buf_status * p2))
  DEFINE_METHOD2(int, codec_get_vdec_state,     (codec_para_t *p1, struct vdec_status * p2))

  DEFINE_METHOD1(int, codec_init_cntl,          (codec_para_t *p1))
  DEFINE_METHOD1(int, codec_poll_cntl,          (codec_para_t *p1))
  DEFINE_METHOD2(int, codec_set_cntl_mode,      (codec_para_t *p1, unsigned int p2))
  DEFINE_METHOD2(int, codec_set_cntl_avthresh,  (codec_para_t *p1, unsigned int p2))
  DEFINE_METHOD2(int, codec_set_cntl_syncthresh,(codec_para_t *p1, unsigned int p2))

  DEFINE_METHOD4(int, h263vld,                  (unsigned char *p1, unsigned char *p2, int p3, int p4))
  DEFINE_METHOD1(int, decodeble_h263,           (unsigned char *p1))

  DEFINE_METHOD2(AVRational, av_d2q,            (double p1, int p2))

  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(codec_init)
    RESOLVE_METHOD(codec_close)
    RESOLVE_METHOD(codec_reset)
    RESOLVE_METHOD(codec_pause)
    RESOLVE_METHOD(codec_resume)
    RESOLVE_METHOD(codec_write)
    RESOLVE_METHOD(codec_checkin_pts)
    RESOLVE_METHOD(codec_get_vbuf_state)
    RESOLVE_METHOD(codec_get_vdec_state)

    RESOLVE_METHOD(codec_init_cntl)
    RESOLVE_METHOD(codec_poll_cntl)
    RESOLVE_METHOD(codec_set_cntl_mode)
    RESOLVE_METHOD(codec_set_cntl_avthresh)
    RESOLVE_METHOD(codec_set_cntl_syncthresh)

    RESOLVE_METHOD(h263vld)
    RESOLVE_METHOD(decodeble_h263)

    RESOLVE_METHOD(av_d2q)
  END_METHOD_RESOLVE()

public:
  void codec_init_para(aml_generic_param *p_in, codec_para_t *p_out)
  {
    memset(p_out, 0x00, sizeof(codec_para_t));

#ifdef TARGET_ANDROID
    bits_writer_t bs = {0};

    // we are always as large as codec_para_t from headers.
    CBitstreamConverter::init_bits_writer(&bs, (uint8_t*)p_out, sizeof(codec_para_t), 1);

    // order matters, so pay attention
    // to codec_para_t in codec_types.h
    CBitstreamConverter::write_bits(&bs, 32, 0);                   // CODEC_HANDLE handle
    CBitstreamConverter::write_bits(&bs, 32, 0);                   // CODEC_HANDLE cntl_handle
    CBitstreamConverter::write_bits(&bs, 32, 0);                   // CODEC_HANDLE sub_handle

    CBitstreamConverter::write_bits(&bs, 32, 0);                   // CODEC_HANDLE audio_utils_handle

    CBitstreamConverter::write_bits(&bs, 32, p_in->stream_type);   // stream_type_t stream_type

    // watch these, using bit fields (which is stupid)
    CBitstreamConverter::write_bits(&bs,  1, 1);                   // unsigned int has_video:1
    CBitstreamConverter::write_bits(&bs,  1, 0);                   // unsigned int has_audio:1
    CBitstreamConverter::write_bits(&bs,  1, 0);                   // unsigned int has_sub:1
    unsigned int value =  p_in->noblock > 0 ? 1:0;
    CBitstreamConverter::write_bits(&bs,  1, value);               // unsigned int noblock:1
    CBitstreamConverter::write_bits(&bs, 28, 0);                   // align back to next word boundary

    CBitstreamConverter::write_bits(&bs, 32, p_in->video_type);    // int video_type
    CBitstreamConverter::write_bits(&bs, 32, 0);                   // int audio_type
    CBitstreamConverter::write_bits(&bs, 32, 0);                   // int sub_type

    CBitstreamConverter::write_bits(&bs, 32, p_in->video_pid);     // int video_pid
    CBitstreamConverter::write_bits(&bs, 32, 0);                   // int audio_pid
    CBitstreamConverter::write_bits(&bs, 32, 0);                   // int sub_pid

    CBitstreamConverter::write_bits(&bs, 32, 0);                   // int audio_channels
    CBitstreamConverter::write_bits(&bs, 32, 0);                   // int audio_samplerate
    CBitstreamConverter::write_bits(&bs, 32, 0);                   // int vbuf_size
    CBitstreamConverter::write_bits(&bs, 32, 0);                   // int abuf_size

    CBitstreamConverter::write_bits(&bs, 32, p_in->format);        // am_sysinfo, unsigned int format
    CBitstreamConverter::write_bits(&bs, 32, p_in->width);         // am_sysinfo, unsigned int width
    CBitstreamConverter::write_bits(&bs, 32, p_in->height);        // am_sysinfo, unsigned int height
    CBitstreamConverter::write_bits(&bs, 32, p_in->rate);          // am_sysinfo, unsigned int rate
    CBitstreamConverter::write_bits(&bs, 32, p_in->extra);         // am_sysinfo, unsigned int extra
    CBitstreamConverter::write_bits(&bs, 32, p_in->status);        // am_sysinfo, unsigned int status
    CBitstreamConverter::write_bits(&bs, 32, p_in->ratio);         // am_sysinfo, unsigned int ratio
    CBitstreamConverter::write_bits(&bs, 32, (unsigned int)p_in->param); // am_sysinfo, unsigned int param
    unsigned int lo = 0x00000000ffffffff & p_in->ratio64;
    unsigned int hi = p_in->ratio64 >> 32;
    CBitstreamConverter::write_bits(&bs, 32, lo);                  // am_sysinfo, unsigned long long ratio64
    CBitstreamConverter::write_bits(&bs, 32, hi);                  // am_sysinfo, unsigned long long ratio64

    // we do not care about the rest, flush and go.
    // FYI there are 4.0 to 4.1 differences here.
    CBitstreamConverter::flush_bits(&bs);
    //CLog::MemDump((char*)p_out, 0xFF);
#else
    // direct struct usage, we do not know which flavor
    // so just use what we get from headers and pray.
    p_out->has_video          = 1;
    p_out->noblock            = p_in->noblock;
    p_out->video_pid          = p_in->video_pid;
    p_out->video_type         = p_in->video_type;
    p_out->stream_type        = p_in->stream_type;
    p_out->am_sysinfo.format  = p_in->format;
    p_out->am_sysinfo.width   = p_in->width;
    p_out->am_sysinfo.height  = p_in->height;
    p_out->am_sysinfo.rate    = p_in->rate;
    p_out->am_sysinfo.extra   = p_in->extra;
    p_out->am_sysinfo.status  = p_in->status;
    p_out->am_sysinfo.ratio   = p_in->ratio;
    p_out->am_sysinfo.ratio64 = p_in->ratio64;
    p_out->am_sysinfo.param   = p_in->param;
#endif
  }
};

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------
// AppContext - Application state
#define MODE_3D_DISABLE         0x00000000
#define MODE_3D_LR              0x00000101
#define MODE_3D_LR_SWITCH       0x00000501
#define MODE_3D_BT              0x00000201
#define MODE_3D_BT_SWITCH       0x00000601
#define MODE_3D_TO_2D_L         0x00000102
#define MODE_3D_TO_2D_R         0x00000902
#define MODE_3D_TO_2D_T         0x00000202
#define MODE_3D_TO_2D_B         0x00000a02

#define PTS_FREQ        90000
#define UNIT_FREQ       96000
#define AV_SYNC_THRESH  PTS_FREQ*30

#define TRICKMODE_NONE  0x00
#define TRICKMODE_I     0x01
#define TRICKMODE_FFFB  0x02

// same as AV_NOPTS_VALUE
#define INT64_0         INT64_C(0x8000000000000000)

#define EXTERNAL_PTS    (1)
#define SYNC_OUTSIDE    (2)

// missing tags
#define CODEC_TAG_VC_1  (0x312D4356)
#define CODEC_TAG_RV30  (0x30335652)
#define CODEC_TAG_RV40  (0x30345652)
#define CODEC_TAG_MJPEG (0x47504a4d)
#define CODEC_TAG_mjpeg (0x47504a4c)
#define CODEC_TAG_jpeg  (0x6765706a)
#define CODEC_TAG_mjpa  (0x61706a6d)

#define RW_WAIT_TIME    (20 * 1000) // 20ms

#define P_PRE           (0x02000000)
#define F_PRE           (0x03000000)
#define PLAYER_SUCCESS          (0)
#define PLAYER_FAILED           (-(P_PRE|0x01))
#define PLAYER_NOMEM            (-(P_PRE|0x02))
#define PLAYER_EMPTY_P          (-(P_PRE|0x03))

#define PLAYER_WR_FAILED        (-(P_PRE|0x21))
#define PLAYER_WR_EMPTYP        (-(P_PRE|0x22))
#define PLAYER_WR_FINISH        (P_PRE|0x1)

#define PLAYER_PTS_ERROR        (-(P_PRE|0x31))
#define PLAYER_UNSUPPORT        (-(P_PRE|0x35))
#define PLAYER_CHECK_CODEC_ERROR  (-(P_PRE|0x39))

#define HDR_BUF_SIZE 1024
typedef struct hdr_buf {
    char *data;
    int size;
} hdr_buf_t;

typedef struct am_packet {
    AVPacket      avpkt;
    int64_t       avpts;
    int64_t       avdts;
    int           avduration;
    int           isvalid;
    int           newflag;
    int64_t       lastpts;
    unsigned char *data;
    unsigned char *buf;
    int           data_size;
    int           buf_size;
    hdr_buf_t     *hdr;
    codec_para_t  *codec;
} am_packet_t;

typedef enum {
    AM_STREAM_UNKNOWN = 0,
    AM_STREAM_TS,
    AM_STREAM_PS,
    AM_STREAM_ES,
    AM_STREAM_RM,
    AM_STREAM_AUDIO,
    AM_STREAM_VIDEO,
} pstream_type;

typedef struct am_private_t
{
  am_packet_t       am_pkt;
  aml_generic_param gcodec;
  codec_para_t      vcodec;

  pstream_type      stream_type;
  int               check_first_pts;

  vformat_t         video_format;
  int               video_pid;
  unsigned int      video_codec_id;
  unsigned int      video_codec_tag;
  vdec_type_t       video_codec_type;
  unsigned int      video_width;
  unsigned int      video_height;
  unsigned int      video_ratio;
  unsigned int      video_ratio64;
  unsigned int      video_rate;
  unsigned int      video_rotation_degree;
  int               flv_flag;
  int               h263_decodable;
  int               extrasize;
  uint8_t           *extradata;
  DllLibAmCodec     *m_dll;

  int               dumpfile;
  bool              dumpdemux;
} am_private_t;

/*************************************************************************/
/*************************************************************************/
void dumpfile_open(am_private_t *para)
{
  if (para->dumpdemux)
  {
    static int amcodec_dumpid = 0;
    char dump_path[128] = {0};
    sprintf(dump_path, "/temp/dump_amcodec-%d.dat", amcodec_dumpid++);

    para->dumpfile = open(dump_path, O_CREAT | O_RDWR, 0666);
  }
}
void dumpfile_close(am_private_t *para)
{
  if (para->dumpdemux && para->dumpfile != -1)
    close(para->dumpfile), para->dumpfile = -1;
}
void dumpfile_write(am_private_t *para, void* buf, int bufsiz)
{
  if (!buf)
  {
    CLog::Log(LOGERROR, "dumpfile_write: wtf ? buf is null, bufsiz(%d)", bufsiz);
    return;
  }

  if (para->dumpdemux && para->dumpfile != -1)
    write(para->dumpfile, buf, bufsiz);
}

/*************************************************************************/
/*************************************************************************/
static int64_t get_pts_video()
{
  int fd = open("/sys/class/tsync/pts_video", O_RDONLY);
  if (fd >= 0)
  {
    char pts_str[16];
    int size = read(fd, pts_str, sizeof(pts_str));
    close(fd);
    if (size > 0)
    {
      unsigned long pts = strtoul(pts_str, NULL, 16);
      return pts;
    }
  }

  CLog::Log(LOGERROR, "get_pts_video: open /tsync/event error");
  return -1;
}

static int set_pts_pcrscr(int64_t value)
{
  int fd = open("/sys/class/tsync/pts_pcrscr", O_WRONLY);
  if (fd >= 0)
  {
    char pts_str[64];
    unsigned long pts = (unsigned long)value;
    sprintf(pts_str, "0x%lx", pts);
    write(fd, pts_str, strlen(pts_str));
    close(fd);
    return 0;
  }

  CLog::Log(LOGERROR, "set_pts_pcrscr: open pts_pcrscr error");
  return -1;
}

static vformat_t codecid_to_vformat(enum AVCodecID id)
{
  vformat_t format;
  switch (id)
  {
    case AV_CODEC_ID_MPEG1VIDEO:
    case AV_CODEC_ID_MPEG2VIDEO:
    case AV_CODEC_ID_MPEG2VIDEO_XVMC:
      format = VFORMAT_MPEG12;
      break;
    case AV_CODEC_ID_H263:
    case AV_CODEC_ID_MPEG4:
    case AV_CODEC_ID_H263P:
    case AV_CODEC_ID_H263I:
    case AV_CODEC_ID_MSMPEG4V2:
    case AV_CODEC_ID_MSMPEG4V3:
    case AV_CODEC_ID_FLV1:
      format = VFORMAT_MPEG4;
      break;
    case AV_CODEC_ID_RV10:
    case AV_CODEC_ID_RV20:
    case AV_CODEC_ID_RV30:
    case AV_CODEC_ID_RV40:
      format = VFORMAT_REAL;
      break;
    case AV_CODEC_ID_H264:
      format = VFORMAT_H264;
      break;
    /*
    case AV_CODEC_ID_H264MVC:
      // H264 Multiview Video Coding (3d blurays)
      format = VFORMAT_H264MVC;
      break;
    */
    case AV_CODEC_ID_MJPEG:
      format = VFORMAT_MJPEG;
      break;
    case AV_CODEC_ID_VC1:
    case AV_CODEC_ID_WMV3:
      format = VFORMAT_VC1;
      break;
    case AV_CODEC_ID_AVS:
    case AV_CODEC_ID_CAVS:
      format = VFORMAT_AVS;
      break;
    case AV_CODEC_ID_HEVC:
      format = VFORMAT_HEVC;
      break;

    default:
      format = VFORMAT_UNSUPPORT;
      break;
  }

  CLog::Log(LOGDEBUG, "codecid_to_vformat, id(%d) -> vformat(%d)", (int)id, format);
  return format;
}

static vdec_type_t codec_tag_to_vdec_type(unsigned int codec_tag)
{
  vdec_type_t dec_type;
  switch (codec_tag)
  {
    case CODEC_TAG_MJPEG:
    case CODEC_TAG_mjpeg:
    case CODEC_TAG_jpeg:
    case CODEC_TAG_mjpa:
      // mjpeg
      dec_type = VIDEO_DEC_FORMAT_MJPEG;
      break;
    case CODEC_TAG_XVID:
    case CODEC_TAG_xvid:
    case CODEC_TAG_XVIX:
      // xvid
      dec_type = VIDEO_DEC_FORMAT_MPEG4_5;
      break;
    case CODEC_TAG_COL1:
    case CODEC_TAG_DIV3:
    case CODEC_TAG_MP43:
      // divx3.11
      dec_type = VIDEO_DEC_FORMAT_MPEG4_3;
      break;
    case CODEC_TAG_DIV4:
    case CODEC_TAG_DIVX:
      // divx4
      dec_type = VIDEO_DEC_FORMAT_MPEG4_4;
      break;
    case CODEC_TAG_DIV5:
    case CODEC_TAG_DX50:
    case CODEC_TAG_M4S2:
    case CODEC_TAG_FMP4:
      // divx5
      dec_type = VIDEO_DEC_FORMAT_MPEG4_5;
      break;
    case CODEC_TAG_DIV6:
      // divx6
      dec_type = VIDEO_DEC_FORMAT_MPEG4_5;
      break;
    case CODEC_TAG_MP4V:
    case CODEC_TAG_RMP4:
    case CODEC_TAG_MPG4:
    case CODEC_TAG_mp4v:
    case AV_CODEC_ID_MPEG4:
      // mp4
      dec_type = VIDEO_DEC_FORMAT_MPEG4_5;
      break;
    case AV_CODEC_ID_H263:
    case CODEC_TAG_H263:
    case CODEC_TAG_h263:
    case CODEC_TAG_s263:
    case CODEC_TAG_F263:
      // h263
      dec_type = VIDEO_DEC_FORMAT_H263;
      break;
    case CODEC_TAG_AVC1:
    case CODEC_TAG_avc1:
    case CODEC_TAG_H264:
    case CODEC_TAG_h264:
    case AV_CODEC_ID_H264:
      // h264
      dec_type = VIDEO_DEC_FORMAT_H264;
      break;
    /*
    case AV_CODEC_ID_H264MVC:
      dec_type = VIDEO_DEC_FORMAT_H264;
      break;
    */
    case AV_CODEC_ID_RV30:
    case CODEC_TAG_RV30:
      // realmedia 3
      dec_type = VIDEO_DEC_FORMAT_REAL_8;
      break;
    case AV_CODEC_ID_RV40:
    case CODEC_TAG_RV40:
      // realmedia 4
      dec_type = VIDEO_DEC_FORMAT_REAL_9;
      break;
    case CODEC_TAG_WMV3:
      // wmv3
      dec_type = VIDEO_DEC_FORMAT_WMV3;
      break;
    case AV_CODEC_ID_VC1:
    case CODEC_TAG_VC_1:
    case CODEC_TAG_WVC1:
    case CODEC_TAG_WMVA:
      // vc1
      dec_type = VIDEO_DEC_FORMAT_WVC1;
      break;
    case AV_CODEC_ID_VP6F:
      // vp6
      dec_type = VIDEO_DEC_FORMAT_SW;
      break;
    case AV_CODEC_ID_CAVS:
    case AV_CODEC_ID_AVS:
      // avs
      dec_type = VIDEO_DEC_FORMAT_AVS;
      break;
    case AV_CODEC_ID_HEVC:
      // h265
      dec_type = VIDEO_DEC_FORMAT_HEVC;
      break;
    default:
      dec_type = VIDEO_DEC_FORMAT_UNKNOW;
      break;
  }

  CLog::Log(LOGDEBUG, "codec_tag_to_vdec_type, codec_tag(%d) -> vdec_type(%d)", codec_tag, dec_type);
  return dec_type;
}

static void am_packet_init(am_packet_t *pkt)
{
  memset(&pkt->avpkt, 0, sizeof(AVPacket));
  pkt->avpts      = 0;
  pkt->avdts      = 0;
  pkt->avduration = 0;
  pkt->isvalid    = 0;
  pkt->newflag    = 0;
  pkt->lastpts    = 0;
  pkt->data       = NULL;
  pkt->buf        = NULL;
  pkt->data_size  = 0;
  pkt->buf_size   = 0;
  pkt->hdr        = NULL;
  pkt->codec      = NULL;
}

void am_packet_release(am_packet_t *pkt)
{
  if (pkt->buf != NULL)
    free(pkt->buf), pkt->buf= NULL;
  if (pkt->hdr != NULL)
  {
    if (pkt->hdr->data != NULL)
      free(pkt->hdr->data), pkt->hdr->data = NULL;
    free(pkt->hdr), pkt->hdr = NULL;
  }

  pkt->codec = NULL;
}

int check_in_pts(am_private_t *para, am_packet_t *pkt)
{
    int last_duration = 0;
    static int last_v_duration = 0;
    int64_t pts = 0;

    last_duration = last_v_duration;

    if (para->stream_type == AM_STREAM_ES) {
        if ((int64_t)INT64_0 != pkt->avpts) {
            pts = pkt->avpts;

            if (para->m_dll->codec_checkin_pts(pkt->codec, pts) != 0) {
                CLog::Log(LOGDEBUG, "ERROR check in pts error!");
                return PLAYER_PTS_ERROR;
            }

        } else if ((int64_t)INT64_0 != pkt->avdts) {
            pts = pkt->avdts * last_duration;

            if (para->m_dll->codec_checkin_pts(pkt->codec, pts) != 0) {
                CLog::Log(LOGDEBUG, "ERROR check in dts error!");
                return PLAYER_PTS_ERROR;
            }

            last_v_duration = pkt->avduration ? pkt->avduration : 1;
        } else {
            if (!para->check_first_pts) {
                if (para->m_dll->codec_checkin_pts(pkt->codec, 0) != 0) {
                    CLog::Log(LOGDEBUG, "ERROR check in 0 to video pts error!");
                    return PLAYER_PTS_ERROR;
                }
            }
        }
        if (!para->check_first_pts) {
            para->check_first_pts = 1;
        }
    }
    if (pts > 0)
      pkt->lastpts = pts;

    return PLAYER_SUCCESS;
}

static int write_header(am_private_t *para, am_packet_t *pkt)
{
    int write_bytes = 0, len = 0;

    if (pkt->hdr && pkt->hdr->size > 0) {
        if ((NULL == pkt->codec) || (NULL == pkt->hdr->data)) {
            CLog::Log(LOGDEBUG, "[write_header]codec null!");
            return PLAYER_EMPTY_P;
        }
        //some wvc1 es data not need to add header
        if (para->video_format == VFORMAT_VC1 && para->video_codec_type == VIDEO_DEC_FORMAT_WVC1) {
            if ((pkt->data) && (pkt->data_size >= 4)
              && (pkt->data[0] == 0) && (pkt->data[1] == 0)
              && (pkt->data[2] == 1) && (pkt->data[3] == 0xd || pkt->data[3] == 0xf)) {
                return PLAYER_SUCCESS;
            }
        }
        while (1) {
            write_bytes = para->m_dll->codec_write(pkt->codec, pkt->hdr->data + len, pkt->hdr->size - len);
            if (write_bytes < 0 || write_bytes > (pkt->hdr->size - len)) {
                if (-errno != AVERROR(EAGAIN)) {
                    CLog::Log(LOGDEBUG, "ERROR:write header failed!");
                    return PLAYER_WR_FAILED;
                } else {
                    continue;
                }
            } else {
                dumpfile_write(para, pkt->hdr->data, write_bytes);
                len += write_bytes;
                if (len == pkt->hdr->size) {
                    break;
                }
            }
        }
    }
    return PLAYER_SUCCESS;
}

int check_avbuffer_enough(am_private_t *para, am_packet_t *pkt)
{
    return 1;
}

int write_av_packet(am_private_t *para, am_packet_t *pkt)
{
  //CLog::Log(LOGDEBUG, "write_av_packet, pkt->isvalid(%d), pkt->data(%p), pkt->data_size(%d)",
  //  pkt->isvalid, pkt->data, pkt->data_size);

    int write_bytes = 0, len = 0, ret;
    unsigned char *buf;
    int size;

    // do we need to check in pts or write the header ?
    if (pkt->newflag) {
        if (pkt->isvalid) {
            ret = check_in_pts(para, pkt);
            if (ret != PLAYER_SUCCESS) {
                CLog::Log(LOGDEBUG, "check in pts failed");
                return PLAYER_WR_FAILED;
            }
        }
        if (write_header(para, pkt) == PLAYER_WR_FAILED) {
            CLog::Log(LOGDEBUG, "[%s]write header failed!", __FUNCTION__);
            return PLAYER_WR_FAILED;
        }
        pkt->newflag = 0;
    }
	
    buf = pkt->data;
    size = pkt->data_size ;
    if (size == 0 && pkt->isvalid) {
        pkt->isvalid = 0;
        pkt->data_size = 0;
    }

    while (size > 0 && pkt->isvalid) {
        write_bytes = para->m_dll->codec_write(pkt->codec, buf, size);
        if (write_bytes < 0 || write_bytes > size) {
            CLog::Log(LOGDEBUG, "write codec data failed, write_bytes(%d), errno(%d), size(%d)", write_bytes, errno, size);
            if (-errno != AVERROR(EAGAIN)) {
                CLog::Log(LOGDEBUG, "write codec data failed!");
                return PLAYER_WR_FAILED;
            } else {
                // adjust for any data we already wrote into codec.
                // we sleep a bit then exit as we will get called again
                // with the same pkt because pkt->isvalid has not been cleared.
                pkt->data += len;
                pkt->data_size -= len;
                usleep(RW_WAIT_TIME);
                CLog::Log(LOGDEBUG, "usleep(RW_WAIT_TIME), len(%d)", len);
                return PLAYER_SUCCESS;
            }
        } else {
            dumpfile_write(para, buf, write_bytes);
            // keep track of what we write into codec from this pkt
            // in case we get hit with EAGAIN.
            len += write_bytes;
            if (len == pkt->data_size) {
                pkt->isvalid = 0;
                pkt->data_size = 0;
                break;
            } else if (len < pkt->data_size) {
                buf += write_bytes;
                size -= write_bytes;
            } else {
                // writing more that we should is a failure.
                return PLAYER_WR_FAILED;
            }
        }
    }

    return PLAYER_SUCCESS;
}

/*************************************************************************/
static int m4s2_dx50_mp4v_add_header(unsigned char *buf, int size,  am_packet_t *pkt)
{
    if (size > pkt->hdr->size) {
        free(pkt->hdr->data), pkt->hdr->data = NULL;
        pkt->hdr->size = 0;

        pkt->hdr->data = (char*)malloc(size);
        if (!pkt->hdr->data) {
            CLog::Log(LOGDEBUG, "[m4s2_dx50_add_header] NOMEM!");
            return PLAYER_FAILED;
        }
    }

    pkt->hdr->size = size;
    memcpy(pkt->hdr->data, buf, size);

    return PLAYER_SUCCESS;
}

static int m4s2_dx50_mp4v_write_header(am_private_t *para, am_packet_t *pkt)
{
    CLog::Log(LOGDEBUG, "m4s2_dx50_mp4v_write_header");
    int ret = m4s2_dx50_mp4v_add_header(para->extradata, para->extrasize, pkt);
    if (ret == PLAYER_SUCCESS) {
        if (1) {
            pkt->codec = &para->vcodec;
        } else {
            CLog::Log(LOGDEBUG, "[m4s2_dx50_mp4v_add_header]invalid video codec!");
            return PLAYER_EMPTY_P;
        }
        pkt->newflag = 1;
        ret = write_av_packet(para, pkt);
    }
    return ret;
}

static int mjpeg_data_prefeeding(am_packet_t *pkt)
{
    const unsigned char mjpeg_addon_data[] = {
        0xff, 0xd8, 0xff, 0xc4, 0x01, 0xa2, 0x00, 0x00, 0x01, 0x05, 0x01, 0x01, 0x01,
        0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02,
        0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x01, 0x00, 0x03, 0x01,
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x10,
        0x00, 0x02, 0x01, 0x03, 0x03, 0x02, 0x04, 0x03, 0x05, 0x05, 0x04, 0x04, 0x00,
        0x00, 0x01, 0x7d, 0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12, 0x21, 0x31,
        0x41, 0x06, 0x13, 0x51, 0x61, 0x07, 0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1,
        0x08, 0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0, 0x24, 0x33, 0x62, 0x72,
        0x82, 0x09, 0x0a, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28, 0x29,
        0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47,
        0x48, 0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x63, 0x64,
        0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
        0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95,
        0x96, 0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9,
        0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4,
        0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8,
        0xd9, 0xda, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xf1,
        0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0x11, 0x00, 0x02, 0x01,
        0x02, 0x04, 0x04, 0x03, 0x04, 0x07, 0x05, 0x04, 0x04, 0x00, 0x01, 0x02, 0x77,
        0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21, 0x31, 0x06, 0x12, 0x41, 0x51,
        0x07, 0x61, 0x71, 0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91, 0xa1, 0xb1,
        0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0, 0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24,
        0x34, 0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26, 0x27, 0x28, 0x29, 0x2a,
        0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
        0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x63, 0x64, 0x65, 0x66,
        0x67, 0x68, 0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x82,
        0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96,
        0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa,
        0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5,
        0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9,
        0xda, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xf2, 0xf3, 0xf4,
        0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa
    };

    if (pkt->hdr->data) {
        memcpy(pkt->hdr->data, &mjpeg_addon_data, sizeof(mjpeg_addon_data));
        pkt->hdr->size = sizeof(mjpeg_addon_data);
    } else {
        CLog::Log(LOGDEBUG, "[mjpeg_data_prefeeding]No enough memory!");
        return PLAYER_FAILED;
    }
    return PLAYER_SUCCESS;
}

static int mjpeg_write_header(am_private_t *para, am_packet_t *pkt)
{
    mjpeg_data_prefeeding(pkt);
    if (1) {
        pkt->codec = &para->vcodec;
    } else {
        CLog::Log(LOGDEBUG, "[mjpeg_write_header]invalid codec!");
        return PLAYER_EMPTY_P;
    }
    pkt->newflag = 1;
    write_av_packet(para, pkt);
    return PLAYER_SUCCESS;
}

static int divx3_data_prefeeding(am_packet_t *pkt, unsigned w, unsigned h)
{
    unsigned i = (w << 12) | (h & 0xfff);
    unsigned char divx311_add[10] = {
        0x00, 0x00, 0x00, 0x01,
        0x20, 0x00, 0x00, 0x00,
        0x00, 0x00
    };
    divx311_add[5] = (i >> 16) & 0xff;
    divx311_add[6] = (i >> 8) & 0xff;
    divx311_add[7] = i & 0xff;

    if (pkt->hdr->data) {
        memcpy(pkt->hdr->data, divx311_add, sizeof(divx311_add));
        pkt->hdr->size = sizeof(divx311_add);
    } else {
        CLog::Log(LOGDEBUG, "[divx3_data_prefeeding]No enough memory!");
        return PLAYER_FAILED;
    }
    return PLAYER_SUCCESS;
}

static int divx3_write_header(am_private_t *para, am_packet_t *pkt)
{
    CLog::Log(LOGDEBUG, "divx3_write_header");
    divx3_data_prefeeding(pkt, para->video_width, para->video_height);
    if (1) {
        pkt->codec = &para->vcodec;
    } else {
        CLog::Log(LOGDEBUG, "[divx3_write_header]invalid codec!");
        return PLAYER_EMPTY_P;
    }
    pkt->newflag = 1;
    write_av_packet(para, pkt);
    return PLAYER_SUCCESS;
}

static int h264_add_header(unsigned char *buf, int size, am_packet_t *pkt)
{
    if (size > HDR_BUF_SIZE)
    {
        free(pkt->hdr->data);
        pkt->hdr->data = (char *)malloc(size);
        if (!pkt->hdr->data)
            return PLAYER_NOMEM;
    }

    memcpy(pkt->hdr->data, buf, size);
    pkt->hdr->size = size;
    return PLAYER_SUCCESS;
}

static int h264_write_header(am_private_t *para, am_packet_t *pkt)
{
    // CLog::Log(LOGDEBUG, "h264_write_header");
    int ret = h264_add_header(para->extradata, para->extrasize, pkt);
    if (ret == PLAYER_SUCCESS) {
        //if (ctx->vcodec) {
        if (1) {
            pkt->codec = &para->vcodec;
        } else {
            //CLog::Log(LOGDEBUG, "[pre_header_feeding]invalid video codec!");
            return PLAYER_EMPTY_P;
        }

        pkt->newflag = 1;
        ret = write_av_packet(para, pkt);
    }
    return ret;
}

static int hevc_add_header(unsigned char *buf, int size,  am_packet_t *pkt)
{
    if (size > HDR_BUF_SIZE)
    {
        free(pkt->hdr->data);
        pkt->hdr->data = (char *)malloc(size);
        if (!pkt->hdr->data)
            return PLAYER_NOMEM;
    }

    memcpy(pkt->hdr->data, buf, size);
    pkt->hdr->size = size;
    return PLAYER_SUCCESS;
}

static int hevc_write_header(am_private_t *para, am_packet_t *pkt)
{
    int ret = -1;

    if (para->extradata) {
      ret = hevc_add_header(para->extradata, para->extrasize, pkt);
    }
    if (ret == PLAYER_SUCCESS) {
      pkt->codec = &para->vcodec;
      pkt->newflag = 1;
      ret = write_av_packet(para, pkt);
    }
    return ret;
}

static int wmv3_write_header(am_private_t *para, am_packet_t *pkt)
{
    CLog::Log(LOGDEBUG, "wmv3_write_header");
    unsigned i, check_sum = 0;
    unsigned data_len = para->extrasize + 4;

    pkt->hdr->data[0] = 0;
    pkt->hdr->data[1] = 0;
    pkt->hdr->data[2] = 1;
    pkt->hdr->data[3] = 0x10;

    pkt->hdr->data[4] = 0;
    pkt->hdr->data[5] = (data_len >> 16) & 0xff;
    pkt->hdr->data[6] = 0x88;
    pkt->hdr->data[7] = (data_len >> 8) & 0xff;
    pkt->hdr->data[8] = data_len & 0xff;
    pkt->hdr->data[9] = 0x88;

    pkt->hdr->data[10] = 0xff;
    pkt->hdr->data[11] = 0xff;
    pkt->hdr->data[12] = 0x88;
    pkt->hdr->data[13] = 0xff;
    pkt->hdr->data[14] = 0xff;
    pkt->hdr->data[15] = 0x88;

    for (i = 4 ; i < 16 ; i++) {
        check_sum += pkt->hdr->data[i];
    }

    pkt->hdr->data[16] = (check_sum >> 8) & 0xff;
    pkt->hdr->data[17] =  check_sum & 0xff;
    pkt->hdr->data[18] = 0x88;
    pkt->hdr->data[19] = (check_sum >> 8) & 0xff;
    pkt->hdr->data[20] =  check_sum & 0xff;
    pkt->hdr->data[21] = 0x88;

    pkt->hdr->data[22] = (para->video_width >> 8) & 0xff;
    pkt->hdr->data[23] =  para->video_width & 0xff;
    pkt->hdr->data[24] = (para->video_height >> 8) & 0xff;
    pkt->hdr->data[25] =  para->video_height & 0xff;

    memcpy(pkt->hdr->data + 26, para->extradata, para->extrasize);
    pkt->hdr->size = para->extrasize + 26;
    if (1) {
        pkt->codec = &para->vcodec;
    } else {
        CLog::Log(LOGDEBUG, "[wmv3_write_header]invalid codec!");
        return PLAYER_EMPTY_P;
    }
    pkt->newflag = 1;
    return write_av_packet(para, pkt);
}

static int wvc1_write_header(am_private_t *para, am_packet_t *pkt)
{
    CLog::Log(LOGDEBUG, "wvc1_write_header");
    memcpy(pkt->hdr->data, para->extradata + 1, para->extrasize - 1);
    pkt->hdr->size = para->extrasize - 1;
    if (1) {
        pkt->codec = &para->vcodec;
    } else {
        CLog::Log(LOGDEBUG, "[wvc1_write_header]invalid codec!");
        return PLAYER_EMPTY_P;
    }
    pkt->newflag = 1;
    return write_av_packet(para, pkt);
}

static int mpeg_add_header(am_private_t *para, am_packet_t *pkt)
{
    CLog::Log(LOGDEBUG, "mpeg_add_header");
#define STUFF_BYTES_LENGTH     (256)
    int size;
    unsigned char packet_wrapper[] = {
        0x00, 0x00, 0x01, 0xe0,
        0x00, 0x00,                                /* pes packet length */
        0x81, 0xc0, 0x0d,
        0x20, 0x00, 0x00, 0x00, 0x00, /* PTS */
        0x1f, 0xff, 0xff, 0xff, 0xff, /* DTS */
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff
    };

    size = para->extrasize + sizeof(packet_wrapper);
    packet_wrapper[4] = size >> 8 ;
    packet_wrapper[5] = size & 0xff ;
    memcpy(pkt->hdr->data, packet_wrapper, sizeof(packet_wrapper));
    size = sizeof(packet_wrapper);
    //CLog::Log(LOGDEBUG, "[mpeg_add_header:%d]wrapper size=%d\n",__LINE__,size);
    memcpy(pkt->hdr->data + size, para->extradata, para->extrasize);
    size += para->extrasize;
    //CLog::Log(LOGDEBUG, "[mpeg_add_header:%d]wrapper+seq size=%d\n",__LINE__,size);
    memset(pkt->hdr->data + size, 0xff, STUFF_BYTES_LENGTH);
    size += STUFF_BYTES_LENGTH;
    pkt->hdr->size = size;
    //CLog::Log(LOGDEBUG, "[mpeg_add_header:%d]hdr_size=%d\n",__LINE__,size);
    if (1) {
        pkt->codec = &para->vcodec;
    } else {
        CLog::Log(LOGDEBUG, "[mpeg_add_header]invalid codec!");
        return PLAYER_EMPTY_P;
    }

    pkt->newflag = 1;
    return write_av_packet(para, pkt);
}

int pre_header_feeding(am_private_t *para, am_packet_t *pkt)
{
    int ret;
    if (para->stream_type == AM_STREAM_ES) {
        if (pkt->hdr == NULL) {
            pkt->hdr = (hdr_buf_t*)malloc(sizeof(hdr_buf_t));
            pkt->hdr->data = (char *)malloc(HDR_BUF_SIZE);
            if (!pkt->hdr->data) {
                //CLog::Log(LOGDEBUG, "[pre_header_feeding] NOMEM!");
                return PLAYER_NOMEM;
            }
        }

        if (VFORMAT_H264 == para->video_format || VFORMAT_H264_4K2K == para->video_format) {
            ret = h264_write_header(para, pkt);
            if (ret != PLAYER_SUCCESS) {
                return ret;
            }
        } else if ((VFORMAT_MPEG4 == para->video_format) && (VIDEO_DEC_FORMAT_MPEG4_3 == para->video_codec_type)) {
            ret = divx3_write_header(para, pkt);
            if (ret != PLAYER_SUCCESS) {
                return ret;
            }
        } else if ((CODEC_TAG_M4S2 == para->video_codec_tag)
                || (CODEC_TAG_DX50 == para->video_codec_tag)
                || (CODEC_TAG_mp4v == para->video_codec_tag)) {
            ret = m4s2_dx50_mp4v_write_header(para, pkt);
            if (ret != PLAYER_SUCCESS) {
                return ret;
            }
        /*
        } else if ((AVI_FILE == para->file_type)
                && (VIDEO_DEC_FORMAT_MPEG4_3 != para->vstream_info.video_codec_type)
                && (VFORMAT_H264 != para->vstream_info.video_format)
                && (VFORMAT_VC1 != para->vstream_info.video_format)) {
            ret = avi_write_header(para);
            if (ret != PLAYER_SUCCESS) {
                return ret;
            }
        */
        } else if (CODEC_TAG_WMV3 == para->video_codec_tag) {
            CLog::Log(LOGDEBUG, "CODEC_TAG_WMV3 == para->video_codec_tag");
            ret = wmv3_write_header(para, pkt);
            if (ret != PLAYER_SUCCESS) {
                return ret;
            }
        } else if ((CODEC_TAG_WVC1 == para->video_codec_tag)
                || (CODEC_TAG_VC_1 == para->video_codec_tag)
                || (CODEC_TAG_WMVA == para->video_codec_tag)) {
            CLog::Log(LOGDEBUG, "CODEC_TAG_WVC1 == para->video_codec_tag");
            ret = wvc1_write_header(para, pkt);
            if (ret != PLAYER_SUCCESS) {
                return ret;
            }
        /*
        } else if ((MKV_FILE == para->file_type) &&
                  ((VFORMAT_MPEG4 == para->vstream_info.video_format)
                || (VFORMAT_MPEG12 == para->vstream_info.video_format))) {
            ret = mkv_write_header(para, pkt);
            if (ret != PLAYER_SUCCESS) {
                return ret;
            }
        */
        } else if (VFORMAT_MJPEG == para->video_format) {
            ret = mjpeg_write_header(para, pkt);
            if (ret != PLAYER_SUCCESS) {
                return ret;
            }
        } else if (VFORMAT_HEVC == para->video_format) {
            ret = hevc_write_header(para, pkt);
            if (ret != PLAYER_SUCCESS) {
                return ret;
            }
        }

        if (pkt->hdr) {
            if (pkt->hdr->data) {
                free(pkt->hdr->data);
                pkt->hdr->data = NULL;
            }
            free(pkt->hdr);
            pkt->hdr = NULL;
        }
    }
    else if (para->stream_type == AM_STREAM_PS) {
        if (pkt->hdr == NULL) {
            pkt->hdr = (hdr_buf_t*)malloc(sizeof(hdr_buf_t));
            pkt->hdr->data = (char*)malloc(HDR_BUF_SIZE);
            if (!pkt->hdr->data) {
                CLog::Log(LOGDEBUG, "[pre_header_feeding] NOMEM!");
                return PLAYER_NOMEM;
            }
        }
        if (( AV_CODEC_ID_MPEG1VIDEO == para->video_codec_id)
          || (AV_CODEC_ID_MPEG2VIDEO == para->video_codec_id)
          || (AV_CODEC_ID_MPEG2VIDEO_XVMC == para->video_codec_id)) {
            ret = mpeg_add_header(para, pkt);
            if (ret != PLAYER_SUCCESS) {
                return ret;
            }
        }
        if (pkt->hdr) {
            if (pkt->hdr->data) {
                free(pkt->hdr->data);
                pkt->hdr->data = NULL;
            }
            free(pkt->hdr);
            pkt->hdr = NULL;
        }
    }
    return PLAYER_SUCCESS;
}

int divx3_prefix(am_packet_t *pkt)
{
#define DIVX311_CHUNK_HEAD_SIZE 13
    const unsigned char divx311_chunk_prefix[DIVX311_CHUNK_HEAD_SIZE] = {
        0x00, 0x00, 0x00, 0x01, 0xb6, 'D', 'I', 'V', 'X', '3', '.', '1', '1'
    };
    if ((pkt->hdr != NULL) && (pkt->hdr->data != NULL)) {
        free(pkt->hdr->data);
        pkt->hdr->data = NULL;
    }

    if (pkt->hdr == NULL) {
        pkt->hdr = (hdr_buf_t*)malloc(sizeof(hdr_buf_t));
        if (!pkt->hdr) {
            CLog::Log(LOGDEBUG, "[divx3_prefix] NOMEM!");
            return PLAYER_FAILED;
        }

        pkt->hdr->data = NULL;
        pkt->hdr->size = 0;
    }

    pkt->hdr->data = (char*)malloc(DIVX311_CHUNK_HEAD_SIZE + 4);
    if (pkt->hdr->data == NULL) {
        CLog::Log(LOGDEBUG, "[divx3_prefix] NOMEM!");
        return PLAYER_FAILED;
    }

    memcpy(pkt->hdr->data, divx311_chunk_prefix, DIVX311_CHUNK_HEAD_SIZE);

    pkt->hdr->data[DIVX311_CHUNK_HEAD_SIZE + 0] = (pkt->data_size >> 24) & 0xff;
    pkt->hdr->data[DIVX311_CHUNK_HEAD_SIZE + 1] = (pkt->data_size >> 16) & 0xff;
    pkt->hdr->data[DIVX311_CHUNK_HEAD_SIZE + 2] = (pkt->data_size >>  8) & 0xff;
    pkt->hdr->data[DIVX311_CHUNK_HEAD_SIZE + 3] = pkt->data_size & 0xff;

    pkt->hdr->size = DIVX311_CHUNK_HEAD_SIZE + 4;
    pkt->newflag = 1;

    return PLAYER_SUCCESS;
}

int set_header_info(am_private_t *para)
{
  am_packet_t *pkt = &para->am_pkt;

  //if (pkt->newflag)
  {
    //if (pkt->hdr)
    //  pkt->hdr->size = 0;

    if (para->video_format == VFORMAT_MPEG4)
    {
      if (para->video_codec_type == VIDEO_DEC_FORMAT_MPEG4_3)
      {
        return divx3_prefix(pkt);
      }
      else if (para->video_codec_type == VIDEO_DEC_FORMAT_H263)
      {
        return PLAYER_UNSUPPORT;
        unsigned char *vld_buf;
        int vld_len, vld_buf_size = para->video_width * para->video_height * 2;

        if (!pkt->data_size) {
            return PLAYER_SUCCESS;
        }

        if ((pkt->data[0] == 0) && (pkt->data[1] == 0) && (pkt->data[2] == 1) && (pkt->data[3] == 0xb6)) {
            return PLAYER_SUCCESS;
        }

        vld_buf = (unsigned char*)malloc(vld_buf_size);
        if (!vld_buf) {
            return PLAYER_NOMEM;
        }

        if (para->flv_flag) {
            vld_len = para->m_dll->h263vld(pkt->data, vld_buf, pkt->data_size, 1);
        } else {
            if (0 == para->h263_decodable) {
                para->h263_decodable = para->m_dll->decodeble_h263(pkt->data);
                if (0 == para->h263_decodable) {
                    CLog::Log(LOGDEBUG, "[%s]h263 unsupport video and audio, exit", __FUNCTION__);
                    return PLAYER_UNSUPPORT;
                }
            }
            vld_len = para->m_dll->h263vld(pkt->data, vld_buf, pkt->data_size, 0);
        }

        if (vld_len > 0) {
            if (pkt->buf) {
                free(pkt->buf);
            }
            pkt->buf = vld_buf;
            pkt->buf_size = vld_buf_size;
            pkt->data = pkt->buf;
            pkt->data_size = vld_len;
        } else {
            free(vld_buf);
            pkt->data_size = 0;
        }
      }
    } else if (para->video_format == VFORMAT_VC1) {
        if (para->video_codec_type == VIDEO_DEC_FORMAT_WMV3) {
            unsigned i, check_sum = 0, data_len = 0;

            if ((pkt->hdr != NULL) && (pkt->hdr->data != NULL)) {
                free(pkt->hdr->data);
                pkt->hdr->data = NULL;
            }

            if (pkt->hdr == NULL) {
                pkt->hdr = (hdr_buf_t*)malloc(sizeof(hdr_buf_t));
                if (!pkt->hdr) {
                    return PLAYER_FAILED;
                }

                pkt->hdr->data = NULL;
                pkt->hdr->size = 0;
            }

            if (pkt->avpkt.flags) {
                pkt->hdr->data = (char*)malloc(para->extrasize + 26 + 22);
                if (pkt->hdr->data == NULL) {
                    return PLAYER_FAILED;
                }

                pkt->hdr->data[0] = 0;
                pkt->hdr->data[1] = 0;
                pkt->hdr->data[2] = 1;
                pkt->hdr->data[3] = 0x10;

                data_len = para->extrasize + 4;
                pkt->hdr->data[4] = 0;
                pkt->hdr->data[5] = (data_len >> 16) & 0xff;
                pkt->hdr->data[6] = 0x88;
                pkt->hdr->data[7] = (data_len >> 8) & 0xff;
                pkt->hdr->data[8] =  data_len & 0xff;
                pkt->hdr->data[9] = 0x88;

                pkt->hdr->data[10] = 0xff;
                pkt->hdr->data[11] = 0xff;
                pkt->hdr->data[12] = 0x88;
                pkt->hdr->data[13] = 0xff;
                pkt->hdr->data[14] = 0xff;
                pkt->hdr->data[15] = 0x88;

                for (i = 4 ; i < 16 ; i++) {
                    check_sum += pkt->hdr->data[i];
                }

                pkt->hdr->data[16] = (check_sum >> 8) & 0xff;
                pkt->hdr->data[17] =  check_sum & 0xff;
                pkt->hdr->data[18] = 0x88;
                pkt->hdr->data[19] = (check_sum >> 8) & 0xff;
                pkt->hdr->data[20] =  check_sum & 0xff;
                pkt->hdr->data[21] = 0x88;

                pkt->hdr->data[22] = (para->video_width  >> 8) & 0xff;
                pkt->hdr->data[23] =  para->video_width  & 0xff;
                pkt->hdr->data[24] = (para->video_height >> 8) & 0xff;
                pkt->hdr->data[25] =  para->video_height & 0xff;

                memcpy(pkt->hdr->data + 26, para->extradata, para->extrasize);

                check_sum = 0;
                data_len = para->extrasize + 26;
            } else {
                pkt->hdr->data = (char*)malloc(22);
                if (pkt->hdr->data == NULL) {
                    return PLAYER_FAILED;
                }
            }

            pkt->hdr->data[data_len + 0]  = 0;
            pkt->hdr->data[data_len + 1]  = 0;
            pkt->hdr->data[data_len + 2]  = 1;
            pkt->hdr->data[data_len + 3]  = 0xd;

            pkt->hdr->data[data_len + 4]  = 0;
            pkt->hdr->data[data_len + 5]  = (pkt->data_size >> 16) & 0xff;
            pkt->hdr->data[data_len + 6]  = 0x88;
            pkt->hdr->data[data_len + 7]  = (pkt->data_size >> 8) & 0xff;
            pkt->hdr->data[data_len + 8]  =  pkt->data_size & 0xff;
            pkt->hdr->data[data_len + 9]  = 0x88;

            pkt->hdr->data[data_len + 10] = 0xff;
            pkt->hdr->data[data_len + 11] = 0xff;
            pkt->hdr->data[data_len + 12] = 0x88;
            pkt->hdr->data[data_len + 13] = 0xff;
            pkt->hdr->data[data_len + 14] = 0xff;
            pkt->hdr->data[data_len + 15] = 0x88;

            for (i = data_len + 4 ; i < data_len + 16 ; i++) {
                check_sum += pkt->hdr->data[i];
            }

            pkt->hdr->data[data_len + 16] = (check_sum >> 8) & 0xff;
            pkt->hdr->data[data_len + 17] =  check_sum & 0xff;
            pkt->hdr->data[data_len + 18] = 0x88;
            pkt->hdr->data[data_len + 19] = (check_sum >> 8) & 0xff;
            pkt->hdr->data[data_len + 20] =  check_sum & 0xff;
            pkt->hdr->data[data_len + 21] = 0x88;

            pkt->hdr->size = data_len + 22;
            pkt->newflag = 1;
        } else if (para->video_codec_type == VIDEO_DEC_FORMAT_WVC1) {
            if ((pkt->hdr != NULL) && (pkt->hdr->data != NULL)) {
                free(pkt->hdr->data);
                pkt->hdr->data = NULL;
            }

            if (pkt->hdr == NULL) {
                pkt->hdr = (hdr_buf_t*)malloc(sizeof(hdr_buf_t));
                if (!pkt->hdr) {
                    CLog::Log(LOGDEBUG, "[wvc1_prefix] NOMEM!");
                    return PLAYER_FAILED;
                }

                pkt->hdr->data = NULL;
                pkt->hdr->size = 0;
            }

            pkt->hdr->data = (char*)malloc(4);
            if (pkt->hdr->data == NULL) {
                CLog::Log(LOGDEBUG, "[wvc1_prefix] NOMEM!");
                return PLAYER_FAILED;
            }

            pkt->hdr->data[0] = 0;
            pkt->hdr->data[1] = 0;
            pkt->hdr->data[2] = 1;
            pkt->hdr->data[3] = 0xd;
            pkt->hdr->size = 4;
            pkt->newflag = 1;
        }
    }
  }
  return PLAYER_SUCCESS;
}

/*************************************************************************/
CAMLCodec::CAMLCodec() : CThread("CAMLCodec")
{
  m_opened = false;
  am_private = new am_private_t;
  memset(am_private, 0, sizeof(am_private_t));
  m_dll = new DllLibAmCodec;
  m_dll->Load();
  am_private->m_dll = m_dll;
}


CAMLCodec::~CAMLCodec()
{
  StopThread();
  delete am_private;
  am_private = NULL;
  delete m_dll, m_dll = NULL;
}

bool CAMLCodec::OpenDecoder(CDVDStreamInfo &hints)
{
#ifdef TARGET_ANDROID
  CLog::Log(LOGDEBUG, "CAMLCodec::OpenDecoder, android version %d", CAndroidFeatures::GetVersion());
#endif

  m_speed = DVD_PLAYSPEED_NORMAL;
  m_1st_pts = 0;
  m_cur_pts = 0;
  m_cur_pictcnt = 0;
  m_old_pictcnt = 0;
  m_dst_rect.SetRect(0, 0, 0, 0);
  m_zoom = -1;
  m_contrast = -1;
  m_brightness = -1;
  m_vbufsize = 500000 * 2;
  m_start_dts = 0;
  m_start_pts = 0;
  m_hints = hints;

  ShowMainVideo(false);

  am_packet_init(&am_private->am_pkt);
  // default stream type
  am_private->stream_type      = AM_STREAM_ES;
  // handle hints.
  am_private->video_width      = hints.width;
  am_private->video_height     = hints.height;
  am_private->video_codec_id   = hints.codec;
  am_private->video_codec_tag  = hints.codec_tag;
  am_private->video_pid        = hints.pid;

  // handle video ratio
  AVRational video_ratio       = m_dll->av_d2q(1, SHRT_MAX);
  //if (!hints.forced_aspect)
  //  video_ratio = m_dll->av_d2q(hints.aspect, SHRT_MAX);
  am_private->video_ratio      = ((int32_t)video_ratio.num << 16) | video_ratio.den;
  am_private->video_ratio64    = ((int64_t)video_ratio.num << 32) | video_ratio.den;

  // handle video rate
  if (hints.rfpsrate > 0 && hints.rfpsscale != 0)
  {
    // check ffmpeg r_frame_rate 1st
    am_private->video_rate = 0.5 + (float)UNIT_FREQ * hints.rfpsscale / hints.rfpsrate;
  }
  else if (hints.fpsrate > 0 && hints.fpsscale != 0)
  {
    // then ffmpeg avg_frame_rate next
    am_private->video_rate = 0.5 + (float)UNIT_FREQ * hints.fpsscale / hints.fpsrate;
  }

  // check for 1920x1080, interlaced, 25 fps
  // incorrectly reported as 50 fps (yes, video_rate == 1920)
  if (hints.width == 1920 && am_private->video_rate == 1920)
  {
    CLog::Log(LOGDEBUG, "CAMLCodec::OpenDecoder video_rate exception");
    am_private->video_rate = 0.5 + (float)UNIT_FREQ * 1001 / 25000;
  }

  // check for SD h264 content incorrectly reported as 60 fsp
  // mp4/avi containers :(
  if (hints.codec == AV_CODEC_ID_H264 && hints.width <= 720 && am_private->video_rate == 1602)
  {
    CLog::Log(LOGDEBUG, "CAMLCodec::OpenDecoder video_rate exception");
    am_private->video_rate = 0.5 + (float)UNIT_FREQ * 1001 / 24000;
  }

  // check for SD h264 content incorrectly reported as some form of 30 fsp
  // mp4/avi containers :(
  if (hints.codec == AV_CODEC_ID_H264 && hints.width <= 720)
  {
    if (am_private->video_rate >= 3200 && am_private->video_rate <= 3210)
    {
      CLog::Log(LOGDEBUG, "CAMLCodec::OpenDecoder video_rate exception");
      am_private->video_rate = 0.5 + (float)UNIT_FREQ * 1001 / 24000;
    }
  }

  // handle orientation
  am_private->video_rotation_degree = 0;
  if (hints.orientation == 90)
    am_private->video_rotation_degree = 1;
  else if (hints.orientation == 180)
    am_private->video_rotation_degree = 2;
  else if (hints.orientation == 270)
    am_private->video_rotation_degree = 3;
  // handle extradata
  am_private->video_format      = codecid_to_vformat(hints.codec);
  if (am_private->video_format == VFORMAT_H264) {
      if (hints.width > 1920 || hints.height > 1088) {
        am_private->video_format = VFORMAT_H264_4K2K;
      }
  }
  switch (am_private->video_format)
  {
    default:
      am_private->extrasize       = hints.extrasize;
      am_private->extradata       = (uint8_t*)malloc(hints.extrasize);
      memcpy(am_private->extradata, hints.extradata, hints.extrasize);
      break;
    case VFORMAT_REAL:
    case VFORMAT_MPEG12:
      break;
  }

  if (am_private->stream_type == AM_STREAM_ES && am_private->video_codec_tag != 0)
    am_private->video_codec_type = codec_tag_to_vdec_type(am_private->video_codec_tag);
  if (am_private->video_codec_type == VIDEO_DEC_FORMAT_UNKNOW)
    am_private->video_codec_type = codec_tag_to_vdec_type(am_private->video_codec_id);

  am_private->flv_flag = 0;
  if (am_private->video_codec_id == AV_CODEC_ID_FLV1)
  {
    am_private->video_codec_tag = CODEC_TAG_F263;
    am_private->flv_flag = 1;
  }

  CLog::Log(LOGDEBUG, "CAMLCodec::OpenDecoder "
    "hints.width(%d), hints.height(%d), hints.codec(%d), hints.codec_tag(%d), hints.pid(%d)",
    hints.width, hints.height, hints.codec, hints.codec_tag, hints.pid);
  CLog::Log(LOGDEBUG, "CAMLCodec::OpenDecoder hints.fpsrate(%d), hints.fpsscale(%d), hints.rfpsrate(%d), hints.rfpsscale(%d), video_rate(%d)",
    hints.fpsrate, hints.fpsscale, hints.rfpsrate, hints.rfpsscale, am_private->video_rate);
  CLog::Log(LOGDEBUG, "CAMLCodec::OpenDecoder hints.aspect(%f), video_ratio.num(%d), video_ratio.den(%d)",
    hints.aspect, video_ratio.num, video_ratio.den);
  CLog::Log(LOGDEBUG, "CAMLCodec::OpenDecoder hints.orientation(%d), hints.forced_aspect(%d), hints.extrasize(%d)",
    hints.orientation, hints.forced_aspect, hints.extrasize);

  // default video codec params
  am_private->gcodec.noblock     = 0;
  am_private->gcodec.video_pid   = am_private->video_pid;
  am_private->gcodec.video_type  = am_private->video_format;
  am_private->gcodec.stream_type = STREAM_TYPE_ES_VIDEO;
  am_private->gcodec.format      = am_private->video_codec_type;
  am_private->gcodec.width       = am_private->video_width;
  am_private->gcodec.height      = am_private->video_height;
  am_private->gcodec.rate        = am_private->video_rate;
  am_private->gcodec.ratio       = am_private->video_ratio;
  am_private->gcodec.ratio64     = am_private->video_ratio64;
  am_private->gcodec.param       = NULL;

  switch(am_private->video_format)
  {
    default:
      break;
    case VFORMAT_MPEG4:
      am_private->gcodec.param = (void*)EXTERNAL_PTS;
      break;
    case VFORMAT_H264:
    case VFORMAT_H264MVC:
      am_private->gcodec.format = VIDEO_DEC_FORMAT_H264;
      am_private->gcodec.param  = (void*)EXTERNAL_PTS;
      // h264 in an avi file
      if (m_hints.ptsinvalid)
        am_private->gcodec.param = (void*)(EXTERNAL_PTS | SYNC_OUTSIDE);
      break;
    case VFORMAT_H264_4K2K:
      am_private->gcodec.format = VIDEO_DEC_FORMAT_H264_4K2K;
      am_private->gcodec.param  = (void*)EXTERNAL_PTS;
      // h264 in an avi file
      if (m_hints.ptsinvalid)
        am_private->gcodec.param = (void*)(EXTERNAL_PTS | SYNC_OUTSIDE);
      break; 
    case VFORMAT_REAL:
      am_private->stream_type = AM_STREAM_RM;
      am_private->vcodec.noblock = 1;
      am_private->vcodec.stream_type = STREAM_TYPE_RM;
      am_private->vcodec.am_sysinfo.ratio = 0x100;
      am_private->vcodec.am_sysinfo.ratio64 = 0;
      {
        static unsigned short tbl[9] = {0};
        if (VIDEO_DEC_FORMAT_REAL_8 == am_private->video_codec_type)
        {
          am_private->gcodec.extra = am_private->extradata[1] & 7;
          tbl[0] = (((am_private->gcodec.width  >> 2) - 1) << 8)
                 | (((am_private->gcodec.height >> 2) - 1) & 0xff);
          unsigned int j;
          for (unsigned int i = 1; i <= am_private->gcodec.extra; i++)
          {
            j = 2 * (i - 1);
            tbl[i] = ((am_private->extradata[8 + j] - 1) << 8) | ((am_private->extradata[8 + j + 1] - 1) & 0xff);
          }
        }
        am_private->gcodec.param = &tbl;
      }
      break;
    case VFORMAT_VC1:
      // vc1 in an avi file
      if (m_hints.ptsinvalid)
        am_private->gcodec.param = (void*)EXTERNAL_PTS;
      break;
    case VFORMAT_HEVC:
      am_private->gcodec.format = VIDEO_DEC_FORMAT_HEVC;
      am_private->gcodec.param  = (void*)EXTERNAL_PTS;
      if (m_hints.ptsinvalid)
        am_private->gcodec.param = (void*)(EXTERNAL_PTS | SYNC_OUTSIDE);
      break;
  }
  am_private->gcodec.param = (void *)((unsigned int)am_private->gcodec.param | (am_private->video_rotation_degree << 16));

  // translate from generic to firemware version dependent
  m_dll->codec_init_para(&am_private->gcodec, &am_private->vcodec);

  int ret = m_dll->codec_init(&am_private->vcodec);
  if (ret != CODEC_ERROR_NONE)
  {
    CLog::Log(LOGDEBUG, "CAMLCodec::OpenDecoder codec init failed, ret=0x%x", -ret);
    return false;
  }

  am_private->dumpdemux = false;
  dumpfile_open(am_private);

  // make sure we are not stuck in pause (amcodec bug)
  m_dll->codec_resume(&am_private->vcodec);
  m_dll->codec_set_cntl_mode(&am_private->vcodec, TRICKMODE_NONE);

  m_dll->codec_set_cntl_avthresh(&am_private->vcodec, AV_SYNC_THRESH);
  m_dll->codec_set_cntl_syncthresh(&am_private->vcodec, 0);
  // disable tsync, we are playing video disconnected from audio.
  SysfsUtils::SetInt("/sys/class/tsync/enable", 0);

  am_private->am_pkt.codec = &am_private->vcodec;
  pre_header_feeding(am_private, &am_private->am_pkt);

  Create();

  g_renderManager.RegisterRenderUpdateCallBack((const void*)this, RenderUpdateCallBack);
  g_renderManager.RegisterRenderFeaturesCallBack((const void*)this, RenderFeaturesCallBack);

  m_display_rect = CRect(0, 0, CDisplaySettings::Get().GetCurrentResolutionInfo().iWidth, CDisplaySettings::Get().GetCurrentResolutionInfo().iHeight);

  std::string strScaler;
  SysfsUtils::GetString("/sys/class/ppmgr/ppscaler", strScaler);
  if (strScaler.find("enabled") == std::string::npos)     // Scaler not enabled, use screen size
    m_display_rect = CRect(0, 0, CDisplaySettings::Get().GetCurrentResolutionInfo().iScreenWidth, CDisplaySettings::Get().GetCurrentResolutionInfo().iScreenHeight);

/*
  // if display is set to 1080xxx, then disable deinterlacer for HD content
  // else bandwidth usage is too heavy and it will slow down video decoder.
  char display_mode[256] = {0};
  SysfsUtils::GetString("/sys/class/display/mode", display_mode, 255);
  if (strstr(display_mode,"1080"))
    SysfsUtils::SetInt("/sys/module/di/parameters/bypass_all", 1);
  else
    SysfsUtils::SetInt("/sys/module/di/parameters/bypass_all", 0);
*/

  m_opened = true;
  // vcodec is open, update speed if it was
  // changed before dvdplayer called OpenDecoder.
  SetSpeed(m_speed);

  return true;
}

void CAMLCodec::CloseDecoder()
{
  CLog::Log(LOGDEBUG, "CAMLCodec::CloseDecoder");
  StopThread();

  g_renderManager.RegisterRenderUpdateCallBack((const void*)NULL, NULL);
  g_renderManager.RegisterRenderFeaturesCallBack((const void*)NULL, NULL);

  // never leave vcodec ff/rw or paused.
  if (m_speed != DVD_PLAYSPEED_NORMAL)
  {
    m_dll->codec_resume(&am_private->vcodec);
    m_dll->codec_set_cntl_mode(&am_private->vcodec, TRICKMODE_NONE);
  }
  m_dll->codec_close(&am_private->vcodec);
  dumpfile_close(am_private);
  m_opened = false;

  am_packet_release(&am_private->am_pkt);
  free(am_private->extradata);
  am_private->extradata = NULL;
  // return tsync to default so external apps work
  SysfsUtils::SetInt("/sys/class/tsync/enable", 1);

  ShowMainVideo(false);

  // add a little delay after closing in case
  // we are reopened too fast.
  usleep(500 * 1000);
}

void CAMLCodec::Reset()
{
  CLog::Log(LOGDEBUG, "CAMLCodec::Reset");

  if (!m_opened)
    return;

  // set the system blackout_policy to leave the last frame showing
  int blackout_policy;
  SysfsUtils::GetInt("/sys/class/video/blackout_policy", blackout_policy);
  SysfsUtils::SetInt("/sys/class/video/blackout_policy", 0);

  // restore the speed (some amcodec versions require this)
  if (m_speed != DVD_PLAYSPEED_NORMAL)
  {
    m_dll->codec_resume(&am_private->vcodec);
    m_dll->codec_set_cntl_mode(&am_private->vcodec, TRICKMODE_NONE);
  }
  // reset the decoder
  m_dll->codec_reset(&am_private->vcodec);
  dumpfile_close(am_private);
  dumpfile_open(am_private);

  // re-init our am_pkt
  am_packet_release(&am_private->am_pkt);
  am_packet_init(&am_private->am_pkt);
  am_private->am_pkt.codec = &am_private->vcodec;
  pre_header_feeding(am_private, &am_private->am_pkt);

  // restore the saved system blackout_policy value
  SysfsUtils::SetInt("/sys/class/video/blackout_policy", blackout_policy);

  // reset some interal vars
  m_1st_pts = 0;
  m_cur_pts = 0;
  m_cur_pictcnt = 0;
  m_old_pictcnt = 0;
  SetSpeed(m_speed);
}

int CAMLCodec::Decode(uint8_t *pData, size_t iSize, double dts, double pts)
{
  if (!m_opened)
    return VC_BUFFER;

  // grr, m_RenderUpdateCallBackFn in g_renderManager is NULL'ed during
  // g_renderManager.Configure call by player, which happens after the codec
  // OpenDecoder call. So we need to restore it but it does not seem to stick :)
  g_renderManager.RegisterRenderUpdateCallBack((const void*)this, RenderUpdateCallBack);

  if (pData)
  {
    am_private->am_pkt.data = pData;
    am_private->am_pkt.data_size = iSize;

    am_private->am_pkt.newflag    = 1;
    am_private->am_pkt.isvalid    = 1;
    am_private->am_pkt.avduration = 0;

    // handle pts, including 31bit wrap, aml can only handle 31
    // bit pts as it uses an int in kernel.
    if (m_hints.ptsinvalid || pts == DVD_NOPTS_VALUE)
      am_private->am_pkt.avpts = AV_NOPTS_VALUE;
    else
    {
      am_private->am_pkt.avpts = 0.5 + (pts * PTS_FREQ) / DVD_TIME_BASE;\
      if (!m_start_pts && am_private->am_pkt.avpts >= 0x7fffffff)
        m_start_pts = am_private->am_pkt.avpts & ~0x0000ffff;
    }
    if (am_private->am_pkt.avpts != (int64_t)AV_NOPTS_VALUE)
      am_private->am_pkt.avpts -= m_start_pts;


    // handle dts, including 31bit wrap, aml can only handle 31
    // bit dts as it uses an int in kernel.
    if (dts == DVD_NOPTS_VALUE)
      am_private->am_pkt.avdts = AV_NOPTS_VALUE;
    else
    {
      am_private->am_pkt.avdts = 0.5 + (dts * PTS_FREQ) / DVD_TIME_BASE;
      if (!m_start_dts && am_private->am_pkt.avdts >= 0x7fffffff)
        m_start_dts = am_private->am_pkt.avdts & ~0x0000ffff;
    }
    if (am_private->am_pkt.avdts != (int64_t)AV_NOPTS_VALUE)
      am_private->am_pkt.avdts -= m_start_dts;

    //CLog::Log(LOGDEBUG, "CAMLCodec::Decode: iSize(%d), dts(%f), pts(%f), avdts(%llx), avpts(%llx)",
    //  iSize, dts, pts, am_private->am_pkt.avdts, am_private->am_pkt.avpts);

    // some formats need header/data tweaks.
    // the actual write occurs once in write_av_packet
    // and is controlled by am_pkt.newflag.
    set_header_info(am_private);

    // loop until we write all into codec, am_pkt.isvalid
    // will get set to zero once everything is consumed.
    // PLAYER_SUCCESS means all is ok, not all bytes were written.
    while (am_private->am_pkt.isvalid)
    {
      // abort on any errors.
      if (write_av_packet(am_private, &am_private->am_pkt) != PLAYER_SUCCESS)
        break;

      if (am_private->am_pkt.isvalid)
        CLog::Log(LOGDEBUG, "CAMLCodec::Decode: write_av_packet looping");
    }

    // if we seek, then GetTimeSize is wrong as
    // reports lastpts - cur_pts and hw decoder has
    // not started outputing new pts values yet.
    // so we grab the 1st pts sent into driver and
    // use that to calc GetTimeSize.
    if (m_1st_pts == 0)
      m_1st_pts = am_private->am_pkt.lastpts;
  }

  // if we have still frames, demux size will be small
  // and we need to pre-buffer more.
  double target_timesize = 1.0;
  if (iSize < 20)
    target_timesize = 2.0;

  // keep hw buffered demux above 1 second
  if (GetTimeSize() < target_timesize && m_speed == DVD_PLAYSPEED_NORMAL)
    return VC_BUFFER;

  // wait until we get a new frame or 25ms,
  if (m_old_pictcnt == m_cur_pictcnt)
    m_ready_event.WaitMSec(25);

  // we must return VC_BUFFER or VC_PICTURE,
  // default to VC_BUFFER.
  int rtn = VC_BUFFER;
  if (m_old_pictcnt != m_cur_pictcnt)
  {
    m_old_pictcnt++;
    rtn = VC_PICTURE;
    // we got a new pict, try and keep hw buffered demux above 2 seconds.
    // this, combined with the above 1 second check, keeps hw buffered demux between 1 and 2 seconds.
    // we also check to make sure we keep from filling hw buffer.
    if (GetTimeSize() < 2.0 && GetDataSize() < m_vbufsize/3)
      rtn |= VC_BUFFER;
  }
/*
  CLog::Log(LOGDEBUG, "CAMLCodec::Decode: "
    "rtn(%d), m_cur_pictcnt(%lld), m_cur_pts(%f), lastpts(%f), GetTimeSize(%f), GetDataSize(%d)",
    rtn, m_cur_pictcnt, (float)m_cur_pts/PTS_FREQ, (float)am_private->am_pkt.lastpts/PTS_FREQ, GetTimeSize(), GetDataSize());
*/
  return rtn;
}

bool CAMLCodec::GetPicture(DVDVideoPicture *pDvdVideoPicture)
{
  if (!m_opened)
    return false;

  pDvdVideoPicture->iFlags = DVP_FLAG_ALLOCATED;
  pDvdVideoPicture->format = RENDER_FMT_BYPASS;
  pDvdVideoPicture->iDuration = (double)(am_private->video_rate * DVD_TIME_BASE) / UNIT_FREQ;

  pDvdVideoPicture->dts = DVD_NOPTS_VALUE;
  if (m_speed == DVD_PLAYSPEED_NORMAL)
  {
    pDvdVideoPicture->pts = GetPlayerPtsSeconds() * (double)DVD_TIME_BASE;
    // video pts cannot be late or dvdplayer goes nuts,
    // so run it one frame ahead
    pDvdVideoPicture->pts += 1 * pDvdVideoPicture->iDuration;
  }
  else
  {
    // We are FF/RW; Do not use the Player clock or it just doesn't work
    if (m_cur_pts == 0)
      pDvdVideoPicture->pts = (double)m_1st_pts / PTS_FREQ * DVD_TIME_BASE;
    else
      pDvdVideoPicture->pts = (double)m_cur_pts / PTS_FREQ * DVD_TIME_BASE;
  }

  return true;
}

void CAMLCodec::SetSpeed(int speed)
{
  CLog::Log(LOGDEBUG, "CAMLCodec::SetSpeed, speed(%d)", speed);

  // update internal vars regardless
  // of if we are open or not.
  m_speed = speed;

  if (!m_opened)
    return;

  switch(speed)
  {
    case DVD_PLAYSPEED_PAUSE:
      m_dll->codec_pause(&am_private->vcodec);
      m_dll->codec_set_cntl_mode(&am_private->vcodec, TRICKMODE_NONE);
      break;
    case DVD_PLAYSPEED_NORMAL:
      m_dll->codec_resume(&am_private->vcodec);
      m_dll->codec_set_cntl_mode(&am_private->vcodec, TRICKMODE_NONE);
      break;
    default:
      m_dll->codec_resume(&am_private->vcodec);
      if ((am_private->video_format == VFORMAT_H264) || (am_private->video_format == VFORMAT_H264_4K2K))
        m_dll->codec_set_cntl_mode(&am_private->vcodec, TRICKMODE_FFFB);
      else
        m_dll->codec_set_cntl_mode(&am_private->vcodec, TRICKMODE_I);
      break;
  }
}

int CAMLCodec::GetDataSize()
{
  if (!m_opened)
    return 0;

  struct buf_status vbuf ={0};
  if (m_dll->codec_get_vbuf_state(&am_private->vcodec, &vbuf) >= 0)
    m_vbufsize = vbuf.size;

  return vbuf.data_len;
}

double CAMLCodec::GetTimeSize()
{
  if (!m_opened)
    return 0;

  // if m_cur_pts is zero, hw decoder was not started yet
  // so we use the pts of the 1st demux packet that was send
  // to hw decoder to calc timesize.
  if (m_cur_pts == 0)
    m_timesize = (double)(am_private->am_pkt.lastpts - m_1st_pts) / PTS_FREQ;
  else
    m_timesize = (double)(am_private->am_pkt.lastpts - m_cur_pts) / PTS_FREQ;

  // lie to DVDPlayer, it is hardcoded to a max of 8 seconds,
  // if you buffer more than 8 seconds, it goes nuts.
  double timesize = m_timesize;
  if (timesize < 0.0)
    timesize = 0.0;
  else if (timesize > 7.0)
    timesize = 7.0;

  return timesize;
}

void CAMLCodec::Process()
{
  CLog::Log(LOGDEBUG, "CAMLCodec::Process Started");

  // bump our priority to be level with SoftAE
  SetPriority(THREAD_PRIORITY_ABOVE_NORMAL);
  while (!m_bStop)
  {
    int64_t pts_video = 0;
    if (am_private->am_pkt.lastpts > 0)
    {
      // this is a blocking poll that returns every vsync.
      // since we are running at a higher priority, make sure
      // we sleep if the call fails or does a timeout.
      if (m_dll->codec_poll_cntl(&am_private->vcodec) < 0)
      {
        CLog::Log(LOGDEBUG, "CAMLCodec::Process: codec_poll_cntl failed");
        Sleep(10);
      }

      pts_video = get_pts_video();
      if (m_cur_pts != pts_video)
      {
        //CLog::Log(LOGDEBUG, "CAMLCodec::Process: pts_video(%lld), pts_video/PTS_FREQ(%f), duration(%f)",
        //  pts_video, (double)pts_video/PTS_FREQ, 1.0/((double)(pts_video - m_cur_pts)/PTS_FREQ));

        // other threads look at these, do them first
        m_cur_pts = pts_video;
        m_cur_pictcnt++;
        m_ready_event.Set();

        // correct video pts by starting pts.
        if (m_start_pts != 0)
          pts_video += m_start_pts;
        else if (m_start_dts != 0)
          pts_video += m_start_dts;

        double app_pts = GetPlayerPtsSeconds();
        // add in audio delay/display latency contribution
        double offset  = g_renderManager.GetDisplayLatency() - CMediaSettings::Get().GetCurrentVideoSettings().m_AudioDelay;
        // correct video pts by user set delay and rendering delay
        app_pts += offset;

        //CLog::Log(LOGDEBUG, "CAMLCodec::Process: app_pts(%f), pts_video/PTS_FREQ(%f)",
        //  app_pts, (double)pts_video/PTS_FREQ);

        double error = app_pts - (double)pts_video/PTS_FREQ;
        double abs_error = fabs(error);
        if (abs_error > 0.125)
        {
          //CLog::Log(LOGDEBUG, "CAMLCodec::Process pts diff = %f", error);
          if (abs_error > 0.150)
          {
            // big error so try to reset pts_pcrscr
            SetVideoPtsSeconds(app_pts);
          }
          else
          {
            // small error so try to avoid a frame jump
            SetVideoPtsSeconds((double)pts_video/PTS_FREQ + error/4);
          }
        }
      }
    }
    else
    {
      Sleep(100);
    }
  }
  SetPriority(THREAD_PRIORITY_NORMAL);
  CLog::Log(LOGDEBUG, "CAMLCodec::Process Stopped");
}

double CAMLCodec::GetPlayerPtsSeconds()
{
  double clock_pts = 0.0;
  CDVDClock *playerclock = CDVDClock::GetMasterClock();
  if (playerclock)
    clock_pts = playerclock->GetClock() / DVD_TIME_BASE;

  return clock_pts;
}

void CAMLCodec::SetVideoPtsSeconds(const double pts)
{
  //CLog::Log(LOGDEBUG, "CAMLCodec::SetVideoPtsSeconds: pts(%f)", pts);
  if (pts >= 0.0)
  {
    int64_t pts_video = (int64_t)(pts * PTS_FREQ);
    if (m_start_pts != 0)
      pts_video -= m_start_pts;
    else if (m_start_dts != 0)
      pts_video -= m_start_dts;

    set_pts_pcrscr(pts_video);
  }
}

void CAMLCodec::ShowMainVideo(const bool show)
{
  static int saved_disable_video = -1;

  int disable_video = show ? 0:1;
  if (saved_disable_video == disable_video)
    return;

  SysfsUtils::SetInt("/sys/class/video/disable_video", disable_video);
  saved_disable_video = disable_video;
}

void CAMLCodec::SetVideoZoom(const float zoom)
{
  // input zoom range is 0.5 to 2.0 with a default of 1.0.
  // output zoom range is 2 to 300 with default of 100.
  // we limit that to a range of 50 to 200 with default of 100.
  SysfsUtils::SetInt("/sys/class/video/zoom", (int)(100 * zoom));
}

void CAMLCodec::SetVideoContrast(const int contrast)
{
  // input contrast range is 0 to 100 with default of 50.
  // output contrast range is -255 to 255 with default of 0.
  int aml_contrast = (255 * (contrast - 50)) / 50;
  SysfsUtils::SetInt("/sys/class/video/contrast", aml_contrast);
}
void CAMLCodec::SetVideoBrightness(const int brightness)
{
  // input brightness range is 0 to 100 with default of 50.
  // output brightness range is -127 to 127 with default of 0.
  int aml_brightness = (127 * (brightness - 50)) / 50;
  SysfsUtils::SetInt("/sys/class/video/brightness", aml_brightness);
}
void CAMLCodec::SetVideoSaturation(const int saturation)
{
  // output saturation range is -127 to 127 with default of 127.
  SysfsUtils::SetInt("/sys/class/video/saturation", saturation);
}

void CAMLCodec::GetRenderFeatures(Features &renderFeatures)
{
  renderFeatures.push_back(RENDERFEATURE_ZOOM);
  renderFeatures.push_back(RENDERFEATURE_CONTRAST);
  renderFeatures.push_back(RENDERFEATURE_BRIGHTNESS);
  renderFeatures.push_back(RENDERFEATURE_STRETCH);
  renderFeatures.push_back(RENDERFEATURE_PIXEL_RATIO);
  renderFeatures.push_back(RENDERFEATURE_ROTATION);
  return;
}

void CAMLCodec::SetVideo3dMode(const int mode3d)
{
  CLog::Log(LOGDEBUG, "CAMLCodec::SetVideo3dMode:mode3d(0x%x)", mode3d);
  SysfsUtils::SetInt("/sys/class/ppmgr/ppmgr_3d_mode", mode3d);
}

std::string CAMLCodec::GetStereoMode()
{
  std::string  stereo_mode;

  switch(CMediaSettings::Get().GetCurrentVideoSettings().m_StereoMode)
  {
    case RENDER_STEREO_MODE_SPLIT_VERTICAL:   stereo_mode = "left_right"; break;
    case RENDER_STEREO_MODE_SPLIT_HORIZONTAL: stereo_mode = "top_bottom"; break;
    default:                                  stereo_mode = m_hints.stereo_mode; break;
  }

  if(CMediaSettings::Get().GetCurrentVideoSettings().m_StereoInvert)
    stereo_mode = RenderManager::GetStereoModeInvert(stereo_mode);
  return stereo_mode;
}

void CAMLCodec::RenderFeaturesCallBack(const void *ctx, Features &renderFeatures)
{
  CAMLCodec *codec = (CAMLCodec*)ctx;
  if (codec)
    codec->GetRenderFeatures(renderFeatures);
}

void CAMLCodec::SetVideoRect(const CRect &SrcRect, const CRect &DestRect)
{
  // this routine gets called every video frame
  // and is in the context of the renderer thread so
  // do not do anything stupid here.
  bool update = false;

  // video zoom adjustment.
  float zoom = CMediaSettings::Get().GetCurrentVideoSettings().m_CustomZoomAmount;
  if ((int)(zoom * 1000) != (int)(m_zoom * 1000))
  {
    m_zoom = zoom;
  }
  // video contrast adjustment.
  int contrast = CMediaSettings::Get().GetCurrentVideoSettings().m_Contrast;
  if (contrast != m_contrast)
  {
    SetVideoContrast(contrast);
    m_contrast = contrast;
  }
  // video brightness adjustment.
  int brightness = CMediaSettings::Get().GetCurrentVideoSettings().m_Brightness;
  if (brightness != m_brightness)
  {
    SetVideoBrightness(brightness);
    m_brightness = brightness;
  }

  // video view mode
  int view_mode = CMediaSettings::Get().GetCurrentVideoSettings().m_ViewMode;
  if (m_view_mode != view_mode)
  {
    m_view_mode = view_mode;
    update = true;
  }

  // video stereo mode/view.
  RENDER_STEREO_MODE stereo_mode = g_graphicsContext.GetStereoMode();
  if (m_stereo_mode != stereo_mode)
  {
    m_stereo_mode = stereo_mode;
    update = true;
  }
  RENDER_STEREO_VIEW stereo_view = g_graphicsContext.GetStereoView();
  if (m_stereo_view != stereo_view)
  {
    // left/right/top/bottom eye,
    // this might change every other frame.
    // we do not care but just track it.
    m_stereo_view = stereo_view;
  }

  // dest_rect
  CRect dst_rect = DestRect;
  // handle orientation
  switch (am_private->video_rotation_degree)
  {
    case 0:
    case 2:
      break;

    case 1:
    case 3:
      {
        int diff = (int) ((dst_rect.Height() - dst_rect.Width()) / 2);
        dst_rect = CRect(DestRect.x1 - diff, DestRect.y1, DestRect.x2 + diff, DestRect.y2);
      }

  }

  if (m_dst_rect != dst_rect)
  {
    m_dst_rect  = dst_rect;
    update = true;
  }

  if (!update)
  {
    // mainvideo 'should' be showing already if we get here, make sure.
    ShowMainVideo(true);
    return;
  }

  CRect gui, display;
  gui = CRect(0, 0, CDisplaySettings::Get().GetCurrentResolutionInfo().iWidth, CDisplaySettings::Get().GetCurrentResolutionInfo().iHeight);

#ifdef TARGET_ANDROID
  display = m_display_rect;
#else
  display = gui;
#endif
  if (gui != display)
  {
    float xscale = display.Width() / gui.Width();
    float yscale = display.Height() / gui.Height();
    if (m_stereo_mode == RENDER_STEREO_MODE_SPLIT_VERTICAL)
      xscale /= 2.0;
    else if (m_stereo_mode == RENDER_STEREO_MODE_SPLIT_HORIZONTAL)
      yscale /= 2.0;
    dst_rect.x1 *= xscale;
    dst_rect.x2 *= xscale;
    dst_rect.y1 *= yscale;
    dst_rect.y2 *= yscale;
  }

  if (m_stereo_mode == RENDER_STEREO_MODE_MONO)
  {
    std::string mode = GetStereoMode();
    if (mode == "left_right")
      SetVideo3dMode(MODE_3D_TO_2D_L);
    else if (mode == "right_left")
      SetVideo3dMode(MODE_3D_TO_2D_R);
    else if (mode == "top_bottom")
      SetVideo3dMode(MODE_3D_TO_2D_T);
    else if (mode == "bottom_top")
      SetVideo3dMode(MODE_3D_TO_2D_B);
    else
      SetVideo3dMode(MODE_3D_DISABLE);
  }
  else if (m_stereo_mode == RENDER_STEREO_MODE_SPLIT_VERTICAL)
  {
    dst_rect.x2 *= 2.0;
    SetVideo3dMode(MODE_3D_DISABLE);
  }
  else if (m_stereo_mode == RENDER_STEREO_MODE_SPLIT_HORIZONTAL)
  {
    dst_rect.y2 *= 2.0;
    SetVideo3dMode(MODE_3D_DISABLE);
  }
  else if (m_stereo_mode == RENDER_STEREO_MODE_INTERLACED)
  {
    std::string mode = GetStereoMode();
    if (mode == "left_right")
      SetVideo3dMode(MODE_3D_LR);
    else if (mode == "right_left")
      SetVideo3dMode(MODE_3D_LR_SWITCH);
    else if (mode == "row_interleaved_lr")
      SetVideo3dMode(MODE_3D_LR);
    else if (mode == "row_interleaved_rl")
      SetVideo3dMode(MODE_3D_LR_SWITCH);
    else
      SetVideo3dMode(MODE_3D_DISABLE);
  }
  else
  {
    SetVideo3dMode(MODE_3D_DISABLE);
  }

#if 1
  std::string s_dst_rect = StringUtils::Format("%i,%i,%i,%i",
    (int)dst_rect.x1, (int)dst_rect.y1,
    (int)dst_rect.Width(), (int)dst_rect.Height());
  std::string s_m_dst_rect = StringUtils::Format("%i,%i,%i,%i",
    (int)m_dst_rect.x1, (int)m_dst_rect.y1,
    (int)m_dst_rect.Width(), (int)m_dst_rect.Height());
  std::string s_display = StringUtils::Format("%i,%i,%i,%i",
    (int)m_display_rect.x1, (int)m_display_rect.y1,
    (int)m_display_rect.Width(), (int)m_display_rect.Height());
  std::string s_gui = StringUtils::Format("%i,%i,%i,%i",
    (int)gui.x1, (int)gui.y1,
    (int)gui.Width(), (int)gui.Height());
  CLog::Log(LOGDEBUG, "CAMLCodec::SetVideoRect:display(%s)", s_display.c_str());
  CLog::Log(LOGDEBUG, "CAMLCodec::SetVideoRect:gui(%s)", s_gui.c_str());
  CLog::Log(LOGDEBUG, "CAMLCodec::SetVideoRect:m_dst_rect(%s)", s_m_dst_rect.c_str());
  CLog::Log(LOGDEBUG, "CAMLCodec::SetVideoRect:dst_rect(%s)", s_dst_rect.c_str());
  CLog::Log(LOGDEBUG, "CAMLCodec::SetVideoRect:m_stereo_mode(%d)", m_stereo_mode);
  CLog::Log(LOGDEBUG, "CAMLCodec::SetVideoRect:m_stereo_view(%d)", m_stereo_view);
#endif

  // goofy 0/1 based difference in aml axis coordinates.
  // fix them.
  dst_rect.x2--;
  dst_rect.y2--;

  char video_axis[256] = {};
  sprintf(video_axis, "%d %d %d %d", (int)dst_rect.x1, (int)dst_rect.y1, (int)dst_rect.x2, (int)dst_rect.y2);

  SysfsUtils::SetString("/sys/class/video/axis", video_axis);
  // make sure we are in 'full stretch' so we can stretch
  SysfsUtils::SetInt("/sys/class/video/screen_mode", 1);

  // we only get called once gui has changed to something
  // that would show video playback, so show it.
  ShowMainVideo(true);
}

void CAMLCodec::RenderUpdateCallBack(const void *ctx, const CRect &SrcRect, const CRect &DestRect)
{
  CAMLCodec *codec = (CAMLCodec*)ctx;
  codec->SetVideoRect(SrcRect, DestRect);
}
