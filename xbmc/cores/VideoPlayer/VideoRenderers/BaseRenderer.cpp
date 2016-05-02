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

#include <cstdlib> // std::abs(int) prototype
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
#include "cores/VideoPlayer/VideoRenderers/RenderFlags.h"


CBaseRenderer::CBaseRenderer()
{
  m_sourceFrameRatio = 1.0f;
  m_sourceWidth = 720;
  m_sourceHeight = 480;
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
}

CBaseRenderer::~CBaseRenderer()
{
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


  int diffX = 0;
  int diffY = 0;
  int centerX = 0;
  int centerY = 0;
  
  if (changeAspect)// we are either rotating by 90 or 270 degrees which inverts aspect ratio
  {
    int newWidth = m_destRect.Height(); // new width is old height
    int newHeight = m_destRect.Width(); // new height is old width
    int diffWidth = newWidth - m_destRect.Width(); // difference between old and new width
    int diffHeight = newHeight - m_destRect.Height(); // difference between old and new height

    // if the new width is bigger then the old or
    // the new height is bigger then the old - we need to scale down
    if (diffWidth > 0 || diffHeight > 0 )
    {
      float aspectRatio = GetAspectRatio();
      // scale to fit screen width because
      // the difference in width is bigger then the
      // difference in height
      if (diffWidth > diffHeight)
      {
        newWidth = m_destRect.Width(); // clamp to the width of the old dest rect
        newHeight *= aspectRatio;
      }
      else // scale to fit screen height
      {
        newHeight = m_destRect.Height(); // clamp to the height of the old dest rect
        newWidth /= aspectRatio;
      }
    }
    
    // calculate the center point of the view
    centerX = m_viewRect.x1 + m_viewRect.Width() / 2;
    centerY = m_viewRect.y1 + m_viewRect.Height() / 2;

    // calculate the number of pixels we need to go in each
    // x direction from the center point
    diffX = newWidth / 2;
    // calculate the number of pixels we need to go in each
    // y direction from the center point
    diffY = newHeight / 2;
    
  }

  for (int destIdx=0, srcIdx=pointOffset; destIdx < 4; destIdx++)
  {
    m_rotatedDestCoords[destIdx].x = origMat[srcIdx][0];
    m_rotatedDestCoords[destIdx].y = origMat[srcIdx][1];

    if (changeAspect)
    {
      switch (srcIdx)
      {
        case 0:// top left
          m_rotatedDestCoords[destIdx].x = centerX - diffX;
          m_rotatedDestCoords[destIdx].y = centerY - diffY;
          break;
        case 1:// top right
          m_rotatedDestCoords[destIdx].x = centerX + diffX;
          m_rotatedDestCoords[destIdx].y = centerY - diffY;
          break;
        case 2:// bottom right
          m_rotatedDestCoords[destIdx].x = centerX + diffX;
          m_rotatedDestCoords[destIdx].y = centerY + diffY;
          break;
        case 3:// bottom left
          m_rotatedDestCoords[destIdx].x = centerX - diffX;
          m_rotatedDestCoords[destIdx].y = centerY + diffY;
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

void CBaseRenderer::CalcNormalRenderRect(float offsetX, float offsetY, float width, float height,
                                         float inputFrameRatio, float zoomAmount, float verticalShift)
{
  // if view window is empty, set empty destination
  if(height == 0 || width == 0)
  {
    m_destRect.SetRect(0.0f, 0.0f, 0.0f, 0.0f);
    return;
  }

  // scale up image as much as possible
  // and keep the aspect ratio (introduces with black bars)
  // calculate the correct output frame ratio (using the users pixel ratio setting
  // and the output pixel ratio setting)

  float outputFrameRatio = inputFrameRatio / g_graphicsContext.GetResInfo().fPixelRatio;

  // allow a certain error to maximize size of render area
  float fCorrection = width / height / outputFrameRatio - 1.0f;
  float fAllowed    = CSettings::GetInstance().GetInt(CSettings::SETTING_VIDEOPLAYER_ERRORINASPECT) * 0.01f;
  if(fCorrection >   fAllowed) fCorrection =   fAllowed;
  if(fCorrection < - fAllowed) fCorrection = - fAllowed;

  outputFrameRatio *= 1.0f + fCorrection;

  // maximize the movie width
  float newWidth = width;
  float newHeight = newWidth / outputFrameRatio;

  if (newHeight > height)
  {
    newHeight = height;
    newWidth = newHeight * outputFrameRatio;
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

  m_destRect.x1 = (float)MathUtils::round_int(posX + offsetX);
  m_destRect.x2 = m_destRect.x1 + MathUtils::round_int(newWidth);
  m_destRect.y1 = (float)MathUtils::round_int(posY + offsetY);
  m_destRect.y2 = m_destRect.y1 + MathUtils::round_int(newHeight);

  // clip as needed
  if (!(g_graphicsContext.IsFullScreenVideo() || g_graphicsContext.IsCalibrating()))
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

void CBaseRenderer::ManageRenderArea()
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

  if (m_format != RENDER_FMT_BYPASS)
  {
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
  }

  CalcNormalRenderRect(m_viewRect.x1, m_viewRect.y1, m_viewRect.Width(), m_viewRect.Height(), GetAspectRatio() * CDisplaySettings::GetInstance().GetPixelRatio(), CDisplaySettings::GetInstance().GetZoomAmount(), CDisplaySettings::GetInstance().GetVerticalShift());
}

void CBaseRenderer::SetViewMode(int viewMode)
{
  if (viewMode < ViewModeNormal || viewMode > ViewModeStretch16x9Nonlin)
    viewMode = ViewModeNormal;

  CMediaSettings::GetInstance().GetCurrentVideoSettings().m_ViewMode = viewMode;

  // get our calibrated full screen resolution
  RESOLUTION res = g_graphicsContext.GetVideoResolution();
  RESOLUTION_INFO info = g_graphicsContext.GetResInfo();
  float screenWidth  = (float)(info.Overscan.right  - info.Overscan.left);
  float screenHeight = (float)(info.Overscan.bottom - info.Overscan.top);

  // and the source frame ratio
  float sourceFrameRatio = GetAspectRatio();

  bool is43 = (sourceFrameRatio < 8.f/(3.f*sqrt(3.f)) &&
              CMediaSettings::GetInstance().GetCurrentVideoSettings().m_ViewMode == ViewModeNormal);

  // Splitres scaling factor
  float xscale = (float)info.iScreenWidth  / (float)info.iWidth;
  float yscale = (float)info.iScreenHeight / (float)info.iHeight;

  screenWidth   *= xscale;
  screenHeight  *= yscale;

  CDisplaySettings::GetInstance().SetVerticalShift(0.0f);
  CDisplaySettings::GetInstance().SetNonLinearStretched(false);

  if ( CMediaSettings::GetInstance().GetCurrentVideoSettings().m_ViewMode == ViewModeZoom ||
       (is43 && CSettings::GetInstance().GetInt(CSettings::SETTING_VIDEOPLAYER_STRETCH43) == ViewModeZoom))
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
  else if (CMediaSettings::GetInstance().GetCurrentVideoSettings().m_ViewMode == ViewModeStretch4x3)
  { // stretch image to 4:3 ratio
    CDisplaySettings::GetInstance().SetZoomAmount(1.0);
    if (res == RES_PAL_4x3 || res == RES_PAL60_4x3 || res == RES_NTSC_4x3 || res == RES_HDTV_480p_4x3)
    { // stretch to the limits of the 4:3 screen.
      // incorrect behaviour, but it's what the users want, so...
      CDisplaySettings::GetInstance().SetPixelRatio((screenWidth / screenHeight) * info.fPixelRatio / sourceFrameRatio);
    }
    else
    {
      // now we need to set CDisplaySettings::GetInstance().GetPixelRatio() so that
      // fOutputFrameRatio = 4:3.
      CDisplaySettings::GetInstance().SetPixelRatio((4.0f / 3.0f) / sourceFrameRatio);
    }
  }
  else if ( CMediaSettings::GetInstance().GetCurrentVideoSettings().m_ViewMode == ViewModeWideZoom ||
           (is43 && CSettings::GetInstance().GetInt(CSettings::SETTING_VIDEOPLAYER_STRETCH43) == ViewModeWideZoom))
  { // super zoom
    float stretchAmount = (screenWidth / screenHeight) * info.fPixelRatio / sourceFrameRatio;
    CDisplaySettings::GetInstance().SetPixelRatio(pow(stretchAmount, float(2.0/3.0)));
    CDisplaySettings::GetInstance().SetZoomAmount(pow(stretchAmount, float((stretchAmount < 1.0) ? -1.0/3.0 : 1.0/3.0)));
    CDisplaySettings::GetInstance().SetNonLinearStretched(true);
  }
  else if ( CMediaSettings::GetInstance().GetCurrentVideoSettings().m_ViewMode == ViewModeStretch16x9 ||
            CMediaSettings::GetInstance().GetCurrentVideoSettings().m_ViewMode == ViewModeStretch16x9Nonlin ||
           (is43 && (CSettings::GetInstance().GetInt(CSettings::SETTING_VIDEOPLAYER_STRETCH43) == ViewModeStretch16x9 ||
                     CSettings::GetInstance().GetInt(CSettings::SETTING_VIDEOPLAYER_STRETCH43) == ViewModeStretch16x9Nonlin)))
  { // stretch image to 16:9 ratio
    CDisplaySettings::GetInstance().SetZoomAmount(1.0);
    if (res == RES_PAL_4x3 || res == RES_PAL60_4x3 || res == RES_NTSC_4x3 || res == RES_HDTV_480p_4x3)
    { // now we need to set CDisplaySettings::GetInstance().GetPixelRatio() so that
      // outputFrameRatio = 16:9.
      CDisplaySettings::GetInstance().SetPixelRatio((16.0f / 9.0f) / sourceFrameRatio);
    }
    else
    { // stretch to the limits of the 16:9 screen.
      // incorrect behaviour, but it's what the users want, so...
      CDisplaySettings::GetInstance().SetPixelRatio((screenWidth / screenHeight) * info.fPixelRatio / sourceFrameRatio);
    }
    bool nonlin = (is43 && CSettings::GetInstance().GetInt(CSettings::SETTING_VIDEOPLAYER_STRETCH43) == ViewModeStretch16x9Nonlin) ||
                  CMediaSettings::GetInstance().GetCurrentVideoSettings().m_ViewMode == ViewModeStretch16x9Nonlin;
    CDisplaySettings::GetInstance().SetNonLinearStretched(nonlin);
  }
  else  if (CMediaSettings::GetInstance().GetCurrentVideoSettings().m_ViewMode == ViewModeOriginal)
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
  else if (CMediaSettings::GetInstance().GetCurrentVideoSettings().m_ViewMode == ViewModeCustom)
  {
    CDisplaySettings::GetInstance().SetZoomAmount(CMediaSettings::GetInstance().GetCurrentVideoSettings().m_CustomZoomAmount);
    CDisplaySettings::GetInstance().SetPixelRatio(CMediaSettings::GetInstance().GetCurrentVideoSettings().m_CustomPixelRatio);
    CDisplaySettings::GetInstance().SetNonLinearStretched(CMediaSettings::GetInstance().GetCurrentVideoSettings().m_CustomNonLinStretch);
    CDisplaySettings::GetInstance().SetVerticalShift(CMediaSettings::GetInstance().GetCurrentVideoSettings().m_CustomVerticalShift);
  }
  else // if (CMediaSettings::GetInstance().GetCurrentVideoSettings().m_ViewMode == ViewModeNormal)
  {
    CDisplaySettings::GetInstance().SetPixelRatio(1.0);
    CDisplaySettings::GetInstance().SetZoomAmount(1.0);
  }

  CMediaSettings::GetInstance().GetCurrentVideoSettings().m_CustomZoomAmount = CDisplaySettings::GetInstance().GetZoomAmount();
  CMediaSettings::GetInstance().GetCurrentVideoSettings().m_CustomPixelRatio = CDisplaySettings::GetInstance().GetPixelRatio();
  CMediaSettings::GetInstance().GetCurrentVideoSettings().m_CustomNonLinStretch = CDisplaySettings::GetInstance().IsNonLinearStretched();
  CMediaSettings::GetInstance().GetCurrentVideoSettings().m_CustomVerticalShift = CDisplaySettings::GetInstance().GetVerticalShift();
}

void CBaseRenderer::MarkDirty()
{
  g_windowManager.MarkDirty(m_destRect);
}

void CBaseRenderer::SettingOptionsRenderMethodsFiller(const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data)
{
  list.push_back(make_pair(g_localizeStrings.Get(13416), RENDER_METHOD_AUTO));

#ifdef HAS_DX
  list.push_back(make_pair(g_localizeStrings.Get(16319), RENDER_METHOD_DXVA));
  list.push_back(make_pair(g_localizeStrings.Get(13431), RENDER_METHOD_D3D_PS));
  list.push_back(make_pair(g_localizeStrings.Get(13419), RENDER_METHOD_SOFTWARE));
#endif

#ifdef HAS_GL
  list.push_back(make_pair(g_localizeStrings.Get(13417), RENDER_METHOD_ARB));
  list.push_back(make_pair(g_localizeStrings.Get(13418), RENDER_METHOD_GLSL));
#endif
}
