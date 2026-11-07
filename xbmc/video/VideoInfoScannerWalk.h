/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "video/VideoDatabase.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

class CFileItem;
class CFileItemList;

namespace KODI::VIDEO
{
typedef struct SScanSettings
{
  SScanSettings()
  {
    parent_name = false;
    parent_name_root = false;
    noupdate = false;
    exclude = false;
    m_allExtAudio = false;
    recurse = 1;
  }
  bool parent_name; /* use the parent dirname as name of lookup */
  bool
      parent_name_root; /* use the name of directory where scan started as name for files in that dir */
  int recurse; /* recurse into sub folders (indicate levels) */
  bool noupdate; /* exclude from update library function */
  bool exclude; /* exclude this path from scraping */
  bool m_allExtAudio; /* treat all audio files in video directory as external tracks */
} SScanSettings;

/*!
 \brief What a scan stores in the path table for a folder it has visited, so that the next scan can
 tell from the folder's parent listing alone whether anything in it has changed.

 Stored as 2|cfg|mtime|digest|flags. cfg stands for the settings the folder was scanned with, mtime
 is the folder's modified time as its parent listed it (? where unknown), digest is that of the
 folder's own listing, and the flags are S (it has subfolders a scan visits) and N (it holds a
 .nomedia file).
 */
struct FolderRecord
{
  std::string cfg;
  std::optional<int64_t> mtime{};
  std::string digest{};
  bool hasSubfolders{false};
  bool noMedia{false};

  /*!
   \brief Read a stored record
   \param stored the value of the path's hash
   \return the record, or nullopt for anything else, which includes the hashes stored before records
   */
  static std::optional<FolderRecord> Parse(std::string_view stored);
  std::string Format() const;

  /*!
   \brief Whether the folder is unchanged without listing it: it has no subfolders to look in, and
   its parent lists it with the time it was recorded with
   \param cfg the settings the folder is scanned with now
   \param mtime the folder's modified time now
   \param fastHash whether the modified time may stand for the folder's content
   */
  bool UnchangedUnlisted(const std::string& cfg, std::optional<int64_t> mtime, bool fastHash) const;

  //! \brief Whether a listing with this digest is the one recorded, for the same settings
  bool UnchangedListing(const std::string& cfg, const std::string& listingDigest) const;
};

//! \brief One entry of a directory listing
struct DirectoryEntry
{
  std::string name;
  bool isFolder{false};
  bool hidden{false};
  int64_t size{0};
  std::optional<int64_t> mtime;
};

/*!
 \brief A directory as one listing of it found it: every entry, hidden ones included, and archives as
 the files they are. Whatever a scan needs to know about the directory is read from here, rather
 than by listing or probing it again.
 */
class CDirectorySnapshot
{
public:
  /*!
   \brief List a directory
   \param path the directory, with a slash at its end
   \return the snapshot, or nullptr if the directory couldn't be listed
   */
  static std::shared_ptr<const CDirectorySnapshot> Take(const std::string& path);

  const std::string& Path() const { return m_path; }
  //! \brief The entries, ordered by name
  const std::vector<DirectoryEntry>& Entries() const { return m_entries; }
  //! \brief The entry of this name, matched as the file system would, or nullptr
  const DirectoryEntry* Find(std::string_view name) const;
  bool HasNoMedia() const { return Find(".nomedia") != nullptr; }

  /*!
   \brief The disc structure file the folder holds, as UTILS::GetOpticalMediaPath() finds it: its
   own entries are looked at, and a BDMV or VIDEO_TS folder looked into only where there is one
   \return the file, or empty where the folder holds no disc
   */
  std::string GetDiscPath() const;
  const std::string& Digest() const { return m_digest; }

  /*!
   \brief The listing as CDirectory::GetDirectory() gives it for a mask: files of other types and
   hidden entries left out, and archives made folders where expandArchives.
   \param items [out] copies of the listed items, which the caller may change
   */
  void GetItems(CFileItemList& items, const std::string& mask, bool expandArchives) const;

private:
  std::string m_path;
  std::vector<DirectoryEntry> m_entries;
  std::vector<std::shared_ptr<CFileItem>> m_items;
  std::string m_digest;
};

//! \brief The modified time a listing gives an item, or nullopt where it gives none
std::optional<int64_t> ListedModifiedTime(const CFileItem& item);

/*!
 \brief The modified time a listing gives a file: its stat time, else the time the listing reports
 for it, as some listings give no stat. Not for a folder, whose time decides whether it is listed
 again, which a reported time may be too coarse for.
 */
std::optional<int64_t> ListedFileTime(const CFileItem& item);

//! \brief The modified time of a folder, read from the file system, or nullopt where it has none
std::optional<int64_t> ReadModifiedTime(const std::string& path);

/*!
 \brief Whether a folder of this name is part of the disc structure of the folder holding it, which
 a scan doesn't look inside
 */
bool IsDiscStructureFolder(std::string_view name);

/*!
 \brief The rows of the path table below the folders a scan was asked to scan, read once, and the
 scraper and settings each path is scanned with.
 */
class CScanPathTable
{
public:
  explicit CScanPathTable(CVideoDatabase& database) : m_database(database) {}

  //! \brief Read the rows of a folder and of those below it, unless already read
  void Load(const std::string& root);

  //! \brief The row of a path, with a slash at its end, or nullopt where it has none
  std::optional<CVideoDatabase::PathScanRow> Find(const std::string& path);

  //! \brief The record stored for a folder, or nullopt where it has none
  std::optional<FolderRecord> Record(const std::string& folder);

  //! \brief As CVideoDatabase::GetScraperForPath()
  ADDON::ScraperPtr Resolve(const std::string& path,
                            SScanSettings& settings,
                            bool& foundDirectly,
                            CVideoDatabase::ScraperCache* scraperCache);

  //! \brief The id of a path, or -1 where it has none
  int PathId(const std::string& path);

  //! \brief Store the record of a folder
  void Store(const std::string& folder, const FolderRecord& record);

  //! \brief Store no record for a folder, so that the next scan looks at it again
  void Clear(const std::string& folder);

private:
  bool IsLoaded(const std::string& path) const;
  //! \brief Add the folders holding a folder, down from the first that has a row
  void AddParents(const std::string& folder);

  struct Resolved
  {
    ADDON::ScraperPtr scraper;
    SScanSettings settings;
    bool foundDirectly{false};
  };

  CVideoDatabase& m_database;
  std::vector<std::string> m_roots;
  std::unordered_map<std::string, std::optional<CVideoDatabase::PathScanRow>> m_rows;
  std::unordered_map<std::string, Resolved> m_resolved;
};
} // namespace KODI::VIDEO
