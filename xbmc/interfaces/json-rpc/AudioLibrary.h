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

#include <memory>
#include <set>
#include <string>
#include <vector>

class CAlbum;
class CFileitem;
class CFileitemList;
class CMusicDatabase;
class CVariant;

namespace JSONRPC
{
  class CAudioLibrary : public CFileItemHandler
  {
  public:
    static JSONRPC_STATUS GetProperties(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS GetArtists(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS GetArtistDetails(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS GetAlbums(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS GetAlbumDetails(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS GetSongs(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS GetSongDetails(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS GetGenres(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS GetRoles(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS GetSources(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS GetAvailableArtTypes(const std::string& method, ITransportLayer* transport, IClient* client, const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS GetAvailableArt(const std::string& method, ITransportLayer* transport, IClient* client, const CVariant& parameterObject, CVariant& result);

    static JSONRPC_STATUS GetRecentlyAddedAlbums(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS GetRecentlyAddedSongs(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS GetRecentlyPlayedAlbums(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS GetRecentlyPlayedSongs(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);

    static JSONRPC_STATUS SetArtistDetails(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS SetAlbumDetails(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS SetSongDetails(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);

    static JSONRPC_STATUS Scan(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS Export(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS Clean(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);

    static bool FillFileItem(
        const std::string& strFilename,
        std::shared_ptr<CFileItem>& item,
        const CVariant& parameterObject = CVariant(CVariant::VariantTypeArray));
    static bool FillFileItemList(const CVariant &parameterObject, CFileItemList &list);

    static JSONRPC_STATUS GetAdditionalDetails(const CVariant &parameterObject, CFileItemList &items);
    static JSONRPC_STATUS GetAdditionalArtistDetails(const CVariant& parameterObject,
                                                     const CFileItemList& items,
                                                     CMusicDatabase& musicdatabase);
    static JSONRPC_STATUS GetAdditionalAlbumDetails(const CVariant& parameterObject,
                                                    const CFileItemList& items,
                                                    CMusicDatabase& musicdatabase);
    static JSONRPC_STATUS GetAdditionalSongDetails(const CVariant& parameterObject,
                                                   const CFileItemList& items,
                                                   CMusicDatabase& musicdatabase);

  private:
    static void FillAlbumItem(const CAlbum& album,
                              const std::string& path,
                              std::shared_ptr<CFileItem>& item);
    static void FillItemArtistIDs(const std::vector<int>& artistids,
                                  std::shared_ptr<CFileItem>& item);

    static bool CheckForAdditionalProperties(const CVariant &properties, const std::set<std::string> &checkProperties, std::set<std::string> &foundProperties);
  };
}
