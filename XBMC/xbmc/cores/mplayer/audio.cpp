#include <xtl.h>
#include <stdio.h>
#include <stdlib.h>
#include "audio.h"
#include "ASyncDirectSound.h"
#include "IAudioCallback.h"
CASyncDirectSound* m_pAudioDecoder=NULL;

static IAudioCallback* m_pAudioCallback=NULL;

ao_info_t audio_info =  {
		"Windows waveOut audio output",
		"win32",
		"SaschaSommer <saschasommer@freenet.de>",
		""
};

ao_data_t* pao_data=NULL;
//******************************************************************************************
// to set/get/query special features/parameters
static int audio_control(int cmd,int arg)
{
	if (!m_pAudioDecoder) return CONTROL_OK;
  DWORD volume;
	DWORD maxvolume;
  switch (cmd)
  {
		case AOCONTROL_GET_VOLUME:
    {
      ao_control_vol_t* vol = (ao_control_vol_t*)arg;
      volume=m_pAudioDecoder->GetCurrentVolume();
			maxvolume=m_pAudioDecoder->GetMaximumVolume();
			vol->left  = ((float)volume) / ((float)maxvolume);
      vol->right = ((float)volume) / ((float)maxvolume);
      //mp_msg(MSGT_AO, MSGL_DBG2,"ao_win32: volume left:%f volume right:%f\n",vol->left,vol->right);
      return CONTROL_OK;
    }

		case AOCONTROL_SET_VOLUME:
    {
      ao_control_vol_t* vol = (ao_control_vol_t*)arg;
			maxvolume=m_pAudioDecoder->GetMaximumVolume();
      volume = (DWORD)(vol->left*((float)maxvolume));
      m_pAudioDecoder->SetCurrentVolume(volume);
      return CONTROL_OK;
    }
  }
	return CONTROL_OK;
}

void RegisterAudioCallback(IAudioCallback* pCallback)
{
	m_pAudioCallback=pCallback;
	if (m_pAudioDecoder) 
		m_pAudioDecoder->RegisterAudioCallback(pCallback);
}

void UnRegisterAudioCallback()
{
	if (m_pAudioDecoder) 
		m_pAudioDecoder->UnRegisterAudioCallback();
	m_pAudioCallback=NULL;
}

//******************************************************************************************
// open & setup audio device
// return: 1=success 0=fail
static int audio_init(int rate,int channels,int format,int flags)
{
		pao_data=GetAOData();
	  m_pAudioDecoder = new CASyncDirectSound(m_pAudioCallback,channels,rate,audio_out_format_bits(format));

    pao_data->channels	= channels;
    pao_data->samplerate= rate;
    pao_data->format		= format;
    pao_data->bps				= channels*rate;
		
    if(format != AFMT_U8 && format != AFMT_S8)
		{
        pao_data->bps *= (audio_out_format_bits(format)/8);
		}
    if(pao_data->buffersize==-1)
    {
        pao_data->buffersize  = audio_out_format_bits(format)/8;
        pao_data->buffersize *= channels;
        pao_data->buffersize *= m_pAudioDecoder->GetChunkLen();
    }
		pao_data->outburst=m_pAudioDecoder->GetChunkLen();
	 return 1;
}

//******************************************************************************************
// close audio device
static void audio_uninit()
{
   if (m_pAudioDecoder)
	 {
		 m_pAudioDecoder->Deinitialize();
		 delete m_pAudioDecoder;
		 m_pAudioDecoder=NULL;
	 }
}

//******************************************************************************************
// stop playing and empty buffers (for seeking/pause)
static void audio_reset()
{
//  m_pAudioDecoder->Stop();
}

//******************************************************************************************
// stop playing, keep buffers (for pause)
static void audio_pause()
{
	m_pAudioDecoder->Pause();
}

//******************************************************************************************
// resume playing, after audio_pause()
static void audio_resume()
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
static int audio_play(void* data,int len,int flags)
{
  if (!m_pAudioDecoder) return 0;
	return m_pAudioDecoder->AddPackets( (unsigned char*)data,len);
} 

//******************************************************************************************
// return: delay in seconds between first and last sample in buffer
static float audio_get_delay()
{
	if (!m_pAudioDecoder) return 0;
	FLOAT fDelay=m_pAudioDecoder->GetDelay();
  fDelay += (float)(m_pAudioDecoder->GetBytesInBuffer() + pao_data->buffersize) / (float)pao_data->bps;
	return fDelay;
	//return (float)(m_pAudioDecoder->GetBytesInBuffer() + pao_data->buffersize) / (float)pao_data->bps;
	
}

// to set/get/query special features/parameters
static int audio_control(int cmd,void *arg)
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


ao_functions_t audio_functions=
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

extern void xbox_audio_registercallback(IAudioCallback* pCallback)
{
	if (!m_pAudioDecoder) return;
	m_pAudioCallback=pCallback;
	m_pAudioDecoder->RegisterAudioCallback(pCallback);
}
extern void xbox_audio_unregistercallback()
{
	if (!m_pAudioDecoder) return;
	m_pAudioDecoder->UnRegisterAudioCallback();
	m_pAudioCallback=NULL;
}