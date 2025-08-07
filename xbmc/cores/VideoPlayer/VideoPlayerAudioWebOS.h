/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "MediaPipelineWebOS.h"
#include "VideoPlayerAudio.h"

/**
 * @class CVideoPlayerAudioWebOS
 * @brief Audio stream player adapter forwarding to CMediaPipelineWebOS.
 */
class CVideoPlayerAudioWebOS final : public IDVDStreamPlayerAudio
{
public:
  CVideoPlayerAudioWebOS(CMediaPipelineWebOS& mediaPipeline, CProcessInfo& processInfo);
  void FlushMessages() override;
  bool OpenStream(CDVDStreamInfo hints) override;
  void CloseStream(const bool waitForBuffers) override;
  void SetSpeed(const int speed) override;
  void Flush(const bool sync) override;
  [[nodiscard]] bool AcceptsData() const override;
  [[nodiscard]] bool HasData() const override;
  [[nodiscard]] int GetLevel() const override;
  [[nodiscard]] bool IsInited() const override;
  void SendMessage(const std::shared_ptr<CDVDMsg> msg, const int priority) override;
  void SetDynamicRangeCompression(long drc) override;
  std::string GetPlayerInfo() override;
  int GetAudioChannels() override;
  double GetCurrentPts() override;
  [[nodiscard]] bool IsStalled() const override;
  [[nodiscard]] bool IsPassthrough() const override;
  [[nodiscard]] float GetDynamicRangeAmplification() const override;

private:
  CMediaPipelineWebOS& m_mediaPipeline;
};
