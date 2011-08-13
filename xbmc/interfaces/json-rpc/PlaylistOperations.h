#pragma once
/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "utils/StdString.h"
#include "JSONRPC.h"
#include "FileItemHandler.h"
#include "playlists/PlayList.h"

namespace JSONRPC
{
  class CPlaylistOperations : public CFileItemHandler
  {
  public:
    static JSON_STATUS Create(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSON_STATUS Destroy(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);

    static JSON_STATUS GetItems(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSON_STATUS Add(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSON_STATUS Remove(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSON_STATUS Swap(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSON_STATUS Clear(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSON_STATUS Shuffle(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSON_STATUS UnShuffle(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);

    static bool FillFileItemList(const CVariant &parameterObject, CFileItemList &list);
  private:
    static std::map<CStdString, PLAYLIST::CPlayListPtr> VirtualPlaylists;
    static CCriticalSection VirtualCriticalSection;
    static PLAYLIST::CPlayListPtr GetPlaylist(const CVariant &parameterObject);
  };
}
