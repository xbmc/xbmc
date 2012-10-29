#ifndef __XLCDPROC_IMON_H__
#define __XLCDPROC_IMON_H__

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

#include "../utils/LCD.h"
#include "XLCDproc.h"
#include "../utils/Stopwatch.h"

#define IMON_OUTPUT_DISC        0
#define IMON_OUTPUT_TOP_ROW     1
#define IMON_OUTPUT_SPKR        4
#define IMON_OUTPUT_SPDIF       6
#define IMON_OUTPUT_SRC         7
#define IMON_OUTPUT_FIT         8
#define IMON_OUTPUT_TV          9
#define IMON_OUTPUT_HDTV        10
#define IMON_OUTPUT_SCR1        11
#define IMON_OUTPUT_SCR2        12
#define IMON_OUTPUT_BR_ICONS    13
#define IMON_OUTPUT_BM_ICONS    16
#define IMON_OUTPUT_BL_ICONS    19
#define IMON_OUTPUT_VOL         22
#define IMON_OUTPUT_TIME        23
#define IMON_OUTPUT_ALARM       24
#define IMON_OUTPUT_REC         25
#define IMON_OUTPUT_REP         26
#define IMON_OUTPUT_SFL         27
#define IMON_OUTPUT_PBARS       28
#define IMON_OUTPUT_DISK_IN     29

/**
 * Sets the "output state" for the device. We use this to control the icons around the outside the
 * display. The bits in \c outputValue correspond to the icons as follows:
 *
 * Bit     : Icon/Function
 * 0       : disc icon (0=off, 1='spin') , if Toprow==4, use CD-animation, else use "HDD-recording-animation"
 * 1,2,3   : top row (0=none, 1=music, 2=movie, 3=photo, 4=CD/DVD, 5=TV, 6=Web, 7=News/Weather)
 * 4,5     : 'speaker' icons (0=off, 1=L+R, 2=5.1ch, 3=7.1ch)
 * 6       : S/PDIF icon
 * 7       : 'SRC'
 * 8       : 'FIT'
 * 9       : 'TV'
 * 10      : 'HDTV'
 * 11      : 'SRC1'
 * 12      : 'SRC2'
 * 13,14,15: bottom-right icons (0=off, 1=MP3, 2=OGG, 3=WMA, 4=WAV)
 * 16,17,18: bottom-middle icons (0=off, 1=MPG, 2=AC3, 3=DTS, 4=WMA)
 * 19,20,21: bottom-left icons (0=off, 1=MPG, 2=DIVX, 3=XVID, 4=WMV)
 * 22      : 'VOL' (volume)
 * 23      : 'TIME'
 * 24      : 'ALARM'
 * 25      : 'REC' (recording)
 * 26      : 'REP' (repeat)
 * 27      : 'SFL' (shuffle)
 * 28      : Abuse this for progress bars (if set to 1), lower bits represent
 *               the length (6 bits each: P|6xTP|6xTL|6xBL|6xBP with P = bit 28,
 *               TP=Top Progress, TL = Top Line, BL = Bottom Line, BP = Bottom Progress).
 *               If bit 28 is set to 1, lower bits are interpreted as
 *               lengths; otherwise setting the symbols as usual.
 *               0 <= length <= 32, bars extend from left to right.
 *               length > 32, bars extend from right to left, length is counted
 *               from 32 up (i.e. 35 means a length of 3).
 *
 *     Remember: There are two kinds of calls!
 *               With bit 28 set to 1: Set all bars (leaving the symbols as is),
 *               with bit 28 set to 0: Set the symbols (leaving the bars as is).
 *     Beware:   TODO: May become a race condition, if both calls are executed
 *                     before the display gets updated. Keep this in mind in your
 *                     client-code.
 * 29      : 'disc-in icon' - half ellipsoid under the disc symbols (0=off, 1=on)
 */

class XLCDproc_imon: public XLCDproc
{
public:
  XLCDproc_imon(int m_localsockfd);
  virtual ~XLCDproc_imon(void);

  // States whether this device sends the icon-information by itself.
  // Returns true, if the caller does not have to care about sending the information.
  virtual bool SendIconStatesToDisplay();
  virtual void HandleStop(void);

  virtual void SetIconMovie(bool on);
  virtual void SetIconMusic(bool on);
  virtual void SetIconWeather(bool on);
  virtual void SetIconTV(bool on);
  virtual void SetIconPhoto(bool on);
  virtual void SetIconResolution(ILCD::LCD_RESOLUTION_INDICATOR resolution);
  virtual void SetProgressBar1(double progress); // range from 0 - 100
  virtual void SetProgressBar2(double progress); // range from 0 - 100
  virtual void SetProgressBar3(double progress); // range from 0 - 100
  virtual void SetProgressBar4(double progress); // range from 0 - 100
  virtual void SetIconMute(bool on);
  virtual void SetIconPlaying(bool on);
  virtual void SetIconPause(bool on);
  virtual void SetIconRepeat(bool on);
  virtual void SetIconShuffle(bool on);
  virtual void SetIconAlarm(bool on);
  virtual void SetIconRecord(bool on);
  virtual void SetIconVolume(bool on);
  virtual void SetIconTime(bool on);
  virtual void SetIconSPDIF(bool on);
  virtual void SetIconDiscIn(bool on);
  virtual void SetIconSource(bool on);
  virtual void SetIconFit(bool on);
  virtual void SetIconSCR1(bool on);
  virtual void SetIconSCR2(bool on);

  // codec icons - video: video stream format
  virtual void SetIconMPEG(bool on);
  virtual void SetIconDIVX(bool on);
  virtual void SetIconXVID(bool on);
  virtual void SetIconWMV(bool on);
  // codec icons - video: audio stream format
  virtual void SetIconMPGA(bool on);
  virtual void SetIconAC3(bool on);
  virtual void SetIconDTS(bool on);
  virtual void SetIconVWMA(bool on);
  // codec icons - audio format
  virtual void SetIconMP3(bool on);
  virtual void SetIconOGG(bool on);
  virtual void SetIconAWMA(bool on);
  virtual void SetIconWAV(bool on);

  virtual void SetIconAudioChannels(int channels);

private:
  CStopWatch m_outputTimer;

  virtual void ResetModeIcons(void);

  int TopProgressBar(int oldValue, double newValue);
  int BottomProgressBar(int oldValue, double newValue);
  int TopLine(int oldValue, double newValue);
  int BottomLine(int oldValue, double newValue);
  bool isOutputValuesTurn();

  int outputValue;
  int outputValueProgressBars;
  int outputValueOld;
  int outputValueProgressBarsOld;
};

#endif // __XLCDPROC_IMON_H__
