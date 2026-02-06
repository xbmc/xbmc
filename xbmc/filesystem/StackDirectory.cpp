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

#include <algorithm>
#include <array>
#include <functional>
#include <string>
#include <vector>

namespace XFILE
{
bool CStackDirectory::GetDirectory(const CURL& url, CFileItemList& items)
{
  items.Clear();
  std::vector<std::string> files;
  const std::string pathToUrl(url.Get());
  if (!GetPaths(pathToUrl, files))
    return false; // error in path

  for (const std::string& i : files)
  {
    auto item = std::make_shared<CFileItem>(i);
    item->SetPath(i);
    item->SetFolder(false);
    items.Add(item);
  }
  return true;
}

std::string CStackDirectory::GetStackTitlePath(const std::string& strPath)
{
  CStackDirectory stack;
  CFileItemList parts;
  stack.GetDirectory(CURL(strPath), parts);
  if (parts.Size() < 2)
  {
    CLog::LogF(LOGDEBUG, "Only one path. Skipping stack title path creation");
    return {};
  }

  std::vector<CRegExp> folderRegExps{
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_folderStackRegExps};

  const bool isFolderStack{
      [&]
      {
        std::string path{StringUtils::ToLower(parts[0]->GetBaseMoviePath(true))};
        URIUtils::RemoveSlashAtEnd(path);
        for (auto& regExp : folderRegExps)
          if (regExp.RegFind(path) != -1)
            return true;
        return false;
      }()};

  const std::string commonPath{URIUtils::GetParentPath(strPath)};
  std::vector<StackPart> stackParts;
  for (const auto& part : parts)
  {
    if (isFolderStack)
    {
      // Folder stack
      std::string path{URIUtils::GetBasePath(part->GetPath())};
      URIUtils::RemoveSlashAtEnd(path);
      path = URIUtils::GetFileName(path);

      // Test each item against each RegExp and save parts
      for (auto& regExp : folderRegExps)
      {
        if (regExp.RegFind(path) != -1)
        {
          stackParts.emplace_back(StackPart{.title = regExp.GetMatch(1)});
          break;
        }
      }
    }
    else
    {
      // File stack
      std::string fileName{URIUtils::GetFileName(part->GetPath())};

      std::vector<CRegExp> fileRegExps =
          CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoStackRegExps;

      // Check if source path uses URL encoding
      if (URIUtils::HasEncodedFilename(CURL(commonPath)))
        fileName = CURL::Decode(fileName);

      // Test each item against each RegExp and save parts
      for (auto& regExp : fileRegExps)
      {
        if (regExp.RegFind(fileName) != -1)
        {
          stackParts.emplace_back(
              StackPart{.title = regExp.GetMatch(1), .volume = regExp.GetMatch(3)});
          break;
        }
      }
    }
  }

  // Check all equal
  std::string stackPath;
  std::string stackTitle;
  if (!stackParts.empty() &&
      std::ranges::adjacent_find(stackParts, std::not_equal_to{}) == stackParts.end())
  {
    // Create stacked title
    stackTitle = stackParts[0].title + stackParts[0].volume +
                 (isFolderStack ? (URIUtils::IsDOSPath(commonPath) ? "\\" : "/")
                                : URIUtils::GetExtension(parts[0]->GetPath()));

    // Check if source path uses URL encoding
    if (!isFolderStack && URIUtils::HasEncodedFilename(CURL(commonPath)))
      stackTitle = CURL::Encode(stackTitle);

    if (!commonPath.empty() && !stackTitle.empty())
      stackPath = commonPath + stackTitle;
  }

  return stackPath;
}

std::string CStackDirectory::GetFirstStackedFile(const std::string& strPath)
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

std::string CStackDirectory::ConstructStackPath(const CFileItemList& items,
                                                const std::vector<int>& stack)
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

bool CStackDirectory::ConstructStackPath(const std::vector<std::string>& paths,
                                         std::string& stackedPath,
                                         const std::string& newPath)
{
  if (paths.size() < 2)
  {
    stackedPath.clear();
    return false;
  }

  stackedPath = "stack://";
  for (auto path : paths)
  {
    // double escape any occurrence of commas
    StringUtils::Replace(path, ",", ",,");
    stackedPath += path + " , ";
  }
  if (!newPath.empty())
  {
    std::string path{newPath};
    StringUtils::Replace(path, ",", ",,");
    stackedPath += path + " , ";
  }
  stackedPath.erase(stackedPath.size() - 3); // remove last " , "

  return true;
}

std::string CStackDirectory::GetParentPath(const std::string& stackPath)
{
  return GetBasePath(stackPath);
}

std::string CStackDirectory::GetBasePath(const std::string& stackPath)
{
  static constexpr int MAX_ITERATIONS{5};
  std::vector<std::string> paths;
  if (!GetPaths(stackPath, paths))
    return {};

  // Loop until we have found a common parent path
  bool first{true};
  int i = 0; // Maximum of 5 iterations
  do
  {
    for (auto& path : paths)
    {
      URIUtils::RemoveSlashAtEnd(path);
      if (first && (URIUtils::IsBDFile(path) || URIUtils::IsDVDFile(path)))
        path = URIUtils::GetDiscBasePath(path);
      else
        path = URIUtils::GetParentPath(path);
    }
    first = false;
    ++i;
  } while (std::ranges::adjacent_find(paths, std::not_equal_to<>()) != paths.end() &&
           i <= MAX_ITERATIONS);

  if (i > MAX_ITERATIONS && std::ranges::adjacent_find(paths, std::not_equal_to<>()) != paths.end())
  {
    CLog::LogF(LOGWARNING, "Failed to find common parent path after 5 iterations. Paths: [{}]",
               StringUtils::Join(paths, ", "));
    return "/";
  }
  return !paths[0].empty() ? paths[0] : "/";
}
} // namespace XFILE
