#ifndef __XLCDProc_imon_CPP__
#define __XLCDProc_imon_CPP__

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

#include "XLCDProc_imon.h"
#include "../utils/log.h"

XLCDProc_imon::XLCDProc_imon()
{
  m_outputTimer.StartZero();
}

XLCDProc_imon::~XLCDProc_imon(void)
{
}

bool XLCDProc_imon::SendIconStatesToDisplay()
{
  char cmd[64];

  if (outputValue != outputValueOld || isOutputValuesTurn())
  {
    // A tweak for the imon-lcd-driver: There is no icon for this bit.
    // Needed because the driver sets ALL symbols (icons and progress bars) to "off", if there is an outputValue == 0
    outputValue |= 1 << 30;
    outputValueOld = outputValue;
    sprintf(cmd, "output %d\n", outputValue);
  }
  else if (outputValueProgressBars != outputValueProgressBarsOld)
  {
    outputValueProgressBarsOld = outputValueProgressBars;
    sprintf(cmd, "output %d\n", outputValueProgressBars);
  }
  else
    return true;

  ResetModeIcons();

  if (write(m_sockfd, cmd, strlen(cmd)) < 0)
  {
    CLog::Log(
        LOGERROR,
        "XLCDproc::%s - Unable to write 'outputValue' to socket, LCDd not running?",
        __FUNCTION__);
  }

  return true;
}

void XLCDProc_imon::ResetModeIcons(void)
{
  outputValue &= ~(7 << IMON_OUTPUT_TOP_ROW); // reset entire row
  outputValue &= ~(7 << IMON_OUTPUT_BL_ICONS); // reset entire row
  outputValue &= ~(7 << IMON_OUTPUT_BM_ICONS); // reset entire row
  outputValue &= ~(7 << IMON_OUTPUT_BR_ICONS); // reset entire row
  outputValue &= ~(3 << IMON_OUTPUT_SPKR); // reset entire block
}

void XLCDProc_imon::SetIconMovie(bool on)
{
  if (on)
    outputValue |= 2 << IMON_OUTPUT_TOP_ROW;
  else
    outputValue &= ~(2 << IMON_OUTPUT_TOP_ROW);
}

void XLCDProc_imon::SetIconMusic(bool on)
{
  if (on)
    outputValue |= 1 << IMON_OUTPUT_TOP_ROW;
  else
    outputValue &= ~(1 << IMON_OUTPUT_TOP_ROW);
}

void XLCDProc_imon::SetIconWeather(bool on)
{
  if (on)
    outputValue |= 7 << IMON_OUTPUT_TOP_ROW;
  else
    outputValue &= ~(7 << IMON_OUTPUT_TOP_ROW);
}

void XLCDProc_imon::SetIconTV(bool on)
{
  if (on)
    outputValue |= 5 << IMON_OUTPUT_TOP_ROW;
  else
    outputValue &= ~(5 << IMON_OUTPUT_TOP_ROW);
}

void XLCDProc_imon::SetIconPhoto(bool on)
{
  if (on)
    outputValue |= 3 << IMON_OUTPUT_TOP_ROW;
  else
    outputValue &= ~(3 << IMON_OUTPUT_TOP_ROW);
}

void XLCDProc_imon::SetIconResolution(ILCD::LCD_RESOLUTION resolution)
{
  // reset both icons to avoid that they may be lit at the same time
  outputValue &= ~(1 << IMON_OUTPUT_TV);
  outputValue &= ~(1 << IMON_OUTPUT_HDTV);

  if (resolution == LCD_RESOLUTION_SD)
    outputValue |= 1 << IMON_OUTPUT_TV;
  else if (resolution == LCD_RESOLUTION_HD)
    outputValue |= 1 << IMON_OUTPUT_HDTV;
}

void XLCDProc_imon::SetProgressBar1(double progress)
{
  XLCDproc::outputValueProgressBars = progressBar(
      XLCDproc::outputValueProgressBars, progress);
}

void XLCDProc_imon::SetProgressBar2(double volume)
{
  XLCDproc::outputValueProgressBars = volumeBar(
      XLCDproc::outputValueProgressBars, volume);
}

inline int XLCDProc_imon::volumeBar(int oldValue, double newValue)
{
  oldValue &= ~0x00FC0000;
  oldValue |= ((int) (32 * (newValue / 100)) << 18) & 0x00FC0000;
  oldValue |= 1 << IMON_OUTPUT_PBARS;
  return oldValue;
}

inline int XLCDProc_imon::progressBar(int oldValue, double newValue)
{
  oldValue &= ~0x00000FC0;
  oldValue |= ((int) (32 * (newValue / 100)) << 6) & 0x00000FC0;
  oldValue |= 1 << IMON_OUTPUT_PBARS;
  return oldValue;
}

bool XLCDProc_imon::isOutputValuesTurn()
{
  if (m_outputTimer.GetElapsedMilliseconds() > 1000)
  {
    m_outputTimer.Reset();
    return true;
  }
  else
  {
    return false;
  }
}

void XLCDProc_imon::SetIconMute(bool on)
{
  // icon not supported by display
}

void XLCDProc_imon::SetIconPlaying(bool on)
{
  if (on)
    outputValue |= 1 << IMON_OUTPUT_DISC;
  else
    outputValue &= ~(1 << IMON_OUTPUT_DISC);
}

void XLCDProc_imon::SetIconPause(bool on)
{
  // icon not supported by display
}

void XLCDProc_imon::SetIconRepeat(bool on)
{
  if (on)
    outputValue |= 1 << IMON_OUTPUT_REP;
  else
    outputValue &= ~(1 << IMON_OUTPUT_REP);
}

void XLCDProc_imon::SetIconShuffle(bool on)
{
  if (on)
    outputValue |= 1 << IMON_OUTPUT_SFL;
  else
    outputValue &= ~(1 << IMON_OUTPUT_SFL);
}

void XLCDProc_imon::SetIconAlarm(bool on)
{
  if (on)
    outputValue |= 1 << IMON_OUTPUT_ALARM;
  else
    outputValue &= ~(1 << IMON_OUTPUT_ALARM);
}

void XLCDProc_imon::SetIconRecord(bool on)
{
  if (on)
    outputValue |= 1 << IMON_OUTPUT_REC;
  else
    outputValue &= ~(1 << IMON_OUTPUT_REC);
}

void XLCDProc_imon::SetIconVolume(bool on)
{
  if (on)
    outputValue |= 1 << IMON_OUTPUT_VOL;
  else
    outputValue &= ~(1 << IMON_OUTPUT_VOL);
}

void XLCDProc_imon::SetIconTime(bool on)
{
  if (on)
    outputValue |= 1 << IMON_OUTPUT_TIME;
  else
    outputValue &= ~(1 << IMON_OUTPUT_TIME);
}

void XLCDProc_imon::SetIconSPDIF(bool on)
{
  if (on)
    outputValue |= 1 << IMON_OUTPUT_SPDIF;
  else
    outputValue &= ~(1 << IMON_OUTPUT_SPDIF);
}

void XLCDProc_imon::SetIconDiscIn(bool on)
{
  if (on)
    outputValue |= 1 << IMON_OUTPUT_DISK_IN;
  else
    outputValue &= ~(1 << IMON_OUTPUT_DISK_IN);
}

void XLCDProc_imon::SetIconSource(bool on)
{
  if (on)
    outputValue |= 1 << IMON_OUTPUT_SRC;
  else
    outputValue &= ~(1 << IMON_OUTPUT_SRC);
}

void XLCDProc_imon::SetIconFit(bool on)
{
  if (on)
    outputValue |= 1 << IMON_OUTPUT_FIT;
  else
    outputValue &= ~(1 << IMON_OUTPUT_FIT);
}

void XLCDProc_imon::SetIconSCR1(bool on)
{
  if (on)
    outputValue |= 1 << IMON_OUTPUT_SCR1;
  else
    outputValue &= ~(1 << IMON_OUTPUT_SCR1);
}

void XLCDProc_imon::SetIconSCR2(bool on)
{
  if (on)
    outputValue |= 1 << IMON_OUTPUT_SCR2;
  else
    outputValue &= ~(1 << IMON_OUTPUT_SCR2);
}

// codec icons - video: video stream format ###################################
void XLCDProc_imon::SetIconMPEG(bool on)
{
  if (on)
    outputValue |= 1 << IMON_OUTPUT_BL_ICONS;
  else
    outputValue &= ~(1 << IMON_OUTPUT_BL_ICONS);
}

void XLCDProc_imon::SetIconDIVX(bool on)
{
  if (on)
    outputValue |= 2 << IMON_OUTPUT_BL_ICONS;
  else
    outputValue &= ~(2 << IMON_OUTPUT_BL_ICONS);
}

void XLCDProc_imon::SetIconXVID(bool on)
{
  if (on)
    outputValue |= 3 << IMON_OUTPUT_BL_ICONS;
  else
    outputValue &= ~(3 << IMON_OUTPUT_BL_ICONS);
}

void XLCDProc_imon::SetIconWMV(bool on)
{
  if (on)
    outputValue |= 4 << IMON_OUTPUT_BL_ICONS;
  else
    outputValue &= ~(4 << IMON_OUTPUT_BL_ICONS);
}
// codec icons - video: audio stream format #################################
void XLCDProc_imon::SetIconMPGA(bool on)
{
  if (on)
    outputValue |= 1 << IMON_OUTPUT_BM_ICONS;
  else
    outputValue &= ~(1 << IMON_OUTPUT_BM_ICONS);
}

void XLCDProc_imon::SetIconAC3(bool on)
{
  if (on)
    outputValue |= 2 << IMON_OUTPUT_BM_ICONS;
  else
    outputValue &= ~(2 << IMON_OUTPUT_BM_ICONS);
}

void XLCDProc_imon::SetIconDTS(bool on)
{
  if (on)
    outputValue |= 3 << IMON_OUTPUT_BM_ICONS;
  else
    outputValue &= ~(3 << IMON_OUTPUT_BM_ICONS);
}

void XLCDProc_imon::SetIconVWMA(bool on)
{
  if (on)
    outputValue |= 4 << IMON_OUTPUT_BM_ICONS;
  else
    outputValue &= ~(4 << IMON_OUTPUT_BM_ICONS);
}
// codec icons - audio format ###############################################
void XLCDProc_imon::SetIconMP3(bool on)
{
  if (on)
    outputValue |= 1 << IMON_OUTPUT_BR_ICONS;
  else
    outputValue &= ~(1 << IMON_OUTPUT_BR_ICONS);
}

void XLCDProc_imon::SetIconOGG(bool on)
{
  if (on)
    outputValue |= 2 << IMON_OUTPUT_BR_ICONS;
  else
    outputValue &= ~(2 << IMON_OUTPUT_BR_ICONS);
}

void XLCDProc_imon::SetIconAWMA(bool on)
{
  if (on)
    outputValue |= 3 << IMON_OUTPUT_BR_ICONS;
  else
    outputValue &= ~(3 << IMON_OUTPUT_BR_ICONS);
}

void XLCDProc_imon::SetIconWAV(bool on)
{
  if (on)
    outputValue |= 4 << IMON_OUTPUT_BR_ICONS;
  else
    outputValue &= ~(4 << IMON_OUTPUT_BR_ICONS);
}

void XLCDProc_imon::SetIconAudioChannels(int channels)
{
  switch (channels)
  {
  case 1:
  case 2:
  case 3:
    outputValue |= 1 << IMON_OUTPUT_SPKR;
    break;
  case 5:
  case 6:
    outputValue |= 2 << IMON_OUTPUT_SPKR;
    break;
  case 7:
  case 8:
    outputValue |= 3 << IMON_OUTPUT_SPKR;
    break;
  default:
    outputValue &= ~(3 << IMON_OUTPUT_SPKR);
    break;
  }
}

void XLCDProc_imon::InitializeSpectrumAnalyzer()
{
  // no need to do anything here
}

void XLCDProc_imon::SetSpectrumAnalyzerData()
{
  // no need to do anything here
}

#endif // __XLCDProc_imon_CPP__
