/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "StackDirectory.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "FileItem.h"
#include "utils/StringUtils.h"
#include "settings/AdvancedSettings.h"
#include "URL.h"

using namespace std;
namespace XFILE
{
  CStackDirectory::CStackDirectory()
  {
  }

  CStackDirectory::~CStackDirectory()
  {
  }

  bool CStackDirectory::GetDirectory(const string& strPath, CFileItemList& items)
  {
    items.Clear();
    vector<string> files;
    if (!GetPaths(strPath, files))
      return false;   // error in path

    for (unsigned int i = 0; i < files.size(); i++)
    {
      string file = files[i];
      CFileItemPtr item(new CFileItem(file));
      item->SetPath(file);
      item->m_bIsFolder = false;
      items.Add(item);
    }
    return true;
  }

  string CStackDirectory::GetStackedTitlePath(const string &strPath)
  {
    // Load up our REs
    VECCREGEXP  RegExps;
    CRegExp     tempRE(true, CRegExp::autoUtf8);
    const std::vector<std::string>& strRegExps = g_advancedSettings.m_videoStackRegExps;
    std::vector<std::string>::const_iterator itRegExp = strRegExps.begin();
    vector<pair<int, string> > badStacks;
    while (itRegExp != strRegExps.end())
    {
      tempRE.RegComp(*itRegExp);
      if (tempRE.GetCaptureTotal() == 4)
        RegExps.push_back(tempRE);
      else
        CLog::Log(LOGERROR, "Invalid video stack RE (%s). Must have exactly 4 captures.", itRegExp->c_str());
      itRegExp++;
    }
    return GetStackedTitlePath(strPath, RegExps);
  }

  string CStackDirectory::GetStackedTitlePath(const string &strPath, VECCREGEXP& RegExps)
  {
    CStackDirectory stack;
    CFileItemList   files;
    string      strStackTitlePath,
                    strCommonDir        = URIUtils::GetParentPath(strPath);

    stack.GetDirectory(strPath, files);

    if (files.Size() > 1)
    {
      string strStackTitle;

      string File1 = URIUtils::GetFileName(files[0]->GetPath());
      string File2 = URIUtils::GetFileName(files[1]->GetPath());
      // Check if source path uses URL encoding
      if (URIUtils::ProtocolHasEncodedFilename(CURL(strCommonDir).GetProtocol()))
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
          string Title1     = itRegExp->GetMatch(1),
                     Volume1    = itRegExp->GetMatch(2),
                     Ignore1    = itRegExp->GetMatch(3),
                     Extension1 = itRegExp->GetMatch(4);
          if (offset)
            Title1 = File1.substr(0, itRegExp->GetSubStart(2));
          if (itRegExp->RegFind(File2, offset) != -1)
          {
            string Title2     = itRegExp->GetMatch(1),
                       Volume2    = itRegExp->GetMatch(2),
                       Ignore2    = itRegExp->GetMatch(3),
                       Extension2 = itRegExp->GetMatch(4);
            if (offset)
              Title2 = File2.substr(0, itRegExp->GetSubStart(2));
            if (StringUtils::EqualsNoCase(Title1, Title2))
            {
              if (!StringUtils::EqualsNoCase(Volume1, Volume2))
              {
                if (StringUtils::EqualsNoCase(Ignore1, Ignore2) && StringUtils::EqualsNoCase(Extension1, Extension2))
                {
                  // got it
                  strStackTitle = Title1 + Ignore1 + Extension1;
                  // Check if source path uses URL encoding
                  if (URIUtils::ProtocolHasEncodedFilename(CURL(strCommonDir).GetProtocol()))
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
        itRegExp++;
      }
      if (!strCommonDir.empty() && !strStackTitle.empty())
        strStackTitlePath = strCommonDir + strStackTitle;
    }

    return strStackTitlePath;
  }

  string CStackDirectory::GetFirstStackedFile(const string &strPath)
  {
    // the stacked files are always in volume order, so just get up to the first filename
    // occurence of " , "
    string file, folder;
    size_t pos = strPath.find(" , ");
    if (pos != std::string::npos)
      URIUtils::Split((string)strPath.substr(0, pos), folder, file);
    else
      URIUtils::Split(strPath, folder, file); // single filed stacks - should really not happen

    // remove "stack://" from the folder
    folder = folder.substr(8);
    StringUtils::Replace(file, ",,", ",");

    return URIUtils::AddFileToFolder(folder, file);
  }

  bool CStackDirectory::GetPaths(const string& strPath, vector<string>& vecPaths)
  {
    // format is:
    // stack://file1 , file2 , file3 , file4
    // filenames with commas are double escaped (ie replaced with ,,), thus the " , " separator used.
    string path = strPath;
    // remove stack:// from the beginning
    path = path.substr(8);
    
    vecPaths.clear();
    vecPaths = StringUtils::Split(path, " , ");
    if (vecPaths.empty())
      return false;

    // because " , " is used as a seperator any "," in the real paths are double escaped
    for (vector<string>::iterator itPath = vecPaths.begin(); itPath != vecPaths.end(); itPath++)
      StringUtils::Replace(*itPath, ",,", ",");

    return true;
  }

  string CStackDirectory::ConstructStackPath(const CFileItemList &items, const vector<int> &stack)
  {
    // no checks on the range of stack here.
    // we replace all instances of comma's with double comma's, then separate
    // the files using " , ".
    string stackedPath = "stack://";
    string folder, file;
    URIUtils::Split(items[stack[0]]->GetPath(), folder, file);
    stackedPath += folder;
    // double escape any occurence of commas
    StringUtils::Replace(file, ",", ",,");
    stackedPath += file;
    for (unsigned int i = 1; i < stack.size(); ++i)
    {
      stackedPath += " , ";
      file = items[stack[i]]->GetPath();

      // double escape any occurence of commas
      StringUtils::Replace(file, ",", ",,");
      stackedPath += file;
    }
    return stackedPath;
  }

  bool CStackDirectory::ConstructStackPath(const vector<string> &paths, string& stackedPath)
  {
    if (paths.size() < 2)
      return false;
    stackedPath = "stack://";
    std::string folder, file;
    URIUtils::Split(paths[0], folder, file);
    stackedPath += folder;
    // double escape any occurence of commas
    StringUtils::Replace(file, ",", ",,");
    stackedPath += file;
    for (unsigned int i = 1; i < paths.size(); ++i)
    {
      stackedPath += " , ";
      file = paths[i];

      // double escape any occurence of commas
      StringUtils::Replace(file, ",", ",,");
      stackedPath += file;
    }
    return true;
  }
}

