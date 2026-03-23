/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DVDVideoCodec.h"
#include "DynamicDll.h"
#include "cores/VideoPlayer/DVDStreamInfo.h"
#include "cores/IPlayer.h"
#include "windowing/Resolution.h"
#include "rendering/RenderSystem.h"
#include "utils/BitstreamConverter.h"
#include "utils/Geometry.h"

#include <queue>
#include <linux/videodev2.h>
#include <deque>
#include <atomic>
#include <cstdint>

extern "C" {
#include <amcodec/codec.h>
}

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

  virtual int codec_init(codec_para_t *pcodec) = 0;
  virtual int codec_close(codec_para_t *pcodec) = 0;
  virtual int codec_reset(codec_para_t *pcodec) = 0;
  virtual int codec_pause(codec_para_t *pcodec) = 0;
  virtual int codec_resume(codec_para_t *pcodec) = 0;
  virtual int codec_write(codec_para_t *pcodec, void *buffer, int len) = 0;
  virtual int codec_checkin_pts_us64(codec_para_t *pcodec, unsigned long long pts) = 0;
  virtual int codec_get_vbuf_state(codec_para_t *pcodec, struct buf_status *buf) = 0;
  virtual int codec_get_vdec_state(codec_para_t *pcodec, struct vdec_status *vdec) = 0;
  virtual int codec_get_vdec_info(codec_para_t *pcodec, struct vdec_info *vdec) = 0;

  virtual int codec_init_cntl(codec_para_t *pcodec) = 0;
  virtual int codec_poll_cntl(codec_para_t *pcodec) = 0;
  virtual int codec_set_cntl_mode(codec_para_t *pcodec, unsigned int mode) = 0;
  virtual int codec_set_cntl_avthresh(codec_para_t *pcodec, unsigned int avthresh) = 0;
  virtual int codec_set_cntl_syncthresh(codec_para_t *pcodec, unsigned int syncthresh) = 0;

  virtual int codec_set_av_threshold(codec_para_t *pcodec, int threshold) = 0;
  virtual int codec_set_video_delay_limited_ms(codec_para_t *pcodec,int delay_ms) = 0;
  virtual int codec_get_video_delay_limited_ms(codec_para_t *pcodec,int *delay_ms) = 0;
  virtual int codec_get_video_cur_delay_ms(codec_para_t *pcodec,int *delay_ms) = 0;
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
  FFmpegExtraData   extradata;
  DllLibAmCodec     *m_dll;

  int               dumpfile;
  bool              dumpdemux;
} am_private_t;

class PosixFile;
typedef std::shared_ptr<PosixFile> PosixFilePtr;

class CProcessInfo;

struct vpp_pq_ctrl_s {
	unsigned int length;
	union {
		void *ptr;/*point to pq_ctrl_s*/
		long long ptr_length;
	};
};

struct pq_ctrl_s {
	unsigned char sharpness0_en;
	unsigned char sharpness1_en;
	unsigned char dnlp_en;
	unsigned char cm_en;
	unsigned char vadj1_en;
	unsigned char vd1_ctrst_en;
	unsigned char vadj2_en;
	unsigned char post_ctrst_en;
	unsigned char wb_en;
	unsigned char gamma_en;
	unsigned char lc_en;
	unsigned char black_ext_en;
	unsigned char chroma_cor_en;
	unsigned char reserved;
};

#define VE_CM  'C'
#define _VE_CM  'C'
#define AMVECM_IOC_S_PQ_CTRL  _IOW(_VE_CM, 0x69, struct vpp_pq_ctrl_s)
#define AMVECM_IOC_G_PQ_CTRL  _IOR(_VE_CM, 0x6a, struct vpp_pq_ctrl_s)

class CAMLCodec
{
public:
  CAMLCodec(CProcessInfo &processInfo, CDVDStreamInfo &hints);
  virtual ~CAMLCodec();

  bool          OpenDecoder(bool restart);
  void          CloseDecoder(bool restart);
  void          Reset();

  bool          Enable_vadj1();

  bool          AddData(uint8_t *pData, size_t size, double dts, double pts);
  CDVDVideoCodec::VCReturn GetPicture(VideoPicture& videoPicture);

  void          SetSpeed(int speed);
  void          SetDrain(bool drain);
  void          SetVideoRect(const CRect &SrcRect, const CRect &DestRect);
  void          SetVideoRate(int videoRate) const;
  uint64_t      GetOMXPts() const { return m_cur_pts; }
  double        GetPts() const { return static_cast<double>(m_cur_pts); }
  uint32_t      GetBufferIndex() const { return m_bufferIndex; };
  static float  OMXPtsToSeconds(int omxpts);
  static int    OMXDurationToNs(int duration);
  int           GetAmlDuration() const;
  int           ReleaseFrame(const uint32_t index, bool bDrop = false);

  static int    PollFrame();
  static void   SetPollDevice(int device);

private:
  void          ShowMainVideo(const bool show);
  void          SetVideoZoom(const float zoom);
  void          SetVideoContrast(const int contrast);
  void          SetVideoBrightness(const int brightness);
  void          SetVideoSaturation(const int saturation);
  bool          OpenAmlVideo();
  void          CloseAmlVideo();
  std::string   GetVfmMap(const std::string &name) const;
  void          SetVfmMap(const std::string &name, const std::string &map) const;
  float         GetBufferLevel();
  float         GetBufferLevel(int new_chunk, int &data_len, int &free_len) const;

  bool          TryDequeueCaptureBuffer(v4l2_buffer& vbuf);
  bool          GetNextDequeuedBuffer();

  unsigned int  GetDecoderVideoRate() const;
  std::string   GetHDRStaticMetadata() const;

  std::string   IntToFourCCString(unsigned int value) const;
  std::string   GetDoViCodecFourCC(unsigned int codec_tag) const;
  void          SetProcessInfoVideoDetails();
  void          ResetFrameTimeoutClock();

  DllLibAmCodec   *m_dll;
  bool             m_opened;
  bool             m_drain = false;
  am_private_t    *am_private;

  int              m_speed;
  uint64_t         m_cur_pts;
  uint64_t         m_last_pts;
  uint32_t         m_bufferIndex;

  CRect            m_dst_rect;
  CRect            m_display_rect;

  int              m_view_mode = -1;
  RENDER_STEREO_MODE m_guiStereoMode = RENDER_STEREO_MODE_OFF;
  RENDER_STEREO_VIEW m_guiStereoView = RENDER_STEREO_VIEW_OFF;
  float            m_zoom = -1.0f;
  int              m_contrast = -1;
  int              m_brightness = -1;
  bool             m_vadj1_enabled = false;
  RESOLUTION       m_video_res = RES_INVALID;

  static const unsigned int STATE_PREFILLED  = 1;
  static const unsigned int STATE_HASPTS     = 2;

  unsigned int     m_state;

  PosixFilePtr     m_amlVideoFile;
  std::string      m_defaultVfmMap;

  static           std::atomic_flag  m_pollSync;
  static int       m_pollDevice;
  static double    m_ttd;

  CDVDStreamInfo  &m_hints;         // Reference as values can change.
  CProcessInfo    &m_processInfo;
  CDataCacheCore  &m_dataCacheCore;

  int              m_decoder_timeout;
  bool             m_decoder_bypass_buffer_ready;
  float            m_decoder_buffer;
  float            m_decoder_stream_buffer;
  float            m_decoder_minimum_buffer;
  float            m_decoder_minimum_stream_buffer;

  std::chrono::time_point<std::chrono::system_clock> m_tp_last_frame;
  float            m_last_drain_buffer_level{0.0f};

  bool             m_buffer_level_ready;
  float            m_minimum_buffer_level{0.0f};

  std::mutex       m_ioControlMutex;
};
