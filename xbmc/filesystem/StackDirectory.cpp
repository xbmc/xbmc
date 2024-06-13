/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "StackDirectory.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

namespace XFILE
{
  CStackDirectory::CStackDirectory() = default;

  CStackDirectory::~CStackDirectory() = default;

  bool CStackDirectory::GetDirectory(const CURL& url, CFileItemList& items)
  {
    items.Clear();
    std::vector<std::string> files;
    const std::string pathToUrl(url.Get());
    if (!GetPaths(pathToUrl, files))
      return false;   // error in path

    for (const std::string& i : files)
    {
      CFileItemPtr item(new CFileItem(i));
      item->SetPath(i);
      item->m_bIsFolder = false;
      items.Add(item);
    }
    return true;
  }

  // Used to derive a filename/path to look for NFO files and art for stacks
  // For file stacks - movie part 1.mp4, movie part 2.mp4 -> movie.mp4
  // For folder stacks - movie part 1/..., movie part 2/... -> movie/
  std::string CStackDirectory::GetStackedTitlePath(const std::string& strPath)
  {
    // Get our REs
    VECCREGEXP folderRegExps;
    VECCREGEXP fileRegExps;
    LoadRegExps(fileRegExps, folderRegExps);

    std::string stackPath{};
    if (fileRegExps.empty() || folderRegExps.empty())
      CLog::LogF(LOGDEBUG, "No stack expressions available. Skipping stack title path creation");
    else
    {
      CStackDirectory stack;
      std::string stackTitle{};
      CFileItemList parts;
      const CURL pathToUrl(strPath);
      const std::string commonPath{URIUtils::GetParentPath(strPath)};

      stack.GetDirectory(pathToUrl, parts);

      if (parts.Size() > 1)
      {
        const bool isFolderStack = [&]
        {
          std::string path{StringUtils::ToLower(parts[0]->GetBaseMoviePath(true))};
          URIUtils::RemoveSlashAtEnd(path);
          for (auto& regExp : folderRegExps)
            if (regExp.RegFind(path) != -1)
              return true;
          return false;
        }();

        std::vector<std::tuple<std::string, std::string>> stackParts;
        for (const auto& part : parts)
        {
          if (isFolderStack)
          {
            // Folder stack
            std::string path = URIUtils::GetBasePath(part->GetPath());
            URIUtils::RemoveSlashAtEnd(path);
            std::string last = URIUtils::GetFileName(path);
            if (last == "BDMV" || last == "VIDEO_TS")
            {
              path = URIUtils::GetParentPath(path);
              URIUtils::RemoveSlashAtEnd(path);
            }
            path = URIUtils::GetFileName(path);

            // Test each item against each RegExp and save parts
            for (auto& regExp : folderRegExps)
              if (regExp.RegFind(path) != -1)
              {
                stackParts.emplace_back(regExp.GetMatch(1), "");
                break;
              }
          }
          else
          {
            // File stack
            std::string fileName{URIUtils::GetFileName(part->GetPath())};

            // Check if source path uses URL encoding
            if (URIUtils::HasEncodedFilename(CURL(commonPath)))
              fileName = CURL::Decode(fileName);

            // Test each item against each RegExp and save parts
            for (auto& regExp : fileRegExps)
              if (regExp.RegFind(fileName) != -1)
              {
                stackParts.emplace_back(regExp.GetMatch(1) /* Title */,
                                        regExp.GetMatch(3) /* Ignore */);
                break;
              }
          }
        }

        // Check all equal
        if (std::adjacent_find(stackParts.begin(), stackParts.end(), std::not_equal_to<>()) ==
            stackParts.end())
        {
          // Create stacked title
          stackTitle = std::get<0>(stackParts[0]) + std::get<1>(stackParts[0]) +
                       (isFolderStack ? "/" : URIUtils::GetExtension(parts[0]->GetPath()));

          // Check if source path uses URL encoding
          if (!isFolderStack && URIUtils::HasEncodedFilename(CURL(commonPath)))
            stackTitle = CURL::Encode(stackTitle);

          if (!commonPath.empty() && !stackTitle.empty())
            stackPath = commonPath + stackTitle;
        }
      }
    }
    return stackPath;
  }

  std::string CStackDirectory::GetFirstStackedFile(const std::string &strPath)
  {
    // the stacked files are always in volume order, so just get up to the first filename
    // occurrence of " , "
    std::string file, folder;
    size_t pos = strPath.find(" , ");
    if (pos != std::string::npos)
      URIUtils::Split(strPath.substr(0, pos), folder, file);
    else
      URIUtils::Split(strPath, folder, file); // single filed stacks - should really not happen

    // remove "stack://" from the folder
    folder = folder.substr(8);
    StringUtils::Replace(file, ",,", ",");

    return URIUtils::AddFileToFolder(folder, file);
  }

  bool CStackDirectory::GetPaths(const std::string& strPath, std::vector<std::string>& vecPaths)
  {
    // format is:
    // stack://file1 , file2 , file3 , file4
    // filenames with commas are double escaped (ie replaced with ,,), thus the " , " separator used.
    std::string path = strPath;
    // remove stack:// from the beginning
    path = path.substr(8);

    vecPaths = StringUtils::Split(path, " , ");
    if (vecPaths.empty())
      return false;

    // because " , " is used as a separator any "," in the real paths are double escaped
    for (std::string& itPath : vecPaths)
      StringUtils::Replace(itPath, ",,", ",");

    return true;
  }

  std::string CStackDirectory::ConstructStackPath(const CFileItemList &items, const std::vector<int> &stack)
  {
    // no checks on the range of stack here.
    // we replace all instances of comma's with double comma's, then separate
    // the files using " , ".
    std::string stackedPath = "stack://";
    std::string folder, file;
    URIUtils::Split(items[stack[0]]->GetPath(), folder, file);
    stackedPath += folder;
    // double escape any occurrence of commas
    StringUtils::Replace(file, ",", ",,");
    stackedPath += file;
    for (unsigned int i = 1; i < stack.size(); ++i)
    {
      stackedPath += " , ";
      file = items[stack[i]]->GetPath();

      // double escape any occurrence of commas
      StringUtils::Replace(file, ",", ",,");
      stackedPath += file;
    }
    return stackedPath;
  }

  bool CStackDirectory::ConstructStackPath(const std::vector<std::string> &paths, std::string& stackedPath)
  {
    if (paths.size() < 2)
      return false;
    stackedPath = "stack://";
    std::string folder, file;
    URIUtils::Split(paths[0], folder, file);
    stackedPath += folder;
    // double escape any occurrence of commas
    StringUtils::Replace(file, ",", ",,");
    stackedPath += file;
    for (unsigned int i = 1; i < paths.size(); ++i)
    {
      stackedPath += " , ";
      file = paths[i];

      // double escape any occurrence of commas
      StringUtils::Replace(file, ",", ",,");
      stackedPath += file;
    }
    return true;
  }

  std::string CStackDirectory::GetParentPath(const std::string& stackPath)
  {
    std::vector<std::string> paths;
    if (GetPaths(stackPath, paths))
    {
      bool first = true;
      do
      {
        for (auto& path : paths)
          if (first)
            path = URIUtils::GetBaseMoviePath(path);
          else
            path = URIUtils::GetParentPath(path);
        first = false;
      } while (std::adjacent_find(paths.begin(), paths.end(), std::not_equal_to<>()) !=
               paths.end());
    }
    return paths[0];
  }

  void CStackDirectory::LoadRegExps(VECCREGEXP& fileRegExps, VECCREGEXP& folderRegExps)
  {
    // Folder RegExps
    CRegExp folderRegExp(true, CRegExp::autoUtf8);
    const std::vector<std::string>& folderStackRegExps =
        CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_folderStackRegExps;
    for (const auto& regExp : folderStackRegExps)
    {
      if (!folderRegExp.RegComp(regExp))
        CLog::LogF(LOGERROR, "Invalid folder stack RegExp:'{}'", regExp);
      else if (folderRegExp.GetCaptureTotal() != 2)
        CLog::LogF(LOGERROR, "Invalid folder stack RE ({}). Must have 2 captures.",
                   folderRegExp.GetPattern());
      else
        folderRegExps.emplace_back(folderRegExp);
    }

    // File RegExps
    CRegExp fileRegExp(true, CRegExp::autoUtf8);
    const std::vector<std::string>& fileStackRegExps =
        CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoStackRegExps;

    for (const auto& regExp : fileStackRegExps)
    {
      if (!fileRegExp.RegComp(regExp))
        CLog::LogF(LOGERROR, "Invalid folder stack RegExp:'{}'", regExp);
      else if (fileRegExp.GetCaptureTotal() != 4)
        CLog::LogF(LOGERROR, "Invalid video stack RE ({}). Must have 4 captures.",
                   fileRegExp.GetPattern());
      else
        fileRegExps.emplace_back(fileRegExp);
    }
  }
  } // namespace XFILE