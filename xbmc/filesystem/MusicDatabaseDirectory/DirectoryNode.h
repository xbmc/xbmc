/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/UrlOptions.h"

class CFileItemList;

namespace XFILE
{
  namespace MUSICDATABASEDIRECTORY
  {
    class CQueryParams;

    enum class NodeType
    {
      NONE = 0,
      ROOT,
      OVERVIEW,
      TOP100,
      ROLE,
      SOURCE,
      GENRE,
      ARTIST,
      ALBUM,
      ALBUM_RECENTLY_ADDED,
      ALBUM_RECENTLY_ADDED_SONGS,
      ALBUM_RECENTLY_PLAYED,
      ALBUM_RECENTLY_PLAYED_SONGS,
      ALBUM_TOP100,
      ALBUM_TOP100_SONGS,
      SONG,
      SONG_TOP100,
      YEAR,
      SINGLES,
      DISC,
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
      static bool GetNodeInfo(const std::string& strPath,
                              NodeType& type,
                              NodeType& childtype,
                              CQueryParams& params);
      virtual ~CDirectoryNode();

      NodeType GetType() const;

      bool GetChilds(CFileItemList& items);
      virtual NodeType GetChildType() const;
      virtual std::string GetLocalizedName() const;

      CDirectoryNode* GetParent() const;
      virtual bool CanCache() const;

      std::string BuildPath() const;

    protected:
      CDirectoryNode(NodeType Type, const std::string& strName, CDirectoryNode* pParent);
      static CDirectoryNode* CreateNode(NodeType Type,
                                        const std::string& strName,
                                        CDirectoryNode* pParent);

      void AddOptions(const std::string &options);
      void CollectQueryParams(CQueryParams& params) const;

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


