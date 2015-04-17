/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

//--------------------------------------------------------------------------
// This module gathers information about a digital image file. This includes:
//   - File name and path
//   - File size
//   - Resolution (if available)
//   - IPTC information (if available)
//   - EXIF information (if available)
// All gathered information is stored in a vector of 'description' and 'value'
// pairs (where both description and value fields are of CStdString types).
//--------------------------------------------------------------------------

#ifndef _LINUX
#include <windows.h>
#else
#include <memory.h>
#include <cstring>
#define min(a,b) (a)>(b)?(b):(a)
typedef unsigned char BYTE;
#endif
#include "JpegParse.h"



//--------------------------------------------------------------------------
#define JPEG_PARSE_STRING_ID_BASE       21500
enum {
  ProcessUnknown = JPEG_PARSE_STRING_ID_BASE,
  ProcessSof0,
  ProcessSof1,
  ProcessSof2,
  ProcessSof3,
  ProcessSof5,
  ProcessSof6,
  ProcessSof7,
  ProcessSof9,
  ProcessSof10,
  ProcessSof11,
  ProcessSof13,
  ProcessSof14,
  ProcessSof15,
};




//--------------------------------------------------------------------------
// Constructor
//--------------------------------------------------------------------------
CJpegParse::CJpegParse():
  m_SectionBuffer(NULL)
{
  memset(&m_ExifInfo, 0, sizeof(m_ExifInfo));
  memset(&m_IPTCInfo, 0, sizeof(m_IPTCInfo));
}

//--------------------------------------------------------------------------
// Process a SOFn marker.  This is useful for the image dimensions
//--------------------------------------------------------------------------
void CJpegParse::ProcessSOFn (void)
{
  m_ExifInfo.Height = CExifParse::Get16(m_SectionBuffer+3);
  m_ExifInfo.Width  = CExifParse::Get16(m_SectionBuffer+5);

  unsigned char num_components = m_SectionBuffer[7];
  if (num_components != 3)
  {
    m_ExifInfo.IsColor = 0;
  }
  else
  {
    m_ExifInfo.IsColor = 1;
  }
}


//--------------------------------------------------------------------------
// Read a section from a JPEG file. Note that this function allocates memory.
// It must be called in pair with ReleaseSection
//--------------------------------------------------------------------------
bool CJpegParse::GetSection (FILE *infile, const unsigned short sectionLength)
{
  m_SectionBuffer = new unsigned char[sectionLength];
  if (m_SectionBuffer == NULL)
  {
    printf("JpgParse: could not allocate memory");
    return false;
  }
  // Store first two pre-read bytes.
  m_SectionBuffer[0] = (unsigned char)(sectionLength >> 8);
  m_SectionBuffer[1] = (unsigned char)(sectionLength & 0x00FF);

  unsigned int len = (unsigned int)sectionLength;

  size_t bytesRead = fread(m_SectionBuffer+sizeof(sectionLength), 1, len-sizeof(sectionLength), infile);
  if (bytesRead != sectionLength-sizeof(sectionLength))
  {
    printf("JpgParse: premature end of file?");
    ReleaseSection();
    return false;
  }
  return true;
}

//--------------------------------------------------------------------------
// Deallocate memory allocated in GetSection. This function must always
// be paired by a preceeding GetSection call.
//--------------------------------------------------------------------------
void CJpegParse::ReleaseSection (void)
{
  delete[] m_SectionBuffer;
  m_SectionBuffer = NULL;
}

//--------------------------------------------------------------------------
// Parse the marker stream until SOS or EOI is seen; infile has already been
// successfully open
//--------------------------------------------------------------------------
bool CJpegParse::ExtractInfo (FILE *infile)
{
  // Get file marker (two bytes - must be 0xFFD8 for JPEG files
  BYTE a;
  size_t bytesRead = fread(&a, 1, sizeof(BYTE), infile);
  if ((bytesRead != sizeof(BYTE)) || (a != 0xFF))
  {
    return false;
  }
  bytesRead = fread(&a, 1, sizeof(BYTE), infile);
  if ((bytesRead != sizeof(BYTE)) || (a != M_SOI))
  {
    return false;
  }

  for(;;)
  {
    BYTE marker = 0;
    for (a=0; a<7; a++) {
      bytesRead = fread(&marker, 1, sizeof(BYTE), infile);
      if (marker != 0xFF)
        break;

      if (a >= 6)
      {
        printf("JpgParse: too many padding bytes");
        return false;
      }
      marker = 0;
    }

    if (marker == 0xff)
    {
      // 0xff is legal padding, but if we get that many, something's wrong.
      printf("JpgParse: too many padding bytes");
      return false;
    }

    // Read the length of the section.
    unsigned short itemlen = 0;
    bytesRead = fread(&itemlen, 1, sizeof(itemlen), infile);
    itemlen = CExifParse::Get16(&itemlen);

    if ((bytesRead != sizeof(itemlen)) || (itemlen < sizeof(itemlen)))
    {
      printf("JpgParse: invalid marker");
      return false;
    }

    switch(marker)
    {
      case M_SOS:   // stop before hitting compressed data
      return true;

      case M_EOI:   // in case it's a tables-only JPEG stream
        printf("JpgParse: No image in jpeg!");
        return false;
      break;

      case M_COM: // Comment section
        GetSection(infile, itemlen);
        if (m_SectionBuffer != NULL)
        {
       //   CExifParse::FixComment(comment);          // Ensure comment is printable
          strncpy(m_ExifInfo.Comments, (char *)&m_SectionBuffer[2], min(itemlen-2, MAX_COMMENT));
		    }
        ReleaseSection();
      break;

      case M_SOF0:
      case M_SOF1:
      case M_SOF2:
      case M_SOF3:
      case M_SOF5:
      case M_SOF6:
      case M_SOF7:
      case M_SOF9:
      case M_SOF10:
      case M_SOF11:
      case M_SOF13:
      case M_SOF14:
      case M_SOF15:
        GetSection(infile, itemlen);
        if ((m_SectionBuffer != NULL) && (itemlen >= 7))
        {
          ProcessSOFn();
          m_ExifInfo.Process = marker;
        }
        ReleaseSection();
      break;

      case M_IPTC:
        GetSection(infile, itemlen);
        if (m_SectionBuffer != NULL)
        {
          CIptcParse::Process(m_SectionBuffer, itemlen, &m_IPTCInfo);
        }
        ReleaseSection();
      break;

      case M_EXIF:
        // Seen files from some 'U-lead' software with Vivitar scanner
        // that uses marker 31 for non exif stuff.  Thus make sure
        // it says 'Exif' in the section before treating it as exif.
        GetSection(infile, itemlen);
        if (m_SectionBuffer != NULL)
        {
          CExifParse exif;
          exif.Process(m_SectionBuffer, itemlen, &m_ExifInfo);
        }
        ReleaseSection();
      break;

      case M_JFIF:
        // Regular jpegs always have this tag, exif images have the exif
        // marker instead, althogh ACDsee will write images with both markers.
        // this program will re-create this marker on absence of exif marker.
        // hence no need to keep the copy from the file.
      // fall through to default case
      default:
        // Skip any other sections.
        GetSection(infile, itemlen);
        ReleaseSection();
      break;
    }
  }
  return true;
}

//--------------------------------------------------------------------------
// Process a file. Check if it is JPEG. Extract exif/iptc info if it is.
//--------------------------------------------------------------------------
bool CJpegParse::Process (const char *picFileName)
{
  FILE *file;

  file = fopen(picFileName, "rb");
  if (!file)
    return false;

  // File exists and successfully opened. Start processing
  // Gather all information about the file

/*    // Get file name...
  CStdString tmp, urlFName, path;
  CURL url(picFileName);
  url.GetURLWithoutUserDetails(urlFName);
  CUtil::Split(urlFName, path, tmp);
  m_JpegInfo[SLIDE_FILE_NAME] = tmp;
  // ...then path...
  m_JpegInfo[SLIDE_FILE_PATH] = path;

  // ...then size...
  __stat64 fileStat;
  CFile::Stat(picFileName, &fileStat);
  float fileSize = (float)fileStat.st_size;
  tmp = "";
  if (fileSize > 1024)
  {
    fileSize /= 1024;
    tmp = "KB";
  }
  if (fileSize > 1024)
  {
    fileSize /= 1024;
    tmp = "MB";
  }
  if (fileSize > 1024)
  {
    fileSize /= 1024;
    tmp = "GB";
  }
  tmp.Format("%.2f %s", fileSize, tmp);
  m_JpegInfo[SLIDE_FILE_SIZE] = tmp;

  // ...then date and time...
  CDateTime date((time_t)fileStat.st_mtime);
  tmp.Format("%s %s", date.GetAsLocalizedDate(), date.GetAsLocalizedTime());
  m_JpegInfo[SLIDE_FILE_DATE] = tmp;*/

  bool result = ExtractInfo(file);
  fclose(file);
  if (result == false)
    printf("JpgParse: Not a JPEG file %s", picFileName);
  return result;
}

