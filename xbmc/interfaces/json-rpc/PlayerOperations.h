/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "FileItemHandler.h"
#include "JSONRPC.h"

#include <string>

class CVariant;

namespace PVR
{
class CPVRChannelGroup;
class CPVREpgInfoTag;
}

namespace KODI::PLAYLIST
{
enum class Id;
enum class RepeatState;
} // namespace PLAYLIST

namespace JSONRPC
{
  enum PlayerType
  {
    None = 0,
    Video = 0x1,
    Audio = 0x2,
    Picture = 0x4,
    External = 0x8,
    Remote = 0x10
  };

  static const int PlayerImplicit = (Video | Audio | Picture);

  class CPlayerOperations : CFileItemHandler
  {
  public:
    static JSONRPC_STATUS GetActivePlayers(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS GetPlayers(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS GetProperties(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS GetItem(const CVariant& parameterObject, CVariant& result);

    static JSONRPC_STATUS PlayPause(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS Stop(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS GetAudioDelay(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS SetAudioDelay(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS NotifyAudioChainReady(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS SetSpeed(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS SetTempo(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS Seek(const CVariant& parameterObject, CVariant& result);

    static JSONRPC_STATUS Move(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS Zoom(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS SetViewMode(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS GetViewMode(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS Rotate(const CVariant& parameterObject, CVariant& result);

    static JSONRPC_STATUS Open(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS GoTo(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS SetShuffle(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS SetRepeat(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS SetPartymode(const CVariant& parameterObject, CVariant& result);

    static JSONRPC_STATUS SetAudioStream(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS AddSubtitle(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS SetSubtitle(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS SetVideoStream(const CVariant& parameterObject, CVariant& result);

    static JSONRPC_STATUS GetChapters(const CVariant& parameterObject, CVariant& result);

  private:
    static int GetActivePlayers();
    static PlayerType GetPlayer(const CVariant &player);
    static KODI::PLAYLIST::Id GetPlaylist(PlayerType player);
    static JSONRPC_STATUS StartSlideshow(const std::string& path, bool recursive, bool random, const std::string &firstPicturePath = "");
    static void SendSlideshowAction(int actionID);
    static JSONRPC_STATUS GetPropertyValue(PlayerType player, const std::string &property, CVariant &result);

    static KODI::PLAYLIST::RepeatState ParseRepeatState(const CVariant& repeat);
    static bool IsPVRChannel();
    static std::shared_ptr<PVR::CPVREpgInfoTag> GetCurrentEpg();
  };
}
