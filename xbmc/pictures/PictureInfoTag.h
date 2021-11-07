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
#include <vector>

namespace KODI
{
namespace ADDONS
{
class CImageDecoder;
} /* namespace ADDONS */
} /* namespace KODI */

class CVariant;

class CPictureInfoTag : public IArchivable, public ISerializable, public ISortable
{
  friend class KODI::ADDONS::CImageDecoder;

  // Mimic structs from libexif.h but with C++ types instead of arrays
  struct ExifInfo
  {
    ExifInfo() = default;
    ExifInfo(const ExifInfo&) = default;
    ExifInfo(ExifInfo&&) = default;
    ExifInfo(const ExifInfo_t& other);

    ExifInfo& operator=(const ExifInfo&) = default;
    ExifInfo& operator=(ExifInfo&&) = default;

    std::string CameraMake;
    std::string CameraModel;
    std::string DateTime;
    int Height{};
    int Width{};
    int Orientation{};
    int IsColor{};
    int Process{};
    int FlashUsed{};
    float FocalLength{};
    float ExposureTime{};
    float ApertureFNumber{};
    float Distance{};
    float CCDWidth{};
    float ExposureBias{};
    float DigitalZoomRatio{};
    int FocalLength35mmEquiv{};
    int Whitebalance{};
    int MeteringMode{};
    int ExposureProgram{};
    int ExposureMode{};
    int ISOequivalent{};
    int LightSource{};
    int CommentsCharset{};
    int XPCommentsCharset{};
    std::string Comments;
    std::string FileComment;
    std::string XPComment;
    std::string Description;

    unsigned ThumbnailOffset{};
    unsigned ThumbnailSize{};
    unsigned LargestExifOffset{};

    char ThumbnailAtEnd{};
    int ThumbnailSizeOffset{};

    std::vector<int> DateTimeOffsets;

    int GpsInfoPresent{};
    std::string GpsLat;
    std::string GpsLong;
    std::string GpsAlt;

  private:
    static std::string Convert(int charset, const char* data);
  };

  struct IPTCInfo
  {
    IPTCInfo() = default;
    IPTCInfo(const IPTCInfo&) = default;
    IPTCInfo(IPTCInfo&&) = default;
    IPTCInfo(const IPTCInfo_t& other);

    IPTCInfo& operator=(const IPTCInfo&) = default;
    IPTCInfo& operator=(IPTCInfo&&) = default;

    std::string RecordVersion;
    std::string SupplementalCategories;
    std::string Keywords;
    std::string Caption;
    std::string Author;
    std::string Headline;
    std::string SpecialInstructions;
    std::string Category;
    std::string Byline;
    std::string BylineTitle;
    std::string Credit;
    std::string Source;
    std::string CopyrightNotice;
    std::string ObjectName;
    std::string City;
    std::string State;
    std::string Country;
    std::string TransmissionReference;
    std::string Date;
    std::string Urgency;
    std::string ReferenceService;
    std::string CountryCode;
    std::string TimeCreated;
    std::string SubLocation;
    std::string ImageType;
  };

public:
  CPictureInfoTag() { Reset(); }
  virtual ~CPictureInfoTag() = default;
  void Reset();
  void Archive(CArchive& ar) override;
  void Serialize(CVariant& value) const override;
  void ToSortable(SortItem& sortable, Field field) const override;
  const std::string GetInfo(int info) const;

  bool Loaded() const { return m_isLoaded; }
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

  ExifInfo m_exifInfo;
  IPTCInfo m_iptcInfo;
  bool       m_isLoaded;             // Set to true if metadata has been loaded from the picture file successfully
  bool       m_isInfoSetExternally;  // Set to true if metadata has been set by an external call to SetInfo
  CDateTime  m_dateTimeTaken;
  void ConvertDateTime();
};

