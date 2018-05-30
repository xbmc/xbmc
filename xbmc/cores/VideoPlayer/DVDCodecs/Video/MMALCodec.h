/*
 *      Copyright (C) 2010-2013 Team XBMC
 *      http://kodi.tv
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

#pragma once

#include "cores/VideoPlayer/DVDStreamInfo.h"
#include "DVDVideoCodec.h"
#include "threads/Event.h"
#include "xbmc/cores/VideoSettings.h"

#include <queue>
#include <semaphore.h>
#include <memory>
#include <string>
#include "utils/Geometry.h"
#include "rendering/RenderSystem.h"
#include "cores/VideoPlayer/VideoRenderers/BaseRenderer.h"
#include "cores/VideoPlayer/VideoRenderers/HwDecRender/MMALRenderer.h"
#include "cores/VideoPlayer/DVDResource.h"

namespace MMAL {

class CMMALVideo;
class CMMALPool;

// a mmal video frame
class CMMALVideoBuffer : public CMMALBuffer
{
public:
  CMMALVideoBuffer(int id);
  virtual ~CMMALVideoBuffer();
protected:
};

class CMMALVideo : public CDVDVideoCodec
{
public:
  CMMALVideo(CProcessInfo &processInfo);
  virtual ~CMMALVideo();

  // Required overrides
  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options) override;
  virtual bool AddData(const DemuxPacket &packet) override;
  virtual void Reset(void) override;
  virtual CDVDVideoCodec::VCReturn GetPicture(VideoPicture *pDvdVideoPicture) override;
  virtual unsigned GetAllowedReferences() override { return 4; }
  virtual const char* GetName(void) override { return m_pFormatName ? m_pFormatName:"mmal-xxx"; }
  virtual void SetCodecControl(int flags) override;
  virtual void SetSpeed(int iSpeed) override;

  // MMAL decoder callback routines.
  void dec_output_port_cb(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer);
  void dec_control_port_cb(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer);
  void dec_input_port_cb(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer);
  static CDVDVideoCodec* Create(CProcessInfo &processInfo);
  static void Register();

protected:
  void QueryCodec(void);
  void Dispose(void);

  // Video format
  unsigned int      m_decoded_width;
  unsigned int      m_decoded_height;
  unsigned int      m_decoded_aligned_width;
  unsigned int      m_decoded_aligned_height;
  unsigned int      m_egl_buffer_count;
  bool              m_finished;
  float             m_aspect_ratio;
  const char        *m_pFormatName;

  // mmal output buffers (video frames)
  CCriticalSection m_output_mutex;
  XbmcThreads::ConditionVariable m_output_cond;
  std::queue<CMMALVideoBuffer*> m_output_ready;

  // initialize mmal and get decoder component
  bool Initialize( const std::string &decoder_name);
  void PortSettingsChanged(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer);
  bool SendCodecConfigData();

  CDVDStreamInfo    m_hints;
  float             m_fps;
  unsigned          m_num_decoded;
  // Components
  MMAL_INTERLACETYPE_T m_interlace_mode;
  double            m_demuxerPts;
  double            m_decoderPts;
  int               m_speed;
  int               m_codecControlFlags;
  bool              m_preroll;
  bool              m_got_eos;
  uint32_t          m_packet_num;
  uint32_t          m_packet_num_eos;

  CCriticalSection m_sharedSection;
  MMAL_COMPONENT_T *m_dec;
  MMAL_PORT_T *m_dec_input;
  MMAL_PORT_T *m_dec_output;
  MMAL_POOL_T *m_dec_input_pool;
  std::shared_ptr<CMMALPool> m_pool;

  MMAL_ES_FORMAT_T *m_es_format;

  MMAL_FOURCC_T m_codingType;
  VideoPicture* m_lastDvdVideoPicture;

  bool change_dec_output_format();
};

};
