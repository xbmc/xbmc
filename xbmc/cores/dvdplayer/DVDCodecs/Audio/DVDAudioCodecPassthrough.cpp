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

#include "DVDAudioCodecPassthrough.h"
#include "DVDCodecs/DVDCodecs.h"
#include "DVDStreamInfo.h"
#include "GUISettings.h"
#include "Settings.h"
#include "utils/log.h"


CDVDAudioCodecPassthrough::CDVDAudioCodecPassthrough(void)
{
}

CDVDAudioCodecPassthrough::~CDVDAudioCodecPassthrough(void)
{
  Dispose();
}

bool CDVDAudioCodecPassthrough::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  bool bSupportsAC3Out = false;
  bool bSupportsDTSOut = false;
  int audioMode = g_guiSettings.GetInt("audiooutput.mode");

  // TODO - move this stuff somewhere else
  if (AUDIO_IS_BITSTREAM(audioMode))
  {
    bSupportsAC3Out = g_guiSettings.GetBool("audiooutput.ac3passthrough");
    bSupportsDTSOut = g_guiSettings.GetBool("audiooutput.dtspassthrough");
  }

  if ((hints.codec == CODEC_ID_AC3 && bSupportsAC3Out) || (hints.codec == CODEC_ID_DTS && bSupportsDTSOut))
    return true;

  return false;
}

void CDVDAudioCodecPassthrough::Dispose()
{
}

int CDVDAudioCodecPassthrough::Decode(BYTE* pData, int iSize)
{
  if (m_packetizer.HasPacket()) return 0;
  int offset = m_packetizer.AddData(pData, iSize);
  return offset;
}

int CDVDAudioCodecPassthrough::GetData(BYTE** dst)
{
  return m_packetizer.GetPacket(dst);
}

void CDVDAudioCodecPassthrough::Reset()
{
  m_packetizer.Reset();
}

