/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#include "system.h"
#include "Visualisation.h"
#include "utils/fft.h"
#include "GUIInfoManager.h"
#include "Application.h"
#include "music/tags/MusicInfoTag.h"
#include "settings/Settings.h"
#include "settings/AdvancedSettings.h"
#include "windowing/WindowingFactory.h"
#include "utils/URIUtils.h"
#include "utils/StringUtils.h"
#include "cores/AudioEngine/AEFactory.h"
#include "cores/AudioEngine/Utils/AEConvert.h"
#ifdef _LINUX
#include <dlfcn.h>
#include "filesystem/SpecialProtocol.h"
#endif

using namespace std;
using namespace MUSIC_INFO;
using namespace ADDON;

CAudioBuffer::CAudioBuffer(int iSize)
{
  m_iLen = iSize;
  m_pBuffer = new float[iSize];
}

CAudioBuffer::~CAudioBuffer()
{
  delete [] m_pBuffer;
}

const float* CAudioBuffer::Get() const
{
  return m_pBuffer;
}

void CAudioBuffer::Set(const float* psBuffer, int iSize)
{
  if (iSize<0)
    return;
  memcpy(m_pBuffer, psBuffer, iSize * sizeof(float));
  for (int i = iSize; i < m_iLen; ++i) m_pBuffer[i] = 0;
}

bool CVisualisation::Create(int x, int y, int w, int h)
{
  m_pInfo = new VIS_PROPS;
  #ifdef HAS_DX
  m_pInfo->device     = g_Windowing.Get3DDevice();
#else
  m_pInfo->device     = NULL;
#endif
  m_pInfo->x = x;
  m_pInfo->y = y;
  m_pInfo->width = w;
  m_pInfo->height = h;
  m_pInfo->pixelRatio = g_settings.m_ResInfo[g_graphicsContext.GetVideoResolution()].fPixelRatio;

  m_pInfo->name = strdup(Name().c_str());
  m_pInfo->presets = strdup(CSpecialProtocol::TranslatePath(Path()).c_str());
  m_pInfo->profile = strdup(CSpecialProtocol::TranslatePath(Profile()).c_str());
  m_pInfo->submodule = NULL;

  if (CAddonDll<DllVisualisation, Visualisation, VIS_PROPS>::Create() == ADDON_STATUS_OK)
  {
    // Start the visualisation
    CStdString strFile = URIUtils::GetFileName(g_application.CurrentFile());
    CLog::Log(LOGDEBUG, "Visualisation::Start()\n");
    try
    {
      m_pStruct->Start(m_iChannels, m_iSamplesPerSec, m_iBitsPerSample, strFile);
    }
    catch (std::exception e)
    {
      HandleException(e, "m_pStruct->Start() (CVisualisation::Create)");
      return false;
    }

    GetPresets();

    if (GetSubModules())
      m_pInfo->submodule = strdup(CSpecialProtocol::TranslatePath(m_submodules.front()).c_str());
    else
      m_pInfo->submodule = NULL;

    CreateBuffers();

    if (g_application.m_pPlayer)
      g_application.m_pPlayer->RegisterAudioCallback(this);

    return true;
  }
  return false;
}

void CVisualisation::Start(int iChannels, int iSamplesPerSec, int iBitsPerSample, const CStdString strSongName)
{
  // notify visz. that new song has been started
  // pass it the nr of audio channels, sample rate, bits/sample and offcourse the songname
  if (Initialized())
  {
    try
    {
      m_pStruct->Start(iChannels, iSamplesPerSec, iBitsPerSample, strSongName.c_str());
    }
    catch (std::exception e)
    {
      HandleException(e, "m_pStruct->Start (CVisualisation::Start)");
    }
  }
}

void CVisualisation::AudioData(const float* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength)
{
  // pass audio data to visz.
  // audio data: is short audiodata [channel][iAudioDataLength] containing the raw audio data
  // iAudioDataLength = length of audiodata array
  // pFreqData = fft-ed audio data
  // iFreqDataLength = length of pFreqData
  if (Initialized())
  {
    try
    {
      m_pStruct->AudioData(pAudioData, iAudioDataLength, pFreqData, iFreqDataLength);
    }
    catch (std::exception e)
    {
      HandleException(e, "m_pStruct->AudioData (CVisualisation::AudioData)");
    }
  }
}

void CVisualisation::Render()
{
  // ask visz. to render itself
  g_graphicsContext.BeginPaint();
  if (Initialized())
  {
    try
    {
      m_pStruct->Render();
    }
    catch (std::exception e)
    {
      HandleException(e, "m_pStruct->Render (CVisualisation::Render)");
    }
  }
  g_graphicsContext.EndPaint();
}

void CVisualisation::Stop()
{
  if (g_application.m_pPlayer) g_application.m_pPlayer->UnRegisterAudioCallback();
  if (Initialized())
  {
    CAddonDll<DllVisualisation, Visualisation, VIS_PROPS>::Stop();
  }
}

void CVisualisation::GetInfo(VIS_INFO *info)
{
  if (Initialized())
  {
    try
    {
      m_pStruct->GetInfo(info);
    }
    catch (std::exception e)
    {
      HandleException(e, "m_pStruct->GetInfo (CVisualisation::GetInfo)");
    }
  }
}

bool CVisualisation::OnAction(VIS_ACTION action, void *param)
{
  if (!Initialized())
    return false;

  // see if vis wants to handle the input
  // returns false if vis doesnt want the input
  // returns true if vis handled the input
  try
  {
    if (action != VIS_ACTION_NONE && m_pStruct->OnAction)
    {
      // if this is a VIS_ACTION_UPDATE_TRACK action, copy relevant
      // tags from CMusicInfoTag to VisTag
      if ( action == VIS_ACTION_UPDATE_TRACK && param )
      {
        const CMusicInfoTag* tag = (const CMusicInfoTag*)param;
        CStdString artist(StringUtils::Join(tag->GetArtist(), g_advancedSettings.m_musicItemSeparator));
        CStdString albumArtist(StringUtils::Join(tag->GetAlbumArtist(), g_advancedSettings.m_musicItemSeparator));
        CStdString genre(StringUtils::Join(tag->GetGenre(), g_advancedSettings.m_musicItemSeparator));
        
        VisTrack track;
        track.title       = tag->GetTitle().c_str();
        track.artist      = artist.c_str();
        track.album       = tag->GetAlbum().c_str();
        track.albumArtist = albumArtist.c_str();
        track.genre       = genre.c_str();
        track.comment     = tag->GetComment().c_str();
        track.lyrics      = tag->GetLyrics().c_str();
        track.trackNumber = tag->GetTrackNumber();
        track.discNumber  = tag->GetDiscNumber();
        track.duration    = tag->GetDuration();
        track.year        = tag->GetYear();
        track.rating      = tag->GetRating();

        return m_pStruct->OnAction(action, &track);
      }
      return m_pStruct->OnAction((int)action, param);
    }
  }
  catch (std::exception e)
  {
    HandleException(e, "m_pStruct->OnAction (CVisualisation::OnAction)");
  }
  return false;
}

void CVisualisation::OnInitialize(int iChannels, int iSamplesPerSec, int iBitsPerSample)
{
  if (!m_pStruct)
    return ;
  CLog::Log(LOGDEBUG, "OnInitialize() started");

  m_iChannels = iChannels;
  m_iSamplesPerSec = iSamplesPerSec;
  m_iBitsPerSample = iBitsPerSample;
  UpdateTrack();

  CLog::Log(LOGDEBUG, "OnInitialize() done");
}

void CVisualisation::OnAudioData(const float* pAudioData, int iAudioDataLength)
{
  if (!m_pStruct)
    return ;

  // FIXME: iAudioDataLength should never be less than 0
  if (iAudioDataLength<0)
    return;

  // Save our audio data in the buffers
  auto_ptr<CAudioBuffer> pBuffer ( new CAudioBuffer(AUDIO_BUFFER_SIZE) );
  pBuffer->Set(pAudioData, iAudioDataLength);
  m_vecBuffers.push_back( pBuffer.release() );

  if ( (int)m_vecBuffers.size() < m_iNumBuffers) return ;

  auto_ptr<CAudioBuffer> ptrAudioBuffer ( m_vecBuffers.front() );
  m_vecBuffers.pop_front();
  // Fourier transform the data if the vis wants it...
  if (m_bWantsFreq)
  {
    const float *psAudioData = ptrAudioBuffer->Get();
    memcpy(m_fFreq, psAudioData, AUDIO_BUFFER_SIZE * sizeof(float));

    // FFT the data
    twochanwithwindow(m_fFreq, AUDIO_BUFFER_SIZE);

    // Normalize the data
    float fMinData = (float)AUDIO_BUFFER_SIZE * AUDIO_BUFFER_SIZE * 3 / 8 * 0.5 * 0.5; // 3/8 for the Hann window, 0.5 as minimum amplitude
    float fInvMinData = 1.0f/fMinData;
    for (int i = 0; i < AUDIO_BUFFER_SIZE + 2; i++)
    {
      m_fFreq[i] *= fInvMinData;
    }

    // Transfer data to our visualisation
    AudioData(psAudioData, AUDIO_BUFFER_SIZE, m_fFreq, AUDIO_BUFFER_SIZE);
  }
  else
  { // Transfer data to our visualisation
    AudioData(ptrAudioBuffer->Get(), AUDIO_BUFFER_SIZE, NULL, 0);
  }
  return ;
}

void CVisualisation::CreateBuffers()
{
  ClearBuffers();

  // Get the number of buffers from the current vis
  VIS_INFO info;
  m_pStruct->GetInfo(&info);
  m_iNumBuffers = info.iSyncDelay + 1;
  m_bWantsFreq = (info.bWantsFreq != 0);
  if (m_iNumBuffers > MAX_AUDIO_BUFFERS)
    m_iNumBuffers = MAX_AUDIO_BUFFERS;
  if (m_iNumBuffers < 1)
    m_iNumBuffers = 1;
}

void CVisualisation::ClearBuffers()
{
  m_bWantsFreq = false;
  m_iNumBuffers = 0;

  while (m_vecBuffers.size() > 0)
  {
    CAudioBuffer* pAudioBuffer = m_vecBuffers.front();
    delete pAudioBuffer;
    m_vecBuffers.pop_front();
  }
  for (int j = 0; j < AUDIO_BUFFER_SIZE*2; j++)
  {
    m_fFreq[j] = 0.0f;
  }
}

bool CVisualisation::UpdateTrack()
{
  bool handled = false;
  if (Initialized())
  {
    // get the current album art filename
    m_AlbumThumb = CSpecialProtocol::TranslatePath(g_infoManager.GetImage(MUSICPLAYER_COVER, WINDOW_INVALID));

    // get the current track tag
    const CMusicInfoTag* tag = g_infoManager.GetCurrentSongTag();

    if (m_AlbumThumb == "DefaultAlbumCover.png")
      m_AlbumThumb = "";
    else
      CLog::Log(LOGDEBUG,"Updating visualisation albumart: %s", m_AlbumThumb.c_str());

    // inform the visualisation of the current album art
    if (OnAction( VIS_ACTION_UPDATE_ALBUMART, (void*)( m_AlbumThumb.c_str() ) ) )
      handled = true;

    // inform the visualisation of the current track's tag information
    if ( tag && OnAction( VIS_ACTION_UPDATE_TRACK, (void*)tag ) )
      handled = true;
  }
  return handled;
}

bool CVisualisation::GetPresetList(std::vector<CStdString> &vecpresets)
{
  vecpresets = m_presets;
  return !m_presets.empty();
}

bool CVisualisation::GetPresets()
{
  m_presets.clear();
  char **presets = NULL;
  unsigned int entries = 0;
  try
  {
    entries = m_pStruct->GetPresets(&presets);
  }
  catch (std::exception e)
  {
    HandleException(e, "m_pStruct->OnAction (CVisualisation::GetPresets)");
    return false;
  }
  if (presets && entries > 0)
  {
    for (unsigned i=0; i < entries; i++)
    {
      if (presets[i])
      {
        m_presets.push_back(presets[i]);
      }
    }
  }
  return (!m_presets.empty());
}

bool CVisualisation::GetSubModuleList(std::vector<CStdString> &vecmodules)
{
  vecmodules = m_submodules;
  return !m_submodules.empty();
}

bool CVisualisation::GetSubModules()
{
  m_submodules.clear();
  char **modules = NULL;
  unsigned int entries = 0;
  try
  {
    entries = m_pStruct->GetSubModules(&modules);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Exception in Visualisation::GetSubModules()");
    return false;
  }
  if (modules && entries > 0)
  {
    for (unsigned i=0; i < entries; i++)
    {
      if (modules[i])
      {
        m_submodules.push_back(modules[i]);
      }
    }
  }
  return (!m_submodules.empty());
}

CStdString CVisualisation::GetFriendlyName(const CStdString& strVisz,
                                           const CStdString& strSubModule)
{
  // should be of the format "moduleName (visName)"
  return CStdString(strSubModule + " (" + strVisz + ")");
}

bool CVisualisation::IsLocked()
{
  if (!m_presets.empty())
  {
    if (!m_pStruct)
      return false;

    return m_pStruct->IsLocked();
  }
  return false;
}

void CVisualisation::Destroy()
{
  // Free what was allocated in method CVisualisation::Create
  if (m_pInfo)
  {
    free((void *) m_pInfo->name);
    free((void *) m_pInfo->presets);
    free((void *) m_pInfo->profile);
    free((void *) m_pInfo->submodule);

    delete m_pInfo;
    m_pInfo = NULL;
  }

  CAddonDll<DllVisualisation, Visualisation, VIS_PROPS>::Destroy();
}

unsigned CVisualisation::GetPreset()
{
  unsigned index = 0;
  try
  {
    index = m_pStruct->GetPreset();
  }
  catch(...)
  {
    return 0;
  }
  return index;
}

CStdString CVisualisation::GetPresetName()
{
  if (!m_presets.empty())
    return m_presets[GetPreset()];
  else
    return "";
}

