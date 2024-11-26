/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PlayListURL.h"

#include "FileItem.h"
#include "filesystem/File.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

using namespace XFILE;

// example url file
//[DEFAULT]
//BASEURL=http://msdn2.microsoft.com/en-us/library/ms812698.aspx
//[InternetShortcut]
//URL=http://msdn2.microsoft.com/en-us/library/ms812698.aspx

namespace KODI::PLAYLIST
{

CPlayListURL::CPlayListURL(void) = default;

CPlayListURL::~CPlayListURL(void) = default;

bool CPlayListURL::Load(const std::string& strFileName)
{
  std::string strLine;
  strLine.reserve(1024);

  Clear();

  m_strPlayListName = URIUtils::GetFileName(strFileName);
  URIUtils::GetParentPath(strFileName, m_strBasePath);

  CFile file;
  if (!file.Open(strFileName) )
  {
    file.Close();
    return false;
  }

  while (file.ReadLine(strLine))
  {
    StringUtils::RemoveCRLF(strLine);

    if (StringUtils::StartsWith(strLine, "[InternetShortcut]"))
    {
      if (file.ReadLine(strLine))
      {
        StringUtils::RemoveCRLF(strLine);
        if (StringUtils::StartsWith(strLine, "URL="))
        {
          CFileItemPtr newItem(new CFileItem(strLine.substr(4), false));
          Add(newItem);
        }
      }
    }
  }

  file.Close();
  return true;
}

} // namespace KODI::PLAYLIST
