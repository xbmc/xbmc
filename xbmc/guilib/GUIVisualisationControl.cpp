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

#include "GUIVisualisationControl.h"

#include "Application.h"
#include "GUIInfoManager.h"
#include "GUIUserMessages.h"
#include "GUIWindowManager.h"
#include "ServiceBroker.h"
#include "cores/AudioEngine/Engines/ActiveAE/ActiveAE.h"
#include "filesystem/SpecialProtocol.h"
#include "guiinfo/GUIInfoLabels.h"
#include "input/Key.h"
#include "music/tags/MusicInfoTag.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "utils/log.h"
#include "utils/URIUtils.h"

using namespace ADDON;

#define LABEL_ROW1 10
#define LABEL_ROW2 11
#define LABEL_ROW3 12

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

int CAudioBuffer::Size() const
{
  return m_iLen;
}

void CAudioBuffer::Set(const float* psBuffer, int iSize)
{
  if (iSize < 0)
    return;

  memcpy(m_pBuffer, psBuffer, iSize * sizeof(float));
  for (int i = iSize; i < m_iLen; ++i)
    m_pBuffer[i] = 0;
}

CGUIVisualisationControl::CGUIVisualisationControl(int parentID, int controlID, float posX, float posY, float width, float height)
  : CGUIControl(parentID, controlID, posX, posY, width, height),
    m_callStart(false),
    m_alreadyStarted(false),
    m_attemptedLoad(false),
    m_updateTrack(false),
    m_instance(nullptr)
{
  ControlType = GUICONTROL_VISUALISATION;
}

CGUIVisualisationControl::CGUIVisualisationControl(const CGUIVisualisationControl &from)
  : CGUIControl(from),
    m_callStart(false),
    m_alreadyStarted(false),
    m_attemptedLoad(false),
    m_instance(nullptr)
{
  ControlType = GUICONTROL_VISUALISATION;
}

std::string CGUIVisualisationControl::Name()
{
  if (m_instance == nullptr)
    return "";
  return m_instance->Name();
}

bool CGUIVisualisationControl::OnMessage(CGUIMessage &message)
{
  if (m_alreadyStarted)
  {
    switch (message.GetMessage())
    {
    case GUI_MSG_GET_VISUALISATION:
      message.SetPointer(this);
      return true;
    case GUI_MSG_VISUALISATION_RELOAD:
      FreeResources(true);
      return true;
    case GUI_MSG_PLAYBACK_STARTED:
      m_updateTrack = true;
      return true;
    default:
      break;
    }
  }
  return CGUIControl::OnMessage(message);
}

bool CGUIVisualisationControl::OnAction(const CAction &action)
{
  if (m_alreadyStarted)
  {
    switch (action.GetID())
    {
    case ACTION_VIS_PRESET_NEXT:
      m_instance->OnAction(VIS_ACTION_NEXT_PRESET, nullptr);
      break;
    case ACTION_VIS_PRESET_PREV:
      m_instance->OnAction(VIS_ACTION_PREV_PRESET, nullptr);
      break;
    case ACTION_VIS_PRESET_RANDOM:
      m_instance->OnAction(VIS_ACTION_RANDOM_PRESET, nullptr);
      break;
    case ACTION_VIS_RATE_PRESET_PLUS:
      m_instance->OnAction(VIS_ACTION_RATE_PRESET_PLUS, nullptr);
      break;
    case ACTION_VIS_RATE_PRESET_MINUS:
      m_instance->OnAction(VIS_ACTION_RATE_PRESET_MINUS, nullptr);
      break;
    case ACTION_VIS_PRESET_LOCK:
      m_instance->OnAction(VIS_ACTION_LOCK_PRESET, nullptr);
      break;
    default:
      break;
    }
    return true;
  }

  return CGUIControl::OnAction(action);
}

void CGUIVisualisationControl::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  if (g_application.m_pPlayer->IsPlayingAudio())
  {
    if (m_bInvalidated)
      FreeResources(true);

    if (!m_instance && !m_attemptedLoad)
    {
      InitVisualization();

      m_attemptedLoad = true;
    }
    else if (m_callStart && m_instance)
    {
      g_graphicsContext.CaptureStateBlock();
      if (m_alreadyStarted)
      {
        m_instance->Stop();
        m_alreadyStarted = false;
      }

      std::string songTitle = URIUtils::GetFileName(g_application.CurrentFile());
      const MUSIC_INFO::CMusicInfoTag* tag = g_infoManager.GetCurrentSongTag();
      if (tag && !tag->GetTitle().empty())
        songTitle = tag->GetTitle();
      m_alreadyStarted = m_instance->Start(m_channels, m_samplesPerSec, m_bitsPerSample, songTitle);
      g_graphicsContext.ApplyStateBlock();
      m_callStart = false;
      m_updateTrack = true;
    }
    else if (m_updateTrack)
    {
      /* Initial update of currently processed track */
      UpdateTrack();
      m_updateTrack = false;
    }

    MarkDirtyRegion();
  }

  CGUIControl::Process(currentTime, dirtyregions);
}

bool CGUIVisualisationControl::IsDirty()
{
  if (m_instance)
    return m_instance->IsDirty();
  return false;
}

void CGUIVisualisationControl::Render()
{
  if (m_instance && m_alreadyStarted)
  {
    /*
     * set the viewport - note: We currently don't have any control over how
     * the addon renders, so the best we can do is attempt to define
     * a viewport??
     */
    g_graphicsContext.SetViewPort(m_posX, m_posY, m_width, m_height);
    g_graphicsContext.CaptureStateBlock();
    m_instance->Render();
    g_graphicsContext.ApplyStateBlock();
    g_graphicsContext.RestoreViewPort();
  }
  
  CGUIControl::Render();
}

void CGUIVisualisationControl::UpdateVisibility(const CGUIListItem *item/* = nullptr*/)
{
  // if made invisible, start timer, only free addonptr after
  // some period, configurable by window class
  CGUIControl::UpdateVisibility(item);
  if (!IsVisible() && m_attemptedLoad)
    FreeResources();
}

bool CGUIVisualisationControl::CanFocusFromPoint(const CPoint &point) const
{ // mouse is allowed to focus this control, but it doesn't actually receive focus
  return IsVisible() && HitTest(point);
}

void CGUIVisualisationControl::FreeResources(bool immediately)
{
  DeInitVisualization();
  
  CGUIControl::FreeResources(immediately);

  CLog::Log(LOGDEBUG, "FreeVisualisation() done");
}

void CGUIVisualisationControl::OnInitialize(int channels, int samplesPerSec, int bitsPerSample)
{
  m_channels = channels;
  m_samplesPerSec = samplesPerSec;
  m_bitsPerSample = bitsPerSample;
  m_callStart = true;
}

void CGUIVisualisationControl::OnAudioData(const float* audioData, unsigned int audioDataLength)
{
  if (!m_instance || !m_alreadyStarted)
    return;

  // Save our audio data in the buffers
  std::unique_ptr<CAudioBuffer> pBuffer(new CAudioBuffer(audioDataLength));
  pBuffer->Set(audioData, audioDataLength);
  //m_vecBuffers.push_back(pBuffer.release());
  m_vecBuffers.emplace_back(std::move(pBuffer));

  if (m_vecBuffers.size() < m_numBuffers)
    return;

  std::unique_ptr<CAudioBuffer> ptrAudioBuffer = std::move(m_vecBuffers.front());
  m_vecBuffers.pop_front();

  // Fourier transform the data if the vis wants it...
  if (m_wantsFreq)
  {
    const float *psAudioData = ptrAudioBuffer->Get();

    if (!m_transform)
      m_transform.reset(new RFFT(AUDIO_BUFFER_SIZE/2, false)); // half due to stereo

    m_transform->calc(psAudioData, m_freq);

    // Transfer data to our visualisation
    m_instance->AudioData(psAudioData, ptrAudioBuffer->Size(), m_freq, AUDIO_BUFFER_SIZE/2); // half due to complex-conjugate
  }
  else
  { // Transfer data to our visualisation
    m_instance->AudioData(ptrAudioBuffer->Get(), ptrAudioBuffer->Size(), nullptr, 0);
  }
  return;
}

void CGUIVisualisationControl::UpdateTrack()
{
  if (!m_instance || !m_alreadyStarted)
    return;

  // get the current album art filename
  m_albumThumb = CSpecialProtocol::TranslatePath(g_infoManager.GetImage(MUSICPLAYER_COVER, WINDOW_INVALID));
  if (m_albumThumb == "DefaultAlbumCover.png")
    m_albumThumb = "";
  else
    CLog::Log(LOGDEBUG, "Updating visualization albumart: %s", m_albumThumb.c_str());

  m_instance->OnAction(VIS_ACTION_UPDATE_ALBUMART, (void*)(m_albumThumb.c_str()));

  const MUSIC_INFO::CMusicInfoTag* tag = g_infoManager.GetCurrentSongTag();
  if (!tag)
    return;

  std::string artist(tag->GetArtistString());
  std::string albumArtist(tag->GetAlbumArtistString());
  std::string genre(StringUtils::Join(tag->GetGenre(), g_advancedSettings.m_musicItemSeparator));

  VisTrack track = {0};
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

  m_instance->OnAction(VIS_ACTION_UPDATE_TRACK, &track);
}

bool CGUIVisualisationControl::IsLocked()
{
  if (m_instance && m_alreadyStarted)
    return m_instance->IsLocked();

  return false;
}

bool CGUIVisualisationControl::HasPresets()
{
  if (m_instance && m_alreadyStarted)
    return m_instance->HasPresets();

  return false;
}

int CGUIVisualisationControl::GetActivePreset()
{
  if (m_instance && m_alreadyStarted)
    return m_instance->GetActivePreset();

  return -1;
}

void CGUIVisualisationControl::SetPreset(int idx)
{
  if (m_instance && m_alreadyStarted)
    m_instance->OnAction(VIS_ACTION_LOAD_PRESET, static_cast<void*>(&idx));
}

std::string CGUIVisualisationControl::GetActivePresetName()
{
  if (m_instance && m_alreadyStarted)
    return m_instance->GetActivePresetName();

  return "";
}

bool CGUIVisualisationControl::GetPresetList(std::vector<std::string> &vecpresets)
{
  if (m_instance && m_alreadyStarted)
    return m_instance->GetPresetList(vecpresets);

  return false;
}

bool CGUIVisualisationControl::InitVisualization()
{
  const ADDON::BinaryAddonBasePtr addonBase = CServiceBroker::GetBinaryAddonManager().GetInstalledAddonInfo(CServiceBroker::GetSettings().GetString(CSettings::SETTING_MUSICPLAYER_VISUALISATION), ADDON::ADDON_VIZ);
  if (!addonBase)
    return false;

  CServiceBroker::GetActiveAE().RegisterAudioCallback(this);

  g_graphicsContext.CaptureStateBlock();

  float x = g_graphicsContext.ScaleFinalXCoord(GetXPosition(), GetYPosition());
  float y = g_graphicsContext.ScaleFinalYCoord(GetXPosition(), GetYPosition());
  float w = g_graphicsContext.ScaleFinalXCoord(GetXPosition() + GetWidth(), GetYPosition() + GetHeight()) - x;
  float h = g_graphicsContext.ScaleFinalYCoord(GetXPosition() + GetWidth(), GetYPosition() + GetHeight()) - y;
  if (x < 0)
    x = 0;
  if (y < 0)
    y = 0;
  if (x + w > g_graphicsContext.GetWidth())
    w = g_graphicsContext.GetWidth() - x;
  if (y + h > g_graphicsContext.GetHeight())
    h = g_graphicsContext.GetHeight() - y;

  m_instance = new ADDON::CVisualization(addonBase, x, y, w, h);
  CreateBuffers();

  m_alreadyStarted = false;
  g_graphicsContext.ApplyStateBlock();
  return true;
}

void CGUIVisualisationControl::DeInitVisualization()
{
  if (!m_attemptedLoad)
    return;

  /*
   * Prevent "UnregisterAudioCallback" in case Kodi is stopped. There becomes
   * audio engine stopped before gui!
   */
  if (!g_application.m_bStop)
    CServiceBroker::GetActiveAE().UnregisterAudioCallback(this);
  
  m_attemptedLoad = false;

  CGUIMessage msg(GUI_MSG_VISUALISATION_UNLOADING, m_controlID, 0);
  g_windowManager.SendMessage(msg);

  CLog::Log(LOGDEBUG, "FreeVisualisation() started");

  if (m_instance)
  {
    if (m_alreadyStarted)
    {
      g_graphicsContext.CaptureStateBlock();
      m_instance->Stop();
      g_graphicsContext.ApplyStateBlock();
      m_alreadyStarted = false;
    }

    delete m_instance;
    m_instance = nullptr;
  }

  ClearBuffers();
}

void CGUIVisualisationControl::CreateBuffers()
{
  ClearBuffers();

  // Get the number of buffers from the current vis
  VIS_INFO info { false, 0 };

  if (m_instance && m_alreadyStarted)
    m_instance->GetInfo(&info);

  m_numBuffers = info.iSyncDelay + 1;
  m_wantsFreq = info.bWantsFreq;
  if (m_numBuffers > MAX_AUDIO_BUFFERS)
    m_numBuffers = MAX_AUDIO_BUFFERS;
  if (m_numBuffers < 1)
    m_numBuffers = 1;
}

void CGUIVisualisationControl::ClearBuffers()
{
  m_wantsFreq = false;
  m_numBuffers = 0;
  m_vecBuffers.clear();

  for (int j = 0; j < AUDIO_BUFFER_SIZE; ++j)
  {
    m_freq[j] = 0.0f;
  }

  if (m_transform)
    m_transform.reset();
}
