#pragma once
/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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

#include "utils/ISerializable.h"
#include "utils/IDBInfoTag.h"
#include "utils/ISortable.h"
#include "utils/Archive.h"
#include "utils/StdString.h"
#include "DllLibExif.h"

#include <vector>
#include <string>

#define SLIDE_FILE_NAME             900         // Note that not all image tags will be present for each image
#define SLIDE_FILE_PATH             901
#define SLIDE_FILE_SIZE             902
#define SLIDE_FILE_DATE             903
#define SLIDE_INDEX                 904
#define SLIDE_RESOLUTION            905
#define SLIDE_COMMENT               906
#define SLIDE_COLOUR                907
#define SLIDE_PROCESS               908

#define SLIDE_EXIF_DATE             919 /* Implementation only to just get
                                           localized date */
#define SLIDE_EXIF_DATE_TIME        920
#define SLIDE_EXIF_DESCRIPTION      921
#define SLIDE_EXIF_CAMERA_MAKE      922
#define SLIDE_EXIF_CAMERA_MODEL     923
#define SLIDE_EXIF_COMMENT          924
#define SLIDE_EXIF_SOFTWARE         925
#define SLIDE_EXIF_APERTURE         926
#define SLIDE_EXIF_FOCAL_LENGTH     927
#define SLIDE_EXIF_FOCUS_DIST       928
#define SLIDE_EXIF_EXPOSURE         929
#define SLIDE_EXIF_EXPOSURE_TIME    930
#define SLIDE_EXIF_EXPOSURE_BIAS    931
#define SLIDE_EXIF_EXPOSURE_MODE    932
#define SLIDE_EXIF_FLASH_USED       933
#define SLIDE_EXIF_WHITE_BALANCE    934
#define SLIDE_EXIF_LIGHT_SOURCE     935
#define SLIDE_EXIF_METERING_MODE    936
#define SLIDE_EXIF_ISO_EQUIV        937
#define SLIDE_EXIF_DIGITAL_ZOOM     938
#define SLIDE_EXIF_CCD_WIDTH        939
#define SLIDE_EXIF_GPS_LATITUDE     940
#define SLIDE_EXIF_GPS_LONGITUDE    941
#define SLIDE_EXIF_GPS_ALTITUDE     942
#define SLIDE_EXIF_ORIENTATION      943

#define SLIDE_IPTC_SUP_CATEGORIES   960
#define SLIDE_IPTC_KEYWORDS         961
#define SLIDE_IPTC_CAPTION          962
#define SLIDE_IPTC_AUTHOR           963
#define SLIDE_IPTC_HEADLINE         964
#define SLIDE_IPTC_SPEC_INSTR       965
#define SLIDE_IPTC_CATEGORY         966
#define SLIDE_IPTC_BYLINE           967
#define SLIDE_IPTC_BYLINE_TITLE     968
#define SLIDE_IPTC_CREDIT           969
#define SLIDE_IPTC_SOURCE           970
#define SLIDE_IPTC_COPYRIGHT_NOTICE 971
#define SLIDE_IPTC_OBJECT_NAME      972
#define SLIDE_IPTC_CITY             973
#define SLIDE_IPTC_STATE            974
#define SLIDE_IPTC_COUNTRY          975
#define SLIDE_IPTC_TX_REFERENCE     976
#define SLIDE_IPTC_DATE             977
#define SLIDE_IPTC_COPYRIGHT        978
#define SLIDE_IPTC_COUNTRY_CODE     979
#define SLIDE_IPTC_REF_SERVICE      980

class CDateTime;

class CPictureInfoTag : public IArchivable, public ISerializable, public ISortable, public IDBInfoTag
{
public:
  CPictureInfoTag() { Reset(); };
  void Reset();
  virtual void Archive(CArchive& ar);
  virtual void Serialize(CVariant& value) const;
  virtual void Serialize(bson *document) const;
  virtual void Deserialize(const bson *document, int dbId);
  virtual void ToSortable(SortItem& sortable);
  const CPictureInfoTag& operator=(const CPictureInfoTag& item);
  const CStdString GetInfo(int info) const;
  bool GetDateTime(CDateTime &datetime) const;

  bool Loaded() const { return m_isLoaded; };
  bool Load(const CStdString &path);

  static int TranslateString(const CStdString &info);

  void SetInfo(int info, const CStdString& value);
  void SetLoaded(bool loaded = true);

  const CStdString &GetPath() const { return m_path; }
  void SetPath(const CStdString &path) { m_path = path; }

  const CStdString &GetFilename() const { return m_file; }
  void SetFilename(const CStdString &file) { m_file = file; }
  
  virtual int GetID() const { return m_databaseID; }
  void SetID(int id) { m_databaseID = id; }

  int64_t GetFileSize() const { return m_size; }
  void SetFileSize(int64_t size) { m_size = size; }

  const CStdString &GetFolder() const { return m_folder; }
  void SetFolder(const CStdString &folder) { m_folder = folder; }

  const CStdString &GetCamera() const { return m_camera; }
  // Instantiate instead of pass-by-reference so we can modify the parameters
  void SetCamera(CStdString make, CStdString model);

  const std::vector<std::string> &GetTags() const { return m_tags; }
  void SetTags(CStdString csvTags); // csvTags is comma-separated or semicolon-separated
  void AddTag(CStdString tag) { m_tags.push_back(tag); }
  void ClearTags(CStdString tag) { m_tags.clear(); }

private:
  void GetStringFromArchive(CArchive &ar, char *string, size_t length);
  ExifInfo_t m_exifInfo;
  IPTCInfo_t m_iptcInfo;
  bool       m_isLoaded;
  CStdString m_file;
  CStdString m_path;
  int        m_databaseID;
  int64_t    m_size;
  CStdString m_folder;
  int        m_year;
  CStdString m_camera;
  std::vector<std::string> m_tags;
};

