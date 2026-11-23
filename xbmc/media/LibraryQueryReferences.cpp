/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "LibraryQueryReferences.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "filesystem/Directory.h"
#include "media/LibraryQuery.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace KODI::MEDIA
{

namespace
{
using enum CDatabaseQueryRule::SearchOperator;

struct PlaylistFile
{
  std::string path;
  //! Empty if the file's name could not be read
  std::string name;
};

bool NamesPlaylists(const CLibraryQueryRuleGroup& group)
{
  return std::ranges::any_of(group.GetRules(), [](const auto& rule)
                             { return CLibraryQueryRule::IsPlaylistField(rule->m_field); }) ||
         std::ranges::any_of(group.GetCombinations(),
                             [](const auto& nested) { return NamesPlaylists(*nested); });
}

bool IsVirtualFolder(const CLibraryQueryRule& rule)
{
  return rule.m_field == Field::VIRTUAL_FOLDER && rule.m_operator == OPERATOR_EQUALS;
}

class CResolver
{
public:
  using Playlists = std::map<std::string, std::shared_ptr<const CLibraryQuery>, std::less<>>;

  //! Finds the playlists the rules of a group name. \p owner is the playlist the group is of.
  bool Resolve(const CLibraryQueryRuleGroup& group, const CLibraryQuery& owner)
  {
    for (const auto& nested : group.GetCombinations())
    {
      if (!Resolve(*nested, owner))
      {
        return false;
      }
    }

    for (const auto& rule : group.GetRules())
    {
      if (IsVirtualFolder(*rule))
      {
        const PlaylistFile* file{FileOf(*rule, owner)};
        if (!file)
        {
          return Refuse(*rule, owner, "is missing");
        }
        m_folders.insert_or_assign(rule->m_parameter.front(), file->path);
      }
      else if (rule->m_field == Field::PLAYLIST && !ResolvePlaylist(*rule, owner))
      {
        return false;
      }
    }
    return true;
  }

  /*!
   * \brief The folders the rules of a group list: its own, then those of each playlist it takes
   * rules from, once.
   */
  void CollectFolders(const CLibraryQueryRuleGroup& group,
                      std::set<const CLibraryQuery*>& followed,
                      std::vector<std::string>& folders) const
  {
    for (const auto& nested : group.GetCombinations())
    {
      CollectFolders(*nested, followed, folders);
    }

    for (const auto& rule : group.GetRules())
    {
      if (IsVirtualFolder(*rule))
      {
        folders.push_back(m_folders.find(rule->m_parameter.front())->second);
      }
      else if (rule->m_field == Field::PLAYLIST && rule->m_operator == OPERATOR_EQUALS)
      {
        const CLibraryQuery* playlist{m_playlists.find(rule->m_parameter.front())->second.get()};
        if (followed.insert(playlist).second)
        {
          CollectFolders(playlist->GetRuleGroup(), followed, folders);
        }
      }
    }
  }

  const Playlists& GetPlaylists() const { return m_playlists; }

private:
  bool ResolvePlaylist(const CLibraryQueryRule& rule, const CLibraryQuery& owner)
  {
    const PlaylistFile* file{FileOf(rule, owner)};
    if (!file)
    {
      return Refuse(rule, owner, "is missing");
    }
    if (m_path.contains(file->path))
    {
      return Refuse(rule, owner, "takes its rules from this one, directly or through others");
    }

    auto loaded = m_loaded.find(file->path);
    const bool isNew{loaded == m_loaded.end()};
    if (isNew)
    {
      const auto playlist{std::make_shared<CLibraryQuery>()};
      if (!playlist->Load(file->path))
      {
        return Refuse(rule, owner, "does not load");
      }
      loaded = m_loaded.emplace(file->path, playlist).first;
    }

    const CLibraryQuery& playlist{*loaded->second};
    if (!LIBRARY_QUERY::CanReference(owner.GetType(), playlist.GetType()))
    {
      return Refuse(rule, owner,
                    StringUtils::Format("lists {}, whose rules a playlist of {} cannot take",
                                        LIBRARY_QUERY::NameOf(playlist.GetType()),
                                        LIBRARY_QUERY::NameOf(owner.GetType())));
    }

    if (isNew)
    {
      m_path.insert(file->path);
      const bool resolved{Resolve(playlist.GetRuleGroup(), playlist)};
      m_path.erase(file->path);
      if (!resolved)
      {
        return false;
      }
    }

    m_playlists.insert_or_assign(rule.m_parameter.front(), loaded->second);
    return true;
  }

  /*!
   * \brief The file a rule's value names, among the playlists \p owner can name: by the name of a
   * playlist in any case, else by file name. A value names the same file throughout the query.
   */
  const PlaylistFile* FileOf(const CLibraryQueryRule& rule, const CLibraryQuery& owner)
  {
    if (rule.m_parameter.empty())
    {
      return nullptr;
    }
    const std::string& name{rule.m_parameter.front()};
    if (const auto found = m_named.find(name); found != m_named.end())
    {
      return &found->second;
    }

    std::vector<const PlaylistFile*> files;
    for (const std::string& folder : PlaylistFoldersOf(owner.GetType()))
    {
      for (const PlaylistFile& file : FilesIn(folder))
      {
        files.push_back(&file);
      }
    }

    auto it = std::ranges::find_if(
        files, [&name](const PlaylistFile* file)
        { return !file->name.empty() && StringUtils::EqualsNoCase(file->name, name); });
    if (it == files.end())
    {
      it = std::ranges::find_if(files, [&name](const PlaylistFile* file)
                                { return URIUtils::GetFileName(file->path) == name; });
    }
    return it == files.end() ? nullptr : &m_named.emplace(name, **it).first->second;
  }

  const std::vector<PlaylistFile>& FilesIn(const std::string& folder)
  {
    if (const auto scanned = m_files.find(folder); scanned != m_files.end())
    {
      return scanned->second;
    }

    std::vector<PlaylistFile> files;
    CFileItemList items;
    if (XFILE::CDirectory::GetDirectory(folder, items, ".xsp", XFILE::DIR_FLAG_DEFAULTS))
    {
      for (const auto& item : items)
      {
        CLibraryQuery playlist;
        const bool named{playlist.OpenAndReadName(item->GetURL())};
        files.push_back({item->GetPath(), named ? playlist.GetName() : std::string{}});
      }
    }
    return m_files.emplace(folder, std::move(files)).first->second;
  }

  static bool Refuse(const CLibraryQueryRule& rule,
                     const CLibraryQuery& owner,
                     std::string_view reason)
  {
    CLog::Log(LOGERROR,
              "Smart playlist \"{}\": the rule \"{} {} {}\" cannot be resolved, the playlist it "
              "names {}",
              owner.GetName(), CLibraryQueryRule::GetFieldName(rule.m_field),
              rule.m_operator == OPERATOR_DOES_NOT_EQUAL ? "isnot" : "is", rule.GetParameter(),
              reason);
    return false;
  }

  //! The playlist files of each folder scanned, by folder
  std::map<std::string, std::vector<PlaylistFile>, std::less<>> m_files;
  //! The file each rule value names
  std::map<std::string, PlaylistFile, std::less<>> m_named;
  //! The playlists loaded, by file
  std::map<std::string, std::shared_ptr<const CLibraryQuery>, std::less<>> m_loaded;
  //! The files of the playlists being resolved, from the query down to the current one
  std::set<std::string, std::less<>> m_path;
  //! The playlists rules take their rules from, by the value of the rule
  Playlists m_playlists;
  //! The files of the playlists rules list as folders, by the value of the rule
  std::map<std::string, std::string, std::less<>> m_folders;
};
} // unnamed namespace

std::vector<std::string> PlaylistFoldersOf(LIBRARY_QUERY::TYPE type)
{
  const std::string playlistsPath{CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(
      CSettings::SETTING_SYSTEM_PLAYLISTSPATH)};

  std::vector<std::string> folders;
  for (const std::string_view name : LIBRARY_QUERY::ReferenceFoldersOf(type))
  {
    std::string folder{URIUtils::AddFileToFolder(playlistsPath, std::string{name})};
    URIUtils::AddSlashAtEnd(folder);
    folders.push_back(std::move(folder));
  }
  return folders;
}

std::optional<CLibraryQueryReferences> CLibraryQueryReferences::Resolve(const CLibraryQuery& query)
{
  CLibraryQueryReferences references;
  const CLibraryQueryRuleGroup& rules{query.GetRuleGroup()};
  if (!NamesPlaylists(rules))
  {
    return references;
  }

  CResolver resolver;
  if (!resolver.Resolve(rules, query))
  {
    return std::nullopt;
  }

  references.m_playlists = resolver.GetPlaylists();
  std::set<const CLibraryQuery*> followed;
  resolver.CollectFolders(rules, followed, references.m_virtualFolders);
  return references;
}

const CLibraryQuery* CLibraryQueryReferences::Find(std::string_view name) const
{
  const auto it = m_playlists.find(name);
  return it == m_playlists.end() ? nullptr : it->second.get();
}

} // namespace KODI::MEDIA
