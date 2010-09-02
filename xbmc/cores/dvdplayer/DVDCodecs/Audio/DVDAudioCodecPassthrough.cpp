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

CDVDAudioCodecPassthrough::CDVDAudioCodecPassthrough(void) :
  m_bufferSize(0)
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

  m_bufferSize = 0;

  if ((hints.codec == CODEC_ID_AC3 && bSupportsAC3Out) || (hints.codec == CODEC_ID_DTS && bSupportsDTSOut))
    return true;

  return false;
}

void CDVDAudioCodecPassthrough::Dispose()
{
}

int CDVDAudioCodecPassthrough::Decode(BYTE* pData, int iSize)
{
  if (iSize <= 0) return 0;
  unsigned int room = sizeof(m_buffer) - m_bufferSize;
  unsigned int copy = std::min(room, (unsigned int)iSize);
  memcpy(m_buffer + m_bufferSize, pData, copy);
  m_bufferSize += copy;
  return copy;
}

int CDVDAudioCodecPassthrough::GetData(BYTE** dst)
{
  int size     = m_bufferSize;
  *dst         = m_buffer;
  m_bufferSize = 0;
  return size;
}

void CDVDAudioCodecPassthrough::Reset()
{
  m_bufferSize = 0;
}

