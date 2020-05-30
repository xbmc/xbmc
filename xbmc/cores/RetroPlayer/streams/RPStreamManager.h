/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IStreamManager.h"

namespace KODI
{
namespace RETRO
{
class CRetroPlayerAudio;
class CRPProcessInfo;
class CRPRenderManager;

class CRPStreamManager : public IStreamManager
{
public:
  CRPStreamManager(CRPRenderManager& renderManager, CRPProcessInfo& processInfo);
  ~CRPStreamManager() override = default;

  void EnableAudio(bool bEnable);

  // Implementation of IStreamManager
  StreamPtr CreateStream(StreamType streamType) override;
  void CloseStream(StreamPtr stream) override;

private:
  // Construction parameters
  CRPRenderManager& m_renderManager;
  CRPProcessInfo& m_processInfo;

  // Stream parameters
  CRetroPlayerAudio* m_audioStream = nullptr;
};
} // namespace RETRO
} // namespace KODI
