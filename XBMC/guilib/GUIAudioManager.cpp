#include "include.h"
#include "GUIAudioManager.h"
#include "audiocontext.h"
#include "../xbmc/settings.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

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

CGUIAudioManager g_audioManager;

CGUIAudioManager::CGUIAudioManager()
{
  m_lpDirectSound=NULL;
  m_lpActionSoundBuffer=NULL;
  m_lpStartSoundBuffer=NULL;
  m_bEnabled=true;
  g_audioContext.SetSoundDeviceCallback(this);
}

CGUIAudioManager::~CGUIAudioManager()
{

}

void CGUIAudioManager::Initialize()
{
  int iDevice=g_audioContext.GetActiveDevice();
  if (iDevice==CAudioContext::DEFAULT_DEVICE)
  {
    bool bAudioOnAllSpeakers=false;
    g_audioContext.SetupSpeakerConfig(2, bAudioOnAllSpeakers);
    g_audioContext.SetActiveDevice(CAudioContext::DIRECTSOUND_DEVICE);
  }
  else if (iDevice==CAudioContext::DIRECTSOUND_DEVICE)
  {
    m_lpDirectSound=g_audioContext.GetDirectSoundDevice();
  }
}

void CGUIAudioManager::DeInitialize()
{
  //  Wait for finish when an action sound is playing
  while(IsPlaying(m_lpActionSoundBuffer));
  FreeBuffer(&m_lpActionSoundBuffer);

  StopPlaying(m_lpStartSoundBuffer);
  FreeBuffer(&m_lpStartSoundBuffer);

  soundBufferMap::iterator it=m_windowSoundBuffers.begin();
  while (it!=m_windowSoundBuffers.end())
  {
    if (IsPlaying(it->second))
      StopPlaying(it->second);

    FreeBuffer(&it->second);
    it=m_windowSoundBuffers.erase(it);
  }

  m_lpDirectSound=NULL;
}

bool CGUIAudioManager::CreateBuffer(LPWAVEFORMATEX wfx, int iLength, LPDIRECTSOUNDBUFFER* ppSoundBuffer)
{
  StopPlaying(*ppSoundBuffer);
  FreeBuffer(ppSoundBuffer);

  //  Use a volume pair preset
  DSMIXBINVOLUMEPAIR vp[2] = { DSMIXBINVOLUMEPAIRS_DEFAULT_STEREO };

  //  Set up DSMIXBINS structure
  DSMIXBINS mixbins;
  mixbins.dwMixBinCount=2;
  mixbins.lpMixBinVolumePairs=vp;

  //  Set up DSBUFFERDESC structure
  DSBUFFERDESC dsbdesc; 
  memset(&dsbdesc, 0, sizeof(DSBUFFERDESC)); 
  dsbdesc.dwSize=sizeof(DSBUFFERDESC); 
  dsbdesc.dwFlags=0; 
  dsbdesc.dwBufferBytes=iLength; 
  dsbdesc.lpwfxFormat=wfx;
  dsbdesc.lpMixBins=&mixbins;

  //  Create buffer
  if (FAILED(m_lpDirectSound->CreateSoundBuffer(&dsbdesc, ppSoundBuffer, NULL)))
  {
    (*ppSoundBuffer) = NULL;
    CLog::Log(LOGERROR, "Sound Manager: Creating sound buffer failed!");
    return false;
  }

  //  Make effects a loud as possible
  (*ppSoundBuffer)->SetVolume(g_stSettings.m_nVolumeLevel);
  (*ppSoundBuffer)->SetHeadroom(0);

  // Set the default mixbins headroom to appropriate level as set in the settings file (to allow the maximum volume)
  for (DWORD i = 0; i < mixbins.dwMixBinCount;i++)
    m_lpDirectSound->SetMixBinHeadroom(i, DWORD(g_guiSettings.GetInt("AudioOutput.Headroom") / 6));

  return true; 
}

bool CGUIAudioManager::FillBuffer(LPBYTE pbData, int iLength, LPDIRECTSOUNDBUFFER pSoundBuffer)
{
  if (!pSoundBuffer)
    return false;

  LPVOID lpvWrite;
  DWORD  dwLength;

  if (SUCCEEDED(pSoundBuffer->Lock(0, 0, &lpvWrite, &dwLength, NULL, NULL, DSBLOCK_ENTIREBUFFER)))
  {
    fast_memcpy(lpvWrite, pbData, iLength);
    pSoundBuffer->Unlock(lpvWrite, dwLength, NULL, 0);
    return true;
  }

  CLog::Log(LOGERROR, "Sound Manager: Filling sound buffer failed!");

  return false;
}

void CGUIAudioManager::FreeBuffer(LPDIRECTSOUNDBUFFER* pSoundBuffer)
{
  SAFE_RELEASE(*pSoundBuffer);
}

// \brief Clear any unused audio buffers
void CGUIAudioManager::FreeUnused()
{
  //  Free sound buffer from actions
  if (!IsPlaying(m_lpActionSoundBuffer))
    FreeBuffer(&m_lpActionSoundBuffer);

  //  Free sound buffer from the start sound
  if (!IsPlaying(m_lpStartSoundBuffer))
    FreeBuffer(&m_lpStartSoundBuffer);

  //  Free sound buffers from windows
  soundBufferMap::iterator it=m_windowSoundBuffers.begin();
  while (it!=m_windowSoundBuffers.end())
  {
    if (!IsPlaying(it->second))
    {
      FreeBuffer(&it->second);
      it=m_windowSoundBuffers.erase(it);
    }
    else ++it;
  }
}

void CGUIAudioManager::StopPlaying(LPDIRECTSOUNDBUFFER pSoundBuffer)
{
  if (pSoundBuffer)
  {
    pSoundBuffer->StopEx( 0, DSBSTOPEX_IMMEDIATE );

    while(IsPlaying(pSoundBuffer));
  }
}

bool CGUIAudioManager::IsPlaying(LPDIRECTSOUNDBUFFER pSoundBuffer)
{
  if (pSoundBuffer)
  {
    DWORD dwStatus;
    pSoundBuffer->GetStatus(&dwStatus);
    return (dwStatus & DSBSTATUS_PLAYING);
  }

  return false;
}

bool CGUIAudioManager::CreateBufferFromFile(const CStdString& strFile, LPDIRECTSOUNDBUFFER* ppSoundBuffer)
{
  if (!m_lpDirectSound || !m_bEnabled)
    return false;

  LPBYTE pbData=NULL;
  WAVEFORMATEX wfx;
  int size=0;
  if (!LoadWav(strFile, &wfx, &pbData, &size))
    return false;

  bool bReady=(CreateBuffer(&wfx, size, ppSoundBuffer) && FillBuffer(pbData, size, *ppSoundBuffer));

  if (!bReady)
    FreeBuffer(ppSoundBuffer);

  delete[] pbData;

  return bReady;
}

bool CGUIAudioManager::LoadWav(const CStdString& strFile, WAVEFORMATEX* wfx, LPBYTE* ppWavData, int* pDataSize)
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

void CGUIAudioManager::Play(LPDIRECTSOUNDBUFFER pSoundBuffer)
{
  if (pSoundBuffer)
    pSoundBuffer->Play(0, 0, DSBPLAY_FROMSTART);
}

// \brief Play a sound associated with a CAction
void CGUIAudioManager::PlayActionSound(const CAction& action)
{
  actionSoundMap::iterator it=m_actionSoundMap.find(action.wID);
  if (it==m_actionSoundMap.end())
    return;

  CStdString strFile=m_strMediaDir+"\\"+it->second;
  if (CreateBufferFromFile(strFile, &m_lpActionSoundBuffer))
    Play(m_lpActionSoundBuffer);
}

// \brief Play a sound associated with a window and its event
// Events: SOUND_INIT, SOUND_DEINIT
void CGUIAudioManager::PlayWindowSound(DWORD dwID, WINDOW_SOUND event)
{
  windowSoundMap::iterator it=m_windowSoundMap.find(dwID);
  if (it==m_windowSoundMap.end())
    return;

  CWindowSounds sounds=it->second;
  CStdString strFile;
  switch (event)
  {
  case SOUND_INIT:
    strFile=sounds.strInitFile;
    break;
  case SOUND_DEINIT:
    strFile=sounds.strDeInitFile;
    break;
  }

  if (strFile.IsEmpty())
    return;

  //  One sound buffer for each window
  soundBufferMap::iterator itsb=m_windowSoundBuffers.find(dwID);
  if (itsb!=m_windowSoundBuffers.end())
  {
    if (IsPlaying(itsb->second))
      StopPlaying(itsb->second);
    FreeBuffer(&itsb->second);
    m_windowSoundBuffers.erase(itsb);
  }

  LPDIRECTSOUNDBUFFER pBuffer=NULL;
  if (CreateBufferFromFile(m_strMediaDir+"\\"+strFile, &pBuffer))
  {
    m_windowSoundBuffers.insert(pair<DWORD, LPDIRECTSOUNDBUFFER>(dwID, pBuffer));
    Play(pBuffer);
  }
}

// \brief Play the startup sound
void CGUIAudioManager::PlayStartSound()
{
  CStdString strFile="Q:\\media\\start.wav";
  if (CreateBufferFromFile(strFile, &m_lpStartSoundBuffer))
    Play(m_lpStartSoundBuffer);
}

// \brief Load the config file (sounds.xml) for nav sounds
// Can be located in a folder "sounds" in the skin or from a
// subfolder of the folder "sounds" in the root directory of
// xbmc
bool CGUIAudioManager::Load()
{
  m_actionSoundMap.clear();
  m_windowSoundMap.clear();

  if (g_guiSettings.GetString("LookAndFeel.SoundSkin")=="OFF")
    return true;

  if (g_guiSettings.GetString("LookAndFeel.SoundSkin")=="SKINDEFAULT")
    m_strMediaDir="Q:\\skin\\"+g_guiSettings.GetString("LookAndFeel.Skin")+"\\sounds";
  else
    m_strMediaDir="Q:\\sounds\\"+g_guiSettings.GetString("LookAndFeel.SoundSkin");
    
  CStdString strSoundsXml=m_strMediaDir+"\\sounds.xml";

  //  Load our xml file
  TiXmlDocument xmlDoc;

  CLog::Log(LOGINFO, "Loading %s", strSoundsXml.c_str());
  
  //  Load the config file
  if (!xmlDoc.LoadFile(strSoundsXml))
  {
    CLog::Log(LOGNOTICE, "%s, Line %d\n%s", strSoundsXml.c_str(), xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
    return false;
  }

  TiXmlElement* pRoot = xmlDoc.RootElement();
  CStdString strValue = pRoot->Value();
  if ( strValue != "sounds")
  {
    CLog::Log(LOGNOTICE, "%s Doesn't contain <sounds>", strSoundsXml.c_str());
    return false;
  }

  //  Load sounds for actions
  TiXmlElement* pActions = pRoot->FirstChildElement("actions");
  if (pActions)
  {
    TiXmlNode* pAction = pActions->FirstChild("action");

    while (pAction)
    {
      TiXmlNode* pIdNode = pAction->FirstChild("id");
      DWORD dwID = 0;    // action identity
      if (pIdNode && pIdNode->FirstChild())
      {
        CStdString strID = pIdNode->FirstChild()->Value();
        dwID=(DWORD)atol(strID.c_str());
      }

      TiXmlNode* pFileNode = pAction->FirstChild("file");
      CStdString strFile;
      if (pFileNode && pFileNode->FirstChild())
        strFile+=pFileNode->FirstChild()->Value();

      if (dwID > 0 && !strFile.IsEmpty())
        m_actionSoundMap.insert(pair<DWORD, CStdString>(dwID, strFile));

      pAction = pAction->NextSibling();
    }
  }

  //  Load window specific sounds
  TiXmlElement* pWindows = pRoot->FirstChildElement("windows");
  if (pWindows)
  {
    TiXmlNode* pWindow = pWindows->FirstChild("window");

    while (pWindow)
    {
      DWORD dwID = 0;

      TiXmlNode* pIdNode = pWindow->FirstChild("id");
      if (pIdNode)
      {
        if (pIdNode->FirstChild())
        {
          CStdString strID = pIdNode->FirstChild()->Value();
          dwID = (DWORD)atol(strID)+WINDOW_HOME;
        }
      }

      CWindowSounds sounds;
      LoadWindowSound(pWindow, "activate", sounds.strInitFile);
      LoadWindowSound(pWindow, "deactivate", sounds.strDeInitFile);

      if (dwID > 0)
        m_windowSoundMap.insert(pair<DWORD, CWindowSounds>(dwID, sounds));

      pWindow = pWindow->NextSibling();
    }
  }

  return true;
}

bool CGUIAudioManager::LoadWindowSound(TiXmlNode* pWindowNode, const CStdString& strIdentifier, CStdString& strFile)
{
  if (!pWindowNode)
    return false;

  TiXmlNode* pFileNode = pWindowNode->FirstChild(strIdentifier);
  if (pFileNode && pFileNode->FirstChild())
  {
    strFile = pFileNode->FirstChild()->Value();
    return true;
  }

  return false;
}

void CGUIAudioManager::Enable(bool bEnable)
{
  m_bEnabled=bEnable;
}

void CGUIAudioManager::SetVolume(int iLevel)
{
  if (m_lpActionSoundBuffer)
    m_lpActionSoundBuffer->SetVolume(iLevel);

  if (m_lpStartSoundBuffer)
    m_lpStartSoundBuffer->SetVolume(iLevel);

  soundBufferMap::iterator it=m_windowSoundBuffers.begin();
  while (it!=m_windowSoundBuffers.end())
  {
    if (it->second)
      it->second->SetVolume(iLevel);

    ++it;
  }
}
