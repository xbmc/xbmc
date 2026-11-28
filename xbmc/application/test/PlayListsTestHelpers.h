/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "FileItem.h"
#include "GUIUserMessages.h"
#include "application/ApplicationPlayLists.h"
#include "guilib/GUIMessage.h"

#include <functional>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace KODI::APPLICATION::TEST
{

//! What the fake playback was asked to open, and the paths it is to fail.
struct CPlayerLog
{
  std::vector<std::string> opened;
  std::vector<StartsRun> startsRun;
  std::vector<std::string> players;
  std::set<std::string, std::less<>> failing;
};

//! Stands in for the player: records the files it is asked to open, and fails the paths it is told to.
class CFakePlayback : public CApplicationPlayLists::IPlayback
{
public:
  explicit CFakePlayback(std::shared_ptr<CPlayerLog> log) : m_log(std::move(log)) {}

  bool Open(const CFileItem& item,
            const CApplicationPlayLists::PlayOptions& options,
            StartsRun startsRun) override
  {
    m_log->opened.push_back(item.GetDynPath());
    m_log->startsRun.push_back(startsRun);
    m_log->players.push_back(options.player);
    return !m_log->failing.contains(item.GetPath());
  }
  bool LoadLibraryTag(CFileItem& item) const override { return false; }
  Queued QueueNext(const CFileItem& item) override { return Queued::Yes; }
  void NothingToQueue() override {}
  void Stop() override {}
  void Close() override {}
  bool IsPlaying() const override { return false; }
  bool IsPlayingVideo() const override { return false; }
  bool IsPlayingAudio() const override { return false; }
  std::string GetName() const override { return {}; }
  bool RestartsOnPrevious() const override { return false; }

private:
  const std::shared_ptr<CPlayerLog> m_log;
};

//! The playlists component over a fake playback, with its protected parts open to the tests.
class CTestPlayLists : public CApplicationPlayLists
{
public:
  CTestPlayLists() : CTestPlayLists(std::make_shared<CPlayerLog>()) {}

  using CApplicationPlayLists::EditPlayList;
  using CApplicationPlayLists::EntriesOf;
  using CApplicationPlayLists::OnNextQueued;

  //! The player reports that playback started, with the item it reports, if any.
  void Started(const std::shared_ptr<CFileItem>& item = nullptr)
  {
    CGUIMessage started(GUI_MSG_PLAYBACK_STARTED, 0, 0, 0, 0, item);
    OnMessage(started);
  }

private:
  explicit CTestPlayLists(const std::shared_ptr<CPlayerLog>& log)
    : CApplicationPlayLists(std::make_unique<CFakePlayback>(log)),
      m_log(log)
  {
  }

  const std::shared_ptr<CPlayerLog> m_log;

public:
  std::vector<std::string>& m_opened{m_log->opened};
  std::vector<StartsRun>& m_startsRun{m_log->startsRun};
  std::set<std::string, std::less<>>& m_failing{m_log->failing};
  std::vector<std::string>& m_players{m_log->players};
};

} // namespace KODI::APPLICATION::TEST
