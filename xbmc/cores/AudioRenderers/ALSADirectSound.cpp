/*
* XBMC Media Center
* Copyright (c) 2002 d7o3g4q and RUNTiME
* Portions Copyright (c) by the authors of ffmpeg and xvid
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "ALSADirectSound.h"
#include "guilib/AudioContext.h"
#include "filesystem/SpecialProtocol.h"
#include "settings/GUISettings.h"
#include "settings/Settings.h"
#include "utils/log.h"
#include "limits.h"
#include "guilib/LocalizeStrings.h"

#define CHECK_ALSA(l,s,e) if ((e)<0) CLog::Log(l,"%s - %s, alsa error: %d - %s",__FUNCTION__,s,e,snd_strerror(e));
#define CHECK_ALSA_RETURN(l,s,e) CHECK_ALSA((l),(s),(e)); if ((e)<0) return false;

using namespace std;

static CStdString QuoteDevice(const CStdString& device)
{
  CStdString result(device);
  result.Replace("'", "\\'");
  return "'" + result + "'";
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
//***********************************************************************************************
CALSADirectSound::CALSADirectSound()
{
  m_pPlayHandle  = NULL;
  m_bIsAllocated = false;
}

bool CALSADirectSound::Initialize(IAudioCallback* pCallback, const CStdString& device, int iChannels, enum PCMChannels *channelMap, unsigned int uiSamplesPerSec, unsigned int uiBitsPerSample, bool bResample, bool bIsMusic, EEncoded encoded)
{
  enum PCMChannels *outLayout;

  static enum PCMChannels ALSAChannelMap[8] =
  {
    PCM_FRONT_LEFT  , PCM_FRONT_RIGHT  ,
    PCM_BACK_LEFT   , PCM_BACK_RIGHT   ,
    PCM_FRONT_CENTER, PCM_LOW_FREQUENCY,
    PCM_SIDE_LEFT   , PCM_SIDE_RIGHT
  };
  static enum PCMChannels HDMIChannelMap[] = { PCM_FRONT_LEFT, PCM_FRONT_RIGHT
                                             , PCM_FRONT_CENTER, PCM_LOW_FREQUENCY
                                             , PCM_BACK_LEFT, PCM_BACK_RIGHT
                                             , PCM_FRONT_LEFT_OF_CENTER, PCM_FRONT_RIGHT_OF_CENTER
                                             , PCM_BACK_CENTER, PCM_SIDE_LEFT, PCM_SIDE_RIGHT};
  CStdString deviceuse(device);


  enum PCMChannels *output_map = ALSAChannelMap;
  if((deviceuse + ":").Left(5) == "hdmi:")
    output_map = HDMIChannelMap;

  /* setup the channel mapping */
  m_uiDataChannels = iChannels;
  m_remap.Reset();

  if (encoded == ENCODED_NONE && channelMap)
  {
    /* set the input format, and get the channel layout so we know what we need to open */
    outLayout = m_remap.SetInputFormat (iChannels, channelMap, uiBitsPerSample / 8, uiSamplesPerSec);
    unsigned int outChannels = 0;
    unsigned int ch = 0, map;
    while(outLayout[ch] != PCM_INVALID)
    {
      for(map = 0; map < 8; ++map)
        if (outLayout[ch] == output_map[map])
        {
          if (map > outChannels)
            outChannels = map;
          break;
        }
      ++ch;
    }

    m_remap.SetOutputFormat(++outChannels, output_map);
    if (m_remap.CanRemap())
    {
      iChannels = outChannels;
      if (m_uiDataChannels != (unsigned int)iChannels)
        CLog::Log(LOGDEBUG, "CALSADirectSound::CALSADirectSound - Requested channels changed from %i to %i", m_uiDataChannels, iChannels);
    }
  }

  bool bAudioOnAllSpeakers(false);
  g_audioContext.SetupSpeakerConfig(iChannels, bAudioOnAllSpeakers, bIsMusic);
  g_audioContext.SetActiveDevice(CAudioContext::DIRECTSOUND_DEVICE);

  m_pPlayHandle = NULL;
  m_bPause = false;
  m_bCanPause = false;
  m_bIsAllocated = false;
  m_uiChannels = iChannels;
  m_uiSamplesPerSec = uiSamplesPerSec;
  m_uiBitsPerSample = uiBitsPerSample;
  m_bPassthrough = encoded != ENCODED_NONE;
  m_drc = 0;

  m_nCurrentVolume = g_settings.m_nVolumeLevel;
  if (!m_bPassthrough)
     m_amp.SetVolume(m_nCurrentVolume);

  m_dwFrameCount = 512;
  m_dwNumPackets = 16;
  m_uiBufferSize = 0;

  snd_pcm_hw_params_t *hw_params=NULL;
  snd_pcm_sw_params_t *sw_params=NULL;

  /* Open the device */
  int nErr;

  /* if this is first access to audio, global sound config might not be loaded */
  if(!snd_config)
    snd_config_update();

  snd_config_t *config = snd_config;

  nErr = snd_config_copy(&config, snd_config);
  CHECK_ALSA_RETURN(LOGERROR,"config_copy",nErr);

  if(m_bPassthrough)
  {
    /* http://www.alsa-project.org/alsa-doc/alsa-lib/group___digital___audio___interface.html */
    deviceuse += (deviceuse.Find(':') >= 0) ? ',' : ':';
    deviceuse += "AES0=0x6";
    deviceuse += ",AES1=0x82";
    deviceuse += ",AES2=0x0";
    if(uiSamplesPerSec == 192000)
      deviceuse += ",AES3=0xe";
    else if(uiSamplesPerSec == 176400)
      deviceuse += ",AES3=0xc";
    else if(uiSamplesPerSec == 96000)
      deviceuse += ",AES3=0xa";
    else if(uiSamplesPerSec == 88200)
      deviceuse += ",AES3=0x8";
    else if(uiSamplesPerSec == 48000)
      deviceuse += ",AES3=0x2";
    else if(uiSamplesPerSec == 44100)
      deviceuse += ",AES3=0x0";
    else if(uiSamplesPerSec == 32000)
      deviceuse += ",AES3=0x3";
    else
      deviceuse += ",AES3=0x1";
  }
  else
  {
    if((deviceuse + ":").Left(5) == "hdmi:"
    || (deviceuse + ":").Left(7) == "iec958:"
    || (deviceuse + ":").Left(6) == "spdif:")
      deviceuse = "plug:" + QuoteDevice(deviceuse);

    if(deviceuse == "default")
      switch(iChannels)
      {
        case 8: deviceuse = "plug:surround71"; break;
        case 6: deviceuse = "plug:surround51"; break;
        case 5: deviceuse = "plug:surround50"; break;
        case 4: deviceuse = "plug:surround40"; break;
      }

    if(deviceuse != device)
    {
      snd_input_t* input;
      nErr = snd_input_stdio_open(&input, _P("special://xbmc/system/asound.conf").c_str(), "r");
      if(nErr >= 0)
      {
        nErr = snd_config_load(config, input);
        CHECK_ALSA_RETURN(LOGERROR,"config_load", nErr);

        snd_input_close(input);
        CHECK_ALSA_RETURN(LOGERROR,"input_close", nErr);
      }
      else
      {
        CLog::Log(LOGWARNING, "%s - Unable to load alsa configuration \"%s\" for device \"%s\" - %s", __FUNCTION__, "special://xbmc/system/asound.conf", deviceuse.c_str(), snd_strerror(nErr));
        deviceuse = device;
      }
    }
  }

  CLog::Log(LOGDEBUG, "%s - using alsa device %s", __FUNCTION__, deviceuse.c_str());

  nErr = snd_pcm_open_lconf(&m_pPlayHandle, deviceuse.c_str(), SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK, config);

  if(nErr == -EBUSY)
  {
    // this could happen if we are in the middle of a resolution switch sometimes
    CLog::Log(LOGERROR, "%s - device %s busy retrying...", __FUNCTION__, deviceuse.c_str());
    if(m_pPlayHandle)
    {
      snd_pcm_close(m_pPlayHandle);
      m_pPlayHandle = NULL;
    }
    Sleep(200);
    nErr = snd_pcm_open_lconf(&m_pPlayHandle, deviceuse.c_str(), SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK, config);
  }

  if(nErr < 0 && deviceuse != device)
  {
    CLog::Log(LOGERROR, "%s - failed to open custom device %s (error:%s), retry with default %s", __FUNCTION__, deviceuse.c_str(), snd_strerror(nErr), device.c_str());
    if(m_pPlayHandle)
    {
      snd_pcm_close(m_pPlayHandle);
      m_pPlayHandle = NULL;
    }
    nErr = snd_pcm_open_lconf(&m_pPlayHandle, device.c_str(), SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK, config);

  }

  CHECK_ALSA_RETURN(LOGERROR,"pcm_open_lconf",nErr);

  snd_config_delete(config);

  /* Allocate Hardware Parameters structures and fills it with config space for PCM */
  snd_pcm_hw_params_malloc(&hw_params);

  /* Allocate Software Parameters structures and fills it with config space for PCM */
  snd_pcm_sw_params_malloc(&sw_params);

  nErr = snd_pcm_hw_params_any(m_pPlayHandle, hw_params);
  CHECK_ALSA_RETURN(LOGERROR,"hw_params_any",nErr);

  nErr = snd_pcm_hw_params_set_access(m_pPlayHandle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
  CHECK_ALSA_RETURN(LOGERROR,"hw_params_set_access",nErr);

  // always use 16 bit samples
  nErr = snd_pcm_hw_params_set_format(m_pPlayHandle, hw_params, SND_PCM_FORMAT_S16);
  CHECK_ALSA_RETURN(LOGERROR,"hw_params_set_format",nErr);

  nErr = snd_pcm_hw_params_set_rate_near(m_pPlayHandle, hw_params, &m_uiSamplesPerSec, NULL);
  CHECK_ALSA_RETURN(LOGERROR,"hw_params_set_rate",nErr);

  nErr = snd_pcm_hw_params_set_channels(m_pPlayHandle, hw_params, iChannels);
  CHECK_ALSA_RETURN(LOGERROR,"hw_params_set_channels",nErr);

  nErr = snd_pcm_hw_params_set_period_size_near(m_pPlayHandle, hw_params, &m_dwFrameCount, NULL);
  CHECK_ALSA_RETURN(LOGERROR,"hw_params_set_period_size",nErr);

  nErr = snd_pcm_hw_params_set_periods_near(m_pPlayHandle, hw_params, &m_dwNumPackets, NULL);
  CHECK_ALSA_RETURN(LOGERROR,"hw_params_set_periods",nErr);

  nErr = snd_pcm_hw_params_get_buffer_size(hw_params, &m_uiBufferSize);
  CHECK_ALSA_RETURN(LOGERROR,"hw_params_get_buffer_size",nErr);

  /* Assign them to the playback handle and free the parameters structure */
  nErr = snd_pcm_hw_params(m_pPlayHandle, hw_params);
  CHECK_ALSA_RETURN(LOGERROR,"snd_pcm_hw_params",nErr);

  nErr = snd_pcm_sw_params_current(m_pPlayHandle, sw_params);
  CHECK_ALSA_RETURN(LOGERROR,"sw_params_current",nErr);

  nErr = snd_pcm_sw_params_set_start_threshold(m_pPlayHandle, sw_params, INT_MAX);
  CHECK_ALSA_RETURN(LOGERROR,"sw_params_set_start_threshold",nErr);

  snd_pcm_uframes_t boundary;
  nErr = snd_pcm_sw_params_get_boundary( sw_params, &boundary );
  CHECK_ALSA_RETURN(LOGERROR,"snd_pcm_sw_params_get_boundary",nErr);

  nErr = snd_pcm_sw_params_set_silence_threshold(m_pPlayHandle, sw_params, 0 );
  CHECK_ALSA_RETURN(LOGERROR,"snd_pcm_sw_params_set_silence_threshold",nErr);

  nErr = snd_pcm_sw_params_set_silence_size( m_pPlayHandle, sw_params, boundary );
  CHECK_ALSA_RETURN(LOGERROR,"snd_pcm_sw_params_set_silence_size",nErr);

  nErr = snd_pcm_sw_params(m_pPlayHandle, sw_params);
  CHECK_ALSA_RETURN(LOGERROR,"snd_pcm_sw_params",nErr);

  m_bCanPause    = !!snd_pcm_hw_params_can_pause(hw_params);

  snd_pcm_hw_params_free (hw_params);
  snd_pcm_sw_params_free (sw_params);


  CLog::Log(LOGDEBUG, "CALSADirectSound::Initialize - frame count:%u, packet count:%u, buffer size:%u"
                    , (unsigned int)m_dwFrameCount
                    , m_dwNumPackets
                    , (unsigned int)m_uiBufferSize);

  if(m_uiSamplesPerSec != uiSamplesPerSec)
    CLog::Log(LOGWARNING, "CALSADirectSound::CALSADirectSound - requested samplerate (%d) not supported by hardware, using %d instead", uiSamplesPerSec, m_uiSamplesPerSec);


  nErr = snd_pcm_prepare (m_pPlayHandle);
  CHECK_ALSA(LOGERROR,"snd_pcm_prepare",nErr);

  m_bIsAllocated = true;
  return true;
}

//***********************************************************************************************
CALSADirectSound::~CALSADirectSound()
{
  Deinitialize();
}


//***********************************************************************************************
bool CALSADirectSound::Deinitialize()
{
  m_bIsAllocated = false;
  if (m_pPlayHandle)
  {
    snd_pcm_drop(m_pPlayHandle);
    snd_pcm_close(m_pPlayHandle);
  }

  m_pPlayHandle=NULL;
  g_audioContext.SetActiveDevice(CAudioContext::DEFAULT_DEVICE);
  return true;
}

void CALSADirectSound::Flush()
{
  if (!m_bIsAllocated)
     return;

  int nErr = snd_pcm_drop(m_pPlayHandle);
  CHECK_ALSA(LOGERROR,"flush-drop",nErr);
  nErr = snd_pcm_prepare(m_pPlayHandle);
  CHECK_ALSA(LOGERROR,"flush-prepare",nErr);
}

//***********************************************************************************************
bool CALSADirectSound::Pause()
{
  if (!m_bIsAllocated)
     return -1;

  if (m_bPause) return true;
  m_bPause = true;

  snd_pcm_state_t state = snd_pcm_state(m_pPlayHandle);

  if(state != SND_PCM_STATE_RUNNING)
  {
    if(state != SND_PCM_STATE_PAUSED
    && state != SND_PCM_STATE_PREPARED)
    {
      CLog::Log(LOGWARNING, "CALSADirectSound::Pause - device in weird state %d", (int)state);
      Flush();
    }
    return true;
  }

  if(m_bCanPause)
  {
    int nErr = snd_pcm_pause(m_pPlayHandle,1); // this is not supported on all devices.
    CHECK_ALSA(LOGERROR,"pcm_pause",nErr);
    if(nErr<0)
      m_bCanPause = false;
  }

  if(!m_bCanPause)
  {
    snd_pcm_sframes_t avail = snd_pcm_avail_update(m_pPlayHandle);
    snd_pcm_sframes_t delay = 0;
    if(avail >= 0)
      delay = (snd_pcm_sframes_t)m_uiBufferSize - avail;

    CLog::Log(LOGWARNING, "CALSADirectSound::CALSADirectSound - device is not able to pause playback, will flush and prefix with %d frames", (int)delay);
    Flush();

    if(delay > 0)
    {
      void* silence = calloc(snd_pcm_frames_to_bytes(m_pPlayHandle, delay), 1);
      int nErr = snd_pcm_writei(m_pPlayHandle, silence, delay);
      CHECK_ALSA(LOGERROR,"snd_pcm_writei", nErr);
      free(silence);
    }
  }

  return true;
}

//***********************************************************************************************
bool CALSADirectSound::Resume()
{
  if (!m_bIsAllocated)
     return -1;

  snd_pcm_state_t state = snd_pcm_state(m_pPlayHandle);
  if(state == SND_PCM_STATE_PAUSED)
    snd_pcm_pause(m_pPlayHandle,0);

  if(state == SND_PCM_STATE_PREPARED)
  {
    snd_pcm_sframes_t avail = snd_pcm_avail_update(m_pPlayHandle);
    if(avail >= 0 && avail < (snd_pcm_sframes_t)m_uiBufferSize)
      snd_pcm_start(m_pPlayHandle);
  }

  m_bPause = false;

  return true;
}

//***********************************************************************************************
bool CALSADirectSound::Stop()
{
  if (!m_bIsAllocated)
     return -1;

  Flush();

  m_bPause = false;

  return true;
}

//***********************************************************************************************
long CALSADirectSound::GetCurrentVolume() const
{
  return m_nCurrentVolume;
}

//***********************************************************************************************
void CALSADirectSound::Mute(bool bMute)
{
  if (!m_bIsAllocated)
    return;

  if (bMute)
    SetCurrentVolume(VOLUME_MINIMUM);
  else
    SetCurrentVolume(m_nCurrentVolume);

}

//***********************************************************************************************
bool CALSADirectSound::SetCurrentVolume(long nVolume)
{
  if (!m_bIsAllocated) return -1;
  m_nCurrentVolume = nVolume;
  m_amp.SetVolume(nVolume);
  return true;
}


//***********************************************************************************************
unsigned int CALSADirectSound::GetSpaceFrames()
{
  if (!m_bIsAllocated) return 0;

  int nSpace = snd_pcm_avail_update(m_pPlayHandle);
  if (nSpace == 0)
  {
    snd_pcm_state_t state = snd_pcm_state(m_pPlayHandle);
    if(state != SND_PCM_STATE_RUNNING && state != SND_PCM_STATE_PREPARED && !m_bPause)
    {
      CLog::Log(LOGWARNING,"CALSADirectSound::GetSpace - buffer underun (%d)", state);
      Flush();
      return m_uiBufferSize;
    }
  }
  if (nSpace < 0)
  {
     CLog::Log(LOGWARNING,"CALSADirectSound::GetSpace - get space failed. err: %d (%s)", nSpace, snd_strerror(nSpace));
     Flush();
     return m_uiBufferSize;
  }
  return nSpace;
}

unsigned int CALSADirectSound::GetSpace()
{
  return GetSpaceFrames() * m_uiDataChannels * m_uiBitsPerSample / 8;
}

//***********************************************************************************************
unsigned int CALSADirectSound::AddPackets(const void* data, unsigned int len)
{
  if (!m_bIsAllocated)
  {
    CLog::Log(LOGERROR,"CALSADirectSound::AddPackets - sanity failed. no valid play handle!");
    return len;
  }
  // if we are paused we don't accept any data as pause doesn't always
  // work, and then playback would start again
  if(m_bPause)
    return 0;

  int framesToWrite, bytesToWrite;

  framesToWrite  = std::min(GetSpaceFrames(), len / ( m_uiDataChannels * m_uiBitsPerSample / 8 ) );
  framesToWrite /= m_dwFrameCount;
  framesToWrite *= m_dwFrameCount;
  bytesToWrite   = snd_pcm_frames_to_bytes(m_pPlayHandle, framesToWrite);

  if(framesToWrite == 0)
  {
    // if we haven't started playback, do so now
    if(snd_pcm_state(m_pPlayHandle) == SND_PCM_STATE_PREPARED && !m_bPause)
      snd_pcm_start(m_pPlayHandle);
    return 0;
  }

  int writeResult;
  if (m_bPassthrough && m_nCurrentVolume == VOLUME_MINIMUM)
  {
    char dummy[bytesToWrite];
    memset(dummy,0,sizeof(dummy));
    writeResult = snd_pcm_writei(m_pPlayHandle, dummy, framesToWrite);
  }
  else
  {
    if (m_remap.CanRemap())
    {
      /* remap the data to the correct channels */
      uint8_t outData[bytesToWrite];
      m_remap.Remap((void *)data, outData, framesToWrite, m_drc);
      m_amp.DeAmplify((short *)outData, bytesToWrite / 2);
      writeResult = snd_pcm_writei(m_pPlayHandle, outData, framesToWrite);
    }
    else
    {
      if (!m_bPassthrough)
        m_amp.DeAmplify((short *)data, framesToWrite * m_uiDataChannels);

      writeResult = snd_pcm_writei(m_pPlayHandle, data, framesToWrite);
    }
  }
  if (  writeResult == -EPIPE  )
  {
    CLog::Log(LOGDEBUG, "CALSADirectSound::AddPackets - buffer underun (tried to write %d frames)",
            framesToWrite);
    Flush();
    return 0;
  }
  else if (writeResult != framesToWrite)
  {
    CLog::Log(LOGERROR, "CALSADirectSound::AddPackets - failed to write %d frames. "
            "bad write (err: %d) - %s",
            framesToWrite, writeResult, snd_strerror(writeResult));
    Flush();
  }

  if (writeResult > 0)
  {
    if(snd_pcm_state(m_pPlayHandle) == SND_PCM_STATE_PREPARED && !m_bPause && GetSpaceFrames()  <= m_dwFrameCount)
      snd_pcm_start(m_pPlayHandle);
    
    return writeResult * m_uiBitsPerSample * m_uiDataChannels / 8;
  }

  return 0;
}

//***********************************************************************************************
float CALSADirectSound::GetDelay()
{
  if (!m_bIsAllocated)
    return 0.0;

  snd_pcm_sframes_t frames = 0;

  int nErr = snd_pcm_delay(m_pPlayHandle, &frames);
  CHECK_ALSA(LOGERROR,"snd_pcm_delay",nErr);
  if (nErr < 0)
  {
    frames = 0;
    Flush();
  }

  if (frames < 0)
  {
#if SND_LIB_VERSION >= 0x000901 /* snd_pcm_forward() exists since 0.9.0rc8 */
    snd_pcm_forward(m_pPlayHandle, -frames);
#endif
    frames = 0;
  }

  return (double)frames / m_uiSamplesPerSec;
}

float CALSADirectSound::GetCacheTime()
{
  return (float)(m_uiBufferSize - GetSpaceFrames()) / m_uiSamplesPerSec;
}

float CALSADirectSound::GetCacheTotal()
{
  return (float)m_uiBufferSize / m_uiSamplesPerSec;
}

//***********************************************************************************************
unsigned int CALSADirectSound::GetChunkLen()
{
  return m_dwFrameCount * m_uiDataChannels * m_uiBitsPerSample / 8;
}
//***********************************************************************************************
int CALSADirectSound::SetPlaySpeed(int iSpeed)
{
  return 0;
}

void CALSADirectSound::RegisterAudioCallback(IAudioCallback *pCallback)
{
  m_pCallback = pCallback;
}

void CALSADirectSound::UnRegisterAudioCallback()
{
  m_pCallback = NULL;
}

void CALSADirectSound::WaitCompletion()
{
  if (!m_bIsAllocated || m_bPause)
    return;

  snd_pcm_wait(m_pPlayHandle, -1);
}

void CALSADirectSound::SwitchChannels(int iAudioStream, bool bAudioOnAllSpeakers)
{
    return ;
}

void CALSADirectSound::EnumerateAudioSinks(AudioSinkList& vAudioSinks, bool passthrough)
{
  if (!passthrough)
  {
    vAudioSinks.push_back(AudioSink(g_localizeStrings.Get(409) + " (ALSA)", "alsa:default"));
    vAudioSinks.push_back(AudioSink("iec958 (ALSA)" , "alsa:plug:iec958"));
    vAudioSinks.push_back(AudioSink("hdmi (ALSA)"   , "alsa:plug:hdmi"));
  }
  else
  {
    vAudioSinks.push_back(AudioSink("iec958 (ALSA)" , "alsa:iec958"));
    vAudioSinks.push_back(AudioSink("hdmi (ALSA)"   , "alsa:hdmi"));
  }

  snd_ctl_t *handle;
  snd_ctl_card_info_t *info;
  snd_ctl_card_info_alloca( &info );
  CStdString strHwName;
  int n_cards = -1;

  while ( snd_card_next( &n_cards ) == 0 && n_cards >= 0 )
  {
    strHwName.Format("hw:%d", n_cards);
    if ( snd_ctl_open( &handle, strHwName.c_str(), 0 ) == 0 )
    {
      if ( snd_ctl_card_info( handle, info ) == 0 )
      {
        CStdString strReadableCardName = snd_ctl_card_info_get_name( info );
        CStdString strCardName = snd_ctl_card_info_get_id( info );

        int dev = -1;
        while( snd_ctl_pcm_next_device( handle, &dev ) == 0 && dev >= 0 )
        {
          if (!passthrough)
            GenSoundLabel(vAudioSinks, "default", strCardName, dev, strReadableCardName);
          GenSoundLabel(vAudioSinks, "iec958", strCardName, dev, strReadableCardName);
          GenSoundLabel(vAudioSinks, "hdmi", strCardName, dev, strReadableCardName);
        }
      }
      else
        CLog::Log(LOGERROR,"((ALSAENUM))control hardware info (%i): failed.\n", n_cards );
      snd_ctl_close( handle );
    }
    else
      CLog::Log(LOGERROR,"((ALSAENUM))control open (%i) failed.\n", n_cards );
  }
}

void CALSADirectSound::GenSoundLabel(AudioSinkList& vAudioSinks, CStdString sink, CStdString card, int dev, CStdString readableCard)
{
  CStdString deviceString;
  deviceString.Format("%s:CARD=%s,DEV=%d", sink, card.c_str(), dev);

  CStdString finalSink;
  finalSink.Format("alsa:%s", deviceString.c_str());
  CStdString label;
  label.Format("%s - %s - %d (ALSA)", readableCard.c_str(), sink.c_str(), dev);
  vAudioSinks.push_back(AudioSink(label, finalSink));
}
