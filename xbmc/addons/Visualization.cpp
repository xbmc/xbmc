/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Visualization.h"

#include "filesystem/SpecialProtocol.h"
#include "guilib/GUIWindowManager.h"
#include "utils/log.h"

namespace ADDON
{

CVisualization::CVisualization(const AddonInfoPtr& addonInfo, float x, float y, float w, float h)
  : IAddonInstanceHandler(ADDON_INSTANCE_VISUALIZATION, addonInfo)
{
  // Setup new Visualization instance
  m_name = Name();
  m_presetsPath = CSpecialProtocol::TranslatePath(Path());
  m_profilePath = CSpecialProtocol::TranslatePath(Profile());

  m_ifc.visualization = new AddonInstance_Visualization;

  m_ifc.visualization->props = new AddonProps_Visualization();
  m_ifc.visualization->props->x = static_cast<int>(x);
  m_ifc.visualization->props->y = static_cast<int>(y);
  m_ifc.visualization->props->width = static_cast<int>(w);
  m_ifc.visualization->props->height = static_cast<int>(h);
  m_ifc.visualization->props->device = CServiceBroker::GetWinSystem()->GetHWContext();
  m_ifc.visualization->props->pixelRatio =
      CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo().fPixelRatio;
  m_ifc.visualization->props->name = m_name.c_str();
  m_ifc.visualization->props->presets = m_presetsPath.c_str();
  m_ifc.visualization->props->profile = m_profilePath.c_str();

  m_ifc.visualization->toKodi = new AddonToKodiFuncTable_Visualization();
  m_ifc.visualization->toKodi->kodiInstance = this;
  m_ifc.visualization->toKodi->transfer_preset = transfer_preset;
  m_ifc.visualization->toKodi->clear_presets = clear_presets;

  m_ifc.visualization->toAddon = new KodiToAddonFuncTable_Visualization();

  /* Open the class "kodi::addon::CInstanceVisualization" on add-on side */
  if (CreateInstance() != ADDON_STATUS_OK)
  {
    CLog::Log(LOGFATAL, "Visualization: failed to create instance for '{}' and not usable!", ID());
    return;
  }

  /* presets becomes send with "transfer_preset" during call of function below */
  if (m_ifc.visualization->toAddon->get_presets)
    m_ifc.visualization->toAddon->get_presets(m_ifc.visualization);
}

CVisualization::~CVisualization()
{
  /* Destroy the class "kodi::addon::CInstanceVisualization" on add-on side */
  DestroyInstance();

  delete m_ifc.visualization->toAddon;
  delete m_ifc.visualization->toKodi;
  delete m_ifc.visualization->props;
  delete m_ifc.visualization;
}

bool CVisualization::Start(int channels, int samplesPerSec, int bitsPerSample, const std::string& songName)
{
  if (m_ifc.visualization->toAddon->start)
    return m_ifc.visualization->toAddon->start(m_ifc.visualization, channels, samplesPerSec,
                                               bitsPerSample, songName.c_str());
  return false;
}

void CVisualization::Stop()
{
  if (m_ifc.visualization->toAddon->stop)
    m_ifc.visualization->toAddon->stop(m_ifc.visualization);
}

void CVisualization::AudioData(const float* audioData, int audioDataLength, float *freqData, int freqDataLength)
{
  if (m_ifc.visualization->toAddon->audio_data)
    m_ifc.visualization->toAddon->audio_data(m_ifc.visualization, audioData, audioDataLength,
                                             freqData, freqDataLength);
}

bool CVisualization::IsDirty()
{
  if (m_ifc.visualization->toAddon->is_dirty)
    return m_ifc.visualization->toAddon->is_dirty(m_ifc.visualization);
  return false;
}

void CVisualization::Render()
{
  if (m_ifc.visualization->toAddon->render)
    m_ifc.visualization->toAddon->render(m_ifc.visualization);
}

void CVisualization::GetInfo(VIS_INFO *info)
{
  if (m_ifc.visualization->toAddon->get_info)
    m_ifc.visualization->toAddon->get_info(m_ifc.visualization, info);
}

bool CVisualization::NextPreset()
{
  if (m_ifc.visualization->toAddon->next_preset)
    return m_ifc.visualization->toAddon->next_preset(m_ifc.visualization);
  return false;
}

bool CVisualization::PrevPreset()
{
  if (m_ifc.visualization->toAddon->prev_preset)
    return m_ifc.visualization->toAddon->prev_preset(m_ifc.visualization);
  return false;
}

bool CVisualization::LoadPreset(int select)
{
  if (m_ifc.visualization->toAddon->load_preset)
    return m_ifc.visualization->toAddon->load_preset(m_ifc.visualization, select);
  return false;
}

bool CVisualization::RandomPreset()
{
  if (m_ifc.visualization->toAddon->random_preset)
    return m_ifc.visualization->toAddon->random_preset(m_ifc.visualization);
  return false;
}

bool CVisualization::LockPreset()
{
  if (m_ifc.visualization->toAddon->lock_preset)
    return m_ifc.visualization->toAddon->lock_preset(m_ifc.visualization);
  return false;
}

bool CVisualization::RatePreset(bool plus_minus)
{
  if (m_ifc.visualization->toAddon->rate_preset)
    return m_ifc.visualization->toAddon->rate_preset(m_ifc.visualization, plus_minus);
  return false;
}

bool CVisualization::UpdateAlbumart(const char* albumart)
{
  if (m_ifc.visualization->toAddon->update_albumart)
    return m_ifc.visualization->toAddon->update_albumart(m_ifc.visualization, albumart);
  return false;
}

bool CVisualization::UpdateTrack(const VIS_TRACK* track)
{
  if (m_ifc.visualization->toAddon->update_track)
    return m_ifc.visualization->toAddon->update_track(m_ifc.visualization, track);
  return false;
}

bool CVisualization::HasPresets()
{
  return !m_presets.empty();
}

bool CVisualization::GetPresetList(std::vector<std::string> &vecpresets)
{
  vecpresets = m_presets;
  return !m_presets.empty();
}

int CVisualization::GetActivePreset()
{
  if (m_ifc.visualization->toAddon->get_active_preset)
    return m_ifc.visualization->toAddon->get_active_preset(m_ifc.visualization);
  return -1;
}

std::string CVisualization::GetActivePresetName()
{
  if (!m_presets.empty())
    return m_presets[GetActivePreset()];
  return "";
}

bool CVisualization::IsLocked()
{
  if (m_ifc.visualization->toAddon->is_locked)
    return m_ifc.visualization->toAddon->is_locked(m_ifc.visualization);
  return false;
}

void CVisualization::transfer_preset(void* kodiInstance, const char* preset)
{
  CVisualization *addon = static_cast<CVisualization*>(kodiInstance);
  if (!addon || !preset)
  {
    CLog::Log(LOGERROR, "CVisualization::{} - invalid handler data", __FUNCTION__);
    return;
  }

  addon->m_presets.emplace_back(preset);
}

void CVisualization::clear_presets(void* kodiInstance)
{
  CVisualization* addon = static_cast<CVisualization*>(kodiInstance);
  if (!addon)
  {
    CLog::Log(LOGERROR, "CVisualization::{} - invalid handler data", __FUNCTION__);
    return;
  }

  addon->m_presets.clear();
}

} /* namespace ADDON */
