/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FTPDirectory.h"

#include "CurlFile.h"
#include "FTPParse.h"
#include "FileItem.h"
#include "FileItemList.h"
#include "URL.h"
#include "utils/CharsetConverter.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#include <climits>

using namespace XFILE;

CFTPDirectory::CFTPDirectory(void) = default;
CFTPDirectory::~CFTPDirectory(void) = default;

bool CFTPDirectory::GetDirectory(const CURL& url2, CFileItemList &items)
{
  CCurlFile reader;

  CURL url(url2);

  std::string path = url.GetFileName();
  if( !path.empty() && !StringUtils::EndsWith(path, "/") )
  {
    path += "/";
    url.SetFileName(path);
  }

  if (!reader.Open(url))
    return false;

  bool serverNotUseUTF8 = url.GetProtocolOption("utf8") == "0";

  char buffer[MAX_PATH + 1024];
  while( reader.ReadString(buffer, sizeof(buffer)) )
  {
    std::string strBuffer = buffer;

    StringUtils::RemoveCRLF(strBuffer);

    CFTPParse parse;
    if (parse.FTPParse(strBuffer))
    {
      if( parse.getName().length() == 0 )
        continue;

      if( parse.getFlagtrycwd() == 0 && parse.getFlagtryretr() == 0 )
        continue;

      /* buffer name */
      std::string name;
      name.assign(parse.getName());

      if( name == ".." || name == "." )
        continue;

      // server returned filename could in utf8 or non-utf8 encoding
      // we need utf8, so convert it to utf8 anyway
      g_charsetConverter.unknownToUTF8(name);

      // convert got empty result, ignore it
      if (name.empty())
        continue;

      if (serverNotUseUTF8 || name != parse.getName())
        // non-utf8 name path, tag it with protocol option.
        // then we can talk to server with the same encoding in CurlFile according to this tag.
        url.SetProtocolOption("utf8", "0");
      else
        url.RemoveProtocolOption("utf8");

      CFileItemPtr pItem(new CFileItem(name));

      pItem->m_bIsFolder = parse.getFlagtrycwd() != 0;
      std::string filePath = path + name;
      if (pItem->m_bIsFolder)
        URIUtils::AddSlashAtEnd(filePath);

      /* qualify the url with host and all */
      url.SetFileName(filePath);
      pItem->SetPath(url.Get());

      pItem->m_dwSize = parse.getSize();
      pItem->m_dateTime=parse.getTime();

      items.Add(pItem);
    }
  }

  return true;
}

bool CFTPDirectory::Exists(const CURL& url)
{
  // make sure ftp dir ends with slash,
  // curl need to known it's a dir to check ftp directory existence.
  std::string file = url.Get();
  URIUtils::AddSlashAtEnd(file);

  CCurlFile ftp;
  CURL url2(file);
  return ftp.Exists(url2);
}
