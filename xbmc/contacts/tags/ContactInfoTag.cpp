/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "ContactInfoTag.h"
#include "XBDateTime.h"
#include "Util.h"
#include "utils/Variant.h"
#include "utils/CharsetConverter.h"

//#include "contacts/ContactAlbum.h"
#include "contacts/Phone.h"
#include "utils/StringUtils.h"
#include "settings/AdvancedSettings.h"


using namespace std;

using namespace CONTACT_INFO;

int CContactInfoTag::GetDatabaseId() const
{
  return m_iDbId;
}


void CContactInfoTag::Reset()
{
  memset(&m_exifInfo, 0, sizeof(m_exifInfo));
  memset(&m_iptcInfo, 0, sizeof(m_iptcInfo));
  m_bLoaded = false;
  m_isInfoSetExternally = false;
  m_dateTimeTaken.Reset();
}


bool CContactInfoTag::Load(const CStdString &path)
{
  m_bLoaded = false;
  
  DllLibExif exifDll;
  if (path.IsEmpty() || !exifDll.Load())
    return false;
  
  if (exifDll.process_jpeg(path.c_str(), &m_exifInfo, &m_iptcInfo))
    m_bLoaded = true;
  
  ConvertDateTime();
  
  return m_bLoaded;
}

void CContactInfoTag::Archive(CArchive& ar)
{
  if (ar.IsStoring())
  {
    ar << m_bLoaded;
    ar << m_isInfoSetExternally;
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
    ar << m_dateTimeTaken;
    
    ar << CStdString(m_iptcInfo.Author);
    ar << CStdString(m_iptcInfo.Byline);
    ar << CStdString(m_iptcInfo.BylineTitle);
    ar << CStdString(m_iptcInfo.Caption);
    ar << CStdString(m_iptcInfo.Category);
    ar << CStdString(m_iptcInfo.City);
    ar << CStdString(m_iptcInfo.Urgency);
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
    ar << CStdString(m_iptcInfo.TimeCreated);
    ar << CStdString(m_iptcInfo.SubPhone);
    ar << CStdString(m_iptcInfo.ImageType);
  }
  else
  {
    ar >> m_bLoaded;
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
    GetStringFromArchive(ar, m_iptcInfo.SubPhone, sizeof(m_iptcInfo.SubPhone));
    GetStringFromArchive(ar, m_iptcInfo.ImageType, sizeof(m_iptcInfo.ImageType));
  }
  if (ar.IsStoring())
  {
    ar << m_strURL;
    ar << m_strTitle;
    ar << m_email;
    ar << m_strAlbum;
    ar << m_albumEmail;
    ar << m_phone;
    ar << m_bLoaded;
    ar << m_takenOn;
    ar << m_strOrientation;
    ar << m_strType;
    ar << m_strComment;
    ar << m_iAlbumId;
    ar << m_iDbId;
    ar << m_type;
    ar << m_strLyrics;
    ar << m_listeners;
  }
  else
  {
    ar >> m_strURL;
    ar >> m_strTitle;
    ar >> m_email;
    ar >> m_strAlbum;
    ar >> m_albumEmail;
    ar >> m_phone;
    ar >> m_strOrientation;
    ar >> m_strType;
    ar >> m_bLoaded;
    ar >> m_takenOn;
    ar >> m_strComment;
    ar >> m_iAlbumId;
    ar >> m_iDbId;
    ar >> m_type;
    ar >> m_strLyrics;
    ar >> m_listeners;
  }
}


void CContactInfoTag::ToSortable(SortItem& sortable)
{
  
  sortable[FieldTitle] = m_strTitle;
  sortable[FieldEmail] = m_email;
  sortable[FieldAlbum] = m_strAlbum;
  //    sortable[FieldAlbumEmail] = FieldAlbumEmail;
  sortable[FieldPhone] = m_phone;
  sortable[FieldComment] = m_strComment;
  sortable[FieldOrientation] = m_strOrientation;
  sortable[FieldTakenOn] = m_takenOn.IsValid() ? m_takenOn.GetAsDBDateTime() : StringUtils::EmptyString;
  sortable[FieldListeners] = m_listeners;
  sortable[FieldId] = (int64_t)m_iDbId;
  
  if (m_dateTimeTaken.IsValid())
    sortable[FieldDateTaken] = m_dateTimeTaken.GetAsDBDateTime();
}

void CContactInfoTag::GetStringFromArchive(CArchive &ar, char *string, size_t length)
{
  CStdString temp;
  ar >> temp;
  length = min((size_t)temp.GetLength(), length - 1);
  if (!temp.IsEmpty())
    memcpy(string, temp.c_str(), length);
  string[length] = 0;
}

const CStdString CContactInfoTag::GetInfo(int info) const
{
  if (!m_bLoaded && !m_isInfoSetExternally) // If no metadata has been loaded from the contact file or set with SetInfo(), just return
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
        value.Format("%4.2f EV", m_exifInfo.ExposureBias);
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
    case SLIDE_IPTC_URGENCY:          value = m_iptcInfo.Urgency;                 break;
    case SLIDE_IPTC_COUNTRY_CODE:     value = m_iptcInfo.CountryCode;             break;
    case SLIDE_IPTC_REF_SERVICE:      value = m_iptcInfo.ReferenceService;        break;
    case SLIDE_IPTC_TIMECREATED:      value = m_iptcInfo.TimeCreated;             break;
    case SLIDE_IPTC_SUBLOCATION:      value = m_iptcInfo.SubPhone;             break;
    case SLIDE_IPTC_IMAGETYPE:        value = m_iptcInfo.ImageType;               break;
    default:
      break;
  }
  return value;
}

int CContactInfoTag::TranslateString(const CStdString &info)
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
  else if (info.Equals("longexiftime")) return SLIDE_EXIF_LONG_DATE_TIME;
  else if (info.Equals("longexifdate")) return SLIDE_EXIF_LONG_DATE;
  else if (info.Equals("exifdescription")) return SLIDE_EXIF_DESCRIPTION;
  else if (info.Equals("cameramake")) return SLIDE_EXIF_CAMERA_MAKE;
  else if (info.Equals("cameramodel")) return SLIDE_EXIF_CAMERA_MODEL;
  else if (info.Equals("exifcomment")) return SLIDE_EXIF_COMMENT;
  else if (info.Equals("exifsoftware")) return SLIDE_EXIF_SOFTWARE;
  else if (info.Equals("aperture")) return SLIDE_EXIF_APERTURE;
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
  else if (info.Equals("headline")) return SLIDE_IPTC_HEADLINE;
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
  else if (info.Equals("urgency")) return SLIDE_IPTC_URGENCY;
  else if (info.Equals("countrycode")) return SLIDE_IPTC_COUNTRY_CODE;
  else if (info.Equals("referenceservice")) return SLIDE_IPTC_REF_SERVICE;
  else if (info.Equals("latitude")) return SLIDE_EXIF_GPS_LATITUDE;
  else if (info.Equals("longitude")) return SLIDE_EXIF_GPS_LONGITUDE;
  else if (info.Equals("altitude")) return SLIDE_EXIF_GPS_ALTITUDE;
  else if (info.Equals("timecreated")) return SLIDE_IPTC_TIMECREATED;
  else if (info.Equals("subphone")) return SLIDE_IPTC_SUBLOCATION;
  else if (info.Equals("imagetype")) return SLIDE_IPTC_IMAGETYPE;
  return 0;
}

void CContactInfoTag::SetInfo(int info, const CStdString& value)
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

const CDateTime& CContactInfoTag::GetDateTimeTaken() const
{
  return m_dateTimeTaken;
}

void CContactInfoTag::ConvertDateTime()
{
  if (strlen(m_exifInfo.DateTime) >= 19 && m_exifInfo.DateTime[0] != ' ')
  {
    CStdString dateTime = m_exifInfo.DateTime;
    int year  = atoi(dateTime.Mid(0, 4).c_str());
    int month = atoi(dateTime.Mid(5, 2).c_str());
    int day   = atoi(dateTime.Mid(8, 2).c_str());
    int hour  = atoi(dateTime.Mid(11,2).c_str());
    int min   = atoi(dateTime.Mid(14,2).c_str());
    int sec   = atoi(dateTime.Mid(17,2).c_str());
    m_dateTimeTaken.SetDateTime(year, month, day, hour, min, sec);
  }
}


///////////
//json support

CContactInfoTag::CContactInfoTag(void)
{
  Clear();
  Reset();
}

CContactInfoTag::CContactInfoTag(const CContactInfoTag& tag)
{
  *this = tag;
}

CContactInfoTag::~CContactInfoTag()
{}

const CContactInfoTag& CContactInfoTag::operator =(const CContactInfoTag& tag)
{
  if (this == &tag) return * this;
  
  m_strURL = tag.m_strURL;
  m_email = tag.m_email;
  m_albumEmail = tag.m_albumEmail;
  m_strAlbum = tag.m_strAlbum;
  m_phone = tag.m_phone;
  m_strTitle = tag.m_strTitle;
  m_strComment = tag.m_strComment;
  m_strType = tag.m_strType;
  m_strOrientation = tag.m_strOrientation;
  m_strLyrics = tag.m_strLyrics;
  m_takenOn = tag.m_takenOn;
  m_bLoaded = tag.m_bLoaded;
  m_listeners = tag.m_listeners;
  m_iDbId = tag.m_iDbId;
  m_type = tag.m_type;
  m_iAlbumId = tag.m_iAlbumId;
  
  m_coverArt = tag.m_coverArt;
  
  if (this == &tag) return * this;
  memcpy(&m_exifInfo, &tag.m_exifInfo, sizeof(m_exifInfo));
  memcpy(&m_iptcInfo, &tag.m_iptcInfo, sizeof(m_iptcInfo));
  m_bLoaded = tag.m_bLoaded;
  m_isInfoSetExternally = tag.m_isInfoSetExternally;
  m_dateTimeTaken = tag.m_dateTimeTaken;
  return *this;
  
  return *this;
}

bool CContactInfoTag::operator !=(const CContactInfoTag& tag) const
{
  if (this == &tag) return false;
  if (m_strURL != tag.m_strURL) return true;
  if (m_strTitle != tag.m_strTitle) return true;
  if (m_email != tag.m_email) return true;
  if (m_albumEmail != tag.m_albumEmail) return true;
  if (m_strAlbum != tag.m_strAlbum) return true;
  return false;
}



const CStdString& CContactInfoTag::GetTitle() const
{
  return m_strTitle;
}

const CStdString& CContactInfoTag::GetURL() const
{
  return m_strURL;
}

const std::vector<std::string>& CContactInfoTag::GetEmail() const
{
  return m_email;
}

const CStdString& CContactInfoTag::GetContactAlbum() const
{
  return m_strAlbum;
}

int CContactInfoTag::GetContactAlbumId() const
{
  return m_iAlbumId;
}

const std::vector<std::string>& CContactInfoTag::GetContactAlbumEmail() const
{
  return m_albumEmail;
}

const std::vector<std::string>& CContactInfoTag::GetPhone() const
{
  return m_phone;
}


const std::string &CContactInfoTag::GetType() const
{
  return m_type;
}


const CStdString &CContactInfoTag::GetComment() const
{
  return m_strComment;
}

const CStdString &CContactInfoTag::GetOrientation() const
{
  return m_strOrientation;
}
const CStdString &CContactInfoTag::GetContactType() const
{
  return m_strType;
}



int CContactInfoTag::GetListeners() const
{
  return m_listeners;
}


const CDateTime &CContactInfoTag::GetTakenOn() const
{
  return m_takenOn;
}


const EmbeddedArtInfo &CContactInfoTag::GetCoverArtInfo() const
{
  return m_coverArt;
}


void CContactInfoTag::SetURL(const CStdString& strURL)
{
  m_strURL = strURL;
}

void CContactInfoTag::SetTitle(const CStdString& strTitle)
{
  m_strTitle = Trim(strTitle);
}

void CContactInfoTag::SetEmail(const CStdString& strEmail)
{
  if (!strEmail.empty())
    SetEmail(StringUtils::Split(strEmail, g_advancedSettings.m_contactItemSeparator));
  else
    m_email.clear();
}

void CContactInfoTag::SetEmail(const std::vector<std::string>& emails)
{
  m_email = emails;
}

void CContactInfoTag::SetAlbum(const CStdString& strAlbum)
{
  m_strAlbum = Trim(strAlbum);
}

void CContactInfoTag::SetAlbumId(const int iAlbumId)
{
  m_iAlbumId = iAlbumId;
}

void CContactInfoTag::SetAlbumEmail(const CStdString& strAlbumEmail)
{
  if (!strAlbumEmail.empty())
    SetAlbumEmail(StringUtils::Split(strAlbumEmail, g_advancedSettings.m_contactItemSeparator));
  else
    m_albumEmail.clear();
}

void CContactInfoTag::SetAlbumEmail(const std::vector<std::string>& albumEmails)
{
  m_albumEmail = albumEmails;
}

void CContactInfoTag::SetPhone(const CStdString& strPhone)
{
  if (!strPhone.empty())
    SetPhone(StringUtils::Split(strPhone, g_advancedSettings.m_contactItemSeparator));
  else
    m_phone.clear();
}

void CContactInfoTag::SetPhone(const std::vector<std::string>& phones)
{
  m_phone = phones;
}


void CContactInfoTag::SetDatabaseId(long id, const std::string &type)
{
  m_iDbId = id;
  m_type = type;
}


void CContactInfoTag::SetComment(const CStdString& comment)
{
  m_strComment = comment;
}

void CContactInfoTag::SetOrientation(const CStdString& orientation)
{
  m_strOrientation = orientation;
}

void CContactInfoTag::SetContactType(const CStdString& type)
{
  m_strType = type;
}



void CContactInfoTag::SetListeners(int listeners)
{
  m_listeners = listeners;
}


void CContactInfoTag::SetTakenOn(const CStdString& takenond)
{
  m_takenOn.SetFromDBDateTime(takenond);
}

void CContactInfoTag::SetTakenOn(const CDateTime& takenond)
{
  m_takenOn = takenond;
}


void CContactInfoTag::SetLoaded(bool bOnOff)
{
  m_bLoaded = bOnOff;
}

bool CContactInfoTag::Loaded() const
{
  return m_bLoaded;
}


void CContactInfoTag::SetCoverArtInfo(size_t size, const std::string &mimeType)
{
  m_coverArt.set(size, mimeType);
}


void CContactInfoTag::SetEmail(const CEmail& email)
{
  SetEmail(email.strEmail);
  SetAlbumEmail(email.strEmail);
  SetPhone(email.phone);
  m_iDbId = email.idEmail;
  m_type = "email";
  m_bLoaded = true;
}

void CContactInfoTag::SetAlbum(const CContactAlbum& album)
{
  SetEmail(album.email);
  SetAlbumId(album.idAlbum);
  SetAlbum(album.strAlbum);
  SetTitle(album.strAlbum);
  SetAlbumEmail(album.email);
  SetPhone(album.phone);
  SYSTEMTIME stTime;
  m_iDbId = album.idAlbum;
  m_type = "album";
  m_bLoaded = true;
}

void CContactInfoTag::SetContact(const CContact& contact)
{
  SetTitle(contact.strTitle);
  SetPhone(contact.phone);
  SetEmail(contact.email);
  SetAlbum(contact.strAlbum);
  SetAlbumEmail(contact.albumEmail);
  SetComment(contact.strComment);
  SetTakenOn(contact.takenOn);
  SetOrientation(contact.strOrientation);
  SetContactType(contact.strContactType);
  m_strURL = contact.strFileName;
  SYSTEMTIME stTime;
  m_iDbId = contact.idContact;
  m_type = "contact";
  m_bLoaded = true;
  m_iAlbumId = contact.idAlbum;
}

void CContactInfoTag::Serialize(CVariant& value) const
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
  value["urgency"] = CStdString(m_iptcInfo.Urgency);
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
  value["timecreated"] = CStdString(m_iptcInfo.TimeCreated);
  value["subphone"] = CStdString(m_iptcInfo.SubPhone);
  value["imagetype"] = CStdString(m_iptcInfo.ImageType);
  
  
  value["url"] = m_strURL;
  value["title"] = m_strTitle;
  if (m_type.compare("email") == 0 && m_email.size() == 1)
    value["email"] = m_email[0];
  else
    value["email"] = m_email;
  value["displayemail"] = StringUtils::Join(m_email, g_advancedSettings.m_contactItemSeparator);
  value["album"] = m_strAlbum;
  value["albumemail"] = m_albumEmail;
  value["phone"] = m_phone;
  value["loaded"] = m_bLoaded;
  value["comment"] = m_strComment;
  value["takenon"] = m_takenOn.IsValid() ? m_takenOn.GetAsDBDateTime() : StringUtils::EmptyString;
  value["lyrics"] = m_strLyrics;
  value["albumid"] = m_iAlbumId;
}

void CContactInfoTag::Clear()
{
  m_strURL.Empty();
  m_email.clear();
  m_strAlbum.Empty();
  m_albumEmail.clear();
  m_phone.clear();
  m_strTitle.Empty();
  m_bLoaded = false;
  m_takenOn.Reset();
  m_strComment.Empty();
  m_iDbId = -1;
  m_type.clear();
  m_iAlbumId = -1;
  m_coverArt.clear();
}

void CContactInfoTag::AppendEmail(const CStdString &email)
{
  for (unsigned int index = 0; index < m_email.size(); index++)
  {
    if (email.Equals(m_email.at(index).c_str()))
      return;
  }
  
  m_email.push_back(email);
}

void CContactInfoTag::AppendAlbumEmail(const CStdString &albumEmail)
{
  for (unsigned int index = 0; index < m_albumEmail.size(); index++)
  {
    if (albumEmail.Equals(m_albumEmail.at(index).c_str()))
      return;
  }
  
  m_albumEmail.push_back(albumEmail);
}

void CContactInfoTag::AppendPhone(const CStdString &phone)
{
  for (unsigned int index = 0; index < m_phone.size(); index++)
  {
    if (phone.Equals(m_phone.at(index).c_str()))
      return;
  }
  
  m_phone.push_back(phone);
}

CStdString CContactInfoTag::Trim(const CStdString &value) const
{
  CStdString trimmedValue(value);
  trimmedValue.TrimLeft(' ');
  trimmedValue.TrimRight(" \n\r");
  return trimmedValue;
}