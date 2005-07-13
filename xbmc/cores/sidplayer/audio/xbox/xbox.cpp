/***************************************************************************
audiodrv.cpp  -  ``DirectSound for Xbox''
specific audio driver interface.
-------------------
begin                : Mon Feb 18 2004
copyright            : (C) 2004 by Richard Crockford
email                : 
***************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#ifdef _XBOX
#include "./xbox.h"
#include "AudioContext.h"

#include <stdio.h>
#ifdef HAVE_EXCEPTIONS
#   include <new>
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }

Audio_Xbox::Audio_Xbox ()
{
  isOpen = false;
  pStream = 0;
  pDS = 0;
  memset(pMPacket, 0, sizeof(pMPacket));
}

Audio_Xbox::~Audio_Xbox()
{
  close();
}

void *Audio_Xbox::open (AudioConfig &cfg, const char *name)
{
  WAVEFORMATEX wfm;
  DSMIXBINVOLUMEPAIR Vols[2] = { DSMIXBINVOLUMEPAIRS_DEFAULT_STEREO };
  DSMIXBINS MixBins = { 2, Vols };

  if (isOpen)
  {
    _errorString = "XBOX ERROR: Audio device already open.";
    goto Audio_Xbox_openError;
  }

  isOpen = true;

  g_audioContext.RemoveActiveDevice();

  if (cfg.channels == 1)
    DirectSoundOverrideSpeakerConfig(DSSPEAKER_MONO);
  else if (cfg.channels == 2)
    DirectSoundOverrideSpeakerConfig(DSSPEAKER_STEREO);
  else
    DirectSoundOverrideSpeakerConfig(DSSPEAKER_USE_DEFAULT);

  g_audioContext.SetActiveDevice(CAudioContext::DIRECTSOUND_DEVICE);
  pDS=g_audioContext.GetDirectSoundDevice();

  XAudioCreatePcmFormat(cfg.channels, cfg.frequency, cfg.precision, &wfm);

  // Setup stream
  DSSTREAMDESC StrmDesc;
  StrmDesc.dwFlags = DSSTREAMCAPS_NOCOALESCE;
  StrmDesc.dwMaxAttachedPackets = AUDIO_XBOX_BUFFERS;
  StrmDesc.lpfnCallback = NULL;
  StrmDesc.lpwfxFormat = &wfm;
  StrmDesc.lpvContext = NULL;
  StrmDesc.lpMixBins = &MixBins;
  if (FAILED(pDS->CreateSoundStream(&StrmDesc, &pStream, 0)))
  {
    _errorString = "XBOX ERROR: Could not open audio stream.";
    goto Audio_Xbox_openError;
  }

  XMEDIAINFO info;
  pStream->GetInfo(&info);

  DWORD BufSize = wfm.nSamplesPerSec / 2 * wfm.nBlockAlign;
  if (BufSize % info.dwInputSize)
    BufSize += info.dwInputSize - (BufSize % info.dwInputSize); // align

  // Allocate stream buffers
  for (int i = 0; i < AUDIO_XBOX_BUFFERS; ++i)
  {
    pMPacket[i].dwMaxSize = BufSize;
    pMPacket[i].hCompletionEvent = CreateEvent(0, TRUE, TRUE, 0);
    pMPacket[i].pvBuffer = malloc(BufSize);
    pMPacket[i].pdwCompletedSize = &dwStreamed[i];
  }
  BufIdx = 0;

  // Update the users settings
  cfg.bufSize = BufSize;
  // Setup the required sample format encoding.
  cfg.encoding = AUDIO_SIGNED_PCM;
  if (cfg.precision == 8)
    cfg.encoding = AUDIO_UNSIGNED_PCM;
  _settings = cfg;
  isPlaying = true;
  _sampleBuffer = pMPacket[BufIdx].pvBuffer;

  return _sampleBuffer;

Audio_Xbox_openError:
  close();
  return NULL;
}

void *Audio_Xbox::write()
{
  if (!isOpen)
  {
    _errorString = "XBOX ERROR: Device not open.";
    return NULL;
  }

  if (!isPlaying)
  {
    pStream->Pause(DSSTREAMPAUSE_RESUME);
    isPlaying = true;
  }

  // queue buffer
  ResetEvent(pMPacket[BufIdx].hCompletionEvent);
  if (FAILED(pStream->Process(&pMPacket[BufIdx], 0)))
  {
    _errorString = "XBOX ERROR: Unable to lock sound buffer.";
    return NULL;
  }

  // wait for next buffer to become free
  ++BufIdx %= AUDIO_XBOX_BUFFERS;
  while (WaitForSingleObject(pMPacket[BufIdx].hCompletionEvent, 50) == WAIT_TIMEOUT)
    DirectSoundDoWork();

  _sampleBuffer = pMPacket[BufIdx].pvBuffer;
  return _sampleBuffer;
}

void *Audio_Xbox::reset(void)
{
  if (!isOpen)
    return NULL;

  // Stop play and kill the current music.
  // Start new music data being added at the begining of
  // the first buffer
  pStream->Flush();
  isPlaying = false;
  BufIdx = 0;

  _sampleBuffer = pMPacket[BufIdx].pvBuffer;
  return _sampleBuffer;
}

void Audio_Xbox::Eof()
{
  if (!isOpen)
    return ;

  pStream->Discontinuity();
  DirectSoundDoWork();
}

// Rev 1.8 (saw) - Alias fix
void Audio_Xbox::close(void)
{
  if (!isOpen)
    return ;

  isOpen = false;
  _sampleBuffer = NULL;

  if (pStream)
  {
    pStream->Flush();
    isPlaying = false;
  }

  for (int i = 0; i < AUDIO_XBOX_BUFFERS; ++i)
  {
    if (pMPacket[i].pvBuffer)
    {
      free(pMPacket[i].pvBuffer);
      pMPacket[i].pvBuffer = 0;
      CloseHandle(pMPacket[i].hCompletionEvent);
    }
  }

  SAFE_RELEASE (pStream);
  pDS=NULL;
  g_audioContext.RemoveActiveDevice();
  g_audioContext.SetActiveDevice(CAudioContext::DEFAULT_DEVICE);
}

void Audio_Xbox::pause(void)
{
  if (pStream)
  {
    pStream->Pause(DSSTREAMPAUSE_PAUSE);
    isPlaying = false;
  }
}

void Audio_Xbox::SetVolume(long nValue)
{
  if (pStream)
    pStream->SetVolume(nValue);
}
#endif
