/*
 *      Copyright (C) 2012-2013 Team XBMC
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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "PictureDbUrl.h"
#include "filesystem/PictureDatabaseDirectory.h"
#include "playlists/SmartPlayList.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"

using namespace std;
using namespace XFILE;
using namespace XFILE::PICTUREDATABASEDIRECTORY;

CPictureDbUrl::CPictureDbUrl()
: CDbUrl()
{ }

CPictureDbUrl::~CPictureDbUrl()
{ }

bool CPictureDbUrl::parse()
{
    // the URL must start with picturedb://
    if (m_url.GetProtocol() != "picturedb" || m_url.GetFileName().empty())
        return false;
    
    CStdString path = m_url.Get();
    NODE_TYPE dirType = CPictureDatabaseDirectory::GetDirectoryType(path);
    NODE_TYPE childType = CPictureDatabaseDirectory::GetDirectoryChildType(path);
    
    switch (dirType)
    {
        case NODE_TYPE_FACE:
            m_type = "faces";
            break;
            
        case NODE_TYPE_ALBUM:
        case NODE_TYPE_ALBUM_RECENTLY_ADDED:
        case NODE_TYPE_ALBUM_RECENTLY_PLAYED:
        case NODE_TYPE_ALBUM_TOP100:
        case NODE_TYPE_ALBUM_COMPILATIONS:
        case NODE_TYPE_YEAR_ALBUM:
            m_type = "albums";
            break;
            
        case NODE_TYPE_ALBUM_RECENTLY_ADDED_PICTURES:
        case NODE_TYPE_ALBUM_RECENTLY_PLAYED_PICTURES:
        case NODE_TYPE_ALBUM_TOP100_PICTURES:
        case NODE_TYPE_ALBUM_COMPILATIONS_PICTURES:
        case NODE_TYPE_PICTURE:
        case NODE_TYPE_PICTURE_TOP100:
        case NODE_TYPE_YEAR_PICTURE:
        case NODE_TYPE_SINGLES:
            m_type = "pictures";
            break;
            
        default:
            break;
    }
    
    switch (childType)
    {
        case NODE_TYPE_FACE:
            m_type = "faces";
            break;
            
        case NODE_TYPE_ALBUM:
        case NODE_TYPE_ALBUM_RECENTLY_ADDED:
        case NODE_TYPE_ALBUM_RECENTLY_PLAYED:
        case NODE_TYPE_ALBUM_TOP100:
        case NODE_TYPE_YEAR_ALBUM:
            m_type = "albums";
            break;
            
        case NODE_TYPE_PICTURE:
        case NODE_TYPE_ALBUM_RECENTLY_ADDED_PICTURES:
        case NODE_TYPE_ALBUM_RECENTLY_PLAYED_PICTURES:
        case NODE_TYPE_ALBUM_TOP100_PICTURES:
        case NODE_TYPE_ALBUM_COMPILATIONS_PICTURES:
        case NODE_TYPE_PICTURE_TOP100:
        case NODE_TYPE_YEAR_PICTURE:
        case NODE_TYPE_SINGLES:
            m_type = "pictures";
            break;
            
        case NODE_TYPE_LOCATION:
            m_type = "locations";
            break;
            
        case NODE_TYPE_YEAR:
            m_type = "years";
            break;
            
        case NODE_TYPE_ALBUM_COMPILATIONS:
            m_type = "albums";
            break;
            
        case NODE_TYPE_TOP100:
            m_type = "top100";
            break;
            
        case NODE_TYPE_ROOT:
        case NODE_TYPE_OVERVIEW:
        default:
            return false;
    }
    
    if (m_type.empty())
        return false;
    
    // parse query params
    CQueryParams queryParams;
    CDirectoryNode::GetDatabaseInfo(path, queryParams);
    
    // retrieve and parse all options
    AddOptions(m_url.GetOptions());
    
    // add options based on the node type
    if (dirType == NODE_TYPE_SINGLES || childType == NODE_TYPE_SINGLES)
        AddOption("singles", true);
    
    // add options based on the QueryParams
    if (queryParams.GetFaceId() != -1)
        AddOption("faceid", (int)queryParams.GetFaceId());
    if (queryParams.GetPictureAlbumId() != -1)
        AddOption("albumid", (int)queryParams.GetPictureAlbumId());
    if (queryParams.GetLocationId() != -1)
        AddOption("locationid", (int)queryParams.GetLocationId());
    if (queryParams.GetPictureId() != -1)
        AddOption("pictureid", (int)queryParams.GetPictureId());
    if (queryParams.GetYear() != -1)
        AddOption("year", (int)queryParams.GetYear());
    
    return true;
}

bool CPictureDbUrl::validateOption(const std::string &key, const CVariant &value)
{
    if (!CDbUrl::validateOption(key, value))
        return false;
    
    // if the value is empty it will remove the option which is ok
    // otherwise we only care about the "filter" option here
    if (value.empty() || !StringUtils::EqualsNoCase(key, "filter"))
        return true;
    
    if (!value.isString())
        return false;
    
    CSmartPlaylist xspFilter;
    if (!xspFilter.LoadFromJson(value.asString()))
        return false;
    
    // check if the filter playlist matches the item type
    return xspFilter.GetType() == m_type;
}
