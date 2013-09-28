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

#include "ContactDbUrl.h"
#include "filesystem/ContactDatabaseDirectory.h"
#include "playlists/SmartPlayList.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"

using namespace std;
using namespace XFILE;;
using namespace XFILE::CONTACTDATABASEDIRECTORY;

CContactDbUrl::CContactDbUrl()
: CDbUrl()
{ }

CContactDbUrl::~CContactDbUrl()
{ }

bool CContactDbUrl::parse()
{
    // the URL must start with contactdb://
    if (m_url.GetProtocol() != "contactdb" || m_url.GetFileName().empty())
        return false;
    
    CStdString path = m_url.Get();
    NODE_TYPE dirType = CContactDatabaseDirectory::GetDirectoryType(path);
    NODE_TYPE childType = CContactDatabaseDirectory::GetDirectoryChildType(path);
    
    switch (dirType)
    {
        case NODE_TYPE_FACE:
            m_type = "faces";
            break;
            
        case NODE_TYPE_CONTACT:
        case NODE_TYPE_CONTACT_RECENTLY_ADDED:
        m_type = "contacts";
            break;
        
        default:
            break;
    }
    
    switch (childType)
    {
        case NODE_TYPE_FACE:
            m_type = "faces";
            break;
            
        case NODE_TYPE_CONTACT:
        case NODE_TYPE_CONTACT_RECENTLY_ADDED:
            m_type = "contacts";
            break;
            
        case NODE_TYPE_LOCATION:
            m_type = "locations";
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
    
    // add options based on the QueryParams
    if (queryParams.GetFaceId() != -1)
        AddOption("faceid", (int)queryParams.GetFaceId());
    if (queryParams.GetContactAlbumId() != -1)
        AddOption("albumid", (int)queryParams.GetContactAlbumId());
    if (queryParams.GetLocationId() != -1)
        AddOption("locationid", (int)queryParams.GetLocationId());
    if (queryParams.GetContactId() != -1)
        AddOption("contactid", (int)queryParams.GetContactId());
    if (queryParams.GetYear() != -1)
        AddOption("year", (int)queryParams.GetYear());
    
    return true;
}

bool CContactDbUrl::validateOption(const std::string &key, const CVariant &value)
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
