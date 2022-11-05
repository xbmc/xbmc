/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PictureInfoTag.h"

#include "ServiceBroker.h"
#include "addons/ExtsMimeSupportList.h"
#include "addons/ImageDecoder.h"
#include "addons/addoninfo/AddonType.h"
#include "guilib/guiinfo/GUIInfoLabels.h"
#include "utils/Archive.h"
#include "utils/CharsetConverter.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"

#include <algorithm>
#include <vector>

using namespace KODI::ADDONS;

CPictureInfoTag::ExifInfo::ExifInfo(const ExifInfo_t& other)
  : CameraMake(other.CameraMake),
    CameraModel(other.CameraModel),
    DateTime(other.DateTime),
    Height(other.Height),
    Width(other.Width),
    Orientation(other.Orientation),
    IsColor(other.IsColor),
    Process(other.Process),
    FlashUsed(other.FlashUsed),
    FocalLength(other.FocalLength),
    ExposureTime(other.ExposureTime),
    ApertureFNumber(other.ApertureFNumber),
    Distance(other.Distance),
    CCDWidth(other.CCDWidth),
    ExposureBias(other.ExposureBias),
    DigitalZoomRatio(other.DigitalZoomRatio),
    FocalLength35mmEquiv(other.FocalLength35mmEquiv),
    Whitebalance(other.Whitebalance),
    MeteringMode(other.MeteringMode),
    ExposureProgram(other.ExposureProgram),
    ExposureMode(other.ExposureMode),
    ISOequivalent(other.ISOequivalent),
    LightSource(other.LightSource),
    CommentsCharset(EXIF_COMMENT_CHARSET_CONVERTED),
    XPCommentsCharset(EXIF_COMMENT_CHARSET_CONVERTED),
    Comments(Convert(other.CommentsCharset, other.Comments)),
    FileComment(Convert(EXIF_COMMENT_CHARSET_UNKNOWN, other.FileComment)),
    XPComment(Convert(other.XPCommentsCharset, other.XPComment)),
    Description(other.Description),
    ThumbnailOffset(other.ThumbnailOffset),
    ThumbnailSize(other.ThumbnailSize),
    LargestExifOffset(other.LargestExifOffset),
    ThumbnailAtEnd(other.ThumbnailAtEnd),
    ThumbnailSizeOffset(other.ThumbnailSizeOffset),
    DateTimeOffsets(other.DateTimeOffsets, other.DateTimeOffsets + other.numDateTimeTags),
    GpsInfoPresent(other.GpsInfoPresent),
    GpsLat(other.GpsLat),
    GpsLong(other.GpsLong),
    GpsAlt(other.GpsAlt)
{
}

std::string CPictureInfoTag::ExifInfo::Convert(int charset, const char* data)
{
  std::string value;

  // The charset used for the UserComment is stored in CommentsCharset:
  // Ascii, Unicode (UCS2), JIS (X208-1990), Unknown (application specific)
  if (charset == EXIF_COMMENT_CHARSET_UNICODE)
  {
    g_charsetConverter.ucs2ToUTF8(std::u16string(reinterpret_cast<const char16_t*>(data)), value);
  }
  else
  {
    // Ascii doesn't need to be converted (EXIF_COMMENT_CHARSET_ASCII)
    // Unknown data can't be converted as it could be any codec (EXIF_COMMENT_CHARSET_UNKNOWN)
    // JIS data can't be converted as CharsetConverter and iconv lacks support (EXIF_COMMENT_CHARSET_JIS)
    g_charsetConverter.unknownToUTF8(data, value);
  }

  return value;
}

CPictureInfoTag::IPTCInfo::IPTCInfo(const IPTCInfo_t& other)
  : RecordVersion(other.RecordVersion),
    SupplementalCategories(other.SupplementalCategories),
    Keywords(other.Keywords),
    Caption(other.Caption),
    Author(other.Author),
    Headline(other.Headline),
    SpecialInstructions(other.SpecialInstructions),
    Category(other.Category),
    Byline(other.Byline),
    BylineTitle(other.BylineTitle),
    Credit(other.Credit),
    Source(other.Source),
    CopyrightNotice(other.CopyrightNotice),
    ObjectName(other.ObjectName),
    City(other.City),
    State(other.State),
    Country(other.Country),
    TransmissionReference(other.TransmissionReference),
    Date(other.Date),
    Urgency(other.Urgency),
    ReferenceService(other.ReferenceService),
    CountryCode(other.CountryCode),
    TimeCreated(other.TimeCreated),
    SubLocation(other.SubLocation),
    ImageType(other.ImageType)
{
}

void CPictureInfoTag::Reset()
{
  m_exifInfo = {};
  m_iptcInfo = {};
  m_isLoaded = false;
  m_isInfoSetExternally = false;
  m_dateTimeTaken.Reset();
}

bool CPictureInfoTag::Load(const std::string &path)
{
  m_isLoaded = false;

  // Get file extensions to find addon related to it.
  std::string strExtension = URIUtils::GetExtension(path);
  StringUtils::ToLower(strExtension);
  if (!strExtension.empty() && CServiceBroker::IsAddonInterfaceUp())
  {
    // Load via available image decoder addons
    auto addonInfos = CServiceBroker::GetExtsMimeSupportList().GetExtensionSupportedAddonInfos(
        strExtension, CExtsMimeSupportList::FilterSelect::all);
    for (const auto& addonInfo : addonInfos)
    {
      if (addonInfo.first != ADDON::AddonType::IMAGEDECODER)
        continue;

      std::unique_ptr<CImageDecoder> result = std::make_unique<CImageDecoder>(addonInfo.second, "");
      if (result->LoadInfoTag(path, this))
      {
        m_isLoaded = true;
        break;
      }
    }
  }

  // Load by Kodi's included own way
  if (!m_isLoaded)
  {
    ExifInfo_t exifInfo;
    IPTCInfo_t iptcInfo;

    if (process_jpeg(path.c_str(), &exifInfo, &iptcInfo))
    {
      m_exifInfo = ExifInfo(exifInfo);
      m_iptcInfo = IPTCInfo(iptcInfo);
      m_isLoaded = true;
    }
  }

  ConvertDateTime();

  return m_isLoaded;
}

void CPictureInfoTag::Archive(CArchive& ar)
{
  if (ar.IsStoring())
  {
    ar << m_isLoaded;
    ar << m_isInfoSetExternally;
    ar << m_exifInfo.ApertureFNumber;
    ar << m_exifInfo.CameraMake;
    ar << m_exifInfo.CameraModel;
    ar << m_exifInfo.CCDWidth;
    ar << m_exifInfo.Comments;
    ar << m_exifInfo.Description;
    ar << m_exifInfo.DateTime;
    for (std::vector<int>::size_type i = 0; i < MAX_DATE_COPIES; ++i)
    {
      if (i < m_exifInfo.DateTimeOffsets.size())
        ar << m_exifInfo.DateTimeOffsets[i];
      else
        ar << static_cast<int>(0);
    }
    ar << m_exifInfo.DigitalZoomRatio;
    ar << m_exifInfo.Distance;
    ar << m_exifInfo.ExposureBias;
    ar << m_exifInfo.ExposureMode;
    ar << m_exifInfo.ExposureProgram;
    ar << m_exifInfo.ExposureTime;
    ar << m_exifInfo.FlashUsed;
    ar << m_exifInfo.FocalLength;
    ar << m_exifInfo.FocalLength35mmEquiv;
    ar << m_exifInfo.GpsInfoPresent;
    ar << m_exifInfo.GpsAlt;
    ar << m_exifInfo.GpsLat;
    ar << m_exifInfo.GpsLong;
    ar << m_exifInfo.Height;
    ar << m_exifInfo.IsColor;
    ar << m_exifInfo.ISOequivalent;
    ar << m_exifInfo.LargestExifOffset;
    ar << m_exifInfo.LightSource;
    ar << m_exifInfo.MeteringMode;
    ar << static_cast<int>(m_exifInfo.DateTimeOffsets.size());
    ar << m_exifInfo.Orientation;
    ar << m_exifInfo.Process;
    ar << m_exifInfo.ThumbnailAtEnd;
    ar << m_exifInfo.ThumbnailOffset;
    ar << m_exifInfo.ThumbnailSize;
    ar << m_exifInfo.ThumbnailSizeOffset;
    ar << m_exifInfo.Whitebalance;
    ar << m_exifInfo.Width;
    ar << m_dateTimeTaken;

    ar << m_iptcInfo.Author;
    ar << m_iptcInfo.Byline;
    ar << m_iptcInfo.BylineTitle;
    ar << m_iptcInfo.Caption;
    ar << m_iptcInfo.Category;
    ar << m_iptcInfo.City;
    ar << m_iptcInfo.Urgency;
    ar << m_iptcInfo.CopyrightNotice;
    ar << m_iptcInfo.Country;
    ar << m_iptcInfo.CountryCode;
    ar << m_iptcInfo.Credit;
    ar << m_iptcInfo.Date;
    ar << m_iptcInfo.Headline;
    ar << m_iptcInfo.Keywords;
    ar << m_iptcInfo.ObjectName;
    ar << m_iptcInfo.ReferenceService;
    ar << m_iptcInfo.Source;
    ar << m_iptcInfo.SpecialInstructions;
    ar << m_iptcInfo.State;
    ar << m_iptcInfo.SupplementalCategories;
    ar << m_iptcInfo.TransmissionReference;
    ar << m_iptcInfo.TimeCreated;
    ar << m_iptcInfo.SubLocation;
    ar << m_iptcInfo.ImageType;
  }
  else
  {
    ar >> m_isLoaded;
    ar >> m_isInfoSetExternally;
    ar >> m_exifInfo.ApertureFNumber;
    ar >> m_exifInfo.CameraMake;
    ar >> m_exifInfo.CameraModel;
    ar >> m_exifInfo.CCDWidth;
    ar >> m_exifInfo.Comments;
    m_exifInfo.CommentsCharset = EXIF_COMMENT_CHARSET_CONVERTED; // Store and restore the comment charset converted
    ar >> m_exifInfo.Description;
    ar >> m_exifInfo.DateTime;
    m_exifInfo.DateTimeOffsets.clear();
    m_exifInfo.DateTimeOffsets.reserve(MAX_DATE_COPIES);
    for (std::vector<int>::size_type i = 0; i < MAX_DATE_COPIES; ++i)
    {
      int dateTimeOffset;
      ar >> dateTimeOffset;
      m_exifInfo.DateTimeOffsets.push_back(dateTimeOffset);
    }
    ar >> m_exifInfo.DigitalZoomRatio;
    ar >> m_exifInfo.Distance;
    ar >> m_exifInfo.ExposureBias;
    ar >> m_exifInfo.ExposureMode;
    ar >> m_exifInfo.ExposureProgram;
    ar >> m_exifInfo.ExposureTime;
    ar >> m_exifInfo.FlashUsed;
    ar >> m_exifInfo.FocalLength;
    ar >> m_exifInfo.FocalLength35mmEquiv;
    ar >> m_exifInfo.GpsInfoPresent;
    ar >> m_exifInfo.GpsAlt;
    ar >> m_exifInfo.GpsLat;
    ar >> m_exifInfo.GpsLong;
    ar >> m_exifInfo.Height;
    ar >> m_exifInfo.IsColor;
    ar >> m_exifInfo.ISOequivalent;
    ar >> m_exifInfo.LargestExifOffset;
    ar >> m_exifInfo.LightSource;
    ar >> m_exifInfo.MeteringMode;
    int numDateTimeTags;
    ar >> numDateTimeTags;
    m_exifInfo.DateTimeOffsets.resize(numDateTimeTags);
    ar >> m_exifInfo.Orientation;
    ar >> m_exifInfo.Process;
    ar >> m_exifInfo.ThumbnailAtEnd;
    ar >> m_exifInfo.ThumbnailOffset;
    ar >> m_exifInfo.ThumbnailSize;
    ar >> m_exifInfo.ThumbnailSizeOffset;
    ar >> m_exifInfo.Whitebalance;
    ar >> m_exifInfo.Width;
    ar >> m_dateTimeTaken;

    ar >> m_iptcInfo.Author;
    ar >> m_iptcInfo.Byline;
    ar >> m_iptcInfo.BylineTitle;
    ar >> m_iptcInfo.Caption;
    ar >> m_iptcInfo.Category;
    ar >> m_iptcInfo.City;
    ar >> m_iptcInfo.Urgency;
    ar >> m_iptcInfo.CopyrightNotice;
    ar >> m_iptcInfo.Country;
    ar >> m_iptcInfo.CountryCode;
    ar >> m_iptcInfo.Credit;
    ar >> m_iptcInfo.Date;
    ar >> m_iptcInfo.Headline;
    ar >> m_iptcInfo.Keywords;
    ar >> m_iptcInfo.ObjectName;
    ar >> m_iptcInfo.ReferenceService;
    ar >> m_iptcInfo.Source;
    ar >> m_iptcInfo.SpecialInstructions;
    ar >> m_iptcInfo.State;
    ar >> m_iptcInfo.SupplementalCategories;
    ar >> m_iptcInfo.TransmissionReference;
    ar >> m_iptcInfo.TimeCreated;
    ar >> m_iptcInfo.SubLocation;
    ar >> m_iptcInfo.ImageType;
  }
}

void CPictureInfoTag::Serialize(CVariant& value) const
{
  value["aperturefnumber"] = m_exifInfo.ApertureFNumber;
  value["cameramake"] = m_exifInfo.CameraMake;
  value["cameramodel"] = m_exifInfo.CameraModel;
  value["ccdwidth"] = m_exifInfo.CCDWidth;
  value["comments"] = m_exifInfo.Comments;
  value["description"] = m_exifInfo.Description;
  value["datetime"] = m_exifInfo.DateTime;
  for (std::vector<int>::size_type i = 0; i < MAX_DATE_COPIES; ++i)
  {
    if (i < m_exifInfo.DateTimeOffsets.size())
      value["datetimeoffsets"][static_cast<int>(i)] = m_exifInfo.DateTimeOffsets[i];
    else
      value["datetimeoffsets"][static_cast<int>(i)] = static_cast<int>(0);
  }
  value["digitalzoomratio"] = m_exifInfo.DigitalZoomRatio;
  value["distance"] = m_exifInfo.Distance;
  value["exposurebias"] = m_exifInfo.ExposureBias;
  value["exposuremode"] = m_exifInfo.ExposureMode;
  value["exposureprogram"] = m_exifInfo.ExposureProgram;
  value["exposuretime"] = m_exifInfo.ExposureTime;
  value["flashused"] = m_exifInfo.FlashUsed;
  value["focallength"] = m_exifInfo.FocalLength;
  value["focallength35mmequiv"] = m_exifInfo.FocalLength35mmEquiv;
  value["gpsinfopresent"] = m_exifInfo.GpsInfoPresent;
  value["gpsinfo"]["alt"] = m_exifInfo.GpsAlt;
  value["gpsinfo"]["lat"] = m_exifInfo.GpsLat;
  value["gpsinfo"]["long"] = m_exifInfo.GpsLong;
  value["height"] = m_exifInfo.Height;
  value["iscolor"] = m_exifInfo.IsColor;
  value["isoequivalent"] = m_exifInfo.ISOequivalent;
  value["largestexifoffset"] = m_exifInfo.LargestExifOffset;
  value["lightsource"] = m_exifInfo.LightSource;
  value["meteringmode"] = m_exifInfo.MeteringMode;
  value["numdatetimetags"] = static_cast<int>(m_exifInfo.DateTimeOffsets.size());
  value["orientation"] = m_exifInfo.Orientation;
  value["process"] = m_exifInfo.Process;
  value["thumbnailatend"] = m_exifInfo.ThumbnailAtEnd;
  value["thumbnailoffset"] = m_exifInfo.ThumbnailOffset;
  value["thumbnailsize"] = m_exifInfo.ThumbnailSize;
  value["thumbnailsizeoffset"] = m_exifInfo.ThumbnailSizeOffset;
  value["whitebalance"] = m_exifInfo.Whitebalance;
  value["width"] = m_exifInfo.Width;

  value["author"] = m_iptcInfo.Author;
  value["byline"] = m_iptcInfo.Byline;
  value["bylinetitle"] = m_iptcInfo.BylineTitle;
  value["caption"] = m_iptcInfo.Caption;
  value["category"] = m_iptcInfo.Category;
  value["city"] = m_iptcInfo.City;
  value["urgency"] = m_iptcInfo.Urgency;
  value["copyrightnotice"] = m_iptcInfo.CopyrightNotice;
  value["country"] = m_iptcInfo.Country;
  value["countrycode"] = m_iptcInfo.CountryCode;
  value["credit"] = m_iptcInfo.Credit;
  value["date"] = m_iptcInfo.Date;
  value["headline"] = m_iptcInfo.Headline;
  value["keywords"] = m_iptcInfo.Keywords;
  value["objectname"] = m_iptcInfo.ObjectName;
  value["referenceservice"] = m_iptcInfo.ReferenceService;
  value["source"] = m_iptcInfo.Source;
  value["specialinstructions"] = m_iptcInfo.SpecialInstructions;
  value["state"] = m_iptcInfo.State;
  value["supplementalcategories"] = m_iptcInfo.SupplementalCategories;
  value["transmissionreference"] = m_iptcInfo.TransmissionReference;
  value["timecreated"] = m_iptcInfo.TimeCreated;
  value["sublocation"] = m_iptcInfo.SubLocation;
  value["imagetype"] = m_iptcInfo.ImageType;
}

void CPictureInfoTag::ToSortable(SortItem& sortable, Field field) const
{
  if (field == FieldDateTaken && m_dateTimeTaken.IsValid())
    sortable[FieldDateTaken] = m_dateTimeTaken.GetAsDBDateTime();
}

const std::string CPictureInfoTag::GetInfo(int info) const
{
  if (!m_isLoaded && !m_isInfoSetExternally) // If no metadata has been loaded from the picture file or set with SetInfo(), just return
    return "";

  std::string value;
  switch (info)
  {
  case SLIDESHOW_RESOLUTION:
    value = StringUtils::Format("{} x {}", m_exifInfo.Width, m_exifInfo.Height);
    break;
  case SLIDESHOW_COLOUR:
    value = m_exifInfo.IsColor ? "Colour" : "Black and White";
    break;
  case SLIDESHOW_PROCESS:
    switch (m_exifInfo.Process)
    {
      case M_SOF0:
        // don't show it if its the plain old boring 'baseline' process, but do
        // show it if its something else, like 'progressive' (used on web sometimes)
        value = "Baseline";
      break;
      case M_SOF1:    value = "Extended sequential";      break;
      case M_SOF2:    value = "Progressive";      break;
      case M_SOF3:    value = "Lossless";      break;
      case M_SOF5:    value = "Differential sequential";      break;
      case M_SOF6:    value = "Differential progressive";      break;
      case M_SOF7:    value = "Differential lossless";      break;
      case M_SOF9:    value = "Extended sequential, arithmetic coding";      break;
      case M_SOF10:   value = "Progressive, arithmetic coding";     break;
      case M_SOF11:   value = "Lossless, arithmetic coding";     break;
      case M_SOF13:   value = "Differential sequential, arithmetic coding";     break;
      case M_SOF14:   value = "Differential progressive, arithmetic coding";     break;
      case M_SOF15:   value = "Differential lossless, arithmetic coding";     break;
      default:        value = "Unknown";   break;
    }
    break;
  case SLIDESHOW_COMMENT:
    value = m_exifInfo.FileComment;
    break;
  case SLIDESHOW_EXIF_COMMENT:
    value = m_exifInfo.Comments;
    break;
  case SLIDESHOW_EXIF_XPCOMMENT:
    value = m_exifInfo.XPComment;
    break;
  case SLIDESHOW_EXIF_LONG_DATE_TIME:
    if (m_dateTimeTaken.IsValid())
      value = m_dateTimeTaken.GetAsLocalizedDateTime(true);
    break;
  case SLIDESHOW_EXIF_DATE_TIME:
    if (m_dateTimeTaken.IsValid())
      value = m_dateTimeTaken.GetAsLocalizedDateTime();
    break;
  case SLIDESHOW_EXIF_LONG_DATE:
    if (m_dateTimeTaken.IsValid())
      value = m_dateTimeTaken.GetAsLocalizedDate(true);
    break;
  case SLIDESHOW_EXIF_DATE:
    if (m_dateTimeTaken.IsValid())
      value = m_dateTimeTaken.GetAsLocalizedDate();
    break;
  case SLIDESHOW_EXIF_DESCRIPTION:
    value = m_exifInfo.Description;
    break;
  case SLIDESHOW_EXIF_CAMERA_MAKE:
    value = m_exifInfo.CameraMake;
    break;
  case SLIDESHOW_EXIF_CAMERA_MODEL:
    value = m_exifInfo.CameraModel;
    break;
//  case SLIDESHOW_EXIF_SOFTWARE:
//    value = m_exifInfo.Software;
  case SLIDESHOW_EXIF_APERTURE:
    if (m_exifInfo.ApertureFNumber)
      value = StringUtils::Format("{:3.1f}", m_exifInfo.ApertureFNumber);
    break;
  case SLIDESHOW_EXIF_ORIENTATION:
    switch (m_exifInfo.Orientation)
    {
      case 1:   value = "Top Left";     break;
      case 2:   value = "Top Right";    break;
      case 3:   value = "Bottom Right"; break;
      case 4:   value = "Bottom Left";  break;
      case 5:   value = "Left Top";     break;
      case 6:   value = "Right Top";    break;
      case 7:   value = "Right Bottom"; break;
      case 8:   value = "Left Bottom";  break;
    }
    break;
  case SLIDESHOW_EXIF_FOCAL_LENGTH:
    if (m_exifInfo.FocalLength)
    {
      value = StringUtils::Format("{:4.2f}mm", m_exifInfo.FocalLength);
      if (m_exifInfo.FocalLength35mmEquiv != 0)
        value += StringUtils::Format("  (35mm Equivalent = {}mm)", m_exifInfo.FocalLength35mmEquiv);
    }
    break;
  case SLIDESHOW_EXIF_FOCUS_DIST:
    if (m_exifInfo.Distance < 0)
      value = "Infinite";
    else if (m_exifInfo.Distance > 0)
      value = StringUtils::Format("{:4.2f}m", m_exifInfo.Distance);
    break;
  case SLIDESHOW_EXIF_EXPOSURE:
    switch (m_exifInfo.ExposureProgram)
    {
      case 1:  value = "Manual";              break;
      case 2:  value = "Program (Auto)";     break;
      case 3:  value = "Aperture priority (Semi-Auto)";    break;
      case 4:  value = "Shutter priority (semi-auto)";     break;
      case 5:  value = "Creative Program (based towards depth of field)";    break;
      case 6:  value = "Action program (based towards fast shutter speed)";      break;
      case 7:  value = "Portrait Mode";    break;
      case 8:  value = "Landscape Mode";   break;
    }
    break;
  case SLIDESHOW_EXIF_EXPOSURE_TIME:
    if (m_exifInfo.ExposureTime)
    {
      if (m_exifInfo.ExposureTime < 0.010f)
        value = StringUtils::Format("{:6.4f}s", m_exifInfo.ExposureTime);
      else
        value = StringUtils::Format("{:5.3f}s", m_exifInfo.ExposureTime);
      if (m_exifInfo.ExposureTime <= 0.5f)
        value += StringUtils::Format(" (1/{})", static_cast<int>(0.5f + 1 / m_exifInfo.ExposureTime));
    }
    break;
  case SLIDESHOW_EXIF_EXPOSURE_BIAS:
    if (m_exifInfo.ExposureBias != 0)
      value = StringUtils::Format("{:4.2f} EV", m_exifInfo.ExposureBias);
    break;
  case SLIDESHOW_EXIF_EXPOSURE_MODE:
    switch (m_exifInfo.ExposureMode)
    {
      case 0:  value = "Automatic";          break;
      case 1:  value = "Manual";             break;
      case 2:  value = "Auto bracketing";    break;
    }
    break;
  case SLIDESHOW_EXIF_FLASH_USED:
    if (m_exifInfo.FlashUsed >= 0)
    {
      if (m_exifInfo.FlashUsed & 1)
      {
        value = "Yes";
        switch (m_exifInfo.FlashUsed)
        {
          case 0x5:  value = "Yes (Strobe light not detected)";                break;
          case 0x7:  value = "Yes (Strobe light detected)";                  break;
          case 0x9:  value = "Yes (Manual)";                  break;
          case 0xd:  value = "Yes (Manual, return light not detected)";          break;
          case 0xf:  value = "Yes (Manual, return light detected)";            break;
          case 0x19: value = "Yes (Auto)";                    break;
          case 0x1d: value = "Yes (Auto, return light not detected)";            break;
          case 0x1f: value = "Yes (Auto, return light detected)";              break;
          case 0x41: value = "Yes (Red eye reduction mode)";                  break;
          case 0x45: value = "Yes (Red eye reduction mode return light not detected)";          break;
          case 0x47: value = "Yes (Red eye reduction mode return light detected)";            break;
          case 0x49: value = "Yes (Manual, red eye reduction mode)";            break;
          case 0x4d: value = "Yes (Manual, red eye reduction mode, return light not detected)";    break;
          case 0x4f: value = "Yes (Manual, red eye reduction mode, return light detected)";      break;
          case 0x59: value = "Yes (Auto, red eye reduction mode)";              break;
          case 0x5d: value = "Yes (Auto, red eye reduction mode, return light not detected)";      break;
          case 0x5f: value = "Yes (Auto, red eye reduction mode, return light detected)";        break;
        }
      }
      else
        value = m_exifInfo.FlashUsed == 0x18 ? "No (Auto)" : "No";
    }
    break;
  case SLIDESHOW_EXIF_WHITE_BALANCE:
    return m_exifInfo.Whitebalance ? "Manual" : "Auto";
  case SLIDESHOW_EXIF_LIGHT_SOURCE:
    switch (m_exifInfo.LightSource)
    {
      case 1:   value = "Daylight";       break;
      case 2:   value = "Fluorescent";    break;
      case 3:   value = "Incandescent";   break;
      case 4:   value = "Flash";          break;
      case 9:   value = "Fine Weather";    break;
      case 11:  value = "Shade";          break;
      default:;   //Quercus: 17-1-2004 There are many more modes for this, check Exif2.2 specs
                  // If it just says 'unknown' or we don't know it, then
                  // don't bother showing it - it doesn't add any useful information.
    }
    break;
  case SLIDESHOW_EXIF_METERING_MODE:
    switch (m_exifInfo.MeteringMode)
    {
      case 2:  value = "Center weight"; break;
      case 3:  value = "Spot";   break;
      case 5:  value = "Matrix"; break;
    }
    break;
  case SLIDESHOW_EXIF_ISO_EQUIV:
    if (m_exifInfo.ISOequivalent)
      value = StringUtils::Format("{:2}", m_exifInfo.ISOequivalent);
    break;
  case SLIDESHOW_EXIF_DIGITAL_ZOOM:
    if (m_exifInfo.DigitalZoomRatio)
      value = StringUtils::Format("{:1.3f}x", m_exifInfo.DigitalZoomRatio);
    break;
  case SLIDESHOW_EXIF_CCD_WIDTH:
    if (m_exifInfo.CCDWidth)
      value = StringUtils::Format("{:4.2f}mm", m_exifInfo.CCDWidth);
    break;
  case SLIDESHOW_EXIF_GPS_LATITUDE:
    value = m_exifInfo.GpsLat;
    break;
  case SLIDESHOW_EXIF_GPS_LONGITUDE:
    value = m_exifInfo.GpsLong;
    break;
  case SLIDESHOW_EXIF_GPS_ALTITUDE:
    value = m_exifInfo.GpsAlt;
    break;
  case SLIDESHOW_IPTC_SUP_CATEGORIES:   value = m_iptcInfo.SupplementalCategories;  break;
  case SLIDESHOW_IPTC_KEYWORDS:         value = m_iptcInfo.Keywords;                break;
  case SLIDESHOW_IPTC_CAPTION:          value = m_iptcInfo.Caption;                 break;
  case SLIDESHOW_IPTC_AUTHOR:           value = m_iptcInfo.Author;                  break;
  case SLIDESHOW_IPTC_HEADLINE:         value = m_iptcInfo.Headline;                break;
  case SLIDESHOW_IPTC_SPEC_INSTR:       value = m_iptcInfo.SpecialInstructions;     break;
  case SLIDESHOW_IPTC_CATEGORY:         value = m_iptcInfo.Category;                break;
  case SLIDESHOW_IPTC_BYLINE:           value = m_iptcInfo.Byline;                  break;
  case SLIDESHOW_IPTC_BYLINE_TITLE:     value = m_iptcInfo.BylineTitle;             break;
  case SLIDESHOW_IPTC_CREDIT:           value = m_iptcInfo.Credit;                  break;
  case SLIDESHOW_IPTC_SOURCE:           value = m_iptcInfo.Source;                  break;
  case SLIDESHOW_IPTC_COPYRIGHT_NOTICE: value = m_iptcInfo.CopyrightNotice;         break;
  case SLIDESHOW_IPTC_OBJECT_NAME:      value = m_iptcInfo.ObjectName;              break;
  case SLIDESHOW_IPTC_CITY:             value = m_iptcInfo.City;                    break;
  case SLIDESHOW_IPTC_STATE:            value = m_iptcInfo.State;                   break;
  case SLIDESHOW_IPTC_COUNTRY:          value = m_iptcInfo.Country;                 break;
  case SLIDESHOW_IPTC_TX_REFERENCE:     value = m_iptcInfo.TransmissionReference;   break;
  case SLIDESHOW_IPTC_DATE:             value = m_iptcInfo.Date;                    break;
  case SLIDESHOW_IPTC_URGENCY:          value = m_iptcInfo.Urgency;                 break;
  case SLIDESHOW_IPTC_COUNTRY_CODE:     value = m_iptcInfo.CountryCode;             break;
  case SLIDESHOW_IPTC_REF_SERVICE:      value = m_iptcInfo.ReferenceService;        break;
  case SLIDESHOW_IPTC_TIMECREATED:      value = m_iptcInfo.TimeCreated;             break;
  case SLIDESHOW_IPTC_SUBLOCATION:      value = m_iptcInfo.SubLocation;             break;
  case SLIDESHOW_IPTC_IMAGETYPE:        value = m_iptcInfo.ImageType;               break;
  default:
    break;
  }
  return value;
}

int CPictureInfoTag::TranslateString(const std::string &info)
{
  if (StringUtils::EqualsNoCase(info, "filename")) return SLIDESHOW_FILE_NAME;
  else if (StringUtils::EqualsNoCase(info, "path")) return SLIDESHOW_FILE_PATH;
  else if (StringUtils::EqualsNoCase(info, "filesize")) return SLIDESHOW_FILE_SIZE;
  else if (StringUtils::EqualsNoCase(info, "filedate")) return SLIDESHOW_FILE_DATE;
  else if (StringUtils::EqualsNoCase(info, "slideindex")) return SLIDESHOW_INDEX;
  else if (StringUtils::EqualsNoCase(info, "resolution")) return SLIDESHOW_RESOLUTION;
  else if (StringUtils::EqualsNoCase(info, "slidecomment")) return SLIDESHOW_COMMENT;
  else if (StringUtils::EqualsNoCase(info, "colour")) return SLIDESHOW_COLOUR;
  else if (StringUtils::EqualsNoCase(info, "process")) return SLIDESHOW_PROCESS;
  else if (StringUtils::EqualsNoCase(info, "exiftime")) return SLIDESHOW_EXIF_DATE_TIME;
  else if (StringUtils::EqualsNoCase(info, "exifdate")) return SLIDESHOW_EXIF_DATE;
  else if (StringUtils::EqualsNoCase(info, "longexiftime")) return SLIDESHOW_EXIF_LONG_DATE_TIME;
  else if (StringUtils::EqualsNoCase(info, "longexifdate")) return SLIDESHOW_EXIF_LONG_DATE;
  else if (StringUtils::EqualsNoCase(info, "exifdescription")) return SLIDESHOW_EXIF_DESCRIPTION;
  else if (StringUtils::EqualsNoCase(info, "cameramake")) return SLIDESHOW_EXIF_CAMERA_MAKE;
  else if (StringUtils::EqualsNoCase(info, "cameramodel")) return SLIDESHOW_EXIF_CAMERA_MODEL;
  else if (StringUtils::EqualsNoCase(info, "exifcomment")) return SLIDESHOW_EXIF_COMMENT;
  else if (StringUtils::EqualsNoCase(info, "exifsoftware")) return SLIDESHOW_EXIF_SOFTWARE;
  else if (StringUtils::EqualsNoCase(info, "aperture")) return SLIDESHOW_EXIF_APERTURE;
  else if (StringUtils::EqualsNoCase(info, "focallength")) return SLIDESHOW_EXIF_FOCAL_LENGTH;
  else if (StringUtils::EqualsNoCase(info, "focusdistance")) return SLIDESHOW_EXIF_FOCUS_DIST;
  else if (StringUtils::EqualsNoCase(info, "exposure")) return SLIDESHOW_EXIF_EXPOSURE;
  else if (StringUtils::EqualsNoCase(info, "exposuretime")) return SLIDESHOW_EXIF_EXPOSURE_TIME;
  else if (StringUtils::EqualsNoCase(info, "exposurebias")) return SLIDESHOW_EXIF_EXPOSURE_BIAS;
  else if (StringUtils::EqualsNoCase(info, "exposuremode")) return SLIDESHOW_EXIF_EXPOSURE_MODE;
  else if (StringUtils::EqualsNoCase(info, "flashused")) return SLIDESHOW_EXIF_FLASH_USED;
  else if (StringUtils::EqualsNoCase(info, "whitebalance")) return SLIDESHOW_EXIF_WHITE_BALANCE;
  else if (StringUtils::EqualsNoCase(info, "lightsource")) return SLIDESHOW_EXIF_LIGHT_SOURCE;
  else if (StringUtils::EqualsNoCase(info, "meteringmode")) return SLIDESHOW_EXIF_METERING_MODE;
  else if (StringUtils::EqualsNoCase(info, "isoequivalence")) return SLIDESHOW_EXIF_ISO_EQUIV;
  else if (StringUtils::EqualsNoCase(info, "digitalzoom")) return SLIDESHOW_EXIF_DIGITAL_ZOOM;
  else if (StringUtils::EqualsNoCase(info, "ccdwidth")) return SLIDESHOW_EXIF_CCD_WIDTH;
  else if (StringUtils::EqualsNoCase(info, "orientation")) return SLIDESHOW_EXIF_ORIENTATION;
  else if (StringUtils::EqualsNoCase(info, "supplementalcategories")) return SLIDESHOW_IPTC_SUP_CATEGORIES;
  else if (StringUtils::EqualsNoCase(info, "keywords")) return SLIDESHOW_IPTC_KEYWORDS;
  else if (StringUtils::EqualsNoCase(info, "caption")) return SLIDESHOW_IPTC_CAPTION;
  else if (StringUtils::EqualsNoCase(info, "author")) return SLIDESHOW_IPTC_AUTHOR;
  else if (StringUtils::EqualsNoCase(info, "headline")) return SLIDESHOW_IPTC_HEADLINE;
  else if (StringUtils::EqualsNoCase(info, "specialinstructions")) return SLIDESHOW_IPTC_SPEC_INSTR;
  else if (StringUtils::EqualsNoCase(info, "category")) return SLIDESHOW_IPTC_CATEGORY;
  else if (StringUtils::EqualsNoCase(info, "byline")) return SLIDESHOW_IPTC_BYLINE;
  else if (StringUtils::EqualsNoCase(info, "bylinetitle")) return SLIDESHOW_IPTC_BYLINE_TITLE;
  else if (StringUtils::EqualsNoCase(info, "credit")) return SLIDESHOW_IPTC_CREDIT;
  else if (StringUtils::EqualsNoCase(info, "source")) return SLIDESHOW_IPTC_SOURCE;
  else if (StringUtils::EqualsNoCase(info, "copyrightnotice")) return SLIDESHOW_IPTC_COPYRIGHT_NOTICE;
  else if (StringUtils::EqualsNoCase(info, "objectname")) return SLIDESHOW_IPTC_OBJECT_NAME;
  else if (StringUtils::EqualsNoCase(info, "city")) return SLIDESHOW_IPTC_CITY;
  else if (StringUtils::EqualsNoCase(info, "state")) return SLIDESHOW_IPTC_STATE;
  else if (StringUtils::EqualsNoCase(info, "country")) return SLIDESHOW_IPTC_COUNTRY;
  else if (StringUtils::EqualsNoCase(info, "transmissionreference")) return SLIDESHOW_IPTC_TX_REFERENCE;
  else if (StringUtils::EqualsNoCase(info, "iptcdate")) return SLIDESHOW_IPTC_DATE;
  else if (StringUtils::EqualsNoCase(info, "urgency")) return SLIDESHOW_IPTC_URGENCY;
  else if (StringUtils::EqualsNoCase(info, "countrycode")) return SLIDESHOW_IPTC_COUNTRY_CODE;
  else if (StringUtils::EqualsNoCase(info, "referenceservice")) return SLIDESHOW_IPTC_REF_SERVICE;
  else if (StringUtils::EqualsNoCase(info, "latitude")) return SLIDESHOW_EXIF_GPS_LATITUDE;
  else if (StringUtils::EqualsNoCase(info, "longitude")) return SLIDESHOW_EXIF_GPS_LONGITUDE;
  else if (StringUtils::EqualsNoCase(info, "altitude")) return SLIDESHOW_EXIF_GPS_ALTITUDE;
  else if (StringUtils::EqualsNoCase(info, "timecreated")) return SLIDESHOW_IPTC_TIMECREATED;
  else if (StringUtils::EqualsNoCase(info, "sublocation")) return SLIDESHOW_IPTC_SUBLOCATION;
  else if (StringUtils::EqualsNoCase(info, "imagetype")) return SLIDESHOW_IPTC_IMAGETYPE;
  return 0;
}

void CPictureInfoTag::SetInfo(const std::string &key, const std::string& value)
{
  int info = TranslateString(key);

  switch (info)
  {
  case SLIDESHOW_RESOLUTION:
    {
      std::vector<std::string> dimension;
      StringUtils::Tokenize(value, dimension, ",");
      if (dimension.size() == 2)
      {
        m_exifInfo.Width = atoi(dimension[0].c_str());
        m_exifInfo.Height = atoi(dimension[1].c_str());
        m_isInfoSetExternally = true; // Set the internal state to show metadata has been set by call to SetInfo
      }
      break;
    }
  case SLIDESHOW_EXIF_DATE_TIME:
    {
      m_exifInfo.DateTime = value;
      m_isInfoSetExternally = true; // Set the internal state to show metadata has been set by call to SetInfo
      ConvertDateTime();
      break;
    }
  default:
    break;
  }
}

const CDateTime& CPictureInfoTag::GetDateTimeTaken() const
{
  return m_dateTimeTaken;
}

void CPictureInfoTag::ConvertDateTime()
{
  const std::string& dateTime = m_exifInfo.DateTime;
  if (dateTime.length() >= 19 && dateTime[0] != ' ')
  {
    int year  = atoi(dateTime.substr(0, 4).c_str());
    int month = atoi(dateTime.substr(5, 2).c_str());
    int day   = atoi(dateTime.substr(8, 2).c_str());
    int hour  = atoi(dateTime.substr(11,2).c_str());
    int min   = atoi(dateTime.substr(14,2).c_str());
    int sec   = atoi(dateTime.substr(17,2).c_str());
    m_dateTimeTaken.SetDateTime(year, month, day, hour, min, sec);
  }
}
