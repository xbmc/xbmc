/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
#include "Visualization.h"
#include "GUIInfoManager.h"
#include "ServiceBroker.h"
#include "guiinfo/GUIInfoLabels.h"
#include "Application.h"
#include "filesystem/SpecialProtocol.h"
#include "guilib/GraphicContext.h"
#include "guilib/WindowIDs.h"
#include "music/tags/MusicInfoTag.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "windowing/WindowingFactory.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "utils/StringUtils.h"
#include "cores/AudioEngine/Engines/ActiveAE/ActiveAE.h"

using namespace MUSIC_INFO;
using namespace ADDON;

CVisualisation::CVisualisation(ADDON::AddonDllPtr addon)
  : CAddonInstanceInfo(addon),
    m_addonInstance(nullptr),
    m_active(false)
{
  memset(&m_struct, 0, sizeof(m_struct));
}

bool CVisualisation::Create(int x, int y, int w, int h, void *device)
{
  m_active = false;
  m_name = Name();
  m_presetsPath = CSpecialProtocol::TranslatePath(Path());
  m_profilePath = CSpecialProtocol::TranslatePath(Profile());

#ifdef HAS_DX
  m_struct.props.device = g_Windowing.Get3D11Context();
#else
  m_struct.props.device = nullptr;
#endif
  m_struct.props.x = x;
  m_struct.props.y = y;
  m_struct.props.width = w;
  m_struct.props.height = h;
  m_struct.props.pixelRatio = g_graphicsContext.GetResInfo().fPixelRatio;
  m_struct.props.name = m_name.c_str();
  m_struct.props.presets = m_presetsPath.c_str();
  m_struct.props.profile = m_profilePath.c_str();
  m_struct.toKodi.kodiInstance = this;
  m_struct.toKodi.transfer_preset = transfer_preset;

  if (m_addon->CreateInstance(ADDON_INSTANCE_VISUALIZATION, ID(), &m_struct, reinterpret_cast<KODI_HANDLE*>(&m_addonInstance)) != ADDON_STATUS_OK)
    return m_active;

  // Start the visualisation
  std::string strFile = URIUtils::GetFileName(g_application.CurrentFile());
  CLog::Log(LOGDEBUG, "Visualisation::Start()");
  if (m_struct.toAddon.Start)
    m_active = m_struct.toAddon.Start(m_addonInstance, m_iChannels, m_iSamplesPerSec, m_iBitsPerSample, strFile.c_str());

  m_hasPresets = GetPresets();

  CreateBuffers();

  CServiceBroker::GetActiveAE().RegisterAudioCallback(this);

  return m_active;
}

void CVisualisation::AudioData(const float* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength)
{
  // pass audio data to visz.
  // audio data: is short audiodata [channel][iAudioDataLength] containing the raw audio data
  // iAudioDataLength = length of audiodata array
  // pFreqData = fft-ed audio data
  // iFreqDataLength = length of pFreqData
  if (m_struct.toAddon.AudioData)
  {
    m_struct.toAddon.AudioData(m_addonInstance, pAudioData, iAudioDataLength, pFreqData, iFreqDataLength);
  }
}

void CVisualisation::Render()
{
  // ask visz. to render itself
  if (m_struct.toAddon.Render)
  {
    m_struct.toAddon.Render(m_addonInstance);
  }
}

void CVisualisation::Stop()
{
  CServiceBroker::GetActiveAE().UnregisterAudioCallback(this);

  if (m_active && m_struct.toAddon.Stop)
  {
    m_struct.toAddon.Stop(m_addonInstance);
    m_active = false;
  }

  m_addon->DestroyInstance(ID());
  memset(&m_struct, 0, sizeof(m_struct));
  m_addonInstance = nullptr;
}

bool CVisualisation::OnAction(VIS_ACTION action, void *param)
{
  if (!m_active || !m_struct.toAddon.OnAction)
    return false;

  // see if vis wants to handle the input
  // returns false if vis doesnt want the input
  // returns true if vis handled the input
  if (action != VIS_ACTION_NONE && m_struct.toAddon.OnAction)
  {
    // if this is a VIS_ACTION_UPDATE_TRACK action, copy relevant
    // tags from CMusicInfoTag to VisTag
    if ( action == VIS_ACTION_UPDATE_TRACK && param )
    {
      const CMusicInfoTag* tag = (const CMusicInfoTag*)param;
      std::string artist(tag->GetArtistString());
      std::string albumArtist(tag->GetAlbumArtistString());
      std::string genre(StringUtils::Join(tag->GetGenre(), g_advancedSettings.m_musicItemSeparator));
      
      kodi::addon::VisTrack track;
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
      track.rating      = tag->GetUserrating();

      return m_struct.toAddon.OnAction(m_addonInstance, action, &track);
    }
    return m_struct.toAddon.OnAction(m_addonInstance, action, param);
  }
  return false;
}

void CVisualisation::OnInitialize(int iChannels, int iSamplesPerSec, int iBitsPerSample)
{
  CLog::Log(LOGDEBUG, "OnInitialize() started");

  m_iChannels = iChannels;
  m_iSamplesPerSec = iSamplesPerSec;
  m_iBitsPerSample = iBitsPerSample;
  UpdateTrack();

  CLog::Log(LOGDEBUG, "OnInitialize() done");
}

void CVisualisation::OnAudioData(const float* pAudioData, int iAudioDataLength)
{
  // FIXME: iAudioDataLength should never be less than 0
  if (iAudioDataLength < 0)
    return;

  // Save our audio data in the buffers
  std::unique_ptr<CAudioBuffer> pBuffer (new CAudioBuffer(iAudioDataLength));
  pBuffer->Set(pAudioData, iAudioDataLength);
  m_vecBuffers.push_back(pBuffer.release());

  if ( (int)m_vecBuffers.size() < m_iNumBuffers)
    return;

  std::unique_ptr<CAudioBuffer> ptrAudioBuffer(m_vecBuffers.front());
  m_vecBuffers.pop_front();

  // Fourier transform the data if the vis wants it...
  if (m_bWantsFreq)
  {
    const float *psAudioData = ptrAudioBuffer->Get();

    if (!m_transform)
      m_transform.reset(new RFFT(AUDIO_BUFFER_SIZE/2, false)); // half due to stereo

    m_transform->calc(psAudioData, m_fFreq);

    // Transfer data to our visualisation
    AudioData(psAudioData, iAudioDataLength, m_fFreq, AUDIO_BUFFER_SIZE/2); // half due to complex-conjugate
  }
  else
  { // Transfer data to our visualisation
    AudioData(ptrAudioBuffer->Get(), iAudioDataLength, nullptr, 0);
  }
  return;
}

void CVisualisation::CreateBuffers()
{
  ClearBuffers();

  // Get the number of buffers from the current vis
  VIS_INFO info;

  if (m_struct.toAddon.GetInfo)
    m_struct.toAddon.GetInfo(m_addonInstance, &info);

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

  while (!m_vecBuffers.empty())
  {
    CAudioBuffer* pAudioBuffer = m_vecBuffers.front();
    delete pAudioBuffer;
    m_vecBuffers.pop_front();
  }
  for (int j = 0; j < AUDIO_BUFFER_SIZE; j++)
  {
    m_fFreq[j] = 0.0f;
  }
}

bool CVisualisation::UpdateTrack()
{
  bool handled = false;

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

  return handled;
}

bool CVisualisation::GetPresetList(std::vector<std::string> &vecpresets)
{
  vecpresets = m_presets;
  return !m_presets.empty();
}

bool CVisualisation::GetPresets()
{
  m_presets.clear();

  if (m_struct.toAddon.GetPresets)
    m_struct.toAddon.GetPresets(m_addonInstance);
  // Note: m_presets becomes filled up with callback function transfer_preset

  return (!m_presets.empty());
}

void CVisualisation::transfer_preset(void* kodiInstance, const char* preset)
{
  CVisualisation *addon = static_cast<CVisualisation*>(kodiInstance);
  if (!addon)
  {
    CLog::Log(LOGERROR, "CVisualization::transfer_preset - invalid handler data");
    return;
  }

  addon->m_presets.push_back(preset);
}

bool CVisualisation::IsLocked()
{
  if (m_presets.empty() || m_struct.toAddon.IsLocked == nullptr)
    return false;

  return m_struct.toAddon.IsLocked(m_addonInstance);
}

unsigned int CVisualisation::GetPreset()
{
  if (m_struct.toAddon.GetPreset)
    return m_struct.toAddon.GetPreset(m_addonInstance);

  return 0;
}

std::string CVisualisation::GetPresetName()
{
  if (!m_presets.empty())
    return m_presets[GetPreset()];

  return "";
}
