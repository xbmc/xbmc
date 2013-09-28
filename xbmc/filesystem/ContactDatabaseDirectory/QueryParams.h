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

#include "DirectoryNode.h"

namespace XFILE
{
    namespace CONTACTDATABASEDIRECTORY
    {
        class CQueryParams
        {
        public:
            CQueryParams();
            long GetFaceId() { return m_idFace; }
            long GetContactAlbumId() { return m_idContactAlbum; }
            long GetLocationId() { return m_idLocation; }
            long GetContactId() { return m_idContact; }
            long GetYear() { return m_year; }
            
        protected:
            void SetQueryParam(NODE_TYPE NodeType, const CStdString& strNodeName);
            
            friend class CDirectoryNode;
        private:
            long m_idFace;
            long m_idContactAlbum;
            long m_idLocation;
            long m_idContact;
            long m_year;
        };
    }
}


