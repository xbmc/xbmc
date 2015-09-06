#pragma once
/*
 *      Copyright (C) 2015 Team XBMC
 *      http://xbmc.org
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

#include <string>
#include <utility>
#include <vector>

#include "FileItem.h"
#include "URL.h"
#include "utils/CharsetConverter.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

namespace XFILE
{
  /**
   * \brief Method definition to convert an entry to a CFileItemPtr.
   *
   * \param entry The entry to convert to a CFileItemPtr
   * \param label The label of the entry
   * \param path The path of the entry
   * \param isFolder Whether the entry is a folder or not
   * \return The CFileItemPtr object created from the given entry and data.
   */
  template<class TEntry>
  using DirectorizeEntryToFileItemFunction = CFileItemPtr(*)(const TEntry& entry, const std::string& label, const std::string& path, bool isFolder);

  template<class TEntry>
  using DirectorizeEntry = std::pair<std::string, TEntry>;
  template<class TEntry>
  using DirectorizeEntries = std::vector<DirectorizeEntry<TEntry>>;

  /**
   * \brief Analyzes the given entry list from the given URL and turns them into files and directories on one directory hierarchy.
   *
   * \param url URL of the directory hierarchy to build
   * \param entries Entries to analyze and turn into files and directories
   * \param converter Converter function to convert an entry into a CFileItemPtr
   * \param items Resulting item list
   */
  template<class TEntry>
  static void Directorize(const CURL& url, const DirectorizeEntries<TEntry>& entries, DirectorizeEntryToFileItemFunction<TEntry> converter, CFileItemList& items)
  {
    if (url.Get().empty() || entries.empty())
      return;

    std::string options = url.GetOptions();
    std::string filePath = url.GetFileName();

    CURL baseUrl(url);
    baseUrl.SetOptions(""); // delete options to have a clean path to add stuff too
    baseUrl.SetFileName(""); // delete filename too as our names later will contain it

    std::string basePath = baseUrl.Get();
    URIUtils::AddSlashAtEnd(basePath);

    std::vector<std::string> filePathTokens;
    if (!filePath.empty())
      StringUtils::Tokenize(filePath, filePathTokens, "/");

    bool fastLookup = items.GetFastLookup();
    items.SetFastLookup(true);
    for (const auto& entry : entries)
    {
      std::string entryPath = entry.first;
      std::string entryFileName = entryPath;
      StringUtils::Replace(entryFileName, '\\', '/');

      // skip the requested entry
      if (entryFileName == filePath)
        continue;

      std::vector<std::string> pathTokens;
      StringUtils::Tokenize(entryFileName, pathTokens, "/");

      // ignore any entries in lower directory hierarchies
      if (pathTokens.size() < filePathTokens.size() + 1)
        continue;

      // ignore any entries in different directory hierarchies
      bool ignoreItem = false;
      entryFileName.clear();
      for (auto filePathToken = filePathTokens.begin(); filePathToken != filePathTokens.end(); ++filePathToken)
      {
        if (*filePathToken != pathTokens[std::distance(filePathTokens.begin(), filePathToken)])
        {
          ignoreItem = true;
          break;
        }
        entryFileName = URIUtils::AddFileToFolder(entryFileName, *filePathToken);
      }
      if (ignoreItem)
        continue;

      entryFileName = URIUtils::AddFileToFolder(entryFileName, pathTokens[filePathTokens.size()]);
      char c = entryPath[entryFileName.size()];
      if (c == '/' || c == '\\')
        URIUtils::AddSlashAtEnd(entryFileName);

      std::string itemPath = URIUtils::AddFileToFolder(basePath, entryFileName) + options;
      bool isFolder = false;
      if (URIUtils::HasSlashAtEnd(entryFileName)) // this is a directory
      {
        // check if the directory has already been added
        if (items.Contains(itemPath)) // already added
          continue;

        isFolder = true;
        URIUtils::AddSlashAtEnd(itemPath);
      }

      // determine the entry's filename
      std::string label = pathTokens[filePathTokens.size()];
      g_charsetConverter.unknownToUTF8(label);

      // convert the entry into a CFileItem
      CFileItemPtr item = converter(entry.second, label, itemPath, isFolder);
      item->SetPath(itemPath);
      item->m_bIsFolder = isFolder;
      if (isFolder)
        item->m_dwSize = 0;

      items.Add(item);
    }
    items.SetFastLookup(fastLookup);
  }
}
