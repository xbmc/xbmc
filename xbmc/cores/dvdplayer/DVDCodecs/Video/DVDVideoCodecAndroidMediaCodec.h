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

#include <queue>
#include <vector>
#include <memory>

#include "DVDVideoCodec.h"
#include "DVDStreamInfo.h"
#include "threads/Thread.h"
#include "threads/SingleLock.h"
#include "android/jni/Surface.h"
#include "guilib/Geometry.h"
#include "cores/VideoRenderers/RenderFeatures.h"

class CJNIMediaCodec;
class CJNIMediaFormat;
class CJNIByteBuffer;
class CBitstreamConverter;

typedef struct amc_demux {
  uint8_t  *pData;
  int       iSize;
  double    dts;
  double    pts;
} amc_demux;

class CDVDMediaCodecInfo
{
public:
  CDVDMediaCodecInfo( int index,
                      std::shared_ptr<CJNIMediaCodec> &codec);

  // reference counting
  CDVDMediaCodecInfo* Retain();
  long                Release();

  // meat and potatos
  void                Validate(bool state);
  // MediaCodec related
  void                ReleaseOutputBuffer(bool render);
  int                 GetIndex() const;

private:
  // private because we are reference counted
  virtual            ~CDVDMediaCodecInfo();

  long                m_refs;
  bool                m_isReleased;
  int                 m_index;
  int64_t             m_timestamp;
  CCriticalSection    m_section;
  // shared_ptr bits, shared between
  // CDVDVideoCodecAndroidMediaCodec and LinuxRenderGLES.
  std::shared_ptr<CJNIMediaCodec> m_codec;
};

class CDVDVideoCodecAndroidMediaCodec : public CDVDVideoCodec
{
public:
  CDVDVideoCodecAndroidMediaCodec();
  virtual ~CDVDVideoCodecAndroidMediaCodec();

  // required overrides
  virtual bool    Open(CDVDStreamInfo &hints, CDVDCodecOptions &options);
  virtual void    Dispose();
  virtual int     Decode(uint8_t *pData, int iSize, double dts, double pts);
  virtual void    Reset();
  virtual bool    GetPicture(DVDVideoPicture *pDvdVideoPicture);
  virtual bool    ClearPicture(DVDVideoPicture* pDvdVideoPicture);
  virtual void    SetDropState(bool bDrop);
  virtual int     GetDataSize(void);
  virtual double  GetTimeSize(void);
  virtual const char* GetName(void) { return m_formatname; }
  virtual unsigned GetAllowedReferences();

protected:
  static void RenderFeaturesCallBack(const void *ctx, Features &renderFeatures);
  static void DeinterlaceMethodsCallBack(const void *ctx, Features &deinterlaceMethods);
  static void RenderUpdateCallBack(const void *ctx, const CRect &SrcRect, const CRect &DestRect, DWORD flags, const void* render_ctx);
  static void RenderCaptureCallBack(const void *ctx, const CRect &SrcRect, const void* render_ctx);
  static void RenderLockCallBack(const void *ctx, const void *render_ctx);
  static void RenderReleaseCallBack(const void *ctx, const void *render_ctx);

  void            FlushInternal(void);
  bool            ConfigureMediaCodec(void);
  int             GetOutputPicture(void);
  void            ConfigureOutputFormat(CJNIMediaFormat* mediaformat);

  CDVDStreamInfo  m_hints;
  std::string     m_mime;
  std::string     m_codecname;
  int             m_colorFormat;
  const char     *m_formatname;
  bool            m_opened;
  bool            m_drop;

  CJNISurface    *m_surface;
  CJNISurface     m_videosurface;
  // we need these as shared_ptr because CDVDVideoCodecAndroidMediaCodec
  // will get deleted before CLinuxRendererGLES is shut down and
  // CLinuxRendererGLES refs them via CDVDMediaCodecInfo.
  std::shared_ptr<CJNIMediaCodec> m_codec;

  amc_demux m_demux_pkt;
  std::vector<CJNIByteBuffer> m_input;
  std::vector<CJNIByteBuffer> m_output;
  std::vector<CDVDMediaCodecInfo*> m_inflight;

  CBitstreamConverter *m_bitstream;
  DVDVideoPicture m_videobuffer;

  bool            m_render_sw;
  int             m_src_offset[4];
  int             m_src_stride[4];
};
