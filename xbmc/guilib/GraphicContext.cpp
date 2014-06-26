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
#include "GraphicContext.h"
#include "threads/SingleLock.h"
#include "Application.h"
#include "ApplicationMessenger.h"
#include "settings/AdvancedSettings.h"
#include "settings/DisplaySettings.h"
#include "settings/lib/Setting.h"
#include "settings/Settings.h"
#include "cores/VideoRenderers/RenderManager.h"
#include "windowing/WindowingFactory.h"
#include "TextureManager.h"
#include "input/MouseStat.h"
#include "GUIWindowManager.h"
#include "utils/JobManager.h"
#include "video/VideoReferenceClock.h"
#include "cores/IPlayer.h"

using namespace std;

extern bool g_fullScreen;

/* quick access to a skin setting, fine unless we starts clearing video settings */
static CSettingInt* g_guiSkinzoom = NULL;

CGraphicContext::CGraphicContext(void) :
  m_iScreenHeight(576),
  m_iScreenWidth(720),
  m_iScreenId(0),
  /*m_videoRect,*/
  m_bFullScreenRoot(false),
  m_bFullScreenVideo(false),
  m_bCalibrating(false),
  m_Resolution(RES_INVALID),
  /*m_windowResolution,*/
  /*,m_cameras, */
  /*m_origins, */
  /*m_clipRegions,*/
  /*m_guiTransform,*/
  /*m_finalTransform, */
  /*m_groupTransform*/
  m_stereoView(RENDER_STEREO_VIEW_OFF)
  , m_stereoMode(RENDER_STEREO_MODE_OFF)
  , m_nextStereoMode(RENDER_STEREO_MODE_OFF)
{
}

CGraphicContext::~CGraphicContext(void)
{
}

void CGraphicContext::OnSettingChanged(const CSetting *setting)
{
  if (setting == NULL)
    return;

  const std::string &settingId = setting->GetId();
  if (settingId == "videoscreen.fakefullscreen")
  {
    if (IsFullScreenRoot())
      SetVideoResolution(GetVideoResolution(), true);
  }
}

void CGraphicContext::SetOrigin(float x, float y)
{
  if (!m_origins.empty())
    m_origins.push(CPoint(x,y) + m_origins.top());
  else
    m_origins.push(CPoint(x,y));

  AddTransform(TransformMatrix::CreateTranslation(x, y));
}

void CGraphicContext::RestoreOrigin()
{
  if (!m_origins.empty())
    m_origins.pop();
  RemoveTransform();
}

// add a new clip region, intersecting with the previous clip region.
bool CGraphicContext::SetClipRegion(float x, float y, float w, float h)
{ // transform from our origin
  CPoint origin;
  if (!m_origins.empty())
    origin = m_origins.top();

  // ok, now intersect with our old clip region
  CRect rect(x, y, x + w, y + h);
  rect += origin;
  if (!m_clipRegions.empty())
  {
    // intersect with original clip region
    rect.Intersect(m_clipRegions.top());
  }

  if (rect.IsEmpty())
    return false;

  m_clipRegions.push(rect);

  // here we could set the hardware clipping, if applicable
  return true;
}

void CGraphicContext::RestoreClipRegion()
{
  if (!m_clipRegions.empty())
    m_clipRegions.pop();

  // here we could reset the hardware clipping, if applicable
}

void CGraphicContext::ClipRect(CRect &vertex, CRect &texture, CRect *texture2)
{
  // this is the software clipping routine.  If the graphics hardware is set to do the clipping
  // (eg via SetClipPlane in D3D for instance) then this routine is unneeded.
  if (!m_clipRegions.empty())
  {
    // take a copy of the vertex rectangle and intersect
    // it with our clip region (moved to the same coordinate system)
    CRect clipRegion(m_clipRegions.top());
    if (!m_origins.empty())
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

bool CGraphicContext::SetViewPort(float fx, float fy, float fwidth, float fheight, bool intersectPrevious /* = false */)
{
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

  int newLeft = (int)(minX + 0.5f);
  int newTop = (int)(minY + 0.5f);
  int newRight = (int)(maxX + 0.5f);
  int newBottom = (int)(maxY + 0.5f);
  if (intersectPrevious)
  {
    CRect oldviewport = m_viewStack.top();
    // do the intersection
    int oldLeft = (int)oldviewport.x1;
    int oldTop = (int)oldviewport.y1;
    int oldRight = (int)oldviewport.x2;
    int oldBottom = (int)oldviewport.y2;
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

  assert(newLeft < newRight);
  assert(newTop < newBottom);

  CRect newviewport((float)newLeft, (float)newTop, (float)newRight, (float)newBottom);

  m_viewStack.push(newviewport);

  newviewport = StereoCorrection(newviewport);
  g_Windowing.SetViewPort(newviewport);


  UpdateCameraPosition(m_cameras.top());
  return true;
}

void CGraphicContext::RestoreViewPort()
{
  if (m_viewStack.size() <= 1) return;

  m_viewStack.pop();
  CRect viewport = StereoCorrection(m_viewStack.top());
  g_Windowing.SetViewPort(viewport);

  UpdateCameraPosition(m_cameras.top());
}

CPoint CGraphicContext::StereoCorrection(const CPoint &point) const
{
  CPoint res(point);

  if(m_stereoMode == RENDER_STEREO_MODE_SPLIT_HORIZONTAL)
  {
    const RESOLUTION_INFO info = GetResInfo();

    if(m_stereoView == RENDER_STEREO_VIEW_RIGHT)
      res.y += info.iHeight + info.iBlanking;
  }
  if(m_stereoMode == RENDER_STEREO_MODE_SPLIT_VERTICAL)
  {
    const RESOLUTION_INFO info = GetResInfo();

    if(m_stereoView == RENDER_STEREO_VIEW_RIGHT)
      res.x += info.iWidth  + info.iBlanking;
  }
  return res;
}

CRect CGraphicContext::StereoCorrection(const CRect &rect) const
{
  CRect res(StereoCorrection(rect.P1())
          , StereoCorrection(rect.P2()));
  return res;
}

void CGraphicContext::SetScissors(const CRect &rect)
{
  m_scissors = rect;
  m_scissors.Intersect(CRect(0,0,(float)m_iScreenWidth, (float)m_iScreenHeight));
  g_Windowing.SetScissors(StereoCorrection(m_scissors));
}

void CGraphicContext::ResetScissors()
{
  m_scissors.SetRect(0, 0, (float)m_iScreenWidth, (float)m_iScreenHeight);
  g_Windowing.SetScissors(StereoCorrection(m_scissors));
}

const CRect CGraphicContext::GetViewWindow() const
{
  if (m_bCalibrating || m_bFullScreenVideo)
  {
    CRect rect;
    RESOLUTION_INFO info = GetResInfo();
    rect.x1 = (float)info.Overscan.left;
    rect.y1 = (float)info.Overscan.top;
    rect.x2 = (float)info.Overscan.right;
    rect.y2 = (float)info.Overscan.bottom;
    return rect;
  }
  return m_videoRect;
}

void CGraphicContext::SetViewWindow(float left, float top, float right, float bottom)
{
  m_videoRect.x1 = ScaleFinalXCoord(left, top);
  m_videoRect.y1 = ScaleFinalYCoord(left, top);
  m_videoRect.x2 = ScaleFinalXCoord(right, bottom);
  m_videoRect.y2 = ScaleFinalYCoord(right, bottom);
}

void CGraphicContext::SetFullScreenVideo(bool bOnOff)
{
  Lock();
  m_bFullScreenVideo = bOnOff;

#if defined(HAS_VIDEO_PLAYBACK)
  if(m_bFullScreenRoot)
  {
    bool allowDesktopRes = CSettings::Get().GetInt("videoplayer.adjustrefreshrate") == ADJUST_REFRESHRATE_ALWAYS;
    if(m_bFullScreenVideo || (!allowDesktopRes && g_application.m_pPlayer->IsPlayingVideo()))
      SetVideoResolution(g_renderManager.GetResolution());
    else if(CDisplaySettings::Get().GetCurrentResolution() > RES_DESKTOP)
      SetVideoResolution(CDisplaySettings::Get().GetCurrentResolution());
    else
      SetVideoResolution(RES_DESKTOP);
  }
  else
    SetVideoResolution(RES_WINDOW);
#endif

  Unlock();
}

bool CGraphicContext::IsFullScreenVideo() const
{
  return m_bFullScreenVideo;
}

bool CGraphicContext::IsCalibrating() const
{
  return m_bCalibrating;
}

void CGraphicContext::SetCalibrating(bool bOnOff)
{
  m_bCalibrating = bOnOff;
}

bool CGraphicContext::IsValidResolution(RESOLUTION res)
{
  if (res >= RES_WINDOW && (size_t) res <= CDisplaySettings::Get().ResolutionInfoSize())
  {
    return true;
  }

  return false;
}

// call SetVideoResolutionInternal and ensure its done from mainthread
void CGraphicContext::SetVideoResolution(RESOLUTION res, bool forceUpdate)
{
  if (g_application.IsCurrentThread())
  {
    SetVideoResolutionInternal(res, forceUpdate);
  }
  else
  {
    ThreadMessage msg = {TMSG_SETVIDEORESOLUTION};
    msg.param1 = res;
    msg.param2 = forceUpdate ? 1 : 0;
    CApplicationMessenger::Get().SendMessage(msg, true);
  }
}

void CGraphicContext::SetVideoResolutionInternal(RESOLUTION res, bool forceUpdate)
{
  RESOLUTION lastRes = m_Resolution;

  // If the user asked us to guess, go with desktop
  if (res == RES_AUTORES || !IsValidResolution(res))
  {
    res = RES_DESKTOP;
  }

  // If we are switching to the same resolution and same window/full-screen, no need to do anything
  if (!forceUpdate && res == lastRes && m_bFullScreenRoot == g_advancedSettings.m_fullScreen)
  {
    return;
  }

  //only pause when switching monitor resolution/refreshrate,
  //not when switching between fullscreen and windowed or when resizing the window
  if ((res != RES_DESKTOP && res != RES_WINDOW) || (lastRes != RES_DESKTOP && lastRes != RES_WINDOW))
  {
    //pause the player during the refreshrate change
    int delay = CSettings::Get().GetInt("videoplayer.pauseafterrefreshchange");
    if (delay > 0 && CSettings::Get().GetInt("videoplayer.adjustrefreshrate") != ADJUST_REFRESHRATE_OFF && g_application.m_pPlayer->IsPlayingVideo() && !g_application.m_pPlayer->IsPausedPlayback())
    {
      g_application.m_pPlayer->Pause();
      ThreadMessage msg = {TMSG_MEDIA_UNPAUSE};
      CDelayedMessage* pauseMessage = new CDelayedMessage(msg, delay * 100);
      pauseMessage->Create(true);
    }
  }

  if (res >= RES_DESKTOP)
  {
    g_advancedSettings.m_fullScreen = true;
    m_bFullScreenRoot = true;
  }
  else
  {
    g_advancedSettings.m_fullScreen = false;
    m_bFullScreenRoot = false;
  }

  Lock();

  RESOLUTION_INFO info_org  = CDisplaySettings::Get().GetResolutionInfo(res);
  RESOLUTION_INFO info_last = CDisplaySettings::Get().GetResolutionInfo(lastRes);

  RENDER_STEREO_MODE stereo_mode = m_stereoMode;

  // if the new mode is an actual stereo mode, switch to that
  // if the old mode was an actual stereo mode, switch to no 3d mode
  if (info_org.dwFlags & D3DPRESENTFLAG_MODE3DTB)
    stereo_mode = RENDER_STEREO_MODE_SPLIT_HORIZONTAL;
  else if (info_org.dwFlags & D3DPRESENTFLAG_MODE3DSBS)
    stereo_mode = RENDER_STEREO_MODE_SPLIT_VERTICAL;
  else if ((info_last.dwFlags & D3DPRESENTFLAG_MODE3DSBS) != 0
        || (info_last.dwFlags & D3DPRESENTFLAG_MODE3DTB)  != 0)
    stereo_mode = RENDER_STEREO_MODE_OFF;

  if(stereo_mode != m_stereoMode)
  {
    m_stereoView     = RENDER_STEREO_VIEW_OFF;
    m_stereoMode     = stereo_mode;
    m_nextStereoMode = stereo_mode;
    CSettings::Get().SetInt("videoscreen.stereoscopicmode", (int)m_stereoMode);
  }

  RESOLUTION_INFO info_mod = GetResInfo(res);

  m_iScreenWidth  = info_mod.iWidth;
  m_iScreenHeight = info_mod.iHeight;
  m_iScreenId     = info_mod.iScreen;
  m_scissors.SetRect(0, 0, (float)m_iScreenWidth, (float)m_iScreenHeight);
  m_Resolution    = res;

  if (g_advancedSettings.m_fullScreen)
  {
#if defined (TARGET_DARWIN) || defined (TARGET_WINDOWS)
    bool blankOtherDisplays = CSettings::Get().GetBool("videoscreen.blankdisplays");
    g_Windowing.SetFullScreen(true,  info_org, blankOtherDisplays);
#else
    g_Windowing.SetFullScreen(true,  info_org, false);
#endif
  }
  else if (lastRes >= RES_DESKTOP )
    g_Windowing.SetFullScreen(false, info_org, false);
  else
    g_Windowing.ResizeWindow(info_org.iWidth, info_org.iHeight, -1, -1);

  //tell the videoreferenceclock that we changed the refreshrate
  g_VideoReferenceClock.RefreshChanged();

  // make sure all stereo stuff are correctly setup
  SetStereoView(RENDER_STEREO_VIEW_OFF);

  // update anyone that relies on sizing information
  g_renderManager.Recover();
  g_Mouse.SetResolution(info_org.iWidth, info_org.iHeight, 1, 1);
  g_windowManager.SendMessage(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_WINDOW_RESIZE);

  Unlock();
}

RESOLUTION CGraphicContext::GetVideoResolution() const
{
  return m_Resolution;
}

void CGraphicContext::ResetOverscan(RESOLUTION_INFO &res)
{
  res.Overscan.left = 0;
  res.Overscan.top = 0;
  res.Overscan.right = res.iWidth;
  res.Overscan.bottom = res.iHeight;
}

void CGraphicContext::ResetOverscan(RESOLUTION res, OVERSCAN &overscan)
{
  overscan.left = 0;
  overscan.top = 0;
  switch (res)
  {
  case RES_HDTV_1080i:
    overscan.right = 1920;
    overscan.bottom = 1080;
    break;
  case RES_HDTV_720pSBS:
    overscan.right = 640;
    overscan.bottom = 720;
    break;
  case RES_HDTV_720pTB:
    overscan.right = 1280;
    overscan.bottom = 360;
    break;
  case RES_HDTV_1080pSBS:
    overscan.right = 960;
    overscan.bottom = 1080;
    break;
  case RES_HDTV_1080pTB:
    overscan.right = 1920;
    overscan.bottom = 540;
    break;
  case RES_HDTV_720p:
    overscan.right = 1280;
    overscan.bottom = 720;
    break;
  case RES_HDTV_480p_16x9:
  case RES_HDTV_480p_4x3:
  case RES_NTSC_16x9:
  case RES_NTSC_4x3:
  case RES_PAL60_16x9:
  case RES_PAL60_4x3:
    overscan.right = 720;
    overscan.bottom = 480;
    break;
  case RES_PAL_16x9:
  case RES_PAL_4x3:
    overscan.right = 720;
    overscan.bottom = 576;
    break;
  default:
    RESOLUTION_INFO info = GetResInfo(res);
    overscan.right  = info.iWidth;
    overscan.bottom = info.iHeight;
    break;
  }
}

void CGraphicContext::ResetScreenParameters(RESOLUTION res)
{
  // For now these are all on the first screen.
  RESOLUTION_INFO& info = CDisplaySettings::Get().GetResolutionInfo(res);

  info.iScreen = 0;

  // 1080i
  switch (res)
  {
  case RES_HDTV_1080i:
    info.iSubtitles = (int)(0.965 * 1080);
    info.iWidth = 1920;
    info.iHeight = 1080;
    info.dwFlags = D3DPRESENTFLAG_INTERLACED | D3DPRESENTFLAG_WIDESCREEN;
    info.fPixelRatio = 1.0f;
    info.strMode ="1080i 16:9";
    break;
  case RES_HDTV_720pSBS:
    info.iSubtitles = (int)(0.965 * 720);
    info.iWidth = 640;
    info.iHeight = 720;
    info.dwFlags = D3DPRESENTFLAG_PROGRESSIVE | D3DPRESENTFLAG_WIDESCREEN | D3DPRESENTFLAG_MODE3DSBS;
    info.fPixelRatio = 2.0f;
    info.strMode = "720pSBS 16:9";
    break;
  case RES_HDTV_720pTB:
    info.iSubtitles = (int)(0.965 * 720);
    info.iWidth = 1280;
    info.iHeight = 720;
    info.dwFlags = D3DPRESENTFLAG_PROGRESSIVE | D3DPRESENTFLAG_WIDESCREEN | D3DPRESENTFLAG_MODE3DTB;
    info.fPixelRatio = 0.5f;
    info.strMode = "720pTB 16:9";
    break;
  case RES_HDTV_1080pSBS:
    info.iSubtitles = (int)(0.965 * 1080);
    info.iWidth = 1920;
    info.iHeight = 1080;
    info.dwFlags = D3DPRESENTFLAG_PROGRESSIVE | D3DPRESENTFLAG_WIDESCREEN | D3DPRESENTFLAG_MODE3DSBS;
    info.fPixelRatio = 2.0f;
    info.strMode = "1080pSBS 16:9";
    break;
  case RES_HDTV_1080pTB:
    info.iSubtitles = (int)(0.965 * 1080);
    info.iWidth = 1920;
    info.iHeight = 1080;
    info.dwFlags = D3DPRESENTFLAG_PROGRESSIVE | D3DPRESENTFLAG_WIDESCREEN | D3DPRESENTFLAG_MODE3DTB;
    info.fPixelRatio = 0.5f;
    info.strMode = "1080pTB 16:9";
    break;
  case RES_HDTV_720p:
    info.iSubtitles = (int)(0.965 * 720);
    info.iWidth = 1280;
    info.iHeight = 720;
    info.dwFlags = D3DPRESENTFLAG_PROGRESSIVE | D3DPRESENTFLAG_WIDESCREEN;
    info.fPixelRatio = 1.0f;
    info.strMode = "720p 16:9";
    break;
  case RES_HDTV_480p_4x3:
    info.iSubtitles = (int)(0.9 * 480);
    info.iWidth = 720;
    info.iHeight = 480;
    info.dwFlags = D3DPRESENTFLAG_PROGRESSIVE;
    info.fPixelRatio = 4320.0f / 4739.0f;
    info.strMode = "480p 4:3";
    break;
  case RES_HDTV_480p_16x9:
    info.iSubtitles = (int)(0.965 * 480);
    info.iWidth = 720;
    info.iHeight = 480;
    info.dwFlags = D3DPRESENTFLAG_PROGRESSIVE | D3DPRESENTFLAG_WIDESCREEN;
    info.fPixelRatio = 4320.0f / 4739.0f*4.0f / 3.0f;
    info.strMode = "480p 16:9";
    break;
  case RES_NTSC_4x3:
    info.iSubtitles = (int)(0.9 * 480);
    info.iWidth = 720;
    info.iHeight = 480;
    info.dwFlags = D3DPRESENTFLAG_INTERLACED;
    info.fPixelRatio = 4320.0f / 4739.0f;
    info.strMode = "NTSC 4:3";
    break;
  case RES_NTSC_16x9:
    info.iSubtitles = (int)(0.965 * 480);
    info.iWidth = 720;
    info.iHeight = 480;
    info.dwFlags = D3DPRESENTFLAG_INTERLACED | D3DPRESENTFLAG_WIDESCREEN;
    info.fPixelRatio = 4320.0f / 4739.0f*4.0f / 3.0f;
    info.strMode = "NTSC 16:9";
    break;
  case RES_PAL_4x3:
    info.iSubtitles = (int)(0.9 * 576);
    info.iWidth = 720;
    info.iHeight = 576;
    info.dwFlags = D3DPRESENTFLAG_INTERLACED;
    info.fPixelRatio = 128.0f / 117.0f;
    info.strMode = "PAL 4:3";
    break;
  case RES_PAL_16x9:
    info.iSubtitles = (int)(0.965 * 576);
    info.iWidth = 720;
    info.iHeight = 576;
    info.dwFlags = D3DPRESENTFLAG_INTERLACED | D3DPRESENTFLAG_WIDESCREEN;
    info.fPixelRatio = 128.0f / 117.0f*4.0f / 3.0f;
    info.strMode = "PAL 16:9";
    break;
  case RES_PAL60_4x3:
    info.iSubtitles = (int)(0.9 * 480);
    info.iWidth = 720;
    info.iHeight = 480;
    info.dwFlags = D3DPRESENTFLAG_INTERLACED;
    info.fPixelRatio = 4320.0f / 4739.0f;
    info.strMode = "PAL60 4:3";
    break;
  case RES_PAL60_16x9:
    info.iSubtitles = (int)(0.965 * 480);
    info.iWidth = 720;
    info.iHeight = 480;
    info.dwFlags = D3DPRESENTFLAG_INTERLACED | D3DPRESENTFLAG_WIDESCREEN;
    info.fPixelRatio = 4320.0f / 4739.0f*4.0f / 3.0f;
    info.strMode = "PAL60 16:9";
    break;
  case RES_WINDOW:
    info.iSubtitles = (int)(0.965 * info.iHeight);
    info.fPixelRatio = 1.0;
    break;
  default:
    break;
  }
  info.iScreenWidth  = info.iWidth;
  info.iScreenHeight = info.iHeight;
  ResetOverscan(res, info.Overscan);
}

void CGraphicContext::Clear(color_t color)
{
  g_Windowing.ClearBuffers(color);
}

void CGraphicContext::CaptureStateBlock()
{
  g_Windowing.CaptureStateBlock();
}

void CGraphicContext::ApplyStateBlock()
{
  g_Windowing.ApplyStateBlock();
}

const RESOLUTION_INFO CGraphicContext::GetResInfo(RESOLUTION res) const
{
  RESOLUTION_INFO info = CDisplaySettings::Get().GetResolutionInfo(res);

  if(m_stereoMode == RENDER_STEREO_MODE_SPLIT_HORIZONTAL)
  {
    if((info.dwFlags & D3DPRESENTFLAG_MODE3DTB) == 0)
    {
      info.fPixelRatio     /= 2;
      info.iBlanking        = 0;
      info.dwFlags         |= D3DPRESENTFLAG_MODE3DTB;
    }
    info.iHeight          = (info.iHeight         - info.iBlanking) / 2;
    info.Overscan.top    /= 2;
    info.Overscan.bottom  = (info.Overscan.bottom - info.iBlanking) / 2;
    info.iSubtitles       = (info.iSubtitles      - info.iBlanking) / 2;
  }

  if(m_stereoMode == RENDER_STEREO_MODE_SPLIT_VERTICAL)
  {
    if((info.dwFlags & D3DPRESENTFLAG_MODE3DSBS) == 0)
    {
      info.fPixelRatio     *= 2;
      info.iBlanking        = 0;
      info.dwFlags         |= D3DPRESENTFLAG_MODE3DSBS;
    }
    info.iWidth           = (info.iWidth         - info.iBlanking) / 2;
    info.Overscan.left   /= 2;
    info.Overscan.right   = (info.Overscan.right - info.iBlanking) / 2;
  }
  return info;
}

void CGraphicContext::SetResInfo(RESOLUTION res, const RESOLUTION_INFO& info)
{
  RESOLUTION_INFO& curr = CDisplaySettings::Get().GetResolutionInfo(res);
  curr.Overscan   = info.Overscan;
  curr.iSubtitles = info.iSubtitles;
  curr.fPixelRatio = info.fPixelRatio;

  if(info.dwFlags & D3DPRESENTFLAG_MODE3DSBS)
  {
    curr.Overscan.right  = info.Overscan.right  * 2 + info.iBlanking;
    if((curr.dwFlags & D3DPRESENTFLAG_MODE3DSBS) == 0)
      curr.fPixelRatio  /= 2.0;
  }

  if(info.dwFlags & D3DPRESENTFLAG_MODE3DTB)
  {
    curr.Overscan.bottom = info.Overscan.bottom * 2 + info.iBlanking;
    curr.iSubtitles      = info.iSubtitles      * 2 + info.iBlanking;
    if((curr.dwFlags & D3DPRESENTFLAG_MODE3DTB) == 0)
      curr.fPixelRatio  *= 2.0;
  }
}

void CGraphicContext::GetGUIScaling(const RESOLUTION_INFO &res, float &scaleX, float &scaleY, TransformMatrix *matrix /* = NULL */)
{
  if (m_Resolution != RES_INVALID)
  {
    // calculate necessary scalings
    RESOLUTION_INFO info = GetResInfo();
    float fFromWidth  = (float)res.iWidth;
    float fFromHeight = (float)res.iHeight;
    float fToPosX     = (float)info.Overscan.left;
    float fToPosY     = (float)info.Overscan.top;
    float fToWidth    = (float)info.Overscan.right  - fToPosX;
    float fToHeight   = (float)info.Overscan.bottom - fToPosY;

    if(!g_guiSkinzoom) // lookup gui setting if we didn't have it already
      g_guiSkinzoom = (CSettingInt*)CSettings::Get().GetSetting("lookandfeel.skinzoom");

    float fZoom = 1.0f;
    if(g_guiSkinzoom)
      fZoom *= (100 + g_guiSkinzoom->GetValue()) * 0.01f;

    fZoom -= 1.0f;
    fToPosX -= fToWidth * fZoom * 0.5f;
    fToWidth *= fZoom + 1.0f;

    // adjust for aspect ratio as zoom is given in the vertical direction and we don't
    // do aspect ratio corrections in the gui code
    fZoom = fZoom / info.fPixelRatio;
    fToPosY -= fToHeight * fZoom * 0.5f;
    fToHeight *= fZoom + 1.0f;

    scaleX = fFromWidth / fToWidth;
    scaleY = fFromHeight / fToHeight;
    if (matrix)
    {
      TransformMatrix guiScaler = TransformMatrix::CreateScaler(fToWidth / fFromWidth, fToHeight / fFromHeight, fToHeight / fFromHeight);
      TransformMatrix guiOffset = TransformMatrix::CreateTranslation(fToPosX, fToPosY);
      *matrix = guiOffset * guiScaler;
    }
  }
  else
  {
    scaleX = scaleY = 1.0f;
    if (matrix)
      matrix->Reset();
  }
}

void CGraphicContext::SetScalingResolution(const RESOLUTION_INFO &res, bool needsScaling)
{
  Lock();
  m_windowResolution = res;
  if (needsScaling && m_Resolution != RES_INVALID)
    GetGUIScaling(res, m_guiTransform.scaleX, m_guiTransform.scaleY, &m_guiTransform.matrix);
  else
  {
    m_guiTransform.Reset();
  }

  // reset our origin and camera
  while (!m_origins.empty())
    m_origins.pop();
  m_origins.push(CPoint(0, 0));
  while (!m_cameras.empty())
    m_cameras.pop();
  m_cameras.push(CPoint(0.5f*m_iScreenWidth, 0.5f*m_iScreenHeight));

  // and reset the final transform
  m_finalTransform = m_guiTransform;
  Unlock();
}

void CGraphicContext::SetRenderingResolution(const RESOLUTION_INFO &res, bool needsScaling)
{
  Lock();
  SetScalingResolution(res, needsScaling);
  UpdateCameraPosition(m_cameras.top());
  Unlock();
}

void CGraphicContext::SetStereoView(RENDER_STEREO_VIEW view)
{
  m_stereoView = view;

  while(!m_viewStack.empty())
    m_viewStack.pop();

  CRect viewport(0.0f, 0.0f, (float)m_iScreenWidth, (float)m_iScreenHeight);

  m_viewStack.push(viewport);

  viewport = StereoCorrection(viewport);
  g_Windowing.SetStereoMode(m_stereoMode, m_stereoView);
  g_Windowing.SetViewPort(viewport);
  g_Windowing.SetScissors(viewport);
}

void CGraphicContext::InvertFinalCoords(float &x, float &y) const
{
  m_finalTransform.matrix.InverseTransformPosition(x, y);
}

float CGraphicContext::GetScalingPixelRatio() const
{
  // assume the resolutions are different - we want to return the aspect ratio of the video resolution
  // but only once it's been corrected for the skin -> screen coordinates scaling
  return GetResInfo().fPixelRatio * (m_finalTransform.scaleY / m_finalTransform.scaleX);
}

void CGraphicContext::SetCameraPosition(const CPoint &camera)
{
  // offset the camera from our current location (this is in XML coordinates) and scale it up to
  // the screen resolution
  CPoint cam(camera);
  if (!m_origins.empty())
    cam += m_origins.top();

  cam.x *= (float)m_iScreenWidth / m_windowResolution.iWidth;
  cam.y *= (float)m_iScreenHeight / m_windowResolution.iHeight;

  m_cameras.push(cam);
  UpdateCameraPosition(m_cameras.top());
}

void CGraphicContext::RestoreCameraPosition()
{ // remove the top camera from the stack
  assert(m_cameras.size());
  m_cameras.pop();
  UpdateCameraPosition(m_cameras.top());
}

CRect CGraphicContext::generateAABB(const CRect &rect) const
{
// ------------------------
// |(x1, y1)      (x2, y2)|
// |                      |
// |(x3, y3)      (x4, y4)|
// ------------------------

  float x1 = rect.x1, x2 = rect.x2, x3 = rect.x1, x4 = rect.x2;
  float y1 = rect.y1, y2 = rect.y1, y3 = rect.y2, y4 = rect.y2;

  float z = 0.0f;
  ScaleFinalCoords(x1, y1, z);
  g_Windowing.Project(x1, y1, z);

  z = 0.0f;
  ScaleFinalCoords(x2, y2, z);
  g_Windowing.Project(x2, y2, z);

  z = 0.0f;
  ScaleFinalCoords(x3, y3, z);
  g_Windowing.Project(x3, y3, z);

  z = 0.0f;
  ScaleFinalCoords(x4, y4, z);
  g_Windowing.Project(x4, y4, z);

  return CRect( min(min(min(x1, x2), x3), x4),
                min(min(min(y1, y2), y3), y4),
                max(max(max(x1, x2), x3), x4),
                max(max(max(y1, y2), y3), y4));
}

// NOTE: This routine is currently called (twice) every time there is a <camera>
//       tag in the skin.  It actually only has to be called before we render
//       something, so another option is to just save the camera coordinates
//       and then have a routine called before every draw that checks whether
//       the camera has changed, and if so, changes it.  Similarly, it could set
//       the world transform at that point as well (or even combine world + view
//       to cut down on one setting)
void CGraphicContext::UpdateCameraPosition(const CPoint &camera)
{
  g_Windowing.SetCameraPosition(camera, m_iScreenWidth, m_iScreenHeight);
}

bool CGraphicContext::RectIsAngled(float x1, float y1, float x2, float y2) const
{ // need only test 3 points, as they must be co-planer
  if (m_finalTransform.matrix.TransformZCoord(x1, y1, 0)) return true;
  if (m_finalTransform.matrix.TransformZCoord(x2, y2, 0)) return true;
  if (m_finalTransform.matrix.TransformZCoord(x1, y2, 0)) return true;
  return false;
}

float CGraphicContext::GetFPS() const
{
  if (m_Resolution != RES_INVALID)
  {
    RESOLUTION_INFO info = g_graphicsContext.GetResInfo();
    if (info.fRefreshRate > 0)
      return info.fRefreshRate;
    if (m_Resolution == RES_PAL_4x3 || m_Resolution == RES_PAL_16x9)
      return 50.0f;
    if (m_Resolution == RES_HDTV_1080i)
      return 30.0f;
  }
  return 60.0f;
}

void CGraphicContext::BeginPaint(bool lock)
{
  if (lock) Lock();
}

void CGraphicContext::EndPaint(bool lock)
{
  if (lock) Unlock();
}

bool CGraphicContext::IsFullScreenRoot () const
{
  return m_bFullScreenRoot;
}

bool CGraphicContext::ToggleFullScreenRoot ()
{
  RESOLUTION uiRes;  ///< resolution to save - not necessarily the same as the one we switch to (e.g. during video playback)
  RESOLUTION videoRes;
  bool setVideoRes = false;

  if (m_bFullScreenRoot)
  {
    uiRes = RES_WINDOW;
  }
  else
  {
    if (CDisplaySettings::Get().GetCurrentResolution() > RES_DESKTOP)
      uiRes = CDisplaySettings::Get().GetCurrentResolution();
    else
      uiRes = (RESOLUTION) g_Windowing.DesktopResolution(g_Windowing.GetCurrentScreen());

#if defined(HAS_VIDEO_PLAYBACK)
    if (IsCalibrating())
    {
      /* we need to trick renderer that we are fullscreen already so it gives us a valid value */
      m_bFullScreenRoot = true;
      uiRes = g_renderManager.GetResolution();
      m_bFullScreenRoot = false;
    }
    // if video is playing we need to switch to resolution chosen by RenderManager
    if (IsFullScreenVideo())
    {
      m_bFullScreenRoot = true;
      videoRes = g_renderManager.GetResolution();
      m_bFullScreenRoot = false;
      setVideoRes = true;
    }
#endif
  }

  CDisplaySettings::Get().SetCurrentResolution(uiRes, true);

  if (setVideoRes)
  {
    SetVideoResolution(videoRes);
  }

  return m_bFullScreenRoot;
}

void CGraphicContext::SetMediaDir(const std::string &strMediaDir)
{
  g_TextureManager.SetTexturePath(strMediaDir);
  m_strMediaDir = strMediaDir;
}

void CGraphicContext::Flip(const CDirtyRegionList& dirty)
{
  g_Windowing.PresentRender(dirty);

  if(m_stereoMode != m_nextStereoMode)
  {
    m_stereoMode = m_nextStereoMode;
    SetVideoResolution(GetVideoResolution(), true);
    g_windowManager.SendMessage(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_RENDERER_RESET);
  }
}

void CGraphicContext::ApplyHardwareTransform()
{
  g_Windowing.ApplyHardwareTransform(m_finalTransform.matrix);
}

void CGraphicContext::RestoreHardwareTransform()
{
  g_Windowing.RestoreHardwareTransform();
}

void CGraphicContext::GetAllowedResolutions(vector<RESOLUTION> &res)
{
  res.clear();

  res.push_back(RES_WINDOW);
  res.push_back(RES_DESKTOP);
  for (size_t r = (size_t) RES_CUSTOM; r < CDisplaySettings::Get().ResolutionInfoSize(); r++)
  {
    res.push_back((RESOLUTION) r);
  }
}

