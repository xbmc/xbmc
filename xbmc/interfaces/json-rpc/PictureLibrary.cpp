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

#include "PictureLibrary.h"
#include "music/MusicDatabase.h"
#include "FileItem.h"
#include "Util.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "music/tags/MusicInfoTag.h"
#include "music/Artist.h"
#include "music/Album.h"
#include "music/Song.h"
#include "music/Artist.h"
#include "ApplicationMessenger.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "settings/Settings.h"

using namespace MUSIC_INFO;
using namespace JSONRPC;
using namespace XFILE;


JSONRPC_STATUS CPictureLibrary::GetPictures(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
    CMusicDatabase musicdatabase;
    if (!musicdatabase.Open())
        return InternalError;
    
    CMusicDbUrl musicUrl;
    musicUrl.FromString("musicdb://albums/");
    int artistID = -1, genreID = -1;
    const CVariant &filter = parameterObject["filter"];
    if (filter.isMember("artistid"))
        artistID = (int)filter["artistid"].asInteger();
    else if (filter.isMember("artist"))
        musicUrl.AddOption("artist", filter["artist"].asString());
    else if (filter.isMember("genreid"))
        genreID = (int)filter["genreid"].asInteger();
    else if (filter.isMember("genre"))
        musicUrl.AddOption("genre", filter["genre"].asString());
    else if (filter.isObject())
    {
        CStdString xsp;
        if (!GetXspFiltering("albums", filter, xsp))
            return InvalidParams;
        
        musicUrl.AddOption("xsp", xsp);
    }
    
    SortDescription sorting;
    ParseLimits(parameterObject, sorting.limitStart, sorting.limitEnd);
    if (!ParseSorting(parameterObject, sorting.sortBy, sorting.sortOrder, sorting.sortAttributes))
        return InvalidParams;
    
    CFileItemList items;
    if (!musicdatabase.GetAlbumsNav(musicUrl.ToString(), items, genreID, artistID, CDatabase::Filter(), sorting))
        return InternalError;
    
    JSONRPC_STATUS ret = GetAdditionalAlbumDetails(parameterObject, items, musicdatabase);
    if (ret != OK)
        return ret;
    
    int size = items.Size();
    if (items.HasProperty("total") && items.GetProperty("total").asInteger() > size)
        size = (int)items.GetProperty("total").asInteger();
    HandleFileItemList("pictureid", false, "pictures", items, parameterObject, result, size, false);
    
    return OK;
}

JSONRPC_STATUS CPictureLibrary::GetPictureDetails(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
    int albumID = (int)parameterObject["albumid"].asInteger();
    
    CMusicDatabase musicdatabase;
    if (!musicdatabase.Open())
        return InternalError;
    
    CAlbum album;
    if (!musicdatabase.GetAlbumInfo(albumID, album, NULL))
        return InvalidParams;
    
    CStdString path;
    if (!musicdatabase.GetAlbumPath(albumID, path))
        return InternalError;
    
//    CFileItemPtr albumItem;
//    FillAlbumItem(album, path, albumItem);
    
//    CFileItemList items;
//    items.Add(albumItem);
//    JSONRPC_STATUS ret = GetAdditionalAlbumDetails(parameterObject, items, musicdatabase);
 //   if (ret != OK)
 //       return ret;
    
//    HandleFileItem("albumid", false, "albumdetails", items[0], parameterObject, parameterObject["properties"], result, false);
    
    return OK;
}

JSONRPC_STATUS CPictureLibrary::GetAdditionalAlbumDetails(const CVariant &parameterObject, CFileItemList &items, CMusicDatabase &musicdatabase)
{
    if (!musicdatabase.Open())
        return InternalError;
    
    std::set<std::string> checkProperties;
    checkProperties.insert("genreid");
    checkProperties.insert("artistid");
    std::set<std::string> additionalProperties;
//    if (!CheckForAdditionalProperties(parameterObject["properties"], checkProperties, additionalProperties))
//        return OK;
    
    for (int i = 0; i < items.Size(); i++)
    {
        CFileItemPtr item = items[i];
        if (additionalProperties.find("genreid") != additionalProperties.end())
        {
            std::vector<int> genreids;
            if (musicdatabase.GetGenresByAlbum(item->GetMusicInfoTag()->GetDatabaseId(), genreids))
            {
                CVariant genreidObj(CVariant::VariantTypeArray);
                for (std::vector<int>::const_iterator genreid = genreids.begin(); genreid != genreids.end(); ++genreid)
                    genreidObj.push_back(*genreid);
                
                item->SetProperty("genreid", genreidObj);
            }
        }
        if (additionalProperties.find("artistid") != additionalProperties.end())
        {
            std::vector<int> artistids;
            if (musicdatabase.GetArtistsByAlbum(item->GetMusicInfoTag()->GetDatabaseId(), true, artistids))
            {
                CVariant artistidObj(CVariant::VariantTypeArray);
                for (std::vector<int>::const_iterator artistid = artistids.begin(); artistid != artistids.end(); ++artistid)
                    artistidObj.push_back(*artistid);
                
                item->SetProperty("artistid", artistidObj);
            }
        }
    }
    
    return OK;
}
