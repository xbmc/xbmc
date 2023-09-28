/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "BaseRenderer.h"

#include "ServiceBroker.h"
#include "cores/VideoPlayer/VideoRenderers/RenderFlags.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "settings/DisplaySettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/SettingDefinitions.h"
#include "utils/MathUtils.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"

#include <algorithm>
#include <cstdlib> // std::abs(int) prototype


CBaseRenderer::CBaseRenderer()
{
  for (int i=0; i < 4; i++)
  {
    m_rotatedDestCoords[i].x = 0;
    m_rotatedDestCoords[i].y = 0;
    m_savedRotatedDestCoords[i].x = 0;
    m_savedRotatedDestCoords[i].y = 0;
  }
}

CBaseRenderer::~CBaseRenderer() = default;

float CBaseRenderer::GetAspectRatio() const
{
  float width = (float)m_sourceWidth;
  float height = (float)m_sourceHeight;
  return m_sourceFrameRatio * width / height * m_sourceHeight / m_sourceWidth;
}

void CBaseRenderer::GetVideoRect(CRect& source, CRect& dest, CRect& view) const
{
  source = m_sourceRect;
  dest = m_destRect;
  view = m_viewRect;
}

inline void CBaseRenderer::ReorderDrawPoints()
{
  // 0 - top left, 1 - top right, 2 - bottom right, 3 - bottom left
  float origMat[4][2] = {{m_destRect.x1, m_destRect.y1},
                         {m_destRect.x2, m_destRect.y1},
                         {m_destRect.x2, m_destRect.y2},
                         {m_destRect.x1, m_destRect.y2}};

  int pointOffset = m_renderOrientation / 90;

  // if renderer doesn't support rotation
  // treat orientation as 0 degree so that
  // ffmpeg might handle it.
  if (!Supports(RENDERFEATURE_ROTATION))
  {
    pointOffset = 0;
  }

  for (int destIdx=0, srcIdx=pointOffset; destIdx < 4; destIdx++)
  {
    m_rotatedDestCoords[destIdx].x = origMat[srcIdx][0];
    m_rotatedDestCoords[destIdx].y = origMat[srcIdx][1];

    srcIdx++;
    srcIdx = srcIdx % 4;
  }
}

void CBaseRenderer::saveRotatedCoords()
{
  for (int i = 0; i < 4; i++)
    m_savedRotatedDestCoords[i] = m_rotatedDestCoords[i];
}

void CBaseRenderer::syncDestRectToRotatedPoints()
{
  m_rotatedDestCoords[0].x = m_destRect.x1;
  m_rotatedDestCoords[0].y = m_destRect.y1;
  m_rotatedDestCoords[1].x = m_destRect.x2;
  m_rotatedDestCoords[1].y = m_destRect.y1;
  m_rotatedDestCoords[2].x = m_destRect.x2;
  m_rotatedDestCoords[2].y = m_destRect.y2;
  m_rotatedDestCoords[3].x = m_destRect.x1;
  m_rotatedDestCoords[3].y = m_destRect.y2;
}

void CBaseRenderer::restoreRotatedCoords()
{
  for (int i = 0; i < 4; i++)
    m_rotatedDestCoords[i] = m_savedRotatedDestCoords[i];
}

void CBaseRenderer::CalcDestRect(float offsetX,
                                 float offsetY,
                                 float width,
                                 float height,
                                 float inputFrameRatio,
                                 float zoomAmount,
                                 float verticalShift,
                                 CRect& destRect)
{
  // if view window is empty, set empty destination
  if (height == 0 || width == 0)
  {
    destRect.SetRect(0.0f, 0.0f, 0.0f, 0.0f);
    return;
  }

  // scale up image as much as possible
  // and keep the aspect ratio (introduces with black bars)
  // calculate the correct output frame ratio (using the users pixel ratio setting
  // and the output pixel ratio setting)

  float outputFrameRatio = inputFrameRatio / CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo().fPixelRatio;

  // allow a certain error to maximize size of render area
  float fCorrection = width / height / outputFrameRatio - 1.0f;
  float fAllowed = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_VIDEOPLAYER_ERRORINASPECT) * 0.01f;
  if (fCorrection > fAllowed)
    fCorrection = fAllowed;
  if (fCorrection < -fAllowed)
    fCorrection = - fAllowed;

  outputFrameRatio *= 1.0f + fCorrection;

  bool isRotated = false;
  if (m_renderOrientation == 90 ||
      m_renderOrientation == 270)
    isRotated = true;

  float newWidth;
  float newHeight;

  if (!isRotated)
  {
    // maximize the movie width
    newWidth = width;
    newHeight = newWidth / outputFrameRatio;
    if (newHeight > height)
    {
      newHeight = height;
      newWidth = newHeight * outputFrameRatio;
    }
  }
  else
  {
    // maximize the movie height
    newHeight = std::min(width, height);
    newWidth = newHeight / outputFrameRatio;
    if (newWidth > width)
    {
      newWidth = std::min(width, height);
      newHeight = newWidth * outputFrameRatio;
    }
  }

  // Scale the movie up by set zoom amount
  newWidth *= zoomAmount;
  newHeight *= zoomAmount;

  // if we are less than one pixel off use the complete screen instead
  if (std::abs(newWidth - width) < 1.0f)
    newWidth = width;
  if (std::abs(newHeight - height) < 1.0f)
    newHeight = height;

  // Centre the movie
  float posY = (height - newHeight) / 2;
  float posX = (width - newWidth) / 2;

  // vertical shift range -1 to 1 shifts within the top and bottom black bars
  // if there are no top and bottom black bars, this range does nothing
  float blackBarSize = std::max((height - newHeight) / 2.0f, 0.0f);
  posY += blackBarSize * std::max(std::min(verticalShift, 1.0f), -1.0f);

  // vertical shift ranges -2 to -1 and 1 to 2 will shift the image out of the screen
  // if vertical shift is -2 it will be completely shifted out the top,
  // if it's 2 it will be completely shifted out the bottom
  float shiftRange = std::min(newHeight, newHeight - (newHeight - height) / 2.0f);
  if (verticalShift > 1.0f)
    posY += shiftRange * (verticalShift - 1.0f);
  else if (verticalShift < -1.0f)
    posY += shiftRange * (verticalShift + 1.0f);

  destRect.x1 = static_cast<float>(MathUtils::round_int(static_cast<double>(posX + offsetX)));
  destRect.x2 = destRect.x1 + MathUtils::round_int(static_cast<double>(newWidth));
  destRect.y1 = static_cast<float>(MathUtils::round_int(static_cast<double>(posY + offsetY)));
  destRect.y2 = destRect.y1 + MathUtils::round_int(static_cast<double>(newHeight));
}

void CBaseRenderer::CalcNormalRenderRect(float offsetX,
                                         float offsetY,
                                         float width,
                                         float height,
                                         float inputFrameRatio,
                                         float zoomAmount,
                                         float verticalShift)
{
  CalcDestRect(offsetX, offsetY, width, height, inputFrameRatio, zoomAmount, verticalShift,
               m_destRect);

  // bail out if view window is empty
  if (height == 0 || width == 0)
    return;

  // clip as needed
  if (m_alwaysClip || !(CServiceBroker::GetWinSystem()->GetGfxContext().IsFullScreenVideo() ||
                        CServiceBroker::GetWinSystem()->GetGfxContext().IsCalibrating()))
  {
    CRect original(m_destRect);
    m_destRect.Intersect(CRect(offsetX, offsetY, offsetX + width, offsetY + height));
    if (m_destRect != original)
    {
      float scaleX = m_sourceRect.Width() / original.Width();
      float scaleY = m_sourceRect.Height() / original.Height();
      m_sourceRect.x1 += (m_destRect.x1 - original.x1) * scaleX;
      m_sourceRect.y1 += (m_destRect.y1 - original.y1) * scaleY;
      m_sourceRect.x2 += (m_destRect.x2 - original.x2) * scaleX;
      m_sourceRect.y2 += (m_destRect.y2 - original.y2) * scaleY;
    }
  }

  ReorderDrawPoints();
}

//***************************************************************************************
// CalculateFrameAspectRatio()
//
// Considers the source frame size and output frame size (as suggested by mplayer)
// to determine if the pixels in the source are not square.  It calculates the aspect
// ratio of the output frame.  We consider the cases of VCD, SVCD and DVD separately,
// as these are intended to be viewed on a non-square pixel TV set, so the pixels are
// defined to be the same ratio as the intended display pixels.
// These formats are determined by frame size.
//***************************************************************************************
void CBaseRenderer::CalculateFrameAspectRatio(unsigned int desired_width, unsigned int desired_height)
{
  m_sourceFrameRatio = (float)desired_width / desired_height;

  // Check whether mplayer has decided that the size of the video file should be changed
  // This indicates either a scaling has taken place (which we didn't ask for) or it has
  // found an aspect ratio parameter from the file, and is changing the frame size based
  // on that.
  if (m_sourceWidth == desired_width && m_sourceHeight == desired_height)
    return ;

  // mplayer is scaling in one or both directions.  We must alter our Source Pixel Ratio
  float imageFrameRatio = (float)m_sourceWidth / m_sourceHeight;

  // OK, most sources will be correct now, except those that are intended
  // to be displayed on non-square pixel based output devices (ie PAL or NTSC TVs)
  // This includes VCD, SVCD, and DVD (and possibly others that we are not doing yet)
  // For this, we can base the pixel ratio on the pixel ratios of PAL and NTSC,
  // though we will need to adjust for anamorphic sources (ie those whose
  // output frame ratio is not 4:3) and for SVCDs which have 2/3rds the
  // horizontal resolution of the default NTSC or PAL frame sizes

  // The following are the defined standard ratios for PAL and NTSC pixels
  // NOTE: These aren't technically (in terms of BT601) correct - the commented values are,
  //       but it seems that many DVDs nowadays are mastered incorrectly, so two wrongs
  //       may indeed make a right.  The "wrong" values here ensure the output frame is
  //       4x3 (or 16x9)
  const float PALPixelRatio = 16.0f / 15.0f;      // 128.0f / 117.0f;
  const float NTSCPixelRatio = 8.0f / 9.0f;       // 4320.0f / 4739.0f;

  // Calculate the correction needed for anamorphic sources
  float Non4by3Correction = m_sourceFrameRatio / (4.0f / 3.0f);

  // Finally, check for a VCD, SVCD or DVD frame size as these need special aspect ratios
  if (m_sourceWidth == 352)
  { // VCD?
    if (m_sourceHeight == 240) // NTSC
      m_sourceFrameRatio = imageFrameRatio * NTSCPixelRatio;
    if (m_sourceHeight == 288) // PAL
      m_sourceFrameRatio = imageFrameRatio * PALPixelRatio;
  }
  if (m_sourceWidth == 480)
  { // SVCD?
    if (m_sourceHeight == 480) // NTSC
      m_sourceFrameRatio = imageFrameRatio * 3.0f / 2.0f * NTSCPixelRatio * Non4by3Correction;
    if (m_sourceHeight == 576) // PAL
      m_sourceFrameRatio = imageFrameRatio * 3.0f / 2.0f * PALPixelRatio * Non4by3Correction;
  }
  if (m_sourceWidth == 720)
  { // DVD?
    if (m_sourceHeight == 480) // NTSC
      m_sourceFrameRatio = imageFrameRatio * NTSCPixelRatio * Non4by3Correction;
    if (m_sourceHeight == 576) // PAL
      m_sourceFrameRatio = imageFrameRatio * PALPixelRatio * Non4by3Correction;
  }
}

void CBaseRenderer::ManageRenderArea()
{
  m_viewRect = CServiceBroker::GetWinSystem()->GetGfxContext().GetViewWindow();

  m_sourceRect.x1 = 0.0f;
  m_sourceRect.y1 = 0.0f;
  m_sourceRect.x2 = (float)m_sourceWidth;
  m_sourceRect.y2 = (float)m_sourceHeight;

  unsigned int stereo_mode  = CONF_FLAGS_STEREO_MODE_MASK(m_iFlags);
  int          stereo_view  = CServiceBroker::GetWinSystem()->GetGfxContext().GetStereoView();

  if(CONF_FLAGS_STEREO_CADENCE(m_iFlags) == CONF_FLAGS_STEREO_CADANCE_RIGHT_LEFT)
  {
    if     (stereo_view == RENDER_STEREO_VIEW_LEFT)  stereo_view = RENDER_STEREO_VIEW_RIGHT;
    else if(stereo_view == RENDER_STEREO_VIEW_RIGHT) stereo_view = RENDER_STEREO_VIEW_LEFT;
  }

  switch(stereo_mode)
  {
    case CONF_FLAGS_STEREO_MODE_TAB:
      if (stereo_view == RENDER_STEREO_VIEW_LEFT)
        m_sourceRect.y2 *= 0.5f;
      else if(stereo_view == RENDER_STEREO_VIEW_RIGHT)
        m_sourceRect.y1 += m_sourceRect.y2*0.5f;
      break;

    case CONF_FLAGS_STEREO_MODE_SBS:
      if     (stereo_view == RENDER_STEREO_VIEW_LEFT)
        m_sourceRect.x2 *= 0.5f;
      else if(stereo_view == RENDER_STEREO_VIEW_RIGHT)
        m_sourceRect.x1 += m_sourceRect.x2*0.5f;
      break;

    default:
      break;
  }

  CalcNormalRenderRect(m_viewRect.x1, m_viewRect.y1, m_viewRect.Width(), m_viewRect.Height(),
                       GetAspectRatio() * CDisplaySettings::GetInstance().GetPixelRatio(),
                       CDisplaySettings::GetInstance().GetZoomAmount(),
                       CDisplaySettings::GetInstance().GetVerticalShift());
}

EShaderFormat CBaseRenderer::GetShaderFormat()
{
  EShaderFormat ret = SHADER_NONE;

  if (m_format == AV_PIX_FMT_YUV420P)
    ret = SHADER_YV12;
  else if (m_format == AV_PIX_FMT_YUV420P9)
    ret = SHADER_YV12_9;
  else if (m_format == AV_PIX_FMT_YUV420P10)
    ret = SHADER_YV12_10;
  else if (m_format == AV_PIX_FMT_YUV420P12)
    ret = SHADER_YV12_12;
  else if (m_format == AV_PIX_FMT_YUV420P14)
    ret = SHADER_YV12_14;
  else if (m_format == AV_PIX_FMT_YUV420P16)
    ret = SHADER_YV12_16;
  else if (m_format == AV_PIX_FMT_NV12)
    ret = SHADER_NV12;
  else if (m_format == AV_PIX_FMT_YUYV422)
    ret = SHADER_YUY2;
  else if (m_format == AV_PIX_FMT_UYVY422)
    ret = SHADER_UYVY;
  else
    CLog::Log(LOGERROR, "CBaseRenderer::GetShaderFormat - unsupported format {}", m_format);

  return ret;
}

void CBaseRenderer::SetViewMode(int viewMode)
{
  if (viewMode < ViewModeNormal || viewMode > ViewModeZoom110Width)
    viewMode = ViewModeNormal;

  m_videoSettings.m_ViewMode = viewMode;

  // get our calibrated full screen resolution
  RESOLUTION_INFO info = CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo();
  float screenWidth  = (float)(info.Overscan.right  - info.Overscan.left);
  float screenHeight = (float)(info.Overscan.bottom - info.Overscan.top);

  // and the source frame ratio
  float sourceFrameRatio = GetAspectRatio();

  bool is43 = (sourceFrameRatio < 8.f/(3.f*sqrt(3.f)) &&
              m_videoSettings.m_ViewMode == ViewModeNormal);

  // Splitres scaling factor
  float xscale = (float)info.iScreenWidth  / (float)info.iWidth;
  float yscale = (float)info.iScreenHeight / (float)info.iHeight;

  screenWidth   *= xscale;
  screenHeight  *= yscale;

  CDisplaySettings::GetInstance().SetVerticalShift(0.0f);
  CDisplaySettings::GetInstance().SetNonLinearStretched(false);

  if (m_videoSettings.m_ViewMode == ViewModeZoom ||
       (is43 && CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_VIDEOPLAYER_STRETCH43) == ViewModeZoom))
  { // zoom image so no black bars
    CDisplaySettings::GetInstance().SetPixelRatio(1.0);
    // calculate the desired output ratio
    float outputFrameRatio = sourceFrameRatio * CDisplaySettings::GetInstance().GetPixelRatio() / info.fPixelRatio;
    // now calculate the correct zoom amount.  First zoom to full height.
    float newHeight = screenHeight;
    float newWidth = newHeight * outputFrameRatio;
    CDisplaySettings::GetInstance().SetZoomAmount(newWidth / screenWidth);
    if (newWidth < screenWidth)
    { // zoom to full width
      newWidth = screenWidth;
      newHeight = newWidth / outputFrameRatio;
      CDisplaySettings::GetInstance().SetZoomAmount(newHeight / screenHeight);
    }
  }
  else if (m_videoSettings.m_ViewMode == ViewModeStretch4x3)
  { // stretch image to 4:3 ratio
    CDisplaySettings::GetInstance().SetZoomAmount(1.0);
    // now we need to set CDisplaySettings::GetInstance().GetPixelRatio() so that
    // fOutputFrameRatio = 4:3.
    CDisplaySettings::GetInstance().SetPixelRatio((4.0f / 3.0f) / sourceFrameRatio);
  }
  else if (m_videoSettings.m_ViewMode == ViewModeWideZoom ||
           (is43 && CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_VIDEOPLAYER_STRETCH43) == ViewModeWideZoom))
  { // super zoom
    float stretchAmount = (screenWidth / screenHeight) * info.fPixelRatio / sourceFrameRatio;
    CDisplaySettings::GetInstance().SetPixelRatio(pow(stretchAmount, float(2.0/3.0)));
    CDisplaySettings::GetInstance().SetZoomAmount(
        pow(stretchAmount, float((stretchAmount < 1.0f) ? -1.0 / 3.0 : 1.0 / 3.0)));
    CDisplaySettings::GetInstance().SetNonLinearStretched(true);
  }
  else if (m_videoSettings.m_ViewMode == ViewModeStretch16x9 ||
            m_videoSettings.m_ViewMode == ViewModeStretch16x9Nonlin ||
           (is43 && (CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_VIDEOPLAYER_STRETCH43) == ViewModeStretch16x9 ||
                     CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_VIDEOPLAYER_STRETCH43) == ViewModeStretch16x9Nonlin)))
  { // stretch image to 16:9 ratio
    CDisplaySettings::GetInstance().SetZoomAmount(1.0);
    // stretch to the limits of the 16:9 screen.
    // incorrect behaviour, but it's what the users want, so...
    CDisplaySettings::GetInstance().SetPixelRatio((screenWidth / screenHeight) * info.fPixelRatio / sourceFrameRatio);
    bool nonlin = (is43 && CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_VIDEOPLAYER_STRETCH43) == ViewModeStretch16x9Nonlin) ||
                  m_videoSettings.m_ViewMode == ViewModeStretch16x9Nonlin;
    CDisplaySettings::GetInstance().SetNonLinearStretched(nonlin);
  }
  else  if (m_videoSettings.m_ViewMode == ViewModeOriginal)
  { // zoom image so that the height is the original size
    CDisplaySettings::GetInstance().SetPixelRatio(1.0);
    // get the size of the media file
    // calculate the desired output ratio
    float outputFrameRatio = sourceFrameRatio * CDisplaySettings::GetInstance().GetPixelRatio() / info.fPixelRatio;
    // now calculate the correct zoom amount.  First zoom to full width.
    float newHeight = screenWidth / outputFrameRatio;
    if (newHeight > screenHeight)
    { // zoom to full height
      newHeight = screenHeight;
    }
    // now work out the zoom amount so that no zoom is done
    CDisplaySettings::GetInstance().SetZoomAmount(m_sourceHeight / newHeight);
  }
  else if (m_videoSettings.m_ViewMode == ViewModeCustom)
  {
    CDisplaySettings::GetInstance().SetZoomAmount(m_videoSettings.m_CustomZoomAmount);
    CDisplaySettings::GetInstance().SetPixelRatio(m_videoSettings.m_CustomPixelRatio);
    CDisplaySettings::GetInstance().SetNonLinearStretched(m_videoSettings.m_CustomNonLinStretch);
    CDisplaySettings::GetInstance().SetVerticalShift(m_videoSettings.m_CustomVerticalShift);
  }
  else if (m_videoSettings.m_ViewMode == ViewModeZoom120Width)
  {
    float fitHeightZoom = sourceFrameRatio * screenHeight / (info.fPixelRatio * screenWidth);
    CDisplaySettings::GetInstance().SetPixelRatio(1.0f);
    CDisplaySettings::GetInstance().SetZoomAmount(fitHeightZoom < 1.0f ? 1.0f : (fitHeightZoom > 1.2f ? 1.2f : fitHeightZoom));
  }
  else if (m_videoSettings.m_ViewMode == ViewModeZoom110Width)
  {
    float fitHeightZoom = sourceFrameRatio * screenHeight / (info.fPixelRatio * screenWidth);
    CDisplaySettings::GetInstance().SetPixelRatio(1.0f);
    CDisplaySettings::GetInstance().SetZoomAmount(fitHeightZoom < 1.0f ? 1.0f : (fitHeightZoom > 1.1f ? 1.1f : fitHeightZoom));
  }
  else // if (CMediaSettings::GetInstance().GetCurrentVideoSettings().m_ViewMode == ViewModeNormal)
  {
    CDisplaySettings::GetInstance().SetPixelRatio(1.0);
    CDisplaySettings::GetInstance().SetZoomAmount(1.0);
  }

  //@TODO
  m_videoSettings.m_CustomZoomAmount = CDisplaySettings::GetInstance().GetZoomAmount();
  m_videoSettings.m_CustomPixelRatio = CDisplaySettings::GetInstance().GetPixelRatio();
  m_videoSettings.m_CustomNonLinStretch = CDisplaySettings::GetInstance().IsNonLinearStretched();
  m_videoSettings.m_CustomVerticalShift = CDisplaySettings::GetInstance().GetVerticalShift();
}

void CBaseRenderer::MarkDirty()
{
  CServiceBroker::GetGUI()->GetWindowManager().MarkDirty(m_destRect);
}

void CBaseRenderer::EnableAlwaysClip()
{
  m_alwaysClip = true;
}

void CBaseRenderer::SetVideoSettings(const CVideoSettings &settings)
{
  m_videoSettings = settings;
}

void CBaseRenderer::SettingOptionsRenderMethodsFiller(
    const std::shared_ptr<const CSetting>& setting,
    std::vector<IntegerSettingOption>& list,
    int& current,
    void* data)
{
  list.emplace_back(g_localizeStrings.Get(13416), RENDER_METHOD_AUTO);

#ifdef HAS_DX
  list.push_back(IntegerSettingOption(g_localizeStrings.Get(16319), RENDER_METHOD_DXVA));
  list.push_back(IntegerSettingOption(g_localizeStrings.Get(13431), RENDER_METHOD_D3D_PS));
  list.push_back(IntegerSettingOption(g_localizeStrings.Get(13419), RENDER_METHOD_SOFTWARE));
#endif
}
