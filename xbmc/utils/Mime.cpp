/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Mime.h"

#include "FileItem.h"
#include "URIUtils.h"
#include "URL.h"
#include "filesystem/CurlFile.h"
#include "music/tags/MusicInfoTag.h"
#include "utils/StringUtils.h"
#include "video/VideoInfoTag.h"

#include <algorithm>
#include <memory>

std::string CMime::GetMimeType(const std::string &extension)
{
  if (extension.empty())
    return "";

  std::string ext = extension;
  size_t posNotPoint = ext.find_first_not_of('.');
  if (posNotPoint != std::string::npos && posNotPoint > 0)
    ext = extension.substr(posNotPoint);
  transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

  std::unique_ptr<CMimeTypes> mime = std::make_unique<CMimeTypes>();

  return mime->Get(ext);
}

std::string CMime::GetMimeType(const CFileItem &item)
{
  std::string path = item.GetDynPath();
  if (item.HasVideoInfoTag() && !item.GetVideoInfoTag()->GetPath().empty())
    path = item.GetVideoInfoTag()->GetPath();
  else if (item.HasMusicInfoTag() && !item.GetMusicInfoTag()->GetURL().empty())
    path = item.GetMusicInfoTag()->GetURL();

  return GetMimeType(URIUtils::GetExtension(path));
}

std::string CMime::GetMimeType(const CURL &url, bool lookup)
{

  std::string strMimeType;

  if( url.IsProtocol("shout") || url.IsProtocol("http") || url.IsProtocol("https"))
  {
    // If lookup is false, bail out early to leave mime type empty
    if (!lookup)
      return strMimeType;

    std::string strmime;
    XFILE::CCurlFile::GetMimeType(url, strmime);

    // try to get mime-type again but with an NSPlayer User-Agent
    // in order for server to provide correct mime-type.  Allows us
    // to properly detect an MMS stream
    if (StringUtils::StartsWithNoCase(strmime, "video/x-ms-"))
      XFILE::CCurlFile::GetMimeType(url, strmime, "NSPlayer/11.00.6001.7000");

    // make sure there are no options set in mime-type
    // mime-type can look like "video/x-ms-asf ; charset=utf8"
    size_t i = strmime.find(';');
    if(i != std::string::npos)
      strmime.erase(i, strmime.length() - i);
    StringUtils::Trim(strmime);
    strMimeType = strmime;
  }
  else
    strMimeType = GetMimeType(url.GetFileType());

  // if it's still empty set to an unknown type
  if (strMimeType.empty())
    strMimeType = "application/octet-stream";

  return strMimeType;
}

CMime::EFileType CMime::GetFileTypeFromMime(const std::string& mimeType)
{
  // based on http://mimesniff.spec.whatwg.org/

  std::string type, subtype;
  if (!parseMimeType(mimeType, type, subtype))
    return FileTypeUnknown;

  if (type == "application")
  {
    if (subtype == "zip")
      return FileTypeZip;
    if (subtype == "x-gzip")
      return FileTypeGZip;
    if (subtype == "x-rar-compressed")
      return FileTypeRar;

    if (subtype == "xml")
      return FileTypeXml;
  }
  else if (type == "text")
  {
    if (subtype == "xml")
      return FileTypeXml;
    if (subtype == "html")
      return FileTypeHtml;
    if (subtype == "plain")
      return FileTypePlainText;
  }
  else if (type == "image")
  {
    if (subtype == "bmp")
      return FileTypeBmp;
    if (subtype == "gif")
      return FileTypeGif;
    if (subtype == "png")
      return FileTypePng;
    if (subtype == "jpeg" || subtype == "pjpeg")
      return FileTypeJpeg;
  }

  if (StringUtils::EndsWith(subtype, "+zip"))
    return FileTypeZip;
  if (StringUtils::EndsWith(subtype, "+xml"))
    return FileTypeXml;

  return FileTypeUnknown;
}

CMime::EFileType CMime::GetFileTypeFromContent(const std::string& fileContent)
{
  // based on http://mimesniff.spec.whatwg.org/#matching-a-mime-type-pattern

  const size_t len = fileContent.length();
  if (len < 2)
    return FileTypeUnknown;

  const unsigned char* const b = (const unsigned char*)fileContent.c_str();

  //! @todo add detection for text types

  // check image types
  if (b[0] == 'B' && b[1] == 'M')
    return FileTypeBmp;
  if (len >= 6 && b[0] == 'G' && b[1] == 'I' && b[2] == 'F' && b[3] == '8' && (b[4] == '7' || b[4] == '9') && b[5] == 'a')
    return FileTypeGif;
  if (len >= 8 && b[0] == 0x89 && b[1] == 'P' && b[2] == 'N' && b[3] == 'G' && b[4] == 0x0D && b[5] == 0x0A && b[6] == 0x1A && b[7] == 0x0A)
    return FileTypePng;
  if (len >= 3 && b[0] == 0xFF && b[1] == 0xD8 && b[2] == 0xFF)
    return FileTypeJpeg;

  // check archive types
  if (len >= 3 && b[0] == 0x1F && b[1] == 0x8B && b[2] == 0x08)
    return FileTypeGZip;
  if (len >= 4 && b[0] == 'P' && b[1] == 'K' && b[2] == 0x03 && b[3] == 0x04)
    return FileTypeZip;
  if (len >= 7 && b[0] == 'R' && b[1] == 'a' && b[2] == 'r' && b[3] == ' ' && b[4] == 0x1A && b[5] == 0x07 && b[6] == 0x00)
    return FileTypeRar;

  //! @todo add detection for other types if required

  return FileTypeUnknown;
}

bool CMime::parseMimeType(const std::string& mimeType, std::string& type, std::string& subtype)
{
  static const char* const whitespaceChars = "\x09\x0A\x0C\x0D\x20"; // tab, LF, FF, CR and space

  type.clear();
  subtype.clear();

  const size_t slashPos = mimeType.find('/');
  if (slashPos == std::string::npos)
    return false;

  type.assign(mimeType, 0, slashPos);
  subtype.assign(mimeType, slashPos + 1, std::string::npos);

  const size_t semicolonPos = subtype.find(';');
  if (semicolonPos != std::string::npos)
    subtype.erase(semicolonPos);

  StringUtils::Trim(type, whitespaceChars);
  StringUtils::Trim(subtype, whitespaceChars);

  if (type.empty() || subtype.empty())
  {
    type.clear();
    subtype.clear();
    return false;
  }

  StringUtils::ToLower(type);
  StringUtils::ToLower(subtype);

  return true;
}
