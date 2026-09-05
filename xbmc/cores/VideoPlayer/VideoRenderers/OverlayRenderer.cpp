/*
 *      Initial code sponsored by: Voddler Inc (voddler.com)
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "OverlayRenderer.h"

#include "OverlayRendererUtil.h"
#include "ServiceBroker.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "cores/VideoPlayer/DVDCodecs/Overlay/DVDOverlay.h"
#include "cores/VideoPlayer/DVDCodecs/Overlay/DVDOverlayImage.h"
#include "cores/VideoPlayer/DVDCodecs/Overlay/DVDOverlayLibass.h"
#include "cores/VideoPlayer/DVDCodecs/Overlay/DVDOverlaySpu.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "settings/DisplaySettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "windowing/GraphicContext.h"
#include "windowing/WinSystem.h"

#include <algorithm>
#include <cstring>
#include <mutex>
#include <utility>

using namespace KODI;
using namespace OVERLAY;

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

namespace
{
/*!
 * \brief Measure the transparent border around a bitmap overlay's visible
 *  pixels, so a padded rectangle is not mistaken for a full screen graphic
 */
OVERLAY::SContentInset MeasureContentInset(const CDVDOverlayImage& o)
{
  OVERLAY::SContentInset inset;

  if (o.width <= 0 || o.height <= 0 || o.pixels.empty())
    return inset;

  int minX = o.width;
  int maxX = -1;
  int minY = o.height;
  int maxY = -1;

  std::vector<bool> opaque;
  opaque.reserve(o.palette.size());
  for (const uint32_t entry : o.palette)
    opaque.push_back(((entry >> PIXEL_ASHIFT) & 0xff) != 0);

  for (int row = 0; row < o.height; ++row)
  {
    const uint8_t* line = o.pixels.data() + static_cast<size_t>(row) * o.linesize;
    int first = -1;
    int last = -1;

    for (int col = 0; col < o.width; ++col)
    {
      bool visible;
      if (opaque.empty())
      {
        uint32_t pixel;
        std::memcpy(&pixel, line + static_cast<size_t>(col) * 4, sizeof(pixel));
        visible = ((pixel >> PIXEL_ASHIFT) & 0xff) != 0;
      }
      else
      {
        const uint8_t index = line[col];
        visible = index < opaque.size() && opaque[index];
      }

      if (!visible)
        continue;

      if (first < 0)
        first = col;
      last = col;
    }

    if (first < 0)
      continue;

    minX = std::min(minX, first);
    maxX = std::max(maxX, last);
    if (minY > row)
      minY = row;
    maxY = row;
  }

  if (maxY < 0)
    return inset;

  const float width = static_cast<float>(o.width);
  const float height = static_cast<float>(o.height);
  inset.left = static_cast<float>(minX) / width;
  inset.right = static_cast<float>(o.width - 1 - maxX) / width;
  inset.top = static_cast<float>(minY) / height;
  inset.bottom = static_cast<float>(o.height - 1 - maxY) / height;
  return inset;
}
} // unnamed namespace

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
}

void CRenderer::Flush()
{
  std::unique_lock lock(m_section);

  for(std::vector<SElement>& buffer : m_buffers)
    Release(buffer);

  ReleaseCache();
  Reset();
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

  // Resolve all geometry first so a later pass can see the whole frame
  std::vector<SRenderItem> items;
  items.reserve(m_buffers[idx].size());

  for (auto& e : m_buffers[idx])
  {
    if (!e.overlay_dvd)
      continue;

    std::shared_ptr<COverlay> o = Convert(e);
    if (!o)
      continue;

    SRenderItem item;
    GetRenderState(o.get(), item.state);
    item.overlay = std::move(o);
    items.emplace_back(std::move(item));
  }

  RepositionBitmapSubtitles(items);

  for (auto& item : items)
    item.overlay->Render(item.state);

  ReleaseUnused();
}

void CRenderer::GetRenderState(COverlay* o, SRenderState& state) const
{
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

  if (o->m_isBitmapSubtitle && m_bitmapZoomPerc != 100)
  {
    const float zoom = static_cast<float>(m_bitmapZoomPerc) / 100.0f;
    const CRect before = GetContentRect(*o, state);
    const bool lowerHalf = before.y1 + before.Height() * 0.5f > m_rv.y1 + m_rv.Height() * 0.5f;

    state.width *= zoom;
    state.height *= zoom;

    // Centre the content and pin the edge it is read against, so resizing
    // does not move the text off its line
    const CRect after = GetContentRect(*o, state);
    state.x += before.x1 + before.Width() * 0.5f - (after.x1 + after.Width() * 0.5f);
    state.y += lowerHalf ? before.y2 - after.y2 : before.y1 - after.y1;
  }

  state.x += GetStereoscopicDepth();
}

CRect CRenderer::GetContentRect(const COverlay& o, const SRenderState& state)
{
  // POSITION_RELATIVE places the quad by its centre, everything else by its top left
  const float x1 = o.m_pos == COverlay::POSITION_RELATIVE ? state.x - state.width * 0.5f : state.x;
  const float y1 = o.m_pos == COverlay::POSITION_RELATIVE ? state.y - state.height * 0.5f : state.y;

  return {x1 + o.m_contentInset.left * state.width, y1 + o.m_contentInset.top * state.height,
          x1 + state.width - o.m_contentInset.right * state.width,
          y1 + state.height - o.m_contentInset.bottom * state.height};
}

void CRenderer::RepositionBitmapSubtitles(std::vector<SRenderItem>& items) const
{
  if (!m_bitmapPosition)
    return;

  std::vector<std::pair<size_t, CRect>> subs;
  for (size_t i = 0; i < items.size(); ++i)
  {
    const COverlay& o = *items[i].overlay;
    if (o.m_isBitmapSubtitle)
      subs.emplace_back(i, GetContentRect(o, items[i].state));
  }

  if (subs.empty())
    return;

  std::sort(subs.begin(), subs.end(),
            [](const auto& a, const auto& b) { return a.second.y1 < b.second.y1; });

  // m_rd reaches outside the screen when the video is zoomed
  CRect picture{m_rd};
  picture.Intersect(m_rv);
  if (picture.Height() <= 0.0f)
    picture = m_rv;

  // Group only objects close enough to be one block of text, so a translated
  // sign is not dragged along with the dialogue
  const float groupGap = picture.Height() * 0.15f;
  // Taller than this is a graphic, not a line of text
  const float maxHeight = picture.Height() / 3.0f;
  const float pictureMiddle = picture.y1 + picture.Height() * 0.5f;

  const RESOLUTION_INFO resInfo = CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo();

  // BOTTOM/TOP_INSIDE measure from the picture, as CDVDSubtitlesLibass does
  // for text subtitles via MarginsMode::INSIDE_VIDEO
  const bool insideVideo = m_subtitleAlign == SUBTITLES::Align::BOTTOM_INSIDE ||
                           m_subtitleAlign == SUBTITLES::Align::TOP_INSIDE;
  const CRect& edges = insideVideo ? picture : m_rv;

  // m_subtitlePosition is the libass baseline in view coordinates with the
  // margin already subtracted, so Align::MANUAL calibration is honoured
  const float bottomTarget =
      insideVideo ? edges.y2 - static_cast<float>(m_subtitleVerticalMargin)
                  : m_rv.y1 + static_cast<float>(m_subtitlePosition - resInfo.Overscan.top);
  const float topTarget = edges.y1 + static_cast<float>(m_subtitleVerticalMargin);

  for (size_t first = 0; first < subs.size();)
  {
    size_t last = first;
    CRect group = subs[first].second;
    while (last + 1 < subs.size() && subs[last + 1].second.y1 <= group.y2 + groupGap)
    {
      ++last;
      group.y1 = std::min(group.y1, subs[last].second.y1);
      group.y2 = std::max(group.y2, subs[last].second.y2);
    }

    const bool lowerHalf = group.y1 + group.Height() * 0.5f > pictureMiddle;
    const bool straddles = group.y1 < pictureMiddle && group.y2 > pictureMiddle;

    if (group.Height() <= maxHeight && !straddles)
    {
      float offset = lowerHalf ? bottomTarget - group.y2 : topTarget - group.y1;
      offset = std::clamp(offset, m_rv.y1 - group.y1, m_rv.y2 - group.y2);

      for (size_t i = first; i <= last; ++i)
        items[subs[i].first].state.y += offset;
    }

    first = last + 1;
  }
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
    // PGS/DVB and DVD SPU: ProcessOverlays inserts these into m_buffers
    // only at PTS values where the bitmap is on screen, so finding one
    // here means it is visible.
    if (o.IsOverlayType(DVDOVERLAY_TYPE_IMAGE) || o.IsOverlayType(DVDOVERLAY_TYPE_SPU))
      return true;

    // libass (TEXT/SSA): the container stays in m_buffers for the whole
    // video (iPTSStopTime=DVD_NOPTS_VALUE). Visibility means
    // ass_render_frame returned images for the current PTS, cached by
    // PrepareOverlays in e.renderedImages.
    if (o.IsOverlayType(DVDOVERLAY_TYPE_TEXT) || o.IsOverlayType(DVDOVERLAY_TYPE_SSA))
    {
      if (e.renderedImages != nullptr)
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
  m_isViewChanged = true;
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

RESOLUTION_INFO CRenderer::SyncSubtitlePosition()
{
  // Set position of subtitles based on video calibration settings
  RESOLUTION_INFO resInfo = CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo();

  // Keep track of subtitle position value change,
  // can be changed by GUI Calibration or by window mode/resolution change or
  // by user manual change (e.g. keyboard shortcut)
  if (m_subtitlePosResInfo != resInfo.iSubtitles)
  {
    if (m_subtitlePosResInfo == POSRESINFO_SAVE_CHANGES)
    {
      // m_subtitlePosition has been changed
      // and has been requested to save the value to resInfo
      resInfo.iSubtitles = m_subtitlePosition + m_subtitleVerticalMargin;
      CServiceBroker::GetWinSystem()->GetGfxContext().SetResInfo(
          CServiceBroker::GetWinSystem()->GetGfxContext().GetVideoResolution(), resInfo);
      m_subtitlePosResInfo = m_subtitlePosition + m_subtitleVerticalMargin;
    }
    else
      ResetSubtitlePosition();
  }

  return resInfo;
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

void CRenderer::PrepareOverlays(int idx)
{
  std::unique_lock lock(m_section);
  if (idx < 0 || idx >= NUM_BUFFERS)
    return;

  bool doMarkDirty = false;
  bool hasImageSpu = false;

  // Load the subtitle settings for any overlay, not only for libass tracks
  bool updateStyle = false;
  RESOLUTION_INFO resInfo;
  if (!m_buffers[idx].empty())
  {
    if (!m_overlayStyle || m_isSettingsChanged)
    {
      m_isSettingsChanged = false;
      LoadSettings();
      CreateSubtitlesStyle();
      updateStyle = true;
    }
    resInfo = SyncSubtitlePosition();
  }

  for (auto& e : m_buffers[idx])
  {
    // Clear last frame's cached output; libass may have invalidated the
    // pointer on its next ass_render_frame call.
    // (assDetectChange is consumed by ConvertLibass, not here.)
    e.renderedImages = nullptr;

    if (!e.overlay_dvd)
      continue;

    CDVDOverlay& o = *e.overlay_dvd;

    // PGS/DVB and DVD SPU: only added to m_buffers at their visible PTS,
    // so finding one means it is on screen now. m_textureid == 0 is the
    // "new arrival" signal (also true every frame for animated PGS where
    // each frame is a fresh CDVDOverlay). Disappearance is caught after
    // the loop by the hasImageSpu vs m_prevHadImageSpu check.
    if (o.IsOverlayType(DVDOVERLAY_TYPE_IMAGE) || o.IsOverlayType(DVDOVERLAY_TYPE_SPU))
    {
      hasImageSpu = true;
      if (o.m_textureid == 0)
        doMarkDirty = true;
      continue;
    }

    if (!o.IsOverlayType(DVDOVERLAY_TYPE_TEXT) && !o.IsOverlayType(DVDOVERLAY_TYPE_SSA))
      continue;

    CDVDOverlayLibass& ovAss = static_cast<CDVDOverlayLibass&>(o);
    if (!ovAss.GetLibassHandler())
      continue;

    // rOpts setup moved from CRenderer::ConvertLibass; duplicated in CDebugRenderer::CRenderer::Render.
    SUBTITLES::STYLE::renderOpts rOpts;

    // Three rects: source (subtitle canvas), video (playing size), frame
    // (render target; may exceed video to include letterbox bars so libass
    // can place subtitles in them).
    rOpts.sourceWidth = m_rs.Width();
    rOpts.sourceHeight = m_rs.Height();
    rOpts.videoWidth = m_rd.Width();
    rOpts.videoHeight = m_rd.Height();
    rOpts.frameWidth = m_rv.Width();
    rOpts.frameHeight = m_rv.Height();

    // Render subtitle of half-sbs and half-ou video in full screen, not in half screen
    if (m_stereomode == "left_right" || m_stereomode == "right_left")
    {
      // only half-sbs video, sbs video don't need to change source size
      if (rOpts.sourceWidth / rOpts.sourceHeight < 1.2f)
        rOpts.sourceWidth = m_rs.Width() * 2;
    }
    else if (m_stereomode == "top_bottom" || m_stereomode == "bottom_top")
    {
      // only half-ou video, ou video don't need to change source size
      if (rOpts.sourceWidth / rOpts.sourceHeight > 2.5f)
        rOpts.sourceHeight = m_rs.Height() * 2;
    }

    rOpts.m_par = resInfo.fPixelRatio;

    // rOpts.position and margins (set to style) can invalidate the text
    // positions to subtitles type that make use of margins to position text on
    // the screen (e.g. ASS/WebVTT) then we allow to set them when position
    // override setting is enabled only
    if (ovAss.IsForcedMargins())
    {
      rOpts.marginsMode = SUBTITLES::STYLE::MarginsMode::DISABLED;
    }
    else if (m_subtitleAlign == SUBTITLES::Align::MANUAL)
    {
      // When vertical margins are used Libass apply a displacement in percentage
      // of the height available to line position, this displacement causes
      // problems with subtitle calibration bar on Video Calibration window,
      // so when you moving the subtitle bar of the GUI the text will no longer
      // match the bar, this calculation compensates for the displacement.
      // Note also that the displacement compensation will cause a different
      // default position of the text, different from the other alignment positions
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
      // To keep consistent the position of text as other alignment positions
      // we avoid apply the displacement compensation
      double posPx =
          static_cast<double>(m_subtitlePosition + m_subtitleVerticalMargin - resInfo.Overscan.top);
      rOpts.position = 100 - posPx / static_cast<double>(rOpts.frameHeight) * 100;
    }
    else if (m_subtitleAlign == SUBTITLES::Align::BOTTOM_INSIDE ||
             m_subtitleAlign == SUBTITLES::Align::TOP_INSIDE)
    {
      rOpts.marginsMode = SUBTITLES::STYLE::MarginsMode::INSIDE_VIDEO;
    }

    // Set the horizontal text alignment (currently used to improve readability on CC subtitles only)
    // This setting influence style->alignment property
    if (ovAss.IsTextAlignEnabled())
    {
      if (m_subtitleHorizontalAlign == SUBTITLES::HorizontalAlign::LEFT)
        rOpts.horizontalAlignment = SUBTITLES::STYLE::HorizontalAlign::LEFT;
      else if (m_subtitleHorizontalAlign == SUBTITLES::HorizontalAlign::RIGHT)
        rOpts.horizontalAlignment = SUBTITLES::STYLE::HorizontalAlign::RIGHT;
      else
        rOpts.horizontalAlignment = SUBTITLES::STYLE::HorizontalAlign::CENTER;
    }

    e.renderedFrameWidth = rOpts.frameWidth;
    e.renderedFrameHeight = rOpts.frameHeight;

    // Pull the libass output for this PTS. Cached on the SElement until
    // ConvertLibass consumes it later in this frame's GUI walk.
    int currentChange = 0;
    e.renderedImages = ovAss.GetLibassHandler()->RenderImage(e.pts, rOpts, updateStyle,
                                                             m_overlayStyle, &currentChange);
    if (currentChange > 0)
    {
      // Persist on the overlay so a skipped GUI render does not drop the change.
      ovAss.m_pendingChange = currentChange;
      doMarkDirty = true;
    }
  }

  // PGS/DVB/SPU disappearance: arrival is caught by m_textureid==0 in
  // the loop above. Without this, a PGS subtitle ending leaves its
  // cached bitmap on the GUI plane until something else dirties.
  if (hasImageSpu != m_prevHadImageSpu)
    doMarkDirty = true;
  m_prevHadImageSpu = hasImageSpu;

  if (doMarkDirty)
    MarkDirty();
}

std::shared_ptr<COverlay> CRenderer::ConvertLibass(SElement& e)
{
  // If no images not execute the renderer
  if (!e.renderedImages)
    return nullptr;

  CDVDOverlayLibass& o = static_cast<CDVDOverlayLibass&>(*e.overlay_dvd);

  if (o.m_textureid)
  {
    if (o.m_pendingChange == 0)
    {
      std::map<unsigned int, std::shared_ptr<COverlay>>::iterator it =
          m_textureCache.find(o.m_textureid);
      if (it != m_textureCache.end())
        return it->second;
    }
  }

  std::shared_ptr<COverlay> overlay =
      COverlay::Create(e.renderedImages, e.renderedFrameWidth, e.renderedFrameHeight);

  m_textureCache[m_textureid] = overlay;
  o.m_textureid = m_textureid;
  m_textureid++;
  o.m_pendingChange = 0; // consume
  return overlay;
}

std::shared_ptr<COverlay> CRenderer::Convert(SElement& e)
{
  if (!e.overlay_dvd)
    return nullptr;

  CDVDOverlay& o = *e.overlay_dvd;
  std::shared_ptr<COverlay> r = NULL;

  if (o.IsOverlayType(DVDOVERLAY_TYPE_TEXT) || o.IsOverlayType(DVDOVERLAY_TYPE_SSA))
  {
    CDVDOverlayLibass& ovAss = static_cast<CDVDOverlayLibass&>(o);
    if (!ovAss.GetLibassHandler())
      return nullptr;

    // Build the COverlay from libass output PrepareOverlays cached on e
    // earlier this frame; avoids re-entering libass during render.
    r = ConvertLibass(e);

    if (!r)
      return nullptr;
  }
  else if (o.m_textureid)
  {
    std::map<unsigned int, std::shared_ptr<COverlay>>::iterator it =
        m_textureCache.find(o.m_textureid);
    if (it != m_textureCache.end())
      r = it->second;
  }

  if (r)
  {
    return r;
  }

  if (o.IsOverlayType(DVDOVERLAY_TYPE_IMAGE))
  {
    CDVDOverlayImage& ovImage = static_cast<CDVDOverlayImage&>(o);
    r = COverlay::Create(ovImage, m_rs);
    if (r && o.IsBitmapSubtitle())
    {
      r->m_isBitmapSubtitle = true;
      r->m_contentInset = MeasureContentInset(ovImage);
    }
  }
  else if (o.IsOverlayType(DVDOVERLAY_TYPE_SPU))
  {
    r = COverlay::Create(static_cast<CDVDOverlaySpu&>(o));
    // COverlayTexture already crops an SPU to its visible pixels
    if (r && o.IsBitmapSubtitle())
      r->m_isBitmapSubtitle = true;
  }

  m_textureCache[m_textureid] = r;
  o.m_textureid = m_textureid;
  m_textureid++;

  return r;
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
  const SUBTITLES::HorizontalAlign horizontalAlign{settings->GetHorizontalAlignment()};
  const SUBTITLES::Align align{settings->GetAlignment()};
  const float verticalMarginPerc{settings->GetVerticalMarginPerc()};

  // Reset only when a baseline input changed, else a hand-set position is lost
  const bool resetPosition{!m_overlayStyle || m_isViewChanged || align != m_subtitleAlign ||
                           verticalMarginPerc != m_subtitleVerticalMarginPerc};
  m_isViewChanged = false;

  m_subtitleHorizontalAlign = horizontalAlign;
  m_subtitleAlign = align;
  m_subtitleVerticalMarginPerc = verticalMarginPerc;
  m_bitmapZoomPerc = settings->GetBitmapZoomPerc();
  m_bitmapPosition = settings->IsBitmapPositionEnabled();

  if (resetPosition)
    ResetSubtitlePosition();
}
