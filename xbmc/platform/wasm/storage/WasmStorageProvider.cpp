/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WasmStorageProvider.h"

#include "ServiceBroker.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "utils/StringUtils.h"

#include "platform/wasm/TizenWebApis.h"

#include <array>

#include <emscripten/em_asm.h>

std::unique_ptr<IStorageProvider> IStorageProvider::CreateInstance()
{
  return std::make_unique<CWasmStorageProvider>();
}

void CWasmStorageProvider::GetLocalDrives(std::vector<CMediaSource>& localDrives)
{
  CMediaSource share;
  share.strPath = "/";
  share.strName = "Root";
  share.m_iDriveType = SourceType::LOCAL;
  localDrives.push_back(share);
}

std::vector<std::string> CWasmStorageProvider::GetDiskUsage()
{
  using namespace KODI::PLATFORM::WASM;
  const auto& strings = CServiceBroker::GetResourcesComponent().GetLocalizeStrings();
  if (CWasmWebApis::GetProvider() == WebApiProvider::TIZEN)
  {
    const auto storage = CTizenWebApis::GetStorageInfo();
    if (storage.capacity == 0)
      return {};

    // "Internal user storage: {0:.2f} GiB free / {1:.2f} GiB total"
    return {StringUtils::Format(strings.Get(39228), storage.availableCapacity / 1073741824.0,
                                storage.capacity / 1073741824.0)};
  }

  std::array<double, 2> values{};
  // Keep the last estimate while one asynchronous refresh is pending.
  // clang-format off
  const bool available = MAIN_THREAD_EM_ASM_INT(({
    try {
      if (typeof navigator === 'undefined' || !navigator.storage ||
          typeof navigator.storage.estimate !== 'function')
        return 0;
      const kodi = (Module.kodi = Module.kodi || {});
      const state = (kodi.browserStorageInfo = kodi.browserStorageInfo ||
                     {pending: false, nextUpdate: 0, data: null});
      const now = performance.now();
      if (!state.pending && now >= state.nextUpdate) {
        state.pending = true;
        state.nextUpdate = now + 10000;
        const failed = () => { state.data = null; state.pending = false; };
        try {
          navigator.storage.estimate().then(info => {
            state.data = null;
            if (info && Number.isSafeInteger(info.usage) && info.usage >= 0 &&
                Number.isSafeInteger(info.quota) && info.quota >= 0)
              state.data = {usage: info.usage, quota: info.quota};
            state.pending = false;
          }).catch(failed);
        } catch (e) { failed(); }
      }
      if (!state.data)
        return 0;
      HEAPF64[$0 >> 3] = state.data.usage;
      HEAPF64[($0 + 8) >> 3] = state.data.quota;
      return 1;
    } catch (e) {
      return 0;
    }
  }), values.data()) != 0;
  // clang-format on
  if (!available)
    return {};

  return {// "Storage (estimate)"
          strings.Get(39229),
          // "Used"
          StringUtils::Format("{}: {}", strings.Get(20162),
                              StringUtils::SizeToString(static_cast<int64_t>(values[0]))),
          // "Browser quota"
          StringUtils::Format("{}: {}", strings.Get(39230),
                              StringUtils::SizeToString(static_cast<int64_t>(values[1])))};
}
