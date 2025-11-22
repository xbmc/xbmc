/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#pragma once

#include "RendererHQ.h"
#include "VideoRenderers/HwDecRender/DXVAHD.h"

#include <map>

#include <d3d11_4.h>

#pragma comment(lib, "libplacebo-354.lib")

extern "C"
{
#include <libavutil/pixfmt.h>
}
#include "libplacebo/colorspace.h"
#include "libplacebo/d3d11.h"
#include "libplacebo/log.h"
#include "libplacebo/renderer.h"
#include "libplacebo/utils/frame_queue.h"
#include "libplacebo/utils/upload.h"

#include <libplacebo/options.h>
#define PL_LIBAV_IMPLEMENTATION 0
#include "VideoRenderers/Libplacebo/PLHelper.h"

#include <libplacebo/utils/libav.h>

#define MAX_FRAME_PASSES 256
#define MAX_BLEND_PASSES 8
#define MAX_BLEND_FRAMES 8

enum RenderMethod;

class CRendererPL : public CRendererBase
{
  class CRenderBufferImpl;

public:
  ~CRendererPL();

  CRenderInfo GetRenderInfo() override;
  bool Supports(ESCALINGMETHOD method) const override;
  bool Supports(ERENDERFEATURE feature) const override;
  bool WantsDoublePass() override { return true; }
  bool Configure(const VideoPicture& picture, float fps, unsigned orientation) override;

  DEBUG_INFO_VIDEO GetDebugInfo(int idx) override;

  static CRendererBase* Create(CVideoSettings& videoSettings);
  static void GetWeight(std::map<RenderMethod, int>& weights, const VideoPicture& picture);
  static DXGI_FORMAT GetDXGIFormat(AVPixelFormat format, DXGI_FORMAT default_fmt);

protected:
  explicit CRendererPL(CVideoSettings& videoSettings);

  void CheckVideoParameters() override;
  void RenderImpl(CD3DTexture& target,
                  CRect& sourceRect,
                  CPoint (&destPoints)[4],
                  uint32_t flags) override;
  CRenderBuffer* CreateBuffer() override;

private:
  // Color space info
  pl_color_space m_colorSpace;

  pl_chroma_location m_chromaLocation;
  PL::pl_d3d_format m_plOutputFormat;

  pl_render_params m_plRenderParams;

  bool m_bTargetColorspaceHint = true;
  //For debug info
  pl_color_system m_videoMatrix;
  pl_color_transfer m_displayTransfer;
  pl_color_primaries m_displayPrimaries;

  // Tracking for parameter changes
  AVColorSpace m_lastColorSpace = AVCOL_SPC_UNSPECIFIED;

  //maybe remove those 2
  AVColorTransferCharacteristic m_lastColorTransfer = AVCOL_TRC_UNSPECIFIED;
  AVColorPrimaries m_lastPrimaries = AVCOL_PRI_UNSPECIFIED;

  AVPixelFormat m_format;


};

class CRendererPL::CRenderBufferImpl : public CRenderBuffer
{
public:
  explicit CRenderBufferImpl(AVPixelFormat av_pix_format, unsigned width, unsigned height);
  ~CRenderBufferImpl();

  void AppendPicture(const VideoPicture& picture) override;
  bool UploadBuffer() override;
  bool GetLibplaceboFrame(pl_frame& frame);
  bool HasHdrData();

  pl_hdr_metadata GetHdrMetadata() { return m_plColorSpace.hdr; }
  

private:
  pl_color_space m_plColorSpace; //< pl_color_space
  pl_color_repr m_plColorRepr;

  bool m_hasHDR10PlusMetadata = false;
  bool m_hasDoviMetadata = false;
  bool m_hasDoviRpuMetadata = false;

  //sw upload
  bool UploadPlanes();
  //When decoded with d3d11va
  bool UploadWrapPlanes();
  //move those to the video codec if linux start to use libplacebo
  AVDynamicHDRPlus m_hdrMetadata;
  AVDOVIMetadata m_doviMetadata;

  //planes are used to create the frame
  pl_plane m_plplanes[3] = {};
  // they are only kept for plane reference
  // we could put them to null according to libplacebo doc but it crash right away
  pl_tex m_pltex[3] = {};
  /* data info for dxva planes formating and bit encoding fill with plhelper
  *  this include the bit format for the color conversion 
  * */
  PL::pl_d3d_format m_plFormat = {};
};