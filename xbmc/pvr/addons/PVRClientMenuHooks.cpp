/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRClientMenuHooks.h"

#include "addons/kodi-dev-kit/include/kodi/c-api/addon-instance/pvr/pvr_menu_hook.h"
#include "guilib/LocalizeStrings.h"
#include "pvr/PVRContextMenus.h"
#include "utils/log.h"

#include <memory>

namespace PVR
{

CPVRClientMenuHook::CPVRClientMenuHook(const std::string& addonId, const PVR_MENUHOOK& hook)
: m_addonId(addonId),
  m_hook(new PVR_MENUHOOK(hook))
{
  if (hook.category != PVR_MENUHOOK_UNKNOWN &&
      hook.category != PVR_MENUHOOK_ALL &&
      hook.category != PVR_MENUHOOK_CHANNEL &&
      hook.category != PVR_MENUHOOK_TIMER &&
      hook.category != PVR_MENUHOOK_EPG &&
      hook.category != PVR_MENUHOOK_RECORDING &&
      hook.category != PVR_MENUHOOK_DELETED_RECORDING &&
      hook.category != PVR_MENUHOOK_SETTING)
    CLog::LogF(LOGERROR, "Unknown PVR_MENUHOOK_CAT value: {}", hook.category);
}

bool CPVRClientMenuHook::operator ==(const CPVRClientMenuHook& right) const
{
  if (this == &right)
    return true;

  return m_addonId == right.m_addonId &&
         m_hook->iHookId == right.m_hook->iHookId &&
         m_hook->iLocalizedStringId == right.m_hook->iLocalizedStringId &&
         m_hook->category == right.m_hook->category;
}

bool CPVRClientMenuHook::IsAllHook() const
{
  return m_hook->category == PVR_MENUHOOK_ALL;
}

bool CPVRClientMenuHook::IsChannelHook() const
{
  return m_hook->category == PVR_MENUHOOK_CHANNEL;
}

bool CPVRClientMenuHook::IsTimerHook() const
{
  return m_hook->category == PVR_MENUHOOK_TIMER;
}

bool CPVRClientMenuHook::IsEpgHook() const
{
  return m_hook->category == PVR_MENUHOOK_EPG;
}

bool CPVRClientMenuHook::IsRecordingHook() const
{
  return m_hook->category == PVR_MENUHOOK_RECORDING;
}

bool CPVRClientMenuHook::IsDeletedRecordingHook() const
{
  return m_hook->category == PVR_MENUHOOK_DELETED_RECORDING;
}

bool CPVRClientMenuHook::IsSettingsHook() const
{
  return m_hook->category == PVR_MENUHOOK_SETTING;
}

std::string CPVRClientMenuHook::GetAddonId() const
{
  return m_addonId;
}

unsigned int CPVRClientMenuHook::GetId() const
{
  return m_hook->iHookId;
}

unsigned int CPVRClientMenuHook::GetLabelId() const
{
  return m_hook->iLocalizedStringId;
}

std::string CPVRClientMenuHook::GetLabel() const
{
  return g_localizeStrings.GetAddonString(m_addonId, m_hook->iLocalizedStringId);
}

void CPVRClientMenuHooks::AddHook(const PVR_MENUHOOK& addonHook)
{
  if (!m_hooks)
    m_hooks = std::make_unique<std::vector<CPVRClientMenuHook>>();

  const CPVRClientMenuHook hook(m_addonId, addonHook);
  m_hooks->emplace_back(hook);
  CPVRContextMenuManager::GetInstance().AddMenuHook(hook);
}

void CPVRClientMenuHooks::Clear()
{
  if (!m_hooks)
    return;

  for (const auto& hook : *m_hooks)
    CPVRContextMenuManager::GetInstance().RemoveMenuHook(hook);

  m_hooks.reset();
}

std::vector<CPVRClientMenuHook> CPVRClientMenuHooks::GetHooks(
    const std::function<bool(const CPVRClientMenuHook& hook)>& function) const
{
  std::vector<CPVRClientMenuHook> hooks;

  if (!m_hooks)
    return hooks;

  for (const CPVRClientMenuHook& hook : *m_hooks)
  {
    if (function(hook) || hook.IsAllHook())
      hooks.emplace_back(hook);
  }
  return hooks;
}

std::vector<CPVRClientMenuHook> CPVRClientMenuHooks::GetChannelHooks() const
{
  return GetHooks([](const CPVRClientMenuHook& hook)
  {
    return hook.IsChannelHook();
  });
}

std::vector<CPVRClientMenuHook> CPVRClientMenuHooks::GetTimerHooks() const
{
  return GetHooks([](const CPVRClientMenuHook& hook)
  {
    return hook.IsTimerHook();
  });
}

std::vector<CPVRClientMenuHook> CPVRClientMenuHooks::GetEpgHooks() const
{
  return GetHooks([](const CPVRClientMenuHook& hook)
  {
    return hook.IsEpgHook();
  });
}

std::vector<CPVRClientMenuHook> CPVRClientMenuHooks::GetRecordingHooks() const
{
  return GetHooks([](const CPVRClientMenuHook& hook)
  {
    return hook.IsRecordingHook();
  });
}

std::vector<CPVRClientMenuHook> CPVRClientMenuHooks::GetDeletedRecordingHooks() const
{
  return GetHooks([](const CPVRClientMenuHook& hook)
  {
    return hook.IsDeletedRecordingHook();
  });
}

std::vector<CPVRClientMenuHook> CPVRClientMenuHooks::GetSettingsHooks() const
{
  return GetHooks([](const CPVRClientMenuHook& hook)
  {
    return hook.IsSettingsHook();
  });
}

} // namespace PVR
