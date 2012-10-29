#pragma once

/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "threads/Thread.h"
#include "utils/XBMCTinyXML.h"
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
                        LCD_MODE_PVRTV,
                        LCD_MODE_PVRRADIO,
                        LCD_MODE_MAX
                };
  enum CUSTOM_CHARSET {
                        CUSTOM_CHARSET_DEFAULT = 0,
                        CUSTOM_CHARSET_SMALLCHAR,
                        CUSTOM_CHARSET_MEDIUMCHAR,
                        CUSTOM_CHARSET_BIGCHAR,
                        CUSTOM_CHARSET_MAX
                };
  enum LCD_RESOLUTION_INDICATOR {
                        LCD_RESOLUTION_INDICATOR_NONE = 0,
                        LCD_RESOLUTION_INDICATOR_SD,
                        LCD_RESOLUTION_INDICATOR_HD
                };
  enum LCD_CHARSET_CONVTABS {
                        LCD_CHARSET_TAB_HD44780 = 0,
                        LCD_CHARSET_TAB_KS0073 = 1,
                        LCD_CHARSET_TAB_IMONMDM = 2
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
 ILCD() : CThread("ILCD"),
          m_disableOnPlay(DISABLE_ON_PLAY_NONE), 
          m_eCurrentCharset(CUSTOM_CHARSET_DEFAULT) {}
protected:
  virtual void Process() = 0;
  void StringToLCDCharSet(CStdString& strText, unsigned int iCharsetTab);
  unsigned char GetLCDCharsetCharacter( UINT _nCharacter, int _nCharset=-1);
  void LoadMode(TiXmlNode *node, LCD_MODE mode);

  virtual bool SendIconStatesToDisplay(void) = 0;
  virtual void SetIconMovie(bool on) = 0;
  virtual void SetIconMusic(bool on) = 0;
  virtual void SetIconWeather(bool on) = 0;
  virtual void SetIconTV(bool on) = 0;
  virtual void SetIconPhoto(bool on) = 0;
  virtual void SetIconResolution(LCD_RESOLUTION_INDICATOR resolution) = 0;
  virtual void SetProgressBar1(double progress) = 0;
  virtual void SetProgressBar2(double progress) = 0;
  virtual void SetProgressBar3(double progress) = 0;
  virtual void SetProgressBar4(double progress) = 0;
  virtual void SetIconMute(bool on) = 0;
  virtual void SetIconPlaying(bool on) = 0;
  virtual void SetIconPause(bool on) = 0;
  virtual void SetIconRepeat(bool on) = 0;
  virtual void SetIconShuffle(bool on) = 0;
  virtual void SetIconAlarm(bool on) = 0;
  virtual void SetIconRecord(bool on) = 0;
  virtual void SetIconVolume(bool on) = 0;
  virtual void SetIconTime(bool on) = 0;
  virtual void SetIconSPDIF(bool on) = 0;
  virtual void SetIconDiscIn(bool on) = 0;
  virtual void SetIconSource(bool on) = 0;
  virtual void SetIconFit(bool on) = 0;
  virtual void SetIconSCR1(bool on) = 0;
  virtual void SetIconSCR2(bool on) = 0;
  // codec icons - video: video stream format
  virtual void SetIconMPEG(bool on) = 0;
  virtual void SetIconDIVX(bool on) = 0;
  virtual void SetIconXVID(bool on) = 0;
  virtual void SetIconWMV(bool on) = 0;
  // codec icons - video: audio stream format
  virtual void SetIconMPGA(bool on) = 0;
  virtual void SetIconAC3(bool on) = 0;
  virtual void SetIconDTS(bool on) = 0;
  virtual void SetIconVWMA(bool on) = 0;
  // codec icons - audio format
  virtual void SetIconMP3(bool on) = 0;
  virtual void SetIconOGG(bool on) = 0;
  virtual void SetIconAWMA(bool on) = 0;
  virtual void SetIconWAV(bool on) = 0;

  virtual void SetIconAudioChannels(int channels) = 0;

private:
  int m_disableOnPlay;

  std::vector<CGUIInfoLabel> m_lcdMode[LCD_MODE_MAX];
  UINT m_eCurrentCharset;

  void RenderIcons();
  void SetCodecInformationIcons();
  void ResetCodecIcons();
  void SetLCDPlayingState(bool on);
};
extern ILCD* g_lcd;
