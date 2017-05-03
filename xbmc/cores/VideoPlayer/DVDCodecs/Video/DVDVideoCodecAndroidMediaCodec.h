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

#include <queue>
#include <vector>
#include <memory>
#include <atomic>

#include <androidjni/Surface.h>

#include "DVDVideoCodec.h"
#include "DVDStreamInfo.h"
#include "threads/Thread.h"
#include "threads/SingleLock.h"
#include "guilib/Geometry.h"

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

class CDVDMediaCodecInfo
{
public:
  CDVDMediaCodecInfo( ssize_t index,
                      unsigned int texture,
                      AMediaCodec* codec,
                      std::shared_ptr<CJNISurfaceTexture> &surfacetexture,
                      std::shared_ptr<CDVDMediaCodecOnFrameAvailable> &frameready);

  // reference counting
  CDVDMediaCodecInfo* Retain();
  long                Release();

  // meat and potatoes
  void                Validate(bool state);
  bool                WaitForFrame(int millis);
  // MediaCodec related
  void                ReleaseOutputBuffer(bool render);
  bool                IsReleased() { return m_isReleased; }
  // SurfaceTexture released
  ssize_t             GetIndex() const;
  int                 GetTextureID() const;
  void                GetTransformMatrix(float *textureMatrix);
  void                UpdateTexImage();
  void                RenderUpdate(const CRect &SrcRect, const CRect &DestRect);

private:
  // private because we are reference counted
  virtual            ~CDVDMediaCodecInfo();

  long                m_refs;
  bool                m_valid;
  bool                m_isReleased;
  ssize_t             m_index;
  unsigned int        m_texture;
  int64_t             m_timestamp;
  CCriticalSection    m_section;
  // shared_ptr bits, shared between
  // CDVDVideoCodecAndroidMediaCodec and LinuxRenderGLES.
  AMediaCodec* m_codec;
  std::shared_ptr<CJNISurfaceTexture> m_surfacetexture;
  std::shared_ptr<CDVDMediaCodecOnFrameAvailable> m_frameready;
};

class CDVDVideoCodecAndroidMediaCodec : public CDVDVideoCodec
{
public:
  CDVDVideoCodecAndroidMediaCodec(CProcessInfo &processInfo, bool surface_render = false);
  virtual ~CDVDVideoCodecAndroidMediaCodec();

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
  void            ReleasePrevFrame();
  void            FlushInternal(void);
  bool            ConfigureMediaCodec(void);
  int             GetOutputPicture(void);
  void            ConfigureOutputFormat(AMediaFormat* mediaformat);

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
  bool            m_checkForPicture;
  bool            m_drop;
  int             m_codecControlFlags;
  int             m_state;
  int             m_noPictureLoop;

  CJNISurface*    m_jnisurface;
  CJNISurface     m_jnivideosurface;
  AMediaCrypto   *m_crypto;
  unsigned int    m_textureId;
  AMediaCodec*    m_codec;
  ANativeWindow*  m_surface;
  std::shared_ptr<CJNISurfaceTexture> m_surfaceTexture;
  std::shared_ptr<CDVDMediaCodecOnFrameAvailable> m_frameAvailable;

  amc_demux m_demux_pkt;
  std::vector<CDVDMediaCodecInfo*> m_inflight;

  static std::atomic<bool> m_InstanceGuard;

  CBitstreamConverter *m_bitstream;
  VideoPicture m_videobuffer;

  int             m_indexInputBuffer;
  bool            m_render_sw;
  bool            m_render_surface;
  mpeg2_sequence  *m_mpeg2_sequence;
  unsigned int    m_lastInflight;
  int             m_src_offset[4];
  int             m_src_stride[4];
};
