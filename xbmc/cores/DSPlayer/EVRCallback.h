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

enum EVR_RENDER_LAYER
{
  EVR_LAYER_ALL,
  EVR_LAYER_UNDER,
  EVR_LAYER_OVER
};


class IEvrAllocatorCallback
{
public:
  virtual ~IEvrAllocatorCallback() {};
};

class IEvrPaintCallback
{
public:
  virtual ~IEvrPaintCallback() {};

  virtual void RenderToUnderTexture(){};
  virtual void RenderToOverTexture(){};
  virtual void EndRender(){};
};


class CEvrCallback : public IEvrAllocatorCallback, public IEvrPaintCallback
{
public:

  /// Retrieve singleton instance
  static CEvrCallback* Get();
  /// Destroy singleton instance
  static void Destroy()
  {
    delete m_pSingleton;
    m_pSingleton = NULL;
  }
  
  // IEvrAllocatorCallback

  // IEvrPaintCallback
  virtual void RenderToUnderTexture();
  virtual void RenderToOverTexture();
  virtual void EndRender();


  void Register(IEvrAllocatorCallback* pAllocatorCallback) { m_pAllocatorCallback = pAllocatorCallback; }
  void Register(IEvrPaintCallback* pPaintCallback) { m_pPaintCallback = pPaintCallback; }
  bool UsingEvr();
  bool ReadyEvr();
  bool GetRenderOnEvr() { return m_renderOnEvr; }
  void SetRenderOnEvr(bool b) { m_renderOnEvr = b; }
  void IncRenderCount();
  void ResetRenderCount();
  void SetCurrentVideoLayer(EVR_RENDER_LAYER layer) { m_currentVideoLayer = layer; }
  bool GuiVisible(EVR_RENDER_LAYER layer = EVR_LAYER_ALL);

private:
  CEvrCallback();
  ~CEvrCallback();

  static CEvrCallback* m_pSingleton;
  IEvrAllocatorCallback* m_pAllocatorCallback;
  IEvrPaintCallback* m_pPaintCallback;
  bool m_renderOnEvr;
  int m_renderUnderCount;
  int m_renderOverCount;
  EVR_RENDER_LAYER m_currentVideoLayer;
};
