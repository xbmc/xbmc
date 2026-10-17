/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "VideoPlayer.h"

#include <memory>

class CMediaPipelineWasm;
class IMediaBackendWasm;

/*!
 * \brief VideoPlayer that plays a file through a media backend when the backend takes its
 * streams, and through the regular stream players otherwise.
 */
class CVideoPlayerWasm final : public CVideoPlayer
{
public:
  explicit CVideoPlayerWasm(IPlayerCallback& callback);
  ~CVideoPlayerWasm() override;
  void GetVideoResolution(unsigned int& width, unsigned int& height) override;

protected:
  void UpdateContent() override;
  bool CanTempo() override;

  void CreatePlayers() override;

private:
  bool CanUseMediaPipeline(IMediaBackendWasm& backend);

  std::unique_ptr<CMediaPipelineWasm> m_mediaPipeline;
};
