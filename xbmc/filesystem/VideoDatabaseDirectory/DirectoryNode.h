/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/UrlOptions.h"

#include <string>

class CFileItemList;

namespace XFILE
{
  namespace VIDEODATABASEDIRECTORY
  {
    class CQueryParams;

    typedef enum _NODE_TYPE
    {
      NODE_TYPE_NONE = 0,
      NODE_TYPE_MOVIES_OVERVIEW,
      NODE_TYPE_TVSHOWS_OVERVIEW,
      NODE_TYPE_GENRE,
      NODE_TYPE_ACTOR,
      NODE_TYPE_ROOT,
      NODE_TYPE_OVERVIEW,
      NODE_TYPE_TITLE_MOVIES,
      NODE_TYPE_YEAR,
      NODE_TYPE_DIRECTOR,
      NODE_TYPE_TITLE_TVSHOWS,
      NODE_TYPE_SEASONS,
      NODE_TYPE_EPISODES,
      NODE_TYPE_RECENTLY_ADDED_MOVIES,
      NODE_TYPE_RECENTLY_ADDED_EPISODES,
      NODE_TYPE_STUDIO,
      NODE_TYPE_MUSICVIDEOS_OVERVIEW,
      NODE_TYPE_RECENTLY_ADDED_MUSICVIDEOS,
      NODE_TYPE_TITLE_MUSICVIDEOS,
      NODE_TYPE_MUSICVIDEOS_ALBUM,
      NODE_TYPE_SETS,
      NODE_TYPE_COUNTRY,
      NODE_TYPE_TAGS,
      NODE_TYPE_INPROGRESS_TVSHOWS,
      NODE_TYPE_VIDEOVERSIONS
    } NODE_TYPE;

    typedef struct {
      NODE_TYPE   node;
      std::string id;
      int         label;
    } Node;

    class CDirectoryNode
    {
    public:
      static CDirectoryNode* ParseURL(const std::string& strPath);
      static void GetDatabaseInfo(const std::string& strPath, CQueryParams& params);
      virtual ~CDirectoryNode();

      NODE_TYPE GetType() const;

      bool GetChilds(CFileItemList& items);
      virtual NODE_TYPE GetChildType() const;
      virtual std::string GetLocalizedName() const;
      void CollectQueryParams(CQueryParams& params) const;

      CDirectoryNode* GetParent() const;

      std::string BuildPath() const;

      virtual bool CanCache() const;
    protected:
      CDirectoryNode(NODE_TYPE Type, const std::string& strName, CDirectoryNode* pParent);
      static CDirectoryNode* CreateNode(NODE_TYPE Type, const std::string& strName, CDirectoryNode* pParent);

      void AddOptions(const std::string& options);

      const std::string& GetName() const;
      int GetID() const;
      void RemoveParent();

      virtual bool GetContent(CFileItemList& items) const;


    private:
      NODE_TYPE m_Type;
      std::string m_strName;
      CDirectoryNode* m_pParent;
      CUrlOptions m_options;
    };
  }
}



