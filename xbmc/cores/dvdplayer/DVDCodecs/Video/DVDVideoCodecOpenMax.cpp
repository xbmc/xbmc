/*
 *      Copyright (C) 2010 Team XBMC
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
#elif defined(_WIN32)
#include "system.h"
#endif

#if defined(HAVE_LIBOPENMAX)
#include "DynamicDll.h"
#include "GUISettings.h"
#include "DVDClock.h"
#include "DVDStreamInfo.h"
#include "DVDVideoCodecOpenMax.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"

#include <OMX_Core.h>
#include <OMX_Component.h>
#include <OMX_Index.h>
#include <OMX_Image.h>

////////////////////////////////////////////////////////////////////////////////////////////
class DllLibOpenMaxInterface
{
public:
  virtual ~DllLibOpenMaxInterface() {}

  virtual OMX_ERRORTYPE OMX_Init(void) = 0;
  virtual OMX_ERRORTYPE OMX_Deinit(void) = 0;
  virtual OMX_ERRORTYPE OMX_GetHandle(
    OMX_HANDLETYPE *pHandle, OMX_STRING cComponentName, OMX_PTR pAppData, OMX_CALLBACKTYPE *pCallBacks) = 0;
  virtual OMX_ERRORTYPE OMX_FreeHandle(OMX_HANDLETYPE hComponent) = 0;
  virtual OMX_ERRORTYPE OMX_GetComponentsOfRole(OMX_STRING role, OMX_U32 *pNumComps, OMX_U8 **compNames) = 0;
  virtual OMX_ERRORTYPE OMX_GetRolesOfComponent(OMX_STRING compName, OMX_U32 *pNumRoles, OMX_U8 **roles) = 0;
  virtual OMX_ERRORTYPE OMX_ComponentNameEnum(OMX_STRING cComponentName, OMX_U32 nNameLength, OMX_U32 nIndex) = 0;
};

class DllLibOpenMax : public DllDynamic, DllLibOpenMaxInterface
{
  DECLARE_DLL_WRAPPER(DllLibOpenMax, "/usr/lib/libnvomx.so")

  DEFINE_METHOD0(OMX_ERRORTYPE, OMX_Init)
  DEFINE_METHOD0(OMX_ERRORTYPE, OMX_Deinit)
  DEFINE_METHOD4(OMX_ERRORTYPE, OMX_GetHandle, (OMX_HANDLETYPE *p1, OMX_STRING p2, OMX_PTR p3, OMX_CALLBACKTYPE *p4))
  DEFINE_METHOD1(OMX_ERRORTYPE, OMX_FreeHandle, (OMX_HANDLETYPE p1))
  DEFINE_METHOD3(OMX_ERRORTYPE, OMX_GetComponentsOfRole, (OMX_STRING p1, OMX_U32 *p2, OMX_U8 **p3))
  DEFINE_METHOD3(OMX_ERRORTYPE, OMX_GetRolesOfComponent, (OMX_STRING p1, OMX_U32 *p2, OMX_U8 **p3))
  DEFINE_METHOD3(OMX_ERRORTYPE, OMX_ComponentNameEnum, (OMX_STRING p1, OMX_U32 p2, OMX_U32 p3))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(OMX_Init)
    RESOLVE_METHOD(OMX_Deinit)
    RESOLVE_METHOD(OMX_GetHandle)
    RESOLVE_METHOD(OMX_FreeHandle)
    RESOLVE_METHOD(OMX_GetComponentsOfRole)
    RESOLVE_METHOD(OMX_GetRolesOfComponent)
    RESOLVE_METHOD(OMX_ComponentNameEnum)
  END_METHOD_RESOLVE()
};

////////////////////////////////////////////////////////////////////////////////////////////
// debug output defines
//#define OMX_INTERNAL_INPUT_BUFFERS
#define OMX_INTERNAL_OUTPUT_BUFFERS
//#define OMX_DEBUG_EVENTHANDLER
#define OMX_DEBUG_FILLBUFFERDONE
//#define OMX_DEBUG_EMPTYBUFFERDONE

#define OMX_H264BASE_DECODER    "OMX.Nvidia.h264.decode"
// OMX.Nvidia.h264ext.decode segfaults, not sure why.
//#define OMX_H264MAIN_DECODER  "OMX.Nvidia.h264ext.decode"
#define OMX_H264MAIN_DECODER    "OMX.Nvidia.h264.decode"
#define OMX_H264HIGH_DECODER    "OMX.Nvidia.h264ext.decode"
#define OMX_MPEG4_DECODER       "OMX.Nvidia.mp4.decode"
#define OMX_MPEG4EXT_DECODER    "OMX.Nvidia.mp4ext.decode"
#define OMX_MPEG2V_DECODER      "OMX.Nvidia.mpeg2v.decode"
#define OMX_VC1_DECODER         "OMX.Nvidia.vc1.decode"

// quick and dirty scalers to calc image buffer sizes
#define FACTORFORMAT422 2
#define FACTORFORMAT420 1.5

#if OMXVERSION > 1
#define OMX_INIT_STRUCTURE(a) \
  memset(&(a), 0, sizeof(a)); \
  (a).nSize = sizeof(a); \
  (a).nVersion.s.nVersionMajor = 1; \
  (a).nVersion.s.nVersionMinor = 1; \
  (a).nVersion.s.nRevision = 0; \
  (a).nVersion.s.nStep = 0
#else
#define OMX_INIT_STRUCTURE(a) \
  memset(&(a), 0, sizeof(a)); \
  (a).nSize = sizeof(a); \
  (a).nVersion.s.nVersionMajor = 1; \
  (a).nVersion.s.nVersionMinor = 0; \
  (a).nVersion.s.nRevision = 0; \
  (a).nVersion.s.nStep = 0
#endif

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
CDVDVideoCodecOpenMax::CDVDVideoCodecOpenMax() : CDVDVideoCodec()
{
  m_dll = new DllLibOpenMax;
  m_dll->Load();

  m_omx_decoder = NULL;
  m_pFormatName = "omx-xxxx";

  pthread_mutex_init(&m_free_input_mutex, NULL);
  pthread_mutex_init(&m_ready_output_mutex, NULL);

  m_omx_state_change = (sem_t*)malloc(sizeof(sem_t));
  sem_init(m_omx_state_change, 0, 0);
  m_videoplayback_done = false;

  m_convert_bitstream = false;
  memset(&m_videobuffer, 0, sizeof(DVDVideoPicture));
}

CDVDVideoCodecOpenMax::~CDVDVideoCodecOpenMax()
{
  Dispose();
  pthread_mutex_destroy(&m_ready_output_mutex);
  pthread_mutex_destroy(&m_free_input_mutex);
  sem_destroy(m_omx_state_change);
  free(m_omx_state_change);
  delete m_dll;
}

bool CDVDVideoCodecOpenMax::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  if (g_guiSettings.GetBool("videoplayer.useomx") && !hints.software)
  {
    OMX_ERRORTYPE omx_err = OMX_ErrorNone;
    std::string decoder_name;

    m_decoded_width  = hints.width;
    m_decoded_height = hints.height;
    m_convert_bitstream = false;

    switch (hints.codec)
    {
      case CODEC_ID_H264:
      {
        switch(hints.profile)
        {
          case FF_PROFILE_H264_BASELINE:
            // (role name) video_decoder.avc
            // H.264 Baseline profile
            decoder_name = OMX_H264BASE_DECODER;
            m_pFormatName = "omx-h264(b)";
          break;
          case FF_PROFILE_H264_MAIN:
            // (role name) video_decoder.avc
            // H.264 Main profile
            decoder_name = OMX_H264MAIN_DECODER;
            m_pFormatName = "omx-h264(m)";
          break;
          case FF_PROFILE_H264_HIGH:
            // (role name) video_decoder.avc
            // H.264 Main profile
            decoder_name = OMX_H264HIGH_DECODER;
            m_pFormatName = "omx-h264(h)";
          break;
          default:
            return false;
          break;
        }
        if (hints.extrasize < 7 || hints.extradata == NULL)
        {
          CLog::Log(LOGNOTICE, "%s - avcC atom too data small or missing", __FUNCTION__);
          return false;
        }
        // valid avcC atom data always starts with the value 1 (version)
        if ( *(char*)hints.extradata == 1 )
          m_convert_bitstream = bitstream_convert_init((uint8_t*)hints.extradata, hints.extrasize);
      }
      break;
      case CODEC_ID_MPEG4:
        // (role name) video_decoder.mpeg4
        // MPEG-4, DivX 4/5 and Xvid compatible
        decoder_name = OMX_MPEG4_DECODER;
        m_pFormatName = "omx-mpeg4";
      break;
      /*
      TODO: what mpeg4 formats are "ext" ????
      case NvxStreamType_MPEG4Ext:
        // (role name) video_decoder.mpeg4
        // MPEG-4, DivX 4/5 and Xvid compatible
        decoder_name = OMX_MPEG4EXT_DECODER;
        m_pFormatName = "omx-mpeg4";
      break;
      */
      case CODEC_ID_MPEG2VIDEO:
        // (role name) video_decoder.mpeg2
        // MPEG-2
        decoder_name = OMX_MPEG2V_DECODER;
        m_pFormatName = "omx-mpeg2";
        CLog::Log(LOGDEBUG, "%s - video format is MPEG2\n", __FUNCTION__);
      break;
      case CODEC_ID_VC1:
        // (role name) video_decoder.vc1
        // VC-1, WMV9
        decoder_name = OMX_VC1_DECODER;
        m_pFormatName = "omx-vc1";
      break;
      default:
        return false;
      break;
    }

    try
    {
      // initialize OpenMAX.
      omx_err = m_dll->OMX_Init();
      if (omx_err)
      {
        CLog::Log(LOGERROR, "%s - OpenMax Codec failed to init, status(%d), codec(%d), profile(%d), level(%d)", 
          __FUNCTION__, omx_err, hints.codec, hints.profile, hints.level);
        return false;
      }

      // TODO: Find component from role name.

      // Get video decoder handle setting up callbacks, component is in loaded state on return.
      static OMX_CALLBACKTYPE decoder_callbacks = {
        &DecoderEventHandler, &DecoderEmptyBufferDone, &DecoderFillBufferDone
      };
      omx_err = m_dll->OMX_GetHandle(&m_omx_decoder, (char*)decoder_name.c_str(), this, &decoder_callbacks);
      if (omx_err)
      {
        CLog::Log(LOGERROR, "%s - could not get decoder handle\n", __FUNCTION__);
        m_dll->OMX_Deinit();
        return false;
      }

      // Get the port information. This will obtain information about the
      // number of ports and index of the first port.
      OMX_PORT_PARAM_TYPE port_param;
      OMX_INIT_STRUCTURE(port_param);
      omx_err = OMX_GetParameter(m_omx_decoder, OMX_IndexParamVideoInit, &port_param);
      if (omx_err)
      {
        CLog::Log(LOGERROR, "%s - failed to get component port parameter\n", __FUNCTION__);
        m_dll->OMX_FreeHandle(m_omx_decoder);
        m_dll->OMX_Deinit();
        return false;
      }
      m_input_port = port_param.nStartPortNumber;
      m_output_port = m_input_port + 1;
      CLog::Log(LOGDEBUG, "%s - decoder_component(0x%p), input_port(0x%x), output_port(0x%x)\n", 
        __FUNCTION__, m_omx_decoder, m_input_port, m_output_port);

      // TODO: Set role for the component because components could have multiple roles.

      // Component will be in OMX_StateLoaded so we can alloc input/output buffers.
      // we can only alloc them in OMX_StateLoaded state or if the component port is disabled 
      // Alloc buffers for the input port.
      omx_err = AllocOMXInputBuffers();
      if (omx_err)
      {
        m_dll->OMX_FreeHandle(m_omx_decoder);
        m_dll->OMX_Deinit();
        return false;
      }
      // Alloc buffers for the output port.
      omx_err = AllocOMXOutputBuffers();
      if (omx_err)
      {
        m_dll->OMX_FreeHandle(m_omx_decoder);
        m_dll->OMX_Deinit();
        return false;
      }
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "%s - OpenMax Codec failed to init, status(%d), codec(%d), profile(%d), level(%d)", 
        __FUNCTION__, omx_err, hints.codec, hints.profile, hints.level);
      m_dll->OMX_FreeHandle(m_omx_decoder);
      m_dll->OMX_Deinit();
      return false;
    }

    // allocate a YV12 DVDVideoPicture buffer.
    // first make sure all properties are reset.
    memset(&m_videobuffer, 0, sizeof(DVDVideoPicture));
    unsigned int luma_pixels = m_decoded_width * m_decoded_height;
    unsigned int chroma_pixels = luma_pixels/4;

    m_videobuffer.pts = DVD_NOPTS_VALUE;
    m_videobuffer.iFlags = DVP_FLAG_ALLOCATED;
    m_videobuffer.format = DVDVideoPicture::FMT_YUV420P;
    m_videobuffer.color_range  = 0;
    m_videobuffer.color_matrix = 4;
    m_videobuffer.iWidth  = m_decoded_width;
    m_videobuffer.iHeight = m_decoded_height;
    m_videobuffer.iDisplayWidth  = m_decoded_width;
    m_videobuffer.iDisplayHeight = m_decoded_height;

    m_videobuffer.iLineSize[0] = m_decoded_width;   //Y
    m_videobuffer.iLineSize[1] = m_decoded_width/2; //U
    m_videobuffer.iLineSize[2] = m_decoded_width/2; //V
    m_videobuffer.iLineSize[3] = 0;

    m_videobuffer.data[0] = (BYTE*)_aligned_malloc(luma_pixels, 16);  //Y
    m_videobuffer.data[1] = (BYTE*)_aligned_malloc(chroma_pixels, 16);//U
    m_videobuffer.data[2] = (BYTE*)_aligned_malloc(chroma_pixels, 16);//V
    m_videobuffer.data[3] = NULL;

    // set all data to 0 for less artifacts.. hmm.. what is black in YUV??
    memset(m_videobuffer.data[0], 0, luma_pixels);
    memset(m_videobuffer.data[1], 0, chroma_pixels);
    memset(m_videobuffer.data[2], 0, chroma_pixels);

    m_drop_pictures = false;
    m_videoplayback_done = false;
    m_convert_bitstream = false;
    
    StartDecoder();

    return true;
  }

  return false;
}

void CDVDVideoCodecOpenMax::Dispose()
{
  if (m_omx_decoder)
  {
    m_omx_decoder = NULL;
  }
  if (m_videobuffer.iFlags & DVP_FLAG_ALLOCATED)
  {
    _aligned_free(m_videobuffer.data[0]);
    _aligned_free(m_videobuffer.data[1]);
    _aligned_free(m_videobuffer.data[2]);
    m_videobuffer.iFlags = 0;
  }
  if (m_sps_pps_context.sps_pps_data)
		free(m_sps_pps_context.sps_pps_data);

}

void CDVDVideoCodecOpenMax::SetDropState(bool bDrop)
{
  m_drop_pictures = bDrop;
  
  if (m_drop_pictures)
  {
    OMX_ERRORTYPE omx_err;
    omx_err = OMX_SendCommand(m_omx_decoder, OMX_CommandFlush, m_output_port, 0);
  }
}

int CDVDVideoCodecOpenMax::Decode(BYTE* pData, int iSize, double dts, double pts)
{
  if (pData)
  {
    int demuxer_bytes = iSize;
    uint8_t *demuxer_content = pData;
    bool bitstream_convered = false;

    if (m_convert_bitstream)
    {
      // convert demuxer packet from bitstream to bytestream (AnnexB)
      int bytestream_size = 0;
      uint8_t *bytestream_buff = NULL;

      bitstream_convert(demuxer_content, demuxer_bytes, &bytestream_buff, &bytestream_size);
      if (bytestream_buff && (bytestream_size > 0))
      {
        bitstream_convered = true;
        demuxer_bytes = bytestream_size;
        demuxer_content = bytestream_buff;
      }
    }

    // wait for an omx input buffer, we can look at empty without needing to lock/unlock
    if (!m_free_input_buffers.empty())
      return VC_ERROR;

    pthread_mutex_lock(&m_free_input_mutex);
    OMX_BUFFERHEADERTYPE* omx_buffer = m_free_input_buffers.front();
    m_free_input_buffers.pop();
    pthread_mutex_unlock(&m_free_input_mutex);

#if !defined(OMX_INTERNAL_INPUT_BUFFERS)
    // creates a new buffer to hold copy of demux packet, delete old one
    delete [] ((uint8_t*)omx_buffer->pBuffer);
    omx_buffer->nAllocLen = demuxer_bytes;
    omx_buffer->pBuffer = new uint8_t[demuxer_bytes];
#endif
    // setup omx_buffer.
    memcpy(omx_buffer->pBuffer, demuxer_content, demuxer_bytes); 
    omx_buffer->nInputPortIndex = m_input_port;
    omx_buffer->nOffset = 0;
    omx_buffer->nFlags = 0;
    omx_buffer->nFlags |= m_input_eos ? OMX_BUFFERFLAG_EOS : 0;
    omx_buffer->nFilledLen = demuxer_bytes;
    omx_buffer->nTimeStamp = pts * 1000.0; // in microseconds;
    omx_buffer->pAppPrivate = this;

#if defined(OMX_DEBUG_EMPTYBUFFERDONE)
    CLog::Log(LOGDEBUG, "%s - omx_buffer->pBuffer(0x%p), demux_byte_count(%d)\n",
      __FUNCTION__, omx_buffer->pBuffer, demux_byte_count);
#endif
    // Give this buffer to OMX.
    OMX_ERRORTYPE omx_err = OMX_EmptyThisBuffer(m_omx_decoder, omx_buffer);
    if (bitstream_convered)
      free(demuxer_content);
    if (omx_err)
    {
      CLog::Log(LOGDEBUG, "%s - OMX_EmptyThisBuffer() failed with result(0x%x)\n", __FUNCTION__, omx_err);
      return VC_ERROR;
    }
    m_dts_queue.push(dts);
  }

  // TODO: queue depth is related to the number of reference frames in encoded h.264.
  // so we need to buffer until we get N ref frames + 1.
  if (m_ready_output_buffers.size() < 4)
    return VC_BUFFER;

  return VC_PICTURE | VC_BUFFER;
}

void CDVDVideoCodecOpenMax::Reset(void)
{
  StopDecoder();
  while (!m_dts_queue.empty())
    m_dts_queue.pop();
  
  StartDecoder();
}

bool CDVDVideoCodecOpenMax::GetPicture(DVDVideoPicture* pDvdVideoPicture)
{
  if (!m_ready_output_buffers.empty())
  {
    // fetch a buffer and pop it off the ready list
    pthread_mutex_lock(&m_ready_output_mutex);
    OMX_BUFFERHEADERTYPE *omx_buffer = m_ready_output_buffers.front();
    m_ready_output_buffers.pop();
    pthread_mutex_unlock(&m_ready_output_mutex);

    bool done = omx_buffer->nFlags & OMX_BUFFERFLAG_EOS;
    if (!done && (omx_buffer->nFilledLen > 0))
    {
      m_videobuffer.dts = m_dts_queue.front();
      m_dts_queue.pop();
      // nTimeStamp is in microseconds
      m_videobuffer.pts = (double)omx_buffer->nTimeStamp / 1000.0;

      // not sure about yv12 buffer layout coming from openmax decoder
      int luma_pixels = m_decoded_width * m_decoded_height;
      int chroma_pixels = luma_pixels/4;
      uint8_t *image_buffer = omx_buffer->pBuffer;
      memcpy(m_videobuffer.data[0], image_buffer, luma_pixels);
      image_buffer += luma_pixels;
      memcpy(m_videobuffer.data[1], image_buffer, chroma_pixels);
      image_buffer += chroma_pixels;
      memcpy(m_videobuffer.data[2], image_buffer, chroma_pixels);

      // release the buffer back to OpenMax to fill.
      OMX_ERRORTYPE omx_err = OMX_FillThisBuffer(m_omx_decoder, omx_buffer);
      // If we get OMX_ErrorPortUnpopulated, it's not really a problem, the component
      // output got OMX_CommandPortDisable from OMX_EventPortSettingsChanged
      // and is purging output buffers.
      if (omx_err && omx_err != OMX_ErrorPortUnpopulated)
      {
        CLog::Log(LOGDEBUG, "%s - OMX_FillThisBuffer, omx_err(0x%x)\n", __FUNCTION__, omx_err);
      }
    }
  }
  *pDvdVideoPicture = m_videobuffer;

  return VC_PICTURE | VC_BUFFER;
}

////////////////////////////////////////////////////////////////////////////////////////////
bool CDVDVideoCodecOpenMax::bitstream_convert_init(uint8_t *in_extradata, int in_extrasize)
{
  // based on h264_mp4toannexb_bsf.c (ffmpeg)
  // which is Copyright (c) 2007 Benoit Fouet <benoit.fouet@free.fr>
  // and Licensed GPL 2.1 or greater

  m_sps_pps_size = 0;
  m_sps_pps_context.sps_pps_data = NULL;
  
  // nothing to filter
  if (!in_extradata || in_extrasize < 6)
    return false;

  uint16_t unit_size;
  uint32_t total_size = 0;
  uint8_t *out = NULL, unit_nb, sps_done = 0;
  const uint8_t *extradata = (uint8_t*)in_extradata + 4;
  static const uint8_t nalu_header[4] = {0, 0, 0, 1};

  // retrieve length coded size
  m_sps_pps_context.length_size = (*extradata++ & 0x3) + 1;
  if (m_sps_pps_context.length_size == 3)
    return false;

  // retrieve sps and pps unit(s)
  unit_nb = *extradata++ & 0x1f;  // number of sps unit(s)
  if (!unit_nb)
  {
    unit_nb = *extradata++;       // number of pps unit(s)
    sps_done++;
  }
  while (unit_nb--)
  {
    unit_size = extradata[0] << 8 | extradata[1];
    total_size += unit_size + 4;
    if ( (extradata + 2 + unit_size) > ((uint8_t*)in_extradata + in_extrasize) )
    {
      free(out);
      return false;
    }
    out = (uint8_t*)realloc(out, total_size);
    if (!out)
      return false;

    memcpy(out + total_size - unit_size - 4, nalu_header, 4);
    memcpy(out + total_size - unit_size, extradata + 2, unit_size);
    extradata += 2 + unit_size;

    if (!unit_nb && !sps_done++)
      unit_nb = *extradata++;     // number of pps unit(s)
  }

  m_sps_pps_context.sps_pps_data = out;
  m_sps_pps_context.size = total_size;
  m_sps_pps_context.first_idr = 1;

  return true;
}

bool CDVDVideoCodecOpenMax::bitstream_convert(BYTE* pData, int iSize, uint8_t **poutbuf, int *poutbuf_size)
{
  // based on h264_mp4toannexb_bsf.c (ffmpeg)
  // which is Copyright (c) 2007 Benoit Fouet <benoit.fouet@free.fr>
  // and Licensed GPL 2.1 or greater

  uint8_t   *buf = pData;
  uint32_t  buf_size = iSize;
  uint8_t   unit_type;
  int32_t nal_size;
  uint32_t cumul_size = 0;
  const uint8_t *buf_end = buf + buf_size;

  do
  {
    if (buf + m_sps_pps_context.length_size > buf_end)
      goto fail;

    if (m_sps_pps_context.length_size == 1)
      nal_size = buf[0];
    else if (m_sps_pps_context.length_size == 2)
      nal_size = buf[0] << 8 | buf[1];
    else
      nal_size = buf[0] << 24 | buf[1] << 16 | buf[2] << 8 | buf[3];

    buf += m_sps_pps_context.length_size;
    unit_type = *buf & 0x1f;

    if (buf + nal_size > buf_end || nal_size < 0)
      goto fail;

    // prepend only to the first type 5 NAL unit of an IDR picture
    if (m_sps_pps_context.first_idr && unit_type == 5)
    {
      bitstream_alloc_and_copy(poutbuf, poutbuf_size,
        m_sps_pps_context.sps_pps_data, m_sps_pps_context.size, buf, nal_size);
      m_sps_pps_context.first_idr = 0;
    }
    else
    {
      bitstream_alloc_and_copy(poutbuf, poutbuf_size, NULL, 0, buf, nal_size);
      if (!m_sps_pps_context.first_idr && unit_type == 1)
          m_sps_pps_context.first_idr = 1;
    }

    buf += nal_size;
    cumul_size += nal_size + m_sps_pps_context.length_size;
  } while (cumul_size < buf_size);

  return true;

fail:
  free(*poutbuf);
  *poutbuf = NULL;
  *poutbuf_size = 0;
  return false;
}

void CDVDVideoCodecOpenMax::bitstream_alloc_and_copy(
  uint8_t **poutbuf,      int *poutbuf_size,
  const uint8_t *sps_pps, uint32_t sps_pps_size,
  const uint8_t *in,      uint32_t in_size)
{
  // based on h264_mp4toannexb_bsf.c (ffmpeg)
  // which is Copyright (c) 2007 Benoit Fouet <benoit.fouet@free.fr>
  // and Licensed GPL 2.1 or greater

  #define CHD_WB32(p, d) { \
    ((uint8_t*)(p))[3] = (d); \
    ((uint8_t*)(p))[2] = (d) >> 8; \
    ((uint8_t*)(p))[1] = (d) >> 16; \
    ((uint8_t*)(p))[0] = (d) >> 24; }

  uint32_t offset = *poutbuf_size;
  uint8_t nal_header_size = offset ? 3 : 4;

  *poutbuf_size += sps_pps_size + in_size + nal_header_size;
  *poutbuf = (uint8_t*)realloc(*poutbuf, *poutbuf_size);
  if (sps_pps)
  {
    memcpy(*poutbuf + offset, sps_pps, sps_pps_size);
  }
  memcpy(*poutbuf + sps_pps_size + nal_header_size + offset, in, in_size);
  if (!offset)
  {
    CHD_WB32(*poutbuf + sps_pps_size, 1);
  }
  else
  {
    (*poutbuf + offset + sps_pps_size)[0] = 0;
    (*poutbuf + offset + sps_pps_size)[1] = 0;
    (*poutbuf + offset + sps_pps_size)[2] = 1;
  }
}

////////////////////////////////////////////////////////////////////////////////////////////
// DecoderEventHandler -- OMX event callback
OMX_ERRORTYPE CDVDVideoCodecOpenMax::DecoderEventHandler(
  OMX_HANDLETYPE hComponent,
  OMX_PTR pAppData,
  OMX_EVENTTYPE eEvent,
  OMX_U32 nData1,
  OMX_U32 nData2,
  OMX_PTR pEventData)
{
  OMX_ERRORTYPE omx_err;
  CDVDVideoCodecOpenMax *ctx = (CDVDVideoCodecOpenMax*)pAppData;

#if defined(OMX_DEBUG_EVENTHANDLER)
  CLog::Log(LOGDEBUG, "%s - hComponent(0x%p), eEvent(0x%x), nData1(0x%lx), nData2(0x%lx), pEventData(0x%p)\n",
    __FUNCTION__, hComponent, eEvent, nData1, nData2, pEventData);
#endif

  switch (eEvent)
  {
    case OMX_EventCmdComplete:
      switch(nData1)
      {
        case OMX_CommandStateSet:
          ctx->m_omx_state = (int)nData2;
          switch (ctx->m_omx_state)
          {
            case OMX_StateInvalid:
              CLog::Log(LOGDEBUG, "%s - OMX_StateInvalid\n", __FUNCTION__);
              break;
            case OMX_StateLoaded:
              CLog::Log(LOGDEBUG, "%s - OMX_StateLoaded\n", __FUNCTION__);
              break;
            case OMX_StateIdle:
              CLog::Log(LOGDEBUG, "%s - OMX_StateIdle\n", __FUNCTION__);
              break;
            case OMX_StateExecuting:
              CLog::Log(LOGDEBUG, "%s - OMX_StateExecuting\n", __FUNCTION__);
              break;
            case OMX_StatePause:
              CLog::Log(LOGDEBUG, "%s - OMX_StatePause\n", __FUNCTION__);
              break;
            case OMX_StateWaitForResources:
              CLog::Log(LOGDEBUG, "%s - OMX_StateWaitForResources\n", __FUNCTION__);
              break;
          }
          sem_post(ctx->m_omx_state_change);
        break;
        case OMX_CommandFlush:
          CLog::Log(LOGDEBUG, "%s - OMX_CommandFlush\n",__FUNCTION__);
        break;
        case OMX_CommandPortDisable:
          CLog::Log(LOGDEBUG, "%s - OMX_CommandPortDisable, nData1(0x%lx), nData2(0x%lx)\n",
            __FUNCTION__, nData1, nData2);
          if (ctx->m_output_port == (int)nData2)
          {
            // Got OMX_CommandPortDisable event, alloc new buffers for the output port.
            omx_err = ctx->AllocOMXOutputBuffers();
            omx_err = OMX_SendCommand(ctx->m_omx_decoder, OMX_CommandPortEnable, ctx->m_output_port, NULL);
          }
        break;
        case OMX_CommandPortEnable:
          CLog::Log(LOGDEBUG, "%s - OMX_CommandPortEnable, nData1(0x%lx), nData2(0x%lx)\n",
            __FUNCTION__, nData1, nData2);
          if (ctx->m_output_port == (int)nData2)
          {
            // Got OMX_CommandPortEnable event.
            // OMX_CommandPortDisable will have re-alloced new ones so re-prime
            ctx->PrimeFillBuffers();
          }
        break;
        case OMX_CommandMarkBuffer:
          CLog::Log(LOGDEBUG, "%s - OMX_CommandMarkBuffer, nData1(0x%lx), nData2(0x%lx)\n",
            __FUNCTION__, nData1, nData2);
        break;
      }
    break;
    case OMX_EventBufferFlag:
      if (ctx->m_omx_decoder == hComponent && (nData2 & OMX_BUFFERFLAG_EOS)) {
        if(ctx->m_input_port  == (int)nData1)
        {
            CLog::Log(LOGDEBUG, "%s - OMX_EventBufferFlag(input)\n" , __FUNCTION__);
        }
        if(ctx->m_output_port == (int)nData1)
        {
            CLog::Log(LOGDEBUG, "%s - OMX_EventBufferFlag(output)\n", __FUNCTION__);
            ctx->m_videoplayback_done = true;
        }
      }
    break;
    case OMX_EventPortSettingsChanged:
      // not sure nData2 is the input/output ports in this call, docs don't say
      if (ctx->m_output_port == (int)nData2)
      {
        CLog::Log(LOGDEBUG, "%s - OMX_EventPortSettingsChanged(output)\n", __FUNCTION__);
        // free the current OMX output buffers, you must do this before sending
        // OMX_CommandPortDisable to component as it expects output buffers
        // to be freed before it will issue a OMX_CommandPortDisable event.
        omx_err = ctx->FreeOMXOutputBuffers();
        omx_err = OMX_SendCommand(ctx->m_omx_decoder, OMX_CommandPortDisable, ctx->m_output_port, NULL);
      }
    break;
    case OMX_EventMark:
      CLog::Log(LOGDEBUG, "%s - OMX_EventMark\n", __FUNCTION__);
    break;
    case OMX_EventResourcesAcquired:
      CLog::Log(LOGDEBUG, "%s - OMX_EventResourcesAcquired\n", __FUNCTION__);
    break;
    case OMX_EventError:
      switch((OMX_S32)nData1)
      {
        case OMX_ErrorInsufficientResources:
          CLog::Log(LOGDEBUG, "%s - OMX_EventError, insufficient resources, exiting\n", __FUNCTION__);
          // I'm so frack'ed
          //exit(0);
        break;
        case OMX_ErrorStreamCorrupt:
          CLog::Log(LOGDEBUG, "%s - OMX_EventError, Bitstream corrupt\n", __FUNCTION__);
          ctx->m_videoplayback_done = true;
        break;
        case OMX_ErrorFormatNotDetected:
          CLog::Log(LOGDEBUG, "%s - OMX_EventError, cannot parse or determine the format of an input stream\n", __FUNCTION__);
        break;
        case OMX_ErrorPortUnpopulated:
          // silently ignore these. We can get them when setting OMX_CommandPortDisable
          // on the output port and the component flushes the output buffers.
        break;
        default:
          CLog::Log(LOGDEBUG, "%s - OMX_EventError detected, nData1(0x%lx), nData2(0x%lx)\n",
            __FUNCTION__, nData1, nData2);
        break;
      }
      // do this so we don't hang any commanded state changes that have posted an error
      sem_post(ctx->m_omx_state_change);
    break;
    default:
      CLog::Log(LOGDEBUG, "%s - eEvent(0x%x), nData1(0x%lx), nData2(0x%lx)\n",
        __FUNCTION__, eEvent, nData1, nData2);
    break;
  }
  return OMX_ErrorNone;
}

// DecoderEmptyBufferDone -- OMX empty buffer done callback
OMX_ERRORTYPE CDVDVideoCodecOpenMax::DecoderEmptyBufferDone(
  OMX_HANDLETYPE hComponent,
  OMX_PTR pAppData,
  OMX_BUFFERHEADERTYPE* pBuffer)
{
  CDVDVideoCodecOpenMax *ctx = (CDVDVideoCodecOpenMax*)pAppData;

  // queue input buffer to free list, the decoder has consumed it.
  pthread_mutex_lock(&ctx->m_free_input_mutex);
  ctx->m_free_input_buffers.push(pBuffer);
  pthread_mutex_unlock(&ctx->m_free_input_mutex);

  return OMX_ErrorNone;
}

// DecoderFillBufferDone -- Buffer has been filled, mark it as such
OMX_ERRORTYPE CDVDVideoCodecOpenMax::DecoderFillBufferDone(
  OMX_HANDLETYPE hComponent,
  OMX_PTR pAppData,
  OMX_BUFFERHEADERTYPE* pBufferHeader)
{
#if defined(OMX_DEBUG_FILLBUFFERDONE)
      CLog::Log(LOGDEBUG, "%s - buffer_size(%lu), timestamp(%f)\n",
        __FUNCTION__, pBufferHeader->nFilledLen, (double)pBufferHeader->nTimeStamp / 1000.0);
#endif
  CDVDVideoCodecOpenMax *ctx = (CDVDVideoCodecOpenMax*)pAppData;

  // queue output buffer to ready list.
  pthread_mutex_lock(&ctx->m_ready_output_mutex);
  ctx->m_ready_output_buffers.push(pBufferHeader);
  pthread_mutex_unlock(&ctx->m_ready_output_mutex);

  return OMX_ErrorNone;
}

OMX_ERRORTYPE CDVDVideoCodecOpenMax::PrimeFillBuffers(void)
{
  OMX_ERRORTYPE omx_err = OMX_ErrorNone;
  // tell OpenMax to start filling output buffers
  for(size_t i = 0; i < m_output_buffers.size(); i++)
  {
    OMX_BUFFERHEADERTYPE *buffer = m_output_buffers[i];

    buffer->nOutputPortIndex = m_output_port;
    // Need to clear the EOS flag.
    buffer->nFlags &= ~OMX_BUFFERFLAG_EOS;
    buffer->pAppPrivate = this;

    omx_err = OMX_FillThisBuffer(m_omx_decoder, buffer);
    if (omx_err)
    {
      CLog::Log(LOGDEBUG, "%s - OMX_FillThisBuffer, omx_err(0x%x)\n", __FUNCTION__, omx_err);
    }
  }
  
  return omx_err;
}

OMX_ERRORTYPE CDVDVideoCodecOpenMax::AllocOMXInputBuffers(void)
{
  OMX_ERRORTYPE omx_err = OMX_ErrorNone;

  // Obtain the information about the input port.
  OMX_PARAM_PORTDEFINITIONTYPE port_format;
  OMX_INIT_STRUCTURE(port_format);
  port_format.nPortIndex = m_input_port;
  OMX_GetParameter(m_omx_decoder, OMX_IndexParamPortDefinition, &port_format);
  //
  CLog::Log(LOGDEBUG, "%s - iport(%d), nBufferCountMin(%lu), nBufferSize(%lu)\n", 
    __FUNCTION__, m_input_port, port_format.nBufferCountMin, port_format.nBufferSize);
  //
  for (size_t i = 0; i < port_format.nBufferCountMin; ++i)
  {
    OMX_BUFFERHEADERTYPE *buffer = NULL;
    // demux packets are variable sized. we can do this two ways.
#if defined(OMX_INTERNAL_INPUT_BUFFERS)
    // a) use the internal buffers and pray that they are always large enough
    omx_err = OMX_AllocateBuffer(m_omx_decoder, &buffer, m_input_port, NULL, port_format.nBufferSize);
    if (omx_err)
    {
      CLog::Log(LOGDEBUG, "%s - OMX_AllocateBuffer for input with result(0x%x)\n", __FUNCTION__, omx_err);
      return(omx_err);
    }
#else
    // b) use an external buffer that's sized according to actual demux
    // packet size, start at internal's buffer size, will get deleted when
    // we start pulling demuxer packets and using demux packet sized buffers.
    uint8_t* data = new uint8_t[port_format.nBufferSize];
    omx_err = OMX_UseBuffer(m_omx_decoder, &buffer, m_input_port, NULL, port_format.nBufferSize, data);
    if (omx_err)
    {
      CLog::Log(LOGDEBUG, "%s - OMX_UseBuffer for input with result(0x%x)\n", __FUNCTION__, omx_err);
      return(omx_err);
    }
#endif
    m_input_buffers.push_back(buffer);
    // don't have to lock/unlock here, we are not decoding
    m_free_input_buffers.push(buffer);
  }
  m_input_eos = false;
  m_input_buffer_size = port_format.nBufferSize;
  m_input_buffer_count = port_format.nBufferCountMin;

  return(omx_err);
}
OMX_ERRORTYPE CDVDVideoCodecOpenMax::FreeOMXInputBuffers(void)
{
  OMX_ERRORTYPE omx_err = OMX_ErrorNone;

  // free openmax input buffers.
  for (size_t i = 0; i < m_input_buffers.size(); ++i)
  {
#if !defined(OMX_INTERNAL_INPUT_BUFFERS)
    // using external buffers (OMX_UseBuffer), free before calling OMX_FreeBuffer
    uint8_t *pBuffer = m_input_buffers[i]->pBuffer;
    delete [] pBuffer;
#endif
    omx_err = OMX_FreeBuffer(m_omx_decoder, m_input_port, m_input_buffers[i]);
  }
  m_input_buffers.clear();
  // empty available input buffer queue. don't need lock/unlock, not decoding.
  while (!m_free_input_buffers.empty())
    m_free_input_buffers.pop();

  return(omx_err);
}

OMX_ERRORTYPE CDVDVideoCodecOpenMax::AllocOMXOutputBuffers(void)
{
  OMX_ERRORTYPE omx_err = OMX_ErrorNone;
  int output_buffer_size;

  // Obtain the information about the output port.
  OMX_PARAM_PORTDEFINITIONTYPE port_format;
  OMX_INIT_STRUCTURE(port_format);
  port_format.nPortIndex = m_output_port;
  omx_err = OMX_GetParameter(m_omx_decoder, OMX_IndexParamPortDefinition, &port_format);
  //
  CLog::Log(LOGDEBUG, 
    "%s - oport(%d), nFrameWidth(%lu), nFrameHeight(%lu), nStride(%lx), nBufferCountMin(%lu), nBufferSize(%lu)\n",
    __FUNCTION__, m_output_port, 
    port_format.format.video.nFrameWidth, port_format.format.video.nFrameHeight,port_format.format.video.nStride,
    port_format.nBufferCountMin, port_format.nBufferSize);

  output_buffer_size = port_format.format.video.nFrameWidth * port_format.format.video.nFrameHeight;
  if (port_format.format.video.eColorFormat == OMX_COLOR_FormatCbYCrY)
    output_buffer_size *= FACTORFORMAT422;
  else if (port_format.format.video.eColorFormat == OMX_COLOR_FormatYUV420Planar)
    output_buffer_size *= FACTORFORMAT420;

  port_format.nBufferSize = output_buffer_size;
 
#if defined(OMX_INTERNAL_OUTPUT_BUFFERS)
  // we are changing the port params (buffer sizes), so write it back.
  omx_err = OMX_SetParameter(m_omx_decoder, OMX_IndexParamPortDefinition, &port_format);
#endif

  CLog::Log(LOGDEBUG, 
    "%s - oport(%d), nFrameWidth(%lu), nFrameHeight(%lu), nStride(%lx), nBufferCountMin(%lu), nBufferSize(%lu)\n",
    __FUNCTION__, m_output_port, 
    port_format.format.video.nFrameWidth, port_format.format.video.nFrameHeight,port_format.format.video.nStride,
    port_format.nBufferCountMin, port_format.nBufferSize);
  //

  for (size_t i = 0; i < port_format.nBufferCountMin; ++i)
  {
    OMX_BUFFERHEADERTYPE *buffer = NULL;
#if defined(OMX_INTERNAL_OUTPUT_BUFFERS)
    omx_err = OMX_AllocateBuffer(m_omx_decoder, &buffer, m_output_port, NULL, port_format.nBufferSize);
    if (omx_err)
    {
      CLog::Log(LOGDEBUG, "%s - OMX_AllocateBuffer for output failed with result(0x%x)\n", __FUNCTION__, omx_err);
      return(omx_err);
    }
#else
    uint8_t* data = new uint8_t[port_format.nBufferSize];
    omx_err = OMX_UseBuffer(m_omx_decoder, &buffer, m_input_port, NULL, port_format.nBufferSize, data);
    if (omx_err)
    {
      CLog::Log(LOGDEBUG, "%s - OMX_UseBuffer for output failed with result(0x%x)\n", __FUNCTION__, omx_err);
      return(omx_err);
    }
#endif
    m_output_buffers.push_back(buffer);
  }
  m_output_eos = false;
  m_output_buffer_size = port_format.nBufferSize;
  m_output_buffer_count = port_format.nBufferCountMin;
  m_decoded_width = port_format.format.video.nFrameWidth;
  m_decoded_height = port_format.format.video.nFrameHeight;

  return(omx_err);
}

OMX_ERRORTYPE CDVDVideoCodecOpenMax::FreeOMXOutputBuffers(void)
{
  OMX_ERRORTYPE omx_err = OMX_ErrorNone;

  // free openmax output buffers.
  for (size_t i = 0; i < m_output_buffers.size(); ++i)
  {
#if !defined(OMX_INTERNAL_OUTPUT_BUFFERS)
    // using external buffers (OMX_UseBuffer), free before calling OMX_FreeBuffer
    uint8_t *pBuffer = m_output_buffers[i]->pBuffer;
    delete [] pBuffer;
#endif
    omx_err = OMX_FreeBuffer(m_omx_decoder, m_output_port, m_output_buffers[i]);
  }
  m_output_buffers.clear();
  // empty available output buffer queue. don't need lock/unlock, not decoding
  while (!m_ready_output_buffers.empty())
    m_ready_output_buffers.pop();

  return(omx_err);
}

// SetStateForAllComponents
// Blocks until all state changes have completed
OMX_ERRORTYPE CDVDVideoCodecOpenMax::SetStateForAllComponents(OMX_STATETYPE state)
{
  // this routine only works for one component, for multiple component
  // issue OMX_SendCommand::OMX_CommandStateSet to each and do the same
  // number of sem_wait's.
  OMX_ERRORTYPE omx_err;

  omx_err = OMX_SendCommand(m_omx_decoder, OMX_CommandStateSet, state, 0);
  if (omx_err == OMX_ErrorNone)
  {
    sem_wait(m_omx_state_change);  
  }
  else
  {
    CLog::Log(LOGDEBUG, "%s - OMX_SendCommand:OMX_CommandStateSet failed \n", __FUNCTION__);
  }

  return omx_err;
}

// StartPlayback -- Kick off video playback.
OMX_ERRORTYPE CDVDVideoCodecOpenMax::StartDecoder(void)
{
  OMX_ERRORTYPE omx_err;
  // transition components to IDLE state
  omx_err = SetStateForAllComponents(OMX_StateIdle);
  if (omx_err)
  {
    CLog::Log(LOGDEBUG, "%s - setting OMX components to OMX_StateIdle\n", __FUNCTION__);
    goto fail;
  }

  // transition components to executing state
  omx_err = SetStateForAllComponents(OMX_StateExecuting);
  if (omx_err)
  {
    CLog::Log(LOGDEBUG, "%s - setting OMX components to OMX_StateExecuting\n", __FUNCTION__);
    goto fail;
  }

  //prime the output buffers.
  PrimeFillBuffers();

fail:
  return omx_err;
}

// StopPlayback -- Stop video playback
OMX_ERRORTYPE CDVDVideoCodecOpenMax::StopDecoder(void)
{
  OMX_ERRORTYPE omx_err;

  // transition all components from executing to idle
  omx_err = SetStateForAllComponents(OMX_StateIdle);
  if (omx_err)
  {
    CLog::Log(LOGDEBUG, "%s - setting OMX components to OMX_StateIdle\n", __FUNCTION__);
    return omx_err;
  }

  // transition all components from idle to loaded
  omx_err = SetStateForAllComponents(OMX_StateLoaded);
  if (omx_err)
  {
    CLog::Log(LOGDEBUG, "%s - setting OMX components to OMX_StateLoaded\n", __FUNCTION__);
  }
  
  return omx_err;
}

#endif
