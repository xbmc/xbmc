
#include "../../stdafx.h"
#include "audio.h"
#include "IDirectSoundRenderer.h"
#include "ASyncDirectSound.h"
#include "Ac97DirectSound.h"
#include "IAudioCallback.h"
#include "mplayer.h"
#include "../VideoRenderers/RenderManager.h"

IDirectSoundRenderer* m_pAudioDecoder = NULL;

static IAudioCallback* m_pAudioCallback = NULL;
static int m_bHasVideo = false;
void audio_uninit(int);

ao_info_t audio_info = {
                         "Windows waveOut audio output",
                         "win32",
                         "SaschaSommer <saschasommer@freenet.de>",
                         ""
                       };
extern "C" int mplayer_getVolume()
{
  if (!m_pAudioDecoder) return 0;
  return m_pAudioDecoder->GetCurrentVolume();
  /*  float fVolumeMin=(float)m_pAudioDecoder->GetMinimumVolume();
    float fVolumeMax=(float)m_pAudioDecoder->GetMaximumVolume();
    if (fVolumeMax > fVolumeMin)
    {
      float fWidth = (fVolumeMax-fVolumeMin);
      float fCurr  = m_pAudioDecoder->GetCurrentVolume() - fVolumeMin;
      float fPercent=(fCurr/fWidth)*100.0f;
      return (int)(fPercent+0.5);
    }
    else
    {
      float fWidth = (fVolumeMin-fVolumeMax);
      float fCurr  = m_pAudioDecoder->GetCurrentVolume() - fVolumeMax;
      float fPercent=(fCurr/fWidth)*100.0f;
      return (int)(fPercent+0.5);
    }*/
}

extern "C" void mplayer_setVolume(long nVolume)
{
  if (!m_pAudioDecoder) return ;
  m_pAudioDecoder->SetCurrentVolume(nVolume);
  /*  float fVolumeMin=(float)m_pAudioDecoder->GetMinimumVolume();
    float fVolumeMax=(float)m_pAudioDecoder->GetMaximumVolume();
    if (fVolumeMax > fVolumeMin)
    {
      float fWidth = (fVolumeMax-fVolumeMin);
      fWidth/=100.0f;
      fWidth*= ((float)iPercentage);
      fWidth+=fVolumeMin;
      m_pAudioDecoder->SetCurrentVolume( (LONG)fWidth);
    }
    else
    {
      float fWidth = (fVolumeMin-fVolumeMax);
      fWidth/=100.0f;
      fWidth*= ((float)iPercentage);
      fWidth+=fVolumeMax;
      m_pAudioDecoder->SetCurrentVolume( (LONG)fWidth);
    }*/
}

ao_data_t* pao_data = NULL;
//******************************************************************************************
// to set/get/query special features/parameters
static int audio_control(int cmd, int arg)
{
  if (!m_pAudioDecoder) return CONTROL_OK;
  DWORD volume;
  DWORD maxvolume;
  switch (cmd)
  {
  case AOCONTROL_GET_VOLUME:
    {
      ao_control_vol_t* vol = (ao_control_vol_t*)arg;
      volume = m_pAudioDecoder->GetCurrentVolume();
      maxvolume = m_pAudioDecoder->GetMaximumVolume();
      vol->left = ((float)volume) / ((float)maxvolume);
      vol->right = ((float)volume) / ((float)maxvolume);
      //mp_msg(MSGT_AO, MSGL_DBG2,"ao_win32: volume left:%f volume right:%f\n",vol->left,vol->right);
      return CONTROL_OK;
    }

  case AOCONTROL_SET_VOLUME:
    {
      ao_control_vol_t* vol = (ao_control_vol_t*)arg;
      maxvolume = m_pAudioDecoder->GetMaximumVolume();
      volume = (DWORD)(vol->left * ((float)maxvolume));
      m_pAudioDecoder->SetCurrentVolume(volume);
      return CONTROL_OK;
    }
  }
  return CONTROL_OK;
}

void RegisterAudioCallback(IAudioCallback* pCallback)
{
  m_pAudioCallback = pCallback;
  if (m_pAudioDecoder)
    m_pAudioDecoder->RegisterAudioCallback(pCallback);
}

void UnRegisterAudioCallback()
{
  if (m_pAudioDecoder)
    m_pAudioDecoder->UnRegisterAudioCallback();
  m_pAudioCallback = NULL;
}

//******************************************************************************************
// open & setup audio device
// return: 1=success 0=fail
static int audio_init(int rate, int channels, int format, int flags)
{
  char strFourCC[10];
  char strAudioCodec[128];
  long lBitRate;
  long lSampleRate;
  int iChannels;
  BOOL bVBR;
  bool bAC3PassThru = false;
  audio_uninit(1); //Make sure nothing else was uninted first. mplayer sometimes forgets.

  mplayer_GetAudioInfo(strFourCC, strAudioCodec, &lBitRate, &lSampleRate, &iChannels, &bVBR);

  int ao_format_bits;

  //Make sure we only accept formats that we can handle
  switch (format)
  {
  case AFMT_AC3:
    ao_format_bits = 16;
    break;
  case AFMT_S24_LE:
  case AFMT_S16_LE:
  case AFMT_S8:
  case AFMT_U8:  //Not sure about this one, added as we have handling for it later
    ao_format_bits = audio_out_format_bits(format);
    break;
  default:
    CLog::Log(LOGDEBUG, "ao_win32: format %d not supported defaulting to Signed 16-bit Little-Endian\n", format);
    format = AFMT_S16_LE;
    ao_format_bits = audio_out_format_bits(format);
  }


  pao_data = GetAOData();

  channels = pao_data->channels;
  m_bHasVideo = mplayer_HasVideo() == TRUE;

  //In the case of forced audio filter, channel number for the GetAudioInfo, and from pao_data
  //is not the same, so the follwing two lines are not correct
  //Make sure we only output as many channels as we how. don't try to create any virtual.
  //if (channels > iChannels) channels = iChannels;

  // Check whether we are passing digital output direct through.
  // Anything with 48kHz 2 channel audio can be passed direct.
  if (g_guiSettings.GetInt("AudioOutput.Mode") == AUDIO_DIGITAL)
  {
    // Check that we are allowed to pass through DD or DTS
    if (strstr(strAudioCodec, "SPDIF"))
      bAC3PassThru = true;
  }
  if (bAC3PassThru)
  {
    channels = 2;
    // ac3 passthru
    m_pAudioDecoder = new CAc97DirectSound(m_pAudioCallback, channels, rate, ao_format_bits, bAC3PassThru, false);
  }
  else
  { // check if we should resample this audio
    // currently we don't do this for videos for fear of CPU issues
    bool bResample(false);
    if( !mplayer_HasVideo())
    {
      bResample = true;
    }
    if ( channels == 3 || channels == 5 || channels > 6 )
      return 1;  // this is an ugly hack due to our code use mplayer_open_file for both playing file, and format detecttion

    /*if ( channels == 2 && !mplayer_HasVideo() && (lSampleRate == 48000 || bResample) && (g_guiSettings.GetInt("MusicAudioOutput.Mode") == AUDIO_DIGITAL) && g_guiSettings.GetBool("AudioOutput.PCMPassThrough")) // need add menu options here
      m_pAudioDecoder = new CAc97DirectSound(m_pAudioCallback, channels, rate, ao_format_bits, bAC3PassThru, bResample);
    else*/
      m_pAudioDecoder = new CASyncDirectSound(m_pAudioCallback, channels, rate, ao_format_bits, bResample, 0, strAudioCodec, !mplayer_HasVideo());
  }
  pao_data->channels = channels;
  pao_data->samplerate = rate;
  pao_data->format = format;
  pao_data->bps = channels * rate;
  if (format != AFMT_U8 && format != AFMT_S8)
  {
    pao_data->bps *= (ao_format_bits / 8);
  }

  //if(pao_data->buffersize==-1)
  //{
  //pao_data->buffersize  = ao_format_bits/8;
  //pao_data->buffersize *= channels;
  //pao_data->buffersize *= m_pAudioDecoder->GetChunkLen();
  pao_data->buffersize = channels * m_pAudioDecoder->GetChunkLen();
  //}
  pao_data->outburst = m_pAudioDecoder->GetChunkLen();
  return 1;
}

//******************************************************************************************
// close audio device
void audio_uninit(int immed)
{
  if (m_pAudioDecoder)
  {
    if (!immed)
      m_pAudioDecoder->WaitCompletion();
    delete m_pAudioDecoder;
    m_pAudioDecoder = NULL;
  }
}

//******************************************************************************************
// stop playing and empty buffers (for seeking/pause)
static void audio_reset()
{
  m_pAudioDecoder->Stop();
}

//******************************************************************************************
// stop playing, keep buffers (for pause)
void audio_pause()
{
  m_pAudioDecoder->Pause();
}

//******************************************************************************************
// resume playing, after audio_pause()
void audio_resume()
{
  m_pAudioDecoder->Resume();
}

//******************************************************************************************
// return: how many bytes can be played without blocking
static int audio_get_space()
{
  if (!m_pAudioDecoder) return 0;
  return m_pAudioDecoder->GetSpace();
}

//******************************************************************************************
// plays 'len' bytes of 'data'
// it should round it down to outburst*n
// return: number of bytes played
static int audio_play(void* data, int len, int flags)
{
  if (!m_pAudioDecoder) return 0;
  //if we have video, don't process any audio before video is ready to go.
  if (m_bHasVideo && (!g_renderManager.IsStarted())) return 0;
  return m_pAudioDecoder->AddPackets( (unsigned char*)data, len);
}

//******************************************************************************************
// return: delay in seconds between first and last sample in buffer
static float audio_get_delay()
{
  if (!m_pAudioDecoder) return 0;
  FLOAT fDelay = m_pAudioDecoder->GetDelay();
  // check our output rate...
  fDelay += (float)pao_data->buffersize / (float)pao_data->bps; //data in pao_data buffer is not resample yet
  if (m_pAudioDecoder->IsResampling())
    fDelay += (float)m_pAudioDecoder->GetBytesInBuffer() / (float)48000 * pao_data->channels * 2;
  else
    fDelay += (float)m_pAudioDecoder->GetBytesInBuffer() / (float)pao_data->bps;
  return fDelay;

  //mplayer:
  //return (float)(buffered_bytes + ao_data.buffersize)/(float)ao_data.bps;
}

// to set/get/query special features/parameters
static int audio_control(int cmd, void *arg)
{
  //    DWORD volume;
  switch (cmd)
  {
  case AOCONTROL_GET_VOLUME:
    {
      //ao_control_vol_t* vol = (ao_control_vol_t*)arg;
      //waveOutGetVolume(hWaveOut,&volume);
      //vol->left = (float)(LOWORD(volume)/655.35);
      //vol->right = (float)(HIWORD(volume)/655.35);
      //mp_msg(MSGT_AO, MSGL_DBG2,"ao_win32: volume left:%f volume right:%f\n",vol->left,vol->right);
      return CONTROL_OK;
    }
  case AOCONTROL_SET_VOLUME:
    {
      //ao_control_vol_t* vol = (ao_control_vol_t*)arg;
      //volume = MAKELONG(vol->left*655.35,vol->right*655.35);
      //waveOutSetVolume(hWaveOut,volume);
      return CONTROL_OK;
    }
  }
  return -1;
}


ao_functions_t audio_functions =
  {
    &audio_info,
    audio_control,
    audio_init,
    audio_uninit,
    audio_reset,
    audio_get_space,
    audio_play,
    audio_get_delay,
    audio_pause,
    audio_resume
  };

void xbox_audio_registercallback(IAudioCallback* pCallback)
{
  if (!m_pAudioDecoder) return ;
  m_pAudioCallback = pCallback;
  m_pAudioDecoder->RegisterAudioCallback(pCallback);
}
void xbox_audio_unregistercallback()
{
  if (!m_pAudioDecoder) return ;
  m_pAudioDecoder->UnRegisterAudioCallback();
  m_pAudioCallback = NULL;
}


void xbox_audio_wait_completion()
{
  if (!m_pAudioDecoder)
    return ;
  m_pAudioDecoder->WaitCompletion();
}

void xbox_audio_do_work()
{
  if (!m_pAudioDecoder)
    return ;
  m_pAudioDecoder->DoWork();
}

void xbox_audio_switch_channel(int iAudioStream, bool bAudioOnAllSpeakers)
{
  if (!m_pAudioDecoder)
    return ;
  m_pAudioDecoder->SwitchChannels(iAudioStream, bAudioOnAllSpeakers);
}
