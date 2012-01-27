#pragma once

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

#include "threads/Thread.h"
#include "tinyXML/tinyxml.h"
#include "guilib/GUILabelControl.h"  // for CInfoPortion

class TiXmlNode;

#define MAX_ROWS 20

class ILCD : public CThread
{
private:
  enum DISABLE_ON_PLAY { DISABLE_ON_PLAY_NONE = 0, DISABLE_ON_PLAY_VIDEO = 1, DISABLE_ON_PLAY_MUSIC = 2 };

public:
  enum LCD_MODE {
                        LCD_MODE_GENERAL = 0,
                        LCD_MODE_MUSIC,
                        LCD_MODE_VIDEO,
                        LCD_MODE_NAVIGATION,
                        LCD_MODE_SCREENSAVER,
                        LCD_MODE_XBE_LAUNCH,
                        LCD_MODE_MAX
                };
  enum CUSTOM_CHARSET {
                        CUSTOM_CHARSET_DEFAULT = 0,
                        CUSTOM_CHARSET_SMALLCHAR,
                        CUSTOM_CHARSET_MEDIUMCHAR,
                        CUSTOM_CHARSET_BIGCHAR,
                        CUSTOM_CHARSET_MAX
                };
  virtual void Initialize();
  virtual bool IsConnected();
  virtual void Stop() = 0;
  virtual void Suspend() = 0;
  virtual void Resume() = 0;
  virtual void SetBackLight(int iLight) = 0;
  virtual void SetContrast(int iContrast) = 0;
  virtual int  GetColumns() = 0;
  virtual int  GetRows() = 0;
  virtual void SetLine(int iLine, const CStdString& strLine) = 0;
  virtual void DisableOnPlayback(bool playingVideo, bool playingMusic);
  CStdString GetProgressBar(double tCurrent, double tTotal);
  void SetCharset( UINT nCharset );
  CStdString GetBigDigit( UINT _nCharset, int _nDigit, UINT _nLine, UINT _nMinSize, UINT _nMaxSize, bool _bSpacePadding );
  void LoadSkin(const CStdString &xmlFile);
  void Reset();
  void Render(LCD_MODE mode);
  ILCD() : m_disableOnPlay(DISABLE_ON_PLAY_NONE), 
           m_eCurrentCharset(CUSTOM_CHARSET_DEFAULT) {}
protected:
  virtual void Process() = 0;
  void StringToLCDCharSet(CStdString& strText);
  unsigned char GetLCDCharsetCharacter( UINT _nCharacter, int _nCharset=-1);
  void LoadMode(TiXmlNode *node, LCD_MODE mode);

private:
  int m_disableOnPlay;

  std::vector<CGUIInfoLabel> m_lcdMode[LCD_MODE_MAX];
  UINT m_eCurrentCharset;
};
extern ILCD* g_lcd;
