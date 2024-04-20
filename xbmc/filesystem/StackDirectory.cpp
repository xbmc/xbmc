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

#include <stdlib.h>

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

  std::string CStackDirectory::GetStackedTitlePath(const std::string &strPath)
  {
    // Load up our REs
    VECCREGEXP  RegExps;
    CRegExp     tempRE(true, CRegExp::autoUtf8);
    const std::vector<std::string>& strRegExps = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoStackRegExps;
    std::vector<std::string>::const_iterator itRegExp = strRegExps.begin();
    while (itRegExp != strRegExps.end())
    {
      (void)tempRE.RegComp(*itRegExp);
      if (tempRE.GetCaptureTotal() == 4)
        RegExps.push_back(tempRE);
      else
        CLog::Log(LOGERROR, "Invalid video stack RE ({}). Must have exactly 4 captures.",
                  *itRegExp);
      ++itRegExp;
    }
    return GetStackedTitlePath(strPath, RegExps);
  }

  std::string CStackDirectory::GetStackedTitlePath(const std::string &strPath, VECCREGEXP& RegExps)
  {
    CStackDirectory stack;
    CFileItemList   files;
    std::string      strStackTitlePath,
                    strCommonDir        = URIUtils::GetParentPath(strPath);

    const CURL pathToUrl(strPath);
    stack.GetDirectory(pathToUrl, files);

    if (files.Size() > 1)
    {
      std::string strStackTitle;

      std::string File1 = URIUtils::GetFileName(files[0]->GetPath());
      std::string File2 = URIUtils::GetFileName(files[1]->GetPath());
      // Check if source path uses URL encoding
      if (URIUtils::HasEncodedFilename(CURL(strCommonDir)))
      {
        File1 = CURL::Decode(File1);
        File2 = CURL::Decode(File2);
      }

      std::vector<CRegExp>::iterator itRegExp = RegExps.begin();
      int offset = 0;

      while (itRegExp != RegExps.end())
      {
        if (itRegExp->RegFind(File1, offset) != -1)
        {
          std::string Title1     = itRegExp->GetMatch(1),
                     Volume1    = itRegExp->GetMatch(2),
                     Ignore1    = itRegExp->GetMatch(3),
                     Extension1 = itRegExp->GetMatch(4);
          if (offset)
            Title1 = File1.substr(0, itRegExp->GetSubStart(2));
          if (itRegExp->RegFind(File2, offset) != -1)
          {
            std::string Title2     = itRegExp->GetMatch(1),
                       Volume2    = itRegExp->GetMatch(2),
                       Ignore2    = itRegExp->GetMatch(3),
                       Extension2 = itRegExp->GetMatch(4);
            if (offset)
              Title2 = File2.substr(0, itRegExp->GetSubStart(2));
            if (StringUtils::EqualsNoCase(Title1, Title2))
            {
              if (!StringUtils::EqualsNoCase(Volume1, Volume2))
              {
                if (StringUtils::EqualsNoCase(Ignore1, Ignore2) &&
                    StringUtils::EqualsNoCase(Extension1, Extension2))
                {
                  // got it
                  strStackTitle = Title1 + Ignore1 + Extension1;
                  // Check if source path uses URL encoding
                  if (URIUtils::HasEncodedFilename(CURL(strCommonDir)))
                    strStackTitle = CURL::Encode(strStackTitle);

                  itRegExp = RegExps.end();
                  break;
                }
                else // Invalid stack
                  break;
              }
              else // Early match, retry with offset
              {
                offset = itRegExp->GetSubStart(3);
                continue;
              }
            }
          }
        }
        offset = 0;
        ++itRegExp;
      }
      if (!strCommonDir.empty() && !strStackTitle.empty())
        strStackTitlePath = strCommonDir + strStackTitle;
    }

    return strStackTitlePath;
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
}

