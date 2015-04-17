/*
*      Copyright (C) 2005-2010 Team XBMC
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

#pragma once

#ifndef HAS_DS_PLAYER
#error DSPlayer's header file included without HAS_DS_PLAYER defined
#endif
/*MADVR*/
#include "guilib/Geometry.h"

class IPaintCallback
{
public:
  virtual ~IPaintCallback() {};

  virtual void OnPaint(CRect destRect) = 0;
  virtual void OnAfterPresent() = 0;
};

class IPaintCallbackMadvr
{
public:
  virtual ~IPaintCallbackMadvr() {};

  virtual LPDIRECT3DDEVICE9 GetDevice() { return NULL; };
  virtual bool IsDeviceSet(){ return false; }
  virtual bool IsEnteringExclusive(){ return false; }
  virtual void SetIsDevice(bool b){};
  virtual void OsdRedrawFrame() {};
  virtual void SetMadvrPixelShader(){};
  virtual void RestoreMadvrSettings(){};
  virtual void SetStartMadvr(){};
  virtual void SetMadvrPosition(CRect wndRect, CRect videoRect) {};
  virtual void SettingSetScaling(CStdStringW path, int scaling) {};
  virtual void SettingSetDoubling(CStdStringW path, int iValue) {};
  virtual void SettingSetDoublingCondition(CStdStringW path, int condition) {};
  virtual void SettingSetQuadrupleCondition(CStdStringW path, int condition) {};
  virtual void SettingSetDeintActive(CStdStringW path, int iValue) {};
  virtual void SettingSetDeintForce(CStdStringW path, int iValue) {};
  virtual void SettingSetSmoothmotion(CStdStringW path, int iValue) {};
  virtual void SettingSetDithering(CStdStringW path, int iValue) {};
  virtual void SettingSetBool(CStdStringW path, BOOL bValue) {};
  virtual void SettingSetInt(CStdStringW path, int iValue) {};
  virtual CStdString GetDXVADecoderDescription() { return ""; };
};




