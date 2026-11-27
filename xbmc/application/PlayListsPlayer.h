/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "application/ApplicationPlayLists.h"

#include <string>

/*!
 * \brief The application's player, as the playlists use it: it resolves what a file needs before
 * the player can take it, and opens it through the application.
 */
class CPlayListsPlayer : public CApplicationPlayLists::IPlayback
{
public:
  bool Open(const CFileItem& item,
            const CApplicationPlayLists::PlayOptions& options,
            KODI::APPLICATION::StartsRun startsRun) override;
  bool LoadLibraryTag(CFileItem& item) const override;
  Queued QueueNext(const CFileItem& item) override;
  void NothingToQueue() override;
  void Stop() override;
  void Close() override;
  bool IsPlaying() const override;
  bool IsPlayingVideo() const override;
  bool IsPlayingAudio() const override;
  std::string GetName() const override;
  bool RestartsOnPrevious() const override;
};
