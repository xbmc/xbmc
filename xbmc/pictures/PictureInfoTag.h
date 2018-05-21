#pragma once
/*
 *      Copyright (C) 2005-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "utils/ISerializable.h"
#include "utils/ISortable.h"
#include "utils/IArchivable.h"
#include "XBDateTime.h"
#include "libexif.h"

class CVariant;

class CPictureInfoTag : public IArchivable, public ISerializable, public ISortable
{
public:
  CPictureInfoTag() { Reset(); };
  void Reset();
  void Archive(CArchive& ar) override;
  void Serialize(CVariant& value) const override;
  void ToSortable(SortItem& sortable, Field field) const override;
  const CPictureInfoTag& operator=(const CPictureInfoTag& item);
  const std::string GetInfo(int info) const;

  bool Loaded() const { return m_isLoaded; };
  bool Load(const std::string &path);

  void SetInfo(const std::string& key, const std::string& value);

  /**
   * GetDateTimeTaken() -- Returns the EXIF DateTimeOriginal for current picture
   * 
   * The exif library returns DateTimeOriginal if available else the other
   * DateTime tags. See libexif CExifParse::ProcessDir for details.
   */
  const CDateTime& GetDateTimeTaken() const;
private:
  static int TranslateString(const std::string &info);
  void GetStringFromArchive(CArchive &ar, char *string, size_t length);

  ExifInfo_t m_exifInfo;
  IPTCInfo_t m_iptcInfo;
  bool       m_isLoaded;             // Set to true if metadata has been loaded from the picture file successfully
  bool       m_isInfoSetExternally;  // Set to true if metadata has been set by an external call to SetInfo
  CDateTime  m_dateTimeTaken;
  void ConvertDateTime();
};

