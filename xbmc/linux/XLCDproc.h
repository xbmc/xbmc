#ifndef __XLCDPROC_H__
#define __XLCDPROC_H__

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

#include "../utils/LCD.h"

#define MAX_ROWS 20
#define INIT_RETRY_INTERVAL 2000
#define INIT_RETRY_INTERVAL_MAX 60000

class XLCDproc : public ILCD
{
public:
  XLCDproc();
  virtual ~XLCDproc(void);
  virtual void Initialize();
  virtual bool IsConnected();
  virtual void Stop();
  virtual void Suspend();
  virtual void Resume();
  virtual void SetBackLight(int iLight);
  virtual void SetContrast(int iContrast);
  virtual int  GetColumns();
  virtual int  GetRows();

  bool         SendLCDd(const CStdString &command);
  void         ReadAndFlushSocket();

  // Handlers for icon-handling sub-instances
  bool         SendIconStatesToDisplay();
  void         HandleStop();

  void         SetIconMovie(bool on);
  void         SetIconMusic(bool on);
  void         SetIconWeather(bool on);
  void         SetIconTV(bool on);
  void         SetIconPhoto(bool on);
  void         SetIconResolution(LCD_RESOLUTION_INDICATOR resolution);
  void         SetProgressBar1(double progress);
  void         SetProgressBar2(double progress);
  void         SetProgressBar3(double progress);
  void         SetProgressBar4(double progress);
  void         SetIconMute(bool on);
  void         SetIconPlaying(bool on);
  void         SetIconPause(bool on);
  void         SetIconRepeat(bool on);
  void         SetIconShuffle(bool on);
  void         SetIconAlarm(bool on);
  void         SetIconRecord(bool on);
  void         SetIconVolume(bool on);
  void         SetIconTime(bool on);
  void         SetIconSPDIF(bool on);
  void         SetIconDiscIn(bool on);
  void         SetIconSource(bool on);
  void         SetIconFit(bool on);
  void         SetIconSCR1(bool on);
  void         SetIconSCR2(bool on);
  void         SetIconMPEG(bool on);
  void         SetIconDIVX(bool on);
  void         SetIconXVID(bool on);
  void         SetIconWMV(bool on);
  void         SetIconMPGA(bool on);
  void         SetIconAC3(bool on);
  void         SetIconDTS(bool on);
  void         SetIconVWMA(bool on);
  void         SetIconMP3(bool on);
  void         SetIconOGG(bool on);
  void         SetIconAWMA(bool on);
  void         SetIconWAV(bool on);
  void         SetIconAudioChannels(int channels);

protected:
  virtual void Process();
  virtual void SetLine(int iLine, const CStdString& strLine);
  bool         Connect();
  void         RecognizeAndSetIconDriver();
  void         CloseSocket();
  unsigned int m_iColumns;        // display columns for each line
  unsigned int m_iRows;           // total number of rows
  unsigned int m_iActualpos;      // actual cursor possition
  int          m_iBackLight;
  int          m_iLCDContrast;
  bool         m_bUpdate[MAX_ROWS];
  CStdString   m_strLine[MAX_ROWS];
  int          m_iPos[MAX_ROWS];
  DWORD        m_dwSleep[MAX_ROWS];
  CEvent       m_event;
  int          m_sockfd;

private:
  int          m_lastInitAttempt;
  int          m_initRetryInterval;
  bool         m_used; //set to false when trying to connect has failed

  XLCDproc     *m_lcdprocIconDevice;
  unsigned int m_iCharsetTab;
};

#endif
