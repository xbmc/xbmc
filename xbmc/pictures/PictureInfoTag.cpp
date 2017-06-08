/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include <cstdlib>
#include <algorithm>

#include "PictureInfoTag.h"
#include "XBDateTime.h"
#include "utils/Variant.h"
#include "utils/CharsetConverter.h"
#include "utils/StringUtils.h"
#include "utils/Archive.h"

void CPictureInfoTag::Reset()
{
  memset(&m_exifInfo, 0, sizeof(m_exifInfo));
  memset(&m_iptcInfo, 0, sizeof(m_iptcInfo));
  m_isLoaded = false;
  m_isInfoSetExternally = false;
  m_dateTimeTaken.Reset();
}

const CPictureInfoTag& CPictureInfoTag::operator=(const CPictureInfoTag& right)
{
  if (this == &right) return * this;
  memcpy(&m_exifInfo, &right.m_exifInfo, sizeof(m_exifInfo));
  memcpy(&m_iptcInfo, &right.m_iptcInfo, sizeof(m_iptcInfo));
  m_isLoaded = right.m_isLoaded;
  m_isInfoSetExternally = right.m_isInfoSetExternally;
  m_dateTimeTaken = right.m_dateTimeTaken;
  return *this;
}

bool CPictureInfoTag::Load(const std::string &path)
{
  m_isLoaded = false;

  if (process_jpeg(path.c_str(), &m_exifInfo, &m_iptcInfo))
    m_isLoaded = true;

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
    ar << std::string(m_exifInfo.CameraMake);
    ar << std::string(m_exifInfo.CameraModel);
    ar << m_exifInfo.CCDWidth;
    ar << GetInfo(SLIDE_EXIF_COMMENT); // Store and restore the comment charset converted
    ar << std::string(m_exifInfo.Description);
    ar << std::string(m_exifInfo.DateTime);
    for (int i = 0; i < 10; i++)
      ar << m_exifInfo.DateTimeOffsets[i];
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
    ar << std::string(m_exifInfo.GpsAlt);
    ar << std::string(m_exifInfo.GpsLat);
    ar << std::string(m_exifInfo.GpsLong);
    ar << m_exifInfo.Height;
    ar << m_exifInfo.IsColor;
    ar << m_exifInfo.ISOequivalent;
    ar << m_exifInfo.LargestExifOffset;
    ar << m_exifInfo.LightSource;
    ar << m_exifInfo.MeteringMode;
    ar << m_exifInfo.numDateTimeTags;
    ar << m_exifInfo.Orientation;
    ar << m_exifInfo.Process;
    ar << m_exifInfo.ThumbnailAtEnd;
    ar << m_exifInfo.ThumbnailOffset;
    ar << m_exifInfo.ThumbnailSize;
    ar << m_exifInfo.ThumbnailSizeOffset;
    ar << m_exifInfo.Whitebalance;
    ar << m_exifInfo.Width;

    // m_dateTimeTaken is private and we can't pass a ref to a private object
    CDateTime tmp(m_dateTimeTaken);
    ar << tmp;

    ar << std::string(m_iptcInfo.Author);
    ar << std::string(m_iptcInfo.Byline);
    ar << std::string(m_iptcInfo.BylineTitle);
    ar << std::string(m_iptcInfo.Caption);
    ar << std::string(m_iptcInfo.Category);
    ar << std::string(m_iptcInfo.City);
    ar << std::string(m_iptcInfo.Urgency);
    ar << std::string(m_iptcInfo.CopyrightNotice);
    ar << std::string(m_iptcInfo.Country);
    ar << std::string(m_iptcInfo.CountryCode);
    ar << std::string(m_iptcInfo.Credit);
    ar << std::string(m_iptcInfo.Date);
    ar << std::string(m_iptcInfo.Headline);
    ar << std::string(m_iptcInfo.Keywords);
    ar << std::string(m_iptcInfo.ObjectName);
    ar << std::string(m_iptcInfo.ReferenceService);
    ar << std::string(m_iptcInfo.Source);
    ar << std::string(m_iptcInfo.SpecialInstructions);
    ar << std::string(m_iptcInfo.State);
    ar << std::string(m_iptcInfo.SupplementalCategories);
    ar << std::string(m_iptcInfo.TransmissionReference);
    ar << std::string(m_iptcInfo.TimeCreated);
    ar << std::string(m_iptcInfo.SubLocation);
    ar << std::string(m_iptcInfo.ImageType);
  }
  else
  {
    ar >> m_isLoaded;
    ar >> m_isInfoSetExternally;
    ar >> m_exifInfo.ApertureFNumber;
    GetStringFromArchive(ar, m_exifInfo.CameraMake, sizeof(m_exifInfo.CameraMake));
    GetStringFromArchive(ar, m_exifInfo.CameraModel, sizeof(m_exifInfo.CameraModel));
    ar >> m_exifInfo.CCDWidth;
    GetStringFromArchive(ar, m_exifInfo.Comments, sizeof(m_exifInfo.Comments));
    m_exifInfo.CommentsCharset = EXIF_COMMENT_CHARSET_CONVERTED; // Store and restore the comment charset converted
    GetStringFromArchive(ar, m_exifInfo.Description, sizeof(m_exifInfo.Description));
    GetStringFromArchive(ar, m_exifInfo.DateTime, sizeof(m_exifInfo.DateTime));
    for (int i = 0; i < 10; i++)
      ar >> m_exifInfo.DateTimeOffsets[i];
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
    GetStringFromArchive(ar, m_exifInfo.GpsAlt, sizeof(m_exifInfo.GpsAlt));
    GetStringFromArchive(ar, m_exifInfo.GpsLat, sizeof(m_exifInfo.GpsLat));
    GetStringFromArchive(ar, m_exifInfo.GpsLong, sizeof(m_exifInfo.GpsLong));
    ar >> m_exifInfo.Height;
    ar >> m_exifInfo.IsColor;
    ar >> m_exifInfo.ISOequivalent;
    ar >> m_exifInfo.LargestExifOffset;
    ar >> m_exifInfo.LightSource;
    ar >> m_exifInfo.MeteringMode;
    ar >> m_exifInfo.numDateTimeTags;
    ar >> m_exifInfo.Orientation;
    ar >> m_exifInfo.Process;
    ar >> m_exifInfo.ThumbnailAtEnd;
    ar >> m_exifInfo.ThumbnailOffset;
    ar >> m_exifInfo.ThumbnailSize;
    ar >> m_exifInfo.ThumbnailSizeOffset;
    ar >> m_exifInfo.Whitebalance;
    ar >> m_exifInfo.Width;
    ar >> m_dateTimeTaken;

    GetStringFromArchive(ar, m_iptcInfo.Author, sizeof(m_iptcInfo.Author));
    GetStringFromArchive(ar, m_iptcInfo.Byline, sizeof(m_iptcInfo.Byline));
    GetStringFromArchive(ar, m_iptcInfo.BylineTitle, sizeof(m_iptcInfo.BylineTitle));
    GetStringFromArchive(ar, m_iptcInfo.Caption, sizeof(m_iptcInfo.Caption));
    GetStringFromArchive(ar, m_iptcInfo.Category, sizeof(m_iptcInfo.Category));
    GetStringFromArchive(ar, m_iptcInfo.City, sizeof(m_iptcInfo.City));
    GetStringFromArchive(ar, m_iptcInfo.Urgency, sizeof(m_iptcInfo.Urgency));
    GetStringFromArchive(ar, m_iptcInfo.CopyrightNotice, sizeof(m_iptcInfo.CopyrightNotice));
    GetStringFromArchive(ar, m_iptcInfo.Country, sizeof(m_iptcInfo.Country));
    GetStringFromArchive(ar, m_iptcInfo.CountryCode, sizeof(m_iptcInfo.CountryCode));
    GetStringFromArchive(ar, m_iptcInfo.Credit, sizeof(m_iptcInfo.Credit));
    GetStringFromArchive(ar, m_iptcInfo.Date, sizeof(m_iptcInfo.Date));
    GetStringFromArchive(ar, m_iptcInfo.Headline, sizeof(m_iptcInfo.Headline));
    GetStringFromArchive(ar, m_iptcInfo.Keywords, sizeof(m_iptcInfo.Keywords));
    GetStringFromArchive(ar, m_iptcInfo.ObjectName, sizeof(m_iptcInfo.ObjectName));
    GetStringFromArchive(ar, m_iptcInfo.ReferenceService, sizeof(m_iptcInfo.ReferenceService));
    GetStringFromArchive(ar, m_iptcInfo.Source, sizeof(m_iptcInfo.Source));
    GetStringFromArchive(ar, m_iptcInfo.SpecialInstructions, sizeof(m_iptcInfo.SpecialInstructions));
    GetStringFromArchive(ar, m_iptcInfo.State, sizeof(m_iptcInfo.State));
    GetStringFromArchive(ar, m_iptcInfo.SupplementalCategories, sizeof(m_iptcInfo.SupplementalCategories));
    GetStringFromArchive(ar, m_iptcInfo.TransmissionReference, sizeof(m_iptcInfo.TransmissionReference));
    GetStringFromArchive(ar, m_iptcInfo.TimeCreated, sizeof(m_iptcInfo.TimeCreated));
    GetStringFromArchive(ar, m_iptcInfo.SubLocation, sizeof(m_iptcInfo.SubLocation));
    GetStringFromArchive(ar, m_iptcInfo.ImageType, sizeof(m_iptcInfo.ImageType));
  }
}

void CPictureInfoTag::Serialize(CVariant& value) const
{
  value["aperturefnumber"] = m_exifInfo.ApertureFNumber;
  value["cameramake"] = std::string(m_exifInfo.CameraMake);
  value["cameramodel"] = std::string(m_exifInfo.CameraModel);
  value["ccdwidth"] = m_exifInfo.CCDWidth;
  value["comments"] = GetInfo(SLIDE_EXIF_COMMENT); // Charset conversion
  value["description"] = std::string(m_exifInfo.Description);
  value["datetime"] = std::string(m_exifInfo.DateTime);
  for (int i = 0; i < 10; i++)
    value["datetimeoffsets"][i] = m_exifInfo.DateTimeOffsets[i];
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
  value["gpsinfo"]["alt"] = std::string(m_exifInfo.GpsAlt);
  value["gpsinfo"]["lat"] = std::string(m_exifInfo.GpsLat);
  value["gpsinfo"]["long"] = std::string(m_exifInfo.GpsLong);
  value["height"] = m_exifInfo.Height;
  value["iscolor"] = m_exifInfo.IsColor;
  value["isoequivalent"] = m_exifInfo.ISOequivalent;
  value["largestexifoffset"] = m_exifInfo.LargestExifOffset;
  value["lightsource"] = m_exifInfo.LightSource;
  value["meteringmode"] = m_exifInfo.MeteringMode;
  value["numdatetimetags"] = m_exifInfo.numDateTimeTags;
  value["orientation"] = m_exifInfo.Orientation;
  value["process"] = m_exifInfo.Process;
  value["thumbnailatend"] = m_exifInfo.ThumbnailAtEnd;
  value["thumbnailoffset"] = m_exifInfo.ThumbnailOffset;
  value["thumbnailsize"] = m_exifInfo.ThumbnailSize;
  value["thumbnailsizeoffset"] = m_exifInfo.ThumbnailSizeOffset;
  value["whitebalance"] = m_exifInfo.Whitebalance;
  value["width"] = m_exifInfo.Width;

  value["author"] = std::string(m_iptcInfo.Author);
  value["byline"] = std::string(m_iptcInfo.Byline);
  value["bylinetitle"] = std::string(m_iptcInfo.BylineTitle);
  value["caption"] = std::string(m_iptcInfo.Caption);
  value["category"] = std::string(m_iptcInfo.Category);
  value["city"] = std::string(m_iptcInfo.City);
  value["urgency"] = std::string(m_iptcInfo.Urgency);
  value["copyrightnotice"] = std::string(m_iptcInfo.CopyrightNotice);
  value["country"] = std::string(m_iptcInfo.Country);
  value["countrycode"] = std::string(m_iptcInfo.CountryCode);
  value["credit"] = std::string(m_iptcInfo.Credit);
  value["date"] = std::string(m_iptcInfo.Date);
  value["headline"] = std::string(m_iptcInfo.Headline);
  value["keywords"] = std::string(m_iptcInfo.Keywords);
  value["objectname"] = std::string(m_iptcInfo.ObjectName);
  value["referenceservice"] = std::string(m_iptcInfo.ReferenceService);
  value["source"] = std::string(m_iptcInfo.Source);
  value["specialinstructions"] = std::string(m_iptcInfo.SpecialInstructions);
  value["state"] = std::string(m_iptcInfo.State);
  value["supplementalcategories"] = std::string(m_iptcInfo.SupplementalCategories);
  value["transmissionreference"] = std::string(m_iptcInfo.TransmissionReference);
  value["timecreated"] = std::string(m_iptcInfo.TimeCreated);
  value["sublocation"] = std::string(m_iptcInfo.SubLocation);
  value["imagetype"] = std::string(m_iptcInfo.ImageType);
}

void CPictureInfoTag::ToSortable(SortItem& sortable, Field field) const
{
  if (field == FieldDateTaken && m_dateTimeTaken.IsValid())
    sortable[FieldDateTaken] = m_dateTimeTaken.GetAsDBDateTime();
}

void CPictureInfoTag::GetStringFromArchive(CArchive &ar, char *string, size_t length)
{
  std::string temp;
  ar >> temp;
  length = std::min((size_t)temp.size(), length - 1);
  if (!temp.empty())
    memcpy(string, temp.c_str(), length);
  string[length] = 0;
}

const std::string CPictureInfoTag::GetInfo(int info) const
{
  if (!m_isLoaded && !m_isInfoSetExternally) // If no metadata has been loaded from the picture file or set with SetInfo(), just return
    return "";

  std::string value;
  switch (info)
  {
  case SLIDE_RESOLUTION:
    value = StringUtils::Format("%d x %d", m_exifInfo.Width, m_exifInfo.Height);
    break;
  case SLIDE_COLOUR:
    value = m_exifInfo.IsColor ? "Colour" : "Black and White";
    break;
  case SLIDE_PROCESS:
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
  case SLIDE_COMMENT:
    g_charsetConverter.unknownToUTF8(m_exifInfo.FileComment, value);
    break;
  case SLIDE_EXIF_COMMENT:
    // The charset used for the UserComment is stored in CommentsCharset:
    // Ascii, Unicode (UCS2), JIS (X208-1990), Unknown (application specific)
    if (m_exifInfo.CommentsCharset == EXIF_COMMENT_CHARSET_UNICODE)
    {
      g_charsetConverter.ucs2ToUTF8(std::u16string((char16_t*)m_exifInfo.Comments), value);
    }
    else
    {
      // Ascii doesn't need to be converted (EXIF_COMMENT_CHARSET_ASCII)
      // Archived data is already converted (EXIF_COMMENT_CHARSET_CONVERTED)
      // Unknown data can't be converted as it could be any codec (EXIF_COMMENT_CHARSET_UNKNOWN)
      // JIS data can't be converted as CharsetConverter and iconv lacks support (EXIF_COMMENT_CHARSET_JIS)
      g_charsetConverter.unknownToUTF8(m_exifInfo.Comments, value);
    }
    break;
  case SLIDE_EXIF_XPCOMMENT:
    if (m_exifInfo.XPCommentsCharset == EXIF_COMMENT_CHARSET_UNICODE)
    {
      g_charsetConverter.ucs2ToUTF8(std::u16string((char16_t*)m_exifInfo.XPComment), value);
    }
    else
    {
      value = "Illegal charset used.";
    }
    break;
  case SLIDE_EXIF_LONG_DATE_TIME:
    if (m_dateTimeTaken.IsValid())
      value = m_dateTimeTaken.GetAsLocalizedDateTime(true);
    break;
  case SLIDE_EXIF_DATE_TIME:
    if (m_dateTimeTaken.IsValid())
      value = m_dateTimeTaken.GetAsLocalizedDateTime();
    break;
  case SLIDE_EXIF_LONG_DATE:
    if (m_dateTimeTaken.IsValid())
      value = m_dateTimeTaken.GetAsLocalizedDate(true);
    break;
  case SLIDE_EXIF_DATE:
    if (m_dateTimeTaken.IsValid())
      value = m_dateTimeTaken.GetAsLocalizedDate();
    break;
  case SLIDE_EXIF_DESCRIPTION:
    value = m_exifInfo.Description;
    break;
  case SLIDE_EXIF_CAMERA_MAKE:
    value = m_exifInfo.CameraMake;
    break;
  case SLIDE_EXIF_CAMERA_MODEL:
    value = m_exifInfo.CameraModel;
    break;
//  case SLIDE_EXIF_SOFTWARE:
//    value = m_exifInfo.Software;
  case SLIDE_EXIF_APERTURE:
    if (m_exifInfo.ApertureFNumber)
      value = StringUtils::Format("%3.1f", m_exifInfo.ApertureFNumber);
    break;
  case SLIDE_EXIF_ORIENTATION:
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
  case SLIDE_EXIF_FOCAL_LENGTH:
    if (m_exifInfo.FocalLength)
    {
      value = StringUtils::Format("%4.2fmm", m_exifInfo.FocalLength);
      if (m_exifInfo.FocalLength35mmEquiv != 0)
        value += StringUtils::Format("  (35mm Equivalent = %umm)", m_exifInfo.FocalLength35mmEquiv);
    }
    break;
  case SLIDE_EXIF_FOCUS_DIST:
    if (m_exifInfo.Distance < 0)
      value = "Infinite";
    else if (m_exifInfo.Distance > 0)
      value = StringUtils::Format("%4.2fm", m_exifInfo.Distance);
    break;
  case SLIDE_EXIF_EXPOSURE:
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
  case SLIDE_EXIF_EXPOSURE_TIME:
    if (m_exifInfo.ExposureTime)
    {
      if (m_exifInfo.ExposureTime < 0.010f)
        value = StringUtils::Format("%6.4fs", m_exifInfo.ExposureTime);
      else
        value = StringUtils::Format("%5.3fs", m_exifInfo.ExposureTime);
      if (m_exifInfo.ExposureTime <= 0.5)
        value += StringUtils::Format(" (1/%d)", (int)(0.5 + 1/m_exifInfo.ExposureTime));
    }
    break;
  case SLIDE_EXIF_EXPOSURE_BIAS:
    if (m_exifInfo.ExposureBias != 0)
      value = StringUtils::Format("%4.2f EV", m_exifInfo.ExposureBias);
    break;
  case SLIDE_EXIF_EXPOSURE_MODE:
    switch (m_exifInfo.ExposureMode)
    {
      case 0:  value = "Automatic";          break;
      case 1:  value = "Manual";             break;
      case 2:  value = "Auto bracketing";    break;
    }
    break;
  case SLIDE_EXIF_FLASH_USED:
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
  case SLIDE_EXIF_WHITE_BALANCE:
    return m_exifInfo.Whitebalance ? "Manual" : "Auto";
  case SLIDE_EXIF_LIGHT_SOURCE:
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
  case SLIDE_EXIF_METERING_MODE:
    switch (m_exifInfo.MeteringMode)
    {
      case 2:  value = "Center weight"; break;
      case 3:  value = "Spot";   break;
      case 5:  value = "Matrix"; break;
    }
    break;
  case SLIDE_EXIF_ISO_EQUIV:
    if (m_exifInfo.ISOequivalent)
      value = StringUtils::Format("%2d", m_exifInfo.ISOequivalent);
    break;
  case SLIDE_EXIF_DIGITAL_ZOOM:
    if (m_exifInfo.DigitalZoomRatio)
      value = StringUtils::Format("%1.3fx", m_exifInfo.DigitalZoomRatio);
    break;
  case SLIDE_EXIF_CCD_WIDTH:
    if (m_exifInfo.CCDWidth)
      value = StringUtils::Format("%4.2fmm", m_exifInfo.CCDWidth);
    break;
  case SLIDE_EXIF_GPS_LATITUDE:
    value = m_exifInfo.GpsLat;
    break;
  case SLIDE_EXIF_GPS_LONGITUDE:
    value = m_exifInfo.GpsLong;
    break;
  case SLIDE_EXIF_GPS_ALTITUDE:
    value = m_exifInfo.GpsAlt;
    break;
  case SLIDE_IPTC_SUP_CATEGORIES:   value = m_iptcInfo.SupplementalCategories;  break;
  case SLIDE_IPTC_KEYWORDS:         value = m_iptcInfo.Keywords;                break;
  case SLIDE_IPTC_CAPTION:          value = m_iptcInfo.Caption;                 break;
  case SLIDE_IPTC_AUTHOR:           value = m_iptcInfo.Author;                  break;
  case SLIDE_IPTC_HEADLINE:         value = m_iptcInfo.Headline;                break;
  case SLIDE_IPTC_SPEC_INSTR:       value = m_iptcInfo.SpecialInstructions;     break;
  case SLIDE_IPTC_CATEGORY:         value = m_iptcInfo.Category;                break;
  case SLIDE_IPTC_BYLINE:           value = m_iptcInfo.Byline;                  break;
  case SLIDE_IPTC_BYLINE_TITLE:     value = m_iptcInfo.BylineTitle;             break;
  case SLIDE_IPTC_CREDIT:           value = m_iptcInfo.Credit;                  break;
  case SLIDE_IPTC_SOURCE:           value = m_iptcInfo.Source;                  break;
  case SLIDE_IPTC_COPYRIGHT_NOTICE: value = m_iptcInfo.CopyrightNotice;         break;
  case SLIDE_IPTC_OBJECT_NAME:      value = m_iptcInfo.ObjectName;              break;
  case SLIDE_IPTC_CITY:             value = m_iptcInfo.City;                    break;
  case SLIDE_IPTC_STATE:            value = m_iptcInfo.State;                   break;
  case SLIDE_IPTC_COUNTRY:          value = m_iptcInfo.Country;                 break;
  case SLIDE_IPTC_TX_REFERENCE:     value = m_iptcInfo.TransmissionReference;   break;
  case SLIDE_IPTC_DATE:             value = m_iptcInfo.Date;                    break;
  case SLIDE_IPTC_URGENCY:          value = m_iptcInfo.Urgency;                 break;
  case SLIDE_IPTC_COUNTRY_CODE:     value = m_iptcInfo.CountryCode;             break;
  case SLIDE_IPTC_REF_SERVICE:      value = m_iptcInfo.ReferenceService;        break;
  case SLIDE_IPTC_TIMECREATED:      value = m_iptcInfo.TimeCreated;             break;
  case SLIDE_IPTC_SUBLOCATION:      value = m_iptcInfo.SubLocation;             break;
  case SLIDE_IPTC_IMAGETYPE:        value = m_iptcInfo.ImageType;               break;
  default:
    break;
  }
  return value;
}

/// \page modules__General__List_of_gui_access
/// @{
/// \table_start
///   \table_row3{   <b>`Slideshow.Altitude`</b>,
///                  \anchor Slideshow_Altitude
///                  _string_,
///     Shows the altitude in meters where the current picture was taken. This
///     is the value of the EXIF GPSInfo.GPSAltitude tag.
///   }
///   \table_row3{   <b>`Slideshow.Aperture`</b>,
///                  \anchor Slideshow_Aperture
///                  _string_,
///     Shows the F-stop used to take the current picture. This is the value of
///     the EXIF FNumber tag (hex code 0x829D).
///   }
///   \table_row3{   <b>`Slideshow.Author`</b>,
///                  \anchor Slideshow_Author
///                  _string_,
///     Shows the name of the person involved in writing about the current
///     picture. This is the value of the IPTC Writer tag (hex code 0x7A).
///   }
///   \table_row3{   <b>`Slideshow.Byline`</b>,
///                  \anchor Slideshow_Byline
///                  _string_,
///     Shows the name of the person who created the current picture. This is
///     the value of the IPTC Byline tag (hex code 0x50).
///   }
///   \table_row3{   <b>`Slideshow.BylineTitle`</b>,
///                  \anchor Slideshow_BylineTitle
///                  _string_,
///     Shows the title of the person who created the current picture. This is
///     the value of the IPTC BylineTitle tag (hex code 0x55).
///   }
///   \table_row3{   <b>`Slideshow.CameraMake`</b>,
///                  \anchor Slideshow_CameraMake
///                  _string_,
///     Shows the manufacturer of the camera used to take the current picture.
///     This is the value of the EXIF Make tag (hex code 0x010F).
///   }
///   \table_row3{   <b>`Slideshow.CameraModel`</b>,
///                  \anchor Slideshow_CameraModel
///                  _string_,
///     Shows the manufacturer's model name or number of the camera used to take
///     the current picture. This is the value of the EXIF Model tag (hex code
///     0x0110).
///   }
///   \table_row3{   <b>`Slideshow.Caption`</b>,
///                  \anchor Slideshow_Caption
///                  _string_,
///     Shows a description of the current picture. This is the value of the
///     IPTC Caption tag (hex code 0x78).
///   }
///   \table_row3{   <b>`Slideshow.Category`</b>,
///                  \anchor Slideshow_Category
///                  _string_,
///     Shows the subject of the current picture as a category code. This is the
///     value of the IPTC Category tag (hex code 0x0F).
///   }
///   \table_row3{   <b>`Slideshow.CCDWidth`</b>,
///                  \anchor Slideshow_CCDWidth
///                  _string_,
///     Shows the width of the CCD in the camera used to take the current
///     picture. This is calculated from three EXIF tags (0xA002 * 0xA210 / 0xA20e).
///   }
///   \table_row3{   <b>`Slideshow.City`</b>,
///                  \anchor Slideshow_City
///                  _string_,
///     Shows the city where the current picture was taken. This is the value of
///     the IPTC City tag (hex code 0x5A).
///   }
///   \table_row3{   <b>`Slideshow.Colour`</b>,
///                  \anchor Slideshow_Colour
///                  _string_,
///     Shows whether the current picture is "Colour" or "Black and White".
///   }
///   \table_row3{   <b>`Slideshow.CopyrightNotice`</b>,
///                  \anchor Slideshow_CopyrightNotice
///                  _string_,
///     Shows the copyright notice of the current picture. This is the value of
///     the IPTC Copyright tag (hex code 0x74).
///   }
///   \table_row3{   <b>`Slideshow.Country`</b>,
///                  \anchor Slideshow_Country
///                  _string_,
///     Shows the full name of the country where the current picture was taken.
///     This is the value of the IPTC CountryName tag (hex code 0x65).
///   }
///   \table_row3{   <b>`Slideshow.CountryCode`</b>,
///                  \anchor Slideshow_CountryCode
///                  _string_,
///     Shows the country code of the country where the current picture was
///     taken. This is the value of the IPTC CountryCode tag (hex code 0x64).
///   }
///   \table_row3{   <b>`Slideshow.Credit`</b>,
///                  \anchor Slideshow_Credit
///                  _string_,
///     Shows who provided the current picture. This is the value of the IPTC
///     Credit tag (hex code 0x6E).
///   }
///   \table_row3{   <b>`Slideshow.DigitalZoom`</b>,
///                  \anchor Slideshow_DigitalZoom
///                  _string_,
///     Shows the digital zoom ratio when the current picture was taken. This is
///     the value of the EXIF .DigitalZoomRatio tag (hex code 0xA404).
///   }
///   \table_row3{   <b>`Slideshow.EXIFComment`</b>,
///                  \anchor Slideshow_EXIFComment
///                  _string_,
///     Shows a description of the current picture. This is the value of the
///     EXIF User Comment tag (hex code 0x9286). This is the same value as
///     Slideshow.SlideComment.
///   }
///   \table_row3{   <b>`Slideshow.EXIFDate`</b>,
///                  \anchor Slideshow_EXIFDate
///                  _string_,
///     Shows the localized date of the current picture. The short form of the
///     date is used. The value of the EXIF DateTimeOriginal tag (hex code
///     0x9003) is preferred. If the DateTimeOriginal tag is not found\, the
///     value of DateTimeDigitized (hex code 0x9004) or of DateTime (hex code
///     0x0132) might be used.
///   }
///   \table_row3{   <b>`Slideshow.EXIFDescription`</b>,
///                  \anchor Slideshow_EXIFDescription
///                  _string_,
///     Shows a short description of the current picture. The SlideComment\,
///     EXIFComment or Caption values might contain a longer description. This
///     is the value of the EXIF ImageDescription tag (hex code 0x010E).
///   }
///   \table_row3{   <b>`Slideshow.EXIFSoftware`</b>,
///                  \anchor Slideshow_EXIFSoftware
///                  _string_,
///     Shows the name and version of the firmware used by the camera that took
///     the current picture. This is the value of the EXIF Software tag (hex
///     code 0x0131).
///   }
///   \table_row3{   <b>`Slideshow.EXIFTime`</b>,
///                  \anchor Slideshow_EXIFTime
///                  _string_,
///     Shows the date/timestamp of the current picture. The localized short
///     form of the date and time is used. The value of the EXIF
///     DateTimeOriginal tag (hex code 0x9003) is preferred. If the
///     DateTimeOriginal tag is not found\, the value of DateTimeDigitized (hex
///     code 0x9004) or of DateTime (hex code 0x0132) might be used.
///   }
///   \table_row3{   <b>`Slideshow.Exposure`</b>,
///                  \anchor Slideshow_Exposure
///                  _string_,
///     Shows the class of the program used by the camera to set exposure when
///     the current picture was taken. Values include "Manual"\,
///     "Program (Auto)"\, "Aperture priority (Semi-Auto)"\, "Shutter priority
///     (semi-auto)"\, etc. This is the value of the EXIF ExposureProgram tag
///     (hex code 0x8822).
///   }
///   \table_row3{   <b>`Slideshow.ExposureBias`</b>,
///                  \anchor Slideshow_ExposureBias
///                  _string_,
///     Shows the exposure bias of the current picture. Typically this is a
///     number between -99.99 and 99.99. This is the value of the EXIF
///     ExposureBiasValue tag (hex code 0x9204).
///   }
///   \table_row3{   <b>`Slideshow.ExposureMode`</b>,
///                  \anchor Slideshow_ExposureMode
///                  _string_,
///     Shows the exposure mode of the current picture. The possible values are
///     "Automatic"\, "Manual"\, and "Auto bracketing". This is the value of the
///     EXIF ExposureMode tag (hex code 0xA402).
///   }
///   \table_row3{   <b>`Slideshow.ExposureTime`</b>,
///                  \anchor Slideshow_ExposureTime
///                  _string_,
///     Shows the exposure time of the current picture\, in seconds. This is the
///     value of the EXIF ExposureTime tag (hex code 0x829A). If the
///     ExposureTime tag is not found\, the ShutterSpeedValue tag (hex code
///     0x9201) might be used.
///   }
///   \table_row3{   <b>`Slideshow.Filedate`</b>,
///                  \anchor Slideshow_Filedate
///                  _string_,
///     Shows the file date of the current picture
///   }
///   \table_row3{   <b>`Slideshow.Filename`</b>,
///                  \anchor Slideshow_Filename
///                  _string_,
///     Shows the file name of the current picture
///   }
///   \table_row3{   <b>`Slideshow.Filesize`</b>,
///                  \anchor Slideshow_Filesize
///                  _string_,
///     Shows the file size of the current picture
///   }
///   \table_row3{   <b>`Slideshow.FlashUsed`</b>,
///                  \anchor Slideshow_FlashUsed
///                  _string_,
///     Shows the status of flash when the current picture was taken. The value
///     will be either "Yes" or "No"\, and might include additional information.
///     This is the value of the EXIF Flash tag (hex code 0x9209).
///   }
///   \table_row3{   <b>`Slideshow.FocalLength`</b>,
///                  \anchor Slideshow_FocalLength
///                  _string_,
///     Shows the focal length of the lens\, in mm. This is the value of the EXIF
///     FocalLength tag (hex code 0x920A).
///   }
///   \table_row3{   <b>`Slideshow.FocusDistance`</b>,
///                  \anchor Slideshow_FocusDistance
///                  _string_,
///     Shows the distance to the subject\, in meters. This is the value of the
///     EXIF SubjectDistance tag (hex code 0x9206).
///   }
///   \table_row3{   <b>`Slideshow.Headline`</b>,
///                  \anchor Slideshow_Headline
///                  _string_,
///     Shows a synopsis of the contents of the current picture. This is the
///     value of the IPTC Headline tag (hex code 0x69).
///   }
///   \table_row3{   <b>`Slideshow.ImageType`</b>,
///                  \anchor Slideshow_ImageType
///                  _string_,
///     Shows the color components of the current picture. This is the value of
///     the IPTC ImageType tag (hex code 0x82).
///   }
///   \table_row3{   <b>`Slideshow.IPTCDate`</b>,
///                  \anchor Slideshow_IPTCDate
///                  _string_,
///     Shows the date when the intellectual content of the current picture was
///     created\, rather than when the picture was created. This is the value of
///     the IPTC DateCreated tag (hex code 0x37).
///   }
///   \table_row3{   <b>`Slideshow.ISOEquivalence`</b>,
///                  \anchor Slideshow_ISOEquivalence
///                  _string_,
///     Shows the ISO speed of the camera when the current picture was taken.
///     This is the value of the EXIF ISOSpeedRatings tag (hex code 0x8827).
///   }
///   \table_row3{   <b>`Slideshow.Keywords`</b>,
///                  \anchor Slideshow_Keywords
///                  _string_,
///     Shows keywords assigned to the current picture. This is the value of the
///     IPTC Keywords tag (hex code 0x19).
///   }
///   \table_row3{   <b>`Slideshow.Latitude`</b>,
///                  \anchor Slideshow_Latitude
///                  _string_,
///     Shows the latitude where the current picture was taken (degrees\,
///     minutes\, seconds North or South). This is the value of the EXIF
///     GPSInfo.GPSLatitude and GPSInfo.GPSLatitudeRef tags.
///   }
///   \table_row3{   <b>`Slideshow.LightSource`</b>,
///                  \anchor Slideshow_LightSource
///                  _string_,
///     Shows the kind of light source when the picture was taken. Possible
///     values include "Daylight"\, "Fluorescent"\, "Incandescent"\, etc. This is
///     the value of the EXIF LightSource tag (hex code 0x9208).
///   }
///   \table_row3{   <b>`Slideshow.LongEXIFDate`</b>,
///                  \anchor Slideshow_LongEXIFDate
///                  _string_,
///     Shows only the localized date of the current picture. The long form of
///     the date is used. The value of the EXIF DateTimeOriginal tag (hex code
///     0x9003) is preferred. If the DateTimeOriginal tag is not found\, the
///     value of DateTimeDigitized (hex code 0x9004) or of DateTime (hex code
///     0x0132) might be used.
///   }
///   \table_row3{   <b>`Slideshow.LongEXIFTime`</b>,
///                  \anchor Slideshow_LongEXIFTime
///                  _string_,
///     Shows the date/timestamp of the current picture. The localized long form
///     of the date and time is used. The value of the EXIF DateTimeOriginal tag
///     (hex code 0x9003) is preferred. if the DateTimeOriginal tag is not found\,
///     the value of DateTimeDigitized (hex code 0x9004) or of DateTime (hex
///     code 0x0132) might be used.
///   }
///   \table_row3{   <b>`Slideshow.Longitude`</b>,
///                  \anchor Slideshow_Longitude
///                  _string_,
///     Shows the longitude where the current picture was taken (degrees\,
///     minutes\, seconds East or West). This is the value of the EXIF
///     GPSInfo.GPSLongitude and GPSInfo.GPSLongitudeRef tags.
///   }
///   \table_row3{   <b>`Slideshow.MeteringMode`</b>,
///                  \anchor Slideshow_MeteringMode
///                  _string_,
///     Shows the metering mode used when the current picture was taken. The
///     possible values are "Center weight"\, "Spot"\, or "Matrix". This is the
///     value of the EXIF MeteringMode tag (hex code 0x9207).
///   }
///   \table_row3{   <b>`Slideshow.ObjectName`</b>,
///                  \anchor Slideshow_ObjectName
///                  _string_,
///     Shows a shorthand reference for the current picture. This is the value
///     of the IPTC ObjectName tag (hex code 0x05).
///   }
///   \table_row3{   <b>`Slideshow.Orientation`</b>,
///                  \anchor Slideshow_Orientation
///                  _string_,
///     Shows the orientation of the current picture. Possible values are "Top
///     Left"\, "Top Right"\, "Left Top"\, "Right Bottom"\, etc. This is the value
///     of the EXIF Orientation tag (hex code 0x0112).
///   }
///   \table_row3{   <b>`Slideshow.Path`</b>,
///                  \anchor Slideshow_Path
///                  _string_,
///     Shows the file path of the current picture
///   }
///   \table_row3{   <b>`Slideshow.Process`</b>,
///                  \anchor Slideshow_Process
///                  _string_,
///     Shows the process used to compress the current picture
///   }
///   \table_row3{   <b>`Slideshow.ReferenceService`</b>,
///                  \anchor Slideshow_ReferenceService
///                  _string_,
///     Shows the Service Identifier of a prior envelope to which the current
///     picture refers. This is the value of the IPTC ReferenceService tag (hex
///     code 0x2D).
///   }
///   \table_row3{   <b>`Slideshow.Resolution`</b>,
///                  \anchor Slideshow_Resolution
///                  _string_,
///     Shows the dimensions of the current picture (Width x Height)
///   }
///   \table_row3{   <b>`Slideshow.SlideComment`</b>,
///                  \anchor Slideshow_SlideComment
///                  _string_,
///     Shows a description of the current picture. This is the value of the
///     EXIF User Comment tag (hex code 0x9286). This is the same value as
///     Slideshow.EXIFComment.
///   }
///   \table_row3{   <b>`Slideshow.SlideIndex`</b>,
///                  \anchor Slideshow_SlideIndex
///                  _string_,
///     Shows the slide index of the current picture
///   }
///   \table_row3{   <b>`Slideshow.Source`</b>,
///                  \anchor Slideshow_Source
///                  _string_,
///     Shows the original owner of the current picture. This is the value of
///     the IPTC Source tag (hex code 0x73).
///   }
///   \table_row3{   <b>`Slideshow.SpecialInstructions`</b>,
///                  \anchor Slideshow_SpecialInstructions
///                  _string_,
///     Shows other editorial instructions concerning the use of the current
///     picture. This is the value of the IPTC SpecialInstructions tag (hex
///     code 0x28).
///   }
///   \table_row3{   <b>`Slideshow.State`</b>,
///                  \anchor Slideshow_State
///                  _string_,
///     Shows the State/Province where the current picture was taken. This is
///     the value of the IPTC ProvinceState tag (hex code 0x5F).
///   }
///   \table_row3{   <b>`Slideshow.Sublocation`</b>,
///                  \anchor Slideshow_Sublocation
///                  _string_,
///     Shows the location within a city where the current picture was taken -
///     might indicate the nearest landmark. This is the value of the IPTC
///     SubLocation tag (hex code 0x5C).
///   }
///   \table_row3{   <b>`Slideshow.SupplementalCategories`</b>,
///                  \anchor Slideshow_SupplementalCategories
///                  _string_,
///     Shows supplemental category codes to further refine the subject of the
///     current picture. This is the value of the IPTC SuppCategory tag (hex
///     code 0x14).
///   }
///   \table_row3{   <b>`Slideshow.TimeCreated`</b>,
///                  \anchor Slideshow_TimeCreated
///                  _string_,
///     Shows the time when the intellectual content of the current picture was
///     created\, rather than when the picture was created. This is the value of
///     the IPTC TimeCreated tag (hex code 0x3C).
///   }
///   \table_row3{   <b>`Slideshow.TransmissionReference`</b>,
///                  \anchor Slideshow_TransmissionReference
///                  _string_,
///     Shows a code representing the location of original transmission of the
///     current picture. This is the value of the IPTC TransmissionReference tag
///     (hex code 0x67).
///   }
///   \table_row3{   <b>`Slideshow.Urgency`</b>,
///                  \anchor Slideshow_Urgency
///                  _string_,
///     Shows the urgency of the current picture. Values are 1-9. The 1 is most
///     urgent. Some image management programs use urgency to indicate picture
///     rating\, where urgency 1 is 5 stars and urgency 5 is 1 star. Urgencies
///     6-9 are not used for rating. This is the value of the IPTC Urgency tag
///     (hex code 0x0A).
///   }
///   \table_row3{   <b>`Slideshow.WhiteBalance`</b>,
///                  \anchor Slideshow_WhiteBalance
///                  _string_,
///     Shows the white balance mode set when the current picture was taken.
///     The possible values are "Manual" and "Auto". This is the value of the
///     EXIF WhiteBalance tag (hex code 0xA403).
///   }
/// \table_end
///
/// -----------------------------------------------------------------------------
/// @}

int CPictureInfoTag::TranslateString(const std::string &info)
{
  if (StringUtils::EqualsNoCase(info, "filename")) return SLIDE_FILE_NAME;
  else if (StringUtils::EqualsNoCase(info, "path")) return SLIDE_FILE_PATH;
  else if (StringUtils::EqualsNoCase(info, "filesize")) return SLIDE_FILE_SIZE;
  else if (StringUtils::EqualsNoCase(info, "filedate")) return SLIDE_FILE_DATE;
  else if (StringUtils::EqualsNoCase(info, "slideindex")) return SLIDE_INDEX;
  else if (StringUtils::EqualsNoCase(info, "resolution")) return SLIDE_RESOLUTION;
  else if (StringUtils::EqualsNoCase(info, "slidecomment")) return SLIDE_COMMENT;
  else if (StringUtils::EqualsNoCase(info, "colour")) return SLIDE_COLOUR;
  else if (StringUtils::EqualsNoCase(info, "process")) return SLIDE_PROCESS;
  else if (StringUtils::EqualsNoCase(info, "exiftime")) return SLIDE_EXIF_DATE_TIME;
  else if (StringUtils::EqualsNoCase(info, "exifdate")) return SLIDE_EXIF_DATE;
  else if (StringUtils::EqualsNoCase(info, "longexiftime")) return SLIDE_EXIF_LONG_DATE_TIME;
  else if (StringUtils::EqualsNoCase(info, "longexifdate")) return SLIDE_EXIF_LONG_DATE;
  else if (StringUtils::EqualsNoCase(info, "exifdescription")) return SLIDE_EXIF_DESCRIPTION;
  else if (StringUtils::EqualsNoCase(info, "cameramake")) return SLIDE_EXIF_CAMERA_MAKE;
  else if (StringUtils::EqualsNoCase(info, "cameramodel")) return SLIDE_EXIF_CAMERA_MODEL;
  else if (StringUtils::EqualsNoCase(info, "exifcomment")) return SLIDE_EXIF_COMMENT;
  else if (StringUtils::EqualsNoCase(info, "exifsoftware")) return SLIDE_EXIF_SOFTWARE;
  else if (StringUtils::EqualsNoCase(info, "aperture")) return SLIDE_EXIF_APERTURE;
  else if (StringUtils::EqualsNoCase(info, "focallength")) return SLIDE_EXIF_FOCAL_LENGTH;
  else if (StringUtils::EqualsNoCase(info, "focusdistance")) return SLIDE_EXIF_FOCUS_DIST;
  else if (StringUtils::EqualsNoCase(info, "exposure")) return SLIDE_EXIF_EXPOSURE;
  else if (StringUtils::EqualsNoCase(info, "exposuretime")) return SLIDE_EXIF_EXPOSURE_TIME;
  else if (StringUtils::EqualsNoCase(info, "exposurebias")) return SLIDE_EXIF_EXPOSURE_BIAS;
  else if (StringUtils::EqualsNoCase(info, "exposuremode")) return SLIDE_EXIF_EXPOSURE_MODE;
  else if (StringUtils::EqualsNoCase(info, "flashused")) return SLIDE_EXIF_FLASH_USED;
  else if (StringUtils::EqualsNoCase(info, "whitebalance")) return SLIDE_EXIF_WHITE_BALANCE;
  else if (StringUtils::EqualsNoCase(info, "lightsource")) return SLIDE_EXIF_LIGHT_SOURCE;
  else if (StringUtils::EqualsNoCase(info, "meteringmode")) return SLIDE_EXIF_METERING_MODE;
  else if (StringUtils::EqualsNoCase(info, "isoequivalence")) return SLIDE_EXIF_ISO_EQUIV;
  else if (StringUtils::EqualsNoCase(info, "digitalzoom")) return SLIDE_EXIF_DIGITAL_ZOOM;
  else if (StringUtils::EqualsNoCase(info, "ccdwidth")) return SLIDE_EXIF_CCD_WIDTH;
  else if (StringUtils::EqualsNoCase(info, "orientation")) return SLIDE_EXIF_ORIENTATION;
  else if (StringUtils::EqualsNoCase(info, "supplementalcategories")) return SLIDE_IPTC_SUP_CATEGORIES;
  else if (StringUtils::EqualsNoCase(info, "keywords")) return SLIDE_IPTC_KEYWORDS;
  else if (StringUtils::EqualsNoCase(info, "caption")) return SLIDE_IPTC_CAPTION;
  else if (StringUtils::EqualsNoCase(info, "author")) return SLIDE_IPTC_AUTHOR;
  else if (StringUtils::EqualsNoCase(info, "headline")) return SLIDE_IPTC_HEADLINE;
  else if (StringUtils::EqualsNoCase(info, "specialinstructions")) return SLIDE_IPTC_SPEC_INSTR;
  else if (StringUtils::EqualsNoCase(info, "category")) return SLIDE_IPTC_CATEGORY;
  else if (StringUtils::EqualsNoCase(info, "byline")) return SLIDE_IPTC_BYLINE;
  else if (StringUtils::EqualsNoCase(info, "bylinetitle")) return SLIDE_IPTC_BYLINE_TITLE;
  else if (StringUtils::EqualsNoCase(info, "credit")) return SLIDE_IPTC_CREDIT;
  else if (StringUtils::EqualsNoCase(info, "source")) return SLIDE_IPTC_SOURCE;
  else if (StringUtils::EqualsNoCase(info, "copyrightnotice")) return SLIDE_IPTC_COPYRIGHT_NOTICE;
  else if (StringUtils::EqualsNoCase(info, "objectname")) return SLIDE_IPTC_OBJECT_NAME;
  else if (StringUtils::EqualsNoCase(info, "city")) return SLIDE_IPTC_CITY;
  else if (StringUtils::EqualsNoCase(info, "state")) return SLIDE_IPTC_STATE;
  else if (StringUtils::EqualsNoCase(info, "country")) return SLIDE_IPTC_COUNTRY;
  else if (StringUtils::EqualsNoCase(info, "transmissionreference")) return SLIDE_IPTC_TX_REFERENCE;
  else if (StringUtils::EqualsNoCase(info, "iptcdate")) return SLIDE_IPTC_DATE;
  else if (StringUtils::EqualsNoCase(info, "urgency")) return SLIDE_IPTC_URGENCY;
  else if (StringUtils::EqualsNoCase(info, "countrycode")) return SLIDE_IPTC_COUNTRY_CODE;
  else if (StringUtils::EqualsNoCase(info, "referenceservice")) return SLIDE_IPTC_REF_SERVICE;
  else if (StringUtils::EqualsNoCase(info, "latitude")) return SLIDE_EXIF_GPS_LATITUDE;
  else if (StringUtils::EqualsNoCase(info, "longitude")) return SLIDE_EXIF_GPS_LONGITUDE;
  else if (StringUtils::EqualsNoCase(info, "altitude")) return SLIDE_EXIF_GPS_ALTITUDE;
  else if (StringUtils::EqualsNoCase(info, "timecreated")) return SLIDE_IPTC_TIMECREATED;
  else if (StringUtils::EqualsNoCase(info, "sublocation")) return SLIDE_IPTC_SUBLOCATION;
  else if (StringUtils::EqualsNoCase(info, "imagetype")) return SLIDE_IPTC_IMAGETYPE;
  return 0;
}

void CPictureInfoTag::SetInfo(int info, const std::string& value)
{
  switch (info)
  {
  case SLIDE_RESOLUTION:
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
  case SLIDE_EXIF_DATE_TIME:
    {
      strcpy(m_exifInfo.DateTime, value.c_str());
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
  if (strlen(m_exifInfo.DateTime) >= 19 && m_exifInfo.DateTime[0] != ' ')
  {
    std::string dateTime = m_exifInfo.DateTime;
    int year  = atoi(dateTime.substr(0, 4).c_str());
    int month = atoi(dateTime.substr(5, 2).c_str());
    int day   = atoi(dateTime.substr(8, 2).c_str());
    int hour  = atoi(dateTime.substr(11,2).c_str());
    int min   = atoi(dateTime.substr(14,2).c_str());
    int sec   = atoi(dateTime.substr(17,2).c_str());
    m_dateTimeTaken.SetDateTime(year, month, day, hour, min, sec);
  }
}
