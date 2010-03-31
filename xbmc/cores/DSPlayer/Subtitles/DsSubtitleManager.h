#ifndef DSSUBTITLEMANAGER_H
#define DSSUBTITLEMANAGER_H
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
#pragma once

#include "streams.h"
#include "Subtitles/DllLibMpcSubs.h"
#include "GraphicContext.h"
#include "WindowingFactory.h"
#include "CharsetConverter.h"

class CSubManager;

class CDsSubManager
{
public:
  CDsSubManager():
      m_bCurrentlyEnabled(false)
  {
  }
  bool Load()
  {
    if (!m_dllMpcSubs.IsLoaded())
      return m_dllMpcSubs.Load();

    return true;
  }
  BOOL LoadSubtitles(const CStdString& fn, IGraphBuilder* pGB, const CStdString& paths, ISubManager** manager)
  {
    SIZE tmpSize;
    tmpSize.cx = g_graphicsContext.GetWidth();
    tmpSize.cy = g_graphicsContext.GetHeight();
    InitDefaultStyle();

    CStdStringW pFilePath,pSubPath;
    g_charsetConverter.subtitleCharsetToW(fn, pFilePath);
    g_charsetConverter.subtitleCharsetToW(paths, pSubPath);

    return m_dllMpcSubs.LoadSubtitles(g_Windowing.Get3DDevice(), tmpSize, pFilePath.c_str(), pGB, L".\\,.\\Subtitles\\", manager);
  }
  void EnableSubtitle(bool enable)
  {
    if (m_dllMpcSubs.IsLoaded())
    {
      m_dllMpcSubs.SetEnable(enable);
      m_bCurrentlyEnabled = enable;
    }
  }
  void SetCurrent(int i)
  {
    if (m_dllMpcSubs.IsLoaded())
    {
      m_dllMpcSubs.SetCurrent(i);
    }
  }
  bool Enabled() { return m_bCurrentlyEnabled; }
  void InitDefaultStyle()
  {
    m_pCurrentStyle.borderWidth = 2;
    m_pCurrentStyle.fontCharset = 1;
    m_pCurrentStyle.fontColor = 16777215;
    m_pCurrentStyle.fontIsBold = true;
    m_pCurrentStyle.fontName = L"Arial";
    m_pCurrentStyle.fontSize = 18;
    m_pCurrentStyle.isBorderOutline = true;
    m_pCurrentStyle.shadow = 3;
    m_dllMpcSubs.SetDefaultStyle(&m_pCurrentStyle, false);
  }
  void GetSubtitlesList()
  {
    int count = m_dllMpcSubs.GetCount();

    CStdString strLangA;

    for (int xx =1 ; xx < count ; xx++)
    {
      g_charsetConverter.wToUTF8(CStdStringW(m_dllMpcSubs.GetLanguage(xx)),strLangA);
      CLog::Log(LOGNOTICE,"%s",strLangA.c_str()); 
    }
  }
  void Render(int x, int y, int width, int height)
  {
    m_dllMpcSubs.Render(x,y,width,height);
  }

protected:
  DllLibMpcSubs m_dllMpcSubs;
  bool m_bCurrentlyEnabled;
  SubtitleStyle_t m_pCurrentStyle;
};

extern class CDsSubManager g_dllMpcSubs;

#endif