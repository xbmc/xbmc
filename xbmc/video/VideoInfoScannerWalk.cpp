/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoInfoScannerWalk.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "filesystem/Directory.h"
#include "filesystem/DirectoryFactory.h"
#include "filesystem/File.h"
#include "filesystem/IDirectory.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/Digest.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <array>
#include <charconv>

using namespace XFILE;
using KODI::UTILITY::CDigest;

namespace KODI::VIDEO
{
namespace
{
constexpr std::string_view RECORD_VERSION{"2"};
constexpr char UNKNOWN_TIME{'?'};
constexpr char HAS_SUBFOLDERS{'S'};
constexpr char NO_MEDIA{'N'};
} // namespace

std::optional<FolderRecord> FolderRecord::Parse(std::string_view stored)
{
  const std::vector<std::string> fields{StringUtils::Split(stored, '|')};
  if (fields.size() != 5 || fields[0] != RECORD_VERSION)
    return std::nullopt;

  FolderRecord record{.cfg = fields[1],
                      .digest = fields[3],
                      .hasSubfolders = fields[4].find(HAS_SUBFOLDERS) != std::string::npos,
                      .noMedia = fields[4].find(NO_MEDIA) != std::string::npos};
  if (const std::string & time{fields[2]}; time != std::string_view{&UNKNOWN_TIME, 1})
  {
    int64_t mtime{0};
    const auto [end, error]{std::from_chars(time.data(), time.data() + time.size(), mtime)};
    if (error != std::errc{} || end != time.data() + time.size())
      return std::nullopt;
    record.mtime = mtime;
  }
  return record;
}

std::string FolderRecord::Format() const
{
  std::string flags;
  if (hasSubfolders)
    flags += HAS_SUBFOLDERS;
  if (noMedia)
    flags += NO_MEDIA;
  return StringUtils::Format("{}|{}|{}|{}|{}", RECORD_VERSION, cfg,
                             mtime ? std::to_string(*mtime) : std::string(1, UNKNOWN_TIME), digest,
                             flags);
}

bool FolderRecord::UnchangedUnlisted(const std::string& currentCfg,
                                     std::optional<int64_t> currentMtime,
                                     bool fastHash) const
{
  return fastHash && !hasSubfolders && cfg == currentCfg && mtime && currentMtime &&
         *mtime == *currentMtime;
}

bool FolderRecord::UnchangedListing(const std::string& currentCfg,
                                    const std::string& listingDigest) const
{
  return cfg == currentCfg && !digest.empty() && digest == listingDigest;
}

std::shared_ptr<const CDirectorySnapshot> CDirectorySnapshot::Take(const std::string& path)
{
  CFileItemList items;
  if (!CDirectory::GetDirectory(path, items, "", DIR_FLAG_GET_HIDDEN | DIR_FLAG_NO_FILE_DIRS))
    return nullptr;

  auto snapshot{std::make_shared<CDirectorySnapshot>()};
  snapshot->m_path = path;
  snapshot->m_items.reserve(items.Size());
  snapshot->m_entries.reserve(items.Size());
  for (const auto& item : items)
  {
    if (item->IsParentFolder())
      continue;
    snapshot->m_items.emplace_back(item);

    std::string name{item->GetPath()};
    URIUtils::RemoveSlashAtEnd(name);
    name = URIUtils::GetFileName(name);

    DirectoryEntry& entry{snapshot->m_entries.emplace_back()};
    entry.name = std::move(name);
    entry.isFolder = item->IsFolder();
    entry.hidden = item->GetProperty("file:hidden").asBoolean();
    entry.size = entry.isFolder ? 0 : item->GetSize();
    entry.mtime = entry.isFolder ? ListedModifiedTime(*item) : ListedFileTime(*item);
  }
  std::ranges::sort(snapshot->m_entries, {}, &DirectoryEntry::name);

  // Each field is delimited, and the name, the only free text, is preceded by its length, so that
  // no listing can be taken for another
  CDigest digest{CDigest::Type::MD5};
  for (const DirectoryEntry& entry : snapshot->m_entries)
  {
    digest.Update(StringUtils::Format(
        "{}:{}|{}|{}|{}\n", entry.name.size(), entry.name, entry.isFolder ? 1 : 0, entry.size,
        entry.mtime ? std::to_string(*entry.mtime) : std::string(1, UNKNOWN_TIME)));
  }
  snapshot->m_digest = digest.Finalize();
  return snapshot;
}

const DirectoryEntry* CDirectorySnapshot::Find(std::string_view name) const
{
  const auto entry{std::ranges::find_if(m_entries, [name](const DirectoryEntry& current)
                                        { return StringUtils::EqualsNoCase(current.name, name); })};
  return entry == m_entries.end() ? nullptr : &*entry;
}

std::string CDirectorySnapshot::GetDiscPath() const
{
  // The names, in the order UTILS::GetOpticalMediaPath() looks for them
  struct DiscFile
  {
    std::string_view folder;
    std::string_view file;
  };
  static constexpr std::array discFiles{
      DiscFile{"", "VIDEO_TS.IFO"},   DiscFile{"VIDEO_TS", "VIDEO_TS.IFO"},
#ifdef HAVE_LIBBLURAY
      DiscFile{"", "index.bdmv"},     DiscFile{"", "INDEX.BDM"},
      DiscFile{"BDMV", "index.bdmv"}, DiscFile{"BDMV", "INDEX.BDM"},
#endif
  };

  for (const DiscFile& disc : discFiles)
  {
    if (disc.folder.empty())
    {
      // Named as GetOpticalMediaPath() names it, where it exists by that name
      if (const DirectoryEntry * entry{Find(disc.file)}; entry && !entry->isFolder)
      {
        const std::string path{URIUtils::AddFileToFolder(m_path, std::string{disc.file})};
        if (entry->name == disc.file || CFile::Exists(path))
          return path;
      }
      continue;
    }

    // A file in a folder of the disc's is looked for as GetOpticalMediaPath() does
    if (const DirectoryEntry * entry{Find(disc.folder)}; entry && entry->isFolder)
    {
      const std::string path{
          URIUtils::AddFileToFolder(m_path, std::string{disc.folder}, std::string{disc.file})};
      if (CFile::Exists(path))
        return path;
    }
  }
  return {};
}

void CDirectorySnapshot::GetItems(CFileItemList& items,
                                  const std::string& mask,
                                  bool expandArchives) const
{
  items.SetPath(m_path);

  // As CDirectory::GetDirectory() filters the listing it caches
  const std::unique_ptr<IDirectory> directory{CDirectoryFactory::Create(CURL{m_path})};
  if (directory)
    directory->SetMask(mask);
  const bool allowAll{!directory || directory->AllowAll()};
  const bool showHidden{CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
      CSettings::SETTING_FILELISTS_SHOWHIDDEN)};

  for (const auto& item : m_items)
  {
    if (!item->IsFolder() && !allowAll && !directory->IsAllowed(item->GetURL()))
      continue;
    if (!showHidden && item->GetProperty("file:hidden").asBoolean())
      continue;
    items.Add(std::make_shared<CFileItem>(*item));
  }

  if (expandArchives)
    CDirectory::FilterFileDirectories(items, mask);
}

std::optional<int64_t> ListedModifiedTime(const CFileItem& item)
{
  int64_t time{item.GetProperty(DIR_PROPERTY_STAT_MTIME).asInteger(0)};
  if (time == 0)
    time = item.GetProperty(DIR_PROPERTY_STAT_CTIME).asInteger(0);
  return time == 0 ? std::nullopt : std::optional<int64_t>{time};
}

std::optional<int64_t> ListedFileTime(const CFileItem& item)
{
  if (const std::optional<int64_t> time{ListedModifiedTime(item)}; time)
    return time;
  if (!item.GetDateTime().IsValid())
    return std::nullopt;
  return item.GetDateTime().GetAsSecondsSinceEpoch();
}

std::optional<int64_t> ReadModifiedTime(const std::string& path)
{
  struct __stat64 buffer;
  if (CFile::Stat(path, &buffer) != 0)
    return std::nullopt;
  const int64_t time{buffer.st_mtime ? buffer.st_mtime : buffer.st_ctime};
  return time == 0 ? std::nullopt : std::optional<int64_t>{time};
}

bool IsDiscStructureFolder(std::string_view name)
{
  static constexpr std::array<std::string_view, 5> names{"BDMV", "VIDEO_TS", "AUDIO_TS",
                                                         "CERTIFICATE", "AACS"};
  return std::ranges::any_of(names, [name](std::string_view discName)
                             { return StringUtils::EqualsNoCase(name, discName); });
}

void CScanProgress::Start(const std::set<std::string, std::less<>>& toScan, std::string_view folder)
{
  // The folders below it follow it, as its path is the start of theirs
  size_t below{0};
  if (const auto first{toScan.find(folder)}; first != toScan.end())
  {
    for (auto path{std::next(first)}; path != toScan.end() && path->starts_with(folder); ++path)
      ++below;
  }

  m_folders = 1 + below;
  m_finished = 0;
  m_shown = 0.f;
}

float CScanProgress::Percentage(float share /* = 0.f */)
{
  share = std::clamp(share, 0.f, 1.f);
  const size_t inHand{share > 0.f ? m_finished + 1 : m_finished};
  const size_t folders{std::max({m_folders, inHand, size_t{1}})};
  const float percentage{100.f * (static_cast<float>(m_finished) + share) /
                         static_cast<float>(folders)};
  m_shown = std::max(m_shown, std::min(percentage, 100.f));
  return m_shown;
}

void CScanPathTable::Load(const std::string& root)
{
  std::string base{root};
  URIUtils::AddSlashAtEnd(base);
  if (IsLoaded(base))
    return;

  std::vector<CVideoDatabase::PathScanRow> rows;
  if (!m_database.GetPathScanRows(base, rows))
    return;

  for (auto& row : rows)
  {
    std::string path{row.path};
    m_rows.insert_or_assign(std::move(path), std::move(row));
  }
  m_roots.emplace_back(std::move(base));
}

bool CScanPathTable::IsLoaded(const std::string& path) const
{
  return std::ranges::any_of(m_roots,
                             [&path](const std::string& root) { return path.starts_with(root); });
}

std::optional<CVideoDatabase::PathScanRow> CScanPathTable::Find(const std::string& path)
{
  if (const auto row{m_rows.find(path)}; row != m_rows.end())
    return row->second;

  // A path missing from the rows read may still be stored as another case of it, which a
  // case-insensitive database finds
  return m_rows.try_emplace(path, m_database.GetPathScanRow(path)).first->second;
}

std::optional<FolderRecord> CScanPathTable::Record(const std::string& folder)
{
  const std::optional<CVideoDatabase::PathScanRow> row{Find(folder)};
  return row ? FolderRecord::Parse(row->hash) : std::nullopt;
}

ADDON::ScraperPtr CScanPathTable::Resolve(const std::string& path,
                                          SScanSettings& settings,
                                          bool& foundDirectly,
                                          CVideoDatabase::ScraperCache* scraperCache)
{
  if (const auto resolved{m_resolved.find(path)}; resolved != m_resolved.end())
  {
    settings = resolved->second.settings;
    foundDirectly = resolved->second.foundDirectly;
    return resolved->second.scraper;
  }

  Resolved resolved;
  resolved.scraper = CVideoDatabase::GetScraperForPath(
      path, [this](const std::string& folder) { return Find(folder); }, resolved.settings,
      resolved.foundDirectly, scraperCache);
  settings = resolved.settings;
  foundDirectly = resolved.foundDirectly;
  return m_resolved.try_emplace(path, std::move(resolved)).first->second.scraper;
}

int CScanPathTable::PathId(const std::string& path)
{
  std::string folder{path};
  URIUtils::AddSlashAtEnd(folder);
  const std::optional<CVideoDatabase::PathScanRow> row{Find(folder)};
  return row ? row->idPath : -1;
}

void CScanPathTable::Store(const std::string& folder, const FolderRecord& record)
{
  // A folder is recorded before the folder holding it, which is added first: a path is linked to
  // its parent only where the parent is there as it is added
  if (PathId(folder) < 0)
    AddParents(folder);

  const std::string hash{record.Format()};
  if (!m_database.SetPathHash(folder, hash))
    return;

  if (auto row{m_rows.find(folder)}; row != m_rows.end() && row->second)
    row->second->hash = hash;
  else
  {
    // A path new to the table has just been added to the database, with nothing but its hash
    CVideoDatabase::PathScanRow added;
    added.idPath = m_database.GetPathId(folder);
    added.path = folder;
    added.hash = hash;
    m_rows.insert_or_assign(folder, std::move(added));
  }
}

void CScanPathTable::AddParents(const std::string& folder)
{
  const std::string parent{URIUtils::GetParentPath(folder)};
  if (parent.empty() || parent == folder || !IsLoaded(parent) || PathId(parent) >= 0)
    return;

  AddParents(parent);
  CVideoDatabase::PathScanRow added;
  added.idPath = m_database.AddPath(parent);
  if (added.idPath < 0)
    return;
  added.path = parent;
  m_rows.insert_or_assign(parent, std::move(added));
}

void CScanPathTable::Clear(const std::string& folder)
{
  if (!m_database.SetPathHash(folder, ""))
    return;
  if (auto row{m_rows.find(folder)}; row != m_rows.end() && row->second)
    row->second->hash.clear();
}
} // namespace KODI::VIDEO
