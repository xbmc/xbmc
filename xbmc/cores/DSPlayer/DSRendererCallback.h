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


#include "DSUtil/DSUtil.h"
#include "guilib/Geometry.h"
#include "settings/lib/SettingsManager.h"
#include "settings/lib/SettingDefinitions.h"

enum DS_RENDER_LAYER
{
  RENDER_LAYER_ALL,
  RENDER_LAYER_UNDER,
  RENDER_LAYER_OVER
};

enum MADVR_GUI_SETTINGS
{
  KODIGUI_NEVER,
  KODIGUI_LOAD_DSPLAYER,
  KODIGUI_LOAD_MADVR
};

enum MADVR_RES_SETTINGS
{
  MADVR_RES_SD = 480,
  MADVR_RES_720 = 720,
  MADVR_RES_1080 = 1080,
  MADVR_RES_2160 = 2160,
  MADVR_RES_ALL = 0,
  MADVR_RES_TVSHOW,
  MADVR_RES_DEFAULT
};

enum DIRECTSHOW_RENDERER
{
  DIRECTSHOW_RENDERER_VMR9 = 1,
  DIRECTSHOW_RENDERER_EVR = 2,
  DIRECTSHOW_RENDERER_MADVR = 3,
  DIRECTSHOW_RENDERER_UNDEF = 4
};

class IDSRendererAllocatorCallback
{
public:
  virtual ~IDSRendererAllocatorCallback() {};

  virtual CRect GetActiveVideoRect(){ CRect activeVideoRect(0, 0, 0, 0); return activeVideoRect; };
  virtual bool IsEnteringExclusive(){ return false; };
  virtual void EnableExclusive(bool bEnable){};
  virtual void SetMadvrPixelShader(){};
  virtual void SetResolution(){};
  virtual bool ParentWindowProc(HWND hWnd, UINT uMsg, WPARAM *wParam, LPARAM *lParam, LRESULT *ret) { return false; };
  virtual void SetMadvrPosition(CRect wndRect, CRect videoRect) {};
  virtual CRect GetMadvrRect(){ CRect madvrRect(0, 0, 0, 0); return madvrRect; };
};

class IDSRendererPaintCallback
{
public:
  virtual ~IDSRendererPaintCallback() {};

  virtual void RenderToUnderTexture(){};
  virtual void RenderToOverTexture(){};
  virtual void EndRender(){};
};

class IMadvrSettingCallback
{
public:
  virtual ~IMadvrSettingCallback() {};

  virtual void LoadSettings(int iSectionId){};
  virtual void RestoreSettings(){};
  virtual void GetProfileActiveName(const std::string &path, std::string *profile){};
  virtual void OnSettingChanged(int iSectionId, CSettingsManager* settingsManager, const CSetting *setting){};
  virtual void AddDependencies(const std::string &xml, CSettingsManager *settingsManager, CSetting *setting){}
  virtual void ListSettings(const std::string &path){};
};

class CDSRendererCallback : public IDSRendererAllocatorCallback, public IDSRendererPaintCallback, public IMadvrSettingCallback
{
public:

  /// Retrieve singleton instance
  static CDSRendererCallback* Get();
  /// Destroy singleton instance
  static void Destroy()
  {
    delete m_pSingleton;
    m_pSingleton = NULL;
  }
  
  // IDSRendererAllocatorCallback
  virtual CRect GetActiveVideoRect();
  
  // IDSRendererAllocatorCallback (madVR)
  virtual bool IsEnteringExclusive();
  virtual void EnableExclusive(bool bEnable);
  virtual void SetMadvrPixelShader();
  virtual void SetResolution();
  virtual bool ParentWindowProc(HWND hWnd, UINT uMsg, WPARAM *wParam, LPARAM *lParam, LRESULT *ret);
  virtual void SetMadvrPosition(CRect wndRect, CRect videoRect);
  virtual CRect GetMadvrRect();

  // IDSRendererPaintCallback
  virtual void RenderToUnderTexture();
  virtual void RenderToOverTexture();
  virtual void EndRender();

  // IMadvrSettingCallback
  virtual void LoadSettings(int iSectionId);
  virtual void RestoreSettings();
  virtual void GetProfileActiveName(const std::string &path, std::string *profile);
  virtual void OnSettingChanged(int iSectionId, CSettingsManager* settingsManager, const CSetting *setting);
  virtual void AddDependencies(const std::string &xml, CSettingsManager *settingsManager, CSetting *setting);
  virtual void ListSettings(const std::string &path);

  void Register(IDSRendererAllocatorCallback* pAllocatorCallback) { m_pAllocatorCallback = pAllocatorCallback; }
  void Register(IMadvrSettingCallback* pSettingCallback) { m_pSettingCallback = pSettingCallback; }
  void Register(IDSRendererPaintCallback* pPaintCallback) { m_pPaintCallback = pPaintCallback; }
  bool UsingDS(DIRECTSHOW_RENDERER renderer = DIRECTSHOW_RENDERER_UNDEF);
  bool ReadyDS(DIRECTSHOW_RENDERER renderer = DIRECTSHOW_RENDERER_UNDEF);
  bool GetRenderOnDS() { return m_renderOnDs; }
  void SetRenderOnDS(bool b) { m_renderOnDs = b; }
  void SetCurrentVideoLayer(DS_RENDER_LAYER layer) { m_currentVideoLayer = layer; }
  void IncRenderCount();
  void ResetRenderCount();
  bool GuiVisible(DS_RENDER_LAYER layer = RENDER_LAYER_ALL);
  int VideoDimsToResolution(int iWidth, int iHeight);
  DIRECTSHOW_RENDERER GetCurrentRenderer() { return m_CurrentRenderer; }
  void SetCurrentRenderer(DIRECTSHOW_RENDERER renderer) { m_CurrentRenderer = renderer; }
  bool GetStop(){ return m_bStop; }
  void SetStop(bool bStop){ m_bStop = bStop; }
  void SetVisibleScreenArea(CRect activeVideoRect);

private:
  CDSRendererCallback();
  ~CDSRendererCallback();

  static CDSRendererCallback* m_pSingleton;
  IDSRendererAllocatorCallback* m_pAllocatorCallback;
  IMadvrSettingCallback* m_pSettingCallback;
  IDSRendererPaintCallback* m_pPaintCallback;
  bool m_renderOnDs;
  bool m_bStop;
  int m_renderUnderCount;
  int m_renderOverCount;
  DS_RENDER_LAYER m_currentVideoLayer;
  DIRECTSHOW_RENDERER m_CurrentRenderer;
};
