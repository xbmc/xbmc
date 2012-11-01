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

#include "PictureInfoTag.h"
#include "XBDateTime.h"
#include "Util.h"
#include "utils/Variant.h"
#include "utils/CharsetConverter.h"

using namespace std;

void CPictureInfoTag::Reset()
{
  memset(&m_exifInfo, 0, sizeof(m_exifInfo));
  memset(&m_iptcInfo, 0, sizeof(m_iptcInfo));
  m_isLoaded = false;
}

const CPictureInfoTag& CPictureInfoTag::operator=(const CPictureInfoTag& right)
{
  if (this == &right) return * this;
  memcpy(&m_exifInfo, &right.m_exifInfo, sizeof(m_exifInfo));
  memcpy(&m_iptcInfo, &right.m_iptcInfo, sizeof(m_iptcInfo));
  m_isLoaded = right.m_isLoaded;
  return *this;
}

bool CPictureInfoTag::Load(const CStdString &path)
{
  m_isLoaded = false;

  DllLibExif exifDll;
  if (path.IsEmpty() || !exifDll.Load())
    return false;

  if (exifDll.process_jpeg(path.c_str(), &m_exifInfo, &m_iptcInfo))
    m_isLoaded = true;

  return m_isLoaded;
}

void CPictureInfoTag::Archive(CArchive& ar)
{
  if (ar.IsStoring())
  {
    ar << m_isLoaded;
    ar << m_exifInfo.ApertureFNumber;
    ar << CStdString(m_exifInfo.CameraMake);
    ar << CStdString(m_exifInfo.CameraModel);
    ar << m_exifInfo.CCDWidth;
    ar << GetInfo(SLIDE_EXIF_COMMENT); // Store and restore the comment charset converted
    ar << CStdString(m_exifInfo.Description);
    ar << CStdString(m_exifInfo.DateTime);
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
    ar << CStdString(m_exifInfo.GpsAlt);
    ar << CStdString(m_exifInfo.GpsLat);
    ar << CStdString(m_exifInfo.GpsLong);
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

    ar << CStdString(m_iptcInfo.Author);
    ar << CStdString(m_iptcInfo.Byline);
    ar << CStdString(m_iptcInfo.BylineTitle);
    ar << CStdString(m_iptcInfo.Caption);
    ar << CStdString(m_iptcInfo.Category);
    ar << CStdString(m_iptcInfo.City);
    ar << CStdString(m_iptcInfo.Copyright);
    ar << CStdString(m_iptcInfo.CopyrightNotice);
    ar << CStdString(m_iptcInfo.Country);
    ar << CStdString(m_iptcInfo.CountryCode);
    ar << CStdString(m_iptcInfo.Credit);
    ar << CStdString(m_iptcInfo.Date);
    ar << CStdString(m_iptcInfo.Headline);
    ar << CStdString(m_iptcInfo.Keywords);
    ar << CStdString(m_iptcInfo.ObjectName);
    ar << CStdString(m_iptcInfo.ReferenceService);
    ar << CStdString(m_iptcInfo.Source);
    ar << CStdString(m_iptcInfo.SpecialInstructions);
    ar << CStdString(m_iptcInfo.State);
    ar << CStdString(m_iptcInfo.SupplementalCategories);
    ar << CStdString(m_iptcInfo.TransmissionReference);
  }
  else
  {
    ar >> m_isLoaded;
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

    GetStringFromArchive(ar, m_iptcInfo.Author, sizeof(m_iptcInfo.Author));
    GetStringFromArchive(ar, m_iptcInfo.Byline, sizeof(m_iptcInfo.Byline));
    GetStringFromArchive(ar, m_iptcInfo.BylineTitle, sizeof(m_iptcInfo.BylineTitle));
    GetStringFromArchive(ar, m_iptcInfo.Caption, sizeof(m_iptcInfo.Caption));
    GetStringFromArchive(ar, m_iptcInfo.Category, sizeof(m_iptcInfo.Category));
    GetStringFromArchive(ar, m_iptcInfo.City, sizeof(m_iptcInfo.City));
    GetStringFromArchive(ar, m_iptcInfo.Copyright, sizeof(m_iptcInfo.Copyright));
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
  }
}

void CPictureInfoTag::Serialize(CVariant& value) const
{
  value["aperturefnumber"] = m_exifInfo.ApertureFNumber;
  value["cameramake"] = CStdString(m_exifInfo.CameraMake);
  value["cameramodel"] = CStdString(m_exifInfo.CameraModel);
  value["ccdwidth"] = m_exifInfo.CCDWidth;
  value["comments"] = GetInfo(SLIDE_EXIF_COMMENT); // Charset conversion
  value["description"] = CStdString(m_exifInfo.Description);
  value["datetime"] = CStdString(m_exifInfo.DateTime);
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
  value["gpsinfo"]["alt"] = CStdString(m_exifInfo.GpsAlt);
  value["gpsinfo"]["lat"] = CStdString(m_exifInfo.GpsLat);
  value["gpsinfo"]["long"] = CStdString(m_exifInfo.GpsLong);
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

  value["author"] = CStdString(m_iptcInfo.Author);
  value["byline"] = CStdString(m_iptcInfo.Byline);
  value["bylinetitle"] = CStdString(m_iptcInfo.BylineTitle);
  value["caption"] = CStdString(m_iptcInfo.Caption);
  value["category"] = CStdString(m_iptcInfo.Category);
  value["city"] = CStdString(m_iptcInfo.City);
  value["copyright"] = CStdString(m_iptcInfo.Copyright);
  value["copyrightnotice"] = CStdString(m_iptcInfo.CopyrightNotice);
  value["country"] = CStdString(m_iptcInfo.Country);
  value["countrycode"] = CStdString(m_iptcInfo.CountryCode);
  value["credit"] = CStdString(m_iptcInfo.Credit);
  value["date"] = CStdString(m_iptcInfo.Date);
  value["headline"] = CStdString(m_iptcInfo.Headline);
  value["keywords"] = CStdString(m_iptcInfo.Keywords);
  value["objectname"] = CStdString(m_iptcInfo.ObjectName);
  value["referenceservice"] = CStdString(m_iptcInfo.ReferenceService);
  value["source"] = CStdString(m_iptcInfo.Source);
  value["specialinstructions"] = CStdString(m_iptcInfo.SpecialInstructions);
  value["state"] = CStdString(m_iptcInfo.State);
  value["supplementalcategories"] = CStdString(m_iptcInfo.SupplementalCategories);
  value["transmissionreference"] = CStdString(m_iptcInfo.TransmissionReference);
}

void CPictureInfoTag::ToSortable(SortItem& sortable)
{
  
}

void CPictureInfoTag::GetStringFromArchive(CArchive &ar, char *string, size_t length)
{
  CStdString temp;
  ar >> temp;
  length = min((size_t)temp.GetLength(), length - 1);
  if (!temp.IsEmpty())
    memcpy(string, temp.c_str(), length);
  string[length] = 0;
}

const CStdString CPictureInfoTag::GetInfo(int info) const
{
  if (!m_isLoaded)
    return "";

  CStdString value;
  switch (info)
  {
  case SLIDE_RESOLUTION:
    value.Format("%d x %d", m_exifInfo.Width, m_exifInfo.Height);
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
  case SLIDE_EXIF_COMMENT:
    // The charset used for the UserComment is stored in CommentsCharset:
    // Ascii, Unicode (UCS2), JIS (X208-1990), Unknown (application specific)
    if (m_exifInfo.CommentsCharset == EXIF_COMMENT_CHARSET_UNICODE)
    {
      g_charsetConverter.ucs2ToUTF8(CStdString16((uint16_t*)m_exifInfo.Comments), value);
    }
    else
    {
      // Ascii doesn't need to be converted (EXIF_COMMENT_CHARSET_ASCII)
      // Archived data is already converted (EXIF_COMMENT_CHARSET_CONVERTED)
      // Unknown data can't be converted as it could be any codec (EXIF_COMMENT_CHARSET_UNKNOWN)
      // JIS data can't be converted as CharsetConverter and iconv lacks support (EXIF_COMMENT_CHARSET_JIS)
      value = m_exifInfo.Comments;
    }
    break;
  case SLIDE_EXIF_DATE_TIME:
  case SLIDE_EXIF_DATE:
    if (strlen(m_exifInfo.DateTime) >= 19 && m_exifInfo.DateTime[0] != ' ')
    {
      CStdString dateTime = m_exifInfo.DateTime;
      int year  = atoi(dateTime.Mid(0, 4).c_str());
      int month = atoi(dateTime.Mid(5, 2).c_str());
      int day   = atoi(dateTime.Mid(8, 2).c_str());
      int hour  = atoi(dateTime.Mid(11,2).c_str());
      int min   = atoi(dateTime.Mid(14,2).c_str());
      int sec   = atoi(dateTime.Mid(17,2).c_str());
      CDateTime date(year, month, day, hour, min, sec);
      if(SLIDE_EXIF_DATE_TIME == info)
          value = date.GetAsLocalizedDateTime();
      else
          value = date.GetAsLocalizedDate();
    }
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
      value.Format("%3.1f", m_exifInfo.ApertureFNumber);
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
      value.Format("%4.2fmm", m_exifInfo.FocalLength);
      if (m_exifInfo.FocalLength35mmEquiv != 0)
        value.AppendFormat("  (35mm Equivalent = %umm)", m_exifInfo.FocalLength35mmEquiv);
    }
    break;
  case SLIDE_EXIF_FOCUS_DIST:
    if (m_exifInfo.Distance < 0)
      value = "Infinite";
    else if (m_exifInfo.Distance > 0)
      value.Format("%4.2fm", m_exifInfo.Distance);
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
        value.Format("%6.4fs", m_exifInfo.ExposureTime);
      else
        value.Format("%5.3fs", m_exifInfo.ExposureTime);
      if (m_exifInfo.ExposureTime <= 0.5)
        value.AppendFormat(" (1/%d)", (int)(0.5 + 1/m_exifInfo.ExposureTime));
    }
    break;
  case SLIDE_EXIF_EXPOSURE_BIAS:
    if (m_exifInfo.ExposureBias != 0)
      value.Format("%4.2", m_exifInfo.ExposureBias);
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
      value.Format("%2d", m_exifInfo.ISOequivalent);
    break;
  case SLIDE_EXIF_DIGITAL_ZOOM:
    if (m_exifInfo.DigitalZoomRatio)
      value.Format("%1.3fx", m_exifInfo.DigitalZoomRatio);
    break;
  case SLIDE_EXIF_CCD_WIDTH:
    if (m_exifInfo.CCDWidth)
      value.Format("%4.2fmm", m_exifInfo.CCDWidth);
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
  case SLIDE_IPTC_COPYRIGHT:        value = m_iptcInfo.Copyright;               break;
  case SLIDE_IPTC_COUNTRY_CODE:     value = m_iptcInfo.CountryCode;             break;
  case SLIDE_IPTC_REF_SERVICE:      value = m_iptcInfo.ReferenceService;        break;
  default:
    break;
  }
  return value;
}

int CPictureInfoTag::TranslateString(const CStdString &info)
{
  if (info.Equals("filename")) return SLIDE_FILE_NAME;
  else if (info.Equals("path")) return SLIDE_FILE_PATH;
  else if (info.Equals("filesize")) return SLIDE_FILE_SIZE;
  else if (info.Equals("filedate")) return SLIDE_FILE_DATE;
  else if (info.Equals("slideindex")) return SLIDE_INDEX;
  else if (info.Equals("resolution")) return SLIDE_RESOLUTION;
  else if (info.Equals("slidecomment")) return SLIDE_COMMENT;
  else if (info.Equals("colour")) return SLIDE_COLOUR;
  else if (info.Equals("process")) return SLIDE_PROCESS;
  else if (info.Equals("exiftime")) return SLIDE_EXIF_DATE_TIME;
  else if (info.Equals("exifdate")) return SLIDE_EXIF_DATE;
  else if (info.Equals("exifdescription")) return SLIDE_EXIF_DESCRIPTION;
  else if (info.Equals("cameramake")) return SLIDE_EXIF_CAMERA_MAKE;
  else if (info.Equals("cameramodel")) return SLIDE_EXIF_CAMERA_MODEL;
  else if (info.Equals("exifcomment")) return SLIDE_EXIF_COMMENT;
  else if (info.Equals("exifsoftware")) return SLIDE_EXIF_SOFTWARE;
  else if (info.Equals("apreture")) return SLIDE_EXIF_APERTURE;
  else if (info.Equals("focallength")) return SLIDE_EXIF_FOCAL_LENGTH;
  else if (info.Equals("focusdistance")) return SLIDE_EXIF_FOCUS_DIST;
  else if (info.Equals("exposure")) return SLIDE_EXIF_EXPOSURE;
  else if (info.Equals("exposuretime")) return SLIDE_EXIF_EXPOSURE_TIME;
  else if (info.Equals("exposurebias")) return SLIDE_EXIF_EXPOSURE_BIAS;
  else if (info.Equals("exposuremode")) return SLIDE_EXIF_EXPOSURE_MODE;
  else if (info.Equals("flashused")) return SLIDE_EXIF_FLASH_USED;
  else if (info.Equals("whitebalance")) return SLIDE_EXIF_WHITE_BALANCE;
  else if (info.Equals("lightsource")) return SLIDE_EXIF_LIGHT_SOURCE;
  else if (info.Equals("meteringmode")) return SLIDE_EXIF_METERING_MODE;
  else if (info.Equals("isoequivalence")) return SLIDE_EXIF_ISO_EQUIV;
  else if (info.Equals("digitalzoom")) return SLIDE_EXIF_DIGITAL_ZOOM;
  else if (info.Equals("ccdwidth")) return SLIDE_EXIF_CCD_WIDTH;
  else if (info.Equals("orientation")) return SLIDE_EXIF_ORIENTATION;
  else if (info.Equals("supplementalcategories")) return SLIDE_IPTC_SUP_CATEGORIES;
  else if (info.Equals("keywords")) return SLIDE_IPTC_KEYWORDS;
  else if (info.Equals("caption")) return SLIDE_IPTC_CAPTION;
  else if (info.Equals("author")) return SLIDE_IPTC_AUTHOR;
  else if (info.Equals("healine")) return SLIDE_IPTC_HEADLINE;
  else if (info.Equals("specialinstructions")) return SLIDE_IPTC_SPEC_INSTR;
  else if (info.Equals("category")) return SLIDE_IPTC_CATEGORY;
  else if (info.Equals("byline")) return SLIDE_IPTC_BYLINE;
  else if (info.Equals("bylinetitle")) return SLIDE_IPTC_BYLINE_TITLE;
  else if (info.Equals("credit")) return SLIDE_IPTC_CREDIT;
  else if (info.Equals("source")) return SLIDE_IPTC_SOURCE;
  else if (info.Equals("copyrightnotice")) return SLIDE_IPTC_COPYRIGHT_NOTICE;
  else if (info.Equals("objectname")) return SLIDE_IPTC_OBJECT_NAME;
  else if (info.Equals("city")) return SLIDE_IPTC_CITY;
  else if (info.Equals("state")) return SLIDE_IPTC_STATE;
  else if (info.Equals("country")) return SLIDE_IPTC_COUNTRY;
  else if (info.Equals("transmissionreference")) return SLIDE_IPTC_TX_REFERENCE;
  else if (info.Equals("iptcdate")) return SLIDE_IPTC_DATE;
  else if (info.Equals("copyright")) return SLIDE_IPTC_COPYRIGHT;
  else if (info.Equals("countrycode")) return SLIDE_IPTC_COUNTRY_CODE;
  else if (info.Equals("referenceservice")) return SLIDE_IPTC_REF_SERVICE;
  else if (info.Equals("latitude")) return SLIDE_EXIF_GPS_LATITUDE;
  else if (info.Equals("longitude")) return SLIDE_EXIF_GPS_LONGITUDE;
  else if (info.Equals("altitude")) return SLIDE_EXIF_GPS_ALTITUDE;
  return 0;
}

void CPictureInfoTag::SetInfo(int info, const CStdString& value)
{
  switch (info)
  {
  case SLIDE_RESOLUTION:
    {
      vector<CStdString> dimension;
      CUtil::Tokenize(value, dimension, ",");
      if (dimension.size() == 2)
      {
        m_exifInfo.Width = atoi(dimension[0].c_str());
        m_exifInfo.Height = atoi(dimension[1].c_str());
      }
      break;
    }
  case SLIDE_EXIF_DATE_TIME:
    {
      strcpy(m_exifInfo.DateTime, value.c_str());
      break;
    }
  default:
    break;
  }
}

void CPictureInfoTag::SetLoaded(bool loaded)
{
  m_isLoaded = loaded;
}

