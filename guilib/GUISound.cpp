#include "include.h"
#include "GUISound.h"
#include "AudioContext.h"
#include "../xbmc/Settings.h"
#ifdef HAS_SDL_AUDIO
#include <SDL/SDL_mixer.h>
#endif

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
#ifndef HAS_SDL_AUDIO
  FreeBuffer();  
#endif
}

// \brief Loads a wav file by filename
bool CGUISound::Load(const CStdString& strFile)
{
#ifndef HAS_SDL_AUDIO
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
#else
  m_soundBuffer = Mix_LoadWAV(strFile);
  if (!m_soundBuffer)
    return false;
    
  return true;    
#endif
}

// \brief Starts playback of the sound
void CGUISound::Play()
{
  if (m_soundBuffer)
#ifdef HAS_XBOX_AUDIO
    m_soundBuffer->Play(0, 0, DSBPLAY_FROMSTART);
#elif !defined(HAS_SDL_AUDIO)
    m_soundBuffer->Play(0, 0, 0);
#else
    Mix_PlayChannel(GUI_SOUND_CHANNEL, m_soundBuffer, 0);    
#endif
}

// \brief returns true if the sound is playing
bool CGUISound::IsPlaying()
{
#ifndef HAS_SDL_AUDIO
  if (m_soundBuffer)
  {
    DWORD dwStatus;
    m_soundBuffer->GetStatus(&dwStatus);
    return (dwStatus & DSBSTATUS_PLAYING);
  }

  return false;
#else
  return Mix_Playing(GUI_SOUND_CHANNEL) != 0;
#endif
}

// \brief Stops playback if the sound
void CGUISound::Stop()
{
  if (m_soundBuffer)
  {
#ifdef HAS_XBOX_AUDIO
    m_soundBuffer->StopEx( 0, DSBSTOPEX_IMMEDIATE );
#elif !defined(HAS_SDL_AUDIO)
    m_soundBuffer->Stop();
#else
    Mix_HaltChannel(GUI_SOUND_CHANNEL);    
#endif

    while(IsPlaying());
  }
}

// \brief Sets the volume of the sound
void CGUISound::SetVolume(int level)
{
  if (m_soundBuffer)
#ifndef HAS_SDL_AUDIO
    m_soundBuffer->SetVolume(level);
#else
    Mix_Volume(GUI_SOUND_CHANNEL, level);
#endif    
}

#ifndef HAS_SDL_AUDIO
bool CGUISound::CreateBuffer(LPWAVEFORMATEX wfx, int iLength)
{
#ifdef HAS_XBOX_AUDIO
  //  Use a volume pair preset
  DSMIXBINVOLUMEPAIR vp[2] = { DSMIXBINVOLUMEPAIRS_DEFAULT_STEREO };

  //  Set up DSMIXBINS structure
  DSMIXBINS mixbins;
  mixbins.dwMixBinCount=2;
  mixbins.lpMixBinVolumePairs=vp;
#endif

  //  Set up DSBUFFERDESC structure
  DSBUFFERDESC dsbdesc; 
  memset(&dsbdesc, 0, sizeof(DSBUFFERDESC)); 
  dsbdesc.dwSize=sizeof(DSBUFFERDESC);
#ifdef HAS_XBOX_AUDIO
  dsbdesc.dwFlags=0; 
  dsbdesc.lpMixBins=&mixbins;
#else
  // directsound requires ctrlvolume to be set
  dsbdesc.dwFlags = DSBCAPS_CTRLVOLUME;
#endif
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
  m_soundBuffer->SetVolume(g_stSettings.m_nVolumeLevel);
#ifdef HAS_XBOX_AUDIO
  m_soundBuffer->SetHeadroom(0);

  // Set the default mixbins headroom to appropriate level as set in the settings file (to allow the maximum volume)
  for (DWORD i = 0; i < mixbins.dwMixBinCount;i++)
    directSound->SetMixBinHeadroom(i, DWORD(g_advancedSettings.m_audioHeadRoom / 6));
#endif

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
  FILE* fd = fopen(strFile, "rb");
  if (!fd)
    return false;

  // read header
  WAVE_RIFFHEADER riffh;
  fread(&riffh, sizeof(WAVE_RIFFHEADER), 1, fd);

  // file valid?
  if (strncmp(riffh.riff, "RIFF", 4)!=0 && strncmp(riffh.rifftype, "WAVE", 4)!=0)
  {
    fclose(fd);
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
    fseek(fd, offset + sizeof(WAVE_CHUNK), SEEK_SET);
    fread(&chunk, sizeof(WAVE_CHUNK), 1, fd);

    if (!strncmp(chunk.chunk_id, "fmt ", 4))
    { // format chunk
      memset(wfx, 0, sizeof(WAVEFORMATEX));
      fread(wfx, 1, 16, fd);
      // we only need 16 bytes of the fmt chunk
      if (chunk.chunksize-16>0)
        fseek(fd, chunk.chunksize-16, SEEK_CUR);
    }
    else if (!strncmp(chunk.chunk_id, "data", 4))
    { // data chunk
      *ppWavData=new BYTE[chunk.chunksize+1];
      fread(*ppWavData, 1, chunk.chunksize, fd);
      *pDataSize=chunk.chunksize;

      if (chunk.chunksize & 1)
        offset++;
    }
    else
    { // other chunk - unused, just skip
      fseek(fd, chunk.chunksize, SEEK_CUR);
    }

    offset+=(chunk.chunksize+sizeof(WAVE_CHUNK));

    if (offset & 1)
      offset++;

  } while (offset+(int)sizeof(WAVE_CHUNK) < riffh.filesize);

  fclose(fd);
  return (*ppWavData!=NULL);
}

#endif
