/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://kodi.tv
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

  static std::map<std::string, std::string> m_mimetypes;
};
