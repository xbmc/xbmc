/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "XBDateTime.h"
#include "libexif.h"
#include "utils/IArchivable.h"
#include "utils/ISerializable.h"
#include "utils/ISortable.h"

#include <string>

class CVariant;

class CPictureInfoTag : public IArchivable, public ISerializable, public ISortable
{
public:
  CPictureInfoTag() { Reset(); };
  virtual ~CPictureInfoTag() = default;
  void Reset();
  void Archive(CArchive& ar) override;
  void Serialize(CVariant& value) const override;
  void ToSortable(SortItem& sortable, Field field) const override;
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

