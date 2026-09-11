/*
 *      Initial code sponsored by: Voddler Inc (voddler.com)
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

// ---------------------------------------------------------------------
// Design notes: async subtitle decode
//
// Complex ASS scripts (heavy \blur, many overlapping drawing/karaoke
// events) can take libass tens of milliseconds to lay out and rasterize
// per frame. Large PGS/DVB/DVD-SPU bitmaps can likewise take a non-trivial
// amount of CPU to decode. Doing this inline on the GUI/render thread once
// per displayed frame (the historical behaviour) means a single slow
// subtitle frame stalls the entire GUI/render loop, not just the
// subtitle.
//
// CRenderer now off-loads both decode paths to background workers
// (OVERLAY::CAsyncSubtitleRenderer, see OverlayRendererAsync.h) and never
// blocks the GUI thread on them. PrepareOverlays() submits a request and
// immediately reads back whatever the worker last finished - which may be
// a frame or more stale under load. This deliberately trades subtitle
// timing accuracy for guaranteed smooth video/GUI playback.
//
// Because "submit, then immediately read" can never observe the result of
// the request just submitted (the worker hasn't even been scheduled yet),
// PrepareOverlays() looks one presentation flip ahead: while preparing the
// slot about to be displayed, it also submits a request for the *next*
// queued slot's pts (supplied by CRenderManager::FrameMove, which knows
// the presentation queue), so that request has a full GUI tick to
// complete before it is actually needed. When no distinct "next" pts is
// known (paused, trick-play, nothing queued), the current slot's own pts
// is requested instead.
//
// Because a presentation flip does not happen every GUI tick (e.g. video
// fps below display refresh rate) and because pausing/seeking can leave a
// lookahead result for a pts that is never actually displayed, the async
// renderer keeps the last two completed results and CRenderer selects
// whichever is closest to the pts actually being displayed this frame,
// within a half-frame-duration tolerance. This one rule handles steady
// playback, pause, and fps-below-refresh-rate content without any
// special-casing between them.
// ---------------------------------------------------------------------

#include "OverlayRenderer.h"

#include "OverlayRendererUtil.h"
#include "ServiceBroker.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "cores/VideoPlayer/DVDCodecs/Overlay/DVDOverlay.h"
#include "cores/VideoPlayer/DVDCodecs/Overlay/DVDOverlayImage.h"
#include "cores/VideoPlayer/DVDCodecs/Overlay/DVDOverlayLibass.h"
#include "cores/VideoPlayer/DVDCodecs/Overlay/DVDOverlaySpu.h"
#include "cores/VideoPlayer/Interface/TimingConstants.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "settings/DisplaySettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"
#include "windowing/WinSystem.h"

#include <algorithm>
#include <mutex>
#include <utility>

using namespace KODI;
using namespace OVERLAY;

namespace
{
// Runs on the ass async worker thread only. 'lastAppliedGeneration' is
// owned by CRenderer but never touched from any other thread - the worker
// decides here, from the job's styleGeneration, whether to re-apply style,
// so a style change is never lost to request coalescing regardless of how
// many intermediate requests were overwritten before this one ran.
std::shared_ptr<const SAssRenderResult> RenderAssJob(const SAssRenderJob& job,
                                                     uint64_t& lastAppliedGeneration)
{
  auto result = std::make_shared<SAssRenderResult>();
  result->pts = job.pts;
  result->frameWidth = job.opts.frameWidth;
  result->frameHeight = job.opts.frameHeight;

  const bool updateStyle = job.styleGeneration != lastAppliedGeneration;
  if (updateStyle)
    lastAppliedGeneration = job.styleGeneration;

  // The expensive call: layout, shaping, blur, glyph rasterization.
  ASS_Image* images = job.handler->RenderImage(job.pts, job.opts, updateStyle, job.subStyle);

  // Copies every pixel out of libass's buffers; after this line, 'images'
  // (owned by libass, valid only until the next ass_render_frame on this
  // renderer) is never touched again.
  result->hasImage = convert_quad(images, result->quads, static_cast<int>(job.opts.frameWidth));
  return result;
}

// Runs on the bitmap async worker thread only.
std::shared_ptr<const SBitmapRenderResult> RenderBitmapJob(const SBitmapRenderJob& job)
{
  auto result = std::make_shared<SBitmapRenderResult>();
  if (!job.overlay)
    return result;

  result->sourceOverlay = job.overlay.get();

  if (job.overlay->IsOverlayType(DVDOVERLAY_TYPE_IMAGE))
  {
    const auto& o = static_cast<const CDVDOverlayImage&>(*job.overlay);
    result->width = o.width;
    result->height = o.height;
    result->minX = 0;
    result->maxX = o.width;
    result->minY = 0;
    result->maxY = o.height;
    result->rgba.resize(static_cast<size_t>(o.width) * o.height);

    if (o.palette.empty())
    {
      // Already RGBA; normalize to a tightly packed buffer here (rather
      // than in the GPU-upload step) so the render thread never needs to
      // know about source stride/linesize.
      const uint8_t* src = o.pixels.data();
      uint32_t* dst = result->rgba.data();
      for (int row = 0; row < o.height; ++row)
      {
        memcpy(dst, src, static_cast<size_t>(o.width) * 4);
        src += o.linesize;
        dst += o.width;
      }
    }
    else
    {
      convert_rgba(o, true /*mergealpha*/, result->rgba);
    }
    result->hasImage = true;
  }
  else if (job.overlay->IsOverlayType(DVDOVERLAY_TYPE_SPU))
  {
    const auto& o = static_cast<const CDVDOverlaySpu&>(*job.overlay);
    result->width = o.width;
    result->height = o.height;
    result->rgba.resize(static_cast<size_t>(o.width) * o.height);
    convert_rgba(o, true /*mergealpha*/, result->minX, result->maxX, result->minY, result->maxY,
                 result->rgba);
    result->hasImage = true;
  }
  return result;
}
} // namespace

COverlay::COverlay()
{
  m_x = 0.0f;
  m_y = 0.0f;
  m_width = 0.0f;
  m_height = 0.0f;
  m_type = TYPE_NONE;
  m_align = ALIGN_SCREEN;
  m_pos = POSITION_RELATIVE;
}

COverlay::~COverlay() = default;

void OVERLAY::MarkDirty()
{
  CServiceBroker::GetGUI()->GetWindowManager().MarkDirty();
}

unsigned int CRenderer::m_textureid = 1;

CRenderer::CRenderer()
{
  CServiceBroker::GetSettingsComponent()->GetSubtitlesSettings()->RegisterObserver(this);
}

CRenderer::~CRenderer()
{
  CServiceBroker::GetSettingsComponent()->GetSubtitlesSettings()->UnregisterObserver(this);
  Flush();
}

void CRenderer::PreInit()
{
  std::unique_lock lock(m_section);
  if (!m_assRenderer)
  {
    m_assRenderer =
        std::make_unique<CAsyncSubtitleRenderer<SAssRenderJob, SAssRenderResult>>(
            "AssSubRenderer", [this](const SAssRenderJob& job)
            { return RenderAssJob(job, m_assLastAppliedStyleGeneration); });
  }
  if (!m_bitmapRenderer)
  {
    m_bitmapRenderer =
        std::make_unique<CAsyncSubtitleRenderer<SBitmapRenderJob, SBitmapRenderResult>>(
            "BitmapSubRenderer", [](const SBitmapRenderJob& job) { return RenderBitmapJob(job); });
  }
}

void CRenderer::AddOverlay(std::shared_ptr<CDVDOverlay> o, double pts, int index)
{
  std::unique_lock lock(m_section);

  SElement   e;
  e.pts = pts;
  e.overlay_dvd = std::move(o);
  m_buffers[index].push_back(e);
}

void CRenderer::Release(std::vector<SElement>& list)
{
  list.clear();
}

void CRenderer::UnInit()
{
  if (m_saveSubtitlePosition)
  {
    m_saveSubtitlePosition = false;
    CDisplaySettings::GetInstance().UpdateCalibrations();
    CServiceBroker::GetSettingsComponent()->GetSettings()->Save();
  }

  Flush();

  // Stop and join the workers. No new PrepareOverlays calls can arrive
  // once the caller has begun tearing down (UnInit is called from
  // CRenderManager::UnInit, itself gated on the render state), so it is
  // safe to destroy these now.
  std::unique_lock lock(m_section);
  m_assRenderer.reset();
  m_bitmapRenderer.reset();
}

void CRenderer::Flush()
{
  std::unique_lock lock(m_section);

  for(std::vector<SElement>& buffer : m_buffers)
    Release(buffer);

  ReleaseCache();
  Reset();

  if (m_assRenderer)
    m_assRenderer->Flush();
  if (m_bitmapRenderer)
    m_bitmapRenderer->Flush();
  m_lastConvertedAssResult.reset();
  m_cachedAssOverlay.reset();
  m_lastConvertedBitmapResult.reset();
  m_cachedBitmapOverlaySource = nullptr;
  m_cachedBitmapOverlay.reset();
  m_lastPreparedAssPts = DVD_NOPTS_VALUE;
}

void CRenderer::Reset()
{
  m_subtitlePosition = 0;
  m_subtitlePosResInfo = -1;
}

void CRenderer::Release(int idx)
{
  std::unique_lock lock(m_section);
  Release(m_buffers[idx]);
}

void CRenderer::ReleaseCache()
{
  m_textureCache.clear();
  m_textureid++;
}

void CRenderer::ReleaseUnused()
{
  for (auto it = m_textureCache.begin(); it != m_textureCache.end(); )
  {
    bool found = false;
    for (auto& buffer : m_buffers)
    {
      for (auto& dvdoverlay : buffer)
      {
        if (dvdoverlay.overlay_dvd && dvdoverlay.overlay_dvd->m_textureid == it->first)
        {
          found = true;
          break;
        }
      }
      if (found)
        break;
    }
    if (!found)
    {
      it = m_textureCache.erase(it);
    }
    else
      ++it;
  }
}

void CRenderer::Render(int idx, float depth)
{
  std::unique_lock lock(m_section);

  std::vector<SElement>& list = m_buffers[idx];
  for(std::vector<SElement>::iterator it = list.begin(); it != list.end(); ++it)
  {
    if (it->overlay_dvd)
    {
      std::shared_ptr<COverlay> o = Convert(*it);

      if (o)
        Render(o.get());
    }
  }

  ReleaseUnused();
}

void CRenderer::Render(COverlay* o)
{
  SRenderState state;
  state.x = o->m_x;
  state.y = o->m_y;
  state.width = o->m_width;
  state.height = o->m_height;

  COverlay::EPosition pos = o->m_pos;
  COverlay::EAlign align = o->m_align;

  if (pos == COverlay::POSITION_RELATIVE)
  {
    float scale_x = 1.0;
    float scale_y = 1.0;
    float scale_w = 1.0;
    float scale_h = 1.0;

    if (align == COverlay::ALIGN_SCREEN || align == COverlay::ALIGN_SUBTITLE)
    {
      scale_x = m_rv.Width();
      scale_y = m_rv.Height();
      scale_w = scale_x;
      scale_h = scale_y;
    }
    else if (align == COverlay::ALIGN_SCREEN_AR)
    {
      // Align to screen by keeping aspect ratio to fit into the screen area
      float source_width = o->m_source_width > 0 ? o->m_source_width : m_rs.Width();
      float source_height = o->m_source_height > 0 ? o->m_source_height : m_rs.Height();
      float ratio = std::min<float>(m_rv.Width() / source_width, m_rv.Height() / source_height);
      scale_x = m_rv.Width();
      scale_y = m_rv.Height();
      scale_w = ratio;
      scale_h = ratio;
    }
    else if (align == COverlay::ALIGN_VIDEO)
    {
      scale_x = m_rs.Width();
      scale_y = m_rs.Height();
      scale_w = scale_x;
      scale_h = scale_y;
    }

    state.x *= scale_x;
    state.y *= scale_y;
    state.width *= scale_w;
    state.height *= scale_h;

    pos = COverlay::POSITION_ABSOLUTE;
  }

  if (pos == COverlay::POSITION_ABSOLUTE)
  {
    if (align == COverlay::ALIGN_SCREEN || align == COverlay::ALIGN_SCREEN_AR ||
        align == COverlay::ALIGN_SUBTITLE)
    {
      if (align == COverlay::ALIGN_SUBTITLE)
      {
        RESOLUTION_INFO resInfo = CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo();
        state.x += m_rv.x1 + m_rv.Width() * 0.5f;
        state.y += m_rv.y1 + (resInfo.iSubtitles - resInfo.Overscan.top);
      }
      else
      {
        state.x += m_rv.x1;
        state.y += m_rv.y1;
      }
    }
    else if (align == COverlay::ALIGN_VIDEO)
    {
      float scale_x = m_rd.Width() / m_rs.Width();
      float scale_y = m_rd.Height() / m_rs.Height();

      state.x *= scale_x;
      state.y *= scale_y;
      state.width *= scale_x;
      state.height *= scale_y;

      state.x += m_rd.x1;
      state.y += m_rd.y1;
    }
  }

  state.x += GetStereoscopicDepth();

  o->Render(state);
}

double CRenderer::GetOverlayPts(int idx) const
{
  std::unique_lock lock(m_section);
  if (idx < 0 || idx >= NUM_BUFFERS)
    return DVD_NOPTS_VALUE;

  for (const auto& e : m_buffers[idx])
  {
    if (e.overlay_dvd && (e.overlay_dvd->IsOverlayType(DVDOVERLAY_TYPE_TEXT) ||
                           e.overlay_dvd->IsOverlayType(DVDOVERLAY_TYPE_SSA)))
        return e.pts;
  }
  return DVD_NOPTS_VALUE;
}

bool CRenderer::HasVisibleOverlay(int idx) const
{
  std::unique_lock lock(m_section);
  if (idx < 0 || idx >= NUM_BUFFERS)
    return false;

  for (const auto& e : m_buffers[idx])
  {
    if (!e.overlay_dvd)
      continue;

    const CDVDOverlay& o = *e.overlay_dvd;
    if (o.IsOverlayType(DVDOVERLAY_TYPE_IMAGE) || o.IsOverlayType(DVDOVERLAY_TYPE_SPU))
    {
      if (e.bitmapResult && e.bitmapResult->hasImage)
        return true;
      continue;
    }
    if (o.IsOverlayType(DVDOVERLAY_TYPE_TEXT) || o.IsOverlayType(DVDOVERLAY_TYPE_SSA))
    {
      if (e.assResult && e.assResult->hasImage)
        return true;
    }
  }
  return false;
}

void CRenderer::SetVideoRect(CRect &source, CRect &dest, CRect &view)
{
  if (m_rv != view) // Screen resolution is changed
  {
    m_rv = view;
    OnViewChange();
  }
  m_rs = source;
  m_rd = dest;
}

void CRenderer::OnViewChange()
{
  m_isSettingsChanged = true;
}

void CRenderer::SetStereoMode(const std::string &stereomode)
{
  m_stereomode = stereomode;
}

void CRenderer::SetSubtitleVerticalPosition(const int value, bool save)
{
  std::unique_lock lock(m_section);
  m_subtitlePosition = value;

  if (save && m_subtitleAlign == SUBTITLES::Align::MANUAL)
  {
    m_subtitlePosResInfo = POSRESINFO_SAVE_CHANGES;
    // We save the value to XML file settings when playback is stopped
    // to avoid saving to disk too many times
    m_saveSubtitlePosition = true;
  }
}

void CRenderer::ResetSubtitlePosition()
{
  // In the 'pos' var the vertical margin has been substracted because
  // we need to know the actual text baseline position on screen
  int pos{0};
  m_saveSubtitlePosition = false;
  RESOLUTION_INFO resInfo = CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo();

  if (m_subtitleAlign == SUBTITLES::Align::MANUAL)
  {
    // The position must be fixed to match the subtitle calibration bar
    m_subtitleVerticalMargin = static_cast<int>(
        static_cast<float>(resInfo.iHeight) / 100 *
        CServiceBroker::GetSettingsComponent()->GetSubtitlesSettings()->GetVerticalMarginPerc());

    m_subtitlePosResInfo = resInfo.iSubtitles;
    pos = resInfo.iSubtitles - m_subtitleVerticalMargin;
  }
  else
  {
    // The position must be relative to the screen frame
    m_subtitleVerticalMargin = static_cast<int>(
        static_cast<float>(m_rv.Height()) / 100 *
        CServiceBroker::GetSettingsComponent()->GetSubtitlesSettings()->GetVerticalMarginPerc());

    m_subtitlePosResInfo = static_cast<int>(m_rv.Height());
    pos = static_cast<int>(m_rv.Height()) - m_subtitleVerticalMargin + resInfo.Overscan.top;
  }

  // Update player value (and callback to CRenderer::SetSubtitleVerticalPosition)
  auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();
  appPlayer->SetSubtitleVerticalPosition(pos, false);
}

void CRenderer::CreateSubtitlesStyle()
{
  m_overlayStyle = std::make_shared<SUBTITLES::STYLE::style>();
  const auto settings{CServiceBroker::GetSettingsComponent()->GetSubtitlesSettings()};

  m_overlayStyle->fontName = settings->GetFontName();
  m_overlayStyle->fontSize = static_cast<double>(settings->GetFontSize());

  SUBTITLES::FontStyle fontStyle = settings->GetFontStyle();
  if (fontStyle == SUBTITLES::FontStyle::BOLD_ITALIC)
    m_overlayStyle->fontStyle = SUBTITLES::STYLE::FontStyle::BOLD_ITALIC;
  else if (fontStyle == SUBTITLES::FontStyle::BOLD)
    m_overlayStyle->fontStyle = SUBTITLES::STYLE::FontStyle::BOLD;
  else if (fontStyle == SUBTITLES::FontStyle::ITALIC)
    m_overlayStyle->fontStyle = SUBTITLES::STYLE::FontStyle::ITALIC;

  m_overlayStyle->fontColor = settings->GetFontColor();
  m_overlayStyle->fontBorderSize = settings->GetBorderSize();
  m_overlayStyle->fontBorderColor = settings->GetBorderColor();
  m_overlayStyle->fontOpacity = settings->GetFontOpacity();

  SUBTITLES::BackgroundType backgroundType = settings->GetBackgroundType();
  if (backgroundType == SUBTITLES::BackgroundType::NONE)
    m_overlayStyle->borderStyle = SUBTITLES::STYLE::BorderType::OUTLINE_NO_SHADOW;
  else if (backgroundType == SUBTITLES::BackgroundType::SHADOW)
    m_overlayStyle->borderStyle = SUBTITLES::STYLE::BorderType::OUTLINE;
  else if (backgroundType == SUBTITLES::BackgroundType::BOX)
    m_overlayStyle->borderStyle = SUBTITLES::STYLE::BorderType::BOX;
  else if (backgroundType == SUBTITLES::BackgroundType::SQUAREBOX)
    m_overlayStyle->borderStyle = SUBTITLES::STYLE::BorderType::SQUARE_BOX;

  m_overlayStyle->backgroundColor = settings->GetBackgroundColor();
  m_overlayStyle->backgroundOpacity = settings->GetBackgroundOpacity();

  m_overlayStyle->shadowColor = settings->GetShadowColor();
  m_overlayStyle->shadowOpacity = settings->GetShadowOpacity();
  m_overlayStyle->shadowSize = settings->GetShadowSize();

  SUBTITLES::Align subAlign = settings->GetAlignment();
  if (subAlign == SUBTITLES::Align::TOP_INSIDE || subAlign == SUBTITLES::Align::TOP_OUTSIDE)
    m_overlayStyle->alignment = SUBTITLES::STYLE::FontAlign::TOP_CENTER;
  else
    m_overlayStyle->alignment = SUBTITLES::STYLE::FontAlign::SUB_CENTER;

  m_overlayStyle->assOverrideFont = settings->IsOverrideFonts();

  SUBTITLES::OverrideStyles overrideStyles = settings->GetOverrideStyles();
  if (overrideStyles == SUBTITLES::OverrideStyles::POSITIONS)
    m_overlayStyle->assOverrideStyles = SUBTITLES::STYLE::OverrideStyles::POSITIONS;
  else if (overrideStyles == SUBTITLES::OverrideStyles::STYLES)
    m_overlayStyle->assOverrideStyles = SUBTITLES::STYLE::OverrideStyles::STYLES;
  else if (overrideStyles == SUBTITLES::OverrideStyles::STYLES_POSITIONS)
    m_overlayStyle->assOverrideStyles = SUBTITLES::STYLE::OverrideStyles::STYLES_POSITIONS;
  else
    m_overlayStyle->assOverrideStyles = SUBTITLES::STYLE::OverrideStyles::DISABLED;

  // Changing vertical margin while in playback causes side effects when you
  // rewind the video, displaying the previous text position (test Libass 15.2)
  // for now vertical margin setting will be disabled during playback
  m_overlayStyle->marginVertical =
      static_cast<int>(SUBTITLES::STYLE::VIEWPORT_HEIGHT / 100 *
                       static_cast<double>(settings->GetVerticalMarginPerc()));

  m_overlayStyle->blur = settings->GetBlurSize();
  m_overlayStyle->lineSpacing = settings->GetLineSpacing();
}

void CRenderer::PrepareOverlays(int idx, double lookaheadPts)
{
  std::unique_lock lock(m_section);
  if (idx < 0 || idx >= NUM_BUFFERS)
    return;
  if (!m_assRenderer || !m_bitmapRenderer)
    return; // PreInit hasn't run yet (e.g. very first call of a session)

  bool doMarkDirty = false;
  bool hasImageSpu = false;
  for (auto& e : m_buffers[idx])
  {
    std::shared_ptr<const SAssRenderResult> prevAssResult = e.assResult;
    std::shared_ptr<const SBitmapRenderResult> prevBitmapResult = e.bitmapResult;
    e.assResult.reset();
    e.bitmapResult.reset();

    if (!e.overlay_dvd)
      continue;

    CDVDOverlay& o = *e.overlay_dvd;

    if (o.IsOverlayType(DVDOVERLAY_TYPE_IMAGE) || o.IsOverlayType(DVDOVERLAY_TYPE_SPU))
    {
      hasImageSpu = true;

      // Static content per event: decode once (submitted the first time
      // we see this exact overlay object) and reuse thereafter, exactly
      // like the pre-existing m_textureid gating did on the GUI thread -
      // the only change is that the decode itself now happens off it.
      SBitmapRenderJob job;
      job.overlay = e.overlay_dvd;
      m_bitmapRenderer->RequestRender(job);

      // GetLatestResult() has no per-job identity of its own - it is just
      // "the most recent thing the worker finished" - so verify it was
      // actually decoded from *this* overlay before accepting it. Without
      // this, a slow decode for a just-superseded overlay object could
      // complete after a new one has taken its place in m_buffers and get
      // shown under the new object's placement, however briefly.
      auto result = m_bitmapRenderer->GetLatestResult();

      if (result && result->sourceOverlay == e.overlay_dvd.get())
        e.bitmapResult = result;

      if (!prevBitmapResult && e.bitmapResult && e.bitmapResult->hasImage)
        doMarkDirty = true;
      continue;
    }

    if (!o.IsOverlayType(DVDOVERLAY_TYPE_TEXT) && !o.IsOverlayType(DVDOVERLAY_TYPE_SSA))
      continue;

    CDVDOverlayLibass& ovAss = static_cast<CDVDOverlayLibass&>(o);
    if (!ovAss.GetLibassHandler())
      continue;

    if (!m_overlayStyle || m_isSettingsChanged)
    {
      m_isSettingsChanged = false;
      LoadSettings();
      CreateSubtitlesStyle();
      ++m_styleGeneration;
    }

    SUBTITLES::STYLE::renderOpts rOpts;

    rOpts.sourceWidth = m_rs.Width();
    rOpts.sourceHeight = m_rs.Height();
    rOpts.videoWidth = m_rd.Width();
    rOpts.videoHeight = m_rd.Height();
    rOpts.frameWidth = m_rv.Width();
    rOpts.frameHeight = m_rv.Height();

    if (m_stereomode == "left_right" || m_stereomode == "right_left")
    {
      if (rOpts.sourceWidth / rOpts.sourceHeight < 1.2f)
        rOpts.sourceWidth = m_rs.Width() * 2;
    }
    else if (m_stereomode == "top_bottom" || m_stereomode == "bottom_top")
    {
      if (rOpts.sourceWidth / rOpts.sourceHeight > 2.5f)
        rOpts.sourceHeight = m_rs.Height() * 2;
    }

    RESOLUTION_INFO resInfo = CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo();
    if (m_subtitlePosResInfo != resInfo.iSubtitles)
    {
      if (m_subtitlePosResInfo == POSRESINFO_SAVE_CHANGES)
      {
        resInfo.iSubtitles = m_subtitlePosition + m_subtitleVerticalMargin;
        CServiceBroker::GetWinSystem()->GetGfxContext().SetResInfo(
            CServiceBroker::GetWinSystem()->GetGfxContext().GetVideoResolution(), resInfo);
        m_subtitlePosResInfo = m_subtitlePosition + m_subtitleVerticalMargin;
      }
      else
        ResetSubtitlePosition();
    }

    rOpts.m_par = resInfo.fPixelRatio;

    if (ovAss.IsForcedMargins())
    {
      rOpts.marginsMode = SUBTITLES::STYLE::MarginsMode::DISABLED;
    }
    else if (m_subtitleAlign == SUBTITLES::Align::MANUAL)
    {
      double posPx = static_cast<double>(m_subtitlePosition - resInfo.Overscan.top);

      int assPlayResY = ovAss.GetLibassHandler()->GetPlayResY();
      double assVertMargin = static_cast<double>(m_overlayStyle->marginVertical) *
                             (static_cast<double>(assPlayResY) / 720);
      double vertMarginScaled =
          assVertMargin / assPlayResY * static_cast<double>(rOpts.frameHeight);

      double pos = posPx / (static_cast<double>(rOpts.frameHeight) - vertMarginScaled);
      rOpts.position = 100 - pos * 100;
    }
    else if (m_subtitleAlign == SUBTITLES::Align::BOTTOM_OUTSIDE)
    {
      double posPx =
          static_cast<double>(m_subtitlePosition + m_subtitleVerticalMargin - resInfo.Overscan.top);
      rOpts.position = 100 - posPx / static_cast<double>(rOpts.frameHeight) * 100;
    }
    else if (m_subtitleAlign == SUBTITLES::Align::BOTTOM_INSIDE ||
             m_subtitleAlign == SUBTITLES::Align::TOP_INSIDE)
    {
      rOpts.marginsMode = SUBTITLES::STYLE::MarginsMode::INSIDE_VIDEO;
    }

    if (ovAss.IsTextAlignEnabled())
    {
      if (m_subtitleHorizontalAlign == SUBTITLES::HorizontalAlign::LEFT)
        rOpts.horizontalAlignment = SUBTITLES::STYLE::HorizontalAlign::LEFT;
      else if (m_subtitleHorizontalAlign == SUBTITLES::HorizontalAlign::RIGHT)
        rOpts.horizontalAlignment = SUBTITLES::STYLE::HorizontalAlign::RIGHT;
      else
        rOpts.horizontalAlignment = SUBTITLES::STYLE::HorizontalAlign::CENTER;
    }

    // Ordinary frame-to-frame pts deltas are a small fraction of a second;
    // a jump past this threshold, in either direction, means a seek (or
    // similar discontinuity) landed since the last call. Any async result
    // history at that point was rendered for pts values on the wrong side
    // of the jump - close-in-value-but-stale content that the ordinary
    // "not in the future" rule would otherwise happily match against the
    // new position - so it must be discarded here, at the point the jump
    // is actually observed, rather than only at the seek's own flush call
    // (DiscardBuffer), which necessarily fires before the presented pts
    // has caught up to the new position and so cannot see this coming.
    constexpr double DISCONTINUITY_THRESHOLD = DVD_TIME_BASE; // 1 second
    if (m_lastPreparedAssPts != DVD_NOPTS_VALUE &&
        std::fabs(e.pts - m_lastPreparedAssPts) > DISCONTINUITY_THRESHOLD)
    {
      m_assRenderer->Flush();
      m_lastConvertedAssResult.reset();
      m_cachedAssOverlay.reset();
    }
    m_lastPreparedAssPts = e.pts;

    // Submit the lookahead request (next presented slot's pts, when
    // known) so it has a full GUI tick to complete before it is actually
    // displayed. Fall back to this slot's own pts when no distinct next
    // pts is available (paused, trick-play, nothing queued yet).
    const double requestPts = (lookaheadPts != DVD_NOPTS_VALUE) ? lookaheadPts : e.pts;

    SAssRenderJob job;
    job.handler = ovAss.GetLibassHandler();
    job.pts = requestPts;
    job.opts = rOpts;
    job.subStyle = m_overlayStyle;
    job.styleGeneration = m_styleGeneration;
    m_assRenderer->RequestRender(job);

    // Consume: pick whichever of the last two completed results has the
    // newest pts not exceeding *this slot's own* pts (not requestPts -
    // that is only where we are asking the worker to look next). Never
    // shows a "future" result early; always accepts an arbitrarily stale
    // "past" one rather than showing nothing while the worker catches up
    // on a run of slow cues. See file-level design notes.
    e.assResult = m_assRenderer->GetBestResult(e.pts);

    if (e.assResult.get() != prevAssResult.get())
    {
      CLog::Log(LOGDEBUG,
                "ASYNC_SUB[ass]: display pts={:.3f} now showing result pts={:.3f} (hasImage={})",
                e.pts / DVD_TIME_BASE, e.assResult ? e.assResult->pts / DVD_TIME_BASE : -1.0,
                e.assResult && e.assResult->hasImage);
    }

    if (e.assResult && (!prevAssResult || prevAssResult.get() != e.assResult.get()) &&
        e.assResult->hasImage)
      doMarkDirty = true;
    else if (prevAssResult && prevAssResult->hasImage && (!e.assResult || !e.assResult->hasImage))
      doMarkDirty = true; // subtitle disappeared
  }

  if (hasImageSpu != m_prevHadImageSpu)
    doMarkDirty = true;
  m_prevHadImageSpu = hasImageSpu;

  if (doMarkDirty)
    MarkDirty();
}

std::shared_ptr<COverlay> CRenderer::ConvertLibass(SElement& e)
{
  if (!e.assResult || !e.assResult->hasImage)
    return nullptr;

  // Pointer-identity cache: the async result is immutable, so if this is
  // the same object we built a COverlay from last time, reuse it - no
  // need to re-upload identical pixels to the GPU. Replaces the previous
  // o.m_textureid/o.m_pendingChange bookkeeping (only ever one libass
  // container active at a time, so a single cache slot suffices).
  if (m_cachedAssOverlay && m_lastConvertedAssResult.get() == e.assResult.get())
    return m_cachedAssOverlay;

  m_cachedAssOverlay =
      COverlay::Create(e.assResult->quads, e.assResult->frameWidth, e.assResult->frameHeight);
  m_lastConvertedAssResult = e.assResult;
  return m_cachedAssOverlay;
}

std::shared_ptr<COverlay> CRenderer::ConvertBitmap(SElement& e)
{
  if (!e.bitmapResult || !e.bitmapResult->hasImage || !e.overlay_dvd)
    return nullptr;

  // Identity check the async mailbox itself cannot provide (it only knows
  // about jobs/results, not "which overlay object"): if the cached
  // COverlay was built from a different source overlay, or the decoded
  // result object has changed, rebuild.
  if (m_cachedBitmapOverlay && m_cachedBitmapOverlaySource == e.overlay_dvd.get() &&
      m_lastConvertedBitmapResult.get() == e.bitmapResult.get())
    return m_cachedBitmapOverlay;

  CDVDOverlay& o = *e.overlay_dvd;
  std::shared_ptr<COverlay> overlay;
  if (o.IsOverlayType(DVDOVERLAY_TYPE_IMAGE))
    overlay = COverlay::Create(static_cast<CDVDOverlayImage&>(o), *e.bitmapResult, m_rs);
  else if (o.IsOverlayType(DVDOVERLAY_TYPE_SPU))
    overlay = COverlay::Create(static_cast<CDVDOverlaySpu&>(o), *e.bitmapResult);

  m_cachedBitmapOverlay = overlay;
  m_cachedBitmapOverlaySource = e.overlay_dvd.get();
  m_lastConvertedBitmapResult = e.bitmapResult;
  return overlay;
}

std::shared_ptr<COverlay> CRenderer::Convert(SElement& e)
{
  if (!e.overlay_dvd)
    return nullptr;

  CDVDOverlay& o = *e.overlay_dvd;

  if (o.IsOverlayType(DVDOVERLAY_TYPE_TEXT) || o.IsOverlayType(DVDOVERLAY_TYPE_SSA))
    return ConvertLibass(e);
  if (o.IsOverlayType(DVDOVERLAY_TYPE_IMAGE) || o.IsOverlayType(DVDOVERLAY_TYPE_SPU))
    return ConvertBitmap(e);
  return nullptr;
}

void CRenderer::Notify(const Observable& obs, const ObservableMessage msg)
{
  switch (msg)
  {
    case ObservableMessageSettingsChanged:
    {
      m_isSettingsChanged = true;
      break;
    }
    case ObservableMessagePositionChanged:
    {
      std::unique_lock lock(m_section);
      m_subtitlePosResInfo = POSRESINFO_UNSET;
      break;
    }
    default:
      break;
  }
}

void CRenderer::LoadSettings()
{
  const auto settings{CServiceBroker::GetSettingsComponent()->GetSubtitlesSettings()};
  m_subtitleHorizontalAlign = settings->GetHorizontalAlignment();
  m_subtitleAlign = settings->GetAlignment();
  ResetSubtitlePosition();
}
