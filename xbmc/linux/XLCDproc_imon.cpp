#ifndef __XLCDPROC_IMON_CPP__
#define __XLCDPROC_IMON_CPP__

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

#include "XLCDproc_imon.h"
#include "../utils/log.h"

XLCDproc_imon::XLCDproc_imon(int m_localsockfd)
{
  // initialize class variables
  outputValue = 0;
  outputValueProgressBars = 0;
  outputValueOld = 1; // for correct icon-initialization this needs to be different to the actual value
  outputValueProgressBarsOld = 1; // for correct icon-initialization this needs to be different to the actual value

  m_sockfd = m_localsockfd;

  m_outputTimer.StartZero();
}

XLCDproc_imon::~XLCDproc_imon(void)
{
}

bool XLCDproc_imon::SendIconStatesToDisplay()
{
  CStdString cmd;

  if (outputValue != outputValueOld || isOutputValuesTurn())
  {
    // A tweak for the imon-lcd-driver: There is no icon for this bit.
    // Needed because the driver sets ALL symbols (icons and progress bars) to "off", if there is an outputValue == 0
    outputValue |= 1 << 30;
    outputValueOld = outputValue;
    cmd.Format("output %d\n", outputValue);
  }
  else if (outputValueProgressBars != outputValueProgressBarsOld || isOutputValuesTurn())
  {
    outputValueProgressBarsOld = outputValueProgressBars;
    cmd.Format("output %d\n", outputValueProgressBars);
  }
  else
  {
    return true;
  }

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

void XLCDproc_imon::HandleStop(void)
{
  CStdString clearcmd = "";

  // clear any icons
  outputValue = 0;

  clearcmd.Format("output %d\n", outputValue);

  if (write(m_sockfd, clearcmd.c_str(), clearcmd.size()) < 0)
    CLog::Log(LOGERROR, "XLCDproc::%s - Unable to write to socket", __FUNCTION__);

  usleep(100000); // we have to wait a bit until the message has been sent to the display
}

void XLCDproc_imon::ResetModeIcons(void)
{
  outputValue &= ~(7 << IMON_OUTPUT_TOP_ROW); // reset entire row
  outputValue &= ~(7 << IMON_OUTPUT_BL_ICONS); // reset entire row
  outputValue &= ~(7 << IMON_OUTPUT_BM_ICONS); // reset entire row
  outputValue &= ~(7 << IMON_OUTPUT_BR_ICONS); // reset entire row
  outputValue &= ~(3 << IMON_OUTPUT_SPKR); // reset entire block
}

void XLCDproc_imon::SetIconMovie(bool on)
{
  if (on)
    outputValue |= 2 << IMON_OUTPUT_TOP_ROW;
  else
    outputValue &= ~(2 << IMON_OUTPUT_TOP_ROW);
}

void XLCDproc_imon::SetIconMusic(bool on)
{
  if (on)
    outputValue |= 1 << IMON_OUTPUT_TOP_ROW;
  else
    outputValue &= ~(1 << IMON_OUTPUT_TOP_ROW);
}

void XLCDproc_imon::SetIconWeather(bool on)
{
  if (on)
    outputValue |= 7 << IMON_OUTPUT_TOP_ROW;
  else
    outputValue &= ~(7 << IMON_OUTPUT_TOP_ROW);
}

void XLCDproc_imon::SetIconTV(bool on)
{
  if (on)
    outputValue |= 5 << IMON_OUTPUT_TOP_ROW;
  else
    outputValue &= ~(5 << IMON_OUTPUT_TOP_ROW);
}

void XLCDproc_imon::SetIconPhoto(bool on)
{
  if (on)
    outputValue |= 3 << IMON_OUTPUT_TOP_ROW;
  else
    outputValue &= ~(3 << IMON_OUTPUT_TOP_ROW);
}

void XLCDproc_imon::SetIconResolution(ILCD::LCD_RESOLUTION_INDICATOR resolution)
{
  // reset both icons to avoid that they may be lit at the same time
  outputValue &= ~(1 << IMON_OUTPUT_TV);
  outputValue &= ~(1 << IMON_OUTPUT_HDTV);

  if (resolution == LCD_RESOLUTION_INDICATOR_SD)
    outputValue |= 1 << IMON_OUTPUT_TV;
  else if (resolution == LCD_RESOLUTION_INDICATOR_HD)
    outputValue |= 1 << IMON_OUTPUT_HDTV;
}

void XLCDproc_imon::SetProgressBar1(double progress)
{
  outputValueProgressBars = TopProgressBar(
    outputValueProgressBars, progress);
}

void XLCDproc_imon::SetProgressBar2(double progress)
{
  outputValueProgressBars = BottomProgressBar(
    outputValueProgressBars, progress);
}

void XLCDproc_imon::SetProgressBar3(double progress)
{
  outputValueProgressBars = TopLine(
    outputValueProgressBars, progress);
}

void XLCDproc_imon::SetProgressBar4(double progress)
{
  outputValueProgressBars = BottomLine(
    outputValueProgressBars, progress);
}

inline int XLCDproc_imon::TopProgressBar(int oldValue, double newValue)
{
  oldValue &= ~0x00000FC0;
  oldValue |= ((int) (32 * (newValue / 100)) << 6) & 0x00000FC0;
  oldValue |= 1 << IMON_OUTPUT_PBARS;
  return oldValue;
}

inline int XLCDproc_imon::BottomProgressBar(int oldValue, double newValue)
{
  oldValue &= ~0x00FC0000;
  oldValue |= ((int) (32 * (newValue / 100)) << 18) & 0x00FC0000;
  oldValue |= 1 << IMON_OUTPUT_PBARS;
  return oldValue;
}

inline int XLCDproc_imon::TopLine(int oldValue, double newValue)
{
  oldValue &= ~0x0000003F;
  oldValue |= ((int) (32 * (newValue / 100)) << 0) & 0x0000003F;
  oldValue |= 1 << IMON_OUTPUT_PBARS;
  return oldValue;
}

inline int XLCDproc_imon::BottomLine(int oldValue, double newValue)
{
  oldValue &= ~0x0003F000;
  oldValue |= ((int) (32 * (newValue / 100)) << 12) & 0x0003F000;
  oldValue |= 1 << IMON_OUTPUT_PBARS;
  return oldValue;
}

bool XLCDproc_imon::isOutputValuesTurn()
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

void XLCDproc_imon::SetIconMute(bool on)
{
  // icon not supported by display
}

void XLCDproc_imon::SetIconPlaying(bool on)
{
  if (on)
  {
    outputValue |= 1 << IMON_OUTPUT_DISC;
  }
  else
  {
    outputValue &= ~(1 << IMON_OUTPUT_DISC);
  }
}

void XLCDproc_imon::SetIconPause(bool on)
{
  // icon not supported by display
}

void XLCDproc_imon::SetIconRepeat(bool on)
{
  if (on)
    outputValue |= 1 << IMON_OUTPUT_REP;
  else
    outputValue &= ~(1 << IMON_OUTPUT_REP);
}

void XLCDproc_imon::SetIconShuffle(bool on)
{
  if (on)
    outputValue |= 1 << IMON_OUTPUT_SFL;
  else
    outputValue &= ~(1 << IMON_OUTPUT_SFL);
}

void XLCDproc_imon::SetIconAlarm(bool on)
{
  if (on)
    outputValue |= 1 << IMON_OUTPUT_ALARM;
  else
    outputValue &= ~(1 << IMON_OUTPUT_ALARM);
}

void XLCDproc_imon::SetIconRecord(bool on)
{
  if (on)
    outputValue |= 1 << IMON_OUTPUT_REC;
  else
    outputValue &= ~(1 << IMON_OUTPUT_REC);
}

void XLCDproc_imon::SetIconVolume(bool on)
{
  if (on)
    outputValue |= 1 << IMON_OUTPUT_VOL;
  else
    outputValue &= ~(1 << IMON_OUTPUT_VOL);
}

void XLCDproc_imon::SetIconTime(bool on)
{
  if (on)
    outputValue |= 1 << IMON_OUTPUT_TIME;
  else
    outputValue &= ~(1 << IMON_OUTPUT_TIME);
}

void XLCDproc_imon::SetIconSPDIF(bool on)
{
  if (on)
    outputValue |= 1 << IMON_OUTPUT_SPDIF;
  else
    outputValue &= ~(1 << IMON_OUTPUT_SPDIF);
}

void XLCDproc_imon::SetIconDiscIn(bool on)
{
  if (on)
    outputValue |= 1 << IMON_OUTPUT_DISK_IN;
  else
    outputValue &= ~(1 << IMON_OUTPUT_DISK_IN);
}

void XLCDproc_imon::SetIconSource(bool on)
{
  if (on)
    outputValue |= 1 << IMON_OUTPUT_SRC;
  else
    outputValue &= ~(1 << IMON_OUTPUT_SRC);
}

void XLCDproc_imon::SetIconFit(bool on)
{
  if (on)
    outputValue |= 1 << IMON_OUTPUT_FIT;
  else
    outputValue &= ~(1 << IMON_OUTPUT_FIT);
}

void XLCDproc_imon::SetIconSCR1(bool on)
{
  if (on)
    outputValue |= 1 << IMON_OUTPUT_SCR1;
  else
    outputValue &= ~(1 << IMON_OUTPUT_SCR1);
}

void XLCDproc_imon::SetIconSCR2(bool on)
{
  if (on)
    outputValue |= 1 << IMON_OUTPUT_SCR2;
  else
    outputValue &= ~(1 << IMON_OUTPUT_SCR2);
}

// codec icons - video: video stream format ###################################
void XLCDproc_imon::SetIconMPEG(bool on)
{
  if (on)
    outputValue |= 1 << IMON_OUTPUT_BL_ICONS;
  else
    outputValue &= ~(1 << IMON_OUTPUT_BL_ICONS);
}

void XLCDproc_imon::SetIconDIVX(bool on)
{
  if (on)
    outputValue |= 2 << IMON_OUTPUT_BL_ICONS;
  else
    outputValue &= ~(2 << IMON_OUTPUT_BL_ICONS);
}

void XLCDproc_imon::SetIconXVID(bool on)
{
  if (on)
    outputValue |= 3 << IMON_OUTPUT_BL_ICONS;
  else
    outputValue &= ~(3 << IMON_OUTPUT_BL_ICONS);
}

void XLCDproc_imon::SetIconWMV(bool on)
{
  if (on)
    outputValue |= 4 << IMON_OUTPUT_BL_ICONS;
  else
    outputValue &= ~(4 << IMON_OUTPUT_BL_ICONS);
}
// codec icons - video: audio stream format #################################
void XLCDproc_imon::SetIconMPGA(bool on)
{
  if (on)
    outputValue |= 1 << IMON_OUTPUT_BM_ICONS;
  else
    outputValue &= ~(1 << IMON_OUTPUT_BM_ICONS);
}

void XLCDproc_imon::SetIconAC3(bool on)
{
  if (on)
    outputValue |= 2 << IMON_OUTPUT_BM_ICONS;
  else
    outputValue &= ~(2 << IMON_OUTPUT_BM_ICONS);
}

void XLCDproc_imon::SetIconDTS(bool on)
{
  if (on)
    outputValue |= 3 << IMON_OUTPUT_BM_ICONS;
  else
    outputValue &= ~(3 << IMON_OUTPUT_BM_ICONS);
}

void XLCDproc_imon::SetIconVWMA(bool on)
{
  if (on)
    outputValue |= 4 << IMON_OUTPUT_BM_ICONS;
  else
    outputValue &= ~(4 << IMON_OUTPUT_BM_ICONS);
}
// codec icons - audio format ###############################################
void XLCDproc_imon::SetIconMP3(bool on)
{
  if (on)
    outputValue |= 1 << IMON_OUTPUT_BR_ICONS;
  else
    outputValue &= ~(1 << IMON_OUTPUT_BR_ICONS);
}

void XLCDproc_imon::SetIconOGG(bool on)
{
  if (on)
    outputValue |= 2 << IMON_OUTPUT_BR_ICONS;
  else
    outputValue &= ~(2 << IMON_OUTPUT_BR_ICONS);
}

void XLCDproc_imon::SetIconAWMA(bool on)
{
  if (on)
    outputValue |= 3 << IMON_OUTPUT_BR_ICONS;
  else
    outputValue &= ~(3 << IMON_OUTPUT_BR_ICONS);
}

void XLCDproc_imon::SetIconWAV(bool on)
{
  if (on)
    outputValue |= 4 << IMON_OUTPUT_BR_ICONS;
  else
    outputValue &= ~(4 << IMON_OUTPUT_BR_ICONS);
}

void XLCDproc_imon::SetIconAudioChannels(int channels)
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

#endif // __XLCDPROC_IMON_CPP__
