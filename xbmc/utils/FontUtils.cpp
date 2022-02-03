/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FontUtils.h"

#include "FileItem.h"
#include "StringUtils.h"
#include "URIUtils.h"
#include "Util.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "utils/log.h"

#include <ft2build.h>

#include FT_FREETYPE_H

using namespace XFILE;

std::string UTILS::FONT::GetFontFamily(std::vector<uint8_t>& buffer)
{
  FT_Library m_library{nullptr};
  FT_Init_FreeType(&m_library);
  if (!m_library)
  {
    CLog::LogF(LOGERROR, "Unable to initialize freetype library");
    return "";
  }

  // Load the font face
  FT_Face face;
  std::string familyName;
  if (FT_New_Memory_Face(m_library, reinterpret_cast<const FT_Byte*>(buffer.data()), buffer.size(),
                         0, &face) == 0)
  {
    familyName = std::string(face->family_name);
  }
  else
  {
    CLog::LogF(LOGERROR, "Failed to process font memory buffer");
  }

  FT_Done_Face(face);
  FT_Done_FreeType(m_library);
  return familyName;
}

std::string UTILS::FONT::GetFontFamily(const std::string& filepath)
{
  std::vector<uint8_t> buffer;
  if (filepath.empty())
    return "";
  if (XFILE::CFile().LoadFile(filepath, buffer) <= 0)
  {
    CLog::LogF(LOGERROR, "Failed to load file {}", filepath);
    return "";
  }
  return GetFontFamily(buffer);
}

bool UTILS::FONT::IsSupportedFontExtension(const std::string& filepath)
{
  return URIUtils::HasExtension(filepath, UTILS::FONT::SUPPORTED_EXTENSIONS_MASK);
}

void UTILS::FONT::ClearTemporaryFonts()
{
  if (!CDirectory::Exists(UTILS::FONT::FONTPATH::TEMP))
    return;

  CFileItemList items;
  CDirectory::GetDirectory(UTILS::FONT::FONTPATH::TEMP, items,
                           UTILS::FONT::SUPPORTED_EXTENSIONS_MASK,
                           DIR_FLAG_NO_FILE_DIRS | DIR_FLAG_BYPASS_CACHE | DIR_FLAG_GET_HIDDEN);
  for (const auto& item : items)
  {
    if (item->m_bIsFolder)
      continue;

    CFile::Delete(item->GetPath());
  }
}

std::string UTILS::FONT::FONTPATH::GetSystemFontPath(const std::string& filename)
{
  std::string fontPath = URIUtils::AddFileToFolder(
      CSpecialProtocol::TranslatePath(UTILS::FONT::FONTPATH::SYSTEM), filename);
  if (XFILE::CFile::Exists(fontPath))
  {
    return CSpecialProtocol::TranslatePath(fontPath);
  }

  CLog::LogF(LOGERROR, "Could not find application system font {}", filename);
  return "";
}
