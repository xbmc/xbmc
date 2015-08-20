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

#include "MadvrCallback.h"

CMadvrCallback *CMadvrCallback::m_pSingleton = NULL;

CMadvrCallback::CMadvrCallback()
{
  m_pAllocatorCallback = NULL;  
  m_pSettingCallback = NULL;
  m_pPaintCallback = NULL;
  m_renderOnMadvr = false;
  m_isInitMadvr = false;
  ResetRenderCount();
  m_currentVideoLayer = RENDER_LAYER_UNDER;
}

CMadvrCallback::~CMadvrCallback()
{
  m_pAllocatorCallback = NULL;
  m_pSettingCallback = NULL;
  m_pPaintCallback = NULL;
}

CMadvrCallback* CMadvrCallback::Get()
{
  return (m_pSingleton) ? m_pSingleton : (m_pSingleton = new CMadvrCallback());
}

void CMadvrCallback::IncRenderCount()
{ 
  if (!ReadyMadvr())
    return;

  m_currentVideoLayer == RENDER_LAYER_UNDER ? m_renderUnderCount += 1 : m_renderOverCount += 1;
}

void CMadvrCallback::ResetRenderCount()
{
  m_renderUnderCount = 0;  
  m_renderOverCount = 0;
}

bool CMadvrCallback::GuiVisible(MADVR_RENDER_LAYER layer)
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

bool CMadvrCallback::UsingMadvr()
{
  return m_pAllocatorCallback;
}

bool CMadvrCallback::ReadyMadvr()
{
  return (m_pAllocatorCallback && m_renderOnMadvr);
}

// IMadvrAllocatorCallback
bool CMadvrCallback::IsEnteringExclusive()
{ 
  if (UsingMadvr())
    return m_pAllocatorCallback->IsEnteringExclusive();

  return false;
}

void CMadvrCallback::EnableExclusive(bool bEnable)
{
  if (UsingMadvr())
    m_pAllocatorCallback->EnableExclusive(bEnable);
};

void CMadvrCallback::SetMadvrPixelShader()
{
  if (UsingMadvr())
    m_pAllocatorCallback->SetMadvrPixelShader();
};

void CMadvrCallback::SetResolution()
{
  if (UsingMadvr())
    m_pAllocatorCallback->SetResolution();
};

bool CMadvrCallback::ParentWindowProc(HWND hWnd, UINT uMsg, WPARAM *wParam, LPARAM *lParam, LRESULT *ret) 
{ 
  if (UsingMadvr())
    return m_pAllocatorCallback->ParentWindowProc(hWnd,uMsg,wParam,lParam,ret);

  return false; 
}

void CMadvrCallback::SetMadvrPosition(CRect wndRect, CRect videoRect) 
{
  if (UsingMadvr())
    m_pAllocatorCallback->SetMadvrPosition(wndRect, videoRect);
};

// IMadvrPaintCallback
HRESULT CMadvrCallback::RenderToTexture(MADVR_RENDER_LAYER layer)
{
  if (m_pPaintCallback && ReadyMadvr())
    return m_pPaintCallback->RenderToTexture(layer);

  return E_UNEXPECTED;
};

void CMadvrCallback::Flush()
{
  if (m_pPaintCallback && ReadyMadvr())
    m_pPaintCallback->Flush();
};

// IMadvrSettingCallback
void CMadvrCallback::RestoreSettings()
{
  if (m_pSettingCallback)
    m_pSettingCallback->RestoreSettings();
};

void CMadvrCallback::LoadSettings(MADVR_LOAD_TYPE type)
{
  if (m_pSettingCallback)
    m_pSettingCallback->LoadSettings(type);
};

void CMadvrCallback::GetProfileActiveName(std::string *profile)
{
  if (m_pSettingCallback)
    m_pSettingCallback->GetProfileActiveName(profile);
};

void CMadvrCallback::SetStr(CStdString path, CStdString sValue) 
{
  if (m_pSettingCallback)
    m_pSettingCallback->SetStr(path, sValue);
};

void CMadvrCallback::SetBool(CStdString path, bool bValue) 
{
  if (m_pSettingCallback)
    m_pSettingCallback->SetBool(path, bValue);
};
void CMadvrCallback::SetInt(CStdString path, int iValue) 
{
  if (m_pSettingCallback)
    m_pSettingCallback->SetInt(path, iValue);
};

void CMadvrCallback::SetFloat(CStdString path, float fValue, int iConv) 
{
  if (m_pSettingCallback)
    m_pSettingCallback->SetFloat(path, fValue);
};

void CMadvrCallback::SetDoubling(CStdString path, int iValue) 
{
  if (m_pSettingCallback)
    m_pSettingCallback->SetDoubling(path, iValue);
};

void CMadvrCallback::SetDeintActive(CStdString path, int iValue) 
{
  if (m_pSettingCallback)
    m_pSettingCallback->SetDeintActive(path, iValue);
};

void CMadvrCallback::SetSmoothmotion(CStdString path, int iValue) 
{
  if (m_pSettingCallback)
    m_pSettingCallback->SetSmoothmotion(path, iValue);
};

void CMadvrCallback::SetDithering(CStdString path, int iValue) 
{
  if (m_pSettingCallback)
    m_pSettingCallback->SetDithering(path, iValue);
};


#endif