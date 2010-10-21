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

#include "cores/AudioEngine/AEFactory.h"

CDVDAudioCodecPassthrough::CDVDAudioCodecPassthrough(void) :
  m_buffer    (NULL),
  m_bufferSize(0),
  m_dataSize  (0)
{
}

CDVDAudioCodecPassthrough::~CDVDAudioCodecPassthrough(void)
{
  Dispose();
}

bool CDVDAudioCodecPassthrough::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  /* dont open if AE doesnt support RAW */
  if (!AE.SupportsRaw())
    return false;

  bool bSupportsAC3Out = false;
  bool bSupportsDTSOut = false;
  int audioMode = g_guiSettings.GetInt("audiooutput.mode");
  if (AUDIO_IS_BITSTREAM(audioMode))
  {
    bSupportsAC3Out = g_guiSettings.GetBool("audiooutput.ac3passthrough");
    bSupportsDTSOut = g_guiSettings.GetBool("audiooutput.dtspassthrough");
  }

  m_bufferSize = 0;

  if (
    (hints.codec == CODEC_ID_AC3  && bSupportsAC3Out) ||
    (hints.codec == CODEC_ID_DTS  && bSupportsDTSOut) ||
    (
      audioMode == AUDIO_HDMI &&
      (hints.codec == CODEC_ID_EAC3 && bSupportsAC3Out)
    )
  )
    return true;

  return false;
}

void CDVDAudioCodecPassthrough::Dispose()
{
  delete[] m_buffer;
  m_buffer     = NULL;
  m_bufferSize = 0;
}

int CDVDAudioCodecPassthrough::Decode(BYTE* pData, int iSize)
{
  if (iSize <= 0) return 0;

  unsigned int size = m_bufferSize;
  unsigned int used = m_info.AddData(pData, iSize, &m_buffer, &size);
  m_bufferSize = std::max(m_bufferSize, size);

  /* if we have a frame */
  if (size)
  {
    /* pack the data into an IEC958 frame */
    CAEPackIEC958::PackFunc pack = m_info.GetPackFunc();
    if (pack)
      m_dataSize = pack(m_buffer, size, m_packedBuffer);
  }

  return used;
}

int CDVDAudioCodecPassthrough::GetData(BYTE** dst)
{
  int size     = m_dataSize;
  m_dataSize   = 0;
  *dst         = m_packedBuffer;
  return size;
}

void CDVDAudioCodecPassthrough::Reset()
{
  m_dataSize = 0;
}

