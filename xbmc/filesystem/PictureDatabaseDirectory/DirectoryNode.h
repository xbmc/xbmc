#pragma once
/*
 *      Copyright (C) 2005-2012 Team XBMC
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

#include "utils/StdString.h"
#include "utils/UrlOptions.h"

class CFileItemList;

namespace XFILE
{
  namespace PICTUREDATABASEDIRECTORY
  {
    class CQueryParams;

    typedef enum _NODE_TYPE
    {
      NODE_TYPE_NONE=0,
      NODE_TYPE_ROOT,
      NODE_TYPE_OVERVIEW,
      NODE_TYPE_FOLDER,
      NODE_TYPE_YEAR,
      NODE_TYPE_CAMERA,
      NODE_TYPE_TAGS,
      NODE_TYPE_PICTURES
    } NODE_TYPE;

    typedef struct {
      NODE_TYPE node;
      int       label;
    } Node;

    class CDirectoryNode
    {
    public:
      static CDirectoryNode* ParseURL(const CStdString& strPath);
      static void GetDatabaseInfo(const CStdString& strPath, CQueryParams& params);
      virtual ~CDirectoryNode();

      NODE_TYPE GetType() const { return m_Type; }

      bool GetChilds(CFileItemList& items);
      /**
       * Should be overloaded by a derived class. Returns the NODE_TYPE of the
       * child nodes.
       */
      virtual NODE_TYPE GetChildType() const { return NODE_TYPE_NONE; }
      virtual CStdString GetLocalizedName() const;

      CDirectoryNode* GetParent() const { return m_pParent; }
      bool CanCache() const { return false; }

    protected:
      CDirectoryNode(NODE_TYPE Type, const CStdString& strName, CDirectoryNode* pParent);
      static CDirectoryNode* CreateNode(NODE_TYPE Type, const CStdString& strName, CDirectoryNode* pParent);

      void AddOptions(const CStdString &options);
      void CollectQueryParams(CQueryParams& params) const;

      const CStdString& GetName() const { return m_strName; }
      void RemoveParent() { m_pParent = NULL; }

      /**
       * Uses m_strName to retrieve a database listing of that item.
       */
      virtual bool GetContent(CFileItemList& items) const;

      CStdString BuildPath() const;

      static const char* GetVanity(NODE_TYPE node);

    private:
      NODE_TYPE       m_Type;
      CStdString      m_strName;
      CDirectoryNode* m_pParent;
      CUrlOptions     m_options;
    };
  }
}
