#pragma once
/*
 *      Copyright (C) 2010-2013 Team XBMC
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

#if defined(HAS_MMAL)

#include <interface/mmal/mmal.h>
#include <interface/mmal/util/mmal_util.h>
#include <interface/mmal/util/mmal_default_components.h>
#include <interface/mmal/util/mmal_util_params.h>
#include <interface/mmal/util/mmal_connection.h>
#include <interface/mmal/mmal_parameters.h>

#include "cores/dvdplayer/DVDStreamInfo.h"
#include "DVDVideoCodec.h"
#include "threads/Event.h"
#include "xbmc/settings/VideoSettings.h"

#include <queue>
#include <semaphore.h>
#include <memory>
#include "utils/StdString.h"
#include "guilib/Geometry.h"
#include "rendering/RenderSystem.h"
#include "cores/VideoRenderers/BaseRenderer.h"

class CMMALVideo;
typedef std::shared_ptr<CMMALVideo> MMALVideoPtr;

// a mmal video frame
class CMMALVideoBuffer
{
public:
  CMMALVideoBuffer(CMMALVideo *omv);
  virtual ~CMMALVideoBuffer();

  MMAL_BUFFER_HEADER_T *mmal_buffer;
  int width;
  int height;
  float m_aspect_ratio;
  int index;
  double dts;
  uint32_t m_changed_count;
  // reference counting
  CMMALVideoBuffer* Acquire();
  long              Release();
  CMMALVideo *m_omv;
  long m_refs;
private:
};

class CMMALVideo
{
  typedef struct mmal_demux_packet {
    uint8_t *buff;
    int size;
    double dts;
    double pts;
  } mmal_demux_packet;

public:
  CMMALVideo();
  virtual ~CMMALVideo();

  // Required overrides
  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options, MMALVideoPtr myself);
  virtual void Dispose(void);
  virtual int  Decode(uint8_t *pData, int iSize, double dts, double pts);
  virtual void Reset(void);
  virtual bool GetPicture(DVDVideoPicture *pDvdVideoPicture);
  virtual bool ClearPicture(DVDVideoPicture* pDvdVideoPicture);
  virtual unsigned GetAllowedReferences() { return NUM_BUFFERS; }
  virtual void SetDropState(bool bDrop);
  virtual const char* GetName(void) { return (const char*)m_pFormatName; }
  virtual bool GetCodecStats(double &pts, int &droppedPics);

  // MMAL decoder callback routines.
  void ReleaseBuffer(CMMALVideoBuffer *buffer);
  void Recycle(MMAL_BUFFER_HEADER_T *buffer);

  // MMAL decoder callback routines.
  void dec_output_port_cb(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer);
  void dec_control_port_cb(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer);
  uint32_t          m_changed_count;
  uint32_t          m_changed_count_dec;

protected:
  void QueryCodec(void);
  void ReturnBuffer(CMMALVideoBuffer *buffer);
  bool CreateDeinterlace(EINTERLACEMETHOD interlace_method);
  bool DestroyDeinterlace();

  // Video format
  bool              m_drop_state;
  int               m_decoded_width;
  int               m_decoded_height;
  unsigned int      m_egl_buffer_count;
  bool              m_finished;
  float             m_aspect_ratio;
  const char        *m_pFormatName;
  MMALVideoPtr      m_myself;

  std::queue<double> m_dts_queue;
  std::queue<mmal_demux_packet> m_demux_queue;
  unsigned           m_demux_queue_length;

  // mmal output buffers (video frames)
  pthread_mutex_t   m_output_mutex;
  int m_output_busy;
  std::queue<CMMALVideoBuffer*> m_output_ready;
  std::vector<CMMALVideoBuffer*> m_output_buffers;

  // initialize mmal and get decoder component
  bool Initialize( const CStdString &decoder_name);
  void PortSettingsChanged(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer);
  bool SendCodecConfigData();

  CDVDStreamInfo    m_hints;
  // Components
  MMAL_INTERLACETYPE_T m_interlace_mode;
  EINTERLACEMETHOD  m_interlace_method;
  bool              m_startframe;
  unsigned int      m_decode_frame_number;
  double            m_decoderPts;
  unsigned int      m_droppedPics;

  MMAL_COMPONENT_T *m_dec;
  MMAL_PORT_T *m_dec_input;
  MMAL_PORT_T *m_dec_output;
  MMAL_POOL_T *m_dec_input_pool;
  MMAL_POOL_T *m_dec_output_pool;

  MMAL_ES_FORMAT_T *m_es_format;
  MMAL_COMPONENT_T *m_deint;
  MMAL_CONNECTION_T *m_deint_connection;

  MMAL_FOURCC_T m_codingType;
  bool change_dec_output_format();
};

// defined(HAS_MMAL)
#endif
