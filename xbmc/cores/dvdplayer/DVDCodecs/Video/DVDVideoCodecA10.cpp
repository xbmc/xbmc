#include "DVDVideoCodecA10.h"
#include "DVDClock.h"
#include "DVDStreamInfo.h"
#include "utils/log.h"

#include <sys/ioctl.h>
#include <math.h>

extern "C" {
#include "os_adapter.h"
#include "drv_display_sun4i.h"
};

#define A10DEBUG

CDVDVideoCodecA10::CDVDVideoCodecA10()
{
  m_hcedarv = NULL;
  m_hdisp   = -1;
  m_hscaler = -1;
  m_yuvdata = NULL;
  memset(&m_picture, 0, sizeof(m_picture));
}

CDVDVideoCodecA10::~CDVDVideoCodecA10()
{
  Dispose();
}

/*
 * Open the decoder, returns true on success
 */
bool CDVDVideoCodecA10::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  s32 ret;

  if (getenv("NOA10")) {
    CLog::Log(LOGNOTICE, "A10: disabled.\n");
    return false;
  }

  m_aspect = hints.aspect;

  memset(&m_info, 0, sizeof(m_info));
  //m_info.frame_rate       = (double)hints.fpsrate / hints.fpsscale * 1000;
  m_info.frame_duration   = 0;
  m_info.video_width      = hints.width;
  m_info.video_height     = hints.height;
  m_info.aspect_ratio     = 1000;
  m_info.sub_format       = CEDARV_SUB_FORMAT_UNKNOW;

  switch(hints.codec) {
  //TODO: all the mapping ...

  //*CEDARV_STREAM_FORMAT_MPEG2
  case CODEC_ID_MPEG1VIDEO:
    m_info.format     = CEDARV_STREAM_FORMAT_MPEG2;
    m_info.sub_format = CEDARV_MPEG2_SUB_FORMAT_MPEG1;
    break;
  case CODEC_ID_MPEG2VIDEO:
    m_info.format     = CEDARV_STREAM_FORMAT_MPEG2;
    m_info.sub_format = CEDARV_MPEG2_SUB_FORMAT_MPEG2;
    break;

  //*CEDARV_STREAM_FORMAT_H264
  case CODEC_ID_H264:
    m_info.format = CEDARV_STREAM_FORMAT_H264;
    m_info.init_data_len = hints.extrasize;
    m_info.init_data     = (u8*)hints.extradata;
    break;

#if 0 //to be done
  //*CEDARV_STREAM_FORMAT_MPEG4
  case CODEC_ID_MPEG4:
    m_info.format = CEDARV_STREAM_FORMAT_MPEG4;
    break;

    //DIVX4
    //DIVX5
    //SORENSSON_H263
    //H263
    //RMG2

    //VP6
  case CODEC_ID_VP6F:
    m_info.format     = CEDARV_STREAM_FORMAT_MPEG4;
    m_info.sub_format = CEDARV_MPEG4_SUB_FORMAT_VP6;
    m_info.init_data_len = hints.extrasize;
    m_info.init_data     = (u8*)hints.extradata;
    break;
    //WMV1
  case CODEC_ID_WMV1:
    m_info.format     = CEDARV_STREAM_FORMAT_MPEG4;
    m_info.sub_format = CEDARV_MPEG4_SUB_FORMAT_WMV1;
    break;
    //WMV2
  case CODEC_ID_WMV2:
    m_info.format     = CEDARV_STREAM_FORMAT_MPEG4;
    m_info.sub_format = CEDARV_MPEG4_SUB_FORMAT_WMV2;
    break;
    //DIVX1
  case CODEC_ID_MSMPEG4V1:
    m_info.format     = CEDARV_STREAM_FORMAT_MPEG4;
    m_info.sub_format = CEDARV_MPEG4_SUB_FORMAT_DIVX1;
    break;
    //DIVX2
  case CODEC_ID_MSMPEG4V2:
    m_info.format     = CEDARV_STREAM_FORMAT_MPEG4;
    m_info.sub_format = CEDARV_MPEG4_SUB_FORMAT_DIVX2;
    break;
    //DIVX3
  case CODEC_ID_MSMPEG4V3:
    m_info.format     = CEDARV_STREAM_FORMAT_MPEG4;
    m_info.sub_format = CEDARV_MPEG4_SUB_FORMAT_DIVX3;
    break;

  //*CEDARV_STREAM_FORMAT_REALVIDEO

  //*CEDARV_STREAM_FORMAT_VC1

  //*CEDARV_STREAM_FORMAT_AVS

  //*CEDARV_STREAM_FORMAT_MJPEG
  case CODEC_ID_MJPEG:
    m_info.format = CEDARV_STREAM_FORMAT_MJPEG;
    break;

  //*CEDARV_STREAM_FORMAT_VP8
  case CODEC_ID_VP8:
    m_info.format = CEDARV_STREAM_FORMAT_VP8;
    break;
#endif

  //*
  default:
    CLog::Log(LOGERROR, "A10: codecid %d is unknown.\n", hints.codec);
    return false;
  }

  m_hcedarv = libcedarv_init(&ret);
  if (ret < 0) {
    CLog::Log(LOGERROR, "A10: libcedarv_init failed. (%d)\n", ret);
    return false;
  }

  ret = m_hcedarv->set_vstream_info(m_hcedarv, &m_info);
  if (ret < 0) {
    CLog::Log(LOGERROR, "A10: set_vstream_m_info failed. (%d)\n", ret);
    return false;
  }

  ret = m_hcedarv->open(m_hcedarv);
  if(ret < 0) {
    CLog::Log(LOGERROR, "A10: open failed. (%d)\n", ret);
    return false;
  }

  ret = m_hcedarv->ioctrl(m_hcedarv, CEDARV_COMMAND_PLAY, 0);
  if (ret < 0) {
    CLog::Log(LOGERROR, "A10: CEDARV_COMMAND_PLAY failed. (%d)\n", ret);
    return false;
  }

#ifndef A10DEBUG
  //*open scaler once
  if (!scaler_open) {
    CLog::Log(LOGERROR, "A10: scaler_open failed.\n");
    return false;
  }
#endif

  CLog::Log(LOGDEBUG, "A10: cedar open.");

  return true;
}

/*
 * Dispose, Free all resources
 */
void CDVDVideoCodecA10::Dispose()
{
  if (m_yuvdata) {
    mem_pfree(m_yuvdata);
    m_yuvdata = NULL;
  }
  scaler_close();
  if (m_hcedarv) {
    m_hcedarv->ioctrl(m_hcedarv, CEDARV_COMMAND_STOP, 0);
    m_hcedarv->close(m_hcedarv);
    libcedarv_exit(m_hcedarv);
    m_hcedarv = NULL;
    CLog::Log(LOGDEBUG, "cedar dispose.");
  }
}

/*
 * returns one or a combination of VC_ messages
 * pData and iSize can be NULL, this means we should flush the rest of the data.
 */
int CDVDVideoCodecA10::Decode(BYTE* pData, int iSize, double dts, double pts)
{
  s32                        ret;
  u8                        *buf0, *buf1;
  u32                        bufsize0, bufsize1;
  cedarv_stream_data_info_t  dinf;
  cedarv_picture_t           picture;

  ret = m_hcedarv->request_write(m_hcedarv, iSize, &buf0, &bufsize0, &buf1, &bufsize1);
  if(ret < 0) {
    CLog::Log(LOGERROR, "request_write failed.\n");
    return VC_ERROR;
  }
  if (bufsize1) {
    memcpy(buf0, pData, bufsize0);
    memcpy(buf1, pData+bufsize0, bufsize1);
  } else {
    memcpy(buf0, pData, iSize);
  }

  memset(&dinf, 0, sizeof(dinf));
  dinf.lengh = iSize;
#ifdef CEDARV_FLAG_DECODE_NO_DELAY
  dinf.flags = CEDARV_FLAG_FIRST_PART | CEDARV_FLAG_LAST_PART | CEDARV_FLAG_DECODE_NO_DELAY;
#else
  dinf.flags = CEDARV_FLAG_FIRST_PART | CEDARV_FLAG_LAST_PART;
#endif
  m_hcedarv->update_data(m_hcedarv, &dinf);

  ret = m_hcedarv->decode(m_hcedarv);

#ifdef A10DEBUG
  if (ret > 3 || ret < 0) {
    printf("decode(%d): %d\n", iSize, ret);
  }
#endif

  ret = m_hcedarv->display_request(m_hcedarv, &picture);

  if (ret == 0) {
    ScalerParameter cdx_scaler_para;
    u32 width32;
    u32 height32;
    u32 height64;
    u32 ysize;
    u32 csize;

    width32  = (picture.display_width  + 31) & ~31;
    height32 = (picture.display_height + 31) & ~31;
    height64 = (picture.display_height + 63) & ~63;

    ysize = width32*height32;   //* for y.
    csize = width32*height64/2; //* for u and v together.

    if (!m_yuvdata) {
      m_yuvdata = (u8*)mem_palloc(ysize + csize, 1024);
      if (!m_yuvdata) {
        CLog::Log(LOGERROR, "A10: can not alloc m_yuvdata!");
        m_hcedarv->display_release(m_hcedarv, picture.id);
        return VC_ERROR;
      }
    }

    cdx_scaler_para.width_in   = picture.width;
    cdx_scaler_para.height_in  = picture.height;
    cdx_scaler_para.addr_y_in  = mem_get_phy_addr((u32)picture.y);
    cdx_scaler_para.addr_c_in  = mem_get_phy_addr((u32)picture.u);
    cdx_scaler_para.width_out  = picture.display_width;
    cdx_scaler_para.height_out = picture.display_height;
    cdx_scaler_para.addr_y_out = mem_get_phy_addr((u32)m_yuvdata);
    cdx_scaler_para.addr_u_out = cdx_scaler_para.addr_y_out + ysize;
    cdx_scaler_para.addr_v_out = cdx_scaler_para.addr_u_out + csize/2;

    m_picture.iWidth  = picture.width;
    m_picture.iHeight = picture.height;

    /* XXX: we suppose the screen has a 1.0 pixel ratio */ // CDVDVideo will compensate it.
    m_picture.iDisplayHeight = m_picture.iHeight;
    m_picture.iDisplayWidth  = ((int)lrint(m_picture.iHeight * m_aspect)) & -3;
    if (m_picture.iDisplayWidth == 0) {
      m_picture.iDisplayWidth = m_picture.iHeight;
    }
    else if (m_picture.iDisplayWidth > m_picture.iWidth)
    {
      m_picture.iDisplayWidth  = m_picture.iWidth;
      m_picture.iDisplayHeight = ((int)lrint(m_picture.iWidth / m_aspect)) & -3;
    }

    m_picture.pts    = pts;
    m_picture.dts    = dts;
    m_picture.format = RENDER_FMT_YUV420P;
    if (picture.is_progressive) m_picture.iFlags &= ~DVP_FLAG_INTERLACED;
    else                        m_picture.iFlags |= DVP_FLAG_INTERLACED;

    if (!(m_picture.iFlags & DVP_FLAG_ALLOCATED)) {
      u32 width16  = (picture.display_width  + 15) & ~15;

      m_picture.iFlags |= DVP_FLAG_ALLOCATED;

      m_picture.iLineSize[0] = width16;   //Y
      m_picture.iLineSize[1] = width16/2; //U
      m_picture.iLineSize[2] = width16/2; //V
      m_picture.iLineSize[3] = 0;

      m_picture.data[0] = m_yuvdata;
      m_picture.data[1] = m_yuvdata+ysize;
      m_picture.data[2] = m_yuvdata+ysize+csize/2;

#ifdef A10DEBUG
      CLog::Log(LOGDEBUG, "A10: p1=%d %d %d %d (%d)\n", picture.width, picture.height, picture.display_width, picture.display_height, picture.pixel_format);
      CLog::Log(LOGDEBUG, "A10: p2=%d %d %d %d\n", m_picture.iWidth, m_picture.iHeight, m_picture.iDisplayWidth, m_picture.iDisplayHeight);
#endif
    }

    if (!HardwarePictureScaler(&cdx_scaler_para)) {
      CLog::Log(LOGERROR, "hardware scaler failed.\n");
      m_hcedarv->display_release(m_hcedarv, picture.id);
      return VC_ERROR;
    }

    m_hcedarv->display_release(m_hcedarv, picture.id);

    return VC_PICTURE | VC_BUFFER;
  }

  return VC_BUFFER;
}

/*
 * Reset the decoder.
 * Should be the same as calling Dispose and Open after each other
 */
void CDVDVideoCodecA10::Reset()
{
  CLog::Log(LOGERROR, "A10: attention: reset!\n");
}

/*
 * returns true if successfull
 * the data is valid until the next Decode call
 */
bool CDVDVideoCodecA10::GetPicture(DVDVideoPicture* pDvdVideoPicture)
{
  if (m_picture.iFlags & DVP_FLAG_ALLOCATED) {
    *pDvdVideoPicture = m_picture;
    return true;
  }
  return false;
}

void CDVDVideoCodecA10::SetDropState(bool bDrop)
{
}

const char* CDVDVideoCodecA10::GetName()
{
  return "A10";
}

bool CDVDVideoCodecA10::HardwarePictureScaler(ScalerParameter *cdx_scaler_para)
{
  unsigned long        arg[4] = {0,0,0,0};
  __disp_scaler_para_t scaler_para;
  bool                 is_open;

  memset(&scaler_para, 0, sizeof(__disp_scaler_para_t));
  scaler_para.input_fb.addr[0] = cdx_scaler_para->addr_y_in;//
  scaler_para.input_fb.addr[1] = cdx_scaler_para->addr_c_in;//
  scaler_para.input_fb.size.width = cdx_scaler_para->width_in;//
  scaler_para.input_fb.size.height = cdx_scaler_para->height_in;//
  scaler_para.input_fb.format =  DISP_FORMAT_YUV420;
  scaler_para.input_fb.seq = DISP_SEQ_UVUV;
  scaler_para.input_fb.mode = DISP_MOD_MB_UV_COMBINED;
  scaler_para.input_fb.br_swap = 0;
  scaler_para.input_fb.cs_mode = DISP_BT601;
  scaler_para.source_regn.x = 0;
  scaler_para.source_regn.y = 0;
  scaler_para.source_regn.width = cdx_scaler_para->width_in;//
  scaler_para.source_regn.height = cdx_scaler_para->height_in;//

  scaler_para.output_fb.addr[0] = (unsigned int)cdx_scaler_para->addr_y_out;//
  scaler_para.output_fb.addr[1] = cdx_scaler_para->addr_u_out;//
  scaler_para.output_fb.addr[2] = cdx_scaler_para->addr_v_out;//
  scaler_para.output_fb.size.width = cdx_scaler_para->width_out;//
  scaler_para.output_fb.size.height = cdx_scaler_para->height_out;//
  scaler_para.output_fb.format = DISP_FORMAT_YUV420;
  scaler_para.output_fb.seq  = DISP_SEQ_P3210;
  scaler_para.output_fb.mode = DISP_MOD_NON_MB_PLANAR;
  scaler_para.output_fb.br_swap = 0;
  scaler_para.output_fb.cs_mode = DISP_YCC;

  is_open = m_hdisp != -1;

  if (!is_open && !scaler_open()) {
    CLog::Log(LOGERROR, "A10: scale_open failed.\n");
    return false;
  }

  arg[1] = m_hscaler;
  arg[2] = (unsigned long) &scaler_para;
  ioctl(m_hdisp, DISP_CMD_SCALER_EXECUTE, (unsigned long) arg);

  if (!is_open) {
    scaler_close();
  }

  return true;
}

bool CDVDVideoCodecA10::scaler_open()
{
  unsigned long arg[4] = {0,0,0,0};

  m_hdisp = open("/dev/disp", O_RDWR);
  if (m_hdisp == -1) {
    CLog::Log(LOGERROR, "A10: open /dev/disp failed. (%d)", errno);
    return false;
  }

  m_hscaler = ioctl(m_hdisp, DISP_CMD_SCALER_REQUEST, (unsigned long) arg);
  if (m_hscaler == -1) {
    CLog::Log(LOGERROR, "A10: DISP_CMD_SCALER_REQUEST failed. (%d)", errno);
    return false;
  }

  return true;
}

void CDVDVideoCodecA10::scaler_close()
{
  unsigned long arg[4] = {0,0,0,0};

  if (m_hscaler != -1) {
    arg[1] = m_hscaler;
    ioctl(m_hdisp, DISP_CMD_SCALER_RELEASE, (unsigned long) arg);
    m_hscaler = -1;
  }
  if (m_hdisp != -1) {
    close(m_hdisp);
    m_hdisp = -1;
  }
}
