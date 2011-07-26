/*
 *      Copyright (C) 2011 Team XBMC
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

#include <cstring>
#include <iostream>
#include "DVDAudioCodecPassthroughAudioFilter.h"

#include "DVDCodecs/DVDCodecs.h"
#include "DVDStreamInfo.h"
#include "settings/GUISettings.h"
#include "settings/Settings.h"
#include "utils/log.h"

//These values are forced to allow spdif out
const int OUT_SAMPLESIZE(16);
const int OUT_CHANNELS(2);

CDVDAudioCodecPassthroughAudioFilter::CDVDAudioCodecPassthroughAudioFilter(void)
  : m_Mhp(new AudioFilter::MultiHeaderParser())
  , m_Spdif(m_Mhp)
  , m_OutputBuffer(0)
  , m_OutputSize(0)
  , m_iSourceSampleRate(0)
{
  m_Mhp->addParser(new AudioFilter::DtsHeaderParser());
  m_Mhp->addParser(new AudioFilter::Ac3HeaderParser());
  m_Mhp->addParser(new AudioFilter::MpaHeaderParser());
}

CDVDAudioCodecPassthroughAudioFilter::~CDVDAudioCodecPassthroughAudioFilter(void)
{
  delete m_Mhp;
  Dispose();
}

bool CDVDAudioCodecPassthroughAudioFilter::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  const int audioMode(g_guiSettings.GetInt("audiooutput.mode"));

  if ( AUDIO_IS_BITSTREAM(audioMode) )
  {
    const bool bSupportsAC3Out(g_guiSettings.GetBool("audiooutput.ac3passthrough"));
    const bool bSupportsDTSOut(g_guiSettings.GetBool("audiooutput.dtspassthrough"));
    const bool bSupportsMP1Out(g_guiSettings.GetBool("audiooutput.passthroughmp1"));
    const bool bSupportsMP2Out(g_guiSettings.GetBool("audiooutput.passthroughmp2"));
    const bool bSupportsMP3Out(g_guiSettings.GetBool("audiooutput.passthroughmp3"));

  /* Samplerate cannot be checked here as we don't know it at this point in time.
   * We should probably have a way to try to decode data so that we know what samplerate it is.
   */
    if ( (hints.codec == CODEC_ID_AC3 && bSupportsAC3Out)
        || (hints.codec == CODEC_ID_DTS && bSupportsDTSOut)
        || (hints.codec == CODEC_ID_MP1 && bSupportsMP1Out)
        || (hints.codec == CODEC_ID_MP2 && bSupportsMP2Out)
        || (hints.codec == CODEC_ID_MP3 && bSupportsMP3Out) )
    {

      // TODO - this is only valid for video files, and should be moved somewhere else
      if( hints.channels == 2 && g_settings.m_currentVideoSettings.m_OutputToAllSpeakers )
      {
        CLog::Log(LOGINFO, "CDVDAudioCodecPassthroughAudioFilter::Open - disabled passthrough due to video OTAS");
        return false;
      }

      return true;
    }
  }

  return false;
}

void CDVDAudioCodecPassthroughAudioFilter::Dispose()
{
}

/* decodes data at pData up to iSize
 * on success, sets m_OutputSize and m_OutputBuffer
 */
int CDVDAudioCodecPassthroughAudioFilter::Decode(BYTE *pData, int iSize)
{
  if ( m_Spdif.parseFrame(pData, iSize) )
  {
    m_iSourceSampleRate = m_Spdif.getSpk().sample_rate;
    m_OutputBuffer = m_Spdif.getRawData();
    m_OutputSize = m_Spdif.getRawSize();
  }
  else
  {
    m_OutputSize = 0;
  }

  return iSize;
}

/* if there is a frame to read
 * sets dst to output buffer and returns number of bytes available
 * otherwise returns 0
 */
int CDVDAudioCodecPassthroughAudioFilter::GetData(BYTE** dst)
{
  if ( m_OutputSize )
  {
    *dst = m_OutputBuffer;
    int size(m_OutputSize);

    m_OutputSize = 0;
    return size;
  }

  return 0;
}

void CDVDAudioCodecPassthroughAudioFilter::Reset()
{
  m_OutputSize = 0;
}

int CDVDAudioCodecPassthroughAudioFilter::GetChannels()
{
  return OUT_CHANNELS;
}

int CDVDAudioCodecPassthroughAudioFilter::GetSampleRate()
{
  return m_iSourceSampleRate;
}

int CDVDAudioCodecPassthroughAudioFilter::GetBitsPerSample()
{
  return OUT_SAMPLESIZE;
}

