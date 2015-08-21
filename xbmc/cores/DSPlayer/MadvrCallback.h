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
#include "settings/lib/SettingDefinitions.h"

// DEFAULT SETTINGS
const int MADVR_DEFAULT_CHROMAUP = 7; // BICUBIC75
const int MADVR_DEFAULT_LUMAUP = 14; // LANCZOS3
const int MADVR_DEFAULT_LUMADOWN = 4; // CATMUL-ROM
const int MADVR_DEFAULT_DOUBLEFACTOR = 1; // 1.5x
const int MADVR_DEFAULT_QUADRUPLEFACTOR =  1; // 3.0x
const int MADVR_DEFAULT_DEINTFORCE = 0; // AUTO
const int MADVR_DEFAULT_DEINTACTIVE = 1; // DEACTIVE
const int MADVR_DEFAULT_DITHERING = 1; // ORDERED
const int MADVR_DEFAULT_DEBAND_LEVEL = 0; // LOW
const int MADVR_DEFAULT_DEBAND_FADELEVEL = 2; // HIGH

const float MADVR_DEFAULT_FINESHARPSTRENGTH = 2.0f;
const float MADVR_DEFAULT_LUMASHARPENSTRENGTH = 0.65f;
const float MADVR_DEFAULT_ADAPTIVESHARPENSTRENGTH = 0.5f;
const float MADVR_DEFAULT_UPFINESHARPSTRENGTH = 2.0f;
const float MADVR_DEFAULT_UPLUMASHARPENSTRENGTH = 0.65f;
const float MADVR_DEFAULT_UPADAPTIVESHARPENSTRENGTH = 0.5f;
const float MADVR_DEFAULT_SUPERRESSTRENGTH = 1.0f;


enum MADVR_RENDER_LAYER
{
  RENDER_LAYER_ALL,
  RENDER_LAYER_UNDER,
  RENDER_LAYER_OVER
};

enum MADVR_SETTINGS_LIST
{
  MADVR_LIST_CHROMAUP,
  MADVR_LIST_LUMAUP,
  MADVR_LIST_LUMADOWN,
  MADVR_LIST_DOUBLEQUALITY,
  MADVR_LIST_DOUBLEFACTOR,
  MADVR_LIST_QUADRUPLEFACTOR,
  MADVR_LIST_DEINTFORCE,
  MADVR_LIST_DEINTACTIVE,
  MADVR_LIST_SMOOTHMOTION,
  MADVR_LIST_DITHERING,
  MADVR_LIST_DEBAND,
  MADVR_LIST_DEBAND_LEVEL,
  MADVR_LIST_DEBAND_FADELEVEL
};

enum MADVR_LOAD_TYPE
{
  MADVR_LOAD_GENERAL,
  MADVR_LOAD_SCALING
};

enum MADVR_GUI_SETTINGS
{
  KODIGUI_NEVER,
  KODIGUI_LOAD_DSPLAYER,
  KODIGUI_LOAD_MADVR
};

enum MADVR_RES_SETTINGS
{
  MADVR_RES_SD,
  MADVR_RES_720,
  MADVR_RES_1080,
  MADVR_RES_2160,
  MADVR_RES_ALL
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
  virtual std::string GetSettingsName(MADVR_SETTINGS_LIST type, int iValue){ return ""; };
  virtual void AddEntry(MADVR_SETTINGS_LIST type, StaticIntegerSettingOptions *entry){};
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
  virtual std::string GetSettingsName(MADVR_SETTINGS_LIST type, int iValue);
  virtual void AddEntry(MADVR_SETTINGS_LIST type, StaticIntegerSettingOptions *entry);

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
