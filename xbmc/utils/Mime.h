/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>
#include <map>

class CURL;

class CFileItem;

class CMime
{
public:
  static std::string GetMimeType(const std::string &extension);
  static std::string GetMimeType(const CFileItem &item);
  static std::string GetMimeType(const CURL &url, bool lookup = true);

  enum EFileType
  {
    FileTypeUnknown = 0,
    FileTypeHtml,
    FileTypeXml,
    FileTypePlainText,
    FileTypeZip,
    FileTypeGZip,
    FileTypeRar,
    FileTypeBmp,
    FileTypeGif,
    FileTypePng,
    FileTypeJpeg,
  };
  static EFileType GetFileTypeFromMime(const std::string& mimeType);
  static EFileType GetFileTypeFromContent(const std::string& fileContent);

private:
  static bool parseMimeType(const std::string& mimeType, std::string& type, std::string& subtype);

  static const std::map<std::string, std::string> m_mimetypes;
};
