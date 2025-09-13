/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "VideoPlayer.h"

#include <memory>

class CMediaPipelineWebOS;

class CVideoPlayerWebOS final : public CVideoPlayer
{
public:
  explicit CVideoPlayerWebOS(IPlayerCallback& callback);
  ~CVideoPlayerWebOS() override;
  void GetVideoResolution(unsigned int& width, unsigned int& height) override;

protected:
  void UpdateContent() override;

  void CreatePlayers() override;

private:
  std::unique_ptr<CMediaPipelineWebOS> m_mediaPipelineWebOS;
};
