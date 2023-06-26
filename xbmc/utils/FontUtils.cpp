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
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "utils/CharsetConverter.h"
#include "utils/log.h"

#include <ft2build.h>

#include FT_FREETYPE_H
#include FT_SFNT_NAMES_H
#include FT_TRUETYPE_IDS_H

using namespace XFILE;

namespace
{
// \brief Get font family from SFNT table entries
std::string GetFamilyNameFromSfnt(FT_Face face)
{
  std::string familyName;

  for (FT_UInt index = 0; index < FT_Get_Sfnt_Name_Count(face); ++index)
  {
    FT_SfntName name;
    if (FT_Get_Sfnt_Name(face, index, &name) != 0)
    {
      CLog::LogF(LOGWARNING, "Failed to get SFNT name at index {}", index);
      continue;
    }

    // Get the unicode font family name (format-specific interface)
    // In properties there may be one or more font family names encoded for each platform.
    // NOTE: we give preference to MS/APPLE platform data, then fallback to MAC
    // because has been found some fonts that provide names not convertible MAC text to UTF8
    if (name.name_id == TT_NAME_ID_FONT_FAMILY)
    {
      const std::string nameEnc{reinterpret_cast<const char*>(name.string), name.string_len};

      if (name.platform_id == TT_PLATFORM_MICROSOFT ||
          name.platform_id == TT_PLATFORM_APPLE_UNICODE)
      {
        if (name.language_id != TT_MAC_LANGID_ENGLISH &&
            name.language_id != TT_MS_LANGID_ENGLISH_UNITED_STATES &&
            name.language_id != TT_MS_LANGID_ENGLISH_UNITED_KINGDOM)
          continue;

        if (CCharsetConverter::utf16BEtoUTF8(nameEnc, familyName))
          break; // Stop here to prefer the name given with this platform
        else
          CLog::LogF(LOGERROR, "Failed to convert the font name string encoded as \"UTF-16BE\"");
      }
      else if (name.platform_id == TT_PLATFORM_MACINTOSH && familyName.empty())
      {
        if (name.language_id != TT_MAC_LANGID_ENGLISH || name.encoding_id != TT_MAC_ID_ROMAN)
          continue;

        if (!CCharsetConverter::MacintoshToUTF8(nameEnc, familyName))
          CLog::LogF(LOGERROR, "Failed to convert the font name string encoded as \"macintosh\"");
      }
      else
      {
        CLog::LogF(LOGERROR, "Unsupported font SFNT name platform \"{}\"", name.platform_id);
      }
    }
  }
  return familyName;
}
} // unnamed namespace

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
    familyName = GetFamilyNameFromSfnt(face);
    if (familyName.empty())
    {
      CLog::LogF(LOGWARNING, "Failed to get the unicode family name for \"{}\", fallback to ASCII",
                 face->family_name);
      // ASCII font family name may differ from the unicode one, use this as fallback only
      familyName = std::string{face->family_name};
      if (familyName.empty())
        CLog::LogF(LOGERROR, "Family name missing in the font");
    }
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
