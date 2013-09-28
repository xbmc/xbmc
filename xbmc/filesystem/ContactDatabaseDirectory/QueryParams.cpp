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

#include "QueryParams.h"

using namespace XFILE::CONTACTDATABASEDIRECTORY;

CQueryParams::CQueryParams()
{
    m_idFace=-1;
    m_idContactAlbum=-1;
    m_idLocation=-1;
    m_idContact=-1;
    m_year=-1;
}

void CQueryParams::SetQueryParam(NODE_TYPE NodeType, const CStdString& strNodeName)
{
    long idDb=atol(strNodeName.c_str());
    
    switch (NodeType)
    {
        case NODE_TYPE_LOCATION:
            m_idLocation=idDb;
            break;
            break;
        case NODE_TYPE_FACE:
            m_idFace=idDb;
            break;
        case NODE_TYPE_CONTACT_RECENTLY_ADDED:
            m_idContactAlbum=idDb;
            break;
        case NODE_TYPE_CONTACT_RECENTLY_ADDED_CONTACTS:
        case NODE_TYPE_CONTACT:
            m_idContact=idDb;
        default:
            break;
    }
}
