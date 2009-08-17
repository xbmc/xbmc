/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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

#include "include.h"
#include "GraphicContext.h"
#include "GUIFontManager.h"
#include "GUIMessage.h"
#include "IMsgSenderCallback.h"
#include "Settings.h"
#include "GUISettings.h"
#include "XBVideoConfig.h"
#include "TextureManager.h"
#include "../xbmc/utils/SingleLock.h"
#include "../xbmc/Application.h"
#include "cores/VideoRenderers/RenderManager.h"
#include "GUIAudioManager.h"

#define D3D_CLEAR_STENCIL 0x0l

#include "SurfaceGL.h"
#include "SkinInfo.h"
using namespace Surface;
#ifdef HAS_GLX
#include <X11/extensions/Xinerama.h>
#elif defined (__APPLE__)
#include "CocoaInterface.h"
#endif
#ifdef HAS_XRANDR
#include "XRandR.h"
#endif

#ifdef HAVE_LIBVDPAU
#include "cores/dvdplayer/DVDCodecs/Video/VDPAU.h"
#endif


using namespace std;

extern bool g_fullScreen;

/* quick access to a skin setting, fine unless we starts clearing video settings */
static CSettingInt* g_guiSkinzoom = NULL;

CGraphicContextBase::CGraphicContextBase(void)
{
  m_iScreenWidth = 720;
  m_iScreenHeight = 576;
  m_screenSurface = NULL;

  m_dwID = 0;
  m_strMediaDir = "";
  m_bCalibrating = false;
  m_Resolution = INVALID;
  m_pCallback = NULL;
  m_guiScaleX = m_guiScaleY = 1.0f;
  m_windowResolution = INVALID;
  m_bFullScreenRoot = false;

  s_RenderVendor = "";
  s_RenderRenderer = "";
  s_RenderxExt = "";
  s_RenderMajVer = 0;
  s_RenderMinVer = 0;
}

CGraphicContextBase::~CGraphicContextBase(void)
{
  while (m_viewStack.size())
  {
    m_viewStack.pop();
  }

  // do not delete m_screenSurface, SDL is already gone and we will
  // crash on exit in Surface::CSurface::~CSurface at SDL_FreeSurface(m_SDLSurface)
  //delete m_screenSurface;
}

std::string CGraphicContextBase::GetRenderVendor()
{
  if(s_RenderVendor.length() == 0)
  {
    int a,b;
    GetRenderVersion(a, b);
  }

  return s_RenderVendor;
}
std::string CGraphicContextBase::GetRenderRenderer()
{
  if(s_RenderRenderer.length() == 0)
  {
    int a,b;
    GetRenderVersion(a, b);
  }

  return s_RenderRenderer;
}

bool CGraphicContextBase::SendMessage(DWORD message, DWORD senderID, DWORD destID, DWORD param1, DWORD param2)
{
  if (!m_pCallback) return false;
  CGUIMessage msg(message, senderID, destID, param1, param2);
  return m_pCallback->SendMessage(msg);
}

bool CGraphicContextBase::SendMessage(CGUIMessage& message)
{
  if (!m_pCallback) return false;
  return m_pCallback->SendMessage(message);
}

void CGraphicContextBase::setMessageSender(IMsgSenderCallback* pCallback)
{
  m_pCallback = pCallback;
}

DWORD CGraphicContextBase::GetNewID()
{
  m_dwID++;
  return m_dwID;
}

void CGraphicContextBase::SetOrigin(float x, float y)
{
  if (m_origins.size())
    m_origins.push(CPoint(x,y) + m_origins.top());
  else
    m_origins.push(CPoint(x,y));
  AddTransform(TransformMatrix::CreateTranslation(x, y));
}

void CGraphicContextBase::RestoreOrigin()
{
  m_origins.pop();
  RemoveTransform();
}

// add a new clip region, intersecting with the previous clip region.
bool CGraphicContextBase::SetClipRegion(float x, float y, float w, float h)
{ // transform from our origin
  CPoint origin;
  if (m_origins.size())
    origin = m_origins.top();
  // ok, now intersect with our old clip region
  CRect rect(x, y, x + w, y + h);
  rect += origin;
  if (m_clipRegions.size())
  { // intersect with original clip region
    rect.Intersect(m_clipRegions.top());
  }
  if (rect.IsEmpty())
    return false;
  m_clipRegions.push(rect);

  // here we could set the hardware clipping, if applicable
  return true;
}

void CGraphicContextBase::RestoreClipRegion()
{
  if (m_clipRegions.size())
    m_clipRegions.pop();

  // here we could reset the hardware clipping, if applicable
}

void CGraphicContextBase::ClipRect(CRect &vertex, CRect &texture, CRect *texture2)
{
  // this is the software clipping routine.  If the graphics hardware is set to do the clipping
  // (eg via SetClipPlane in D3D for instance) then this routine is unneeded.
  if (m_clipRegions.size())
  {
    // take a copy of the vertex rectangle and intersect
    // it with our clip region (moved to the same coordinate system)
    CRect clipRegion(m_clipRegions.top());
    if (m_origins.size())
      clipRegion -= m_origins.top();
    CRect original(vertex);
    vertex.Intersect(clipRegion);
    // and use the original to compute the texture coordinates
    if (original != vertex)
    {
      float scaleX = texture.Width() / original.Width();
      float scaleY = texture.Height() / original.Height();
      texture.x1 += (vertex.x1 - original.x1) * scaleX;
      texture.y1 += (vertex.y1 - original.y1) * scaleY;
      texture.x2 += (vertex.x2 - original.x2) * scaleX;
      texture.y2 += (vertex.y2 - original.y2) * scaleY;
      if (texture2)
      {
        scaleX = texture2->Width() / original.Width();
        scaleY = texture2->Height() / original.Height();
        texture2->x1 += (vertex.x1 - original.x1) * scaleX;
        texture2->y1 += (vertex.y1 - original.y1) * scaleY;
        texture2->x2 += (vertex.x2 - original.x2) * scaleX;
        texture2->y2 += (vertex.y2 - original.y2) * scaleY;
      }
    }
  }
}

bool CGraphicContextBase::SetViewPort(float fx, float fy , float fwidth, float fheight, bool intersectPrevious /* = false */)
{
  CRect newviewport;
  CRect oldviewport;

  oldviewport = GetRenderViewPort();

  // transform coordinates - we may have a rotation which changes the positioning of the
  // minimal and maximal viewport extents.  We currently go to the maximal extent.
  float x[4], y[4];
  x[0] = x[3] = fx;
  x[1] = x[2] = fx + fwidth;
  y[0] = y[1] = fy;
  y[2] = y[3] = fy + fheight;
  float minX = (float)m_iScreenWidth;
  float maxX = 0;
  float minY = (float)m_iScreenHeight;
  float maxY = 0;
  for (int i = 0; i < 4; i++)
  {
    float z = 0;
    ScaleFinalCoords(x[i], y[i], z);
    if (x[i] < minX) minX = x[i];
    if (x[i] > maxX) maxX = x[i];
    if (y[i] < minY) minY = y[i];
    if (y[i] > maxY) maxY = y[i];
  }

  newviewport.x1 = (minX + 0.5f);
  newviewport.y1 = (minY + 0.5f);
  newviewport.x2 = (maxX + 0.5f);
  newviewport.y2 = (maxY + 0.5f);

  int newLeft = (int)newviewport.x1;
  int newRight = (int)newviewport.x2;
  int newTop = (int)newviewport.y1;
  int newBottom = (int)newviewport.y2;

  int oldLeft = (int)oldviewport.x1;
  int oldRight = (int)oldviewport.x2;
  int oldTop = (int)oldviewport.y1;
  int oldBottom = (int)oldviewport.y2;
  if (intersectPrevious)
  {
    if (newLeft >= oldRight || newTop >= oldBottom || newRight <= oldLeft || newBottom <= oldTop)
    { // empty intersection - return false to indicate no rendering should occur
      return false;
    }
    // ok, they intersect, do the intersection
    if (newLeft < oldLeft) newLeft = oldLeft;
    if (newTop < oldTop) newTop = oldTop;
    if (newRight > oldRight) newRight = oldRight;
    if (newBottom > oldBottom) newBottom = oldBottom;
  }
  // check range against screen size
  if (newRight <= 0 || newBottom <= 0 ||
      newTop >= m_iScreenHeight || newLeft >= m_iScreenWidth ||
      newLeft >= newRight || newTop >= newBottom)
  { // no intersection with the screen

    return false;
  }
  // intersection with the screen
  if (newLeft < 0) newLeft = 0;
  if (newTop < 0) newTop = 0;
  if (newRight > m_iScreenWidth) newRight = m_iScreenWidth;
  if (newBottom > m_iScreenHeight) newBottom = m_iScreenHeight;

  ASSERT(newLeft < newRight);
  ASSERT(newTop < newBottom);

  SetRendrViewPort(newviewport);
  m_viewStack.push(oldviewport);

  UpdateCameraPosition(m_cameras.top());
  return true;
}

void CGraphicContextBase::RestoreViewPort()
{
  if (!m_viewStack.size()) 
    return;

  CRect oldviewport = m_viewStack.top();
  SetRendrViewPort(oldviewport);

  m_viewStack.pop();

  UpdateCameraPosition(m_cameras.top());
}

const RECT CGraphicContextBase::GetViewWindow() const
{
  return m_videoRect;
}
void CGraphicContextBase::SetViewWindow(float left, float top, float right, float bottom)
{
  if (m_bCalibrating)
  {
    SetFullScreenViewWindow(m_Resolution);
  }
  else
  {
    m_videoRect.left = (long)(ScaleFinalXCoord(left, top) + 0.5f);
    m_videoRect.top = (long)(ScaleFinalYCoord(left, top) + 0.5f);
    m_videoRect.right = (long)(ScaleFinalXCoord(right, bottom) + 0.5f);
    m_videoRect.bottom = (long)(ScaleFinalYCoord(right, bottom) + 0.5f);
  }
}

void CGraphicContextBase::ClipToViewWindow()
{

}

void CGraphicContextBase::SetFullScreenViewWindow(RESOLUTION &res)
{
  m_videoRect.left = g_settings.m_ResInfo[res].Overscan.left;
  m_videoRect.top = g_settings.m_ResInfo[res].Overscan.top;
  m_videoRect.right = g_settings.m_ResInfo[res].Overscan.right;
  m_videoRect.bottom = g_settings.m_ResInfo[res].Overscan.bottom;
}

void CGraphicContextBase::SetFullScreenVideo(bool bOnOff)
{
  Lock();
  m_bFullScreenVideo = bOnOff;
  SetFullScreenViewWindow(m_Resolution);
  Unlock();
}

bool CGraphicContextBase::IsFullScreenVideo() const
{
  return m_bFullScreenVideo;
}

bool CGraphicContextBase::IsCalibrating() const
{
  return m_bCalibrating;
}

void CGraphicContextBase::SetCalibrating(bool bOnOff)
{
  m_bCalibrating = bOnOff;
}

bool CGraphicContextBase::IsValidResolution(RESOLUTION res)
{
  return g_videoConfig.IsValidResolution(res);
}

void CGraphicContextBase::GetAllowedResolutions(vector<RESOLUTION> &res, bool bAllowPAL60)
{
  bool bCanDoWidescreen = g_videoConfig.HasWidescreen();
  res.clear();
  if (g_videoConfig.HasPAL())
  {
    res.push_back(PAL_4x3);
    if (bCanDoWidescreen) res.push_back(PAL_16x9);
    if (bAllowPAL60 && g_videoConfig.HasPAL60())
    {
      res.push_back(PAL60_4x3);
      if (bCanDoWidescreen) res.push_back(PAL60_16x9);
    }
  }
  if (g_videoConfig.HasNTSC())
  {
    res.push_back(NTSC_4x3);
    if (bCanDoWidescreen) res.push_back(NTSC_16x9);
    if (g_videoConfig.Has480p())
    {
      res.push_back(HDTV_480p_4x3);
      if (bCanDoWidescreen) res.push_back(HDTV_480p_16x9);
    }
    if (g_videoConfig.Has720p())
      res.push_back(HDTV_720p);
    if (g_videoConfig.Has1080i())
      res.push_back(HDTV_1080i);
  }

  if (g_videoConfig.GetNumberOfResolutions())
  {
    res.push_back(CUSTOM);
  }
  res.push_back(DESKTOP);
}

RESOLUTION CGraphicContextBase::GetVideoResolution() const
{
  return m_Resolution;
}

void CGraphicContextBase::ResetOverscan(RESOLUTION_INFO &res)
{
  res.Overscan.left = 0;
  res.Overscan.top = 0;
  res.Overscan.right = res.iWidth;
  res.Overscan.bottom = res.iHeight;
}

void CGraphicContextBase::ResetOverscan(RESOLUTION res, OVERSCAN &overscan)
{
  overscan.left = 0;
  overscan.top = 0;
  switch (res)
  {
  case HDTV_1080i:
    overscan.right = 1920;
    overscan.bottom = 1080;
    break;
  case HDTV_720p:
    overscan.right = 1280;
    overscan.bottom = 720;
    break;
  case HDTV_480p_16x9:
  case HDTV_480p_4x3:
  case NTSC_16x9:
  case NTSC_4x3:
  case PAL60_16x9:
  case PAL60_4x3:
    overscan.right = 720;
    overscan.bottom = 480;
    break;
  case PAL_16x9:
  case PAL_4x3:
    overscan.right = 720;
    overscan.bottom = 576;
    break;
  default:
    overscan.right = g_settings.m_ResInfo[res].iWidth;
    overscan.bottom = g_settings.m_ResInfo[res].iHeight;
    break;
  }
}

void CGraphicContextBase::ResetScreenParameters(RESOLUTION res)
{
  // For now these are all on the first screen.
  g_settings.m_ResInfo[res].iScreen = 0;
  static const float fOptimalSwitchPoint = 8.0f / (3.0f*sqrt(3.0f)); // see XboxRenderer.cpp

  // 1080i
  switch (res)
  {
  case HDTV_1080i:
    g_settings.m_ResInfo[res].iSubtitles = (int)(0.965 * 1080);
    g_settings.m_ResInfo[res].iWidth = 1920;
    g_settings.m_ResInfo[res].iHeight = 1080;
    g_settings.m_ResInfo[res].dwFlags = D3DPRESENTFLAG_INTERLACED | D3DPRESENTFLAG_WIDESCREEN;
    g_settings.m_ResInfo[res].fPixelRatio = 1.0f;
    strcpy(g_settings.m_ResInfo[res].strMode, "1080i 16:9");
    break;
  case HDTV_720p:
    g_settings.m_ResInfo[res].iSubtitles = (int)(0.965 * 720);
    g_settings.m_ResInfo[res].iWidth = 1280;
    g_settings.m_ResInfo[res].iHeight = 720;
    g_settings.m_ResInfo[res].dwFlags = D3DPRESENTFLAG_PROGRESSIVE | D3DPRESENTFLAG_WIDESCREEN;
    g_settings.m_ResInfo[res].fPixelRatio = 1.0f;
    strcpy(g_settings.m_ResInfo[res].strMode, "720p 16:9");
    break;
  case HDTV_480p_4x3:
    g_settings.m_ResInfo[res].iSubtitles = (int)(0.9 * 480);
    g_settings.m_ResInfo[res].iWidth = 720;
    g_settings.m_ResInfo[res].iHeight = 480;
    g_settings.m_ResInfo[res].dwFlags = D3DPRESENTFLAG_PROGRESSIVE;
    g_settings.m_ResInfo[res].fPixelRatio = 4320.0f / 4739.0f;
    strcpy(g_settings.m_ResInfo[res].strMode, "480p 4:3");
    break;
  case HDTV_480p_16x9:
    g_settings.m_ResInfo[res].iSubtitles = (int)(0.965 * 480);
    g_settings.m_ResInfo[res].iWidth = 720;
    g_settings.m_ResInfo[res].iHeight = 480;
    g_settings.m_ResInfo[res].dwFlags = D3DPRESENTFLAG_PROGRESSIVE | D3DPRESENTFLAG_WIDESCREEN;
    g_settings.m_ResInfo[res].fPixelRatio = 4320.0f / 4739.0f*4.0f / 3.0f;
    strcpy(g_settings.m_ResInfo[res].strMode, "480p 16:9");
    break;
  case NTSC_4x3:
    g_settings.m_ResInfo[res].iSubtitles = (int)(0.9 * 480);
    g_settings.m_ResInfo[res].iWidth = 720;
    g_settings.m_ResInfo[res].iHeight = 480;
    g_settings.m_ResInfo[res].dwFlags = D3DPRESENTFLAG_INTERLACED;
    g_settings.m_ResInfo[res].fPixelRatio = 4320.0f / 4739.0f;
    strcpy(g_settings.m_ResInfo[res].strMode, "NTSC 4:3");
    break;
  case NTSC_16x9:
    g_settings.m_ResInfo[res].iSubtitles = (int)(0.965 * 480);
    g_settings.m_ResInfo[res].iWidth = 720;
    g_settings.m_ResInfo[res].iHeight = 480;
    g_settings.m_ResInfo[res].dwFlags = D3DPRESENTFLAG_INTERLACED | D3DPRESENTFLAG_WIDESCREEN;
    g_settings.m_ResInfo[res].fPixelRatio = 4320.0f / 4739.0f*4.0f / 3.0f;
    strcpy(g_settings.m_ResInfo[res].strMode, "NTSC 16:9");
    break;
  case PAL_4x3:
    g_settings.m_ResInfo[res].iSubtitles = (int)(0.9 * 576);
    g_settings.m_ResInfo[res].iWidth = 720;
    g_settings.m_ResInfo[res].iHeight = 576;
    g_settings.m_ResInfo[res].dwFlags = D3DPRESENTFLAG_INTERLACED;
    g_settings.m_ResInfo[res].fPixelRatio = 128.0f / 117.0f;
    strcpy(g_settings.m_ResInfo[res].strMode, "PAL 4:3");
    break;
  case PAL_16x9:
    g_settings.m_ResInfo[res].iSubtitles = (int)(0.965 * 576);
    g_settings.m_ResInfo[res].iWidth = 720;
    g_settings.m_ResInfo[res].iHeight = 576;
    g_settings.m_ResInfo[res].dwFlags = D3DPRESENTFLAG_INTERLACED | D3DPRESENTFLAG_WIDESCREEN;
    g_settings.m_ResInfo[res].fPixelRatio = 128.0f / 117.0f*4.0f / 3.0f;
    strcpy(g_settings.m_ResInfo[res].strMode, "PAL 16:9");
    break;
  case PAL60_4x3:
    g_settings.m_ResInfo[res].iSubtitles = (int)(0.9 * 480);
    g_settings.m_ResInfo[res].iWidth = 720;
    g_settings.m_ResInfo[res].iHeight = 480;
    g_settings.m_ResInfo[res].dwFlags = D3DPRESENTFLAG_INTERLACED;
    g_settings.m_ResInfo[res].fPixelRatio = 4320.0f / 4739.0f;
    strcpy(g_settings.m_ResInfo[res].strMode, "PAL60 4:3");
    break;
  case PAL60_16x9:
    g_settings.m_ResInfo[res].iSubtitles = (int)(0.965 * 480);
    g_settings.m_ResInfo[res].iWidth = 720;
    g_settings.m_ResInfo[res].iHeight = 480;
    g_settings.m_ResInfo[res].dwFlags = D3DPRESENTFLAG_INTERLACED | D3DPRESENTFLAG_WIDESCREEN;
    g_settings.m_ResInfo[res].fPixelRatio = 4320.0f / 4739.0f*4.0f / 3.0f;
    strcpy(g_settings.m_ResInfo[res].strMode, "PAL60 16:9");
    break;
  case DESKTOP:
    g_videoConfig.GetCurrentResolution(g_settings.m_ResInfo[res]);
    g_settings.m_ResInfo[res].iSubtitles = (int)(0.965 * g_settings.m_ResInfo[res].iHeight);
    if(g_settings.m_ResInfo[res].fRefreshRate)
      snprintf(g_settings.m_ResInfo[res].strMode
            , sizeof(g_settings.m_ResInfo[res].strMode)
            , "%dx%d @ %.2fHz (Full Screen)"
            , g_settings.m_ResInfo[res].iWidth
            , g_settings.m_ResInfo[res].iHeight
            , g_settings.m_ResInfo[res].fRefreshRate);
    else
      snprintf(g_settings.m_ResInfo[res].strMode
            , sizeof(g_settings.m_ResInfo[res].strMode)
            , "%dx%d (Full Screen)"
            , g_settings.m_ResInfo[res].iWidth
            , g_settings.m_ResInfo[res].iHeight);
    if ((float)g_settings.m_ResInfo[res].iWidth/(float)g_settings.m_ResInfo[res].iHeight >= fOptimalSwitchPoint)
      g_settings.m_ResInfo[res].dwFlags = D3DPRESENTFLAG_WIDESCREEN;
    g_settings.m_ResInfo[res].fPixelRatio = 1.0f;
    break;
  case WINDOW:
    {
      RESOLUTION_INFO info = {};
      g_videoConfig.GetCurrentResolution(info);
      g_settings.m_ResInfo[res] = g_settings.m_ResInfo[PAL60_4x3];
      g_settings.m_ResInfo[res].fPixelRatio = 1.0f;
      g_settings.m_ResInfo[res].fRefreshRate = info.fRefreshRate;
    }
    break;
  default:
    break;
  }
  ResetOverscan(res, g_settings.m_ResInfo[res].Overscan);
}

float CGraphicContextBase::GetPixelRatio(RESOLUTION iRes) const
{
  // TODO: Why do we set a pixel ratio of 1.0 if the
  //       user is not running fullscreen?
  if (!m_bFullScreenRoot)
    return 1.0f;

  return g_settings.m_ResInfo[iRes].fPixelRatio;
}

void CGraphicContextBase::SetScalingResolution(RESOLUTION res, float posX, float posY, bool needsScaling)
{
  Lock();
  m_windowResolution = res;
  if (needsScaling && m_Resolution != INVALID)
  {
    // calculate necessary scalings
    float fFromWidth;
    float fFromHeight;
    float fToPosX;
    float fToPosY;
    float fToWidth;
    float fToHeight;

    {
      fFromWidth = (float)g_settings.m_ResInfo[res].iWidth;
      fFromHeight = (float)g_settings.m_ResInfo[res].iHeight;
      fToPosX = (float)g_settings.m_ResInfo[m_Resolution].Overscan.left;
      fToPosY = (float)g_settings.m_ResInfo[m_Resolution].Overscan.top;
      fToWidth = (float)g_settings.m_ResInfo[m_Resolution].Overscan.right - fToPosX;
      fToHeight = (float)g_settings.m_ResInfo[m_Resolution].Overscan.bottom - fToPosY;
    }

    // add additional zoom to compensate for any overskan built in skin
    float fZoom = g_SkinInfo.GetSkinZoom();

    if(!g_guiSkinzoom) // lookup gui setting if we didn't have it already
      g_guiSkinzoom = (CSettingInt*)g_guiSettings.GetSetting("lookandfeel.skinzoom");

    if(g_guiSkinzoom)
      fZoom *= (100 + g_guiSkinzoom->GetData()) * 0.01f;

    fZoom -= 1.0f;
    fToPosX -= fToWidth * fZoom * 0.5f;
    fToWidth *= fZoom + 1.0f;

    // adjust for aspect ratio as zoom is given in the vertical direction and we don't
    // do aspect ratio corrections in the gui code
    fZoom = fZoom / g_settings.m_ResInfo[m_Resolution].fPixelRatio;
    fToPosY -= fToHeight * fZoom * 0.5f;
    fToHeight *= fZoom + 1.0f;

    m_guiScaleX = fFromWidth / fToWidth;
    m_guiScaleY = fFromHeight / fToHeight;
    TransformMatrix windowOffset = TransformMatrix::CreateTranslation(posX, posY);
    TransformMatrix guiScaler = TransformMatrix::CreateScaler(fToWidth / fFromWidth, fToHeight / fFromHeight, fToHeight / fFromHeight);
    TransformMatrix guiOffset = TransformMatrix::CreateTranslation(fToPosX, fToPosY);
    m_guiTransform = guiOffset * guiScaler * windowOffset;
  }
  else
  {
    m_guiTransform = TransformMatrix::CreateTranslation(posX, posY);
    m_guiScaleX = 1.0f;
    m_guiScaleY = 1.0f;
  }
  // reset our origin and camera
  while (m_origins.size())
    m_origins.pop();
  m_origins.push(CPoint(posX, posY));
  while (m_cameras.size())
    m_cameras.pop();
  m_cameras.push(CPoint(0.5f*m_iScreenWidth, 0.5f*m_iScreenHeight));

  // and reset the final transform
  UpdateFinalTransform(m_guiTransform);
  Unlock();
}

void CGraphicContextBase::SetRenderingResolution(RESOLUTION res, float posX, float posY, bool needsScaling)
{
  Lock();
  SetScalingResolution(res, posX, posY, needsScaling);
  UpdateCameraPosition(m_cameras.top());
  Unlock();
}

void CGraphicContextBase::UpdateFinalTransform(const TransformMatrix &matrix)
{
  m_finalTransform = matrix;
  // We could set the world transform here to GPU-ize the animation system.
  // trouble is that we require the resulting x,y coords to be rounded to
  // the nearest pixel (vertex shader perhaps?)
}

void CGraphicContextBase::InvertFinalCoords(float &x, float &y) const
{
  m_finalTransform.InverseTransformPosition(x, y);
}

float CGraphicContextBase::GetScalingPixelRatio() const
{
  if (m_Resolution == m_windowResolution)
    return GetPixelRatio(m_windowResolution);

  RESOLUTION checkRes = m_windowResolution;
  if (checkRes == INVALID)
    checkRes = m_Resolution;
  // resolutions are different - we want to return the aspect ratio of the video resolution
  // but only once it's been corrected for the skin -> screen coordinates scaling
  float winWidth = (float)g_settings.m_ResInfo[checkRes].iWidth;
  float winHeight = (float)g_settings.m_ResInfo[checkRes].iHeight;
  float outWidth = (float)g_settings.m_ResInfo[m_Resolution].iWidth;
  float outHeight = (float)g_settings.m_ResInfo[m_Resolution].iHeight;
  float outPR = GetPixelRatio(m_Resolution);

  return outPR * (outWidth / outHeight) / (winWidth / winHeight);
}

void CGraphicContextBase::SetCameraPosition(const CPoint &camera)
{
  // offset the camera from our current location (this is in XML coordinates) and scale it up to
  // the screen resolution
  /* elis
  CPoint cam(camera);
  if (m_origins.size())
    cam += m_origins.top();

  RESOLUTION windowRes = (m_windowResolution == INVALID) ? m_Resolution : m_windowResolution;
  cam.x *= (float)m_iScreenWidth / g_settings.m_ResInfo[windowRes].iWidth;
  cam.y *= (float)m_iScreenHeight / g_settings.m_ResInfo[windowRes].iHeight;

  m_cameras.push(cam);
  UpdateCameraPosition(m_cameras.top());
  */
}

void CGraphicContextBase::RestoreCameraPosition()
{ // remove the top camera from the stack
  ASSERT(m_cameras.size());
  m_cameras.pop();
  UpdateCameraPosition(m_cameras.top());
}

bool CGraphicContextBase::RectIsAngled(float x1, float y1, float x2, float y2) const
{ // need only test 3 points, as they must be co-planer
  if (m_finalTransform.TransformZCoord(x1, y1, 0)) return true;
  if (m_finalTransform.TransformZCoord(x2, y2, 0)) return true;
  if (m_finalTransform.TransformZCoord(x1, y2, 0)) return true;
  return false;
}

float CGraphicContextBase::GetFPS() const
{
  // eli
  return 60.0f;


  if (g_settings.m_ResInfo[m_Resolution].fRefreshRate > 0)
    return g_settings.m_ResInfo[m_Resolution].fRefreshRate;
  if (m_Resolution == PAL_4x3 || m_Resolution == PAL_16x9)
    return 50.0f;
  if (m_Resolution == HDTV_1080i)
    return 30.0f;
  return 60.0f;
}

bool CGraphicContextBase::IsFullScreenRoot () const
{
  return m_bFullScreenRoot;
}

bool CGraphicContextBase::ToggleFullScreenRoot ()
{
#ifndef _WIN32PC
  // FIXME: see if #5256 works also for Linux and Mac
  static RESOLUTION desktopres = DESKTOP;
  static RESOLUTION windowres = WINDOW;
  static RESOLUTION lastres = INVALID;
  if (m_bFullScreenRoot)
  {
    lastres = GetVideoResolution();
    SetVideoResolution(windowres);
  }
  else
  {
    if (lastres != INVALID)
    {
      SetVideoResolution(lastres);
    }
    else
    {
      SetVideoResolution(desktopres);
    }
  }
  return  m_bFullScreenRoot;
#else
  if (m_bFullScreenRoot)
  {
    g_advancedSettings.m_fullScreen = false;
    SetVideoResolution(m_Resolution);
  }
  else
  {
    g_advancedSettings.m_fullScreen = true;
    SetVideoResolution(m_Resolution);
  }
  return  m_bFullScreenRoot;
#endif
}

void CGraphicContextBase::SetMediaDir(const CStdString &strMediaDir)
{
  g_TextureManager.SetTexturePath(strMediaDir);
  m_strMediaDir = strMediaDir;
}

void CGraphicContextBase::NotifyAppFocusChange(bool bGaining)
{
  /* Notification from the Application that we are either becoming the foreground window or are losing focus */
  if (m_screenSurface)
    m_screenSurface->NotifyAppFocusChange(bGaining);
}

