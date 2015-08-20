#pragma once
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

#ifndef HAS_DS_PLAYER
#error DSPlayer's header file included without HAS_DS_PLAYER defined
#endif

#include "cores/DSPlayer/Filters/MadvrSettings.h"
#include "DSUtil/DSUtil.h"
#include "guilib/Geometry.h"

enum MADVR_RENDER_LAYER
{
  RENDER_LAYER_ALL,
  RENDER_LAYER_UNDER,
  RENDER_LAYER_OVER
};

class IMadvrAllocatorCallback
{
public:
  virtual ~IMadvrAllocatorCallback() {};

  virtual bool IsEnteringExclusive(){ return false; };
  virtual void EnableExclusive(bool bEnable){};
  virtual void SetMadvrPixelShader(){};
  virtual void SetResolution(){};
  virtual bool ParentWindowProc(HWND hWnd, UINT uMsg, WPARAM *wParam, LPARAM *lParam, LRESULT *ret) { return false; };
  virtual void SetMadvrPosition(CRect wndRect, CRect videoRect) {};
};

class IMadvrPaintCallback
{
public:
  virtual ~IMadvrPaintCallback() {};

  virtual HRESULT RenderToTexture(MADVR_RENDER_LAYER layer){ return E_UNEXPECTED; };
  virtual void Flush(){};
};

class IMadvrSettingCallback
{
public:
  virtual ~IMadvrSettingCallback() {};

  virtual void LoadSettings(MADVR_LOAD_TYPE type){};
  virtual void RestoreSettings(){};
  virtual void GetProfileActiveName(std::string *profile){};
  virtual void SetStr(std::string path, std::string str){};
  virtual void SetBool(std::string path, bool bValue){};
  virtual void SetInt(std::string path, int iValue){};
  virtual void SetFloat(std::string path, float fValue, int iConv = 100){};
  virtual void SetDoubling(std::string path, int iValue){};
  virtual void SetDeintActive(std::string path, int iValue){};
  virtual void SetSmoothmotion(std::string path, int iValue){};
  virtual void SetDithering(std::string path, int iValue){};
};

class CMadvrCallback : public IMadvrAllocatorCallback, public IMadvrSettingCallback
{
public:

  /// Retrieve singleton instance
  static CMadvrCallback* Get();
  /// Destroy singleton instance
  static void Destroy()
  {
    delete m_pSingleton;
    m_pSingleton = NULL;
  }

  void Register(IMadvrAllocatorCallback* pAllocatorCallback) { m_pAllocatorCallback = pAllocatorCallback; }
  void Register(IMadvrSettingCallback* pSettingCallback) { m_pSettingCallback = pSettingCallback; }
  void Register(IMadvrPaintCallback* pPaintCallback) { m_pPaintCallback = pPaintCallback; }
  bool UsingMadvr();
  bool ReadyMadvr();
  bool IsInitMadvr() { return m_isInitMadvr; };
  void SetInitMadvr(bool b) { m_isInitMadvr = b; }
  bool GetRenderOnMadvr() { return m_renderOnMadvr; }
  void SetRenderOnMadvr(bool b) { m_renderOnMadvr = b; }
  void SetCurrentVideoLayer(MADVR_RENDER_LAYER layer) { m_currentVideoLayer = layer; }
  void IncRenderCount();
  void ResetRenderCount();
  bool GuiVisible(MADVR_RENDER_LAYER layer = RENDER_LAYER_ALL);
  
  // IMadvrAllocatorCallback
  virtual bool IsEnteringExclusive();
  virtual void EnableExclusive(bool bEnable);
  virtual void SetMadvrPixelShader();
  virtual void SetResolution();
  virtual bool ParentWindowProc(HWND hWnd, UINT uMsg, WPARAM *wParam, LPARAM *lParam, LRESULT *ret);
  virtual void SetMadvrPosition(CRect wndRect, CRect videoRect);

  // IMadvrPaintCallback
  virtual HRESULT RenderToTexture(MADVR_RENDER_LAYER layer);
  virtual void Flush();

  // IMadvrSettingCallback
  virtual void LoadSettings(MADVR_LOAD_TYPE type);
  virtual void RestoreSettings();
  virtual void GetProfileActiveName(std::string *profile);
  virtual void SetStr(std::string path, std::string str);
  virtual void SetBool(std::string path, bool bValue);
  virtual void SetInt(std::string path, int iValue);
  virtual void SetFloat(std::string path, float fValue, int iConv = 100);
  virtual void SetDoubling(std::string path, int iValue);
  virtual void SetDeintActive(std::string path, int iValue);
  virtual void SetSmoothmotion(std::string path, int iValue);
  virtual void SetDithering(std::string path, int iValue);

private:
  CMadvrCallback();
  ~CMadvrCallback();

  static CMadvrCallback* m_pSingleton;
  IMadvrAllocatorCallback* m_pAllocatorCallback;
  IMadvrSettingCallback* m_pSettingCallback;
  IMadvrPaintCallback* m_pPaintCallback;
  bool m_isInitMadvr;
  bool m_renderOnMadvr;
  int m_renderUnderCount;
  int m_renderOverCount;
  MADVR_RENDER_LAYER m_currentVideoLayer;
};
