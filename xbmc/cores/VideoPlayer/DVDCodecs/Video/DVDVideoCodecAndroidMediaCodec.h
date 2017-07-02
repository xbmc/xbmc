#pragma once
/*
 *      Copyright (C) 2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "system.h"

#include <deque>
#include <vector>
#include <memory>
#include <atomic>

#include <androidjni/Surface.h>

#include "DVDVideoCodec.h"
#include "DVDStreamInfo.h"
#include "platform/android/activity/JNIXBMCVideoView.h"
#include "threads/Thread.h"
#include "threads/SingleLock.h"
#include "guilib/Geometry.h"
#include "cores/VideoPlayer/Process/VideoBuffer.h"

#include <media/NdkMediaCodec.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>

class CJNISurface;
class CJNISurfaceTexture;
class CJNIMediaCodec;
class CJNIMediaFormat;
class CDVDMediaCodecOnFrameAvailable;
class CJNIByteBuffer;
class CBitstreamConverter;

struct AMediaCrypto;
struct DemuxCryptoInfo;
struct mpeg2_sequence;


typedef struct amc_demux {
  uint8_t  *pData;
  int       iSize;
  double    dts;
  double    pts;
} amc_demux;

struct CMediaCodec
{
  CMediaCodec(const char *name);
  virtual ~CMediaCodec();

  AMediaCodec *codec() const { return m_codec; };
private:
  AMediaCodec *m_codec;
};

class CMediaCodecVideoBuffer : public CVideoBuffer
{
public:
  CMediaCodecVideoBuffer(int id) : CVideoBuffer(id) {};
  virtual ~CMediaCodecVideoBuffer() {};

  void Set(int internalId, int textureId,
   std::shared_ptr<CJNISurfaceTexture> surfaceTexture,
   std::shared_ptr<CDVDMediaCodecOnFrameAvailable> frameAvailable,
   std::shared_ptr<CJNIXBMCVideoView> videoView);

  // meat and potatoes
  bool                WaitForFrame(int millis);
  // MediaCodec related
  void                ReleaseOutputBuffer(bool render);
  // SurfaceTexture released
  int                 GetBufferId() const;
  int                 GetTextureId() const;
  void                GetTransformMatrix(float *textureMatrix);
  void                UpdateTexImage();
  void                RenderUpdate(const CRect &DestRect);
  bool                HasSurfaceTexture() const { return m_surfacetexture.operator bool(); };

private:
  int                 m_bufferId = -1;
  unsigned int        m_textureId = 0;
  // shared_ptr bits, shared between
  // CDVDVideoCodecAndroidMediaCodec and LinuxRenderGLES.
  std::shared_ptr<CJNISurfaceTexture> m_surfacetexture;
  std::shared_ptr<CDVDMediaCodecOnFrameAvailable> m_frameready;
  std::shared_ptr<CJNIXBMCVideoView> m_videoview;
};

class CMediaCodecVideoBufferPool : public IVideoBufferPool
{
public:
  CMediaCodecVideoBufferPool(std::shared_ptr<CMediaCodec> mediaCodec) : m_codec(mediaCodec) {};

  virtual ~CMediaCodecVideoBufferPool();

  virtual CVideoBuffer* Get() override;
  virtual void Return(int id) override;

  std::shared_ptr<CMediaCodec> GetMediaCodec();
  void ResetMediaCodec();

private:
  CCriticalSection m_criticalSection;;
  std::shared_ptr<CMediaCodec> m_codec;

  std::vector<CMediaCodecVideoBuffer*> m_videoBuffers;
  std::vector<int> m_freeBuffers;
};

class CDVDVideoCodecAndroidMediaCodec : public CDVDVideoCodec, public CJNISurfaceHolderCallback
{
public:
  CDVDVideoCodecAndroidMediaCodec(CProcessInfo &processInfo, bool surface_render = false);
  virtual ~CDVDVideoCodecAndroidMediaCodec();

  // registration
  static CDVDVideoCodec* Create(CProcessInfo &processInfo);
  static bool Register();

  // required overrides
  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options) override;
  virtual bool AddData(const DemuxPacket &packet) override;
  virtual void Reset() override;
  virtual bool Reconfigure(CDVDStreamInfo &hints) override;
  virtual VCReturn GetPicture(VideoPicture* pVideoPicture) override;
  virtual const char* GetName() override { return m_formatname.c_str(); };
  virtual void SetCodecControl(int flags) override;
  virtual unsigned GetAllowedReferences() override;

protected:
  void            Dispose();
  void            FlushInternal(void);
  bool            ConfigureMediaCodec(void);
  int             GetOutputPicture(void);
  void            ConfigureOutputFormat(AMediaFormat* mediaformat);
  void            UpdateFpsDuration();

  // surface handling functions
  static void     CallbackInitSurfaceTexture(void*);
  void            InitSurfaceTexture(void);
  void            ReleaseSurfaceTexture(void);

  CDVDStreamInfo  m_hints;
  std::string     m_mime;
  std::string     m_codecname;
  int             m_colorFormat;
  std::string     m_formatname;
  bool            m_opened;
  int             m_codecControlFlags;
  int             m_state;
  int             m_noPictureLoop;

  std::shared_ptr<CJNIXBMCVideoView> m_jnivideoview;
  CJNISurface*    m_jnisurface;
  CJNISurface     m_jnivideosurface;
  AMediaCrypto   *m_crypto;
  unsigned int    m_textureId;
  std::shared_ptr<CMediaCodec> m_codec;
  ANativeWindow*  m_surface;
  std::shared_ptr<CJNISurfaceTexture> m_surfaceTexture;
  std::shared_ptr<CDVDMediaCodecOnFrameAvailable> m_frameAvailable;

  amc_demux m_demux_pkt;
  std::shared_ptr<CMediaCodecVideoBufferPool> m_videoBufferPool;

  uint32_t m_OutputDuration, m_fpsDuration;
  int64_t m_lastPTS;

  static std::atomic<bool> m_InstanceGuard;

  CBitstreamConverter *m_bitstream;
  VideoPicture m_videobuffer;

  int             m_indexInputBuffer;
  bool            m_render_surface;
  mpeg2_sequence  *m_mpeg2_sequence;
  int             m_src_offset[4];
  int             m_src_stride[4];

  // CJNISurfaceHolderCallback interface
public:
  virtual void surfaceChanged(CJNISurfaceHolder holder, int format, int width, int height) override;
  virtual void surfaceCreated(CJNISurfaceHolder holder) override;
  virtual void surfaceDestroyed(CJNISurfaceHolder holder) override;
};
