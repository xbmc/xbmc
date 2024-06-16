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
#include "pictures/metadata/ImageMetadataParser.h"
#include "utils/Archive.h"
#include "utils/CharsetConverter.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"

#include <algorithm>
#include <vector>

using namespace KODI::ADDONS;

void CPictureInfoTag::Reset()
{
  m_imageMetadata = {};
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
    std::unique_ptr<ImageMetadata> metadata = CImageMetadataParser::ExtractMetadata(path);
    if (metadata)
    {
      m_imageMetadata = *(metadata.release());
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
    ar << m_imageMetadata.exifInfo.ApertureFNumber;
    ar << m_imageMetadata.exifInfo.CameraMake;
    ar << m_imageMetadata.exifInfo.CameraModel;
    ar << m_imageMetadata.exifInfo.CCDWidth;
    ar << m_imageMetadata.exifInfo.Comments;
    ar << m_imageMetadata.exifInfo.Description;
    ar << m_imageMetadata.exifInfo.DateTime;
    ar << m_imageMetadata.exifInfo.DigitalZoomRatio;
    ar << m_imageMetadata.exifInfo.Distance;
    ar << m_imageMetadata.exifInfo.ExposureBias;
    ar << m_imageMetadata.exifInfo.ExposureMode;
    ar << m_imageMetadata.exifInfo.ExposureProgram;
    ar << m_imageMetadata.exifInfo.ExposureTime;
    ar << m_imageMetadata.exifInfo.FlashUsed;
    ar << m_imageMetadata.exifInfo.FocalLength;
    ar << m_imageMetadata.exifInfo.FocalLength35mmEquiv;
    ar << m_imageMetadata.exifInfo.GpsInfoPresent;
    ar << m_imageMetadata.exifInfo.GpsAlt;
    ar << m_imageMetadata.exifInfo.GpsLat;
    ar << m_imageMetadata.exifInfo.GpsLong;
    ar << m_imageMetadata.height;
    ar << m_imageMetadata.isColor;
    ar << m_imageMetadata.exifInfo.ISOequivalent;
    ar << m_imageMetadata.exifInfo.LightSource;
    ar << m_imageMetadata.exifInfo.MeteringMode;
    ar << m_imageMetadata.exifInfo.Orientation;
    ar << m_imageMetadata.encodingProcess;
    ar << m_imageMetadata.exifInfo.Whitebalance;
    ar << m_imageMetadata.width;
    ar << m_dateTimeTaken;

    ar << m_imageMetadata.iptcInfo.Author;
    ar << m_imageMetadata.iptcInfo.Byline;
    ar << m_imageMetadata.iptcInfo.BylineTitle;
    ar << m_imageMetadata.iptcInfo.Caption;
    ar << m_imageMetadata.iptcInfo.Category;
    ar << m_imageMetadata.iptcInfo.City;
    ar << m_imageMetadata.iptcInfo.Urgency;
    ar << m_imageMetadata.iptcInfo.CopyrightNotice;
    ar << m_imageMetadata.iptcInfo.Country;
    ar << m_imageMetadata.iptcInfo.CountryCode;
    ar << m_imageMetadata.iptcInfo.Credit;
    ar << m_imageMetadata.iptcInfo.Date;
    ar << m_imageMetadata.iptcInfo.Headline;
    ar << m_imageMetadata.iptcInfo.Keywords;
    ar << m_imageMetadata.iptcInfo.ObjectName;
    ar << m_imageMetadata.iptcInfo.ReferenceService;
    ar << m_imageMetadata.iptcInfo.Source;
    ar << m_imageMetadata.iptcInfo.SpecialInstructions;
    ar << m_imageMetadata.iptcInfo.State;
    ar << m_imageMetadata.iptcInfo.SupplementalCategories;
    ar << m_imageMetadata.iptcInfo.TransmissionReference;
    ar << m_imageMetadata.iptcInfo.TimeCreated;
    ar << m_imageMetadata.iptcInfo.SubLocation;
    ar << m_imageMetadata.iptcInfo.ImageType;
  }
  else
  {
    ar >> m_isLoaded;
    ar >> m_isInfoSetExternally;
    ar >> m_imageMetadata.exifInfo.ApertureFNumber;
    ar >> m_imageMetadata.exifInfo.CameraMake;
    ar >> m_imageMetadata.exifInfo.CameraModel;
    ar >> m_imageMetadata.exifInfo.CCDWidth;
    ar >> m_imageMetadata.exifInfo.Comments;
    ar >> m_imageMetadata.exifInfo.Description;
    ar >> m_imageMetadata.exifInfo.DateTime;
    ar >> m_imageMetadata.exifInfo.DigitalZoomRatio;
    ar >> m_imageMetadata.exifInfo.Distance;
    ar >> m_imageMetadata.exifInfo.ExposureBias;
    ar >> m_imageMetadata.exifInfo.ExposureMode;
    ar >> m_imageMetadata.exifInfo.ExposureProgram;
    ar >> m_imageMetadata.exifInfo.ExposureTime;
    ar >> m_imageMetadata.exifInfo.FlashUsed;
    ar >> m_imageMetadata.exifInfo.FocalLength;
    ar >> m_imageMetadata.exifInfo.FocalLength35mmEquiv;
    ar >> m_imageMetadata.exifInfo.GpsInfoPresent;
    ar >> m_imageMetadata.exifInfo.GpsAlt;
    ar >> m_imageMetadata.exifInfo.GpsLat;
    ar >> m_imageMetadata.exifInfo.GpsLong;
    ar >> m_imageMetadata.height;
    ar >> m_imageMetadata.isColor;
    ar >> m_imageMetadata.exifInfo.ISOequivalent;
    ar >> m_imageMetadata.exifInfo.LightSource;
    ar >> m_imageMetadata.exifInfo.MeteringMode;
    ar >> m_imageMetadata.exifInfo.Orientation;
    ar >> m_imageMetadata.encodingProcess;
    ar >> m_imageMetadata.exifInfo.Whitebalance;
    ar >> m_imageMetadata.width;
    ar >> m_dateTimeTaken;

    ar >> m_imageMetadata.iptcInfo.Author;
    ar >> m_imageMetadata.iptcInfo.Byline;
    ar >> m_imageMetadata.iptcInfo.BylineTitle;
    ar >> m_imageMetadata.iptcInfo.Caption;
    ar >> m_imageMetadata.iptcInfo.Category;
    ar >> m_imageMetadata.iptcInfo.City;
    ar >> m_imageMetadata.iptcInfo.Urgency;
    ar >> m_imageMetadata.iptcInfo.CopyrightNotice;
    ar >> m_imageMetadata.iptcInfo.Country;
    ar >> m_imageMetadata.iptcInfo.CountryCode;
    ar >> m_imageMetadata.iptcInfo.Credit;
    ar >> m_imageMetadata.iptcInfo.Date;
    ar >> m_imageMetadata.iptcInfo.Headline;
    ar >> m_imageMetadata.iptcInfo.Keywords;
    ar >> m_imageMetadata.iptcInfo.ObjectName;
    ar >> m_imageMetadata.iptcInfo.ReferenceService;
    ar >> m_imageMetadata.iptcInfo.Source;
    ar >> m_imageMetadata.iptcInfo.SpecialInstructions;
    ar >> m_imageMetadata.iptcInfo.State;
    ar >> m_imageMetadata.iptcInfo.SupplementalCategories;
    ar >> m_imageMetadata.iptcInfo.TransmissionReference;
    ar >> m_imageMetadata.iptcInfo.TimeCreated;
    ar >> m_imageMetadata.iptcInfo.SubLocation;
    ar >> m_imageMetadata.iptcInfo.ImageType;
  }
}

void CPictureInfoTag::Serialize(CVariant& value) const
{
  value["aperturefnumber"] = m_imageMetadata.exifInfo.ApertureFNumber;
  value["cameramake"] = m_imageMetadata.exifInfo.CameraMake;
  value["cameramodel"] = m_imageMetadata.exifInfo.CameraModel;
  value["ccdwidth"] = m_imageMetadata.exifInfo.CCDWidth;
  value["comments"] = m_imageMetadata.exifInfo.Comments;
  value["description"] = m_imageMetadata.exifInfo.Description;
  value["datetime"] = m_imageMetadata.exifInfo.DateTime;
  value["digitalzoomratio"] = m_imageMetadata.exifInfo.DigitalZoomRatio;
  value["distance"] = m_imageMetadata.exifInfo.Distance;
  value["exposurebias"] = m_imageMetadata.exifInfo.ExposureBias;
  value["exposuremode"] = m_imageMetadata.exifInfo.ExposureMode;
  value["exposureprogram"] = m_imageMetadata.exifInfo.ExposureProgram;
  value["exposuretime"] = m_imageMetadata.exifInfo.ExposureTime;
  value["flashused"] = m_imageMetadata.exifInfo.FlashUsed;
  value["focallength"] = m_imageMetadata.exifInfo.FocalLength;
  value["focallength35mmequiv"] = m_imageMetadata.exifInfo.FocalLength35mmEquiv;
  value["gpsinfopresent"] = m_imageMetadata.exifInfo.GpsInfoPresent;
  value["gpsinfo"]["alt"] = m_imageMetadata.exifInfo.GpsAlt;
  value["gpsinfo"]["lat"] = m_imageMetadata.exifInfo.GpsLat;
  value["gpsinfo"]["long"] = m_imageMetadata.exifInfo.GpsLong;
  value["height"] = m_imageMetadata.height;
  value["iscolor"] = m_imageMetadata.isColor;
  value["isoequivalent"] = m_imageMetadata.exifInfo.ISOequivalent;
  value["lightsource"] = m_imageMetadata.exifInfo.LightSource;
  value["meteringmode"] = m_imageMetadata.exifInfo.MeteringMode;
  value["orientation"] = m_imageMetadata.exifInfo.Orientation;
  value["process"] = m_imageMetadata.encodingProcess;
  value["whitebalance"] = m_imageMetadata.exifInfo.Whitebalance;
  value["width"] = m_imageMetadata.width;

  value["author"] = m_imageMetadata.iptcInfo.Author;
  value["byline"] = m_imageMetadata.iptcInfo.Byline;
  value["bylinetitle"] = m_imageMetadata.iptcInfo.BylineTitle;
  value["caption"] = m_imageMetadata.iptcInfo.Caption;
  value["category"] = m_imageMetadata.iptcInfo.Category;
  value["city"] = m_imageMetadata.iptcInfo.City;
  value["urgency"] = m_imageMetadata.iptcInfo.Urgency;
  value["copyrightnotice"] = m_imageMetadata.iptcInfo.CopyrightNotice;
  value["country"] = m_imageMetadata.iptcInfo.Country;
  value["countrycode"] = m_imageMetadata.iptcInfo.CountryCode;
  value["credit"] = m_imageMetadata.iptcInfo.Credit;
  value["date"] = m_imageMetadata.iptcInfo.Date;
  value["headline"] = m_imageMetadata.iptcInfo.Headline;
  value["keywords"] = m_imageMetadata.iptcInfo.Keywords;
  value["objectname"] = m_imageMetadata.iptcInfo.ObjectName;
  value["referenceservice"] = m_imageMetadata.iptcInfo.ReferenceService;
  value["source"] = m_imageMetadata.iptcInfo.Source;
  value["specialinstructions"] = m_imageMetadata.iptcInfo.SpecialInstructions;
  value["state"] = m_imageMetadata.iptcInfo.State;
  value["supplementalcategories"] = m_imageMetadata.iptcInfo.SupplementalCategories;
  value["transmissionreference"] = m_imageMetadata.iptcInfo.TransmissionReference;
  value["timecreated"] = m_imageMetadata.iptcInfo.TimeCreated;
  value["sublocation"] = m_imageMetadata.iptcInfo.SubLocation;
  value["imagetype"] = m_imageMetadata.iptcInfo.ImageType;
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
    value = StringUtils::Format("{} x {}", m_imageMetadata.width, m_imageMetadata.height);
    break;
  case SLIDESHOW_COLOUR:
    value = m_imageMetadata.isColor ? "Colour" : "Black and White";
    break;
  case SLIDESHOW_PROCESS:
  {
    auto process = m_imageMetadata.encodingProcess;
    value = !process.empty() ? process : "Unknown";
    break;
  }
  case SLIDESHOW_COMMENT:
    value = m_imageMetadata.fileComment;
    break;
  case SLIDESHOW_EXIF_COMMENT:
    value = m_imageMetadata.exifInfo.Comments;
    break;
  case SLIDESHOW_EXIF_XPCOMMENT:
    value = m_imageMetadata.exifInfo.XPComment;
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
    value = m_imageMetadata.exifInfo.Description;
    break;
  case SLIDESHOW_EXIF_CAMERA_MAKE:
    value = m_imageMetadata.exifInfo.CameraMake;
    break;
  case SLIDESHOW_EXIF_CAMERA_MODEL:
    value = m_imageMetadata.exifInfo.CameraModel;
    break;
  case SLIDESHOW_EXIF_APERTURE:
    if (m_imageMetadata.exifInfo.ApertureFNumber)
      value = StringUtils::Format("{:3.1f}", m_imageMetadata.exifInfo.ApertureFNumber);
    break;
  case SLIDESHOW_EXIF_ORIENTATION:
    switch (m_imageMetadata.exifInfo.Orientation)
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
    if (m_imageMetadata.exifInfo.FocalLength)
    {
      value = StringUtils::Format("{:4.2f}mm", m_imageMetadata.exifInfo.FocalLength);
      if (m_imageMetadata.exifInfo.FocalLength35mmEquiv != 0)
        value += StringUtils::Format("  (35mm Equivalent = {}mm)",
                                     m_imageMetadata.exifInfo.FocalLength35mmEquiv);
    }
    break;
  case SLIDESHOW_EXIF_FOCUS_DIST:
    if (m_imageMetadata.exifInfo.Distance < 0)
      value = "Infinite";
    else if (m_imageMetadata.exifInfo.Distance > 0)
      value = StringUtils::Format("{:4.2f}m", m_imageMetadata.exifInfo.Distance);
    break;
  case SLIDESHOW_EXIF_EXPOSURE:
    switch (m_imageMetadata.exifInfo.ExposureProgram)
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
    if (m_imageMetadata.exifInfo.ExposureTime)
    {
      if (m_imageMetadata.exifInfo.ExposureTime < 0.010f)
        value = StringUtils::Format("{:6.4f}s", m_imageMetadata.exifInfo.ExposureTime);
      else
        value = StringUtils::Format("{:5.3f}s", m_imageMetadata.exifInfo.ExposureTime);
      if (m_imageMetadata.exifInfo.ExposureTime <= 0.5f)
        value += StringUtils::Format(
            " (1/{})", static_cast<int>(0.5f + 1 / m_imageMetadata.exifInfo.ExposureTime));
    }
    break;
  case SLIDESHOW_EXIF_EXPOSURE_BIAS:
    if (m_imageMetadata.exifInfo.ExposureBias != 0)
      value = StringUtils::Format("{:4.2f} EV", m_imageMetadata.exifInfo.ExposureBias);
    break;
  case SLIDESHOW_EXIF_EXPOSURE_MODE:
    switch (m_imageMetadata.exifInfo.ExposureMode)
    {
      case 0:  value = "Automatic";          break;
      case 1:  value = "Manual";             break;
      case 2:  value = "Auto bracketing";    break;
    }
    break;
  case SLIDESHOW_EXIF_FLASH_USED:
    if (m_imageMetadata.exifInfo.FlashUsed >= 0)
    {
      if (m_imageMetadata.exifInfo.FlashUsed & 1)
      {
        value = "Yes";
        switch (m_imageMetadata.exifInfo.FlashUsed)
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
        value = m_imageMetadata.exifInfo.FlashUsed == 0x18 ? "No (Auto)" : "No";
    }
    break;
  case SLIDESHOW_EXIF_WHITE_BALANCE:
    return m_imageMetadata.exifInfo.Whitebalance ? "Manual" : "Auto";
  case SLIDESHOW_EXIF_LIGHT_SOURCE:
    switch (m_imageMetadata.exifInfo.LightSource)
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
    switch (m_imageMetadata.exifInfo.MeteringMode)
    {
      case 2:  value = "Center weight"; break;
      case 3:  value = "Spot";   break;
      case 5:  value = "Matrix"; break;
    }
    break;
  case SLIDESHOW_EXIF_ISO_EQUIV:
    if (m_imageMetadata.exifInfo.ISOequivalent)
      value = StringUtils::Format("{:2}", m_imageMetadata.exifInfo.ISOequivalent);
    break;
  case SLIDESHOW_EXIF_DIGITAL_ZOOM:
    if (m_imageMetadata.exifInfo.DigitalZoomRatio)
      value = StringUtils::Format("{:1.3f}x", m_imageMetadata.exifInfo.DigitalZoomRatio);
    break;
  case SLIDESHOW_EXIF_CCD_WIDTH:
    if (m_imageMetadata.exifInfo.CCDWidth)
      value = StringUtils::Format("{:4.2f}mm", m_imageMetadata.exifInfo.CCDWidth);
    break;
  case SLIDESHOW_EXIF_GPS_LATITUDE:
    value = m_imageMetadata.exifInfo.GpsLat;
    break;
  case SLIDESHOW_EXIF_GPS_LONGITUDE:
    value = m_imageMetadata.exifInfo.GpsLong;
    break;
  case SLIDESHOW_EXIF_GPS_ALTITUDE:
    value = m_imageMetadata.exifInfo.GpsAlt;
    break;
  case SLIDESHOW_IPTC_SUP_CATEGORIES:
    value = m_imageMetadata.iptcInfo.SupplementalCategories;
    break;
  case SLIDESHOW_IPTC_KEYWORDS:
    value = m_imageMetadata.iptcInfo.Keywords;
    break;
  case SLIDESHOW_IPTC_CAPTION:
    value = m_imageMetadata.iptcInfo.Caption;
    break;
  case SLIDESHOW_IPTC_AUTHOR:
    value = m_imageMetadata.iptcInfo.Author;
    break;
  case SLIDESHOW_IPTC_HEADLINE:
    value = m_imageMetadata.iptcInfo.Headline;
    break;
  case SLIDESHOW_IPTC_SPEC_INSTR:
    value = m_imageMetadata.iptcInfo.SpecialInstructions;
    break;
  case SLIDESHOW_IPTC_CATEGORY:
    value = m_imageMetadata.iptcInfo.Category;
    break;
  case SLIDESHOW_IPTC_BYLINE:
    value = m_imageMetadata.iptcInfo.Byline;
    break;
  case SLIDESHOW_IPTC_BYLINE_TITLE:
    value = m_imageMetadata.iptcInfo.BylineTitle;
    break;
  case SLIDESHOW_IPTC_CREDIT:
    value = m_imageMetadata.iptcInfo.Credit;
    break;
  case SLIDESHOW_IPTC_SOURCE:
    value = m_imageMetadata.iptcInfo.Source;
    break;
  case SLIDESHOW_IPTC_COPYRIGHT_NOTICE:
    value = m_imageMetadata.iptcInfo.CopyrightNotice;
    break;
  case SLIDESHOW_IPTC_OBJECT_NAME:
    value = m_imageMetadata.iptcInfo.ObjectName;
    break;
  case SLIDESHOW_IPTC_CITY:
    value = m_imageMetadata.iptcInfo.City;
    break;
  case SLIDESHOW_IPTC_STATE:
    value = m_imageMetadata.iptcInfo.State;
    break;
  case SLIDESHOW_IPTC_COUNTRY:
    value = m_imageMetadata.iptcInfo.Country;
    break;
  case SLIDESHOW_IPTC_TX_REFERENCE:
    value = m_imageMetadata.iptcInfo.TransmissionReference;
    break;
  case SLIDESHOW_IPTC_DATE:
    value = m_imageMetadata.iptcInfo.Date;
    break;
  case SLIDESHOW_IPTC_URGENCY:
    value = m_imageMetadata.iptcInfo.Urgency;
    break;
  case SLIDESHOW_IPTC_COUNTRY_CODE:
    value = m_imageMetadata.iptcInfo.CountryCode;
    break;
  case SLIDESHOW_IPTC_REF_SERVICE:
    value = m_imageMetadata.iptcInfo.ReferenceService;
    break;
  case SLIDESHOW_IPTC_TIMECREATED:
    value = m_imageMetadata.iptcInfo.TimeCreated;
    break;
  case SLIDESHOW_IPTC_SUBLOCATION:
    value = m_imageMetadata.iptcInfo.SubLocation;
    break;
  case SLIDESHOW_IPTC_IMAGETYPE:
    value = m_imageMetadata.iptcInfo.ImageType;
    break;
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
  else if (StringUtils::EqualsNoCase(info, "exifcomment"))
    return SLIDESHOW_EXIF_COMMENT;
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
        m_imageMetadata.width = std::atoi(dimension[0].c_str());
        m_imageMetadata.height = std::atoi(dimension[1].c_str());
        m_isInfoSetExternally =
            true; // Set the internal state to show metadata has been set by call to SetInfo
      }
      break;
    }
  case SLIDESHOW_EXIF_DATE_TIME:
    {
      m_imageMetadata.exifInfo.DateTime = value;
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
  const std::string& dateTime = m_imageMetadata.exifInfo.DateTime;
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
