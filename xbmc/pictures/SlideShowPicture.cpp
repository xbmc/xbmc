/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SlideShowPicture.h"
#include "ServiceBroker.h"
#include "windowing/GraphicContext.h"
#include "guilib/Texture.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "threads/SingleLock.h"
#include "windowing/WinSystem.h"
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <math.h>

#if defined(HAS_GL)
#include "rendering/gl/RenderSystemGL.h"
#elif defined(HAS_GLES)
#include "rendering/gles/RenderSystemGLES.h"
#elif defined(TARGET_WINDOWS)
#include "rendering/dx/DeviceResources.h"
#include "rendering/dx/RenderContext.h"
#include <DirectXMath.h>
using namespace DirectX;
using namespace Microsoft::WRL;
#endif

#define IMMEDIATE_TRANSITION_TIME          20

#define PICTURE_MOVE_AMOUNT              0.02f
#define PICTURE_MOVE_AMOUNT_ANALOG       0.01f
#define PICTURE_VIEW_BOX_COLOR      0xffffff00 // YELLOW
#define PICTURE_VIEW_BOX_BACKGROUND 0xff000000 // BLACK

#define FPS                                 25

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

static float zoomamount[10] = { 1.0f, 1.2f, 1.5f, 2.0f, 2.8f, 4.0f, 6.0f, 9.0f, 13.5f, 20.0f };

CSlideShowPic::CSlideShowPic()
{
  m_pImage = NULL;
  m_bIsLoaded = false;
  m_bIsFinished = false;
  m_bDrawNextImage = false;
  m_bTransitionImmediately = false;

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
    m_pImage = nullptr;
  }
  m_bIsLoaded = false;
  m_bIsFinished = false;
  m_bDrawNextImage = false;
  m_bTransitionImmediately = false;
  m_bIsDirty = true;
  m_alpha = 0;
#ifdef HAS_DX
  m_vb = nullptr;
#endif
}

void CSlideShowPic::Reset(DISPLAY_EFFECT dispEffect, TRANSITION_EFFECT transEffect)
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

void CSlideShowPic::SetTexture(int iSlideNumber, CBaseTexture* pTexture, DISPLAY_EFFECT dispEffect, TRANSITION_EFFECT transEffect)
{
  CSingleLock lock(m_textureAccess);
  Close();
  SetTexture_Internal(iSlideNumber, pTexture, dispEffect, transEffect);
}

void CSlideShowPic::SetTexture_Internal(int iSlideNumber, CBaseTexture* pTexture, DISPLAY_EFFECT dispEffect, TRANSITION_EFFECT transEffect)
{
  CSingleLock lock(m_textureAccess);
  m_bPause = false;
  m_bNoEffect = false;
  m_bTransitionImmediately = false;
  m_iSlideNumber = iSlideNumber;

  m_bIsDirty = true;
  m_pImage = pTexture;
  m_fWidth = (float)pTexture->GetWidth();
  m_fHeight = (float)pTexture->GetHeight();
  if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_SLIDESHOW_HIGHQUALITYDOWNSCALING))
  { // activate mipmapping when high quality downscaling is 'on'
    pTexture->SetMipmapping();
  }
  // reset our counter
  m_iCounter = 0;
  // initialize our transition effect
  m_transitionStart.type = transEffect;
  m_transitionStart.start = 0;

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

  // the +1's make sure it actually occurs
  float fadeTime = 0.2f;
  if (m_displayEffect != EFFECT_NO_TIMEOUT)
    fadeTime = std::min(0.2f*CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_SLIDESHOW_STAYTIME), 3.0f);
  m_transitionStart.length = (int)(CServiceBroker::GetWinSystem()->GetGfxContext().GetFPS() * fadeTime); // transition time in frames
  m_transitionEnd.type = transEffect;
  m_transitionEnd.length = m_transitionStart.length;
  m_transitionTemp.type = TRANSITION_NONE;
  m_fTransitionAngle = 0;
  m_fTransitionZoom = 0;
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
  m_fPosX = m_fPosY = 0.0f;
  m_fPosZ = 1.0f;
  m_fVelocityX = m_fVelocityY = m_fVelocityZ = 0.0f;
  int iFrames = std::max((int)(CServiceBroker::GetWinSystem()->GetGfxContext().GetFPS() * CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_SLIDESHOW_STAYTIME)), 1);
  if (m_displayEffect == EFFECT_PANORAMA)
  {
    RESOLUTION_INFO res = CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo();
    float fScreenWidth  = (float)res.Overscan.right  - res.Overscan.left;
    float fScreenHeight = (float)res.Overscan.bottom - res.Overscan.top;

    if (m_fWidth > m_fHeight)
    {
      iFrames = (int)(iFrames * (m_fWidth - m_fHeight) / m_fHeight);
      m_iTotalFrames = m_transitionStart.length + m_transitionEnd.length + iFrames;

      m_fPosX = 0.5f - (fScreenWidth / fScreenHeight) * (m_fHeight / m_fWidth) * 0.5f;
      if (rand() % 2)
        m_fPosX = -m_fPosX;
      m_fVelocityX = -m_fPosX * 2.0f / m_iTotalFrames;
    }
    else
    {
      iFrames = (int)(iFrames * (m_fHeight - (0.5f * m_fWidth)) / m_fWidth);
      m_iTotalFrames = m_transitionStart.length + m_transitionEnd.length + iFrames;

      m_fPosY = 0.5f - (fScreenHeight / fScreenWidth) * (m_fWidth / m_fHeight) * 0.5f;
      if (rand() % 2)
        m_fPosY = -m_fPosY;
      m_fVelocityY = -m_fPosY * 2.0f / m_iTotalFrames;
    }
  }
  else
  {
    m_iTotalFrames = m_transitionStart.length + m_transitionEnd.length + iFrames;

    if (m_displayEffect == EFFECT_FLOAT)
    {
      // Calculate start and end positions
      // choose a random direction
      float angle = (rand() % 1000) / 1000.0f * 2 * (float)M_PI;
      m_fPosX = cos(angle) * CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_slideshowPanAmount * m_iTotalFrames * 0.00005f;
      m_fPosY = sin(angle) * CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_slideshowPanAmount * m_iTotalFrames * 0.00005f;
      m_fVelocityX = -m_fPosX * 2.0f / m_iTotalFrames;
      m_fVelocityY = -m_fPosY * 2.0f / m_iTotalFrames;
    }
    else if (m_displayEffect == EFFECT_ZOOM)
    {
      m_fPosZ = 1.0f;
      m_fVelocityZ = 0.0001f * CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_slideshowZoomAmount;
    }
  }

  m_transitionEnd.start = m_transitionStart.length + iFrames;

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
    m_pImage = nullptr;
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
    dirtyregions.push_back(CDirtyRegion(GetRectangle(cur_x, cur_y)));
    dirtyregions.push_back(CDirtyRegion(GetRectangle(new_x, new_y)));
    memcpy(cur_x, new_x, count);
    memcpy(cur_y, new_y, count);
  }
}

void CSlideShowPic::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  if (!m_pImage || !m_bIsLoaded || m_bIsFinished) return ;
  UTILS::Color alpha = m_alpha;
  if (m_iCounter <= m_transitionStart.length)
  { // do start transition
    if (m_transitionStart.type == CROSSFADE)
    { // fade in at 1x speed
      alpha = (UTILS::Color)((float)m_iCounter / (float)m_transitionStart.length * 255.0f);
    }
    else if (m_transitionStart.type == FADEIN_FADEOUT)
    { // fade in at 2x speed, then keep solid
      alpha = (UTILS::Color)((float)m_iCounter / (float)m_transitionStart.length * 255.0f * 2);
      if (alpha > 255) alpha = 255;
    }
    else // m_transitionEffect == TRANSITION_NONE
    {
      alpha = 0xFF; // opaque
    }
  }
  bool bPaused = m_bPause | (m_fZoomAmount != 1.0f);
  // check if we're doing a temporary effect (such as rotate + zoom)
  if (m_transitionTemp.type != TRANSITION_NONE)
  {
    bPaused = true;
    if (m_iCounter >= m_transitionTemp.start)
    {
      if (m_iCounter >= m_transitionTemp.start + m_transitionTemp.length)
      { // we're finished this transition
        if (m_transitionTemp.type == TRANSITION_ZOOM)
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
        m_transitionTemp.type = TRANSITION_NONE;
      }
      else
      {
        if (m_transitionTemp.type == TRANSITION_ROTATE)
          m_fAngle += m_fTransitionAngle;
        if (m_transitionTemp.type == TRANSITION_ZOOM)
          m_fZoomAmount += m_fTransitionZoom;
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
      float fMoveAmount = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_slideshowPanAmount * m_iTotalFrames * 0.0001f;
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
/*      if (m_fPosZ > 1.0f + 0.01f*CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt("Slideshow.ZoomAmount"))
      {
        m_fPosZ = 1.0f + 0.01f * CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt("Slideshow.ZoomAmount");
        m_fVelocityZ = -m_fVelocityZ;
      }
      if (m_fPosZ < 1.0f)
      {
        m_fPosZ = 1.0f;
        m_fVelocityZ = -m_fVelocityZ;
      }*/
    }
  }
  if (m_displayEffect != EFFECT_NO_TIMEOUT && bPaused && !m_bTransitionImmediately)
  { // paused - increment the last transition start time
    m_transitionEnd.start++;
  }
  if (m_iCounter >= m_transitionEnd.start)
  { // do end transition
//    CLog::Log(LOGDEBUG,"Transitioning");
    m_bDrawNextImage = true;
    if (m_transitionEnd.type == CROSSFADE)
    { // fade out at 1x speed
      alpha = 255 - (UTILS::Color)((float)(m_iCounter - m_transitionEnd.start) / (float)m_transitionEnd.length * 255.0f);
    }
    else if (m_transitionEnd.type == FADEIN_FADEOUT)
    { // keep solid, then fade out at 2x speed
      alpha = (UTILS::Color)((float)(m_transitionEnd.length - m_iCounter + m_transitionEnd.start) / (float)m_transitionEnd.length * 255.0f * 2);
      if (alpha > 255) alpha = 255;
    }
    else // m_transitionEffect == TRANSITION_NONE
    {
      alpha = 0xFF; // opaque
    }
  }
  if (alpha != m_alpha)
  {
    m_alpha = alpha;
    m_bIsDirty = true;
  }
  if (m_displayEffect != EFFECT_NO_TIMEOUT || m_iCounter < m_transitionStart.length || m_iCounter >= m_transitionEnd.start || (m_iCounter >= m_transitionTemp.start && m_iCounter < m_transitionTemp.start + m_transitionTemp.length))
  {
    /* this really annoying.  there's non-stop logging when viewing a pic outside of the slideshow
    if (m_displayEffect == EFFECT_NO_TIMEOUT)
      CLog::Log(LOGDEBUG, "Incrementing counter (%i) while not in slideshow (startlength=%i,endstart=%i,endlength=%i)", m_iCounter, m_transitionStart.length, m_transitionEnd.start, m_transitionEnd.length);
    */
    m_iCounter++;
  }
  if (m_iCounter > m_transitionEnd.start + m_transitionEnd.length)
    m_bIsFinished = true;

  RESOLUTION_INFO info = CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo();

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
  float fComp = 1.0f + 0.01f * CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_slideshowBlackBarCompensation;
  float fScreenRatio = fScreenWidth / fScreenHeight * fPixelRatio;
  // work out if we should be compensating the zoom to minimize blackbars
  // we should compute this based on the % of black bars on screen perhaps??
  //! @todo change m_displayEffect != EFFECT_NO_TIMEOUT to whether we're running the slideshow
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
    fScale *= (1.0f + CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_slideshowPanAmount * m_iTotalFrames * 0.0001f);
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
      for (float& i : x)
        i += w * m_fZoomLeft;
    }
    if (h >= fScreenHeight)
    { // must have no black bars
      if (miny + m_fZoomTop*h > fOffsetY)
        m_fZoomTop = (fOffsetY - miny) / h;
      if (maxy + m_fZoomTop*h < fOffsetY + fScreenHeight)
        m_fZoomTop = (fScreenHeight + fOffsetY - maxy) / h;
      for (float& i : y)
        i += m_fZoomTop * h;
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
  m_transitionEnd.start = m_iCounter;
}

bool CSlideShowPic::StartTransition()
{
  // this is called if we need to start transitioning immediately to the new picture
  if (m_bDrawNextImage) return false; // don't need to do anything as we are already transitioning
  // decrease the number of display frame
  m_transitionEnd.start = m_iCounter;
  m_bTransitionImmediately = true;
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

int CSlideShowPic::GetTransitionTime(int iType) const
{
  if (iType == 0) // start transition
    return m_transitionStart.length;
  else // iType == 1 // end transition
    return m_transitionEnd.length;
}

void CSlideShowPic::SetTransitionTime(int iType, int iTime)
{
  if (iType == 0) // start transition
    m_transitionStart.length = iTime;
  else // iType == 1 // end transition
    m_transitionEnd.length = iTime;
}

void CSlideShowPic::Rotate(float fRotateAngle, bool immediate /* = false */)
{
  if (m_bDrawNextImage) return;
  if (m_transitionTemp.type == TRANSITION_ZOOM) return;
  if (immediate)
  {
    m_fAngle += fRotateAngle;
    return;
  }

  // if there is a rotation ongoing already
  // add the new angle to the old destination angle
  if (m_transitionTemp.type == TRANSITION_ROTATE &&
      m_transitionTemp.start + m_transitionTemp.length > m_iCounter)
  {
    int remainder = m_transitionTemp.start + m_transitionTemp.length - m_iCounter;
    fRotateAngle += m_fTransitionAngle * remainder;
  }

  m_transitionTemp.type = TRANSITION_ROTATE;
  m_transitionTemp.start = m_iCounter;
  m_transitionTemp.length = IMMEDIATE_TRANSITION_TIME;
  m_fTransitionAngle = fRotateAngle / (float)m_transitionTemp.length;
  // reset the timer
  m_transitionEnd.start = m_iCounter + m_transitionStart.length + (int)(CServiceBroker::GetWinSystem()->GetGfxContext().GetFPS() * CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_SLIDESHOW_STAYTIME));
}

void CSlideShowPic::Zoom(float fZoom, bool immediate /* = false */)
{
  if (m_bDrawNextImage) return;
  if (m_transitionTemp.type == TRANSITION_ROTATE) return;
  if (immediate)
  {
    m_fZoomAmount = fZoom;
    return;
  }
  m_transitionTemp.type = TRANSITION_ZOOM;
  m_transitionTemp.start = m_iCounter;
  m_transitionTemp.length = IMMEDIATE_TRANSITION_TIME;
  m_fTransitionZoom = (fZoom - m_fZoomAmount) / (float)m_transitionTemp.length;
  // reset the timer
  m_transitionEnd.start = m_iCounter + m_transitionStart.length + (int)(CServiceBroker::GetWinSystem()->GetGfxContext().GetFPS() * CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_SLIDESHOW_STAYTIME));
  // turn off the render effects until we're back down to normal zoom
  m_bNoEffect = true;
}

void CSlideShowPic::Move(float fDeltaX, float fDeltaY)
{
  m_fZoomLeft += fDeltaX;
  m_fZoomTop += fDeltaY;
  // reset the timer
 // m_transitionEnd.start = m_iCounter + m_transitionStart.length + (int)(CServiceBroker::GetWinSystem()->GetGfxContext().GetFPS() * CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_SLIDESHOW_STAYTIME));
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

#ifdef HAS_DX
bool CSlideShowPic::UpdateVertexBuffer(Vertex* vertices)
{
  if (!m_vb) // create new
  {
    CD3D11_BUFFER_DESC desc(sizeof(Vertex) * 5, D3D11_BIND_VERTEX_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = vertices;
    initData.SysMemPitch = sizeof(Vertex) * 5;
    if (SUCCEEDED(DX::DeviceResources::Get()->GetD3DDevice()->CreateBuffer(&desc, &initData, m_vb.ReleaseAndGetAddressOf())))
      return true;
  }
  else // update
  {
    ID3D11DeviceContext* pContext = DX::DeviceResources::Get()->GetD3DContext();
    D3D11_MAPPED_SUBRESOURCE res;
    if (SUCCEEDED(pContext->Map(m_vb.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &res)))
    {
      memcpy(res.pData, vertices, sizeof(Vertex) * 5);
      pContext->Unmap(m_vb.Get(), 0);
      return true;
    }
  }

  return false;
}
#endif

void CSlideShowPic::Render(float *x, float *y, CBaseTexture* pTexture, UTILS::Color color)
{
#ifdef HAS_DX
  Vertex vertex[5];
  for (int i = 0; i < 4; i++)
  {
    vertex[i].pos = XMFLOAT3( x[i], y[i], 0);
    CD3DHelper::XMStoreColor(&vertex[i].color, color);
    vertex[i].texCoord = XMFLOAT2(0.0f, 0.0f);
    vertex[i].texCoord2 = XMFLOAT2(0.0f, 0.0f);
  }

  if (pTexture)
  {
    vertex[1].texCoord.x = vertex[2].texCoord.x = (float) pTexture->GetWidth() / pTexture->GetTextureWidth();
    vertex[2].texCoord.y = vertex[3].texCoord.y = (float) pTexture->GetHeight() / pTexture->GetTextureHeight();
  }
  else
  {
    vertex[1].texCoord.x = vertex[2].texCoord.x = 1.0f;
    vertex[2].texCoord.y = vertex[3].texCoord.y = 1.0f;
  }
  vertex[4] = vertex[0]; // Not used when pTexture != NULL

  CGUIShaderDX* pGUIShader = DX::Windowing()->GetGUIShader();
  pGUIShader->Begin(SHADER_METHOD_RENDER_TEXTURE_BLEND);

  // Set state to render the image
  if (pTexture)
  {
    pTexture->LoadToGPU();
    CDXTexture* dxTexture = reinterpret_cast<CDXTexture*>(pTexture);
    ID3D11ShaderResourceView* shaderRes = dxTexture->GetShaderResource();
    pGUIShader->SetShaderViews(1, &shaderRes);
    pGUIShader->DrawQuad(vertex[0], vertex[1], vertex[2], vertex[3]);
  }
  else
  {
    if (!UpdateVertexBuffer(vertex))
      return;

    ComPtr<ID3D11DeviceContext> pContext = DX::DeviceResources::Get()->GetD3DContext();

    unsigned stride = sizeof(Vertex);
    unsigned offset = 0;
    pContext->IASetVertexBuffers(0, 1, m_vb.GetAddressOf(), &stride, &offset);
    pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);

    pGUIShader->Draw(5, 0);
    pGUIShader->RestoreBuffers();
  }

#elif defined(HAS_GL)
  CRenderSystemGL *renderSystem = dynamic_cast<CRenderSystemGL*>(CServiceBroker::GetRenderSystem());
  if (pTexture)
  {
    pTexture->LoadToGPU();
    pTexture->BindToUnit(0);

    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    renderSystem->EnableShader(SM_TEXTURE);
  }
  else
  {
    renderSystem->EnableShader(SM_DEFAULT);
  }

  float u1 = 0, u2 = 1, v1 = 0, v2 = 1;
  if (pTexture)
  {
    u2 = (float)pTexture->GetWidth() / pTexture->GetTextureWidth();
    v2 = (float)pTexture->GetHeight() / pTexture->GetTextureHeight();
  }

  GLubyte colour[4];
  GLubyte idx[4] = {0, 1, 3, 2};  //determines order of the vertices
  GLuint vertexVBO;
  GLuint indexVBO;
  struct PackedVertex
  {
    float x, y, z;
    float u1, v1;
  } vertex[4];

  // Setup vertex position values
  vertex[0].x = x[0];
  vertex[0].y = y[0];
  vertex[0].z = 0;
  vertex[0].u1 = u1;
  vertex[0].v1 = v1;

  vertex[1].x = x[1];
  vertex[1].y = y[1];
  vertex[1].z = 0;
  vertex[1].u1 = u2;
  vertex[1].v1 = v1;

  vertex[2].x = x[2];
  vertex[2].y = y[2];
  vertex[2].z = 0;
  vertex[2].u1 = u2;
  vertex[2].v1 = v2;

  vertex[3].x = x[3];
  vertex[3].y = y[3];
  vertex[3].z = 0;
  vertex[3].u1 = u1;
  vertex[3].v1 = v2;

  GLint posLoc  = renderSystem->ShaderGetPos();
  GLint tex0Loc = renderSystem->ShaderGetCoord0();
  GLint uniColLoc= renderSystem->ShaderGetUniCol();

  glGenBuffers(1, &vertexVBO);
  glBindBuffer(GL_ARRAY_BUFFER, vertexVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(PackedVertex)*4, &vertex[0], GL_STATIC_DRAW);

  glVertexAttribPointer(posLoc, 3, GL_FLOAT, 0, sizeof(PackedVertex), BUFFER_OFFSET(offsetof(PackedVertex, x)));
  glVertexAttribPointer(tex0Loc, 2, GL_FLOAT, 0, sizeof(PackedVertex), BUFFER_OFFSET(offsetof(PackedVertex, u1)));

  glEnableVertexAttribArray(posLoc);
  glEnableVertexAttribArray(tex0Loc);

  // Setup Colour values
  colour[0] = (GLubyte)GET_R(color);
  colour[1] = (GLubyte)GET_G(color);
  colour[2] = (GLubyte)GET_B(color);
  colour[3] = (GLubyte)GET_A(color);

  glUniform4f(uniColLoc,(colour[0] / 255.0f), (colour[1] / 255.0f),
                        (colour[2] / 255.0f), (colour[3] / 255.0f));

  glGenBuffers(1, &indexVBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexVBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLubyte)*4, idx, GL_STATIC_DRAW);

  glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_BYTE, 0);

  glDisableVertexAttribArray(posLoc);
  glDisableVertexAttribArray(tex0Loc);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glDeleteBuffers(1, &vertexVBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  glDeleteBuffers(1, &indexVBO);

  renderSystem->DisableShader();

#elif defined(HAS_GLES)
  CRenderSystemGLES *renderSystem = dynamic_cast<CRenderSystemGLES*>(CServiceBroker::GetRenderSystem());
  if (pTexture)
  {
    pTexture->LoadToGPU();
    pTexture->BindToUnit(0);

    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);          // Turn Blending On

    renderSystem->EnableGUIShader(SM_TEXTURE);
  }
  else
  {
    renderSystem->EnableGUIShader(SM_DEFAULT);
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

  GLint posLoc  = renderSystem->GUIShaderGetPos();
  GLint tex0Loc = renderSystem->GUIShaderGetCoord0();
  GLint uniColLoc= renderSystem->GUIShaderGetUniCol();

  glVertexAttribPointer(posLoc,  3, GL_FLOAT, 0, 0, ver);
  glVertexAttribPointer(tex0Loc, 2, GL_FLOAT, 0, 0, tex);

  glEnableVertexAttribArray(posLoc);
  glEnableVertexAttribArray(tex0Loc);

  // Setup Colour values
  col[0] = (GLubyte)GET_R(color);
  col[1] = (GLubyte)GET_G(color);
  col[2] = (GLubyte)GET_B(color);
  col[3] = (GLubyte)GET_A(color);

  if (CServiceBroker::GetWinSystem()->UseLimitedColor())
  {
    col[0] = (235 - 16) * col[0] / 255 + 16;
    col[1] = (235 - 16) * col[1] / 255 + 16;
    col[2] = (235 - 16) * col[2] / 255 + 16;
  }

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

  renderSystem->DisableGUIShader();

#endif
}
