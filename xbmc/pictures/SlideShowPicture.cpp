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

#include "SlideShowPicture.h"
#include "system.h"
#include "guilib/GraphicContext.h"
#include "guilib/Texture.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "windowing/WindowingFactory.h"
#include "threads/SingleLock.h"
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <math.h>

using namespace std;

#define IMMEDIATE_TRANSISTION_TIME          20

#define PICTURE_MOVE_AMOUNT              0.02f
#define PICTURE_MOVE_AMOUNT_ANALOG       0.01f
#define PICTURE_VIEW_BOX_COLOR      0xffffff00 // YELLOW
#define PICTURE_VIEW_BOX_BACKGROUND 0xff000000 // BLACK

#define FPS                                 25

static float zoomamount[10] = { 1.0f, 1.2f, 1.5f, 2.0f, 2.8f, 4.0f, 6.0f, 9.0f, 13.5f, 20.0f };

CSlideShowPic::CSlideShowPic()
{
  m_pImage = NULL;
  m_bIsLoaded = false;
  m_bIsFinished = false;
  m_bDrawNextImage = false;
  m_bTransistionImmediately = false;

  m_bCanMoveHorizontally = false;
  m_bCanMoveVertically = false;
}

CSlideShowPic::~CSlideShowPic()
{
  Close();
}

void CSlideShowPic::Close()
{
  CSingleLock lock(m_textureAccess);
  if (m_pImage)
  {
    delete m_pImage;
    m_pImage = NULL;
  }
  m_bIsLoaded = false;
  m_bIsFinished = false;
  m_bDrawNextImage = false;
  m_bTransistionImmediately = false;
  m_bIsDirty = true;
}

void CSlideShowPic::Reset(DISPLAY_EFFECT dispEffect, TRANSISTION_EFFECT transEffect)
{
  CSingleLock lock(m_textureAccess);
  if (m_pImage)
    SetTexture_Internal(m_iSlideNumber, m_pImage, dispEffect, transEffect);
  else
    Close();
}

bool CSlideShowPic::DisplayEffectNeedChange(DISPLAY_EFFECT newDispEffect) const
{
  if (m_displayEffect == newDispEffect)
    return false;
  if (newDispEffect == EFFECT_RANDOM && m_displayEffect != EFFECT_NONE && m_displayEffect != EFFECT_NO_TIMEOUT)
    return false;
  return true;
}

void CSlideShowPic::SetTexture(int iSlideNumber, CBaseTexture* pTexture, DISPLAY_EFFECT dispEffect, TRANSISTION_EFFECT transEffect)
{
  CSingleLock lock(m_textureAccess);
  Close();
  SetTexture_Internal(iSlideNumber, pTexture, dispEffect, transEffect);
}

void CSlideShowPic::SetTexture_Internal(int iSlideNumber, CBaseTexture* pTexture, DISPLAY_EFFECT dispEffect, TRANSISTION_EFFECT transEffect)
{
  CSingleLock lock(m_textureAccess);
  m_bPause = false;
  m_bNoEffect = false;
  m_bTransistionImmediately = false;
  m_iSlideNumber = iSlideNumber;

  m_bIsDirty = true;
  m_pImage = pTexture;
  m_fWidth = (float)pTexture->GetWidth();
  m_fHeight = (float)pTexture->GetHeight();
  // reset our counter
  m_iCounter = 0;
  // initialize our transistion effect
  m_transistionStart.type = transEffect;
  m_transistionStart.start = 0;
  // the +1's make sure it actually occurs
  float fadeTime = 0.2f;
  if (m_displayEffect != EFFECT_NO_TIMEOUT)
    fadeTime = std::min(0.2f*CSettings::Get().GetInt("slideshow.staytime"), 3.0f);
  m_transistionStart.length = (int)(g_graphicsContext.GetFPS() * fadeTime); // transition time in frames
  m_transistionEnd.type = transEffect;
  m_transistionEnd.length = m_transistionStart.length;
  m_transistionTemp.type = TRANSISTION_NONE;
  m_fTransistionAngle = 0;
  m_fTransistionZoom = 0;
  m_fAngle = 0.0f;
  if (pTexture->GetOrientation() == 7)
  { // rotate to 270 degrees
    m_fAngle = 270.0f;
  }
  if (pTexture->GetOrientation() == 2)
  { // rotate to 180 degrees
      m_fAngle = 180.0f;
  }
  if (pTexture->GetOrientation() == 5)
  { // rotate to 90 degrees
    m_fAngle = 90.0f;
  }
  m_fZoomAmount = 1;
  m_fZoomLeft = 0;
  m_fZoomTop = 0;
  // initialize our display effect
  if (dispEffect == EFFECT_RANDOM)
  {
    if (((m_fWidth / m_fHeight) > 1.9) || ((m_fHeight / m_fWidth) > 1.9))
      m_displayEffect = EFFECT_PANORAMA;
    else
      m_displayEffect = (DISPLAY_EFFECT)((rand() % (EFFECT_RANDOM - 1)) + 1);
  }
  else
    m_displayEffect = dispEffect;
  m_fPosX = m_fPosY = 0.0f;
  m_fPosZ = 1.0f;
  m_fVelocityX = m_fVelocityY = m_fVelocityZ = 0.0f;
  int iFrames = max((int)(g_graphicsContext.GetFPS() * CSettings::Get().GetInt("slideshow.staytime")), 1);
  if (m_displayEffect == EFFECT_PANORAMA)
  {
    RESOLUTION_INFO res = g_graphicsContext.GetResInfo();
    float fScreenWidth  = (float)res.Overscan.right  - res.Overscan.left;
    float fScreenHeight = (float)res.Overscan.bottom - res.Overscan.top;

    if (m_fWidth > m_fHeight)
    {
      iFrames = (int)(iFrames * (m_fWidth - m_fHeight) / m_fHeight);
      m_iTotalFrames = m_transistionStart.length + m_transistionEnd.length + iFrames;

      m_fPosX = 0.5f - (fScreenWidth / fScreenHeight) * (m_fHeight / m_fWidth) * 0.5f;
      if (rand() % 2)
        m_fPosX = -m_fPosX;
      m_fVelocityX = -m_fPosX * 2.0f / m_iTotalFrames;
    }
    else
    {
      iFrames = (int)(iFrames * (m_fHeight - (0.5f * m_fWidth)) / m_fWidth);
      m_iTotalFrames = m_transistionStart.length + m_transistionEnd.length + iFrames;

      m_fPosY = 0.5f - (fScreenHeight / fScreenWidth) * (m_fWidth / m_fHeight) * 0.5f;
      if (rand() % 2)
        m_fPosY = -m_fPosY;
      m_fVelocityY = -m_fPosY * 2.0f / m_iTotalFrames;
    }
  }
  else
  {
    m_iTotalFrames = m_transistionStart.length + m_transistionEnd.length + iFrames;

    if (m_displayEffect == EFFECT_FLOAT)
    {
      // Calculate start and end positions
      // choose a random direction
      float angle = (rand() % 1000) / 1000.0f * 2 * (float)M_PI;
      m_fPosX = cos(angle) * g_advancedSettings.m_slideshowPanAmount * m_iTotalFrames * 0.00005f;
      m_fPosY = sin(angle) * g_advancedSettings.m_slideshowPanAmount * m_iTotalFrames * 0.00005f;
      m_fVelocityX = -m_fPosX * 2.0f / m_iTotalFrames;
      m_fVelocityY = -m_fPosY * 2.0f / m_iTotalFrames;
    }
    else if (m_displayEffect == EFFECT_ZOOM)
    {
      m_fPosZ = 1.0f;
      m_fVelocityZ = 0.0001f * g_advancedSettings.m_slideshowZoomAmount;
    }
  }

  m_transistionEnd.start = m_transistionStart.length + iFrames;

  m_bIsFinished = false;
  m_bDrawNextImage = false;
  m_bIsLoaded = true;
  return ;
}

void CSlideShowPic::SetOriginalSize(int iOriginalWidth, int iOriginalHeight, bool bFullSize)
{
  m_iOriginalWidth = iOriginalWidth;
  m_iOriginalHeight = iOriginalHeight;
  m_bFullSize = bFullSize;
}

int CSlideShowPic::GetOriginalWidth()
{
  int iAngle = (int)(m_fAngle / 90.0f + 0.4f);
  if (iAngle % 2)
    return m_iOriginalHeight;
  else
    return m_iOriginalWidth;
}

int CSlideShowPic::GetOriginalHeight()
{
  int iAngle = (int)(m_fAngle / 90.0f + 0.4f);
  if (iAngle % 2)
    return m_iOriginalWidth;
  else
    return m_iOriginalHeight;
}

void CSlideShowPic::UpdateTexture(CBaseTexture* pTexture)
{
  CSingleLock lock(m_textureAccess);
  if (m_pImage)
  {
    delete m_pImage;
    m_pImage = NULL;
  }
  m_pImage = pTexture;
  m_fWidth = (float)pTexture->GetWidth();
  m_fHeight = (float)pTexture->GetHeight();
  m_bIsDirty = true;
}

static CRect GetRectangle(const float x[4], const float y[4])
{
  CRect rect;
  rect.x1 = *std::min_element(x, x+4);
  rect.y1 = *std::min_element(y, y+4);
  rect.x2 = *std::max_element(x, x+4);
  rect.y2 = *std::max_element(y, y+4);
  return rect;
}

void CSlideShowPic::UpdateVertices(float cur_x[4], float cur_y[4], const float new_x[4], const float new_y[4], CDirtyRegionList &dirtyregions)
{
  const size_t count = sizeof(float)*4;
  if(memcmp(cur_x, new_x, count)
  || memcmp(cur_y, new_y, count)
  || m_bIsDirty)
  {
    dirtyregions.push_back(GetRectangle(cur_x, cur_y));
    dirtyregions.push_back(GetRectangle(new_x, new_y));
    memcpy(cur_x, new_x, count);
    memcpy(cur_y, new_y, count);
  }
}

void CSlideShowPic::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  if (!m_pImage || !m_bIsLoaded || m_bIsFinished) return ;
  color_t alpha = m_alpha;
  if (m_iCounter <= m_transistionStart.length)
  { // do start transistion
    if (m_transistionStart.type == CROSSFADE)
    { // fade in at 1x speed
      alpha = (color_t)((float)m_iCounter / (float)m_transistionStart.length * 255.0f);
    }
    else if (m_transistionStart.type == FADEIN_FADEOUT)
    { // fade in at 2x speed, then keep solid
      alpha = (color_t)((float)m_iCounter / (float)m_transistionStart.length * 255.0f * 2);
      if (alpha > 255) alpha = 255;
    }
    else // m_transistionEffect == TRANSISTION_NONE
    {
      alpha = 0xFF; // opaque
    }
  }
  bool bPaused = m_bPause | (m_fZoomAmount != 1.0f);
  // check if we're doing a temporary effect (such as rotate + zoom)
  if (m_transistionTemp.type != TRANSISTION_NONE)
  {
    bPaused = true;
    if (m_iCounter >= m_transistionTemp.start)
    {
      if (m_iCounter >= m_transistionTemp.start + m_transistionTemp.length)
      { // we're finished this transistion
        if (m_transistionTemp.type == TRANSISTION_ZOOM)
        { // correct for any introduced inaccuracies.
          int i;
          for (i = 0; i < 10; i++)
          {
            if (fabs(m_fZoomAmount - zoomamount[i]) < 0.01*zoomamount[i])
            {
              m_fZoomAmount = zoomamount[i];
              break;
            }
          }
          m_bNoEffect = (m_fZoomAmount != 1.0f); // turn effect rendering back on.
        }
        m_transistionTemp.type = TRANSISTION_NONE;
      }
      else
      {
        if (m_transistionTemp.type == TRANSISTION_ROTATE)
          m_fAngle += m_fTransistionAngle;
        if (m_transistionTemp.type == TRANSISTION_ZOOM)
          m_fZoomAmount += m_fTransistionZoom;
      }
    }
  }
  // now just display
  if (!m_bNoEffect && !bPaused)
  {
    if (m_displayEffect == EFFECT_PANORAMA)
    {
      m_fPosX += m_fVelocityX;
      m_fPosY += m_fVelocityY;
    }
    else if (m_displayEffect == EFFECT_FLOAT)
    {
      m_fPosX += m_fVelocityX;
      m_fPosY += m_fVelocityY;
      float fMoveAmount = g_advancedSettings.m_slideshowPanAmount * m_iTotalFrames * 0.0001f;
      if (m_fPosX > fMoveAmount)
      {
        m_fPosX = fMoveAmount;
        m_fVelocityX = -m_fVelocityX;
      }
      if (m_fPosX < -fMoveAmount)
      {
        m_fPosX = -fMoveAmount;
        m_fVelocityX = -m_fVelocityX;
      }
      if (m_fPosY > fMoveAmount)
      {
        m_fPosY = fMoveAmount;
        m_fVelocityY = -m_fVelocityY;
      }
      if (m_fPosY < -fMoveAmount)
      {
        m_fPosY = -fMoveAmount;
        m_fVelocityY = -m_fVelocityY;
      }
    }
    else if (m_displayEffect == EFFECT_ZOOM)
    {
      m_fPosZ += m_fVelocityZ;
/*      if (m_fPosZ > 1.0f + 0.01f*CSettings::Get().GetInt("Slideshow.ZoomAmount"))
      {
        m_fPosZ = 1.0f + 0.01f * CSettings::Get().GetInt("Slideshow.ZoomAmount");
        m_fVelocityZ = -m_fVelocityZ;
      }
      if (m_fPosZ < 1.0f)
      {
        m_fPosZ = 1.0f;
        m_fVelocityZ = -m_fVelocityZ;
      }*/
    }
  }
  if (m_displayEffect != EFFECT_NO_TIMEOUT && bPaused && !m_bTransistionImmediately)
  { // paused - increment the last transistion start time
    m_transistionEnd.start++;
  }
  if (m_iCounter >= m_transistionEnd.start)
  { // do end transistion
//    CLog::Log(LOGDEBUG,"Transistioning");
    m_bDrawNextImage = true;
    if (m_transistionEnd.type == CROSSFADE)
    { // fade out at 1x speed
      alpha = 255 - (color_t)((float)(m_iCounter - m_transistionEnd.start) / (float)m_transistionEnd.length * 255.0f);
    }
    else if (m_transistionEnd.type == FADEIN_FADEOUT)
    { // keep solid, then fade out at 2x speed
      alpha = (color_t)((float)(m_transistionEnd.length - m_iCounter + m_transistionEnd.start) / (float)m_transistionEnd.length * 255.0f * 2);
      if (alpha > 255) alpha = 255;
    }
    else // m_transistionEffect == TRANSISTION_NONE
    {
      alpha = 0xFF; // opaque
    }
  }
  if (alpha != m_alpha)
  {
    m_alpha = alpha;
    m_bIsDirty = true;
  }
  if (m_displayEffect != EFFECT_NO_TIMEOUT || m_iCounter < m_transistionStart.length || m_iCounter >= m_transistionEnd.start || (m_iCounter >= m_transistionTemp.start && m_iCounter < m_transistionTemp.start + m_transistionTemp.length))
  {
    /* this really annoying.  there's non-stop logging when viewing a pic outside of the slideshow
    if (m_displayEffect == EFFECT_NO_TIMEOUT)
      CLog::Log(LOGDEBUG, "Incrementing counter (%i) while not in slideshow (startlength=%i,endstart=%i,endlength=%i)", m_iCounter, m_transistionStart.length, m_transistionEnd.start, m_transistionEnd.length);
    */
    m_iCounter++;
  }
  if (m_iCounter > m_transistionEnd.start + m_transistionEnd.length)
    m_bIsFinished = true;

  RESOLUTION_INFO info = g_graphicsContext.GetResInfo();

  // calculate where we should render (and how large it should be)
  // calculate aspect ratio correction factor
  float fOffsetX      = (float)info.Overscan.left;
  float fOffsetY      = (float)info.Overscan.top;
  float fScreenWidth  = (float)info.Overscan.right  - info.Overscan.left;
  float fScreenHeight = (float)info.Overscan.bottom - info.Overscan.top;
  float fPixelRatio   = info.fPixelRatio;

  // Rotate the image as needed
  float x[4];
  float y[4];
  float si = (float)sin(m_fAngle / 180.0f * M_PI);
  float co = (float)cos(m_fAngle / 180.0f * M_PI);
  x[0] = -m_fWidth * co + m_fHeight * si;
  y[0] = -m_fWidth * si - m_fHeight * co;
  x[1] = m_fWidth * co + m_fHeight * si;
  y[1] = m_fWidth * si - m_fHeight * co;
  x[2] = m_fWidth * co - m_fHeight * si;
  y[2] = m_fWidth * si + m_fHeight * co;
  x[3] = -m_fWidth * co - m_fHeight * si;
  y[3] = -m_fWidth * si + m_fHeight * co;

  // calculate our scale amounts
  float fSourceAR = m_fWidth / m_fHeight;
  float fSourceInvAR = 1 / fSourceAR;
  float fAR = si * si * (fSourceInvAR - fSourceAR) + fSourceAR;

  //float fOutputFrameAR = fAR / fPixelRatio;

  float fScaleNorm = fScreenWidth / m_fWidth;
  float fScaleInv = fScreenWidth / m_fHeight;

  bool bFillScreen = false;
  float fComp = 1.0f + 0.01f * g_advancedSettings.m_slideshowBlackBarCompensation;
  float fScreenRatio = fScreenWidth / fScreenHeight * fPixelRatio;
  // work out if we should be compensating the zoom to minimize blackbars
  // we should compute this based on the % of black bars on screen perhaps??
  // TODO: change m_displayEffect != EFFECT_NO_TIMEOUT to whether we're running the slideshow
  if (m_displayEffect != EFFECT_NO_TIMEOUT && fScreenRatio < fSourceAR * fComp && fSourceAR < fScreenRatio * fComp)
    bFillScreen = true;
  if ((!bFillScreen && fScreenWidth*fPixelRatio > fScreenHeight*fSourceAR) || (bFillScreen && fScreenWidth*fPixelRatio < fScreenHeight*fSourceAR))
    fScaleNorm = fScreenHeight / (m_fHeight * fPixelRatio);
  bFillScreen = false;
  if (m_displayEffect != EFFECT_NO_TIMEOUT && fScreenRatio < fSourceInvAR * fComp && fSourceInvAR < fScreenRatio * fComp)
    bFillScreen = true;
  if ((!bFillScreen && fScreenWidth*fPixelRatio > fScreenHeight*fSourceInvAR) || (bFillScreen && fScreenWidth*fPixelRatio < fScreenHeight*fSourceInvAR))
    fScaleInv = fScreenHeight / (m_fWidth * fPixelRatio);

  float fScale = si * si * (fScaleInv - fScaleNorm) + fScaleNorm;
  // scale if we need to due to the effect we're using
  if (m_displayEffect == EFFECT_PANORAMA)
  {
    if (m_fWidth > m_fHeight)
      fScale *= m_fWidth / fScreenWidth * fScreenHeight / m_fHeight;
    else
      fScale *= m_fHeight / fScreenHeight * fScreenWidth / m_fWidth;
  }
  if (m_displayEffect == EFFECT_FLOAT)
    fScale *= (1.0f + g_advancedSettings.m_slideshowPanAmount * m_iTotalFrames * 0.0001f);
  if (m_displayEffect == EFFECT_ZOOM)
    fScale *= m_fPosZ;
  // zoom image
  fScale *= m_fZoomAmount;

  // calculate the resultant coordinates
  for (int i = 0; i < 4; i++)
  {
    x[i] *= fScale * 0.5f; // as the offsets x[] and y[] are from center
    y[i] *= fScale * fPixelRatio * 0.5f;
    // center it
    x[i] += 0.5f * fScreenWidth + fOffsetX;
    y[i] += 0.5f * fScreenHeight + fOffsetY;
  }
  // shift if we're zooming
  if (m_fZoomAmount > 1)
  {
    float minx = x[0];
    float maxx = x[0];
    float miny = y[0];
    float maxy = y[0];
    for (int i = 1; i < 4; i++)
    {
      if (x[i] < minx) minx = x[i];
      if (x[i] > maxx) maxx = x[i];
      if (y[i] < miny) miny = y[i];
      if (y[i] > maxy) maxy = y[i];
    }
    float w = maxx - minx;
    float h = maxy - miny;
    m_bCanMoveHorizontally = (w >= fScreenWidth);
    m_bCanMoveVertically   = (h >= fScreenHeight);
    if (w >= fScreenWidth)
    { // must have no black bars
      if (minx + m_fZoomLeft*w > fOffsetX)
        m_fZoomLeft = (fOffsetX - minx) / w;
      if (maxx + m_fZoomLeft*w < fOffsetX + fScreenWidth)
        m_fZoomLeft = (fScreenWidth + fOffsetX - maxx) / w;
      for (int i = 0; i < 4; i++)
        x[i] += w * m_fZoomLeft;
    }
    if (h >= fScreenHeight)
    { // must have no black bars
      if (miny + m_fZoomTop*h > fOffsetY)
        m_fZoomTop = (fOffsetY - miny) / h;
      if (maxy + m_fZoomTop*h < fOffsetY + fScreenHeight)
        m_fZoomTop = (fScreenHeight + fOffsetY - maxy) / h;
      for (int i = 0; i < 4; i++)
        y[i] += m_fZoomTop * h;
    }
  }
  // add offset from display effects
  for (int i = 0; i < 4; i++)
  {
    x[i] += m_fPosX * m_fWidth * fScale;
    y[i] += m_fPosY * m_fHeight * fScale;
  }

  UpdateVertices(m_ax, m_ay, x, y, dirtyregions);

  // now render the image in the top right corner if we're zooming
  if (m_fZoomAmount == 1 || m_bIsComic)
  {
    const float empty[4] = {0};
    UpdateVertices(m_bx, m_by, empty, empty, dirtyregions);
    UpdateVertices(m_sx, m_sy, empty, empty, dirtyregions);
    UpdateVertices(m_ox, m_oy, empty, empty, dirtyregions);
    m_bIsDirty = false;
    return;
  }

  float sx[4], sy[4];
  sx[0] = -m_fWidth * co + m_fHeight * si;
  sy[0] = -m_fWidth * si - m_fHeight * co;
  sx[1] = m_fWidth * co + m_fHeight * si;
  sy[1] = m_fWidth * si - m_fHeight * co;
  sx[2] = m_fWidth * co - m_fHeight * si;
  sy[2] = m_fWidth * si + m_fHeight * co;
  sx[3] = -m_fWidth * co - m_fHeight * si;
  sy[3] = -m_fWidth * si + m_fHeight * co;
  // convert to the appropriate scale
  float fSmallArea = fScreenWidth * fScreenHeight / 50;
  float fSmallWidth = sqrt(fSmallArea * fAR / fPixelRatio); // fAR*height = width, so total area*far = width*width
  float fSmallHeight = fSmallArea / fSmallWidth;
  float fSmallX = fOffsetX + fScreenWidth * 0.95f - fSmallWidth * 0.5f;
  float fSmallY = fOffsetY + fScreenHeight * 0.05f + fSmallHeight * 0.5f;
  fScaleNorm = fSmallWidth / m_fWidth;
  fScaleInv = fSmallWidth / m_fHeight;
  fScale = si * si * (fScaleInv - fScaleNorm) + fScaleNorm;
  for (int i = 0; i < 4; i++)
  {
    sx[i] *= fScale * 0.5f;
    sy[i] *= fScale * fPixelRatio * 0.5f;
  }
  // calculate a black border
  float bx[4];
  float by[4];
  for (int i = 0; i < 4; i++)
  {
    if (sx[i] > 0)
      bx[i] = sx[i] + 1;
    else
      bx[i] = sx[i] - 1;
    if (sy[i] > 0)
      by[i] = sy[i] + 1;
    else
      by[i] = sy[i] - 1;
    sx[i] += fSmallX;
    sy[i] += fSmallY;
    bx[i] += fSmallX;
    by[i] += fSmallY;
  }

  fSmallX -= fSmallWidth * 0.5f;
  fSmallY -= fSmallHeight * 0.5f;

  UpdateVertices(m_bx, m_by, bx, by, dirtyregions);
  UpdateVertices(m_sx, m_sy, sx, sy, dirtyregions);

  // now we must render the wireframe image of the view window
  // work out the direction of the top of pic vector
  float scale;
  if (fabs(x[1] - x[0]) > fabs(x[3] - x[0]))
    scale = (sx[1] - sx[0]) / (x[1] - x[0]);
  else
    scale = (sx[3] - sx[0]) / (x[3] - x[0]);
  float ox[4];
  float oy[4];
  ox[0] = (fOffsetX - x[0]) * scale + sx[0];
  oy[0] = (fOffsetY - y[0]) * scale + sy[0];
  ox[1] = (fOffsetX + fScreenWidth - x[0]) * scale + sx[0];
  oy[1] = (fOffsetY - y[0]) * scale + sy[0];
  ox[2] = (fOffsetX + fScreenWidth - x[0]) * scale + sx[0];
  oy[2] = (fOffsetY + fScreenHeight - y[0]) * scale + sy[0];
  ox[3] = (fOffsetX - x[0]) * scale + sx[0];
  oy[3] = (fOffsetY + fScreenHeight - y[0]) * scale + sy[0];
  // crop to within the range of our piccy
  for (int i = 0; i < 4; i++)
  {
    if (ox[i] < fSmallX) ox[i] = fSmallX;
    if (ox[i] > fSmallX + fSmallWidth) ox[i] = fSmallX + fSmallWidth;
    if (oy[i] < fSmallY) oy[i] = fSmallY;
    if (oy[i] > fSmallY + fSmallHeight) oy[i] = fSmallY + fSmallHeight;
  }

  UpdateVertices(m_ox, m_oy, ox, oy, dirtyregions);
  m_bIsDirty = false;
}

void CSlideShowPic::Keep()
{
  // this is called if we need to keep the current pic on screen
  // to wait for the next pic to load
  if (!m_bDrawNextImage) return ; // don't need to keep pic
  // hold off the start of the next frame
  m_transistionEnd.start = m_iCounter;
}

bool CSlideShowPic::StartTransistion()
{
  // this is called if we need to start transistioning immediately to the new picture
  if (m_bDrawNextImage) return false; // don't need to do anything as we are already transistioning
  // decrease the number of display frame
  m_transistionEnd.start = m_iCounter;
  m_bTransistionImmediately = true;
  return true;
}

void CSlideShowPic::Pause(bool bPause)
{
  if (!m_bDrawNextImage)
    m_bPause = bPause;
}

void CSlideShowPic::SetInSlideshow(bool slideshow)
{
  if (slideshow && m_displayEffect == EFFECT_NO_TIMEOUT)
    m_displayEffect = EFFECT_NONE;
}

int CSlideShowPic::GetTransistionTime(int iType) const
{
  if (iType == 0) // start transistion
    return m_transistionStart.length;
  else // iType == 1 // end transistion
    return m_transistionEnd.length;
}

void CSlideShowPic::SetTransistionTime(int iType, int iTime)
{
  if (iType == 0) // start transistion
    m_transistionStart.length = iTime;
  else // iType == 1 // end transistion
    m_transistionEnd.length = iTime;
}

void CSlideShowPic::Rotate(float fRotateAngle, bool immediate /* = false */)
{
  if (m_bDrawNextImage) return;
  if (m_transistionTemp.type == TRANSISTION_ZOOM) return;
  if (immediate)
  {
    m_fAngle += fRotateAngle;
    return;
  }

  // if there is a rotation ongoing already
  // add the new angle to the old destination angle
  if (m_transistionTemp.type == TRANSISTION_ROTATE && 
      m_transistionTemp.start + m_transistionTemp.length > m_iCounter)
  {
    int remainder = m_transistionTemp.start + m_transistionTemp.length - m_iCounter;
    fRotateAngle += m_fTransistionAngle * remainder;
  }

  m_transistionTemp.type = TRANSISTION_ROTATE;
  m_transistionTemp.start = m_iCounter;
  m_transistionTemp.length = IMMEDIATE_TRANSISTION_TIME;
  m_fTransistionAngle = (float)fRotateAngle / (float)m_transistionTemp.length;
  // reset the timer
  m_transistionEnd.start = m_iCounter + m_transistionStart.length + (int)(g_graphicsContext.GetFPS() * CSettings::Get().GetInt("slideshow.staytime"));
}

void CSlideShowPic::Zoom(float fZoom, bool immediate /* = false */)
{
  if (m_bDrawNextImage) return;
  if (m_transistionTemp.type == TRANSISTION_ROTATE) return;
  if (immediate)
  {
    m_fZoomAmount = fZoom;
    return;
  }
  m_transistionTemp.type = TRANSISTION_ZOOM;
  m_transistionTemp.start = m_iCounter;
  m_transistionTemp.length = IMMEDIATE_TRANSISTION_TIME;
  m_fTransistionZoom = (fZoom - m_fZoomAmount) / (float)m_transistionTemp.length;
  // reset the timer
  m_transistionEnd.start = m_iCounter + m_transistionStart.length + (int)(g_graphicsContext.GetFPS() * CSettings::Get().GetInt("slideshow.staytime"));
  // turn off the render effects until we're back down to normal zoom
  m_bNoEffect = true;
}

void CSlideShowPic::Move(float fDeltaX, float fDeltaY)
{
  m_fZoomLeft += fDeltaX;
  m_fZoomTop += fDeltaY;
  // reset the timer
 // m_transistionEnd.start = m_iCounter + m_transistionStart.length + (int)(g_graphicsContext.GetFPS() * CSettings::Get().GetInt("slideshow.staytime"));
}

void CSlideShowPic::Render()
{
  CSingleLock lock(m_textureAccess);

  Render(m_ax, m_ay, m_pImage, (m_alpha << 24) | 0xFFFFFF);

  // now render the image in the top right corner if we're zooming
  if (m_fZoomAmount == 1.0f || m_bIsComic) return ;

  Render(m_bx, m_by, NULL, PICTURE_VIEW_BOX_BACKGROUND);
  Render(m_sx, m_sy, m_pImage, 0xFFFFFFFF);
  Render(m_ox, m_oy, NULL, PICTURE_VIEW_BOX_COLOR);
}

void CSlideShowPic::Render(float *x, float *y, CBaseTexture* pTexture, color_t color)
{
#ifdef HAS_DX
  struct VERTEX
  {
    D3DXVECTOR3 p;
    D3DCOLOR col;
    FLOAT tu, tv;
  };
  static const DWORD FVF_VERTEX = D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1;

  VERTEX vertex[5];

  for (int i = 0; i < 4; i++)
  {
    vertex[i].p = D3DXVECTOR3( x[i], y[i], 0);
    vertex[i].tu = 0;
    vertex[i].tv = 0;
    vertex[i].col = color;
  }

  if (pTexture)
  {
    vertex[1].tu = vertex[2].tu = (float)pTexture->GetWidth() / pTexture->GetTextureWidth();
    vertex[2].tv = vertex[3].tv = (float)pTexture->GetHeight() / pTexture->GetTextureHeight();
  }
  else
  {
    vertex[1].tu = vertex[2].tu = 1.0f;
    vertex[2].tv = vertex[3].tv = 1.0f;
  }
  
  vertex[4] = vertex[0]; // Not used when pTexture != NULL

  // Set state to render the image
  if (pTexture)
  {
    pTexture->LoadToGPU();
    pTexture->BindToUnit(0);
  }

  g_Windowing.Get3DDevice()->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
  g_Windowing.Get3DDevice()->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
  g_Windowing.Get3DDevice()->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
  g_Windowing.Get3DDevice()->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE );
  g_Windowing.Get3DDevice()->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
  g_Windowing.Get3DDevice()->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
  g_Windowing.Get3DDevice()->SetTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );
  g_Windowing.Get3DDevice()->SetTextureStageState( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
  g_Windowing.Get3DDevice()->SetRenderState( D3DRS_ZENABLE, FALSE );
  g_Windowing.Get3DDevice()->SetRenderState( D3DRS_FOGENABLE, FALSE );
  g_Windowing.Get3DDevice()->SetRenderState( D3DRS_FOGTABLEMODE, D3DFOG_NONE );
  g_Windowing.Get3DDevice()->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
  g_Windowing.Get3DDevice()->SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
  g_Windowing.Get3DDevice()->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
  g_Windowing.Get3DDevice()->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
  g_Windowing.Get3DDevice()->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
  g_Windowing.Get3DDevice()->SetRenderState(D3DRS_LIGHTING, FALSE);
  g_Windowing.Get3DDevice()->SetFVF( FVF_VERTEX );
  // Render the image
  if (pTexture)
  {
    g_Windowing.Get3DDevice()->DrawPrimitiveUP( D3DPT_TRIANGLEFAN, 2, vertex, sizeof(VERTEX) );
    g_Windowing.Get3DDevice()->SetTexture(0, NULL);
  } else
    g_Windowing.Get3DDevice()->DrawPrimitiveUP( D3DPT_LINESTRIP, 4, vertex, sizeof(VERTEX) );

#elif defined(HAS_GL)
  g_graphicsContext.BeginPaint();
  if (pTexture)
  {
    int unit = 0;
    pTexture->LoadToGPU();
    pTexture->BindToUnit(unit++);

    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);          // Turn Blending On

    // diffuse coloring
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
    glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
    glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE0);
    glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
    glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PRIMARY_COLOR);
    glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);

    if(g_Windowing.UseLimitedColor())
    {
      // compress range
      pTexture->BindToUnit(unit++); // dummy bind
      const GLfloat rgba1[4] = {(235.0 - 16.0f) / 255.0f, (235.0 - 16.0f) / 255.0f, (235.0 - 16.0f) / 255.0f, 1.0f};
      glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE , GL_COMBINE);
      glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, rgba1);
      glTexEnvi (GL_TEXTURE_ENV, GL_COMBINE_RGB , GL_MODULATE);
      glTexEnvi (GL_TEXTURE_ENV, GL_SOURCE0_RGB , GL_PREVIOUS);
      glTexEnvi (GL_TEXTURE_ENV, GL_SOURCE1_RGB , GL_CONSTANT);
      glTexEnvi (GL_TEXTURE_ENV, GL_OPERAND0_RGB , GL_SRC_COLOR);
      glTexEnvi (GL_TEXTURE_ENV, GL_OPERAND1_RGB , GL_SRC_COLOR);
      glTexEnvi (GL_TEXTURE_ENV, GL_COMBINE_ALPHA , GL_REPLACE);
      glTexEnvi (GL_TEXTURE_ENV, GL_SOURCE0_ALPHA , GL_PREVIOUS);

      // transition
      pTexture->BindToUnit(unit++); // dummy bind
      const GLfloat rgba2[4] = {16.0f / 255.0f, 16.0f / 255.0f, 16.0f / 255.0f, 0.0f};
      glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE , GL_COMBINE);
      glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, rgba2);
      glTexEnvi (GL_TEXTURE_ENV, GL_COMBINE_RGB , GL_ADD);
      glTexEnvi (GL_TEXTURE_ENV, GL_SOURCE0_RGB , GL_PREVIOUS);
      glTexEnvi (GL_TEXTURE_ENV, GL_SOURCE1_RGB , GL_CONSTANT);
      glTexEnvi (GL_TEXTURE_ENV, GL_OPERAND0_RGB , GL_SRC_COLOR);
      glTexEnvi (GL_TEXTURE_ENV, GL_OPERAND1_RGB , GL_SRC_COLOR);
      glTexEnvi (GL_TEXTURE_ENV, GL_COMBINE_ALPHA , GL_REPLACE);
      glTexEnvi (GL_TEXTURE_ENV, GL_SOURCE0_ALPHA , GL_PREVIOUS);
    }
  }
  else
    glDisable(GL_TEXTURE_2D);
  glPolygonMode(GL_FRONT_AND_BACK, pTexture ? GL_FILL : GL_LINE);

  glBegin(GL_QUADS);
  float u1 = 0, u2 = 1, v1 = 0, v2 = 1;
  if (pTexture)
  {
    u2 = (float)pTexture->GetWidth() / pTexture->GetTextureWidth();
    v2 = (float)pTexture->GetHeight() / pTexture->GetTextureHeight();
  }

  glColor4ub((GLubyte)GET_R(color), (GLubyte)GET_G(color), (GLubyte)GET_B(color), (GLubyte)GET_A(color));
  glTexCoord2f(u1, v1);
  glVertex3f(x[0], y[0], 0);

  // Bottom-left vertex (corner)
  glColor4ub((GLubyte)GET_R(color), (GLubyte)GET_G(color), (GLubyte)GET_B(color), (GLubyte)GET_A(color));
  glTexCoord2f(u2, v1);
  glVertex3f(x[1], y[1], 0);

  // Bottom-right vertex (corner)
  glColor4ub((GLubyte)GET_R(color), (GLubyte)GET_G(color), (GLubyte)GET_B(color), (GLubyte)GET_A(color));
  glTexCoord2f(u2, v2);
  glVertex3f(x[2], y[2], 0);

  // Top-right vertex (corner)
  glColor4ub((GLubyte)GET_R(color), (GLubyte)GET_G(color), (GLubyte)GET_B(color), (GLubyte)GET_A(color));
  glTexCoord2f(u1, v2);
  glVertex3f(x[3], y[3], 0);

  glEnd();
  g_graphicsContext.EndPaint();
#elif defined(HAS_GLES)
  g_graphicsContext.BeginPaint();
  if (pTexture)
  {
    pTexture->LoadToGPU();
    pTexture->BindToUnit(0);

    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);          // Turn Blending On

    g_Windowing.EnableGUIShader(SM_TEXTURE);
  }
  else
  {
    glDisable(GL_TEXTURE_2D);

    g_Windowing.EnableGUIShader(SM_DEFAULT);
  }

  float u1 = 0, u2 = 1, v1 = 0, v2 = 1;
  if (pTexture)
  {
    u2 = (float)pTexture->GetWidth() / pTexture->GetTextureWidth();
    v2 = (float)pTexture->GetHeight() / pTexture->GetTextureHeight();
  }

  GLubyte col[4];
  GLfloat ver[4][3];
  GLfloat tex[4][2];
  GLubyte idx[4] = {0, 1, 3, 2};        //determines order of triangle strip

  GLint posLoc  = g_Windowing.GUIShaderGetPos();
  GLint tex0Loc = g_Windowing.GUIShaderGetCoord0();
  GLint uniColLoc= g_Windowing.GUIShaderGetUniCol();

  glVertexAttribPointer(posLoc,  3, GL_FLOAT, 0, 0, ver);
  glVertexAttribPointer(tex0Loc, 2, GL_FLOAT, 0, 0, tex);

  glEnableVertexAttribArray(posLoc);
  glEnableVertexAttribArray(tex0Loc);

  // Setup Colour values
  col[0] = (GLubyte)GET_R(color);
  col[1] = (GLubyte)GET_G(color);
  col[2] = (GLubyte)GET_B(color);
  col[3] = (GLubyte)GET_A(color);

  for (int i=0; i<4; i++)
  {
    // Setup vertex position values
    ver[i][0] = x[i];
    ver[i][1] = y[i];
    ver[i][2] = 0.0f;
  }
  // Setup texture coordinates
  tex[0][0] = tex[3][0] = u1;
  tex[0][1] = tex[1][1] = v1;
  tex[1][0] = tex[2][0] = u2;
  tex[2][1] = tex[3][1] = v2;

  glUniform4f(uniColLoc,(col[0] / 255.0f), (col[1] / 255.0f), (col[2] / 255.0f), (col[3] / 255.0f));
  glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_BYTE, idx);

  glDisableVertexAttribArray(posLoc);
  glDisableVertexAttribArray(tex0Loc);

  g_Windowing.DisableGUIShader();

  g_graphicsContext.EndPaint();
#else
// SDL render
  g_Windowing.BlitToScreen(m_pImage, NULL, NULL);
#endif
}
