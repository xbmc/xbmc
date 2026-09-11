/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "SavestateWorker.h"

#include <cstddef>
#include <cstdint>
#include <memory>

namespace KODI::RETRO
{
template<typename Snapshot, typename GameClient, typename CaptureMetadata>
std::unique_ptr<Snapshot> CaptureSavestate(CSavestateWorker<Snapshot>& worker,
                                           GameClient& gameClient,
                                           size_t memorySize,
                                           CaptureMetadata&& captureMetadata)
{
  auto snapshot = worker.Acquire();
  auto clientLock = gameClient.LockForSnapshot();
  if (!gameClient.Serialize(reinterpret_cast<uint8_t*>(snapshot->memory.get()), memorySize))
  {
    worker.Release(snapshot);
    return nullptr;
  }
  captureMetadata(*snapshot);
  return snapshot;
}
} // namespace KODI::RETRO
