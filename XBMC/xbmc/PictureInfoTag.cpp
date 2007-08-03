#include "stdafx.h"
#include "PictureInfoTag.h"
#include "DateTime.h"
#include "Util.h"

void CPictureInfoTag::Reset()
{
  memset(&m_exifInfo, 0, sizeof(m_exifInfo));
  memset(&m_iptcInfo, 0, sizeof(m_iptcInfo));
  m_isLoaded = false;
  m_filePath.Empty();
}

bool CPictureInfoTag::Load(const CStdString &path)
{
  m_isLoaded = false;
  m_filePath = path;

  DllLibExif exifDll;
  if (path.IsEmpty() || !exifDll.Load())
    return false;

  if (exifDll.process_jpeg(path.c_str(), &m_exifInfo, &m_iptcInfo))
    m_isLoaded = true;

  return m_isLoaded;
}

void CPictureInfoTag::Serialize(CArchive& ar)
{
  /*
  if (ar.IsStoring())
  {
    ar << m_exifInfo;
    ar << m_iptcInfo;
  }
  else
  {
    ar >> m_exifInfo;
    ar >> m_iptcInfo;
  }*/
}

CStdString CPictureInfoTag::GetInfo(int info) const
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
    value = m_exifInfo.Comments;
    break;
  case SLIDE_EXIF_DATE_TIME:
    if (m_exifInfo.DateTime && m_exifInfo.DateTime[0] != ' ')
    {
      CStdString dateTime = m_exifInfo.DateTime;
      int year  = atoi(dateTime.Mid(0, 4).c_str());
      int month = atoi(dateTime.Mid(5, 2).c_str());
      int day   = atoi(dateTime.Mid(8, 2).c_str());
      int hour  = atoi(dateTime.Mid(11,2).c_str());
      int min   = atoi(dateTime.Mid(14,2).c_str());
      int sec   = atoi(dateTime.Mid(17,2).c_str());
      CDateTime date(year, month, day, hour, min, sec);
      value = date.GetAsLocalizedDateTime();
    }
    break;
//  case SLIDE_EXIF_DESCRIPTION:
//    value = m_exifInfo.Description;
//    break;
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