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

#include "system.h"
#include "GUISound.h"
#include "AudioContext.h"
#include "settings/Settings.h"
#include "filesystem/File.h"
#include "utils/log.h"
#ifdef HAS_SDL_AUDIO
#include <SDL/SDL_mixer.h>
#endif
#include "filesystem/SpecialProtocol.h"
#ifndef HAS_SDL_AUDIO

typedef struct
{
  char chunk_id[4];
  long chunksize;
} WAVE_CHUNK;

typedef struct
{
  char riff[4];
  long filesize;
  char rifftype[4];
} WAVE_RIFFHEADER;
#else
#define GUI_SOUND_CHANNEL 0
#endif

CGUISound::CGUISound()
{
  m_soundBuffer=NULL;
}

CGUISound::~CGUISound()
{
#ifdef _WIN32
  FreeBuffer();
#elif defined(HAS_SDL_AUDIO)
  Mix_FreeChunk(m_soundBuffer);
#endif
}

// \brief Loads a wav file by filename
bool CGUISound::Load(const CStdString& strFile)
{
#ifdef _WIN32
  LPBYTE pbData=NULL;
  WAVEFORMATEX wfx;
  int size=0;
  if (!LoadWav(strFile, &wfx, &pbData, &size))
    return false;

  bool bReady=(CreateBuffer(&wfx, size) && FillBuffer(pbData, size));

  if (!bReady)
    FreeBuffer();

  delete[] pbData;

  return bReady;
#elif defined(HAS_SDL_AUDIO)
  m_soundBuffer = Mix_LoadWAV(_P(strFile));
  if (!m_soundBuffer)
    return false;

  return true;
#else
  return false;
#endif
}

// \brief Starts playback of the sound
void CGUISound::Play()
{
  if (m_soundBuffer)
  {
#ifdef _WIN32
    m_soundBuffer->Play(0, 0, 0);
#elif defined(HAS_SDL_AUDIO)
    Mix_PlayChannel(GUI_SOUND_CHANNEL, m_soundBuffer, 0);
#endif
  }
}

// \brief returns true if the sound is playing
bool CGUISound::IsPlaying()
{
#ifdef _WIN32
  if (m_soundBuffer)
  {
    DWORD dwStatus;
    m_soundBuffer->GetStatus(&dwStatus);
    return (dwStatus & DSBSTATUS_PLAYING);
  }

  return false;
#elif defined(HAS_SDL_AUDIO)
  return Mix_Playing(GUI_SOUND_CHANNEL) != 0;
#else
  return false;
#endif
}

// \brief Stops playback if the sound
void CGUISound::Stop()
{
  if (m_soundBuffer)
  {
#ifdef _WIN32
    m_soundBuffer->Stop();
#elif defined(HAS_SDL_AUDIO)
    Mix_HaltChannel(GUI_SOUND_CHANNEL);
#endif
    Wait();
  }
}

// \brief Sets the volume of the sound
void CGUISound::SetVolume(int level)
{
  if (m_soundBuffer)
  {
#ifdef _WIN32
    m_soundBuffer->SetVolume(level);
#elif defined(HAS_SDL_AUDIO)
    Mix_Volume(GUI_SOUND_CHANNEL, level);
#endif
  }
}

void CGUISound::Wait(uint32_t millis/*=500*/)
{
  millis /= 10;
  while (IsPlaying() && millis--)
    Sleep(10);
}

#ifdef _WIN32
bool CGUISound::CreateBuffer(LPWAVEFORMATEX wfx, int iLength)
{
  //  Set up DSBUFFERDESC structure
  DSBUFFERDESC dsbdesc;
  memset(&dsbdesc, 0, sizeof(DSBUFFERDESC));
  dsbdesc.dwSize=sizeof(DSBUFFERDESC);
  // directsound requires ctrlvolume to be set
  dsbdesc.dwFlags = DSBCAPS_CTRLVOLUME;
  dsbdesc.dwBufferBytes=iLength;
  dsbdesc.lpwfxFormat=wfx;

  LPDIRECTSOUND directSound=g_audioContext.GetDirectSoundDevice();
  if (!directSound)
    return false;

  //  Create buffer
  if (FAILED(directSound->CreateSoundBuffer(&dsbdesc, &m_soundBuffer, NULL)))
  {
    m_soundBuffer = NULL;
    CLog::Log(LOGERROR, __FUNCTION__" Creating sound buffer failed!");
    return false;
  }

  //  Make effects as loud as possible
  m_soundBuffer->SetVolume(g_settings.m_nVolumeLevel);

  return true;
}

bool CGUISound::FillBuffer(LPBYTE pbData, int iLength)
{
  if (!m_soundBuffer)
    return false;

  LPVOID lpvWrite;
  DWORD  dwLength;

  if (SUCCEEDED(m_soundBuffer->Lock(0, 0, &lpvWrite, &dwLength, NULL, NULL, DSBLOCK_ENTIREBUFFER)))
  {
    memcpy(lpvWrite, pbData, iLength);
    m_soundBuffer->Unlock(lpvWrite, dwLength, NULL, 0);
    return true;
  }

  CLog::Log(LOGERROR, __FUNCTION__" Filling sound buffer failed!");

  return false;
}

void CGUISound::FreeBuffer()
{
  if (IsPlaying())
    Stop();

  SAFE_RELEASE(m_soundBuffer);
}

bool CGUISound::LoadWav(const CStdString& strFile, WAVEFORMATEX* wfx, LPBYTE* ppWavData, int* pDataSize)
{
  XFILE::CFile file;
  if (!file.Open(strFile))
    return false;

  // read header
  WAVE_RIFFHEADER riffh;
  file.Read(&riffh, sizeof(WAVE_RIFFHEADER));

  // file valid?
  if (strncmp(riffh.riff, "RIFF", 4)!=0 && strncmp(riffh.rifftype, "WAVE", 4)!=0)
  {
    file.Close();
    return false;
  }

  long offset=0;
  offset += sizeof(WAVE_RIFFHEADER);
  offset -= sizeof(WAVE_CHUNK);

  // parse chunks
  do
  {
    WAVE_CHUNK chunk;

    // always seeking to the start of a chunk
    file.Seek(offset + sizeof(WAVE_CHUNK), SEEK_SET);
    file.Read(&chunk, sizeof(WAVE_CHUNK));

    if (!strncmp(chunk.chunk_id, "fmt ", 4))
    { // format chunk
      memset(wfx, 0, sizeof(WAVEFORMATEX));
      file.Read(wfx, 16);
      // we only need 16 bytes of the fmt chunk
      if (chunk.chunksize-16>0)
        file.Seek(chunk.chunksize-16, SEEK_CUR);
    }
    else if (!strncmp(chunk.chunk_id, "data", 4))
    { // data chunk
      *ppWavData=new BYTE[chunk.chunksize+1];
      file.Read(*ppWavData, chunk.chunksize);
      *pDataSize=chunk.chunksize;

      if (chunk.chunksize & 1)
        offset++;
    }
    else
    { // other chunk - unused, just skip
      file.Seek(chunk.chunksize, SEEK_CUR);
    }

    offset+=(chunk.chunksize+sizeof(WAVE_CHUNK));

    if (offset & 1)
      offset++;

  } while (offset+(int)sizeof(WAVE_CHUNK) < riffh.filesize);

  file.Close();
  return (*ppWavData!=NULL);
}

#endif
