/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IStreamManager.h"

#include <vector>

namespace KODI
{
namespace RETRO
{
class CRetroPlayerAudio;
class CRetroPlayerRendering;
class CRetroPlayerVideo;
class CRPProcessInfo;
class CRPRenderManager;

class CRPStreamManager : public IStreamManager
{
public:
  CRPStreamManager(CRPRenderManager& renderManager, CRPProcessInfo& processInfo);
  ~CRPStreamManager() override;

  void EnableAudio(bool bEnable);

  /*!
   * \brief Hold back the sound of frames that are rolled back
   *
   * Separate from EnableAudio(), so it can't undo muting or a speed change.
   */
  void SuppressAudio(bool bSuppress);

  /*!
   * \brief Whether the client has sent sound from a thread other than the one
   *        running its frames, which can't be held back with a frame
   */
  bool AudioSentOffFrameThread() const;

  /*!
   * \brief Whether frames the client produces reach the screen
   */
  void EnableVideo(bool bEnable);

  // Implementation of IStreamManager
  StreamPtr CreateStream(StreamType streamType) override;
  void CloseStream(StreamPtr stream) override;
  void SetVideoFps(float fps) override;
  HwProcedureAddress GetHwProcedureAddress(const char* symbol) override;
  bool HasHardwareRendering() const override;
  bool BeginClientFrame() override;
  void EndClientFrame() override;

private:
  // Construction parameters
  CRPRenderManager& m_renderManager;
  CRPProcessInfo& m_processInfo;

  // Stream parameters. A client can open a replacement before closing the
  // stream it replaces, so every open stream is kept.
  std::vector<CRetroPlayerAudio*> m_audioStreams;
  std::vector<CRetroPlayerVideo*> m_videoStreams;
  std::vector<CRetroPlayerRendering*> m_renderingStreams;

  // Applied to streams the client reopens partway through a frame too
  bool m_audioSuppressed = false;
  bool m_videoEnabled = true;
};
} // namespace RETRO
} // namespace KODI
