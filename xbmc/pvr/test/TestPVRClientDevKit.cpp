/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "addons/kodi-dev-kit/include/kodi/addon-instance/PVR.h"

#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace
{
class CTestPVRClient : public kodi::addon::CInstancePVRClient
{
public:
  using CInstancePVRClient::CInstancePVRClient;

  PVR_ERROR GetCapabilities(kodi::addon::PVRCapabilities& capabilities) override
  {
    return PVR_ERROR_NO_ERROR;
  }
  PVR_ERROR GetBackendName(std::string& name) override { return PVR_ERROR_NO_ERROR; }
  PVR_ERROR GetBackendVersion(std::string& version) override { return PVR_ERROR_NO_ERROR; }
};

//! An add-on's PVR instance, created by a Kodi of the given PVR API version
class DevKitPVRClient
{
public:
  explicit DevKitPVRClient(const char* kodiVersion)
    : m_previous(kodi::addon::CPrivateBase::m_interface)
  {
    m_callbacks.kodiInstance = this;
    m_callbacks.PlayingChannelChange = [](void* kodiInstance, unsigned int channelUid)
    { static_cast<DevKitPVRClient*>(kodiInstance)->m_channelChanges.emplace_back(channelUid); };
    m_info.version = kodiVersion;
    m_instance.info = &m_info;
    m_instance.functions = &m_functions;
    m_instance.pvr = &m_pvr;
    kodi::addon::CPrivateBase::m_interface = &m_global;
    m_addon = std::make_unique<CTestPVRClient>(kodi::addon::IInstanceInfo(&m_instance));
  }
  ~DevKitPVRClient()
  {
    m_addon.reset();
    kodi::addon::CPrivateBase::m_interface = m_previous;
  }

  void PlayingChannelChange(unsigned int channelUid) const
  {
    m_addon->PlayingChannelChange(channelUid);
  }

  std::vector<unsigned int> m_channelChanges;

private:
  AddonGlobalInterface* m_previous;
  AddonToKodiFuncTable_PVR m_callbacks{};
  KodiToAddonFuncTable_PVR m_addonCallbacks{};
  AddonProperties_PVR m_props{};
  AddonInstance_PVR m_pvr{&m_props, &m_callbacks, &m_addonCallbacks};
  KODI_ADDON_INSTANCE_INFO m_info{};
  KODI_ADDON_INSTANCE_FUNC m_functions{};
  KODI_ADDON_INSTANCE_STRUCT m_instance{};
  AddonGlobalInterface m_global{};
  std::unique_ptr<CTestPVRClient> m_addon;
};
} // namespace

TEST(TestPVRClientDevKit, PlayingChannelChangeReachesOnlyAKodiThatHasIt)
{
  // A 9.2.0 Kodi's function table ends before the call
  {
    const DevKitPVRClient addon("9.2.0");
    addon.PlayingChannelChange(101);
    EXPECT_TRUE(addon.m_channelChanges.empty());
  }
  const DevKitPVRClient addon("9.3.0");
  addon.PlayingChannelChange(101);
  EXPECT_EQ(addon.m_channelChanges, std::vector<unsigned int>{101});
}
