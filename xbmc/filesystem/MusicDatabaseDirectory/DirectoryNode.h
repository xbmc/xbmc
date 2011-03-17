#pragma once
/*
 *      Copyright (C) 2005-2008 Team XBMC
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

class CFileItemList;

namespace XFILE
{
  namespace MUSICDATABASEDIRECTORY
  {
    class CQueryParams;

    typedef enum _NODE_TYPE
    {
      NODE_TYPE_NONE=0,
      NODE_TYPE_ROOT,
      NODE_TYPE_OVERVIEW,
      NODE_TYPE_TOP100,
      NODE_TYPE_GENRE,
      NODE_TYPE_ARTIST,
      NODE_TYPE_ALBUM,
      NODE_TYPE_ALBUM_RECENTLY_ADDED,
      NODE_TYPE_ALBUM_RECENTLY_ADDED_SONGS,
      NODE_TYPE_ALBUM_RECENTLY_PLAYED,
      NODE_TYPE_ALBUM_RECENTLY_PLAYED_SONGS,
      NODE_TYPE_ALBUM_TOP100,
      NODE_TYPE_ALBUM_TOP100_SONGS,
      NODE_TYPE_ALBUM_COMPILATIONS,
      NODE_TYPE_ALBUM_COMPILATIONS_SONGS,
      NODE_TYPE_SONG,
      NODE_TYPE_SONG_TOP100,
      NODE_TYPE_YEAR,
      NODE_TYPE_YEAR_ALBUM,
      NODE_TYPE_YEAR_SONG,
      NODE_TYPE_SINGLES
    } NODE_TYPE;

    class CDirectoryNode
    {
    public:
      static CDirectoryNode* ParseURL(const CStdString& strPath);
      static void GetDatabaseInfo(const CStdString& strPath, CQueryParams& params);
      virtual ~CDirectoryNode();

      NODE_TYPE GetType();

      bool GetChilds(CFileItemList& items);
      virtual NODE_TYPE GetChildType();

      CDirectoryNode* GetParent();
      bool CanCache();

    protected:
      CDirectoryNode(NODE_TYPE Type, const CStdString& strName, CDirectoryNode* pParent);
      static CDirectoryNode* CreateNode(NODE_TYPE Type, const CStdString& strName, CDirectoryNode* pParent);

      void CollectQueryParams(CQueryParams& params);

      const CStdString& GetName() const;
      void RemoveParent();

      virtual bool GetContent(CFileItemList& items);

      CStdString BuildPath();

    private:
      void AddQueuingFolder(CFileItemList& items);

    private:
      NODE_TYPE m_Type;
      CStdString m_strName;
      CDirectoryNode* m_pParent;
    };
  }
}


