#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <set>

#include "utils/StdString.h"
#include "JSONRPC.h"
#include "FileItemHandler.h"

class CPictureDatabase;
class CPictureAlbum;

namespace JSONRPC
{
    class CPictureLibrary : public CFileItemHandler
    {
    public:
        static JSONRPC_STATUS GetFaces(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
      
        static JSONRPC_STATUS GetFaceDetails(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
      
      static JSONRPC_STATUS AddPictureAlbum(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
      static JSONRPC_STATUS AddVideoAlbum(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
      static JSONRPC_STATUS AddPicture(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
      static JSONRPC_STATUS AddVideo(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
        static JSONRPC_STATUS GetLocations(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
        
      static JSONRPC_STATUS GetPictureAlbums(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
      static JSONRPC_STATUS GetPictureAlbumDetails(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
      static JSONRPC_STATUS GetPictures(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
      static JSONRPC_STATUS GetPictureDetails(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);

      static JSONRPC_STATUS GetRecentlyAddedPictureAlbums(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
        static JSONRPC_STATUS GetRecentlyAddedPictures(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
        static JSONRPC_STATUS GetRecentlyPlayedPictureAlbums(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
        static JSONRPC_STATUS GetRecentlyPlayedPictures(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);

      
      static JSONRPC_STATUS GetVideoAlbums(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
      static JSONRPC_STATUS GetVideoAlbumDetails(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
      static JSONRPC_STATUS GetVideos(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
      static JSONRPC_STATUS GetVideoDetails(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
      
      static JSONRPC_STATUS GetRecentlyAddedVideoAlbums(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
      static JSONRPC_STATUS GetRecentlyAddedVideos(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
      static JSONRPC_STATUS GetRecentlyPlayedVideoAlbums(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
      static JSONRPC_STATUS GetRecentlyPlayedVideos(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
      
        static JSONRPC_STATUS SetFaceDetails(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
        static JSONRPC_STATUS SetPictureAlbumDetails(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
        static JSONRPC_STATUS SetPictureDetails(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
        
        static JSONRPC_STATUS Scan(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
        static JSONRPC_STATUS Export(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
        static JSONRPC_STATUS Clean(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
        
        static bool FillFileItem(const CStdString &strFilename, CFileItemPtr &item, const CVariant &parameterObject = CVariant(CVariant::VariantTypeArray));
        static bool FillFileItemList(const CVariant &parameterObject, CFileItemList &list);
        
        static JSONRPC_STATUS GetAdditionalPictureAlbumDetails(const CVariant &parameterObject, CFileItemList &items, CPictureDatabase &musicdatabase);
        static JSONRPC_STATUS GetAdditionalPictureDetails(const CVariant &parameterObject, CFileItemList &items, CPictureDatabase &musicdatabase);
        
    private:
        static void FillPictureAlbumItem(const CPictureAlbum &album, const CStdString &path, CFileItemPtr &item);
        
        static bool CheckForAdditionalProperties(const CVariant &properties, const std::set<std::string> &checkProperties, std::set<std::string> &foundProperties);
    };
}
