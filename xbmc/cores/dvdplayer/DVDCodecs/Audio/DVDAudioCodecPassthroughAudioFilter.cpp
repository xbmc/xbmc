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
#include "settings/AdvancedSettings.h"
#include "utils/log.h"

//These values are forced to allow spdif out
const int OUT_SAMPLESIZE(16);

CDVDAudioCodecPassthroughAudioFilter::CDVDAudioCodecPassthroughAudioFilter(void)
  : m_Mhp(new AudioFilter::MultiHeaderParser())
  , m_Spdif(0)
  , m_OutputBuffer(0)
  , m_OutputSize(0)
  , m_CodecId(0)
{
  m_Mhp->addParser(new AudioFilter::DtsHeaderParser());
  m_Mhp->addParser(new AudioFilter::Ac3HeaderParser());
  m_Mhp->addParser(new AudioFilter::MpaHeaderParser());
}

CDVDAudioCodecPassthroughAudioFilter::~CDVDAudioCodecPassthroughAudioFilter(void)
{
  if ( m_Spdif )
    delete m_Spdif;

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

      m_CodecId = hints.codec;
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
  if ( m_Spdif )
    m_Spdif->parseFrame(pData, iSize);
  else if ( ! SetupParser(pData, iSize) )
  {
    CLog::Log(LOGWARNING, "CDVDAudioCodecPassthroughAudioFilter::SetupParser:"
				 " unable to parse initial frame");
  }

  m_OutputBuffer = m_Spdif->getRawData();
  m_OutputSize = m_Spdif->getRawSize();
  return iSize;
}

bool CDVDAudioCodecPassthroughAudioFilter::SetupParser(BYTE *pData, int iSize)
{
  if ( m_Spdif )
  {
    delete m_Spdif;
    m_Spdif = 0;
  }

  m_Spdif = new AudioFilter::SpdifWrapper(m_Mhp);
  int maxBandwidth(g_advancedSettings.m_maxPassthroughBandwidth);

  if ( maxBandwidth >= 768000 )
  {
    CLog::Log(LOGDEBUG, "CDVDAudioCodecPassthroughAudioFilter::SetupParser: adding 8x192000 mapping");
    m_Spdif->addChannelMap(8, 192000);
  }

  if ( maxBandwidth >= 192000 )
  {
	if ( maxBandwidth == 192001 ) // magic number to trigger 8 channel * 48k
	{
      CLog::Log(LOGDEBUG, "CDVDAudioCodecPassthroughAudioFilter::SetupParser: adding 8x48000 mapping");
      m_Spdif->addChannelMap(8, 48000);
	}
	else
	{
      CLog::Log(LOGDEBUG, "CDVDAudioCodecPassthroughAudioFilter::SetupParser: adding 2x192000 mapping");
      m_Spdif->addChannelMap(2, 192000);
	}
  }

  CLog::Log(LOGDEBUG, "CDVDAudioCodecPassthroughAudioFilter::SetupParser: adding 2x48000 mapping");
  m_Spdif->addChannelMap(2, 48000);

  return m_Spdif->parseFrame(pData, iSize);
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
  return (m_Spdif) ? m_Spdif->getSpeakers().getChannelCount() : 2;
}

int CDVDAudioCodecPassthroughAudioFilter::GetSampleRate()
{
  return (m_Spdif) ? m_Spdif->getSpeakers().getSampleRate() : 48000;
}

int CDVDAudioCodecPassthroughAudioFilter::GetBitsPerSample()
{
  return OUT_SAMPLESIZE;
}

