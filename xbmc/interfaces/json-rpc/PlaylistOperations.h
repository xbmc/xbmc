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

class CFileItemList;
class CVariant;

namespace PLAYLIST
{
using Id = int;
} // namespace PLAYLIST

namespace JSONRPC
{
  class CPlaylistOperations : public CFileItemHandler
  {
  public:
    static JSONRPC_STATUS GetPlaylists(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS GetProperties(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);

    static JSONRPC_STATUS GetItems(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS Add(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS Remove(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS Insert(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS Clear(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS Swap(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
  private:
    static PLAYLIST::Id GetPlaylist(const CVariant& playlist);
    static JSONRPC_STATUS GetPropertyValue(PLAYLIST::Id playlistId,
                                           const std::string& property,
                                           CVariant& result);
    static bool CheckMediaParameter(PLAYLIST::Id playlistId, const CVariant& itemObject);
    static bool HandleItemsParameter(PLAYLIST::Id playlistId,
                                     const CVariant& itemParam,
                                     CFileItemList& items);
  };
}
