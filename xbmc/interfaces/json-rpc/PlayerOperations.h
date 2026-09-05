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
#include "PlayerIds.h"

#include <string>

class CFileItemList;
class CVariant;

namespace PVR
{
class CPVRChannelGroup;
class CPVREpgInfoTag;
}

namespace JSONRPC
{
  static const int PlayerImplicit = (Video | Audio | Picture);

  class CPlayerOperations : CFileItemHandler
  {
  public:
    static JSONRPC_STATUS GetActivePlayers(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS GetPlayers(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS GetProperties(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS GetItem(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);

    static JSONRPC_STATUS PlayPause(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS Stop(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS GetAudioDelay(const std::string& method,
                                        ITransportLayer* transport,
                                        IClient* client,
                                        const CVariant& parameterObject,
                                        CVariant& result);
    static JSONRPC_STATUS SetAudioDelay(const std::string& method,
                                        ITransportLayer* transport,
                                        IClient* client,
                                        const CVariant& parameterObject,
                                        CVariant& result);
    static JSONRPC_STATUS SetSpeed(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS SetTempo(const std::string& method,
                                   ITransportLayer* transport,
                                   IClient* client,
                                   const CVariant& parameterObject,
                                   CVariant& result);
    static JSONRPC_STATUS Seek(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);

    static JSONRPC_STATUS Move(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS Zoom(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS SetViewMode(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS GetViewMode(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS Rotate(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);

    static JSONRPC_STATUS Open(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS GoTo(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS SetShuffle(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS SetRepeat(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS SetPartymode(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);

    static JSONRPC_STATUS SetAudioStream(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS AddSubtitle(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS SetSubtitle(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS SetVideoStream(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);

    static JSONRPC_STATUS GetChapters(const std::string& method,
                                      ITransportLayer* transport,
                                      IClient* client,
                                      const CVariant& parameterObject,
                                      CVariant& result);

  protected:
    /*!
     \brief List a directory as a slideshow would, split into its pictures and its playable media
     \param path The directory
     \param recursive Whether to descend into subdirectories
     \param pictures Receives the picture files
     \param media Receives the video and audio files, in file order
     \return false if the directory could not be listed
     */
    static bool ListSlideshowDirectory(const std::string& path,
                                       bool recursive,
                                       CFileItemList& pictures,
                                       CFileItemList& media);

  private:
    static JSONRPC_STATUS PlayFileItemList(CFileItemList& list, const CVariant& options);
    static int GetActivePlayers();
    static PlayerState GetPlayerState();
    static JSONRPC_STATUS ResolvePlayer(const CVariant& parameterObject, PlayerType& player);
    static KODI::PLAYLIST::Id GetPlaylist(PlayerType player);
    static JSONRPC_STATUS StartSlideshow(const std::string& path, bool recursive, bool random, const std::string &firstPicturePath = "");
    static void SendSlideshowAction(int actionID);
    static JSONRPC_STATUS GetPropertyValue(PlayerType player, const std::string &property, CVariant &result);
    static bool IsPVRChannel();
    static std::shared_ptr<PVR::CPVREpgInfoTag> GetCurrentEpg();
  };
}
