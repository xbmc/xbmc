#ifndef __XLCDPROC_MDM166A_H__
#define __XLCDPROC_MDM166A_H__

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

#define MDM166A_OUTPUT_PLAY     0
#define MDM166A_OUTPUT_PAUSE    1
#define MDM166A_OUTPUT_REC      2
#define MDM166A_OUTPUT_ALARM    3
#define MDM166A_OUTPUT_AT       4
#define MDM166A_OUTPUT_MUTE     5
#define MDM166A_OUTPUT_WLAN_ANT 6
#define MDM166A_OUTPUT_VOL      7
#define MDM166A_OUTPUT_VOLUME   8
#define MDM166A_OUTPUT_WLAN_STR 13
#define MDM166A_OUTPUT_PROGRESS 15

/**
 * Sets the "output state" for the device. We use this to control the icons around the outside the
 * display. The bits in \c outputValue correspond to the icons as follows:
 *
 * Bit     : Icon/Function
 * 0       : Play
 * 1       : Pause
 * 2       : Record
 * 3       : Message
 * 4       : @ (in the envelope (=Message symbol))
 * 5       : Mute
 * 6       : WLAN tower
 * 7       : Volume (the word "Vol.")
 * 8...12  : Volume value (which may be 0-28)
 * 13...14 : WLAN signal strength (which may be 0-3)
 */

class XLCDproc_mdm166a: public XLCDproc
{
public:
  XLCDproc_mdm166a(int m_localsockfd);
  virtual ~XLCDproc_mdm166a(void);

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
  virtual void ResetModeIcons(void);

  int volumeBar(int oldValue, double newValue);
  int progressBar(int oldValue, double newValue);

  int outputValue;
  int outputValueOld;
};

#endif // __XLCDPROC_MDM166A_H__
