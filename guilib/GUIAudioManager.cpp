#include "stdafx.h"
#include "GUIAudioManager.h"
#include "audiocontext.h"
#include "../xbmc/Util.h"
#include ".\guiaudiomanager.h"

typedef struct wav_header{ /* RIFF */
 char riff[4];         /* "RIFF" (4 bytes) */
 long TotLen;          /* File length - 8 bytes  (4 bytes) */
 char wavefmt[8];      /* "WAVEfmt "  (8 bytes) */
 long Len;             /* Remaining length  (4 bytes) */
 short format_tag;     /* Tag (1 = PCM) (2 bytes) */
 short channels;       /* Mono=1 Stereo=2 (2 bytes) */
 long SamplesPerSec;   /* No samples/sec (4 bytes) */
 long AvgBytesPerSec;  /* Average bytes/sec (4 bytes) */
 short BlockAlign;     /* Block align (2 bytes) */
 short FormatSpecific; /* 8 or 16 bit (2 bytes) */
 char data[4];         /* "data" (4 bytes) */
 long datalen;         /* Raw data length (4 bytes) */
 /* ..data follows. Total header size = 44 bytes */
} wav_header_s;

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
  (*ppSoundBuffer)->SetVolume(DSBVOLUME_MAX);
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

  FILE* fd = fopen(strFile, "rb");
  if (!fd)
    return false;

  //  Load WAV header
  wav_header_s header;
  fread(&header, 44, 1, fd);

  //  Load WAV data into memory
  LPBYTE pbData=new BYTE[header.datalen+1];
  fread(pbData, header.datalen, 1, fd);
  fclose(fd);

  // Set up wave format structure. 
  WAVEFORMATEX wfx; 
  memset(&wfx, 0, sizeof(WAVEFORMATEX)); 
  wfx.wFormatTag = header.format_tag; 
  wfx.nChannels = header.channels; 
  wfx.nSamplesPerSec = header.SamplesPerSec; 
  wfx.nBlockAlign = header.BlockAlign; 
  wfx.nAvgBytesPerSec = header.AvgBytesPerSec;
  wfx.wBitsPerSample = header.FormatSpecific; 
  wfx.cbSize = 0;

  bool bReady=(CreateBuffer(&wfx, header.datalen, ppSoundBuffer) && FillBuffer(pbData, header.datalen, *ppSoundBuffer));

  if (!bReady)
    FreeBuffer(ppSoundBuffer);

  delete[] pbData;

  return bReady;
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
