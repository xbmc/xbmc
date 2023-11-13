/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIVisualisationControl.h"

#include "GUIComponent.h"
#include "GUIInfoManager.h"
#include "GUIUserMessages.h"
#include "GUIWindowManager.h"
#include "ServiceBroker.h"
#include "addons/AddonManager.h"
#include "addons/Visualization.h"
#include "addons/addoninfo/AddonType.h"
#include "application/Application.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "cores/AudioEngine/Interfaces/AE.h"
#include "filesystem/SpecialProtocol.h"
#include "guilib/guiinfo/GUIInfoLabels.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "music/tags/MusicInfoTag.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <memory>

namespace
{
constexpr unsigned int MAX_AUDIO_BUFFERS = 16;
} // namespace

CAudioBuffer::CAudioBuffer(int iSize)
{
  m_iLen = iSize;
  m_pBuffer = new float[iSize];
}

CAudioBuffer::~CAudioBuffer()
{
  delete[] m_pBuffer;
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

CGUIVisualisationControl::CGUIVisualisationControl(
    int parentID, int controlID, float posX, float posY, float width, float height)
  : CGUIControl(parentID, controlID, posX, posY, width, height)
{
  ControlType = GUICONTROL_VISUALISATION;
}

CGUIVisualisationControl::CGUIVisualisationControl(const CGUIVisualisationControl& from)
  : CGUIControl(from)
{
  ControlType = GUICONTROL_VISUALISATION;
}

std::string CGUIVisualisationControl::Name()
{
  if (m_instance == nullptr)
    return "";
  return m_instance->Name();
}

bool CGUIVisualisationControl::OnMessage(CGUIMessage& message)
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

bool CGUIVisualisationControl::OnAction(const CAction& action)
{
  if (m_alreadyStarted)
  {
    switch (action.GetID())
    {
      case ACTION_VIS_PRESET_NEXT:
        m_instance->NextPreset();
        break;
      case ACTION_VIS_PRESET_PREV:
        m_instance->PrevPreset();
        break;
      case ACTION_VIS_PRESET_RANDOM:
        m_instance->RandomPreset();
        break;
      case ACTION_VIS_RATE_PRESET_PLUS:
        m_instance->RatePreset(true);
        break;
      case ACTION_VIS_RATE_PRESET_MINUS:
        m_instance->RatePreset(false);
        break;
      case ACTION_VIS_PRESET_LOCK:
        m_instance->LockPreset();
        break;
      default:
        break;
    }
    return true;
  }

  return CGUIControl::OnAction(action);
}

void CGUIVisualisationControl::Process(unsigned int currentTime, CDirtyRegionList& dirtyregions)
{
  const auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();
  if (appPlayer->IsPlayingAudio())
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
      auto& context = CServiceBroker::GetWinSystem()->GetGfxContext();

      context.CaptureStateBlock();
      if (m_alreadyStarted)
      {
        m_instance->Stop();
        m_alreadyStarted = false;
      }

      std::string songTitle = URIUtils::GetFileName(g_application.CurrentFile());
      const MUSIC_INFO::CMusicInfoTag* tag =
          CServiceBroker::GetGUI()->GetInfoManager().GetCurrentSongTag();
      if (tag && !tag->GetTitle().empty())
        songTitle = tag->GetTitle();
      m_alreadyStarted = m_instance->Start(m_channels, m_samplesPerSec, m_bitsPerSample, songTitle);
      context.ApplyStateBlock();
      m_callStart = false;
      m_updateTrack = true;
    }
    else if (m_updateTrack)
    {
      /* Initial update of currently processed track */
      UpdateTrack();
      m_updateTrack = false;
    }

    if (m_instance && m_instance->IsDirty())
      MarkDirtyRegion();
  }

  CGUIControl::Process(currentTime, dirtyregions);
}

void CGUIVisualisationControl::Render()
{
  if (m_instance && m_alreadyStarted)
  {
    auto& context = CServiceBroker::GetWinSystem()->GetGfxContext();

    /*
     * set the viewport - note: We currently don't have any control over how
     * the addon renders, so the best we can do is attempt to define
     * a viewport??
     */
    context.SetViewPort(m_posX, m_posY, m_width, m_height);
    context.CaptureStateBlock();
    m_instance->Render();
    context.ApplyStateBlock();
    context.RestoreViewPort();
  }

  CGUIControl::Render();
}

void CGUIVisualisationControl::UpdateVisibility(const CGUIListItem* item /* = nullptr*/)
{
  // if made invisible, start timer, only free addonptr after
  // some period, configurable by window class
  CGUIControl::UpdateVisibility(item);
  if (!IsVisible() && m_attemptedLoad)
    FreeResources();
}

bool CGUIVisualisationControl::CanFocusFromPoint(const CPoint& point) const
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
  if (!m_instance || !m_alreadyStarted || !audioData || audioDataLength == 0)
    return;

  // Save our audio data in the buffers
  std::unique_ptr<CAudioBuffer> pBuffer(new CAudioBuffer(audioDataLength));
  pBuffer->Set(audioData, audioDataLength);
  m_vecBuffers.emplace_back(std::move(pBuffer));

  if (m_vecBuffers.size() < m_numBuffers)
    return;

  std::unique_ptr<CAudioBuffer> ptrAudioBuffer = std::move(m_vecBuffers.front());
  m_vecBuffers.pop_front();

  // Transfer data to our visualisation
  m_instance->AudioData(ptrAudioBuffer->Get(), ptrAudioBuffer->Size());
}

void CGUIVisualisationControl::UpdateTrack()
{
  if (!m_instance || !m_alreadyStarted)
    return;

  // get the current album art filename
  m_albumThumb = CSpecialProtocol::TranslatePath(
      CServiceBroker::GetGUI()->GetInfoManager().GetImage(MUSICPLAYER_COVER, WINDOW_INVALID));
  if (m_albumThumb == "DefaultAlbumCover.png")
    m_albumThumb = "";
  else
    CLog::Log(LOGDEBUG, "Updating visualization albumart: {}", m_albumThumb);

  m_instance->UpdateAlbumart(m_albumThumb.c_str());

  const MUSIC_INFO::CMusicInfoTag* tag =
      CServiceBroker::GetGUI()->GetInfoManager().GetCurrentSongTag();
  if (!tag)
    return;

  const std::string artist(tag->GetArtistString());
  const std::string albumArtist(tag->GetAlbumArtistString());
  const std::string genre(StringUtils::Join(
      tag->GetGenre(),
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator));

  KODI_ADDON_VISUALIZATION_TRACK track = {};
  track.title = tag->GetTitle().c_str();
  track.artist = artist.c_str();
  track.album = tag->GetAlbum().c_str();
  track.albumArtist = albumArtist.c_str();
  track.genre = genre.c_str();
  track.comment = tag->GetComment().c_str();
  track.lyrics = tag->GetLyrics().c_str();
  track.trackNumber = tag->GetTrackNumber();
  track.discNumber = tag->GetDiscNumber();
  track.duration = tag->GetDuration();
  track.year = tag->GetYear();
  track.rating = tag->GetUserrating();

  m_instance->UpdateTrack(&track);
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
    m_instance->LoadPreset(idx);
}

std::string CGUIVisualisationControl::GetActivePresetName()
{
  if (m_instance && m_alreadyStarted)
    return m_instance->GetActivePresetName();

  return "";
}

bool CGUIVisualisationControl::GetPresetList(std::vector<std::string>& vecpresets)
{
  if (m_instance && m_alreadyStarted)
    return m_instance->GetPresetList(vecpresets);

  return false;
}

bool CGUIVisualisationControl::InitVisualization()
{
  IAE* ae = CServiceBroker::GetActiveAE();
  CWinSystemBase* const winSystem = CServiceBroker::GetWinSystem();
  if (!ae || !winSystem)
    return false;

  const std::string addon = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(
      CSettings::SETTING_MUSICPLAYER_VISUALISATION);
  const ADDON::AddonInfoPtr addonBase =
      CServiceBroker::GetAddonMgr().GetAddonInfo(addon, ADDON::AddonType::VISUALIZATION);
  if (!addonBase)
    return false;

  ae->RegisterAudioCallback(this);

  auto& context = winSystem->GetGfxContext();

  context.CaptureStateBlock();

  float x = context.ScaleFinalXCoord(GetXPosition(), GetYPosition());
  float y = context.ScaleFinalYCoord(GetXPosition(), GetYPosition());
  float w = context.ScaleFinalXCoord(GetXPosition() + GetWidth(), GetYPosition() + GetHeight()) - x;
  float h = context.ScaleFinalYCoord(GetXPosition() + GetWidth(), GetYPosition() + GetHeight()) - y;
  if (x < 0)
    x = 0;
  if (y < 0)
    y = 0;
  if (x + w > context.GetWidth())
    w = context.GetWidth() - x;
  if (y + h > context.GetHeight())
    h = context.GetHeight() - y;

  m_instance = std::make_unique<KODI::ADDONS::CVisualization>(addonBase, x, y, w, h);
  CreateBuffers();

  m_alreadyStarted = false;
  context.ApplyStateBlock();
  return true;
}

void CGUIVisualisationControl::DeInitVisualization()
{
  if (!m_attemptedLoad)
    return;

  CWinSystemBase* const winSystem = CServiceBroker::GetWinSystem();
  if (!winSystem)
    return;

  IAE* ae = CServiceBroker::GetActiveAE();
  if (ae)
    ae->UnregisterAudioCallback(this);

  m_attemptedLoad = false;

  CGUIMessage msg(GUI_MSG_VISUALISATION_UNLOADING, m_controlID, 0);
  CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);

  CLog::Log(LOGDEBUG, "FreeVisualisation() started");

  if (m_instance)
  {
    if (m_alreadyStarted)
    {
      auto& context = winSystem->GetGfxContext();

      context.CaptureStateBlock();
      m_instance->Stop();
      context.ApplyStateBlock();
      m_alreadyStarted = false;
    }

    m_instance.reset();
  }

  ClearBuffers();
}

void CGUIVisualisationControl::CreateBuffers()
{
  ClearBuffers();

  m_numBuffers = 1;
  if (m_instance)
    m_numBuffers += m_instance->GetSyncDelay();
  if (m_numBuffers > MAX_AUDIO_BUFFERS)
    m_numBuffers = MAX_AUDIO_BUFFERS;
  if (m_numBuffers < 1)
    m_numBuffers = 1;
}

void CGUIVisualisationControl::ClearBuffers()
{
  m_numBuffers = 0;
  m_vecBuffers.clear();
}
