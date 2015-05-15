/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "system.h"

#include <algorithm>
#include "BaseRenderer.h"
#include "settings/DisplaySettings.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "guilib/GraphicContext.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "utils/log.h"
#include "utils/MathUtils.h"
#include "utils/SystemInfo.h"
#include "settings/AdvancedSettings.h"
#include "cores/VideoRenderers/RenderFlags.h"


CBaseRenderer::CBaseRenderer()
{
  m_sourceFrameRatio = 1.0f;
  m_sourceWidth = 720;
  m_sourceHeight = 480;
  m_resolution = RES_DESKTOP;
  m_fps = 0.0f;
  m_renderOrientation = 0;
  m_oldRenderOrientation = 0;
  m_oldDestRect.SetRect(0.0f, 0.0f, 0.0f, 0.0f);
  m_iFlags = 0;

  for(int i=0; i < 4; i++)
  {
    m_rotatedDestCoords[i].x = 0;
    m_rotatedDestCoords[i].y = 0;
    m_savedRotatedDestCoords[i].x = 0;
    m_savedRotatedDestCoords[i].y = 0;    
  }

  m_RenderUpdateCallBackFn = NULL;
  m_RenderUpdateCallBackCtx = NULL;
  m_RenderFeaturesCallBackFn = NULL;
  m_RenderFeaturesCallBackCtx = NULL;
}

CBaseRenderer::~CBaseRenderer()
{
}

void CBaseRenderer::RegisterRenderUpdateCallBack(const void *ctx, RenderUpdateCallBackFn fn)
{
  m_RenderUpdateCallBackFn = fn;
  m_RenderUpdateCallBackCtx = ctx;
}

void CBaseRenderer::RegisterRenderFeaturesCallBack(const void *ctx, RenderFeaturesCallBackFn fn)
{
  m_RenderFeaturesCallBackFn = fn;
  m_RenderFeaturesCallBackCtx = ctx;
}

void CBaseRenderer::ChooseBestResolution(float fps)
{
  if (fps == 0.0) return;

  // Adjust refreshrate to match source fps
#if !defined(TARGET_DARWIN_IOS)
  if (CSettings::Get().GetInt("videoplayer.adjustrefreshrate") != ADJUST_REFRESHRATE_OFF)
  {
    float weight;
    if (!FindResolutionFromOverride(fps, weight, false)) //find a refreshrate from overrides
    {
      if (!FindResolutionFromOverride(fps, weight, true))//if that fails find it from a fallback
        FindResolutionFromFpsMatch(fps, weight);//if that fails use automatic refreshrate selection
    }

    CLog::Log(LOGNOTICE, "Display resolution ADJUST : %s (%d) (weight: %.3f)",
        g_graphicsContext.GetResInfo(m_resolution).strMode.c_str(), m_resolution, weight);
  }
  else
#endif
    CLog::Log(LOGNOTICE, "Display resolution %s : %s (%d)",
        m_resolution == RES_DESKTOP ? "DESKTOP" : "USER", g_graphicsContext.GetResInfo(m_resolution).strMode.c_str(), m_resolution);
}

bool CBaseRenderer::FindResolutionFromOverride(float fps, float& weight, bool fallback)
{
  RESOLUTION_INFO curr = g_graphicsContext.GetResInfo(m_resolution);

  //try to find a refreshrate from the override
  for (int i = 0; i < (int)g_advancedSettings.m_videoAdjustRefreshOverrides.size(); i++)
  {
    RefreshOverride& override = g_advancedSettings.m_videoAdjustRefreshOverrides[i];

    if (override.fallback != fallback)
      continue;

    //if we're checking for overrides, check if the fps matches
    if (!fallback && (fps < override.fpsmin || fps > override.fpsmax))
      continue;

    for (size_t j = (int)RES_DESKTOP; j < CDisplaySettings::Get().ResolutionInfoSize(); j++)
    {
      RESOLUTION_INFO info = g_graphicsContext.GetResInfo((RESOLUTION)j);

      if (info.iScreenWidth  == curr.iScreenWidth
       && info.iScreenHeight == curr.iScreenHeight
       && (info.dwFlags & D3DPRESENTFLAG_MODEMASK) == (curr.dwFlags & D3DPRESENTFLAG_MODEMASK)
       && info.iScreen       == curr.iScreen)
      {
        if (info.fRefreshRate <= override.refreshmax
         && info.fRefreshRate >= override.refreshmin)
        {
          m_resolution = (RESOLUTION)j;

          if (fallback)
          {
            CLog::Log(LOGDEBUG, "Found Resolution %s (%d) from fallback (refreshmin:%.3f refreshmax:%.3f)",
                      info.strMode.c_str(), m_resolution,
                      override.refreshmin, override.refreshmax);
          }
          else
          {
            CLog::Log(LOGDEBUG, "Found Resolution %s (%d) from override of fps %.3f (fpsmin:%.3f fpsmax:%.3f refreshmin:%.3f refreshmax:%.3f)",
                      info.strMode.c_str(), m_resolution, fps,
                      override.fpsmin, override.fpsmax, override.refreshmin, override.refreshmax);
          }

          weight = RefreshWeight(info.fRefreshRate, fps);

          return true; //fps and refresh match with this override, use this resolution
        }
      }
    }
  }

  return false; //no override found
}

void CBaseRenderer::FindResolutionFromFpsMatch(float fps, float& weight)
{
  const float maxWeight = 0.0021f;
  RESOLUTION_INFO curr;

  m_resolution = FindClosestResolution(fps, 1.0, m_resolution, weight);
  curr = g_graphicsContext.GetResInfo(m_resolution);

  if (weight >= maxWeight) //not a very good match, try a 2:3 cadence instead
  {
    CLog::Log(LOGDEBUG, "Resolution %s (%d) not a very good match for fps %.3f (weight: %.3f), trying 2:3 cadence",
        curr.strMode.c_str(), m_resolution, fps, weight);

    m_resolution = FindClosestResolution(fps, 2.5, m_resolution, weight);
    curr = g_graphicsContext.GetResInfo(m_resolution);

    if (weight >= maxWeight) //2:3 cadence not a good match
    {
      CLog::Log(LOGDEBUG, "Resolution %s (%d) not a very good match for fps %.3f with 2:3 cadence (weight: %.3f), choosing 60 hertz",
          curr.strMode.c_str(), m_resolution, fps, weight);

      //get the resolution with the refreshrate closest to 60 hertz
      for (size_t i = (int)RES_DESKTOP; i < CDisplaySettings::Get().ResolutionInfoSize(); i++)
      {
        RESOLUTION_INFO info = g_graphicsContext.GetResInfo((RESOLUTION)i);

        if (MathUtils::round_int(info.fRefreshRate) == 60
         && info.iScreenWidth  == curr.iScreenWidth
         && info.iScreenHeight == curr.iScreenHeight
         && (info.dwFlags & D3DPRESENTFLAG_MODEMASK) == (curr.dwFlags & D3DPRESENTFLAG_MODEMASK)
         && info.iScreen       == curr.iScreen)
        {
          if (fabs(info.fRefreshRate - 60.0) < fabs(curr.fRefreshRate - 60.0)) {
            m_resolution = (RESOLUTION)i;
            curr = info;
          }
        }
      }

      //60 hertz not available, get the highest refreshrate
      if (MathUtils::round_int(curr.fRefreshRate) != 60)
      {
        CLog::Log(LOGDEBUG, "60 hertz refreshrate not available, choosing highest");
        for (size_t i = (int)RES_DESKTOP; i < CDisplaySettings::Get().ResolutionInfoSize(); i++)
        {
          RESOLUTION_INFO info = g_graphicsContext.GetResInfo((RESOLUTION)i);

          if (info.fRefreshRate  >  curr.fRefreshRate
           && info.iScreenWidth  == curr.iScreenWidth
           && info.iScreenHeight == curr.iScreenHeight
           && (info.dwFlags & D3DPRESENTFLAG_MODEMASK) == (curr.dwFlags & D3DPRESENTFLAG_MODEMASK)
           && info.iScreen       == curr.iScreen)
          {
            m_resolution = (RESOLUTION)i;
            curr = info;
          }
        }
      }

      weight = RefreshWeight(curr.fRefreshRate, fps);
    }
  }
}

RESOLUTION CBaseRenderer::FindClosestResolution(float fps, float multiplier, RESOLUTION current, float& weight)
{
  RESOLUTION_INFO curr = g_graphicsContext.GetResInfo(current);

  float fRefreshRate = fps;

  float last_diff = fRefreshRate;

  // Find closest refresh rate
  for (size_t i = (int)RES_DESKTOP; i < CDisplaySettings::Get().ResolutionInfoSize(); i++)
  {
    const RESOLUTION_INFO info = g_graphicsContext.GetResInfo((RESOLUTION)i);

    //discard resolutions that are not the same width and height (and interlaced/3D flags)
    //or have a too low refreshrate
    if (info.iScreenWidth  != curr.iScreenWidth
    ||  info.iScreenHeight != curr.iScreenHeight
    ||  info.iScreen       != curr.iScreen
    ||  (info.dwFlags & D3DPRESENTFLAG_MODEMASK) != (curr.dwFlags & D3DPRESENTFLAG_MODEMASK)
    ||  info.fRefreshRate < (fRefreshRate * multiplier / 1.001) - 0.001)
      continue;

    // For 3D choose the closest refresh rate 
    if(CONF_FLAGS_STEREO_MODE_MASK(m_iFlags))
    {
      float diff = (info.fRefreshRate - fRefreshRate);
      if(diff < 0)
        diff *= -1.0f;

      if(diff < last_diff)
      {
        last_diff = diff;
        current = (RESOLUTION)i;
        curr = info;
      }
    }
    else
    {
      int c_weight = MathUtils::round_int(RefreshWeight(curr.fRefreshRate, fRefreshRate * multiplier) * 1000.0);
      int i_weight = MathUtils::round_int(RefreshWeight(info.fRefreshRate, fRefreshRate * multiplier) * 1000.0);

      // Closer the better, prefer higher refresh rate if the same
      if ((i_weight <  c_weight)
      ||  (i_weight == c_weight && info.fRefreshRate > curr.fRefreshRate))
      {
        current = (RESOLUTION)i;
        curr    = info;
      }
    }
  }

  // For 3D overwrite weight
  if(CONF_FLAGS_STEREO_MODE_MASK(m_iFlags))
    weight = 0;
  else
    weight = RefreshWeight(curr.fRefreshRate, fRefreshRate * multiplier);

  return current;
}

//distance of refresh to the closest multiple of fps (multiple is 1 or higher), as a multiplier of fps
float CBaseRenderer::RefreshWeight(float refresh, float fps)
{
  float div   = refresh / fps;
  int   round = MathUtils::round_int(div);

  if (round < 1)
    return (fps - refresh) / fps;
  else
    return (float)fabs(div / round - 1.0);
}

RESOLUTION CBaseRenderer::GetResolution() const
{
  if (g_graphicsContext.IsFullScreenRoot() && (g_graphicsContext.IsFullScreenVideo() || g_graphicsContext.IsCalibrating()))
    return m_resolution;

  return g_graphicsContext.GetVideoResolution();
}

float CBaseRenderer::GetAspectRatio() const
{
  float width = (float)m_sourceWidth;
  float height = (float)m_sourceHeight;
  return m_sourceFrameRatio * width / height * m_sourceHeight / m_sourceWidth;
}

void CBaseRenderer::GetVideoRect(CRect &source, CRect &dest, CRect &view)
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
  bool changeAspect = false;
  int pointOffset = 0;

  switch (m_renderOrientation)
  {
    case 90:
      pointOffset = 1;
      changeAspect = true;
      break;
    case 180:
      pointOffset = 2;
      break;
    case 270:
      pointOffset = 3;
      changeAspect = true;
      break;
  }

  // if renderer doesn't support rotation
  // treat orientation as 0 degree so that
  // ffmpeg might handle it.
  if (!Supports(RENDERFEATURE_ROTATION))
  {
    pointOffset = 0;
    changeAspect = false;
  }


  int diff = (int) ((m_destRect.Height() - m_destRect.Width()) / 2);

  for (int destIdx=0, srcIdx=pointOffset; destIdx < 4; destIdx++)
  {
    m_rotatedDestCoords[destIdx].x = origMat[srcIdx][0];
    m_rotatedDestCoords[destIdx].y = origMat[srcIdx][1];

    if (changeAspect)
    {
      switch (srcIdx)
      {
        case 0:
          m_rotatedDestCoords[destIdx].x -= diff;
          m_rotatedDestCoords[destIdx].y += diff;
          break;
        case 1:
          m_rotatedDestCoords[destIdx].x += diff;
          m_rotatedDestCoords[destIdx].y += diff;
          break;
        case 2:
          m_rotatedDestCoords[destIdx].x += diff;
          m_rotatedDestCoords[destIdx].y -= diff;
          break;
        case 3:
          m_rotatedDestCoords[destIdx].x -= diff;
          m_rotatedDestCoords[destIdx].y -= diff;
          break;
      }
    }
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

void CBaseRenderer::CalcNormalDisplayRect(float offsetX, float offsetY, float screenWidth, float screenHeight, float inputFrameRatio, float zoomAmount, float verticalShift)
{
  // if view window is empty, set empty destination
  if(screenHeight == 0 || screenWidth == 0)
  {
    m_destRect.SetRect(0.0f, 0.0f, 0.0f, 0.0f);
    return;
  }

  // scale up image as much as possible
  // and keep the aspect ratio (introduces with black bars)
  // calculate the correct output frame ratio (using the users pixel ratio setting
  // and the output pixel ratio setting)

  float outputFrameRatio = inputFrameRatio / g_graphicsContext.GetResInfo(GetResolution()).fPixelRatio;

  // allow a certain error to maximize screen size
  float fCorrection = screenWidth / screenHeight / outputFrameRatio - 1.0f;
  float fAllowed    = CSettings::Get().GetInt("videoplayer.errorinaspect") * 0.01f;
  if(fCorrection >   fAllowed) fCorrection =   fAllowed;
  if(fCorrection < - fAllowed) fCorrection = - fAllowed;

  outputFrameRatio *= 1.0f + fCorrection;

  // maximize the movie width
  float newWidth = screenWidth;
  float newHeight = newWidth / outputFrameRatio;

  if (newHeight > screenHeight)
  {
    newHeight = screenHeight;
    newWidth = newHeight * outputFrameRatio;
  }

  // Scale the movie up by set zoom amount
  newWidth *= zoomAmount;
  newHeight *= zoomAmount;

  // if we are less than one pixel off use the complete screen instead
  if (std::abs(newWidth - screenWidth) < 1.0f)
    newWidth = screenWidth;
  if (std::abs(newHeight - screenHeight) < 1.0f)
    newHeight = screenHeight;

  // Centre the movie
  float posY = (screenHeight - newHeight) / 2;
  float posX = (screenWidth - newWidth) / 2;

  // vertical shift range -1 to 1 shifts within the top and bottom black bars
  // if there are no top and bottom black bars, this range does nothing
  float blackBarSize = std::max((screenHeight - newHeight) / 2.0f, 0.0f);
  posY += blackBarSize * std::max(std::min(verticalShift, 1.0f), -1.0f);

  // vertical shift ranges -2 to -1 and 1 to 2 will shift the image out of the screen
  // if vertical shift is -2 it will be completely shifted out the top,
  // if it's 2 it will be completely shifted out the bottom
  float shiftRange = std::min(newHeight, newHeight - (newHeight - screenHeight) / 2.0f);
  if (verticalShift > 1.0f)
    posY += shiftRange * (verticalShift - 1.0f);
  else if (verticalShift < -1.0f)
    posY += shiftRange * (verticalShift + 1.0f);

  m_destRect.x1 = (float)MathUtils::round_int(posX + offsetX);
  m_destRect.x2 = m_destRect.x1 + MathUtils::round_int(newWidth);
  m_destRect.y1 = (float)MathUtils::round_int(posY + offsetY);
  m_destRect.y2 = m_destRect.y1 + MathUtils::round_int(newHeight);

  // clip as needed
  if (!(g_graphicsContext.IsFullScreenVideo() || g_graphicsContext.IsCalibrating()))
  {
    CRect original(m_destRect);
    m_destRect.Intersect(CRect(offsetX, offsetY, offsetX + screenWidth, offsetY + screenHeight));
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

  if (m_oldDestRect != m_destRect || m_oldRenderOrientation != m_renderOrientation)
  {
    // adapt the drawing rect points if we have to rotate
    // and either destrect or orientation changed
    ReorderDrawPoints();
    m_oldDestRect = m_destRect;
    m_oldRenderOrientation = m_renderOrientation;
  }
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
  if (m_sourceWidth == (unsigned int) desired_width && m_sourceHeight == (unsigned int) desired_height)
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

void CBaseRenderer::ManageDisplay()
{
  m_viewRect = g_graphicsContext.GetViewWindow();

  m_sourceRect.x1 = 0.0f;
  m_sourceRect.y1 = 0.0f;
  m_sourceRect.x2 = (float)m_sourceWidth;
  m_sourceRect.y2 = (float)m_sourceHeight;

  unsigned int stereo_mode  = CONF_FLAGS_STEREO_MODE_MASK(m_iFlags);
  int          stereo_view  = g_graphicsContext.GetStereoView();

  if(CONF_FLAGS_STEREO_CADENCE(m_iFlags) == CONF_FLAGS_STEREO_CADANCE_RIGHT_LEFT)
  {
    if     (stereo_view == RENDER_STEREO_VIEW_LEFT)  stereo_view = RENDER_STEREO_VIEW_RIGHT;
    else if(stereo_view == RENDER_STEREO_VIEW_RIGHT) stereo_view = RENDER_STEREO_VIEW_LEFT;
  }

  switch(stereo_mode)
  {
    case CONF_FLAGS_STEREO_MODE_TAB:
      // Those are flipped in y
      if (m_format == RENDER_FMT_CVBREF || m_format == RENDER_FMT_MEDIACODEC)
      {
        if (stereo_view == RENDER_STEREO_VIEW_LEFT)
          m_sourceRect.y1 += m_sourceRect.y2*0.5f;
        else if(stereo_view == RENDER_STEREO_VIEW_RIGHT)
          m_sourceRect.y2 *= 0.5f;
      }
      else
      {
        if (stereo_view == RENDER_STEREO_VIEW_LEFT)
          m_sourceRect.y2 *= 0.5f;
        else if(stereo_view == RENDER_STEREO_VIEW_RIGHT)
          m_sourceRect.y1 += m_sourceRect.y2*0.5f;
      }
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

  CalcNormalDisplayRect(m_viewRect.x1, m_viewRect.y1, m_viewRect.Width(), m_viewRect.Height(), GetAspectRatio() * CDisplaySettings::Get().GetPixelRatio(), CDisplaySettings::Get().GetZoomAmount(), CDisplaySettings::Get().GetVerticalShift());
}

void CBaseRenderer::SetViewMode(int viewMode)
{
  if (viewMode < ViewModeNormal || viewMode > ViewModeCustom)
    viewMode = ViewModeNormal;

  CMediaSettings::Get().GetCurrentVideoSettings().m_ViewMode = viewMode;

  // get our calibrated full screen resolution
  RESOLUTION res = GetResolution();
  RESOLUTION_INFO info = g_graphicsContext.GetResInfo(m_resolution);
  float screenWidth  = (float)(info.Overscan.right  - info.Overscan.left);
  float screenHeight = (float)(info.Overscan.bottom - info.Overscan.top);

  // and the source frame ratio
  float sourceFrameRatio = GetAspectRatio();

  bool is43 = (sourceFrameRatio < 8.f/(3.f*sqrt(3.f)) &&
              CMediaSettings::Get().GetCurrentVideoSettings().m_ViewMode == ViewModeNormal);

  // Splitres scaling factor
  float xscale = (float)info.iScreenWidth  / (float)info.iWidth;
  float yscale = (float)info.iScreenHeight / (float)info.iHeight;

  screenWidth   *= xscale;
  screenHeight  *= yscale;

  CDisplaySettings::Get().SetVerticalShift(0.0f);
  CDisplaySettings::Get().SetNonLinearStretched(false);

  if ( CMediaSettings::Get().GetCurrentVideoSettings().m_ViewMode == ViewModeZoom ||
       (is43 && CSettings::Get().GetInt("videoplayer.stretch43") == ViewModeZoom))
  { // zoom image so no black bars
    CDisplaySettings::Get().SetPixelRatio(1.0);
    // calculate the desired output ratio
    float outputFrameRatio = sourceFrameRatio * CDisplaySettings::Get().GetPixelRatio() / info.fPixelRatio;
    // now calculate the correct zoom amount.  First zoom to full height.
    float newHeight = screenHeight;
    float newWidth = newHeight * outputFrameRatio;
    CDisplaySettings::Get().SetZoomAmount(newWidth / screenWidth);
    if (newWidth < screenWidth)
    { // zoom to full width
      newWidth = screenWidth;
      newHeight = newWidth / outputFrameRatio;
      CDisplaySettings::Get().SetZoomAmount(newHeight / screenHeight);
    }
  }
  else if (CMediaSettings::Get().GetCurrentVideoSettings().m_ViewMode == ViewModeStretch4x3)
  { // stretch image to 4:3 ratio
    CDisplaySettings::Get().SetZoomAmount(1.0);
    if (res == RES_PAL_4x3 || res == RES_PAL60_4x3 || res == RES_NTSC_4x3 || res == RES_HDTV_480p_4x3)
    { // stretch to the limits of the 4:3 screen.
      // incorrect behaviour, but it's what the users want, so...
      CDisplaySettings::Get().SetPixelRatio((screenWidth / screenHeight) * info.fPixelRatio / sourceFrameRatio);
    }
    else
    {
      // now we need to set CDisplaySettings::Get().GetPixelRatio() so that
      // fOutputFrameRatio = 4:3.
      CDisplaySettings::Get().SetPixelRatio((4.0f / 3.0f) / sourceFrameRatio);
    }
  }
  else if ( CMediaSettings::Get().GetCurrentVideoSettings().m_ViewMode == ViewModeWideZoom ||
           (is43 && CSettings::Get().GetInt("videoplayer.stretch43") == ViewModeWideZoom))
  { // super zoom
    float stretchAmount = (screenWidth / screenHeight) * info.fPixelRatio / sourceFrameRatio;
    CDisplaySettings::Get().SetPixelRatio(pow(stretchAmount, float(2.0/3.0)));
    CDisplaySettings::Get().SetZoomAmount(pow(stretchAmount, float((stretchAmount < 1.0) ? -1.0/3.0 : 1.0/3.0)));
    CDisplaySettings::Get().SetNonLinearStretched(true);
  }
  else if ( CMediaSettings::Get().GetCurrentVideoSettings().m_ViewMode == ViewModeStretch16x9 ||
           (is43 && CSettings::Get().GetInt("videoplayer.stretch43") == ViewModeStretch16x9))
  { // stretch image to 16:9 ratio
    CDisplaySettings::Get().SetZoomAmount(1.0);
    if (res == RES_PAL_4x3 || res == RES_PAL60_4x3 || res == RES_NTSC_4x3 || res == RES_HDTV_480p_4x3)
    { // now we need to set CDisplaySettings::Get().GetPixelRatio() so that
      // outputFrameRatio = 16:9.
      CDisplaySettings::Get().SetPixelRatio((16.0f / 9.0f) / sourceFrameRatio);
    }
    else
    { // stretch to the limits of the 16:9 screen.
      // incorrect behaviour, but it's what the users want, so...
      CDisplaySettings::Get().SetPixelRatio((screenWidth / screenHeight) * info.fPixelRatio / sourceFrameRatio);
    }
  }
  else  if (CMediaSettings::Get().GetCurrentVideoSettings().m_ViewMode == ViewModeOriginal)
  { // zoom image so that the height is the original size
    CDisplaySettings::Get().SetPixelRatio(1.0);
    // get the size of the media file
    // calculate the desired output ratio
    float outputFrameRatio = sourceFrameRatio * CDisplaySettings::Get().GetPixelRatio() / info.fPixelRatio;
    // now calculate the correct zoom amount.  First zoom to full width.
    float newHeight = screenWidth / outputFrameRatio;
    if (newHeight > screenHeight)
    { // zoom to full height
      newHeight = screenHeight;
    }
    // now work out the zoom amount so that no zoom is done
    CDisplaySettings::Get().SetZoomAmount(m_sourceHeight / newHeight);
  }
  else if (CMediaSettings::Get().GetCurrentVideoSettings().m_ViewMode == ViewModeCustom)
  {
    CDisplaySettings::Get().SetZoomAmount(CMediaSettings::Get().GetCurrentVideoSettings().m_CustomZoomAmount);
    CDisplaySettings::Get().SetPixelRatio(CMediaSettings::Get().GetCurrentVideoSettings().m_CustomPixelRatio);
    CDisplaySettings::Get().SetNonLinearStretched(CMediaSettings::Get().GetCurrentVideoSettings().m_CustomNonLinStretch);
    CDisplaySettings::Get().SetVerticalShift(CMediaSettings::Get().GetCurrentVideoSettings().m_CustomVerticalShift);
  }
  else // if (CMediaSettings::Get().GetCurrentVideoSettings().m_ViewMode == ViewModeNormal)
  {
    CDisplaySettings::Get().SetPixelRatio(1.0);
    CDisplaySettings::Get().SetZoomAmount(1.0);
  }

  CMediaSettings::Get().GetCurrentVideoSettings().m_CustomZoomAmount = CDisplaySettings::Get().GetZoomAmount();
  CMediaSettings::Get().GetCurrentVideoSettings().m_CustomPixelRatio = CDisplaySettings::Get().GetPixelRatio();
  CMediaSettings::Get().GetCurrentVideoSettings().m_CustomNonLinStretch = CDisplaySettings::Get().IsNonLinearStretched();
  CMediaSettings::Get().GetCurrentVideoSettings().m_CustomVerticalShift = CDisplaySettings::Get().GetVerticalShift();
}

void CBaseRenderer::MarkDirty()
{
  g_windowManager.MarkDirty(m_destRect);
}

void CBaseRenderer::SettingOptionsRenderMethodsFiller(const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data)
{
  list.push_back(make_pair(g_localizeStrings.Get(13416), RENDER_METHOD_AUTO));

#ifdef HAS_DX
  if (CSysInfo::IsWindowsVersionAtLeast(CSysInfo::WindowsVersionWin7))
    list.push_back(make_pair(g_localizeStrings.Get(16326), RENDER_METHOD_DXVAHD));
  list.push_back(make_pair(g_localizeStrings.Get(16319), RENDER_METHOD_DXVA));
  list.push_back(make_pair(g_localizeStrings.Get(13431), RENDER_METHOD_D3D_PS));
  list.push_back(make_pair(g_localizeStrings.Get(13419), RENDER_METHOD_SOFTWARE));
#endif

#ifdef HAS_GL
  list.push_back(make_pair(g_localizeStrings.Get(13417), RENDER_METHOD_ARB));
  list.push_back(make_pair(g_localizeStrings.Get(13418), RENDER_METHOD_GLSL));
  list.push_back(make_pair(g_localizeStrings.Get(13419), RENDER_METHOD_SOFTWARE));
#endif
}
