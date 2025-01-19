/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */


#include "AMLCodec.h"
#include "DynamicDll.h"

#include "cores/VideoPlayer/Interface/TimingConstants.h"
#include "cores/VideoPlayer/Process/ProcessInfo.h"
#include "cores/VideoPlayer/VideoRenderers/RenderFlags.h"
#include "cores/VideoPlayer/VideoRenderers/RenderManager.h"
#include "settings/AdvancedSettings.h"
#include "windowing/GraphicContext.h"
#include "settings/DisplaySettings.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/AMLUtils.h"
#include "utils/log.h"
#include "utils/StreamDetails.h"
#include "utils/StringUtils.h"
#include "utils/TimeUtils.h"
#include "ServiceBroker.h"

#include "platform/linux/SysfsPath.h"

#include <unistd.h>
#include <queue>
#include <vector>
#include <signal.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/utsname.h>
#include <linux/videodev2.h>
#include <sys/poll.h>
#include <chrono>
#include <thread>
#include "aom_integer.h"
#include "obu_util.h"

namespace
{

std::mutex pollSyncMutex;

}

// amcodec include
extern "C" {
#include <amcodec/codec.h>
}  // extern "C"

CEvent g_aml_sync_event;

class PosixFile
{
public:
  PosixFile() :
    m_fd(-1)
  {
  }

  PosixFile(int fd) :
    m_fd(fd)
  {
  }

  ~PosixFile()
  {
    if (m_fd >= 0)
     close(m_fd);
  }

  bool Open(const std::string &pathName, int flags)
  {
    m_fd = open(pathName.c_str(), flags);
    return m_fd >= 0;
  }

  int GetDescriptor() const { return m_fd; }

  int IOControl(unsigned long request, void *param)
  {
    return ioctl(m_fd, request, param);
  }

private:
  int m_fd;
};

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
  void          *param;
  dec_mode_t    dec_mode;
  enum FRAME_BASE_VIDEO_PATH video_path;
  unsigned int  dv_enable;
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
  virtual int codec_checkin_pts_us64(codec_para_t *pcodec, unsigned long long pts)=0;
  virtual int codec_get_vbuf_state(codec_para_t *pcodec, struct buf_status *buf)=0;
  virtual int codec_get_vdec_state(codec_para_t *pcodec, struct vdec_status *vdec)=0;
  virtual int codec_get_vdec_info(codec_para_t *pcodec, struct vdec_info *vdec) = 0;

  virtual int codec_init_cntl(codec_para_t *pcodec)=0;
  virtual int codec_poll_cntl(codec_para_t *pcodec)=0;
  virtual int codec_set_cntl_mode(codec_para_t *pcodec, unsigned int mode)=0;
  virtual int codec_set_cntl_avthresh(codec_para_t *pcodec, unsigned int avthresh)=0;
  virtual int codec_set_cntl_syncthresh(codec_para_t *pcodec, unsigned int syncthresh)=0;

  virtual int codec_set_av_threshold(codec_para_t *pcodec, int threshold)=0;
  virtual int codec_set_video_delay_limited_ms(codec_para_t *pcodec,int delay_ms)=0;
  virtual int codec_get_video_delay_limited_ms(codec_para_t *pcodec,int *delay_ms)=0;
  virtual int codec_get_video_cur_delay_ms(codec_para_t *pcodec,int *delay_ms)=0;
};

class DllLibAmCodec : public DllDynamic, DllLibamCodecInterface
{
  // libamcodec is static linked into libamcodec.so
  DECLARE_DLL_WRAPPER(DllLibAmCodec, "libamcodec.so")

  DEFINE_METHOD1(int, codec_init,               (codec_para_t *p1))
  DEFINE_METHOD1(int, codec_close,              (codec_para_t *p1))
  DEFINE_METHOD1(int, codec_reset,              (codec_para_t *p1))
  DEFINE_METHOD1(int, codec_pause,              (codec_para_t *p1))
  DEFINE_METHOD1(int, codec_resume,             (codec_para_t *p1))
  DEFINE_METHOD3(int, codec_write,              (codec_para_t *p1, void *p2, int p3))
  DEFINE_METHOD2(int, codec_checkin_pts_us64,   (codec_para_t *p1, unsigned long long p2))
  DEFINE_METHOD2(int, codec_get_vbuf_state,     (codec_para_t *p1, struct buf_status * p2))
  DEFINE_METHOD2(int, codec_get_vdec_state,     (codec_para_t *p1, struct vdec_status * p2))
  DEFINE_METHOD2(int, codec_get_vdec_info,      (codec_para_t *p1, struct vdec_info * p2))

  DEFINE_METHOD1(int, codec_init_cntl,          (codec_para_t *p1))
  DEFINE_METHOD1(int, codec_poll_cntl,          (codec_para_t *p1))
  DEFINE_METHOD2(int, codec_set_cntl_mode,      (codec_para_t *p1, unsigned int p2))
  DEFINE_METHOD2(int, codec_set_cntl_avthresh,  (codec_para_t *p1, unsigned int p2))
  DEFINE_METHOD2(int, codec_set_cntl_syncthresh,(codec_para_t *p1, unsigned int p2))

  DEFINE_METHOD2(int, codec_set_av_threshold,   (codec_para_t *p1, int p2))
  DEFINE_METHOD2(int, codec_set_video_delay_limited_ms, (codec_para_t *p1, int p2))
  DEFINE_METHOD2(int, codec_get_video_delay_limited_ms, (codec_para_t *p1, int *p2))
  DEFINE_METHOD2(int, codec_get_video_cur_delay_ms, (codec_para_t *p1, int *p2))

  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(codec_init)
    RESOLVE_METHOD(codec_close)
    RESOLVE_METHOD(codec_reset)
    RESOLVE_METHOD(codec_pause)
    RESOLVE_METHOD(codec_resume)
    RESOLVE_METHOD(codec_write)
    RESOLVE_METHOD(codec_checkin_pts_us64)
    RESOLVE_METHOD(codec_get_vbuf_state)
    RESOLVE_METHOD(codec_get_vdec_state)
    RESOLVE_METHOD(codec_get_vdec_info)

    RESOLVE_METHOD(codec_init_cntl)
    RESOLVE_METHOD(codec_poll_cntl)
    RESOLVE_METHOD(codec_set_cntl_mode)
    RESOLVE_METHOD(codec_set_cntl_avthresh)
    RESOLVE_METHOD(codec_set_cntl_syncthresh)

    RESOLVE_METHOD(codec_set_av_threshold)
    RESOLVE_METHOD(codec_set_video_delay_limited_ms)
    RESOLVE_METHOD(codec_get_video_delay_limited_ms)
    RESOLVE_METHOD(codec_get_video_cur_delay_ms)
  END_METHOD_RESOLVE()

public:
  void codec_init_para(aml_generic_param *p_in, codec_para_t *p_out)
  {
    memset(p_out, 0x00, sizeof(codec_para_t));

    // direct struct usage, we do not know which flavor
    // so just use what we get from headers and pray.
    p_out->handle             = -1; //init to invalid
    p_out->cntl_handle        = -1;
    p_out->sub_handle         = -1;
    p_out->audio_utils_handle = -1;
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
    p_out->dec_mode           = p_in->dec_mode;
    p_out->video_path         = p_in->video_path;
    p_out->dv_enable          = p_in->dv_enable;
  }
};

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------
// AppContext - Application state
#define MODE_3D_DISABLE         0x00000000
#define MODE_3D_ENABLE          0x00000001
#define MODE_3D_FA              0x00000020
#define MODE_3D_LR              0x00000101
#define MODE_3D_LR_SWITCH       0x00000501
#define MODE_3D_BT              0x00000201
#define MODE_3D_BT_SWITCH       0x00000601
#define MODE_3D_TO_2D_L         0x00000200
#define MODE_3D_TO_2D_R         0x00000400
#define MODE_3D_TO_2D_T         0x00000202
#define MODE_3D_TO_2D_B         0x00000a02
#define MODE_3D_OUT_TB          0x00010000
#define MODE_3D_OUT_LR          0x00020000

#define PTS_FREQ        90000
#define UNIT_FREQ       96000
#define AV_SYNC_THRESH  PTS_FREQ*30

#define TRICKMODE_NONE  0x00
#define TRICKMODE_I     0x01
#define TRICKMODE_FFFB  0x02

static const uint64_t UINT64_0 = 0x8000000000000000ULL;

#define EXTERNAL_PTS    (1)
#define SYNC_OUTSIDE    (2)
#define KEYFRAME_PTS_ONLY 0x100

// missing tags
#ifndef CODEC_TAG_VC_1
#define CODEC_TAG_VC_1  (0x312D4356)
#endif

#ifndef HAS_LIBAMCODEC_VP9
#define VFORMAT_VP9           VFORMAT_UNSUPPORT
#define VIDEO_DEC_FORMAT_VP9  VIDEO_DEC_FORMAT_MAX
#endif

#define CODEC_TAG_RV30  (0x30335652)
#define CODEC_TAG_RV40  (0x30345652)
#define CODEC_TAG_MJPEG (0x47504a4d)
#define CODEC_TAG_mjpeg (0x47504a4c)
#define CODEC_TAG_jpeg  (0x6765706a)
#define CODEC_TAG_mjpa  (0x61706a6d)

#define RW_WAIT_TIME    (5 * 1000) // 20ms

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
    uint64_t      avpts;
    uint64_t      avdts;
    int           avduration;
    int           isvalid;
    int           newflag;
    uint64_t      lastpts;
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
  hdr_buf_t         hdr_buf;
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
  int               extrasize;
  FFmpegExtraData   extradata;
  DllLibAmCodec     *m_dll;

  int               dumpfile;
  bool              dumpdemux;
} am_private_t;

typedef struct vframe_states
{
  int vf_pool_size;
  int buf_free_num;
  int buf_recycle_num;
  int buf_avail_num;
} vframe_states_t;

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
    CLog::Log(LOGERROR, "dumpfile_write: wtf ? buf is null, bufsiz({:d})", bufsiz);
    return;
  }

  if (para->dumpdemux && para->dumpfile != -1)
    write(para->dumpfile, buf, bufsiz);
}

static vformat_t codecid_to_vformat(enum AVCodecID id)
{
  vformat_t format;
  switch (id)
  {
    case AV_CODEC_ID_MPEG1VIDEO:
    case AV_CODEC_ID_MPEG2VIDEO:
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
    case AV_CODEC_ID_VP9:
      format = VFORMAT_VP9;
      break;
    case AV_CODEC_ID_AV1:
      format = VFORMAT_AV1;
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

  CLog::Log(LOGDEBUG, "codecid_to_vformat, id({:d}) -> vformat({:d})", (int)id, format);
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
    case CODEC_TAG_AMVC:
    case CODEC_TAG_MVC1:
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
    case AV_CODEC_ID_VP9:
      dec_type = VIDEO_DEC_FORMAT_VP9;
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

  CLog::Log(LOGDEBUG, "codec_tag_to_vdec_type, codec_tag({:d}) -> vdec_type({:d})", codec_tag, dec_type);
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
  pkt->lastpts    = UINT64_0;
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
  av_buffer_unref(&pkt->avpkt.buf);
  pkt->codec = NULL;
}

int check_in_pts(am_private_t *para, am_packet_t *pkt)
{
  if (para->stream_type == AM_STREAM_ES
    && UINT64_0 != pkt->avpts
    && para->m_dll->codec_checkin_pts_us64(pkt->codec, pkt->avpts) != 0)
  {
    CLog::Log(LOGDEBUG, "ERROR check in pts error!");
    return PLAYER_PTS_ERROR;
  }
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
  //CLog::Log(LOGDEBUG, "write_av_packet, pkt->isvalid({:d}), pkt->data({:p}), pkt->data_size({:d})",
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
            CLog::Log(LOGDEBUG, "[{}]write header failed!", __FUNCTION__);
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
            CLog::Log(LOGDEBUG, "write codec data failed, write_bytes({:d}), errno({:d}), size({:d})", write_bytes, errno, size);
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
                CLog::Log(LOGDEBUG, "Codec buffer full, try after {:d} ms, len({:d})",
                  RW_WAIT_TIME / 1000, len);
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
static int m4s2_dx50_mp4v_add_header(am_private_t *para, unsigned char *buf, int size,  am_packet_t *pkt)
{
  hdr_buf_t *hdr = &para->hdr_buf;

  if (size > hdr->size) {
      free(hdr->data), hdr->data = NULL;
      hdr->size = 0;

      hdr->data = (char*)malloc(size);
      if (!hdr->data) {
          CLog::Log(LOGDEBUG, "[m4s2_dx50_add_header] NOMEM!");
          return PLAYER_FAILED;
      }
  }

  hdr->size = size;
  memcpy(hdr->data, buf, size);
  return PLAYER_SUCCESS;
}

static int m4s2_dx50_mp4v_write_header(am_private_t *para, am_packet_t *pkt)
{
    CLog::Log(LOGDEBUG, "m4s2_dx50_mp4v_write_header");
    return m4s2_dx50_mp4v_add_header(para, para->extradata.GetData(), para->extradata.GetSize(), pkt);
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
    int ret = h264_add_header(para->extradata.GetData(), para->extradata.GetSize(), pkt);
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
      ret = hevc_add_header(para->extradata.GetData(), para->extradata.GetSize(), pkt);
    }
    if (ret == PLAYER_SUCCESS) {
      pkt->codec = &para->vcodec;
      pkt->newflag = 1;
      ret = write_av_packet(para, pkt);
    }
    return ret;
}

int mpeg12_add_frame_dec_info(am_private_t *para)
{
  am_packet_t *pkt = &para->am_pkt;
  int ret;

  pkt->avpkt.data = pkt->data;
  pkt->avpkt.size = pkt->data_size;

  av_buffer_unref(&pkt->avpkt.buf);
  ret = av_grow_packet(&(pkt->avpkt), 4);
  if (ret < 0)
  {
    CLog::Log(LOGDEBUG, "ERROR!!! grow_packet for apk failed.!!!");
    return ret;
  }

  pkt->data = pkt->avpkt.data;
  pkt->data_size = pkt->avpkt.size;

  uint8_t *fdata = pkt->data + pkt->data_size - 4;
  fdata[0] = 0x00;
  fdata[1] = 0x00;
  fdata[2] = 0x01;
  fdata[3] = 0x00;

  return PLAYER_SUCCESS;
}

char obu_type_name[16][32] = {
  "UNKNOWN",
  "OBU_SEQUENCE_HEADER",
  "OBU_TEMPORAL_DELIMITER",
  "OBU_FRAME_HEADER",
  "OBU_TILE_GROUP",
  "OBU_METADATA",
  "OBU_FRAME",
  "OBU_REDUNDANT_FRAME_HEADER",
  "OBU_TILE_LIST",
  "UNKNOWN",
  "UNKNOWN",
  "UNKNOWN",
  "UNKNOWN",
  "UNKNOWN",
  "UNKNOWN",
  "OBU_PADDING"
};

char meta_type_name[6][32] = {
  "OBU_METADATA_TYPE_RESERVED_0",
  "OBU_METADATA_TYPE_HDR_CLL",
  "OBU_METADATA_TYPE_HDR_MDCV",
  "OBU_METADATA_TYPE_SCALABILITY",
  "OBU_METADATA_TYPE_ITUT_T35",
  "OBU_METADATA_TYPE_TIMECODE"
};

typedef struct DataBuffer {
    const uint8_t *data;
    size_t size;
} DataBuffer;

int av1_parser_frame(
    int is_annexb,
    uint8_t *data,
    const uint8_t *data_end,
    uint8_t *dst_data,
    uint32_t *frame_len,
    uint8_t *meta_buf,
    uint32_t *meta_len) {
    int frame_decoding_finished = 0;
    uint32_t obu_size = 0;
    ObuHeader obu_header;
    memset(&obu_header, 0, sizeof(obu_header));
    int seen_frame_header = 0;
    uint8_t header[20] = {
        0x00, 0x00, 0x01, 0x54,
        0xFF, 0xFF, 0xFE, 0xAB,
        0x00, 0x00, 0x00, 0x01,
        0x41, 0x4D, 0x4C, 0x56,
        0xD0, 0x82, 0x80, 0x00
    };
    uint8_t *p = NULL;
    uint32_t rpu_size = 0;

    // decode frame as a series of OBUs
    while (!frame_decoding_finished) {
        //      struct read_bit_buffer rb;
        size_t payload_size = 0;
        size_t header_size = 0;
        size_t bytes_read = 0;
        size_t bytes_written = 0;
        const size_t bytes_available = data_end - data;
        unsigned int i;
        OBU_METADATA_TYPE meta_type;
        uint64_t type;

        if (bytes_available == 0 && !seen_frame_header) {
            break;
        }

        int status =
            aom_read_obu_header_and_size(data, bytes_available, is_annexb,
                &obu_header, &payload_size, &bytes_read);

        if (status != 0) {
            return -1;
        }

        // Note: aom_read_obu_header_and_size() takes care of checking that this
        // doesn't cause 'data' to advance past 'data_end'.

        if ((size_t)(data_end - data - bytes_read) < payload_size) {
            return -1;
        }

        CLog::Log(LOGDEBUG, "\tobu {} len {:d}+{:d}", obu_type_name[obu_header.type], bytes_read, payload_size);

        obu_size = bytes_read + payload_size + 4;

        if (!is_annexb) {
            obu_size = bytes_read + payload_size + 4;
            header_size = 20;
            aom_uleb_encode_fixed_size(obu_size, 4, 4, header + 16, &bytes_written);
        }
        else {
            obu_size = bytes_read + payload_size;
            header_size = 16;
        }
        header[0] = ((obu_size + 4) >> 24) & 0xff;
        header[1] = ((obu_size + 4) >> 16) & 0xff;
        header[2] = ((obu_size + 4) >> 8) & 0xff;
        header[3] = ((obu_size + 4) >> 0) & 0xff;
        header[4] = header[0] ^ 0xff;
        header[5] = header[1] ^ 0xff;
        header[6] = header[2] ^ 0xff;
        header[7] = header[3] ^ 0xff;
        memcpy(dst_data, header, header_size);
        dst_data += header_size;
        memcpy(dst_data, data, bytes_read + payload_size);
        dst_data += bytes_read + payload_size;

        data += bytes_read;
        *frame_len += 20 + bytes_read + payload_size;

        switch (obu_header.type) {
        case OBU_TEMPORAL_DELIMITER:
            seen_frame_header = 0;
            break;
        case OBU_SEQUENCE_HEADER:
            // The sequence header should not change in the middle of a frame.
            if (seen_frame_header) {
                return -1;
            }
            break;
        case OBU_FRAME_HEADER:
            if (data_end == data + payload_size) {
                frame_decoding_finished = 1;
            }
            else {
                seen_frame_header = 1;
            }
            break;
        case OBU_REDUNDANT_FRAME_HEADER:
        case OBU_FRAME:
            if (obu_header.type == OBU_REDUNDANT_FRAME_HEADER) {
                if (!seen_frame_header) {
                    return -1;
                }
            }
            else {
                // OBU_FRAME_HEADER or OBU_FRAME.
                if (seen_frame_header) {
                    return -1;
                }
            }
            if (obu_header.type == OBU_FRAME) {
                if (data_end == data + payload_size) {
                    frame_decoding_finished = 1;
                    seen_frame_header = 0;
                }
            }
            break;

        case OBU_TILE_GROUP:
            if (!seen_frame_header) {
                return -1;
            }
            if (data + payload_size == data_end)
                frame_decoding_finished = 1;
            if (frame_decoding_finished)
                seen_frame_header = 0;
            break;
        case OBU_METADATA:
            aom_uleb_decode(data, 8, &type, &bytes_read);
            if (type < 6)
                meta_type = static_cast<OBU_METADATA_TYPE>(type);
            else
                meta_type = OBU_METADATA_TYPE_AOM_RESERVED_0;
            p = data + bytes_read;
            CLog::Log(LOGDEBUG, "\tmeta type {} {:d}+{:d}", meta_type_name[type], bytes_read, payload_size - bytes_read);

            if (meta_type == OBU_METADATA_TYPE_ITUT_T35 && meta_buf != NULL) {
                if ((p[0] == 0xb5) /* country code */
                    && ((p[1] == 0x00) && (p[2] == 0x3b)) /* terminal_provider_code */
                    && ((p[3] == 0x00) && (p[4] == 0x00) && (p[5] == 0x08) && (p[6] == 0x00))) { /* terminal_provider_oriented_code */
                    CLog::Log(LOGDEBUG, "\t\tdolbyvison rpu");
                    meta_buf[0] = meta_buf[1] = meta_buf[2] = 0;
                    meta_buf[3] = 0x01;    meta_buf[4] = 0x19;

                    if (p[11] & 0x10) {
                        rpu_size = 0x100;
                        rpu_size |= (p[11] & 0x0f) << 4;
                        rpu_size |= (p[12] >> 4) & 0x0f;
                        if (p[12] & 0x08) {
                            CLog::Log(LOGDEBUG, "\tmeta rpu in obu exceed 512 bytes");
                            break;
                        }
                        for (i = 0; i < rpu_size; i++) {
                            meta_buf[5 + i] = (p[12 + i] & 0x07) << 5;
                            meta_buf[5 + i] |= (p[13 + i] >> 3) & 0x1f;
                        }
                        rpu_size += 5;
                    }
                    else {
                        rpu_size = (p[10] & 0x1f) << 3;
                        rpu_size |= (p[11] >> 5) & 0x07;
                        for (i = 0; i < rpu_size; i++) {
                            meta_buf[5 + i] = (p[11 + i] & 0x0f) << 4;
                            meta_buf[5 + i] |= (p[12 + i] >> 4) & 0x0f;
                        }
                        rpu_size += 5;
                    }
                    *meta_len = rpu_size;
                }
            }
            else if (meta_type == OBU_METADATA_TYPE_HDR_CLL) {
                CLog::Log(LOGDEBUG, "\t\thdr10 cll:");
                CLog::Log(LOGDEBUG, "\t\tmax_cll = {:x}", (p[0] << 8) | p[1]);
                CLog::Log(LOGDEBUG, "\t\tmax_fall = {:x}", (p[2] << 8) | p[3]);
            }
            else if (meta_type == OBU_METADATA_TYPE_HDR_MDCV) {
                CLog::Log(LOGDEBUG, "\t\thdr10 primaries[r,g,b] =");
                for (i = 0; i < 3; i++) {
                    CLog::Log(LOGDEBUG, "\t\t {:x}, {:x}",
                        (p[i * 4] << 8) | p[i * 4 + 1],
                        (p[i * 4 + 2] << 8) | p[i * 4 + 3]);
                }
                CLog::Log(LOGDEBUG, "\t\twhite point = {:x}, {:x}", (p[12] << 8) | p[13], (p[14] << 8) | p[15]);
                CLog::Log(LOGDEBUG, "\t\tmaxl = {:x}", (p[16] << 24) | (p[17] << 16) | (p[18] << 8) | p[19]);
                CLog::Log(LOGDEBUG, "\t\tminl = {:x}", (p[20] << 24) | (p[21] << 16) | (p[22] << 8) | p[23]);
            }
                break;
        case OBU_TILE_LIST:
            break;
        case OBU_PADDING:
            break;
        default:
            // Skip unrecognized OBUs
            break;
        }

        data += payload_size;
    }

    return 0;
}

int av1_add_frame_dec_info(am_private_t *para)
{
  int ret;
  am_packet_t *pkt = &para->am_pkt;

  unsigned int dst_frame_size = 0;
  uint8_t *dst_data = (uint8_t *)calloc(1, pkt->data_size + 4096);
  av1_parser_frame(0, pkt->data, pkt->data + pkt->data_size, dst_data, &dst_frame_size, NULL, NULL);

  if (dst_frame_size - pkt->data_size > 0)
  {
    pkt->avpkt.data = pkt->data;
    pkt->avpkt.size = pkt->data_size;

    av_buffer_unref(&pkt->avpkt.buf);
    ret = av_grow_packet(&(pkt->avpkt), dst_frame_size - pkt->data_size);
    if (ret < 0)
    {
      CLog::Log(LOGDEBUG, "ERROR!!! grow_packet for apk failed.!!!");
      return ret;
    }

    pkt->data = pkt->avpkt.data;
    pkt->data_size = dst_frame_size;
    memcpy(pkt->data, dst_data, dst_frame_size);
  }
  free(dst_data);

  return PLAYER_SUCCESS;
}

int vp9_update_frame_header(am_packet_t *pkt)
{
  int dsize = pkt->data_size;
  unsigned char *buf = pkt->data;
  unsigned char marker;
  int frame_number;
  int cur_frame, cur_mag, mag, index_sz, offset[9], size[8], tframesize[9];
  int mag_ptr;
  int ret;
  unsigned char *old_header = NULL;
  int total_datasize = 0;

  pkt->avpkt.data = pkt->data;
  pkt->avpkt.size = pkt->data_size;

  if (buf == NULL)
    return PLAYER_SUCCESS; /*something error. skip add header*/

  marker = buf[dsize - 1];

  if ((marker & 0xe0) == 0xc0)
  {
    frame_number = (marker & 0x7) + 1;
    mag = ((marker >> 3) & 0x3) + 1;
    index_sz = 2 + mag * frame_number;
    CLog::Log(LOGDEBUG, " frame_number : {:d}, mag : {:d}; index_sz : {:d}", frame_number, mag, index_sz);
    offset[0] = 0;
    mag_ptr = dsize - mag * frame_number - 2;
    if (buf[mag_ptr] != marker)
    {
      CLog::Log(LOGDEBUG, " Wrong marker2 : 0x{:X} --> 0x{:X}", marker, buf[mag_ptr]);
      return PLAYER_SUCCESS;
    }

    mag_ptr++;

    for (cur_frame = 0; cur_frame < frame_number; cur_frame++)
    {
      size[cur_frame] = 0; // or size[0] = bytes_in_buffer - 1; both OK

      for (cur_mag = 0; cur_mag < mag; cur_mag++)
      {
        size[cur_frame] = size[cur_frame]|(buf[mag_ptr] << (cur_mag*8));
        mag_ptr++;
      }

      offset[cur_frame+1] = offset[cur_frame] + size[cur_frame];

      if (cur_frame == 0)
        tframesize[cur_frame] = size[cur_frame];
      else
        tframesize[cur_frame] = tframesize[cur_frame - 1] + size[cur_frame];

      total_datasize += size[cur_frame];
    }
  }
  else
  {
    frame_number = 1;
    offset[0] = 0;
    size[0] = dsize; // or size[0] = bytes_in_buffer - 1; both OK
    total_datasize += dsize;
    tframesize[0] = dsize;
  }

  if (total_datasize > dsize)
  {
    CLog::Log(LOGDEBUG, "DATA overflow : 0x{:X} --> 0x{:X}", total_datasize, dsize);
    return PLAYER_SUCCESS;
  }

  if (frame_number >= 1)
  {
    /*
    if only one frame ,can used headers.
    */
    int need_more = total_datasize + frame_number * 16 - dsize;

    av_buffer_unref(&pkt->avpkt.buf);
    ret = av_grow_packet(&(pkt->avpkt), need_more);
    if (ret < 0)
    {
      CLog::Log(LOGDEBUG, "ERROR!!! grow_packet for apk failed.!!!");
      return ret;
    }

    pkt->data = pkt->avpkt.data;
    pkt->data_size = pkt->avpkt.size;
  }

  for (cur_frame = frame_number - 1; cur_frame >= 0; cur_frame--)
  {
    AVPacket *avpkt = &(pkt->avpkt);
    int framesize = size[cur_frame];
    int oldframeoff = tframesize[cur_frame] - framesize;
    int outheaderoff = oldframeoff + cur_frame * 16;
    uint8_t *fdata = avpkt->data + outheaderoff;
    uint8_t *old_framedata = avpkt->data + oldframeoff;
    memmove(fdata + 16, old_framedata, framesize);
    framesize += 4;/*add 4. for shift.....*/

    /*add amlogic frame headers.*/
    fdata[0] = (framesize >> 24) & 0xff;
    fdata[1] = (framesize >> 16) & 0xff;
    fdata[2] = (framesize >> 8) & 0xff;
    fdata[3] = (framesize >> 0) & 0xff;
    fdata[4] = ((framesize >> 24) & 0xff) ^0xff;
    fdata[5] = ((framesize >> 16) & 0xff) ^0xff;
    fdata[6] = ((framesize >> 8) & 0xff) ^0xff;
    fdata[7] = ((framesize >> 0) & 0xff) ^0xff;
    fdata[8] = 0;
    fdata[9] = 0;
    fdata[10] = 0;
    fdata[11] = 1;
    fdata[12] = 'A';
    fdata[13] = 'M';
    fdata[14] = 'L';
    fdata[15] = 'V';
    framesize -= 4;/*del 4 to real framesize for check.....*/

    if (!old_header)
    {
      // nothing
    }
    else if (old_header > fdata + 16 + framesize)
    {
      CLog::Log(LOGDEBUG, "data has gaps,set to 0");
      memset(fdata + 16 + framesize, 0, (old_header - fdata + 16 + framesize));
    }
    else if (old_header < fdata + 16 + framesize)
      CLog::Log(LOGDEBUG, "ERROR!!! data over writed!!!! over write {:d}", fdata + 16 + framesize - old_header);

    old_header = fdata;
  }

  return PLAYER_SUCCESS;
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

    memcpy(pkt->hdr->data + 26, para->extradata.GetData(), para->extradata.GetSize());
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
    memcpy(pkt->hdr->data, para->extradata.GetData() + 1, para->extradata.GetSize() - 1);
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
    //CLog::Log(LOGDEBUG, "[mpeg_add_header:{:d}]wrapper size={:d}",__LINE__,size);
    memcpy(pkt->hdr->data + size, para->extradata.GetData(), para->extradata.GetSize());
    size += para->extrasize;
    //CLog::Log(LOGDEBUG, "[mpeg_add_header:{:d}]wrapper+seq size={:d}",__LINE__,size);
    memset(pkt->hdr->data + size, 0xff, STUFF_BYTES_LENGTH);
    size += STUFF_BYTES_LENGTH;
    pkt->hdr->size = size;
    //CLog::Log(LOGDEBUG, "[mpeg_add_header:{:d}]hdr_size={:d}",__LINE__,size);
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

        if (para->video_format == VFORMAT_H264 ||
            para->video_format == VFORMAT_H264_4K2K ||
            para->video_format == VFORMAT_H264MVC) {
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
            if (para->extradata.GetSize() > 4 && !para->extradata.GetData()[0] && !para->extradata.GetData()[1] &&
                para->extradata.GetData()[2] == 0x01 && para->extradata.GetData()[3] == 0x0f && ((para->extradata.GetData()[4] & 0x03) == 0x03))
            {
                CLog::Log(LOGDEBUG, "CODEC_TAG_WVC1 == para->video_codec_tag, using wmv3_write_header");
                ret = wmv3_write_header(para, pkt);
            }
            else
            {
                CLog::Log(LOGDEBUG, "CODEC_TAG_WVC1 == para->video_codec_tag");
                ret = wvc1_write_header(para, pkt);
            }
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
          || (AV_CODEC_ID_MPEG2VIDEO == para->video_codec_id)) {
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

                memcpy(pkt->hdr->data + 26, para->extradata.GetData(), para->extradata.GetSize());

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
    else if (para->video_format == VFORMAT_VP9) {
      vp9_update_frame_header(pkt);
    }
    else if (para->vcodec.dec_mode == STREAM_TYPE_FRAME && para->video_format == VFORMAT_AV1) {
      av1_add_frame_dec_info(para);
    }
    else if (para->vcodec.dec_mode == STREAM_TYPE_FRAME && para->video_format == VFORMAT_MPEG12)
      mpeg12_add_frame_dec_info(para);

  }
  return PLAYER_SUCCESS;
}

// calc padding if needed to match up pkt size
static inline int calc_chunk_size(int size)
{
// arch/arm64/include/asm/cache.h
#define L1_CACHE_SHIFT         (6)
#define L1_CACHE_BYTES         (1 << L1_CACHE_SHIFT)
//arch/arm64/include/asm/page-def.h
#define PAGE_SIZE              (4096)
//drivers/frame_provider/decoder/utils/vdec_input.c
#define MIN_FRAME_PADDING_SIZE ((int)(L1_CACHE_BYTES))

  int need_padding_size = MIN_FRAME_PADDING_SIZE;
  if (size < PAGE_SIZE) {
    need_padding_size += PAGE_SIZE - ((size + need_padding_size) & (PAGE_SIZE - 1));
  } else {
    /*to 64 bytes aligned;*/
    if (size & 0x3f)
      need_padding_size += 64 - (size & 0x3f);
  }
  return (size + need_padding_size);
}

/*************************************************************************/
CAMLCodec::CAMLCodec(CProcessInfo &processInfo, CDVDStreamInfo &hints)
  : m_opened(false)
  , m_speed(DVD_PLAYSPEED_NORMAL)
  , m_cur_pts(DVD_NOPTS_VALUE)
  , m_last_pts(DVD_NOPTS_VALUE)
  , m_bufferIndex(-1)
  , m_state(0)
  , m_hints(hints)
  , m_processInfo(processInfo)
  , m_dataCacheCore(CServiceBroker::GetDataCacheCore())
{
  am_private = new am_private_t();
  m_dll = new DllLibAmCodec;
  if(!m_dll->Load())
    CLog::Log(LOGWARNING, "CAMLCodec::CAMLCodec libamcodec.so not found");
  am_private->m_dll = m_dll;
  am_private->vcodec.handle             = -1; //init to invalid
  am_private->vcodec.cntl_handle        = -1;
  am_private->vcodec.sub_handle         = -1;
  am_private->vcodec.audio_utils_handle = -1;
}


CAMLCodec::~CAMLCodec()
{
  delete am_private;
  am_private = NULL;
  delete m_dll, m_dll = NULL;
}

float CAMLCodec::OMXPtsToSeconds(int omxpts)
{
  return static_cast<float>(omxpts) / PTS_FREQ;
}

int CAMLCodec::OMXDurationToNs(int duration)
{
  return static_cast<int>(static_cast<float>(duration) / PTS_FREQ * 1000000 );
}

int CAMLCodec::GetAmlDuration() const
{
  return am_private ? (am_private->video_rate * PTS_FREQ) / UNIT_FREQ : 0;
};

std::string CAMLCodec::intToFourCCString(unsigned int value) 
{
  char bytes[4];
  bytes[0] = value & 0xFF;
  bytes[1] = (value >> 8) & 0xFF;
  bytes[2] = (value >> 16) & 0xFF;
  bytes[3] = (value >> 24) & 0xFF;

  std::string fourCCString(bytes, 4);

  for (auto& c : fourCCString) {
      c = std::tolower(c, std::locale());
  }

  return fourCCString;
}

std::string CAMLCodec::GetDoViCodecFourCC(unsigned int codec_tag)
{
  if (codec_tag == 0) return "----";

  std::string fourCC = intToFourCCString(codec_tag);

  // some files don't have dvhe or dvh1 tag set up but have Dolby Vision side data
  // page 10, table 2 from https://professional.dolby.com/siteassets/content-creation/dolby-vision-for-content-creators/dolby-vision-streams-within-the-http-live-streaming-format-v2.0-13-november-2018.pdf
  if (fourCC == "hev1") return "dvhe";
  if (fourCC == "hvc1") return "dvh1";
  if (fourCC == "avc3") return "dvav";
  if (fourCC == "avc1") return "dva1";
  if (fourCC == "vvc1") return "dvc1";
  if (fourCC == "vvi1") return "dvi1";

  return fourCC;
}

void CAMLCodec::SetProcessInfoVideoDetails() 
{
  m_dataCacheCore.SetVideoHdrType(m_hints.hdrType);
  m_dataCacheCore.SetVideoColorSpace(m_hints.colorSpace);
  m_dataCacheCore.SetVideoColorRange(m_hints.colorRange);
  m_dataCacheCore.SetVideoColorPrimaries(m_hints.colorPrimaries);
  m_dataCacheCore.SetVideoColorTransferCharacteristic(m_hints.colorTransferCharacteristic);

  if (m_hints.hdrType == StreamHdrType::HDR_TYPE_DOLBYVISION) 
  {
    m_dataCacheCore.SetVideoDoViCodecFourCC(GetDoViCodecFourCC(m_hints.codec_tag));

    if (m_hints.dovi_el_type == DOVIELType::TYPE_FEL)
      m_dataCacheCore.SetVideoBitDepth(12); // 12 bit for FEL (once DV processed)
    else
      m_dataCacheCore.SetVideoBitDepth(m_hints.bitdepth);
  }
  else
  {
    m_dataCacheCore.SetVideoBitDepth(m_hints.bitdepth);
  }
}

bool CAMLCodec::OpenDecoder()
{
  m_speed = DVD_PLAYSPEED_NORMAL;
  m_drain = false;
  m_cur_pts = DVD_NOPTS_VALUE;
  m_dst_rect.SetRect(0, 0, 0, 0);
  m_zoom = -1.0f;
  CDVDStreamInfo &hints = m_hints;  // Fudge to avoid large chnage delta renaming hints to m_hints.
  m_state = 0;
  m_hints.pClock = hints.pClock;
  m_tp_last_frame = std::chrono::system_clock::now();
  m_decoder_timeout = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoDecoderTimeout;

  if (!OpenAmlVideo(hints))
  {
    CLog::Log(LOGERROR, "CAMLCodec::OpenDecoder - cannot open amlvideo device");
    return false;
  }

  ShowMainVideo(false);

  am_packet_init(&am_private->am_pkt);
  // default stream type
  am_private->stream_type      = AM_STREAM_ES;
  // handle hints.
  am_private->video_width      = hints.width;
  am_private->video_height     = hints.height;
  am_private->video_codec_id   = hints.codec;
  am_private->video_codec_tag  = hints.codec_tag;

  am_private->video_pid        = -1;

  // handle video ratio
  AVRational video_ratio       = av_d2q(1, SHRT_MAX);
  //if (!hints.forced_aspect)
  //  video_ratio = av_d2q(hints.aspect, SHRT_MAX);
  am_private->video_ratio      = ((int32_t)video_ratio.num << 16) | video_ratio.den;
  am_private->video_ratio64    = ((int64_t)video_ratio.num << 32) | video_ratio.den;

  // handle video rate
  if (hints.fpsrate > 0 && hints.fpsscale != 0)
  {
    // then ffmpeg avg_frame_rate next
    am_private->video_rate = 0.5f + (float)UNIT_FREQ * hints.fpsscale / hints.fpsrate;
  }
  else
    am_private->video_rate = 0.5f + (float)UNIT_FREQ * 1001 / 30000;

  // check for 1920x1080, interlaced, 25 fps
  // incorrectly reported as 50 fps (yes, video_rate == 1920)
  if (hints.width == 1920 && am_private->video_rate == 1920)
  {
    CLog::Log(LOGDEBUG, "CAMLCodec::OpenDecoder video_rate exception");
    am_private->video_rate = 0.5f + (float)UNIT_FREQ * 1001 / 25000;
  }

  // check for SD h264 content incorrectly reported as 60 fsp
  // mp4/avi containers :(
  if (hints.codec == AV_CODEC_ID_H264 && hints.width <= 720 && am_private->video_rate == 1602)
  {
    CLog::Log(LOGDEBUG, "CAMLCodec::OpenDecoder video_rate exception");
    am_private->video_rate = 0.5f + (float)UNIT_FREQ * 1001 / 24000;
  }

  // check for SD h264 content incorrectly reported as some form of 30 fsp
  // mp4/avi containers :(
  if (hints.codec == AV_CODEC_ID_H264 && hints.width <= 720)
  {
    if (am_private->video_rate >= 3200 && am_private->video_rate <= 3210)
    {
      CLog::Log(LOGDEBUG, "CAMLCodec::OpenDecoder video_rate exception");
      am_private->video_rate = 0.5f + (float)UNIT_FREQ * 1001 / 24000;
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
  if ((am_private->video_format == VFORMAT_H264)
    && (hints.width > 1920 || hints.height > 1088)
    && (aml_support_h264_4k2k() == AML_HAS_H264_4K2K))
  {
    am_private->video_format = VFORMAT_H264_4K2K;
  }
  else if ((am_private->video_format == VFORMAT_H264) &&
          ((am_private->video_codec_tag == CODEC_TAG_AMVC) ||
           (am_private->video_codec_tag == CODEC_TAG_MVC1)))
  {
    am_private->video_format = VFORMAT_H264MVC;
  }
  switch (am_private->video_format)
  {
    default:
      am_private->extradata = hints.extradata;
      break;
    case VFORMAT_REAL:
    case VFORMAT_MPEG12:
      break;
  }

  if (am_private->stream_type == AM_STREAM_ES && am_private->video_codec_tag != 0)
    am_private->video_codec_type = codec_tag_to_vdec_type(am_private->video_codec_tag);
  if (am_private->video_codec_type == VIDEO_DEC_FORMAT_UNKNOW)
    am_private->video_codec_type = codec_tag_to_vdec_type(am_private->video_codec_id);

  CLog::Log(LOGINFO, "CAMLCodec::OpenDecoder "
    "hints.width({:d}), hints.height({:d}), hints.codec({:d}), hints.codec_tag({:d}), hints.bitdepth({:d})",
    hints.width, hints.height, hints.codec, hints.codec_tag, hints.bitdepth);
  CLog::Log(LOGINFO, "CAMLCodec::OpenDecoder hints.fpsrate({:d}), hints.fpsscale({:d}), video_rate({:d})",
    hints.fpsrate, hints.fpsscale, am_private->video_rate);
  CLog::Log(LOGINFO, "CAMLCodec::OpenDecoder hints.aspect({:f}), video_ratio.num({:d}), video_ratio.den({:d})",
    hints.aspect, video_ratio.num, video_ratio.den);
  CLog::Log(LOGINFO, "CAMLCodec::OpenDecoder hints.orientation({:d}), hints.forced_aspect({:d}), hints.extrasize({:d})",
    hints.orientation, hints.forced_aspect, hints.extradata.GetSize());

  std::string hdrType = CStreamDetails::HdrTypeToString(hints.hdrType);
  if (hdrType.size())
    CLog::Log(LOGINFO, "CAMLCodec::OpenDecoder hdr type: {}", hdrType);
  if (hints.hdrType == StreamHdrType::HDR_TYPE_DOLBYVISION)
    CLog::Log(LOGINFO, "CAMLCodec::OpenDecoder DOVI: version {:d}.{:d}, profile {:d}, el type {:d}",
      hints.dovi.dv_version_major, hints.dovi.dv_version_minor, hints.dovi.dv_profile, hints.dovi_el_type);

  m_processInfo.SetVideoDAR(hints.aspect);
  CLog::Log(LOGINFO, "CAMLCodec::OpenDecoder decoder timeout: {:d}s",
    m_decoder_timeout);

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
  am_private->gcodec.dec_mode    = STREAM_TYPE_FRAME;
  am_private->gcodec.video_path  = FRAME_BASE_PATH_AMLVIDEO_AMVIDEO;

  aml_dv_open(hints.hdrType, hints.bitdepth);

  // Now have the HDRType resolved, ok to set the transfer pq - so renderer can set the shaders as needed.
  aml_set_transfer_pq(hints.hdrType, hints.bitdepth);

  SetProcessInfoVideoDetails();

  // Setup Codec for DV Content
  if ((hints.hdrType == StreamHdrType::HDR_TYPE_DOLBYVISION) && aml_is_dv_enable()) 
  {
    am_private->gcodec.dv_enable = 1;
    if ((hints.dovi.dv_profile == 4) || (hints.dovi.dv_profile == 7))
    {
      if (hints.dovi_el_type == DOVIELType::TYPE_FEL) // use stream path if FEL
      {
        CSysfsPath amdolby_vision_debug{"/sys/class/amdolby_vision/debug"};
        if (amdolby_vision_debug.Exists())
          amdolby_vision_debug.Set("enable_fel 1");
        am_private->gcodec.dec_mode = STREAM_TYPE_STREAM;
      }
    }
  }

  // DEC_CONTROL_FLAG_DISABLE_FAST_POC
  CSysfsPath("/sys/module/amvdec_h264/parameters/dec_control", 4);

  CSysfsPath di_debug_flag{"/sys/module/di/parameters/di_debug_flag"};
  CSysfsPath di_debug{"/sys/class/deinterlace/di0/debug"};
  if (di_debug_flag.Exists() && di_debug.Exists())
  {
    if (am_private->video_format == VFORMAT_VC1) 					/* workaround to fix slowdown VC1 progressive */
    {
      di_debug_flag.Set(0x10000);
      di_debug.Set("di_debug_flag0x10000");
    }
    else
    {
      di_debug_flag.Set(0);
      di_debug.Set("di_debug_flag0x0");
    }
  }

  switch(am_private->video_format)
  {
    default:
      break;
    case VFORMAT_MPEG4:
      am_private->gcodec.param = (void*)EXTERNAL_PTS;
      if (m_hints.ptsinvalid)
        am_private->gcodec.param = (void*)(EXTERNAL_PTS | KEYFRAME_PTS_ONLY);
      break;
    case VFORMAT_H264MVC:
      am_private->gcodec.dec_mode = STREAM_TYPE_SINGLE;
      [[fallthrough]];
    case VFORMAT_H264:
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
      am_private->gcodec.noblock = 1;
      am_private->gcodec.stream_type = STREAM_TYPE_RM;
      am_private->gcodec.ratio = 0x100;
      am_private->gcodec.ratio64 = 0;
      {
        static unsigned short tbl[9] = {0};
        if (VIDEO_DEC_FORMAT_REAL_8 == am_private->video_codec_type)
        {
          am_private->gcodec.extra = am_private->extradata.GetData()[1] & 7;
          tbl[0] = (((am_private->gcodec.width  >> 2) - 1) << 8)
                 | (((am_private->gcodec.height >> 2) - 1) & 0xff);
          unsigned int j;
          for (unsigned int i = 1; i <= am_private->gcodec.extra; i++)
          {
            j = 2 * (i - 1);
            tbl[i] = ((am_private->extradata.GetData()[8 + j] - 1) << 8) | ((am_private->extradata.GetData()[8 + j + 1] - 1) & 0xff);
          }
        }
        am_private->gcodec.param = &tbl;
      }
      break;
    case VFORMAT_VC1:
      // vc1 in an avi file
      if (m_hints.ptsinvalid)
        am_private->gcodec.param = (void*)KEYFRAME_PTS_ONLY;
      am_private->gcodec.dec_mode = STREAM_TYPE_SINGLE;
      break;
    case VFORMAT_HEVC:
      am_private->gcodec.format = VIDEO_DEC_FORMAT_HEVC;
      am_private->gcodec.param  = (void*)EXTERNAL_PTS;
      if (m_hints.ptsinvalid)
        am_private->gcodec.param = (void*)(EXTERNAL_PTS | SYNC_OUTSIDE);
      break;
    case VFORMAT_VP9:
      am_private->gcodec.format = VIDEO_DEC_FORMAT_VP9;
      am_private->gcodec.param  = (void*)EXTERNAL_PTS;
      if (m_hints.ptsinvalid)
        am_private->gcodec.param = (void*)(EXTERNAL_PTS | SYNC_OUTSIDE);
      break;
  }
  am_private->gcodec.param = (void *)((std::uintptr_t)am_private->gcodec.param | (am_private->video_rotation_degree << 16));

  // translate from generic to firmware version dependent
  m_dll->codec_init_para(&am_private->gcodec, &am_private->vcodec);

  std::string config_data = GetHDRStaticMetadata();
  if (!config_data.empty())
  {
    am_private->vcodec.config_len = static_cast<int>(config_data.size());
    am_private->vcodec.config = (char*)malloc(config_data.size() + 1);
    config_data.copy(am_private->vcodec.config, config_data.size());
    am_private->vcodec.config[am_private->vcodec.config_len] = '\0';
  }

  if (am_private->vcodec.dec_mode == STREAM_TYPE_SINGLE)
    SetVfmMap("default", "decoder ppmgr amlvideo deinterlace amvideo");

  int ret = m_dll->codec_init(&am_private->vcodec);
  if (ret != CODEC_ERROR_NONE)
  {
    CLog::Log(LOGDEBUG, "CAMLCodec::OpenDecoder codec init failed, ret=0x{:x}", -ret);
    return false;
  }

  am_private->dumpdemux = false;
  dumpfile_open(am_private);

  m_dll->codec_pause(&am_private->vcodec);

  m_dll->codec_set_cntl_mode(&am_private->vcodec, TRICKMODE_NONE);
  m_dll->codec_set_video_delay_limited_ms(&am_private->vcodec, 1000);

  m_dll->codec_set_cntl_avthresh(&am_private->vcodec, AV_SYNC_THRESH);
  m_dll->codec_set_cntl_syncthresh(&am_private->vcodec, 0);
  // disable tsync, we are playing video disconnected from audio.
  CSysfsPath("/sys/class/tsync/enable", 0);

  am_private->am_pkt.codec = &am_private->vcodec;
  am_private->hdr_buf.size = 0;
  free(am_private->hdr_buf.data);
  am_private->hdr_buf.data = NULL;
  pre_header_feeding(am_private, &am_private->am_pkt);

  m_display_rect = CRect(0, 0, CDisplaySettings::GetInstance().GetCurrentResolutionInfo().iWidth, CDisplaySettings::GetInstance().GetCurrentResolutionInfo().iHeight);

  std::string strScaler;
  CSysfsPath ppscaler{"/sys/class/ppmgr/ppscaler"};
  if (ppscaler.Exists())
    strScaler = ppscaler.Get<std::string>().value();
  if (strScaler.find("enabled") == std::string::npos)     // Scaler not enabled, use screen size
    m_display_rect = CRect(0, 0, CDisplaySettings::GetInstance().GetCurrentResolutionInfo().iScreenWidth, CDisplaySettings::GetInstance().GetCurrentResolutionInfo().iScreenHeight);

  CSysfsPath("/sys/class/video/freerun_mode", 1);

  // set video mute to hide waste frames
  //aml_video_mute(true);

  m_opened = true;
  // vcodec is open, update speed if it was
  // changed before VideoPlayer called OpenDecoder.
  SetSpeed(m_speed);
  SetPollDevice(am_private->vcodec.cntl_handle);

  return true;
}

bool CAMLCodec::OpenAmlVideo(const CDVDStreamInfo &hints)
{
  PosixFilePtr amlVideoFile = std::make_shared<PosixFile>();
  if (!amlVideoFile->Open("/dev/video10", O_RDONLY | O_NONBLOCK))
  {
    CLog::Log(LOGERROR, "CAMLCodec::OpenAmlVideo - cannot open V4L amlvideo device /dev/video10: {}", strerror(errno));
    return false;
  }

  m_amlVideoFile = amlVideoFile;
  m_defaultVfmMap = GetVfmMap("default");

  return true;
}

std::string CAMLCodec::GetVfmMap(const std::string &name)
{
  std::string vfmMap;
  CSysfsPath map{"/sys/class/vfm/map"};
  if (map.Exists())
    vfmMap = map.Get<std::string>().value();
  std::vector<std::string> sections = StringUtils::Split(vfmMap, '\n');
  std::string sectionMap;
  for (size_t i = 0; i < sections.size(); ++i)
  {
    if (StringUtils::StartsWith(sections[i], name + " {"))
    {
      sectionMap = sections[i];
      break;
    }
  }

  int openingBracePos = sectionMap.find('{') + 1;
  sectionMap = sectionMap.substr(openingBracePos, sectionMap.size() - openingBracePos - 1);
  StringUtils::Replace(sectionMap, "(0)", "");

  return sectionMap;
}

void CAMLCodec::SetVfmMap(const std::string &name, const std::string &map)
{
  CSysfsPath vfm_map{"/sys/class/vfm/map"};
  if (vfm_map.Exists())
  {
    vfm_map.Set("rm " + name);
    vfm_map.Set("add " + name + " " + map);
  }
}

void CAMLCodec::CloseDecoder()
{
  CLog::Log(LOGINFO, "CAMLCodec::CloseDecoder");

  SetPollDevice(-1);

  int blackout_policy = aml_blackout_policy(1);

  // never leave vcodec ff/rw or paused.
  if (m_speed != DVD_PLAYSPEED_NORMAL)
  {
    //m_dll->codec_resume(&am_private->vcodec);
    m_dll->codec_set_cntl_mode(&am_private->vcodec, TRICKMODE_NONE);
  }
  m_dll->codec_close(&am_private->vcodec);
  dumpfile_close(am_private);
  m_opened = false;

  am_packet_release(&am_private->am_pkt);
  am_private->extradata = {};
  if (am_private->vcodec.config)
    free(am_private->vcodec.config);
  // return tsync to default so external apps work
  CSysfsPath("/sys/class/tsync/enable", 1);

  aml_dv_wait_video_off(m_decoder_timeout);

  // restore the saved system blackout_policy value
  aml_blackout_policy(blackout_policy);

  ShowMainVideo(false);

  CloseAmlVideo();

  aml_dv_close();
}

void CAMLCodec::CloseAmlVideo()
{
  m_amlVideoFile.reset();

  if (am_private->vcodec.dec_mode == STREAM_TYPE_SINGLE)
    SetVfmMap("default", m_defaultVfmMap);

  m_amlVideoFile = NULL;
}

void CAMLCodec::Reset()
{
  CLog::Log(LOGDEBUG, "CAMLCodec::Reset");

  if (!m_opened)
    return;

  SetPollDevice(-1);

  // restore the speed (some amcodec versions require this)
  if (m_speed != DVD_PLAYSPEED_NORMAL)
  {
    m_dll->codec_set_cntl_mode(&am_private->vcodec, TRICKMODE_NONE);
  }
  m_dll->codec_pause(&am_private->vcodec);

  // reset the decoder
  m_dll->codec_reset(&am_private->vcodec);
  m_dll->codec_set_video_delay_limited_ms(&am_private->vcodec, 1000);

  dumpfile_close(am_private);
  dumpfile_open(am_private);

  // re-init our am_pkt
  am_packet_release(&am_private->am_pkt);
  am_packet_init(&am_private->am_pkt);
  am_private->am_pkt.codec = &am_private->vcodec;
  pre_header_feeding(am_private, &am_private->am_pkt);

  // reset some interal vars
  m_cur_pts = DVD_NOPTS_VALUE;
  m_last_pts = DVD_NOPTS_VALUE;
  m_state = 0;

  SetSpeed(m_speed);

  SetPollDevice(am_private->vcodec.cntl_handle);
}

bool CAMLCodec::AddData(uint8_t *pData, size_t iSize, double dts, double pts)
{
  int data_len, free_len;
  int chunk_size = calc_chunk_size(iSize);
  float new_buffer_level = GetBufferLevel(chunk_size, data_len, free_len);
  bool streambuffer(am_private->gcodec.dec_mode == STREAM_TYPE_STREAM);
 
  m_minimum_buffer_level = 5.0f;

  if (!m_opened || !pData || free_len == 0 || new_buffer_level >= 100.0f)
  {
    CLog::Log(LOGDEBUG, LOGVIDEO,
      "CAMLCodec::{}: skip add data dl:{:d} fl:{:d} sz:{:d}({:d}) lv:{:.1f}% dts:{:.3f} pts:{:.3f}", __FUNCTION__,
      data_len,
      free_len,
      static_cast<unsigned int>(iSize),
      chunk_size,
      new_buffer_level,
      dts / DVD_TIME_BASE,
      pts / DVD_TIME_BASE
    );
    return false;
  }

  if (am_private->hdr_buf.size > 0)
  {
    CLog::Log(LOGDEBUG, "CAMLCodec::{}: feed extradata on first frame. extradata size: {:d}", __FUNCTION__,
      am_private->hdr_buf.size);

    am_packet_t *pkt = &am_private->am_pkt;
    pkt->data = pData;
    pkt->data_size = iSize;
    pkt->avpkt.data = pkt->data;
    pkt->avpkt.size = pkt->data_size;

    av_buffer_unref(&pkt->avpkt.buf);
    int ret = av_grow_packet(&(pkt->avpkt), am_private->hdr_buf.size);
    if (ret < 0)
    {
      CLog::Log(LOGDEBUG, "CAMLCodec::{}: ERROR!!! grow_packet for apk failed.!!!", __FUNCTION__);
      return ret;
    }

    pkt->data = pkt->avpkt.data;
    pkt->data_size = pkt->avpkt.size;

    memmove(pkt->data + am_private->hdr_buf.size, pkt->data, iSize);
    memcpy(pkt->data, am_private->hdr_buf.data, am_private->hdr_buf.size);

    iSize += am_private->hdr_buf.size;
    am_private->hdr_buf.size = 0;
    free(am_private->hdr_buf.data);
    am_private->hdr_buf.data = NULL;
  }
  else
  {
    am_private->am_pkt.data = pData;
    am_private->am_pkt.data_size = iSize;
  }

  am_private->am_pkt.newflag    = 1;
  am_private->am_pkt.isvalid    = 1;
  am_private->am_pkt.avduration = 0;

  // handle pts
  if (m_hints.ptsinvalid || pts == DVD_NOPTS_VALUE)
    am_private->am_pkt.avpts = UINT64_0;
  else
  {
    am_private->am_pkt.avpts = pts;
    m_state |= STATE_HASPTS;
  }

  // handle dts
  if (dts == DVD_NOPTS_VALUE)
    am_private->am_pkt.avdts = am_private->am_pkt.avpts;
  else
  {
    am_private->am_pkt.avdts = dts;

    // For VC1 AML decoder uses PTS only on I-Frames
    if (am_private->am_pkt.avpts == UINT64_0 && (((size_t)am_private->gcodec.param) & KEYFRAME_PTS_ONLY))
      am_private->am_pkt.avpts = am_private->am_pkt.avdts;
  }

  // We use this to determine the fill state if no PTS is given
  if (m_cur_pts == DVD_NOPTS_VALUE)
  {
    // No PTS given -> use first DTS for AML ptsserver initialization
    if ((m_state & STATE_HASPTS) == 0)
      am_private->am_pkt.avpts = am_private->am_pkt.avdts;
  }

  // some formats need header/data tweaks.
  // the actual write occurs once in write_av_packet
  // and is controlled by am_pkt.newflag.
  set_header_info(am_private);

  // loop until we write all into codec, am_pkt.isvalid
  // will get set to zero once everything is consumed.
  // PLAYER_SUCCESS means all is ok, not all bytes were written.
  int loop = 0;
  while (am_private->am_pkt.isvalid && loop < 100)
  {
    // abort on any errors.
    if (write_av_packet(am_private, &am_private->am_pkt) != PLAYER_SUCCESS)
      break;

    if (am_private->am_pkt.isvalid)
      CLog::Log(LOGDEBUG, "CAMLCodec::{} Decode: write_av_packet looping", __FUNCTION__);
    loop++;
  }
  if (loop == 100)
  {
    // Decoder got stuck; Reset
    Reset();
    return false;
  }
  if (iSize > 50000)
    usleep(2000); // wait 2ms to process larger packets

  if (iSize > 0)
    CLog::Log(LOGDEBUG, LOGVIDEO,
      "CAMLCodec::{}: dl:{:d} fl:{:d} sz:{:d}({:d}) lv:{:.1f}% dts:{:.3f} pts:{:.3f}", __FUNCTION__,
      data_len + chunk_size,
      free_len - chunk_size,
      static_cast<unsigned int>(iSize),
      chunk_size,
      new_buffer_level,
      dts / DVD_TIME_BASE,
      pts / DVD_TIME_BASE
    );
  return true;
}

int CAMLCodec::m_pollDevice;

int CAMLCodec::PollFrame()
{
  std::lock_guard<std::mutex> lock(pollSyncMutex);
  if (m_pollDevice < 0)
    return 0;

  struct pollfd codec_poll_fd[1];
  codec_poll_fd[0].fd = m_pollDevice;
  codec_poll_fd[0].events = POLLOUT;

  std::chrono::time_point<std::chrono::system_clock> now(std::chrono::system_clock::now());
  poll(codec_poll_fd, 1, 50);
  g_aml_sync_event.Set();
  int elapsed = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - now).count();
  CLog::Log(LOGDEBUG, LOGAVTIMING, "CAMLCodec::PollFrame elapsed:{:.3f}ms", elapsed / 1000.0);
  return 1;
}

void CAMLCodec::SetPollDevice(int dev)
{
  std::lock_guard<std::mutex> lock(pollSyncMutex);
  m_pollDevice = dev;
}

int CAMLCodec::ReleaseFrame(const uint32_t index, bool drop)
{
  int ret;
  v4l2_buffer vbuf = v4l2_buffer();
  vbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  vbuf.index = index;

  if (!m_amlVideoFile)
    return 0;

  if (drop)
    vbuf.flags |= V4L2_BUF_FLAG_DONE;

  CLog::Log(LOGDEBUG, LOGVIDEO, "CAMLCodec::ReleaseFrame idx:{:d}, drop:{:d}", index, static_cast<int>(drop));

  if ((ret = m_amlVideoFile->IOControl(VIDIOC_QBUF, &vbuf)) < 0)
    CLog::Log(LOGERROR, "CAMLCodec::ReleaseFrame - VIDIOC_QBUF failed: {}", strerror(errno));
  return ret;
}

float CAMLCodec::GetBufferLevel()
{
  int new_chunk = 0, data_len, free_len;
  return GetBufferLevel(new_chunk, data_len, free_len);
}

float CAMLCodec::GetBufferLevel(int new_chunk, int &data_len, int &free_len)
{
  struct buf_status bs;
  float level = 0.0f;
  m_dll->codec_get_vbuf_state(&am_private->vcodec, &bs);

  data_len = bs.data_len;
  free_len = bs.free_len;

  if (bs.free_len > 0)
  {
    if (bs.size)
      level = (float)((100.0f / (float)bs.size) * (bs.data_len + new_chunk));
  }
  else
    level = 100.0f;

  return level;
}

int CAMLCodec::DequeueBuffer()
{
  v4l2_buffer vbuf = v4l2_buffer();
  vbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

  int ret = (m_amlVideoFile->IOControl(VIDIOC_DQBUF, &vbuf) < 0) ? errno : 0;

  if (ret == 0)
  {
    m_last_pts = m_cur_pts;

    m_cur_pts =  static_cast<uint64_t>(static_cast<uint32_t>(vbuf.timestamp.tv_sec)) << 32;
    m_cur_pts += static_cast<uint32_t>(vbuf.timestamp.tv_usec);

    CLog::Log(LOGDEBUG, LOGAVTIMING, "CAMLCodec::DequeueBuffer: pts:{:.3f} idx:{:d}",
  			static_cast<double>(m_cur_pts) /  DVD_TIME_BASE, vbuf.index);

    m_bufferIndex = vbuf.index;
  }
  else if (ret != EAGAIN)
  {
    CLog::Log(LOGERROR, "CAMLCodec::DequeueBuffer - VIDIOC_DQBUF failed: {}", strerror(ret));
  }

  return ret;
}

CDVDVideoCodec::VCReturn CAMLCodec::GetPicture(VideoPicture *pVideoPicture)
{
  struct vdec_info vi;
  int ret = EAGAIN;
  float buffer_level = GetBufferLevel();
  std::chrono::milliseconds elapsed_since_last_frame(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()
    - m_tp_last_frame).count());
  bool streambuffer(am_private->gcodec.dec_mode == STREAM_TYPE_STREAM);

  if (!m_opened)
    return CDVDVideoCodec::VC_ERROR;

  if (!m_drain && buffer_level > m_minimum_buffer_level && (ret = DequeueBuffer()) == 0)
  {
    pVideoPicture->iFlags = 0;

    m_minimum_buffer_level = (streambuffer ? m_minimum_buffer_level : 0.0f);

    m_tp_last_frame = std::chrono::system_clock::now();

    if (m_last_pts == DVD_NOPTS_VALUE)
      pVideoPicture->iDuration = static_cast<double>(am_private->video_rate * DVD_TIME_BASE) / UNIT_FREQ;
    else
      pVideoPicture->iDuration = static_cast<double>(m_cur_pts - m_last_pts);

    pVideoPicture->dts = DVD_NOPTS_VALUE;
    pVideoPicture->pts = static_cast<double>(m_cur_pts);

    m_dll->codec_get_vdec_info(&am_private->vcodec, &vi);
    if  (vi.ratio_control ) {
      m_hints.aspect = 65536.0 / vi.ratio_control;
      m_processInfo.SetVideoDAR(m_hints.aspect);
    }

    CLog::Log(LOGDEBUG, LOGVIDEO, "CAMLCodec::GetPicture: index: {:d}, pts: {:.3f}, dur:{:.3f}ms ar:{:.2f} elf:{:d}ms",
      m_bufferIndex, pVideoPicture->pts / DVD_TIME_BASE, pVideoPicture->iDuration / 1000, m_hints.aspect, elapsed_since_last_frame.count());

    pVideoPicture->stereoMode = m_hints.stereo_mode;
    if (pVideoPicture->stereoMode == "block_lr" && m_processInfo.GetVideoSettings().m_StereoInvert)
      pVideoPicture->stereoMode = "block_rl";
    else if (pVideoPicture->stereoMode == "block_rl" && m_processInfo.GetVideoSettings().m_StereoInvert)
      pVideoPicture->stereoMode = "block_lr";

    return CDVDVideoCodec::VC_PICTURE;
  }
  else if (m_drain)
    return CDVDVideoCodec::VC_EOF;
  else if (buffer_level > (streambuffer ? 100.0f : 10.0f))
    return CDVDVideoCodec::VC_NONE;
  else if (ret != EAGAIN || elapsed_since_last_frame > std::chrono::seconds(m_decoder_timeout))
  {
    CLog::Log(LOGERROR, "CAMLCodec::GetPicture: time elapsed since last frame: {:d}ms ({:d}:{})",
      elapsed_since_last_frame.count(), ret, strerror(ret));
    m_tp_last_frame = std::chrono::system_clock::now();
    return CDVDVideoCodec::VC_FLUSHED;
  }

  return CDVDVideoCodec::VC_BUFFER;
}

void CAMLCodec::SetSpeed(int speed)
{
  if (m_speed == speed)
    return;

  CLog::Log(LOGDEBUG, "CAMLCodec::SetSpeed, speed({:d})", speed);

  // update internal vars regardless
  // of if we are open or not.
  m_speed = speed;

  if (!m_opened)
    return;

  switch(speed)
  {
    case DVD_PLAYSPEED_PAUSE:
      //m_dll->codec_pause(&am_private->vcodec);
      m_dll->codec_set_cntl_mode(&am_private->vcodec, TRICKMODE_NONE);
      break;
    case DVD_PLAYSPEED_NORMAL:
      //m_dll->codec_resume(&am_private->vcodec);
      m_dll->codec_set_cntl_mode(&am_private->vcodec, TRICKMODE_NONE);
      m_tp_last_frame = std::chrono::system_clock::now();
      break;
    default:
      //m_dll->codec_resume(&am_private->vcodec);
      if ((am_private->video_format == VFORMAT_H264) || (am_private->video_format == VFORMAT_H264_4K2K))
        m_dll->codec_set_cntl_mode(&am_private->vcodec, TRICKMODE_FFFB);
      else
        m_dll->codec_set_cntl_mode(&am_private->vcodec, TRICKMODE_I);
      break;
  }
}

void CAMLCodec::ShowMainVideo(const bool show)
{
  static int saved_disable_video = -1;

  int disable_video = show ? 0:1;
  if (saved_disable_video == disable_video)
    return;

  CSysfsPath("/sys/class/video/disable_video", disable_video);
  saved_disable_video = disable_video;
}

void CAMLCodec::SetVideoZoom(const float zoom)
{
  // input zoom range is 0.5 to 2.0 with a default of 1.0.
  // output zoom range is 2 to 300 with default of 100.
  // we limit that to a range of 50 to 200 with default of 100.
  int aml_zoom = 100 * zoom;
  CSysfsPath("/sys/class/video/zoom", aml_zoom);
}

void CAMLCodec::SetVideoRect(const CRect &SrcRect, const CRect &DestRect)
{
  // this routine gets called every video frame
  // and is in the context of the renderer thread so
  // do not do anything stupid here.
  bool update = false;

  // video zoom adjustment.
  float zoom = m_processInfo.GetVideoSettings().m_CustomZoomAmount;
  if ((int)(zoom * 1000) != (int)(m_zoom * 1000))
  {
    m_zoom = zoom;
  }

  // video rate adjustment.
  unsigned int video_rate = GetDecoderVideoRate();
  if (video_rate > 0 && video_rate != am_private->video_rate)
  {
    CLog::Log(LOGDEBUG, "CAMLCodec::SetVideoRect: decoder fps has changed, video_rate adjusted from {:d} to {:d}", am_private->video_rate, video_rate);
    am_private->video_rate = video_rate;
  }

  // video view mode
  int view_mode = m_processInfo.GetVideoSettings().m_ViewMode;
  if (m_view_mode != view_mode)
  {
    m_view_mode = view_mode;
    update = true;
  }

  // GUI stereo mode/view.
  RENDER_STEREO_MODE guiStereoMode = CServiceBroker::GetWinSystem()->GetGfxContext().GetStereoMode();
  if (m_guiStereoMode != guiStereoMode)
  {
    m_guiStereoMode = guiStereoMode;
    update = true;
  }
  RENDER_STEREO_VIEW guiStereoView = CServiceBroker::GetWinSystem()->GetGfxContext().GetStereoView();
  if (m_guiStereoView != guiStereoView)
  {
    // left/right/top/bottom eye,
    // this might change every other frame.
    // we do not care but just track it.
    m_guiStereoView = guiStereoView;
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
        float scale = static_cast<float>(dst_rect.Height()) / dst_rect.Width();
        int diff = (int) ((dst_rect.Height()*scale - dst_rect.Width()) / 2);
        dst_rect = CRect(DestRect.x1 - diff, DestRect.y1, DestRect.x2 + diff, DestRect.y2);
      }

  }

  if (m_dst_rect != dst_rect)
  {
    m_dst_rect  = dst_rect;
    update = true;
  }

  RESOLUTION video_res = CServiceBroker::GetWinSystem()->GetGfxContext().GetVideoResolution();
  if (m_video_res != video_res)
  {
    m_video_res = video_res;
    update = true;
  }

  if (!update)
  {
    // mainvideo 'should' be showing already if we get here, make sure.
    ShowMainVideo(true);
    return;
  }

  CRect gui, display;

  const RESOLUTION_INFO& video_res_info = CDisplaySettings::GetInstance().GetResolutionInfo(video_res);
  display = m_display_rect = CRect(0, 0, video_res_info.iScreenWidth, video_res_info.iScreenHeight);
  gui = CRect(0, 0, video_res_info.iWidth, video_res_info.iHeight);

  if (gui != display)
  {
    float xscale = display.Width() / gui.Width();
    float yscale = display.Height() / gui.Height();
    dst_rect.x1 *= xscale;
    dst_rect.x2 *= xscale;
    dst_rect.y1 *= yscale;
    dst_rect.y2 *= yscale;
  }

  if (m_guiStereoMode == RENDER_STEREO_MODE_MONO)
  {
    std::string videoStereoMode = m_processInfo.GetVideoStereoMode();
    if (videoStereoMode == "left_right" || videoStereoMode == "righ_left")
      dst_rect.x2 *= 2.0f;
    else if (videoStereoMode == "top_bottom" || videoStereoMode == "bottom_top")
      dst_rect.y2 *= 2.0f;
  }
  else if (m_guiStereoMode == RENDER_STEREO_MODE_SPLIT_VERTICAL)
  {
    dst_rect.x2 *= 2.0f;
  }
  else if (m_guiStereoMode == RENDER_STEREO_MODE_SPLIT_HORIZONTAL)
  {
    dst_rect.y2 *= 2.0f;
  }
  else if (m_guiStereoMode == RENDER_STEREO_MODE_HARDWAREBASED)
  {
    // 3D frame packed output: get the screen height from the graphic context
    // (will work in fullscreen mode only)
    RESOLUTION_INFO info = CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo();
    dst_rect.y2 = info.iHeight * 2 + info.iBlanking;
  }

  if (aml_display_support_3d())
  {
    int mvc_view_mode = 3;
    switch (am_private->video_format)
    {
      case VFORMAT_H264MVC:
        {
          mvc_view_mode = m_processInfo.GetVideoStereoMode() == "block_lr" ? 3 : 2;
          switch (m_guiStereoMode)
          {
            case RENDER_STEREO_MODE_HARDWAREBASED:
              aml_set_3d_video_mode(MODE_3D_ENABLE | MODE_3D_FA, true, mvc_view_mode);
              break;
            case RENDER_STEREO_MODE_SPLIT_VERTICAL:
              aml_set_3d_video_mode(MODE_3D_OUT_LR | MODE_3D_FA | MODE_3D_ENABLE, false, mvc_view_mode);
              break;
            case RENDER_STEREO_MODE_SPLIT_HORIZONTAL:
              aml_set_3d_video_mode(MODE_3D_OUT_TB | MODE_3D_FA | MODE_3D_ENABLE, false, mvc_view_mode);
              break;
            default:
              aml_set_3d_video_mode(MODE_3D_TO_2D_R | MODE_3D_FA | MODE_3D_ENABLE, false, mvc_view_mode);
              break;
          }
          break;
        }
      default:
        aml_set_3d_video_mode(MODE_3D_DISABLE, false, mvc_view_mode);
        break;
    }
  }

#if 1
  std::string s_dst_rect = StringUtils::Format("{:d},{:d},{:d},{:d}",
    (int)dst_rect.x1, (int)dst_rect.y1,
    (int)dst_rect.Width(), (int)dst_rect.Height());
  std::string s_m_dst_rect = StringUtils::Format("{:d},{:d},{:d},{:d}",
    (int)m_dst_rect.x1, (int)m_dst_rect.y1,
    (int)m_dst_rect.Width(), (int)m_dst_rect.Height());
  std::string s_display = StringUtils::Format("{:d},{:d},{:d},{:d}",
    (int)m_display_rect.x1, (int)m_display_rect.y1,
    (int)m_display_rect.Width(), (int)m_display_rect.Height());
  std::string s_gui = StringUtils::Format("{:d},{:d},{:d},{:d}",
    (int)gui.x1, (int)gui.y1,
    (int)gui.Width(), (int)gui.Height());
  CLog::Log(LOGDEBUG, "CAMLCodec::SetVideoRect:display({})", s_display.c_str());
  CLog::Log(LOGDEBUG, "CAMLCodec::SetVideoRect:gui({})", s_gui.c_str());
  CLog::Log(LOGDEBUG, "CAMLCodec::SetVideoRect:m_dst_rect({})", s_m_dst_rect.c_str());
  CLog::Log(LOGDEBUG, "CAMLCodec::SetVideoRect:dst_rect({})", s_dst_rect.c_str());
  CLog::Log(LOGDEBUG, "CAMLCodec::SetVideoRect:m_guiStereoMode({:d})", m_guiStereoMode);
  CLog::Log(LOGDEBUG, "CAMLCodec::SetVideoRect:m_guiStereoView({:d})", m_guiStereoView);
#endif

  // goofy 0/1 based difference in aml axis coordinates.
  // fix them.
  dst_rect.x2--;
  dst_rect.y2--;

  char video_axis[256] = {};
  sprintf(video_axis, "%d %d %d %d", (int)dst_rect.x1, (int)dst_rect.y1, (int)dst_rect.x2, (int)dst_rect.y2);

  int screen_mode = CDisplaySettings::GetInstance().IsNonLinearStretched() ? 4 : 1;

  CSysfsPath("/sys/class/video/axis", video_axis);
  CSysfsPath("/sys/class/video/screen_mode", screen_mode);

  // we only get called once gui has changed to something
  // that would show video playback, so show it.
  ShowMainVideo(true);
}

void CAMLCodec::SetVideoRate(int videoRate)
{
  if (am_private)
    am_private->video_rate = videoRate;
}

unsigned int CAMLCodec::GetDecoderVideoRate()
{
  if (m_speed != DVD_PLAYSPEED_NORMAL || m_pollDevice < 0)
    return 0;

  struct vdec_info vi = {};
  if (m_dll->codec_get_vdec_info(&am_private->vcodec, &vi) == 0 && vi.frame_dur > 0)
    return vi.frame_dur;
  else
    return 0;
}

std::string CAMLCodec::GetHDRStaticMetadata()
{
  // add static HDR metadata for VP9 content
  if (am_private->video_format == VFORMAT_VP9 && m_hints.masteringMetadata)
  {
    // for more information, see CTA+861.3-A standard document
    static const double MAX_CHROMATICITY = 50000;
    static const double MAX_LUMINANCE = 10000;
    std::stringstream stream;
    stream << "HDRStaticInfo:1";
    stream << ";mR.x:" << static_cast<int>(av_q2d(m_hints.masteringMetadata->display_primaries[0][0]) * MAX_CHROMATICITY + 0.5);
    stream << ";mR.y:" << static_cast<int>(av_q2d(m_hints.masteringMetadata->display_primaries[0][1]) * MAX_CHROMATICITY + 0.5);
    stream << ";mG.x:" << static_cast<int>(av_q2d(m_hints.masteringMetadata->display_primaries[1][0]) * MAX_CHROMATICITY + 0.5);
    stream << ";mG.y:" << static_cast<int>(av_q2d(m_hints.masteringMetadata->display_primaries[1][1]) * MAX_CHROMATICITY + 0.5);
    stream << ";mB.x:" << static_cast<int>(av_q2d(m_hints.masteringMetadata->display_primaries[2][0]) * MAX_CHROMATICITY + 0.5);
    stream << ";mB.y:" << static_cast<int>(av_q2d(m_hints.masteringMetadata->display_primaries[2][1]) * MAX_CHROMATICITY + 0.5);
    stream << ";mW.x:" << static_cast<int>(av_q2d(m_hints.masteringMetadata->white_point[0]) * MAX_CHROMATICITY + 0.5);
    stream << ";mW.y:" << static_cast<int>(av_q2d(m_hints.masteringMetadata->white_point[1]) * MAX_CHROMATICITY + 0.5);
    stream << ";mMaxDL:" << static_cast<int>(av_q2d(m_hints.masteringMetadata->max_luminance) * MAX_LUMINANCE + 0.5);
    stream << ";mMinDL:" << static_cast<int>(av_q2d(m_hints.masteringMetadata->min_luminance) * MAX_LUMINANCE + 0.5);
    if (m_hints.contentLightMetadata)
    {
      stream << ";mCLLPresent:1";
      stream << ";mMaxCLL:" << m_hints.contentLightMetadata->MaxCLL;
      stream << ";mMaxFALL:" << m_hints.contentLightMetadata->MaxFALL;
    }
    if (m_hints.colorTransferCharacteristic != AVCOL_TRC_UNSPECIFIED)
      stream << ";mTransfer:" << static_cast<int>(m_hints.colorTransferCharacteristic);
    std::string config_data = stream.str();
    CLog::Log(LOGDEBUG, "CAMLCodec::GetHDRStaticMetadata - Created the following config: {}", config_data.c_str());
    return config_data;
  }
  return std::string();
}
