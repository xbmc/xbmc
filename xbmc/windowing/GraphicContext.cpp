/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GraphicContext.h"

#include "ServiceBroker.h"
#include "WinSystem.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/TextureManager.h"
#include "guilib/gui3d.h"
#include "input/InputManager.h"
#include "messaging/ApplicationMessenger.h"
#include "rendering/RenderSystem.h"
#include "settings/AdvancedSettings.h"
#include "settings/DisplaySettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "utils/log.h"

#include <cassert>
#include <mutex>

CGraphicContext::CGraphicContext() = default;
CGraphicContext::~CGraphicContext() = default;

void CGraphicContext::SetOrigin(float x, float y)
{
  if (!m_origins.empty())
    m_origins.push(CPoint(x,y) + m_origins.top());
  else
    m_origins.emplace(x, y);

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

CRect CGraphicContext::GetClipRegion()
{
  if (m_clipRegions.empty())
    return CRect(0, 0, m_iScreenWidth, m_iScreenHeight);
  CRect clipRegion(m_clipRegions.top());
  if (!m_origins.empty())
    clipRegion -= m_origins.top();
  return clipRegion;
}

void CGraphicContext::AddGUITransform()
{
  m_transforms.push(m_finalTransform);
  m_finalTransform = m_guiTransform;
}

TransformMatrix CGraphicContext::AddTransform(const TransformMatrix &matrix)
{
  m_transforms.push(m_finalTransform);
  m_finalTransform.matrix *= matrix;
  return m_finalTransform.matrix;
}

void CGraphicContext::SetTransform(const TransformMatrix &matrix)
{
  m_transforms.push(m_finalTransform);
  m_finalTransform.matrix = matrix;
}

void CGraphicContext::SetTransform(const TransformMatrix &matrix, float scaleX, float scaleY)
{
  m_transforms.push(m_finalTransform);
  m_finalTransform.matrix = matrix;
  m_finalTransform.scaleX = scaleX;
  m_finalTransform.scaleY = scaleY;
}

void CGraphicContext::RemoveTransform()
{
  if (!m_transforms.empty())
  {
    m_finalTransform = m_transforms.top();
    m_transforms.pop();
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
  CServiceBroker::GetRenderSystem()->SetViewPort(newviewport);


  UpdateCameraPosition(m_cameras.top(), m_stereoFactors.top());
  return true;
}

void CGraphicContext::RestoreViewPort()
{
  if (m_viewStack.size() <= 1) return;

  m_viewStack.pop();
  CRect viewport = StereoCorrection(m_viewStack.top());
  CServiceBroker::GetRenderSystem()->SetViewPort(viewport);

  UpdateCameraPosition(m_cameras.top(), m_stereoFactors.top());
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
  CServiceBroker::GetRenderSystem()->SetScissors(StereoCorrection(m_scissors));
}

const CRect &CGraphicContext::GetScissors() const
{
  return m_scissors;
}

void CGraphicContext::ResetScissors()
{
  m_scissors.SetRect(0, 0, (float)m_iScreenWidth, (float)m_iScreenHeight);
  CServiceBroker::GetRenderSystem()->SetScissors(StereoCorrection(m_scissors));
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
  std::unique_lock<CCriticalSection> lock(*this);

  m_bFullScreenVideo = bOnOff;

  if (m_bFullScreenRoot)
  {
    bool bTriggerUpdateRes = false;
    auto& components = CServiceBroker::GetAppComponents();
    const auto appPlayer = components.GetComponent<CApplicationPlayer>();
    if (m_bFullScreenVideo)
      bTriggerUpdateRes = true;
    else
    {
      bool allowDesktopRes = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_VIDEOPLAYER_ADJUSTREFRESHRATE) == ADJUST_REFRESHRATE_ALWAYS;
      if (!allowDesktopRes)
      {
        if (appPlayer->IsPlayingVideo())
          bTriggerUpdateRes = true;
      }
    }

    bool allowResolutionChangeOnStop = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_VIDEOPLAYER_ADJUSTREFRESHRATE) != ADJUST_REFRESHRATE_ON_START;
    RESOLUTION targetResolutionOnStop = RES_DESKTOP;
    if (bTriggerUpdateRes)
      appPlayer->TriggerUpdateResolution();
    else if (CDisplaySettings::GetInstance().GetCurrentResolution() > RES_DESKTOP)
    {
      targetResolutionOnStop = CDisplaySettings::GetInstance().GetCurrentResolution();
    }

    if (allowResolutionChangeOnStop && !bTriggerUpdateRes)
    {
      SetVideoResolution(targetResolutionOnStop, false);
    }
  }
  else
    SetVideoResolution(RES_WINDOW, false);
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
  if (res >= RES_WINDOW && (size_t) res < CDisplaySettings::GetInstance().ResolutionInfoSize())
  {
    return true;
  }

  return false;
}

// call SetVideoResolutionInternal and ensure its done from mainthread
void CGraphicContext::SetVideoResolution(RESOLUTION res, bool forceUpdate)
{
  if (CServiceBroker::GetAppMessenger()->IsProcessThread())
  {
    SetVideoResolutionInternal(res, forceUpdate);
  }
  else
  {
    CServiceBroker::GetAppMessenger()->SendMsg(TMSG_SETVIDEORESOLUTION, res, forceUpdate ? 1 : 0);
  }
}

void CGraphicContext::SetVideoResolutionInternal(RESOLUTION res, bool forceUpdate)
{
  RESOLUTION lastRes = m_Resolution;

  // If the user asked us to guess, go with desktop
  if (!IsValidResolution(res))
  {
    res = RES_DESKTOP;
  }

  // If we are switching to the same resolution and same window/full-screen, no need to do anything
  if (!forceUpdate && res == lastRes && m_bFullScreenRoot == CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_fullScreen)
  {
    return;
  }

  if (res >= RES_DESKTOP)
  {
    CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_fullScreen = true;
    m_bFullScreenRoot = true;
  }
  else
  {
    CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_fullScreen = false;
    m_bFullScreenRoot = false;
  }

  std::unique_lock<CCriticalSection> lock(*this);

  // FIXME Wayland windowing needs some way to "deny" resolution updates since what Kodi
  // requests might not get actually set by the compositor.
  // So in theory, m_iScreenWidth etc. would not need to be updated at all before the
  // change is confirmed.
  // But other windowing code expects these variables to be already set when
  // SetFullScreen() is called, so set them anyway and remember the old values.
  int origScreenWidth = m_iScreenWidth;
  int origScreenHeight = m_iScreenHeight;
  float origFPSOverride = m_fFPSOverride;

  UpdateInternalStateWithResolution(res);
  RESOLUTION_INFO info_org  = CDisplaySettings::GetInstance().GetResolutionInfo(res);

  bool switched = false;
  if (CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_fullScreen)
  {
#if defined (TARGET_DARWIN) || defined (TARGET_WINDOWS)
    bool blankOtherDisplays = CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_VIDEOSCREEN_BLANKDISPLAYS);
    switched = CServiceBroker::GetWinSystem()->SetFullScreen(true,  info_org, blankOtherDisplays);
#else
    switched = CServiceBroker::GetWinSystem()->SetFullScreen(true,  info_org, false);
#endif
  }
  else if (lastRes >= RES_DESKTOP )
    switched = CServiceBroker::GetWinSystem()->SetFullScreen(false, info_org, false);
  else
    switched = CServiceBroker::GetWinSystem()->ResizeWindow(info_org.iWidth, info_org.iHeight, -1, -1);

  if (switched)
  {
    m_scissors.SetRect(0, 0, (float)m_iScreenWidth, (float)m_iScreenHeight);

    // make sure all stereo stuff are correctly setup
    SetStereoView(RENDER_STEREO_VIEW_OFF);

    // update anyone that relies on sizing information
    CServiceBroker::GetInputManager().SetMouseResolution(info_org.iWidth, info_org.iHeight, 1, 1);

    CGUIComponent *gui = CServiceBroker::GetGUI();
    if (gui)
      gui->GetWindowManager().SendMessage(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_WINDOW_RESIZE);
  }
  else
  {
    // Reset old state
    m_iScreenWidth = origScreenWidth;
    m_iScreenHeight = origScreenHeight;
    m_fFPSOverride = origFPSOverride;
    if (IsValidResolution(lastRes))
    {
      m_Resolution = lastRes;
    }
    else
    {
      // FIXME Resolution has become invalid
      // This happens e.g. when switching monitors and the new monitor has fewer
      // resolutions than the old one. Fall back to RES_DESKTOP and hope that
      // the real resolution is set soon.
      // Again, must be fixed as part of a greater refactor.
      m_Resolution = RES_DESKTOP;
    }
  }
}

void CGraphicContext::ApplyVideoResolution(RESOLUTION res)
{
  if (!IsValidResolution(res))
  {
    CLog::LogF(LOGWARNING, "Asked to apply invalid resolution {}, falling back to RES_DESKTOP",
               res);
    res = RES_DESKTOP;
  }

  if (res >= RES_DESKTOP)
  {
    CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_fullScreen = true;
    m_bFullScreenRoot = true;
  }
  else
  {
    CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_fullScreen = false;
    m_bFullScreenRoot = false;
  }

  std::unique_lock<CCriticalSection> lock(*this);

  UpdateInternalStateWithResolution(res);

  m_scissors.SetRect(0, 0, (float)m_iScreenWidth, (float)m_iScreenHeight);

  // make sure all stereo stuff are correctly setup
  SetStereoView(RENDER_STEREO_VIEW_OFF);

  // update anyone that relies on sizing information
  RESOLUTION_INFO info_org  = CDisplaySettings::GetInstance().GetResolutionInfo(res);
  CServiceBroker::GetInputManager().SetMouseResolution(info_org.iWidth, info_org.iHeight, 1, 1);
  CServiceBroker::GetGUI()->GetWindowManager().SendMessage(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_WINDOW_RESIZE);
}

void CGraphicContext::UpdateInternalStateWithResolution(RESOLUTION res)
{
  RESOLUTION_INFO info_mod = GetResInfo(res);

  m_iScreenWidth = info_mod.iWidth;
  m_iScreenHeight = info_mod.iHeight;
  m_Resolution = res;
  m_fFPSOverride = 0;
}

void CGraphicContext::ApplyModeChange(RESOLUTION res)
{
  ApplyVideoResolution(res);
  CServiceBroker::GetWinSystem()->FinishModeChange(res);
}

void CGraphicContext::ApplyWindowResize(int newWidth, int newHeight)
{
  CServiceBroker::GetWinSystem()->SetWindowResolution(newWidth, newHeight);
  ApplyVideoResolution(RES_WINDOW);
  CServiceBroker::GetWinSystem()->FinishWindowResize(newWidth, newHeight);
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

  RESOLUTION_INFO info = GetResInfo(res);
  overscan.right  = info.iWidth;
  overscan.bottom = info.iHeight;
}

void CGraphicContext::ResetScreenParameters(RESOLUTION res)
{
  RESOLUTION_INFO& info = CDisplaySettings::GetInstance().GetResolutionInfo(res);

  info.iSubtitles = info.iHeight;
  info.fPixelRatio = 1.0f;
  info.iScreenWidth = info.iWidth;
  info.iScreenHeight = info.iHeight;
  ResetOverscan(res, info.Overscan);
}

void CGraphicContext::Clear()
{
  CServiceBroker::GetRenderSystem()->InvalidateColorBuffer();
}

void CGraphicContext::Clear(UTILS::COLOR::Color color)
{
  CServiceBroker::GetRenderSystem()->ClearBuffers(color);
}

void CGraphicContext::CaptureStateBlock()
{
  CServiceBroker::GetRenderSystem()->CaptureStateBlock();
}

void CGraphicContext::ApplyStateBlock()
{
  CServiceBroker::GetRenderSystem()->ApplyStateBlock();
}

const RESOLUTION_INFO CGraphicContext::GetResInfo(RESOLUTION res) const
{
  RESOLUTION_INFO info = CDisplaySettings::GetInstance().GetResolutionInfo(res);

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

  if (res == m_Resolution && m_fFPSOverride != 0)
  {
    info.fRefreshRate = m_fFPSOverride;
  }

  return info;
}

void CGraphicContext::SetResInfo(RESOLUTION res, const RESOLUTION_INFO& info)
{
  RESOLUTION_INFO& curr = CDisplaySettings::GetInstance().GetResolutionInfo(res);
  curr.Overscan   = info.Overscan;
  curr.iSubtitles = info.iSubtitles;
  curr.fPixelRatio = info.fPixelRatio;
  curr.guiInsets = info.guiInsets;

  if(info.dwFlags & D3DPRESENTFLAG_MODE3DSBS)
  {
    curr.Overscan.right  = info.Overscan.right  * 2 + info.iBlanking;
    if((curr.dwFlags & D3DPRESENTFLAG_MODE3DSBS) == 0)
      curr.fPixelRatio /= 2.0f;
  }

  if(info.dwFlags & D3DPRESENTFLAG_MODE3DTB)
  {
    curr.Overscan.bottom = info.Overscan.bottom * 2 + info.iBlanking;
    curr.iSubtitles      = info.iSubtitles      * 2 + info.iBlanking;
    if((curr.dwFlags & D3DPRESENTFLAG_MODE3DTB) == 0)
      curr.fPixelRatio *= 2.0f;
  }
}

const RESOLUTION_INFO CGraphicContext::GetResInfo() const
{
  return GetResInfo(m_Resolution);
}

void CGraphicContext::GetGUIScaling(const RESOLUTION_INFO &res, float &scaleX, float &scaleY, TransformMatrix *matrix /* = NULL */)
{
  if (m_Resolution != RES_INVALID)
  {
    // calculate necessary scalings
    RESOLUTION_INFO info = GetResInfo();
    float fFromWidth  = (float)res.iWidth;
    float fFromHeight = (float)res.iHeight;
    auto fToPosX = info.Overscan.left + info.guiInsets.left;
    auto fToPosY = info.Overscan.top + info.guiInsets.top;
    auto fToWidth = info.Overscan.right - info.guiInsets.right - fToPosX;
    auto fToHeight = info.Overscan.bottom - info.guiInsets.bottom - fToPosY;

    float fZoom = (100 + CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_LOOKANDFEEL_SKINZOOM)) * 0.01f;

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
  m_origins.emplace(.0f, .0f);
  while (!m_cameras.empty())
    m_cameras.pop();
  m_cameras.emplace(0.5f * m_iScreenWidth, 0.5f * m_iScreenHeight);
  while (!m_stereoFactors.empty())
    m_stereoFactors.pop();
  m_stereoFactors.push(0.0f);

  // and reset the final transform
  m_finalTransform = m_guiTransform;
}

void CGraphicContext::SetRenderingResolution(const RESOLUTION_INFO &res, bool needsScaling)
{
  std::unique_lock<CCriticalSection> lock(*this);

  SetScalingResolution(res, needsScaling);
  UpdateCameraPosition(m_cameras.top(), m_stereoFactors.top());
}

void CGraphicContext::SetStereoView(RENDER_STEREO_VIEW view)
{
  m_stereoView = view;

  while(!m_viewStack.empty())
    m_viewStack.pop();

  CRect viewport(0.0f, 0.0f, (float)m_iScreenWidth, (float)m_iScreenHeight);

  m_viewStack.push(viewport);

  viewport = StereoCorrection(viewport);
  CServiceBroker::GetRenderSystem()->SetStereoMode(m_stereoMode, m_stereoView);
  CServiceBroker::GetRenderSystem()->SetViewPort(viewport);
  CServiceBroker::GetRenderSystem()->SetScissors(viewport);
}

void CGraphicContext::InvertFinalCoords(float &x, float &y) const
{
  m_finalTransform.matrix.InverseTransformPosition(x, y);
}

float CGraphicContext::ScaleFinalXCoord(float x, float y) const
{
  return m_finalTransform.matrix.TransformXCoord(x, y, 0);
}

float CGraphicContext::ScaleFinalYCoord(float x, float y) const
{
  return m_finalTransform.matrix.TransformYCoord(x, y, 0);
}

float CGraphicContext::ScaleFinalZCoord(float x, float y) const
{
  return m_finalTransform.matrix.TransformZCoord(x, y, 0);
}

void CGraphicContext::ScaleFinalCoords(float &x, float &y, float &z) const
{
  m_finalTransform.matrix.TransformPosition(x, y, z);
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
  UpdateCameraPosition(m_cameras.top(), m_stereoFactors.top());
}

void CGraphicContext::RestoreCameraPosition()
{ // remove the top camera from the stack
  assert(m_cameras.size());
  m_cameras.pop();
  UpdateCameraPosition(m_cameras.top(), m_stereoFactors.top());
}

void CGraphicContext::SetStereoFactor(float factor)
{
  m_stereoFactors.push(factor);
  UpdateCameraPosition(m_cameras.top(), m_stereoFactors.top());
}

void CGraphicContext::RestoreStereoFactor()
{ // remove the top factor from the stack
  assert(m_stereoFactors.size());
  m_stereoFactors.pop();
  UpdateCameraPosition(m_cameras.top(), m_stereoFactors.top());
}

float CGraphicContext::GetNormalizedDepth(uint32_t depth)
{
  float normalizedDepth = static_cast<float>(depth);
  normalizedDepth /= m_layer;
  normalizedDepth = normalizedDepth * 2 - 1;
  return normalizedDepth;
}

float CGraphicContext::GetTransformDepth(int32_t depthOffset)
{
  float depth = static_cast<float>(m_finalTransform.matrix.depth + depthOffset);
  depth /= m_layer;
  depth = depth * 2 - 1;
  return depth;
}

CRect CGraphicContext::GenerateAABB(const CRect &rect) const
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
  CServiceBroker::GetRenderSystem()->Project(x1, y1, z);

  z = 0.0f;
  ScaleFinalCoords(x2, y2, z);
  CServiceBroker::GetRenderSystem()->Project(x2, y2, z);

  z = 0.0f;
  ScaleFinalCoords(x3, y3, z);
  CServiceBroker::GetRenderSystem()->Project(x3, y3, z);

  z = 0.0f;
  ScaleFinalCoords(x4, y4, z);
  CServiceBroker::GetRenderSystem()->Project(x4, y4, z);

  return CRect( std::min(std::min(std::min(x1, x2), x3), x4),
                std::min(std::min(std::min(y1, y2), y3), y4),
                std::max(std::max(std::max(x1, x2), x3), x4),
                std::max(std::max(std::max(y1, y2), y3), y4));
}

// NOTE: This routine is currently called (twice) every time there is a <camera>
//       tag in the skin.  It actually only has to be called before we render
//       something, so another option is to just save the camera coordinates
//       and then have a routine called before every draw that checks whether
//       the camera has changed, and if so, changes it.  Similarly, it could set
//       the world transform at that point as well (or even combine world + view
//       to cut down on one setting)
void CGraphicContext::UpdateCameraPosition(const CPoint &camera, const float &factor)
{
  float stereoFactor = 0.f;
  if ( m_stereoMode != RENDER_STEREO_MODE_OFF
    && m_stereoMode != RENDER_STEREO_MODE_MONO
    && m_stereoView != RENDER_STEREO_VIEW_OFF)
  {
    RESOLUTION_INFO res = GetResInfo();
    RESOLUTION_INFO desktop = GetResInfo(RES_DESKTOP);
    float scaleRes = (static_cast<float>(res.iWidth) / static_cast<float>(desktop.iWidth));
    float scaleX = static_cast<float>(CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_LOOKANDFEEL_STEREOSTRENGTH)) * scaleRes;
    stereoFactor = factor * (m_stereoView == RENDER_STEREO_VIEW_LEFT ? scaleX : -scaleX);
  }
  CServiceBroker::GetRenderSystem()->SetCameraPosition(camera, m_iScreenWidth, m_iScreenHeight, stereoFactor);
}

bool CGraphicContext::RectIsAngled(float x1, float y1, float x2, float y2) const
{ // need only test 3 points, as they must be co-planer
  if (m_finalTransform.matrix.TransformZCoord(x1, y1, 0)) return true;
  if (m_finalTransform.matrix.TransformZCoord(x2, y2, 0)) return true;
  if (m_finalTransform.matrix.TransformZCoord(x1, y2, 0)) return true;
  return false;
}

const TransformMatrix &CGraphicContext::GetGUIMatrix() const
{
  return m_finalTransform.matrix;
}

float CGraphicContext::GetGUIScaleX() const
{
  return m_finalTransform.scaleX;
}

float CGraphicContext::GetGUIScaleY() const
{
  return m_finalTransform.scaleY;
}

UTILS::COLOR::Color CGraphicContext::MergeAlpha(UTILS::COLOR::Color color) const
{
  UTILS::COLOR::Color alpha = m_finalTransform.matrix.TransformAlpha((color >> 24) & 0xff);
  if (alpha > 255) alpha = 255;
  return ((alpha << 24) & 0xff000000) | (color & 0xffffff);
}

UTILS::COLOR::Color CGraphicContext::MergeColor(UTILS::COLOR::Color color) const
{
  return m_finalTransform.matrix.TransformColor(color);
}

int CGraphicContext::GetWidth() const
{
  return m_iScreenWidth;
}

int CGraphicContext::GetHeight() const
{
  return m_iScreenHeight;
}

float CGraphicContext::GetFPS() const
{
  if (m_Resolution != RES_INVALID)
  {
    RESOLUTION_INFO info = GetResInfo();
    if (info.fRefreshRate > 0)
      return info.fRefreshRate;
  }
  return 60.0f;
}

float CGraphicContext::GetDisplayLatency() const
{
  float latency = CServiceBroker::GetWinSystem()->GetDisplayLatency();
  if (latency < 0.0f)
  {
    // fallback
    latency = (CServiceBroker::GetWinSystem()->NoOfBuffers() + 1) / GetFPS() * 1000.0f;
  }

  return latency;
}

bool CGraphicContext::IsFullScreenRoot () const
{
  return m_bFullScreenRoot;
}

void CGraphicContext::ToggleFullScreen()
{
  RESOLUTION uiRes;

  if (m_bFullScreenRoot)
  {
    uiRes = RES_WINDOW;
  }
  else
  {
    if (CDisplaySettings::GetInstance().GetCurrentResolution() > RES_DESKTOP)
      uiRes = CDisplaySettings::GetInstance().GetCurrentResolution();
    else
      uiRes = RES_DESKTOP;
  }

  CDisplaySettings::GetInstance().SetCurrentResolution(uiRes, true);
}

void CGraphicContext::SetMediaDir(const std::string &strMediaDir)
{
  CServiceBroker::GetGUI()->GetTextureManager().SetTexturePath(strMediaDir);
  m_strMediaDir = strMediaDir;
}

const std::string& CGraphicContext::GetMediaDir() const
{
  return m_strMediaDir;

}

void CGraphicContext::Flip(bool rendered, bool videoLayer)
{
  CServiceBroker::GetRenderSystem()->PresentRender(rendered, videoLayer);

  if(m_stereoMode != m_nextStereoMode)
  {
    m_stereoMode = m_nextStereoMode;
    SetVideoResolution(GetVideoResolution(), true);
    CServiceBroker::GetGUI()->GetWindowManager().SendMessage(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_RENDERER_RESET);
  }
}

void CGraphicContext::GetAllowedResolutions(std::vector<RESOLUTION> &res)
{
  res.clear();

  res.push_back(RES_WINDOW);
  res.push_back(RES_DESKTOP);
  for (size_t r = (size_t) RES_CUSTOM; r < CDisplaySettings::GetInstance().ResolutionInfoSize(); r++)
  {
    res.push_back((RESOLUTION) r);
  }
}

void CGraphicContext::SetRenderOrder(RENDER_ORDER renderOrder)
{
  m_renderOrder = renderOrder;
  if (renderOrder == RENDER_ORDER_ALL_BACK_TO_FRONT)
    CServiceBroker::GetRenderSystem()->SetDepthCulling(DEPTH_CULLING_OFF);
  else if (renderOrder == RENDER_ORDER_BACK_TO_FRONT)
    CServiceBroker::GetRenderSystem()->SetDepthCulling(DEPTH_CULLING_BACK_TO_FRONT);
  else if (renderOrder == RENDER_ORDER_FRONT_TO_BACK)
    CServiceBroker::GetRenderSystem()->SetDepthCulling(DEPTH_CULLING_FRONT_TO_BACK);
}

uint32_t CGraphicContext::GetDepth(uint32_t addLayers)
{
  uint32_t layer = m_layer;
  m_layer += addLayers;
  return layer;
}

void CGraphicContext::SetFPS(float fps)
{
  m_fFPSOverride = fps;
}
