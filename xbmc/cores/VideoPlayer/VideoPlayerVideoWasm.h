/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "MediaPipelineWasm.h"
#include "VideoPlayerVideo.h"

#include <memory>

/**
 * @class CVideoPlayerVideoWasm
 * @brief Video stream player adapter forwarding to CMediaPipelineWasm.
 */
class CVideoPlayerVideoWasm final : public IDVDStreamPlayerVideo
{
public:
  CVideoPlayerVideoWasm(CMediaPipelineWasm& mediaPipeline, CProcessInfo& processInfo);
  void FlushMessages() override;
  bool OpenStream(const CDVDStreamInfo hint) override;
  void CloseStream(const bool waitForBuffers) override;
  void Flush(const bool sync) override;
  [[nodiscard]] bool AcceptsData() const override;
  [[nodiscard]] bool HasData() const override;
  [[nodiscard]] bool IsInited() const override;
  void SendMessage(const std::shared_ptr<CDVDMsg> msg, const int priority) override;
  void EnableSubtitle(const bool enable) override;
  bool IsSubtitleEnabled() override;
  double GetSubtitleDelay() override;
  void SetSubtitleDelay(const double delay) override;
  [[nodiscard]] bool IsStalled() const override;
  double GetCurrentPts() override;
  double GetOutputDelay() override;
  std::string GetPlayerInfo() override;
  int GetVideoBitrate() override;
  void SetSpeed(const int speed) override;

private:
  CMediaPipelineWasm& m_mediaPipeline;
};
