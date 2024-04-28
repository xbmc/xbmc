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
#include "settings/DisplaySettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "windowing/GraphicContext.h"

#include <algorithm>
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
  std::unique_lock<CCriticalSection> lock(m_section);

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
  std::unique_lock<CCriticalSection> lock(m_section);

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
  std::unique_lock<CCriticalSection> lock(m_section);
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
  std::unique_lock<CCriticalSection> lock(m_section);

  std::vector<SElement>& list = m_buffers[idx];
  for(std::vector<SElement>::iterator it = list.begin(); it != list.end(); ++it)
  {
    if (it->overlay_dvd)
    {
      std::shared_ptr<COverlay> o = Convert(*(it->overlay_dvd), it->pts);

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

bool CRenderer::HasOverlay(int idx)
{
  bool hasOverlay = false;

  std::unique_lock<CCriticalSection> lock(m_section);

  std::vector<SElement>& list = m_buffers[idx];
  for(std::vector<SElement>::iterator it = list.begin(); it != list.end(); ++it)
  {
    if (it->overlay_dvd)
    {
      hasOverlay = true;
      break;
    }
  }
  return hasOverlay;
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
  std::unique_lock<CCriticalSection> lock(m_section);
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
}

std::shared_ptr<COverlay> CRenderer::ConvertLibass(
    CDVDOverlayLibass& o,
    double pts,
    bool updateStyle,
    const std::shared_ptr<struct SUBTITLES::STYLE::style>& overlayStyle)
{
  SUBTITLES::STYLE::renderOpts rOpts;

  // libass render in a target area which named as frame. the frame size may bigger than video size,
  // and including margins between video to frame edge. libass allow to render subtitles into the margins.
  // this has been used to show subtitles in the top or bottom "black bar" between video to frame border.
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

  rOpts.m_par = resInfo.fPixelRatio;

  // rOpts.position and margins (set to style) can invalidate the text
  // positions to subtitles type that make use of margins to position text on
  // the screen (e.g. ASS/WebVTT) then we allow to set them when position
  // override setting is enabled only
  if (o.IsForcedMargins())
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

    int assPlayResY = o.GetLibassHandler()->GetPlayResY();
    double assVertMargin = static_cast<double>(overlayStyle->marginVertical) *
                           (static_cast<double>(assPlayResY) / 720);
    double vertMarginScaled = assVertMargin / assPlayResY * static_cast<double>(rOpts.frameHeight);

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
  if (o.IsTextAlignEnabled())
  {
    if (m_subtitleHorizontalAlign == SUBTITLES::HorizontalAlign::LEFT)
      rOpts.horizontalAlignment = SUBTITLES::STYLE::HorizontalAlign::LEFT;
    else if (m_subtitleHorizontalAlign == SUBTITLES::HorizontalAlign::RIGHT)
      rOpts.horizontalAlignment = SUBTITLES::STYLE::HorizontalAlign::RIGHT;
    else
      rOpts.horizontalAlignment = SUBTITLES::STYLE::HorizontalAlign::CENTER;
  }

  // changes: Detect changes from previously rendered images, if > 0 they are changed
  int changes = 0;
  ASS_Image* images =
      o.GetLibassHandler()->RenderImage(pts, rOpts, updateStyle, overlayStyle, &changes);

  // If no images not execute the renderer
  if (!images)
    return nullptr;

  if (o.m_textureid)
  {
    if (changes == 0)
    {
      std::map<unsigned int, std::shared_ptr<COverlay>>::iterator it =
          m_textureCache.find(o.m_textureid);
      if (it != m_textureCache.end())
        return it->second;
    }
  }

  std::shared_ptr<COverlay> overlay = COverlay::Create(images, rOpts.frameWidth, rOpts.frameHeight);

  m_textureCache[m_textureid] = overlay;
  o.m_textureid = m_textureid;
  m_textureid++;
  return overlay;
}

std::shared_ptr<COverlay> CRenderer::Convert(CDVDOverlay& o, double pts)
{
  std::shared_ptr<COverlay> r = NULL;

  if (o.IsOverlayType(DVDOVERLAY_TYPE_TEXT) || o.IsOverlayType(DVDOVERLAY_TYPE_SSA))
  {
    CDVDOverlayLibass& ovAss = static_cast<CDVDOverlayLibass&>(o);
    if (!ovAss.GetLibassHandler())
      return nullptr;
    bool updateStyle = !m_overlayStyle || m_isSettingsChanged;
    if (updateStyle)
    {
      m_isSettingsChanged = false;
      LoadSettings();
      CreateSubtitlesStyle();
    }

    r = ConvertLibass(ovAss, pts, updateStyle, m_overlayStyle);

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
    r = COverlay::Create(static_cast<CDVDOverlayImage&>(o), m_rs);
  else if (o.IsOverlayType(DVDOVERLAY_TYPE_SPU))
    r = COverlay::Create(static_cast<CDVDOverlaySpu&>(o));

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
      std::unique_lock<CCriticalSection> lock(m_section);
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
