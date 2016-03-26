/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
 *
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

#ifdef HAS_DS_PLAYER

#include "DSRendererCallback.h"
#include "cores/DSPlayer/Filters/MadvrSettingsManager.h"
#include "guilib/GraphicContext.h"
#include "settings/Settings.h"

CDSRendererCallback *CDSRendererCallback::m_pSingleton = NULL;

CDSRendererCallback::CDSRendererCallback()
{
  m_CurrentRenderer = DIRECTSHOW_RENDERER_UNDEF;
  m_pAllocatorCallback = NULL;  
  m_pSettingCallback = NULL;
  m_pPaintCallback = NULL;
  m_renderOnDs = false;
  m_bStop = false;
  ResetRenderCount();
  m_currentVideoLayer = RENDER_LAYER_UNDER;
}

CDSRendererCallback::~CDSRendererCallback()
{
  CRect activeVideoRect(0,0,0,0);
  if (m_pAllocatorCallback)
    activeVideoRect = m_pAllocatorCallback->GetActiveVideoRect();
  SetVisibleScreenArea(activeVideoRect);

  m_pAllocatorCallback = NULL;
  m_pSettingCallback = NULL;
  m_pPaintCallback = NULL;
}

CDSRendererCallback* CDSRendererCallback::Get()
{
  return (m_pSingleton) ? m_pSingleton : (m_pSingleton = new CDSRendererCallback());
}

void CDSRendererCallback::IncRenderCount()
{ 
  if (!ReadyDS())
    return;

  m_currentVideoLayer == RENDER_LAYER_UNDER ? m_renderUnderCount += 1 : m_renderOverCount += 1;
}

void CDSRendererCallback::ResetRenderCount()
{
  m_renderUnderCount = 0;  
  m_renderOverCount = 0;
}

bool CDSRendererCallback::GuiVisible(DS_RENDER_LAYER layer)
{
  bool result = false;
  switch (layer)
  {
  case RENDER_LAYER_UNDER:
    result = m_renderUnderCount > 0;
    break;
  case RENDER_LAYER_OVER:
    result = m_renderOverCount > 0;
    break;
  case RENDER_LAYER_ALL:
    result = m_renderOverCount + m_renderUnderCount > 0;
    break;
  }
  return result;
}

int CDSRendererCallback::VideoDimsToResolution(int iWidth, int iHeight)
{
  int res = 0;
  int madvr_res = -1;

  if (iWidth == 0 || iHeight == 0)
    res = 0;
  else if (iWidth <= 720 && iHeight <= 480)
    res = 480;
  // 720x576 (PAL) (768 when rescaled for square pixels)
  else if (iWidth <= 768 && iHeight <= 576)
    res = 576;
  // 960x540 (sometimes 544 which is multiple of 16)
  else if (iWidth <= 960 && iHeight <= 544)
    res = 540;
  // 1280x720
  else if (iWidth <= 1280 && iHeight <= 720)
    res = 720;
  // 1920x1080
  else if (iWidth <= 1920 && iHeight <= 1080)
    res = 1080;
  // 4K
  else if (iWidth * iHeight >= 6000000)
    res = 2160;
  else
    res = 0;

  if (res == 480 || res == 540 || res == 576)
    madvr_res = MADVR_RES_SD;

  if (res == 720)
    madvr_res = MADVR_RES_720;

  if (res == 1080)
    madvr_res = MADVR_RES_1080;

  if (res == 2160)
    madvr_res = MADVR_RES_2160;

  return madvr_res;
}

bool CDSRendererCallback::UsingDS(DIRECTSHOW_RENDERER renderer)
{
  if (renderer == DIRECTSHOW_RENDERER_UNDEF)
    renderer = m_CurrentRenderer;

  return (m_pAllocatorCallback != NULL && m_CurrentRenderer == renderer);
}

bool CDSRendererCallback::ReadyDS(DIRECTSHOW_RENDERER renderer)
{
  if (renderer == DIRECTSHOW_RENDERER_UNDEF)
    renderer = m_CurrentRenderer;

  return (m_pAllocatorCallback != NULL && m_renderOnDs && m_CurrentRenderer == renderer);
}

void CDSRendererCallback::SetVisibleScreenArea(CRect activeVideoRect)
{
  if (CSettings::GetInstance().GetBool(CSettings::SETTING_DSPLAYER_OSDINTOACTIVEAREA)
    && CSettings::GetInstance().GetBool(CSettings::SETTING_DSPLAYER_COPYACTIVERECT))
  {
    CRect wndRect = g_graphicsContext.GetViewWindow();
    CSettings::GetInstance().SetInt(CSettings::SETTING_DSPLAYER_DSAREALEFT, activeVideoRect.x1 - wndRect.x1);
    CSettings::GetInstance().SetInt(CSettings::SETTING_DSPLAYER_DSAREARIGHT, wndRect.x2 - activeVideoRect.x2);
    CSettings::GetInstance().SetInt(CSettings::SETTING_DSPLAYER_DSAREATOP, activeVideoRect.y1 - wndRect.y1);
    CSettings::GetInstance().SetInt(CSettings::SETTING_DSPLAYER_DSAREABOTTOM, wndRect.y2 - activeVideoRect.y2);
  }
}

// IDSRendererAllocatorCallback

CRect CDSRendererCallback::GetActiveVideoRect()
{
  CRect activeVideoRect(0, 0, 0, 0);

  if (ReadyDS())
    activeVideoRect = m_pAllocatorCallback->GetActiveVideoRect();

  return activeVideoRect;
}

bool CDSRendererCallback::IsEnteringExclusive()
{ 
  if (UsingDS(DIRECTSHOW_RENDERER_MADVR))
    return m_pAllocatorCallback->IsEnteringExclusive();

  return false;
}

void CDSRendererCallback::EnableExclusive(bool bEnable)
{
  if (UsingDS(DIRECTSHOW_RENDERER_MADVR))
    m_pAllocatorCallback->EnableExclusive(bEnable);
}

void CDSRendererCallback::SetMadvrPixelShader()
{
  if (UsingDS(DIRECTSHOW_RENDERER_MADVR))
    m_pAllocatorCallback->SetMadvrPixelShader();
}

void CDSRendererCallback::SetResolution()
{
  if (UsingDS(DIRECTSHOW_RENDERER_MADVR))
    m_pAllocatorCallback->SetResolution();
}

bool CDSRendererCallback::ParentWindowProc(HWND hWnd, UINT uMsg, WPARAM *wParam, LPARAM *lParam, LRESULT *ret) 
{ 
  if (UsingDS(DIRECTSHOW_RENDERER_MADVR))
    return m_pAllocatorCallback->ParentWindowProc(hWnd,uMsg,wParam,lParam,ret);

  return false; 
}

void CDSRendererCallback::SetMadvrPosition(CRect wndRect, CRect videoRect) 
{
  if (UsingDS(DIRECTSHOW_RENDERER_MADVR))
    m_pAllocatorCallback->SetMadvrPosition(wndRect, videoRect);
}

CRect CDSRendererCallback::GetMadvrRect()
{
  CRect madvrRect(0, 0, 0, 0);

  if (UsingDS(DIRECTSHOW_RENDERER_MADVR))
    madvrRect = m_pAllocatorCallback->GetMadvrRect();

  return madvrRect;
}

// IDSRendererPaintCallback
void CDSRendererCallback::RenderToUnderTexture()
{
  if (m_pPaintCallback && ReadyDS())
    m_pPaintCallback->RenderToUnderTexture();
}

void CDSRendererCallback::RenderToOverTexture()
{
  if (m_pPaintCallback && ReadyDS())
    m_pPaintCallback->RenderToOverTexture();
}

void CDSRendererCallback::EndRender()
{
  if (m_pPaintCallback && ReadyDS())
    m_pPaintCallback->EndRender();
}

// IMadvrSettingCallback
void CDSRendererCallback::RestoreSettings()
{
  if (m_pSettingCallback)
    m_pSettingCallback->RestoreSettings();
}

void CDSRendererCallback::LoadSettings(int iSectionId)
{
  if (m_pSettingCallback)
    m_pSettingCallback->LoadSettings(iSectionId);
}

void CDSRendererCallback::GetProfileActiveName(const std::string &path, std::string *profile)
{
  if (m_pSettingCallback)
    m_pSettingCallback->GetProfileActiveName(path, profile);
}
void  CDSRendererCallback::OnSettingChanged(int iSectionId, CSettingsManager* settingsManager, const CSetting *setting)
{
  if (m_pSettingCallback)
    m_pSettingCallback->OnSettingChanged(iSectionId, settingsManager, setting);
}

void CDSRendererCallback::AddDependencies(const std::string &xml, CSettingsManager *settingsManager, CSetting *setting)
{
  if (m_pSettingCallback)
    m_pSettingCallback->AddDependencies(xml, settingsManager, setting);
}

void CDSRendererCallback::ListSettings(const std::string &path)
{
  if (m_pSettingCallback)
    m_pSettingCallback->ListSettings(path);
}

#endif