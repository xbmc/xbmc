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

#include "PictureInfoTag.h"
#include "XBDateTime.h"
#include "Util.h"
#include "utils/Variant.h"
#include "utils/CharsetConverter.h"

#include "pictures/PictureAlbum.h"
#include "pictures/Face.h"
#include "utils/StringUtils.h"
#include "settings/AdvancedSettings.h"


using namespace std;

using namespace PICTURE_INFO;

EmbeddedArtInfo::EmbeddedArtInfo(size_t siz, const std::string &mim)
{
    set(siz, mim);
}

void EmbeddedArtInfo::set(size_t siz, const std::string &mim)
{
    size = siz;
    mime = mim;
}

void EmbeddedArtInfo::clear()
{
    mime.clear();
    size = 0;
}

bool EmbeddedArtInfo::empty() const
{
    return size == 0;
}

bool EmbeddedArtInfo::matches(const EmbeddedArtInfo &right) const
{
    return (size == right.size &&
            mime == right.mime);
}

EmbeddedArt::EmbeddedArt(const uint8_t *dat, size_t siz, const std::string &mim)
{
    set(dat, siz, mim);
}

void EmbeddedArt::set(const uint8_t *dat, size_t siz, const std::string &mim)
{
    EmbeddedArtInfo::set(siz, mim);
    data.resize(siz);
    memcpy(&data[0], dat, siz);
}


int CPictureInfoTag::GetDatabaseId() const
{
    return m_iDbId;
}


void CPictureInfoTag::Reset()
{
  memset(&m_exifInfo, 0, sizeof(m_exifInfo));
  memset(&m_iptcInfo, 0, sizeof(m_iptcInfo));
  m_bLoaded = false;
  m_isInfoSetExternally = false;
  m_dateTimeTaken.Reset();
}


bool CPictureInfoTag::Load(const CStdString &path)
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

void CPictureInfoTag::Archive(CArchive& ar)
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
    ar << CStdString(m_iptcInfo.SubLocation);
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
    GetStringFromArchive(ar, m_iptcInfo.SubLocation, sizeof(m_iptcInfo.SubLocation));
    GetStringFromArchive(ar, m_iptcInfo.ImageType, sizeof(m_iptcInfo.ImageType));
  }
    if (ar.IsStoring())
    {
        ar << m_strURL;
        ar << m_strTitle;
        ar << m_face;
        ar << m_strAlbum;
        ar << m_albumFace;
        ar << m_location;
        ar << m_iDuration;
        ar << m_iTrack;
        ar << m_bLoaded;
        ar << m_dwReleaseDate;
        ar << m_strPictureBrainzTrackID;
        ar << m_pictureBrainzFaceID;
        ar << m_strPictureBrainzAlbumID;
        ar << m_pictureBrainzAlbumFaceID;
        ar << m_strPictureBrainzTRMID;
        ar << m_lastPlayed;
        ar << m_strComment;
        ar << m_rating;
        ar << m_iTimesPlayed;
        ar << m_iAlbumId;
        ar << m_iDbId;
        ar << m_type;
        ar << m_strLyrics;
        ar << m_bCompilation;
        ar << m_listeners;
    }
    else
    {
        ar >> m_strURL;
        ar >> m_strTitle;
        ar >> m_face;
        ar >> m_strAlbum;
        ar >> m_albumFace;
        ar >> m_location;
        ar >> m_iDuration;
        ar >> m_iTrack;
        ar >> m_bLoaded;
        ar >> m_dwReleaseDate;
        ar >> m_strPictureBrainzTrackID;
        ar >> m_pictureBrainzFaceID;
        ar >> m_strPictureBrainzAlbumID;
        ar >> m_pictureBrainzAlbumFaceID;
        ar >> m_strPictureBrainzTRMID;
        ar >> m_lastPlayed;
        ar >> m_strComment;
        ar >> m_rating;
        ar >> m_iTimesPlayed;
        ar >> m_iAlbumId;
        ar >> m_iDbId;
        ar >> m_type;
        ar >> m_strLyrics;
        ar >> m_bCompilation;
        ar >> m_listeners;
    }
}


void CPictureInfoTag::ToSortable(SortItem& sortable)
{
    /*
    sortable[FieldTitle] = m_strTitle;
    sortable[FieldFace] = m_face;
    sortable[FieldAlbum] = m_strAlbum;
    sortable[FieldAlbumFace] = FieldAlbumFace;
    sortable[FieldLocation] = m_location;
    sortable[FieldTime] = m_iDuration;
    sortable[FieldTrackNumber] = m_iTrack;
    sortable[FieldYear] = m_dwReleaseDate.wYear;
    sortable[FieldComment] = m_strComment;
    sortable[FieldRating] = (float)(m_rating - '0');
    sortable[FieldPlaycount] = m_iTimesPlayed;
    sortable[FieldLastPlayed] = m_lastPlayed.IsValid() ? m_lastPlayed.GetAsDBDateTime() : StringUtils::EmptyString;
    sortable[FieldListeners] = m_listeners;
    sortable[FieldId] = (int64_t)m_iDbId;
    */
  if (m_dateTimeTaken.IsValid())
    sortable[FieldDateTaken] = m_dateTimeTaken.GetAsDBDateTime();
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
  if (!m_bLoaded && !m_isInfoSetExternally) // If no metadata has been loaded from the picture file or set with SetInfo(), just return
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
  case SLIDE_IPTC_SUBLOCATION:      value = m_iptcInfo.SubLocation;             break;
  case SLIDE_IPTC_IMAGETYPE:        value = m_iptcInfo.ImageType;               break;
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
  else if (info.Equals("sublocation")) return SLIDE_IPTC_SUBLOCATION;
  else if (info.Equals("imagetype")) return SLIDE_IPTC_IMAGETYPE;
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

CPictureInfoTag::CPictureInfoTag(void)
{
    Clear();
    Reset();
}

CPictureInfoTag::CPictureInfoTag(const CPictureInfoTag& tag)
{
    *this = tag;
}

CPictureInfoTag::~CPictureInfoTag()
{}

const CPictureInfoTag& CPictureInfoTag::operator =(const CPictureInfoTag& tag)
{
    if (this == &tag) return * this;
    
    m_strURL = tag.m_strURL;
    m_face = tag.m_face;
    m_albumFace = tag.m_albumFace;
    m_strAlbum = tag.m_strAlbum;
    m_location = tag.m_location;
    m_strTitle = tag.m_strTitle;
    m_strPictureBrainzTrackID = tag.m_strPictureBrainzTrackID;
    m_pictureBrainzFaceID = tag.m_pictureBrainzFaceID;
    m_strPictureBrainzAlbumID = tag.m_strPictureBrainzAlbumID;
    m_pictureBrainzAlbumFaceID = tag.m_pictureBrainzAlbumFaceID;
    m_strPictureBrainzTRMID = tag.m_strPictureBrainzTRMID;
    m_strComment = tag.m_strComment;
    m_strLyrics = tag.m_strLyrics;
    m_lastPlayed = tag.m_lastPlayed;
    m_bCompilation = tag.m_bCompilation;
    m_iDuration = tag.m_iDuration;
    m_iTrack = tag.m_iTrack;
    m_bLoaded = tag.m_bLoaded;
    m_rating = tag.m_rating;
    m_listeners = tag.m_listeners;
    m_iTimesPlayed = tag.m_iTimesPlayed;
    m_iDbId = tag.m_iDbId;
    m_type = tag.m_type;
    m_iAlbumId = tag.m_iAlbumId;
    m_iTrackGain = tag.m_iTrackGain;
    m_iAlbumGain = tag.m_iAlbumGain;
    m_fTrackPeak = tag.m_fTrackPeak;
    m_fAlbumPeak = tag.m_fAlbumPeak;
    m_iHasGainInfo = tag.m_iHasGainInfo;
    
    memcpy(&m_dwReleaseDate, &tag.m_dwReleaseDate, sizeof(m_dwReleaseDate) );
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

bool CPictureInfoTag::operator !=(const CPictureInfoTag& tag) const
{
    if (this == &tag) return false;
    if (m_strURL != tag.m_strURL) return true;
    if (m_strTitle != tag.m_strTitle) return true;
    if (m_bCompilation != tag.m_bCompilation) return true;
    if (m_face != tag.m_face) return true;
    if (m_albumFace != tag.m_albumFace) return true;
    if (m_strAlbum != tag.m_strAlbum) return true;
    if (m_iDuration != tag.m_iDuration) return true;
    if (m_iTrack != tag.m_iTrack) return true;
    return false;
}

int CPictureInfoTag::GetTrackNumber() const
{
    return (m_iTrack & 0xffff);
}

int CPictureInfoTag::GetDiscNumber() const
{
    return (m_iTrack >> 16);
}

int CPictureInfoTag::GetTrackAndDiskNumber() const
{
    return m_iTrack;
}

int CPictureInfoTag::GetDuration() const
{
    return m_iDuration;
}

const CStdString& CPictureInfoTag::GetTitle() const
{
    return m_strTitle;
}

const CStdString& CPictureInfoTag::GetURL() const
{
    return m_strURL;
}

const std::vector<std::string>& CPictureInfoTag::GetFace() const
{
    return m_face;
}

const CStdString& CPictureInfoTag::GetAlbum() const
{
    return m_strAlbum;
}

int CPictureInfoTag::GetAlbumId() const
{
    return m_iAlbumId;
}

const std::vector<std::string>& CPictureInfoTag::GetAlbumFace() const
{
    return m_albumFace;
}

const std::vector<std::string>& CPictureInfoTag::GetLocation() const
{
    return m_location;
}

void CPictureInfoTag::GetReleaseDate(SYSTEMTIME& dateTime) const
{
    memcpy(&dateTime, &m_dwReleaseDate, sizeof(m_dwReleaseDate) );
}

int CPictureInfoTag::GetYear() const
{
    return m_dwReleaseDate.wYear;
}

const std::string &CPictureInfoTag::GetType() const
{
    return m_type;
}

CStdString CPictureInfoTag::GetYearString() const
{
    CStdString strReturn;
    strReturn.Format("%i", m_dwReleaseDate.wYear);
    return m_dwReleaseDate.wYear ? strReturn : "";
}

const CStdString &CPictureInfoTag::GetComment() const
{
    return m_strComment;
}

const CStdString &CPictureInfoTag::GetLyrics() const
{
    return m_strLyrics;
}

char CPictureInfoTag::GetRating() const
{
    return m_rating;
}

int CPictureInfoTag::GetListeners() const
{
    return m_listeners;
}

int CPictureInfoTag::GetPlayCount() const
{
    return m_iTimesPlayed;
}

const CDateTime &CPictureInfoTag::GetLastPlayed() const
{
    return m_lastPlayed;
}

bool CPictureInfoTag::GetCompilation() const
{
    return m_bCompilation;
}

const EmbeddedArtInfo &CPictureInfoTag::GetCoverArtInfo() const
{
    return m_coverArt;
}

int CPictureInfoTag::GetReplayGainTrackGain() const
{
    return m_iTrackGain;
}

int CPictureInfoTag::GetReplayGainAlbumGain() const
{
    return m_iAlbumGain;
}

float CPictureInfoTag::GetReplayGainTrackPeak() const
{
    return m_fTrackPeak;
}

float CPictureInfoTag::GetReplayGainAlbumPeak() const
{
    return m_fAlbumPeak;
}

int CPictureInfoTag::HasReplayGainInfo() const
{
    return m_iHasGainInfo;
}

void CPictureInfoTag::SetURL(const CStdString& strURL)
{
    m_strURL = strURL;
}

void CPictureInfoTag::SetTitle(const CStdString& strTitle)
{
    m_strTitle = Trim(strTitle);
}

void CPictureInfoTag::SetFace(const CStdString& strFace)
{
    if (!strFace.empty())
        SetFace(StringUtils::Split(strFace, g_advancedSettings.m_pictureItemSeparator));
    else
        m_face.clear();
}

void CPictureInfoTag::SetFace(const std::vector<std::string>& faces)
{
    m_face = faces;
}

void CPictureInfoTag::SetAlbum(const CStdString& strAlbum)
{
    m_strAlbum = Trim(strAlbum);
}

void CPictureInfoTag::SetAlbumId(const int iAlbumId)
{
    m_iAlbumId = iAlbumId;
}

void CPictureInfoTag::SetAlbumFace(const CStdString& strAlbumFace)
{
    if (!strAlbumFace.empty())
        SetAlbumFace(StringUtils::Split(strAlbumFace, g_advancedSettings.m_pictureItemSeparator));
    else
        m_albumFace.clear();
}

void CPictureInfoTag::SetAlbumFace(const std::vector<std::string>& albumFaces)
{
    m_albumFace = albumFaces;
}

void CPictureInfoTag::SetLocation(const CStdString& strLocation)
{
    if (!strLocation.empty())
        SetLocation(StringUtils::Split(strLocation, g_advancedSettings.m_pictureItemSeparator));
    else
        m_location.clear();
}

void CPictureInfoTag::SetLocation(const std::vector<std::string>& locations)
{
    m_location = locations;
}

void CPictureInfoTag::SetYear(int year)
{
    memset(&m_dwReleaseDate, 0, sizeof(m_dwReleaseDate) );
    m_dwReleaseDate.wYear = year;
}

void CPictureInfoTag::SetDatabaseId(long id, const std::string &type)
{
    m_iDbId = id;
    m_type = type;
}

void CPictureInfoTag::SetReleaseDate(SYSTEMTIME& dateTime)
{
    memcpy(&m_dwReleaseDate, &dateTime, sizeof(m_dwReleaseDate) );
}

void CPictureInfoTag::SetTrackNumber(int iTrack)
{
    m_iTrack = (m_iTrack & 0xffff0000) | (iTrack & 0xffff);
}

void CPictureInfoTag::SetPartOfSet(int iPartOfSet)
{
    m_iTrack = (m_iTrack & 0xffff) | (iPartOfSet << 16);
}

void CPictureInfoTag::SetTrackAndDiskNumber(int iTrackAndDisc)
{
    m_iTrack=iTrackAndDisc;
}

void CPictureInfoTag::SetDuration(int iSec)
{
    m_iDuration = iSec;
}

void CPictureInfoTag::SetComment(const CStdString& comment)
{
    m_strComment = comment;
}

void CPictureInfoTag::SetLyrics(const CStdString& lyrics)
{
    m_strLyrics = lyrics;
}

void CPictureInfoTag::SetRating(char rating)
{
    m_rating = rating;
}

void CPictureInfoTag::SetListeners(int listeners)
{
    m_listeners = listeners;
}

void CPictureInfoTag::SetPlayCount(int playcount)
{
    m_iTimesPlayed = playcount;
}

void CPictureInfoTag::SetLastPlayed(const CStdString& lastplayed)
{
    m_lastPlayed.SetFromDBDateTime(lastplayed);
}

void CPictureInfoTag::SetLastPlayed(const CDateTime& lastplayed)
{
    m_lastPlayed = lastplayed;
}

void CPictureInfoTag::SetCompilation(bool compilation)
{
    m_bCompilation = compilation;
}

void CPictureInfoTag::SetLoaded(bool bOnOff)
{
    m_bLoaded = bOnOff;
}

bool CPictureInfoTag::Loaded() const
{
    return m_bLoaded;
}

const CStdString& CPictureInfoTag::GetPictureBrainzTrackID() const
{
    return m_strPictureBrainzTrackID;
}

const std::vector<std::string>& CPictureInfoTag::GetPictureBrainzFaceID() const
{
    return m_pictureBrainzFaceID;
}

const CStdString& CPictureInfoTag::GetPictureBrainzAlbumID() const
{
    return m_strPictureBrainzAlbumID;
}

const std::vector<std::string>& CPictureInfoTag::GetPictureBrainzAlbumFaceID() const
{
    return m_pictureBrainzAlbumFaceID;
}

const CStdString& CPictureInfoTag::GetPictureBrainzTRMID() const
{
    return m_strPictureBrainzTRMID;
}

void CPictureInfoTag::SetPictureBrainzTrackID(const CStdString& strTrackID)
{
    m_strPictureBrainzTrackID=strTrackID;
}

void CPictureInfoTag::SetPictureBrainzFaceID(const std::vector<std::string>& pictureBrainzFaceId)
{
    m_pictureBrainzFaceID = pictureBrainzFaceId;
}

void CPictureInfoTag::SetPictureBrainzAlbumID(const CStdString& strAlbumID)
{
    m_strPictureBrainzAlbumID=strAlbumID;
}

void CPictureInfoTag::SetPictureBrainzAlbumFaceID(const std::vector<std::string>& pictureBrainzAlbumFaceId)
{
    m_pictureBrainzAlbumFaceID = pictureBrainzAlbumFaceId;
}

void CPictureInfoTag::SetPictureBrainzTRMID(const CStdString& strTRMID)
{
    m_strPictureBrainzTRMID=strTRMID;
}

void CPictureInfoTag::SetCoverArtInfo(size_t size, const std::string &mimeType)
{
    m_coverArt.set(size, mimeType);
}

void CPictureInfoTag::SetReplayGainTrackGain(int trackGain)
{
    m_iTrackGain = trackGain;
    //m_iHasGainInfo |= REPLAY_GAIN_HAS_TRACK_INFO;
}

void CPictureInfoTag::SetReplayGainAlbumGain(int albumGain)
{
    m_iAlbumGain = albumGain;
    //m_iHasGainInfo |= REPLAY_GAIN_HAS_ALBUM_INFO;
}

void CPictureInfoTag::SetReplayGainTrackPeak(float trackPeak)
{
    m_fTrackPeak = trackPeak;
    //m_iHasGainInfo |= REPLAY_GAIN_HAS_TRACK_PEAK;
}

void CPictureInfoTag::SetReplayGainAlbumPeak(float albumPeak)
{
    m_fAlbumPeak = albumPeak;
    //m_iHasGainInfo |= REPLAY_GAIN_HAS_ALBUM_PEAK;
}

void CPictureInfoTag::SetFace(const CFace& face)
{
    SetFace(face.strFace);
    SetAlbumFace(face.strFace);
    SetLocation(face.location);
    m_iDbId = face.idFace;
    m_type = "face";
    m_bLoaded = true;
}

void CPictureInfoTag::SetAlbum(const CPictureAlbum& album)
{
    SetFace(album.face);
    SetAlbumId(album.idAlbum);
    SetAlbum(album.strAlbum);
    SetTitle(album.strAlbum);
    SetAlbumFace(album.face);
    SetLocation(album.location);
    SetRating('0' + album.iRating);
    SetCompilation(album.bCompilation);
    SYSTEMTIME stTime;
    stTime.wYear = album.iYear;
    SetReleaseDate(stTime);
    m_iTimesPlayed = album.iTimesPlayed;
    m_iDbId = album.idAlbum;
    m_type = "album";
    m_bLoaded = true;
}

void CPictureInfoTag::SetPicture(const CPicture& picture)
{
    SetTitle(picture.strTitle);
    SetLocation(picture.location);
    SetFace(picture.face);
    SetAlbum(picture.strAlbum);
    SetAlbumFace(picture.albumFace);
    SetPictureBrainzTrackID(picture.strPictureBrainzTrackID);
    SetComment(picture.strComment);
    SetPlayCount(picture.iTimesPlayed);
    SetLastPlayed(picture.lastPlayed);
    m_rating = picture.rating;
    m_strURL = picture.strFileName;
    SYSTEMTIME stTime;
    stTime.wYear = picture.iYear;
    SetReleaseDate(stTime);
    m_iTrack = picture.iTrack;
    m_iDuration = picture.iDuration;
    m_iDbId = picture.idPicture;
    m_type = "picture";
    m_bLoaded = true;
    m_iTimesPlayed = picture.iTimesPlayed;
    m_iAlbumId = picture.idAlbum;
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
    value["sublocation"] = CStdString(m_iptcInfo.SubLocation);
    value["imagetype"] = CStdString(m_iptcInfo.ImageType);

    
    value["url"] = m_strURL;
    value["title"] = m_strTitle;
    if (m_type.compare("face") == 0 && m_face.size() == 1)
        value["face"] = m_face[0];
    else
        value["face"] = m_face;
    value["displayface"] = StringUtils::Join(m_face, g_advancedSettings.m_pictureItemSeparator);
    value["album"] = m_strAlbum;
    value["albumface"] = m_albumFace;
    value["location"] = m_location;
    value["duration"] = m_iDuration;
    value["track"] = GetTrackNumber();
    value["disc"] = GetDiscNumber();
    value["loaded"] = m_bLoaded;
    value["year"] = m_dwReleaseDate.wYear;
    value["picturebrainztrackid"] = m_strPictureBrainzTrackID;
    value["picturebrainzfaceid"] = StringUtils::Join(m_pictureBrainzFaceID, " / ");
    value["picturebrainzalbumid"] = m_strPictureBrainzAlbumID;
    value["picturebrainzalbumfaceid"] = StringUtils::Join(m_pictureBrainzAlbumFaceID, " / ");
    value["picturebrainztrmid"] = m_strPictureBrainzTRMID;
    value["comment"] = m_strComment;
    value["rating"] = (int)(m_rating - '0');
    value["playcount"] = m_iTimesPlayed;
    value["lastplayed"] = m_lastPlayed.IsValid() ? m_lastPlayed.GetAsDBDateTime() : StringUtils::EmptyString;
    value["lyrics"] = m_strLyrics;
    value["albumid"] = m_iAlbumId;
    value["compilationface"] = m_bCompilation;
}

void CPictureInfoTag::Clear()
{
    m_strURL.Empty();
    m_face.clear();
    m_strAlbum.Empty();
    m_albumFace.clear();
    m_location.clear();
    m_strTitle.Empty();
    m_strPictureBrainzTrackID.Empty();
    m_pictureBrainzFaceID.clear();
    m_strPictureBrainzAlbumID.Empty();
    m_pictureBrainzAlbumFaceID.clear();
    m_strPictureBrainzTRMID.Empty();
    m_iDuration = 0;
    m_iTrack = 0;
    m_bLoaded = false;
    m_lastPlayed.Reset();
    m_bCompilation = false;
    m_strComment.Empty();
    m_rating = '0';
    m_iDbId = -1;
    m_type.clear();
    m_iTimesPlayed = 0;
    memset(&m_dwReleaseDate, 0, sizeof(m_dwReleaseDate) );
    m_iAlbumId = -1;
    m_coverArt.clear();
    m_iTrackGain = 0;
    m_iAlbumGain = 0;
    m_fTrackPeak = 0.0f;
    m_fAlbumPeak = 0.0f;
    m_iHasGainInfo = 0;
}

void CPictureInfoTag::AppendFace(const CStdString &face)
{
    for (unsigned int index = 0; index < m_face.size(); index++)
    {
        if (face.Equals(m_face.at(index).c_str()))
            return;
    }
    
    m_face.push_back(face);
}

void CPictureInfoTag::AppendAlbumFace(const CStdString &albumFace)
{
    for (unsigned int index = 0; index < m_albumFace.size(); index++)
    {
        if (albumFace.Equals(m_albumFace.at(index).c_str()))
            return;
    }
    
    m_albumFace.push_back(albumFace);
}

void CPictureInfoTag::AppendLocation(const CStdString &location)
{
    for (unsigned int index = 0; index < m_location.size(); index++)
    {
        if (location.Equals(m_location.at(index).c_str()))
            return;
    }
    
    m_location.push_back(location);
}

CStdString CPictureInfoTag::Trim(const CStdString &value) const
{
    CStdString trimmedValue(value);
    trimmedValue.TrimLeft(' ');
    trimmedValue.TrimRight(" \n\r");
    return trimmedValue;
}