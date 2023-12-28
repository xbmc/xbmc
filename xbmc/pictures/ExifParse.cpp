/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

//--------------------------------------------------------------------------
// Program to pull the EXIF information out of various types of digital
// images and present it in a reasonably consistent way
//
// Original code pulled from 'jhead' by Matthias Wandel (http://www.sentex.net/~mwandel/) - jhead
// Adapted for XBMC by DD.
//--------------------------------------------------------------------------

// Note: Jhead supports TAG_MAKER_NOTE exif field,
//       but that is omitted for now - to make porting easier and addition smaller

#include "ExifParse.h"

#ifdef TARGET_WINDOWS
#include <windows.h>
#else
#include <memory.h>
#include <cstring>
#endif

#include <math.h>
#include <stdint.h>
#include <stdio.h>

#ifndef min
#define min(a,b) (a)>(b)?(b):(a)
#endif
#ifndef max
#define max(a,b) (a)<(b)?(b):(a)
#endif


// Prototypes for exif utility functions.
static void ErrNonfatal(const char* const msg, int a1, int a2);

#define DIR_ENTRY_ADDR(Start, Entry) ((Start)+2+12*(Entry))


//--------------------------------------------------------------------------
// Describes tag values
#define TAG_DESCRIPTION        0x010E
#define TAG_MAKE               0x010F
#define TAG_MODEL              0x0110
#define TAG_ORIENTATION        0x0112
#define TAG_X_RESOLUTION       0x011A           // Not processed. Format rational64u (see http://search.cpan.org/src/EXIFTOOL/Image-ExifTool-6.76/html/TagNames/EXIF.html)
#define TAG_Y_RESOLUTION       0x011B           // Not processed. Format rational64u
#define TAG_RESOLUTION_UNIT    0x0128           // Not processed. Format int16u. Values: 1-none; 2-inches; 3-cm
#define TAG_SOFTWARE           0x0131
#define TAG_DATETIME           0x0132
#define TAG_THUMBNAIL_OFFSET   0x0201
#define TAG_THUMBNAIL_LENGTH   0x0202
#define TAG_Y_CB_CR_POS        0x0213           // Not processed. Format int16u. Values: 1-Centered; 2-Co-sited
#define TAG_EXPOSURETIME       0x829A
#define TAG_FNUMBER            0x829D
#define TAG_EXIF_OFFSET        0x8769
#define TAG_EXPOSURE_PROGRAM   0x8822
#define TAG_GPSINFO            0x8825
#define TAG_ISO_EQUIVALENT     0x8827
#define TAG_EXIF_VERSION       0x9000           // Not processed.
#define TAG_COMPONENT_CFG      0x9101           // Not processed.
#define TAG_DATETIME_ORIGINAL  0x9003
#define TAG_DATETIME_DIGITIZED 0x9004
#define TAG_SHUTTERSPEED       0x9201
#define TAG_APERTURE           0x9202
#define TAG_EXPOSURE_BIAS      0x9204
#define TAG_MAXAPERTURE        0x9205
#define TAG_SUBJECT_DISTANCE   0x9206
#define TAG_METERING_MODE      0x9207
#define TAG_LIGHT_SOURCE       0x9208
#define TAG_FLASH              0x9209
#define TAG_FOCALLENGTH        0x920A
#define TAG_MAKER_NOTE         0x927C           // Not processed yet. Maybe in the future.
#define TAG_USERCOMMENT        0x9286
#define TAG_XP_COMMENT         0x9c9c
#define TAG_FLASHPIX_VERSION   0xA000           // Not processed.
#define TAG_COLOUR_SPACE       0xA001           // Not processed. Format int16u. Values: 1-RGB; 2-Adobe RGB 65535-Uncalibrated
#define TAG_EXIF_IMAGEWIDTH    0xa002
#define TAG_EXIF_IMAGELENGTH   0xa003
#define TAG_INTEROP_OFFSET     0xa005
#define TAG_FOCALPLANEXRES     0xa20E
#define TAG_FOCALPLANEUNITS    0xa210
#define TAG_EXPOSURE_INDEX     0xa215
#define TAG_EXPOSURE_MODE      0xa402
#define TAG_WHITEBALANCE       0xa403
#define TAG_DIGITALZOOMRATIO   0xA404
#define TAG_FOCALLENGTH_35MM   0xa405

#define TAG_GPS_LAT_REF        1
#define TAG_GPS_LAT            2
#define TAG_GPS_LONG_REF       3
#define TAG_GPS_LONG           4
#define TAG_GPS_ALT_REF        5
#define TAG_GPS_ALT            6

//--------------------------------------------------------------------------
// Exif format descriptor stuff
namespace
{
constexpr auto FMT_BYTE = 1;
[[maybe_unused]] constexpr auto FMT_STRING = 2;
constexpr auto FMT_USHORT = 3;
constexpr auto FMT_ULONG = 4;
constexpr auto FMT_URATIONAL = 5;
constexpr auto FMT_SBYTE = 6;
[[maybe_unused]] constexpr auto FMT_UNDEFINED = 7;
constexpr auto FMT_SSHORT = 8;
constexpr auto FMT_SLONG = 9;
constexpr auto FMT_SRATIONAL = 10;
constexpr auto FMT_SINGLE = 11;
constexpr auto FMT_DOUBLE = 12;
// NOTE: Remember to change NUM_FORMATS if you define a new format
constexpr auto NUM_FORMATS = 12;

const unsigned int BytesPerFormat[NUM_FORMATS] = {1, 1, 2, 4, 8, 1, 1, 2, 4, 8, 4, 8};
} // namespace

//--------------------------------------------------------------------------
// Internationalisation string IDs. The enum order must match that in the
// language file (e.g. 'language/resource.language.en_gb/strings.po', and EXIF_PARSE_STRING_ID_BASE
// must match the ID of the first Exif string in that file.
#define EXIF_PARSE_STRING_ID_BASE       21800
enum {
// Distance
  ExifStrDistanceInfinite = EXIF_PARSE_STRING_ID_BASE,
// Whitebalance et.al.
  ExifStrManual,
  ExifStrAuto,
// Flash modes
  ExifStrYes,
  ExifStrNo,
  ExifStrFlashNoStrobe,
  ExifStrFlashStrobe,
  ExifStrFlashManual,
  ExifStrFlashManualNoReturn,
  ExifStrFlashManualReturn,
  ExifStrFlashAuto,
  ExifStrFlashAutoNoReturn,
  ExifStrFlashAutoReturn,
  ExifStrFlashRedEye,
  ExifStrFlashRedEyeNoReturn,
  ExifStrFlashRedEyeReturn,
  ExifStrFlashManualRedEye,
  ExifStrFlashManualRedEyeNoReturn,
  ExifStrFlashManualRedEyeReturn,
  ExifStrFlashAutoRedEye,
  ExifStrFlashAutoRedEyeNoReturn,
  ExifStrFlashAutoRedEyeReturn,
// Light sources
  ExifStrDaylight,
  ExifStrFluorescent,
  ExifStrIncandescent,
  ExifStrFlash,
  ExifStrFineWeather,
  ExifStrShade,
// Metering Mode
  ExifStrMeteringCenter,
  ExifStrMeteringSpot,
  ExifStrMeteringMatrix,
// Exposure Program
  ExifStrExposureProgram,
  ExifStrExposureAperture,
  ExifStrExposureShutter,
  ExifStrExposureCreative,
  ExifStrExposureAction,
  ExifStrExposurePortrait,
  ExifStrExposureLandscape,
// Exposure mode
  ExifStrExposureModeAuto,
// ISO equivalent
  ExifStrIsoEquivalent,
// GPS latitude, longitude, altitude
  ExifStrGpsLatitude,
  ExifStrGpsLongitude,
  ExifStrGpsAltitude,
};




//--------------------------------------------------------------------------
// Report non fatal errors.  Now that microsoft.net modifies exif headers,
// there's corrupted ones, and there could be more in the future.
//--------------------------------------------------------------------------
static void ErrNonfatal(const char* const msg, int a1, int a2)
{
  printf("ExifParse - Nonfatal Error : %s %d %d", msg, a1, a2);
}

//--------------------------------------------------------------------------
// Convert a 16 bit unsigned value from file's native byte order
//--------------------------------------------------------------------------
int CExifParse::Get16(const void* const Short, const bool motorolaOrder)
{
    if (motorolaOrder) {
        return (((const unsigned char *)Short)[0] << 8) | ((const unsigned char *)Short)[1];
    } else {
        return (((const unsigned char *)Short)[1] << 8) | ((const unsigned char *)Short)[0];
    }
}

//--------------------------------------------------------------------------
// Convert a 32 bit signed value from file's native byte order
//--------------------------------------------------------------------------
int CExifParse::Get32(const void* const Long, const bool motorolaOrder)
{
    if (motorolaOrder) {
        return  (((const char *)Long)[0] << 24) | (((const unsigned char *)Long)[1] << 16)
          | (((const unsigned char *)Long)[2] << 8 ) | (((const unsigned char *)Long)[3] << 0 );
    } else {
        return  (((const char *)Long)[3] << 24) | (((const unsigned char *)Long)[2] << 16)
          | (((const unsigned char *)Long)[1] << 8 ) | (((const unsigned char *)Long)[0] << 0 );
    }
}

//--------------------------------------------------------------------------
// It appears that CStdString constructor replaces "\n" with "\r\n" which results
// in "\r\r\n" if there already is "\r\n", which in turn results in corrupted
// display. So this is an attempt to undo effects of a smart constructor. Also,
// replaces all nonprintable characters with "."
//--------------------------------------------------------------------------
/*void CExifParse::FixComment(CStdString& comment)
{
  comment.Replace("\r\r\n", "\r\n");
  for (unsigned int i=0; i<comment.length(); i++)
  {
    if ((comment[i] < 32) && (comment[i] != '\n') && (comment[i] != '\t') && (comment[i] != '\r'))
    {
      comment[i] = '.';
    }
  }
}*/

//--------------------------------------------------------------------------
// Evaluate number, be it int, rational, or float from directory.
//--------------------------------------------------------------------------
double CExifParse::ConvertAnyFormat(const void* const ValuePtr, int Format)
{
  double Value;
  Value = 0;

  switch(Format)
  {
    case FMT_SBYTE:     Value = *(const   signed char*)ValuePtr;          break;
    case FMT_BYTE:      Value = *(const unsigned char*)ValuePtr;          break;

    case FMT_USHORT:    Value = Get16(ValuePtr, m_MotorolaOrder);   break;
    case FMT_ULONG:     Value = (unsigned)Get32(ValuePtr, m_MotorolaOrder);   break;

    case FMT_URATIONAL:
    case FMT_SRATIONAL:
    {
      int Num,Den;
      Num = Get32(ValuePtr, m_MotorolaOrder);
      Den = Get32(4+(const char *)ValuePtr, m_MotorolaOrder);

      if (Den == 0)    Value = 0;
      else             Value = (double)Num/Den;
    }
    break;

    case FMT_SSHORT:    Value = (signed short)Get16(ValuePtr, m_MotorolaOrder);    break;
    case FMT_SLONG:     Value = Get32(ValuePtr, m_MotorolaOrder);                  break;

    // Not sure if this is correct (never seen float used in Exif format)
    case FMT_SINGLE:    Value = (double)*(const float*)ValuePtr;          break;
    case FMT_DOUBLE:    Value = *(const double*)ValuePtr;                 break;

    default:
      ErrNonfatal("Illegal format code %d",Format,0);
  }
  return Value;
}

//--------------------------------------------------------------------------
// Exif date tag is stored as a fixed format string "YYYY:MM:DD HH:MM:SS".
// If date is not set, then the string is filled with blanks and colons:
// "    :  :     :  :  ". We want this string localised.
//--------------------------------------------------------------------------
/*void CExifParse::LocaliseDate (void)
{
    if (m_ExifInfo[SLIDESHOW_EXIF_DATE_TIME][0] != ' ')
    {
        int year  = atoi(m_ExifInfo[SLIDESHOW_EXIF_DATE_TIME].substr(0, 4).c_str());
        int month = atoi(m_ExifInfo[SLIDESHOW_EXIF_DATE_TIME].substr(5, 2).c_str());
        int day   = atoi(m_ExifInfo[SLIDESHOW_EXIF_DATE_TIME].substr(8, 2).c_str());
        int hour  = atoi(m_ExifInfo[SLIDESHOW_EXIF_DATE_TIME].substr(11,2).c_str());
        int min   = atoi(m_ExifInfo[SLIDESHOW_EXIF_DATE_TIME].substr(14,2).c_str());
        int sec   = atoi(m_ExifInfo[SLIDESHOW_EXIF_DATE_TIME].substr(17,2).c_str());
        CDateTime date(year, month, day, hour, min, sec);
        m_ExifInfo[SLIDESHOW_EXIF_DATE_TIME] = date.GetAsLocalizedDateTime();
    }
}*/


//--------------------------------------------------------------------------
// Convert exposure time into a human readable format
//--------------------------------------------------------------------------
/*void CExifParse::GetExposureTime(const float exposureTime, CStdString& outStr)
{
  if (exposureTime)
  {
    if (exposureTime < 0.010)   outStr.Format("%6.4fs ", exposureTime);
    else                        outStr.Format("%5.3fs ", exposureTime);
    if (exposureTime <= 0.5)    outStr.Format("%s (1/%d)", outStr, (int)(0.5 + 1/exposureTime));
  }
}*/

//--------------------------------------------------------------------------
// Process one of the nested EXIF directories.
//--------------------------------------------------------------------------
void CExifParse::ProcessDir(const unsigned char* const DirStart,
                            const unsigned char* const OffsetBase,
                            const unsigned ExifLength,
                            int NestingLevel)
{
  if (NestingLevel > 4)
  {
    ErrNonfatal("Maximum directory nesting exceeded (corrupt exif header)", 0,0);
    return;
  }

  char IndentString[25];
  memset(IndentString, ' ', 25);
  IndentString[NestingLevel * 4] = '\0';


  int NumDirEntries = Get16((const void*)DirStart, m_MotorolaOrder);

  const unsigned char* const DirEnd = DIR_ENTRY_ADDR(DirStart, NumDirEntries);
  if (DirEnd+4 > (OffsetBase+ExifLength))
  {
    if (DirEnd+2 == OffsetBase+ExifLength || DirEnd == OffsetBase+ExifLength)
    {
      // Version 1.3 of jhead would truncate a bit too much.
      // This also caught later on as well.
    }
    else
    {
      ErrNonfatal("Illegally sized directory", 0,0);
      return;
    }
  }

  for (int de=0;de<NumDirEntries;de++)
  {
    int Tag, Format, Components;
    unsigned char* ValuePtr;
    int ByteCount;
    const unsigned char* const DirEntry = DIR_ENTRY_ADDR(DirStart, de);

    Tag = Get16(DirEntry, m_MotorolaOrder);
    Format = Get16(DirEntry+2, m_MotorolaOrder);
    Components = Get32(DirEntry+4, m_MotorolaOrder);

    if (Format <= 0 || Format > NUM_FORMATS)
    {
      ErrNonfatal("Illegal number format %d for tag %04x", Format, Tag);
      continue;
    }

    if ((unsigned)Components > 0x10000)
    {
      ErrNonfatal("Illegal number of components %d for tag %04x", Components, Tag);
      continue;
    }

    ByteCount = Components * BytesPerFormat[Format - 1];

    if (ByteCount > 4)
    {
      unsigned OffsetVal;
      OffsetVal = (unsigned)Get32(DirEntry+8, m_MotorolaOrder);
      // If its bigger than 4 bytes, the dir entry contains an offset.
      if (OffsetVal > UINT32_MAX - ByteCount || OffsetVal + ByteCount > ExifLength)
      {
        // Bogus pointer offset and / or bytecount value
        ErrNonfatal("Illegal value pointer for tag %04x", Tag,0);
        continue;
      }
      ValuePtr = (unsigned char*)(const_cast<unsigned char*>(OffsetBase)+OffsetVal);

      if (OffsetVal > m_LargestExifOffset)
      {
        m_LargestExifOffset = OffsetVal;
      }

    }
    else {
      // 4 bytes or less and value is in the dir entry itself
      ValuePtr = (unsigned char*)(const_cast<unsigned char*>(DirEntry)+8);
    }


    // Extract useful components of tag
    switch(Tag)
    {
      case TAG_DESCRIPTION:
      {
        int length = max(ByteCount, 0);
        length = min(length, MAX_COMMENT);
        strncpy(m_ExifInfo->Description, (char *)ValuePtr, length);
        m_ExifInfo->Description[length] = '\0';
        break;
      }
      case TAG_MAKE:
      {
        int space = sizeof(m_ExifInfo->CameraMake);
        if (space > 0)
        {
          strncpy(m_ExifInfo->CameraMake, (char *)ValuePtr, space - 1);
          m_ExifInfo->CameraMake[space - 1] = '\0';
        }
        break;
      }
      case TAG_MODEL:
      {
        int space = sizeof(m_ExifInfo->CameraModel);
        if (space > 0)
        {
          strncpy(m_ExifInfo->CameraModel, (char *)ValuePtr, space - 1);
          m_ExifInfo->CameraModel[space - 1] = '\0';
        }
        break;
      }
//      case TAG_SOFTWARE:          strncpy(m_ExifInfo->Software, ValuePtr, 5);    break;
      case TAG_FOCALPLANEXRES:    m_FocalPlaneXRes  = ConvertAnyFormat(ValuePtr, Format);               break;
      case TAG_THUMBNAIL_OFFSET:  m_ExifInfo->ThumbnailOffset = (unsigned)ConvertAnyFormat(ValuePtr, Format);     break;
      case TAG_THUMBNAIL_LENGTH:  m_ExifInfo->ThumbnailSize   = (unsigned)ConvertAnyFormat(ValuePtr, Format);     break;

      case TAG_MAKER_NOTE:
        continue;
      break;

      case TAG_DATETIME_ORIGINAL:
      {

        int space = sizeof(m_ExifInfo->DateTime);
        if (space > 0)
        {
          strncpy(m_ExifInfo->DateTime, (char *)ValuePtr, space - 1);
          m_ExifInfo->DateTime[space - 1] = '\0';
          // If we get a DATETIME_ORIGINAL, we use that one.
          m_DateFound = true;
        }
        break;
      }
      case TAG_DATETIME_DIGITIZED:
      case TAG_DATETIME:
      {
        if (!m_DateFound)
        {
          // If we don't already have a DATETIME_ORIGINAL, use whatever
          // time fields we may have.
          int space = sizeof(m_ExifInfo->DateTime);
          if (space > 0)
          {
            strncpy(m_ExifInfo->DateTime, (char *)ValuePtr, space - 1);
            m_ExifInfo->DateTime[space - 1] = '\0';
          }
        }
        break;
      }
      case TAG_USERCOMMENT:
      {
        // The UserComment allows comments without the charset limitations of ImageDescription.
        // Therefore the UserComment field is prefixed by a CharacterCode field (8 Byte):
        //  - ASCII:         'ASCII\0\0\0'
        //  - Unicode:       'UNICODE\0'
        //  - JIS X208-1990: 'JIS\0\0\0\0\0'
        //  - Unknown:       '\0\0\0\0\0\0\0\0' (application specific)

        m_ExifInfo->CommentsCharset = EXIF_COMMENT_CHARSET_UNKNOWN;

        const int EXIF_COMMENT_CHARSET_LENGTH = 8;
        if (ByteCount >= EXIF_COMMENT_CHARSET_LENGTH)
        {
          // As some implementations use spaces instead of \0 for the padding,
          // we're not so strict and check only the prefix.
          if (memcmp(ValuePtr, "ASCII", 5) == 0)
            m_ExifInfo->CommentsCharset = EXIF_COMMENT_CHARSET_ASCII;
          else if (memcmp(ValuePtr, "UNICODE", 7) == 0)
            m_ExifInfo->CommentsCharset = EXIF_COMMENT_CHARSET_UNICODE;
          else if (memcmp(ValuePtr, "JIS", 3) == 0)
            m_ExifInfo->CommentsCharset = EXIF_COMMENT_CHARSET_JIS;

          int length = ByteCount - EXIF_COMMENT_CHARSET_LENGTH;
          length = min(length, MAX_COMMENT);
          memcpy(m_ExifInfo->Comments, ValuePtr + EXIF_COMMENT_CHARSET_LENGTH, length);
          m_ExifInfo->Comments[length] = '\0';
//          FixComment(comment);                          // Ensure comment is printable
        }
      }
      break;

      case TAG_XP_COMMENT:
      {
        // The XP user comment field is always unicode (UCS-2) encoded
        m_ExifInfo->XPCommentsCharset = EXIF_COMMENT_CHARSET_UNICODE;
        size_t length = min(ByteCount, MAX_COMMENT);
        memcpy(m_ExifInfo->XPComment, ValuePtr, length);
        m_ExifInfo->XPComment[length] = '\0';
      }
      break;

      case TAG_FNUMBER:
        // Simplest way of expressing aperture, so I trust it the most.
        // (overwrite previously computd value if there is one)
        m_ExifInfo->ApertureFNumber = (float)ConvertAnyFormat(ValuePtr, Format);
      break;

      case TAG_APERTURE:
      case TAG_MAXAPERTURE:
        // More relevant info always comes earlier, so only use this field if we don't
        // have appropriate aperture information yet.
        if (m_ExifInfo->ApertureFNumber == 0)
        {
          m_ExifInfo->ApertureFNumber = (float)exp(ConvertAnyFormat(ValuePtr, Format)*log(2.0)*0.5);
        }
      break;

      case TAG_FOCALLENGTH:
        // Nice digital cameras actually save the focal length as a function
        // of how far they are zoomed in.
        m_ExifInfo->FocalLength = (float)ConvertAnyFormat(ValuePtr, Format);
      break;

      case TAG_SUBJECT_DISTANCE:
        // Inidcates the distacne the autofocus camera is focused to.
        // Tends to be less accurate as distance increases.
        {
          float distance = (float)ConvertAnyFormat(ValuePtr, Format);
          m_ExifInfo->Distance = distance;
        }
      break;

      case TAG_EXPOSURETIME:
        {
        // Simplest way of expressing exposure time, so I trust it most.
        // (overwrite previously computd value if there is one)
        float expTime = (float)ConvertAnyFormat(ValuePtr, Format);
        if (expTime)
          m_ExifInfo->ExposureTime = expTime;
        }
      break;

      case TAG_SHUTTERSPEED:
        // More complicated way of expressing exposure time, so only use
        // this value if we don't already have it from somewhere else.
        if (m_ExifInfo->ExposureTime == 0)
        {
          m_ExifInfo->ExposureTime = (float)(1/exp(ConvertAnyFormat(ValuePtr, Format)*log(2.0)));
        }
      break;

      case TAG_FLASH:
        m_ExifInfo->FlashUsed = (int)ConvertAnyFormat(ValuePtr, Format);
      break;

      case TAG_ORIENTATION:
        m_ExifInfo->Orientation = (int)ConvertAnyFormat(ValuePtr, Format);
        if (m_ExifInfo->Orientation < 0 || m_ExifInfo->Orientation > 8)
        {
          ErrNonfatal("Undefined rotation value %d", m_ExifInfo->Orientation, 0);
          m_ExifInfo->Orientation = 0;
        }
      break;

      case TAG_EXIF_IMAGELENGTH:
      case TAG_EXIF_IMAGEWIDTH:
        // Use largest of height and width to deal with images that have been
        // rotated to portrait format.
        {
          int a = (int)ConvertAnyFormat(ValuePtr, Format);
          if (m_ExifImageWidth < a) m_ExifImageWidth = a;
        }
      break;

      case TAG_FOCALPLANEUNITS:
        switch((int)ConvertAnyFormat(ValuePtr, Format))
        {
          // According to the information I was using, 2 means meters.
          // But looking at the Cannon powershot's files, inches is the only
          // sensible value.
          case 1: m_FocalPlaneUnits = 25.4; break;  // inch
          case 2: m_FocalPlaneUnits = 25.4; break;
          case 3: m_FocalPlaneUnits = 10;   break;  // centimeter
          case 4: m_FocalPlaneUnits = 1;    break;  // millimeter
          case 5: m_FocalPlaneUnits = .001; break;  // micrometer
        }
      break;

      case TAG_EXPOSURE_BIAS:
        m_ExifInfo->ExposureBias = (float)ConvertAnyFormat(ValuePtr, Format);
      break;

      case TAG_WHITEBALANCE:
        m_ExifInfo->Whitebalance = (int)ConvertAnyFormat(ValuePtr, Format);
      break;

      case TAG_LIGHT_SOURCE:
        //Quercus: 17-1-2004 Added LightSource, some cams return this, whitebalance or both
        m_ExifInfo->LightSource = (int)ConvertAnyFormat(ValuePtr, Format);
      break;

      case TAG_METERING_MODE:
        m_ExifInfo->MeteringMode = (int)ConvertAnyFormat(ValuePtr, Format);
      break;

      case TAG_EXPOSURE_PROGRAM:
        m_ExifInfo->ExposureProgram = (int)ConvertAnyFormat(ValuePtr, Format);
      break;

      case TAG_EXPOSURE_INDEX:
        if (m_ExifInfo->ISOequivalent == 0)
        {
          // Exposure index and ISO equivalent are often used interchangeably,
          // so we will do the same.
          // http://photography.about.com/library/glossary/bldef_ei.htm
          m_ExifInfo->ISOequivalent = (int)ConvertAnyFormat(ValuePtr, Format);
        }
      break;

      case TAG_ISO_EQUIVALENT:
        m_ExifInfo->ISOequivalent = (int)ConvertAnyFormat(ValuePtr, Format);
        if (m_ExifInfo->ISOequivalent < 50)
          m_ExifInfo->ISOequivalent *= 200;          // Fixes strange encoding on some older digicams.
      break;

      case TAG_EXPOSURE_MODE:
        m_ExifInfo->ExposureMode = (int)ConvertAnyFormat(ValuePtr, Format);
      break;

      case TAG_DIGITALZOOMRATIO:
        m_ExifInfo->DigitalZoomRatio = (float)ConvertAnyFormat(ValuePtr, Format);
      break;

      case TAG_EXIF_OFFSET:
      case TAG_INTEROP_OFFSET:
      {
        const unsigned char* const SubdirStart = OffsetBase + (unsigned)Get32(ValuePtr, m_MotorolaOrder);
        if (SubdirStart < OffsetBase || SubdirStart > OffsetBase+ExifLength)
        {
          ErrNonfatal("Illegal exif or interop ofset directory link",0,0);
        }
        else
        {
          ProcessDir(SubdirStart, OffsetBase, ExifLength, NestingLevel+1);
        }
        continue;
      }
      break;

      case TAG_GPSINFO:
      {
        const unsigned char* const SubdirStart = OffsetBase + (unsigned)Get32(ValuePtr, m_MotorolaOrder);
        if (SubdirStart < OffsetBase || SubdirStart > OffsetBase+ExifLength)
        {
          ErrNonfatal("Illegal GPS directory link",0,0);
        }
        else
        {
          ProcessGpsInfo(SubdirStart, ByteCount, OffsetBase, ExifLength);
        }
        continue;
      }
      break;

      case TAG_FOCALLENGTH_35MM:
        // The focal length equivalent 35 mm is a 2.2 tag (defined as of April 2002)
        // if its present, use it to compute equivalent focal length instead of
        // computing it from sensor geometry and actual focal length.
        m_ExifInfo->FocalLength35mmEquiv = (unsigned)ConvertAnyFormat(ValuePtr, Format);
      break;
    }
  }


  // In addition to linking to subdirectories via exif tags,
  // there's also a potential link to another directory at the end of each
  // directory.  this has got to be the result of a committee!
  unsigned Offset;

  if (DIR_ENTRY_ADDR(DirStart, NumDirEntries) + 4 <= OffsetBase+ExifLength)
  {
    Offset = (unsigned)Get32(DirStart+2+12*NumDirEntries, m_MotorolaOrder);
    if (Offset)
    {
      const unsigned char* const SubdirStart = OffsetBase + Offset;
      if (SubdirStart > OffsetBase+ExifLength || SubdirStart < OffsetBase)
      {
        if (SubdirStart > OffsetBase && SubdirStart < OffsetBase+ExifLength+20)
        {
          // Jhead 1.3 or earlier would crop the whole directory!
          // As Jhead produces this form of format incorrectness,
          // I'll just let it pass silently
        }
        else
        {
          ErrNonfatal("Illegal subdirectory link",0,0);
        }
      }
      else
      {
        if (SubdirStart <= OffsetBase+ExifLength)
        {
          ProcessDir(SubdirStart, OffsetBase, ExifLength, NestingLevel+1);
        }
      }
      if (Offset > m_LargestExifOffset)
      {
        m_LargestExifOffset = Offset;
      }
    }
  }
  else
  {
    // The exif header ends before the last next directory pointer.
  }

  if (m_ExifInfo->ThumbnailOffset)
  {
    m_ExifInfo->ThumbnailAtEnd = false;

    if (m_ExifInfo->ThumbnailOffset <= ExifLength)
    {
      if (m_ExifInfo->ThumbnailSize > ExifLength - m_ExifInfo->ThumbnailOffset)
      {
        // If thumbnail extends past exif header, only save the part that
        // actually exists.  Canon's EOS viewer utility will do this - the
        // thumbnail extracts ok with this hack.
        m_ExifInfo->ThumbnailSize = ExifLength - m_ExifInfo->ThumbnailOffset;
      }
    }
  }
}


//--------------------------------------------------------------------------
// Process a EXIF marker
// Describes all the drivel that most digital cameras include...
//--------------------------------------------------------------------------
bool CExifParse::Process (const unsigned char* const ExifSection, const unsigned short length, ExifInfo_t *info)
{
  m_ExifInfo = info;
  // EXIF signature: "Exif\0\0"
  // Check EXIF signatures
  const char ExifHeader[]     = "Exif\0\0";
  const char ExifAlignment0[] = "II";
  const char ExifAlignment1[] = "MM";
  const char ExifExtra        = 0x2a;

  const char* pos = (const char*)(ExifSection + sizeof(short));   // position data pointer after length field

  if (memcmp(pos, ExifHeader,6))
  {
    printf("ExifParse: incorrect Exif header");
    return false;
  }
  pos += 6;

  if (memcmp(pos, ExifAlignment0, strlen(ExifAlignment0)) == 0)
  {
    m_MotorolaOrder = false;
  }
  else if (memcmp(pos, ExifAlignment1, strlen(ExifAlignment1)) == 0)
  {
    m_MotorolaOrder = true;
  }
  else
  {
    printf("ExifParse: invalid Exif alignment marker");
    return false;
  }
  pos += strlen(ExifAlignment0);

  // Check the next value for correctness.
  if (Get16((const void*)(pos), m_MotorolaOrder) != ExifExtra)
  {
    printf("ExifParse: invalid Exif start (1)");
    return false;
  }
  pos += sizeof(short);

  unsigned long FirstOffset = (unsigned)Get32((const void*)pos, m_MotorolaOrder);
  if (FirstOffset < 8 || FirstOffset + 8 >= length)
  {
    ErrNonfatal("Invalid offset of first IFD value: %u", FirstOffset, 0);
    return false;
  }



  // First directory starts 16 bytes in.  All offset are relative to 8 bytes in.
  ProcessDir(ExifSection+8+FirstOffset, ExifSection+8, length-8, 0);

  m_ExifInfo->ThumbnailAtEnd = m_ExifInfo->ThumbnailOffset >= m_LargestExifOffset;

  // Compute the CCD width, in millimeters.
  if (m_FocalPlaneXRes != 0)
  {
    // Note: With some cameras, its not possible to compute this correctly because
    // they don't adjust the indicated focal plane resolution units when using less
    // than maximum resolution, so the CCDWidth value comes out too small.  Nothing
    // that Jhead can do about it - its a camera problem.
    m_ExifInfo->CCDWidth = (float)(m_ExifImageWidth * m_FocalPlaneUnits / m_FocalPlaneXRes);
  }

  if (m_ExifInfo->FocalLength)
  {
    if (m_ExifInfo->FocalLength35mmEquiv == 0)
    {
      // Compute 35 mm equivalent focal length based on sensor geometry if we haven't
      // already got it explicitly from a tag.
      if (m_ExifInfo->CCDWidth != 0.0f)
      {
        m_ExifInfo->FocalLength35mmEquiv =
            (int)(m_ExifInfo->FocalLength / m_ExifInfo->CCDWidth * 36 + 0.5f);
      }
    }
  }
  return true;
}



//--------------------------------------------------------------------------
// GPS Lat/Long extraction helper function
//--------------------------------------------------------------------------
void CExifParse::GetLatLong(
        const unsigned int Format,
        const unsigned char* ValuePtr,
        const int ComponentSize,
        char *latLongString)
{
  if (Format != FMT_URATIONAL)
  {
    ErrNonfatal("Illegal number format %d for GPS Lat/Long", Format, 0);
  }
  else
  {
    double Values[3];
    for (unsigned a=0; a<3 ;a++)
    {
      Values[a] = ConvertAnyFormat(ValuePtr+a*ComponentSize, Format);
    }
    if (Values[0] < 0 || Values[0] > 180 || Values[1] < 0 || Values[1] >= 60 || Values[2] < 0 || Values[2] >= 60)
    {
      // Ignore invalid values (DMS format expected)
      ErrNonfatal("Invalid Lat/Long value", 0, 0);
      latLongString[0] = 0;
    }
    else
    {
      char latLong[30];
      snprintf(latLong, sizeof(latLong), "%3.0fd %2.0f' %5.2f\"", Values[0], Values[1], Values[2]);
      strcat(latLongString, latLong);
    }
  }
}

//--------------------------------------------------------------------------
// Process GPS info directory
//--------------------------------------------------------------------------
void CExifParse::ProcessGpsInfo(
                    const unsigned char* const DirStart,
                    int ByteCountUnused,
                    const unsigned char* const OffsetBase,
                    unsigned ExifLength)
{
  int NumDirEntries = Get16(DirStart, m_MotorolaOrder);

  for (int de=0;de<NumDirEntries;de++)
  {
    const unsigned char* DirEntry = DIR_ENTRY_ADDR(DirStart, de);

    // Fix from aosp 34a2564d3268a5ca1472c5076675782fbaf724d6
    if (DirEntry + 12 > OffsetBase + ExifLength)
    {
      ErrNonfatal("GPS info directory goes past end of exif", 0, 0);
      return;
    }

    unsigned Tag        = Get16(DirEntry, m_MotorolaOrder);
    unsigned Format     = Get16(DirEntry+2, m_MotorolaOrder);
    unsigned Components = (unsigned)Get32(DirEntry+4, m_MotorolaOrder);
    if (Format == 0 || Format > NUM_FORMATS)
    {
      ErrNonfatal("Illegal number format %d for tag %04x", Format, Tag);
      continue;
    }

    unsigned ComponentSize = BytesPerFormat[Format - 1];
    unsigned ByteCount = Components * ComponentSize;

    const unsigned char* ValuePtr;

    if (ByteCount > 4)
    {
      unsigned OffsetVal = (unsigned)Get32(DirEntry+8, m_MotorolaOrder);
      // If its bigger than 4 bytes, the dir entry contains an offset.
      if (OffsetVal > UINT32_MAX - ByteCount || OffsetVal + ByteCount > ExifLength)
      {
        // Bogus pointer offset and / or bytecount value
        ErrNonfatal("Illegal value pointer for tag %04x", Tag,0);
        continue;
      }
      ValuePtr = OffsetBase+OffsetVal;
    }
    else
    {
      // 4 bytes or less and value is in the dir entry itself
      ValuePtr = DirEntry+8;
    }

    switch(Tag)
    {
      case TAG_GPS_LAT_REF:
        m_ExifInfo->GpsLat[0] = ValuePtr[0];
        m_ExifInfo->GpsLat[1] = 0;
      break;

      case TAG_GPS_LONG_REF:
        m_ExifInfo->GpsLong[0] = ValuePtr[0];
        m_ExifInfo->GpsLong[1] = 0;
      break;

      case TAG_GPS_LAT:
        GetLatLong(Format, ValuePtr, ComponentSize, m_ExifInfo->GpsLat);
      break;
      case TAG_GPS_LONG:
        GetLatLong(Format, ValuePtr, ComponentSize, m_ExifInfo->GpsLong);
      break;

      case TAG_GPS_ALT_REF:
        if (ValuePtr[0] != 0)
          m_ExifInfo->GpsAlt[0] = '-';
        m_ExifInfo->GpsAlt[1] = 0;
      break;

      case TAG_GPS_ALT:
        {
          char temp[18];
          snprintf(temp, sizeof(temp), "%.2fm",
                   static_cast<double>(ConvertAnyFormat(ValuePtr, Format)));
          strcat(m_ExifInfo->GpsAlt, temp);
        }
      break;
    }
  }
}

