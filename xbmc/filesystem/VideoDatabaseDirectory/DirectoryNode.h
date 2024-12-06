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

    enum class NodeType
    {
      NONE = 0,
      MOVIES_OVERVIEW,
      TVSHOWS_OVERVIEW,
      GENRE,
      ACTOR,
      ROOT,
      OVERVIEW,
      TITLE_MOVIES,
      YEAR,
      DIRECTOR,
      TITLE_TVSHOWS,
      SEASONS,
      EPISODES,
      RECENTLY_ADDED_MOVIES,
      RECENTLY_ADDED_EPISODES,
      STUDIO,
      MUSICVIDEOS_OVERVIEW,
      RECENTLY_ADDED_MUSICVIDEOS,
      TITLE_MUSICVIDEOS,
      MUSICVIDEOS_ALBUM,
      SETS,
      COUNTRY,
      TAGS,
      INPROGRESS_TVSHOWS,
      VIDEOVERSIONS
    };

    typedef struct {
      NodeType node;
      std::string id;
      int         label;
    } Node;

    class CDirectoryNode
    {
    public:
      static CDirectoryNode* ParseURL(const std::string& strPath);
      static void GetDatabaseInfo(const std::string& strPath, CQueryParams& params);
      virtual ~CDirectoryNode();

      NodeType GetType() const;

      bool GetChilds(CFileItemList& items);
      virtual NodeType GetChildType() const;
      virtual std::string GetLocalizedName() const;
      void CollectQueryParams(CQueryParams& params) const;

      CDirectoryNode* GetParent() const;

      std::string BuildPath() const;

      virtual bool CanCache() const;

    protected:
      CDirectoryNode(NodeType Type, const std::string& strName, CDirectoryNode* pParent);
      static CDirectoryNode* CreateNode(NodeType Type,
                                        const std::string& strName,
                                        CDirectoryNode* pParent);

      void AddOptions(const std::string& options);

      const std::string& GetName() const;
      int GetID() const;
      void RemoveParent();

      virtual bool GetContent(CFileItemList& items) const;


    private:
      NodeType m_Type;
      std::string m_strName;
      CDirectoryNode* m_pParent;
      CUrlOptions m_options;
    };
  }
}



