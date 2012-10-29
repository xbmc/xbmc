#ifndef __XLCDPROC_MDM166A_CPP__
#define __XLCDPROC_MDM166A_CPP__

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

#include "XLCDproc_mdm166a.h"
#include "../utils/log.h"

XLCDproc_mdm166a::XLCDproc_mdm166a(int m_localsockfd)
{
  outputValue = 0;
  outputValueOld = 1; // for correct icon-initialization this needs to be different to the actual value

  m_sockfd = m_localsockfd;
}

XLCDproc_mdm166a::~XLCDproc_mdm166a(void)
{
}

bool XLCDproc_mdm166a::SendIconStatesToDisplay()
{
  CStdString cmd;

  if (outputValue != outputValueOld)
  {
    outputValueOld = outputValue;
    cmd.Format("output %d\n", outputValue);
  }
  else
    return true;

  ResetModeIcons();

  if (write(m_sockfd, cmd.c_str(), cmd.size()) < 0)
  {
    CLog::Log(
        LOGERROR,
        "XLCDproc::%s - Unable to write 'outputValue' to socket, LCDd not running?",
        __FUNCTION__);

    return false;
  }

  return true;
}

void XLCDproc_mdm166a::HandleStop(void)
{
  CStdString clearcmd = "";

  // clear any icons
  outputValue = 0;

  clearcmd.Format("output %d\n", outputValue);

  if (write(m_sockfd, clearcmd.c_str(), clearcmd.size()) < 0)
    CLog::Log(LOGERROR, "XLCDproc::%s - Unable to write to socket", __FUNCTION__);

  usleep(100000); // we have to wait a bit until the message has been sent to the display
}

void XLCDproc_mdm166a::ResetModeIcons(void)
{
  outputValue &= ~(3 << MDM166A_OUTPUT_WLAN_STR); // reset entire block
}

void XLCDproc_mdm166a::SetIconMovie(bool on)
{
  // icon not supported by display
}

void XLCDproc_mdm166a::SetIconMusic(bool on)
{
  // icon not supported by display
}

void XLCDproc_mdm166a::SetIconWeather(bool on)
{
  // icon not supported by display
}

void XLCDproc_mdm166a::SetIconTV(bool on)
{
  // icon not supported by display
}

void XLCDproc_mdm166a::SetIconPhoto(bool on)
{
  // icon not supported by display
}

void XLCDproc_mdm166a::SetIconResolution(ILCD::LCD_RESOLUTION_INDICATOR resolution)
{
  // icon not supported by display
}

void XLCDproc_mdm166a::SetProgressBar1(double progress)
{
  outputValue = progressBar(outputValue, progress);
}

void XLCDproc_mdm166a::SetProgressBar2(double progress)
{
  outputValue = volumeBar(outputValue, progress);
}

inline int XLCDproc_mdm166a::volumeBar(int oldValue, double newValue)
{
  oldValue &= ~0x1F00;
  oldValue |= ((int) (28 * (newValue / 100)) << MDM166A_OUTPUT_VOLUME) & 0x1F00;
  return oldValue;
}

inline int XLCDproc_mdm166a::progressBar(int oldValue, double newValue)
{
  oldValue &= ~0x3F8000;
  oldValue |= ((int) (96 * (newValue / 100)) << MDM166A_OUTPUT_PROGRESS) & 0x3F8000;
  return oldValue;
}

void XLCDproc_mdm166a::SetIconMute(bool on)
{
  if (on)
    outputValue |= 1 << MDM166A_OUTPUT_MUTE;
  else
    outputValue &= ~(1 << MDM166A_OUTPUT_MUTE);
}

void XLCDproc_mdm166a::SetIconPlaying(bool on)
{
  if (on)
    outputValue |= 1 << MDM166A_OUTPUT_PLAY;
  else
    outputValue &= ~(1 << MDM166A_OUTPUT_PLAY);
}

void XLCDproc_mdm166a::SetIconPause(bool on)
{
  if (on)
    outputValue |= 1 << MDM166A_OUTPUT_PAUSE;
  else
    outputValue &= ~(1 << MDM166A_OUTPUT_PAUSE);
}

void XLCDproc_mdm166a::SetIconRepeat(bool on)
{
  // icon not supported by display
}

void XLCDproc_mdm166a::SetIconShuffle(bool on)
{
  // icon not supported by display
}

void XLCDproc_mdm166a::SetIconAlarm(bool on)
{
  if (on)
    outputValue |= 1 << MDM166A_OUTPUT_ALARM;
  else
    outputValue &= ~(1 << MDM166A_OUTPUT_ALARM);
}

void XLCDproc_mdm166a::SetIconRecord(bool on)
{
  if (on)
    outputValue |= 1 << MDM166A_OUTPUT_REC;
  else
    outputValue &= ~(1 << MDM166A_OUTPUT_REC);
}

void XLCDproc_mdm166a::SetIconVolume(bool on)
{
  if (on)
    outputValue |= 1 << MDM166A_OUTPUT_VOL;
  else
    outputValue &= ~(1 << MDM166A_OUTPUT_VOL);
}

void XLCDproc_mdm166a::SetIconTime(bool on)
{
  // icon not supported by display
}

void XLCDproc_mdm166a::SetIconSPDIF(bool on)
{
  if (on)
    outputValue |= 1 << MDM166A_OUTPUT_WLAN_ANT;
  else
    outputValue &= ~(1 << MDM166A_OUTPUT_WLAN_ANT);
}

void XLCDproc_mdm166a::SetIconDiscIn(bool on)
{
  // icon not supported by display
}

void XLCDproc_mdm166a::SetIconSource(bool on)
{
  // icon not supported by display
}

void XLCDproc_mdm166a::SetIconFit(bool on)
{
  // icon not supported by display
}

void XLCDproc_mdm166a::SetIconSCR1(bool on)
{
  // icon not supported by display
}

void XLCDproc_mdm166a::SetIconSCR2(bool on)
{
  // icon not supported by display
}

// codec icons - video: video stream format ###################################
void XLCDproc_mdm166a::SetIconMPEG(bool on)
{
  // icon not supported by display
}

void XLCDproc_mdm166a::SetIconDIVX(bool on)
{
  // icon not supported by display
}

void XLCDproc_mdm166a::SetIconXVID(bool on)
{
  // icon not supported by display
}

void XLCDproc_mdm166a::SetIconWMV(bool on)
{
  // icon not supported by display
}

// codec icons - video: audio stream format #################################
void XLCDproc_mdm166a::SetIconMPGA(bool on)
{
  // icon not supported by display
}

void XLCDproc_mdm166a::SetIconAC3(bool on)
{
  // icon not supported by display
}

void XLCDproc_mdm166a::SetIconDTS(bool on)
{
  // icon not supported by display
}

void XLCDproc_mdm166a::SetIconVWMA(bool on)
{
  // icon not supported by display
}

// codec icons - audio format ###############################################
void XLCDproc_mdm166a::SetIconMP3(bool on)
{
  // icon not supported by display
}

void XLCDproc_mdm166a::SetIconOGG(bool on)
{
  // icon not supported by display
}

void XLCDproc_mdm166a::SetIconAWMA(bool on)
{
  // icon not supported by display
}

void XLCDproc_mdm166a::SetIconWAV(bool on)
{
  // icon not supported by display
}

void XLCDproc_mdm166a::SetIconAudioChannels(int channels)
{
  switch (channels)
  {
  case 1:
  case 2:
  case 3:
    outputValue |= 1 << MDM166A_OUTPUT_WLAN_STR;
    break;
  case 5:
  case 6:
    outputValue |= 2 << MDM166A_OUTPUT_WLAN_STR;
    break;
  case 7:
  case 8:
    outputValue |= 3 << MDM166A_OUTPUT_WLAN_STR;
    break;
  default:
    outputValue &= ~(3 << MDM166A_OUTPUT_WLAN_STR);
    break;
  }
}

#endif // __XLCDPROC_MDM166A_CPP__
