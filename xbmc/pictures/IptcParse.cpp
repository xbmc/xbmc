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
// Module to pull IPTC information out of various types of digital images.
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
//  Process IPTC data.
//--------------------------------------------------------------------------
#ifndef _LINUX
#include <windows.h>
#else
#include <string.h>
#endif
#include <stdio.h>
#include "IptcParse.h"
#include "ExifParse.h"

#ifndef min
#define min(a,b) (a)>(b)?(b):(a)
#endif

// Supported IPTC entry types
#define IPTC_RECORD_VERSION         0x00
#define IPTC_SUPLEMENTAL_CATEGORIES 0x14
#define IPTC_KEYWORDS               0x19
#define IPTC_CAPTION                0x78
#define IPTC_AUTHOR                 0x7A
#define IPTC_HEADLINE               0x69
#define IPTC_SPECIAL_INSTRUCTIONS   0x28
#define IPTC_CATEGORY               0x0F
#define IPTC_BYLINE                 0x50
#define IPTC_BYLINE_TITLE           0x55
#define IPTC_CREDIT                 0x6E
#define IPTC_SOURCE                 0x73
#define IPTC_COPYRIGHT_NOTICE       0x74
#define IPTC_OBJECT_NAME            0x05
#define IPTC_CITY                   0x5A
#define IPTC_STATE                  0x5F
#define IPTC_COUNTRY                0x65
#define IPTC_TRANSMISSION_REFERENCE 0x67
#define IPTC_DATE                   0x37
#define IPTC_URGENCY                0x0A
#define IPTC_COUNTRY_CODE           0x64
#define IPTC_REFERENCE_SERVICE      0x2D
#define IPTC_TIME_CREATED           0x3C
#define IPTC_SUB_LOCATION           0x5C
#define IPTC_IMAGE_TYPE             0x82


//--------------------------------------------------------------------------
//  Process IPTC marker. Return FALSE if unable to process marker.
//
//  IPTC block consists of:
//    - Marker:              1 byte   (0xED)
//    - Block length:        2 bytes
//    - IPTC Signature:     14 bytes    ("Photoshop 3.0\0")
//    - 8BIM Signature       4 bytes     ("8BIM")
//    - IPTC Block start     2 bytes     (0x04, 0x04)
//    - IPTC Header length   1 byte
//    - IPTC header         Header is padded to even length, counting the length byte
//    - Length               4 bytes
//    - IPTC Data which consists of a number of entries, each of which has the following format:
//            - Signature    2 bytes     (0x1C02)
//            - Entry type   1 byte   (for defined entry types, see #defines above)
//            - entry length 2 bytes
//            - entry data   'entry length' bytes
//
//--------------------------------------------------------------------------
bool CIptcParse::Process (const unsigned char* const Data, const unsigned short itemlen, IPTCInfo_t *info)
{
  if (!info) return false;

  const char IptcSignature1[] = "Photoshop 3.0";
  const char IptcSignature2[] = "8BIM";
  const char IptcSignature3[] = {0x04, 0x04};

  // Check IPTC signatures
  char* pos = (char*)(Data + sizeof(short));  // position data pointer after length field
  char* maxpos = (char*)(Data+itemlen);
  unsigned char headerLen = 0;
  unsigned char dataLen = 0;
  memset(info, 0, sizeof(IPTCInfo_t));

  if (itemlen < 25) return false;

  if (memcmp(pos, IptcSignature1, strlen(IptcSignature1)-1) != 0) return false;
  pos += sizeof(IptcSignature1);          // move data pointer to the next field

  if (memcmp(pos, IptcSignature2, strlen(IptcSignature2)-1) != 0) return false;
  pos += sizeof(IptcSignature2)-1;              // move data pointer to the next field

  while (memcmp(pos, IptcSignature3, sizeof(IptcSignature3)) != 0) { // loop on valid Photoshop blocks

    pos += sizeof(IptcSignature3); // move data pointer to the Header Length
    // Skip header
    headerLen = *pos; // get header length and move data pointer to the next field
    pos += (headerLen & 0xfe) + 2; // move data pointer to the next field (Header is padded to even length, counting the length byte)

    pos += 3; // move data pointer to length, assume only one byte, TODO: use all 4 bytes

    dataLen = *pos++;
    pos += dataLen; // skip data section

    if (memcmp(pos, IptcSignature2, sizeof(IptcSignature2) - 1) != 0) return false;
    pos += sizeof(IptcSignature2) - 1; // move data pointer to the next field
  }

  pos += sizeof(IptcSignature3);          // move data pointer to the next field
  if (pos >= maxpos) return false;

  // IPTC section found

  // Skip header
  headerLen = *pos++;           // get header length and move data pointer to the next field
  pos += headerLen + 1 - (headerLen % 2);     // move data pointer to the next field (Header is padded to even length, counting the length byte)

  if (pos + 4 >= maxpos) return false;

  pos += 4;                                   // move data pointer to the next field

  // Now read IPTC data
  while (pos < (char*)(Data + itemlen-5))
  {
    if (pos + 5 > maxpos) return false;

    short signature = (*pos << 8) + (*(pos+1));

    pos += 2;
    if (signature != 0x1C01 && signature != 0x1C02)
      break;

    unsigned char  type = *pos++;
    unsigned short length  = (*pos << 8) + (*(pos+1));
    pos += 2;                   // Skip tag length

    if (pos + length > maxpos) return false;

    // Process tag here
    char *tag = NULL;
    if (signature == 0x1C02)
    {
      switch (type)
      {
        case IPTC_RECORD_VERSION:           tag = info->RecordVersion;           break;
        case IPTC_SUPLEMENTAL_CATEGORIES:   tag = info->SupplementalCategories;  break;
        case IPTC_KEYWORDS:                 tag = info->Keywords;                break;
        case IPTC_CAPTION:                  tag = info->Caption;                 break;
        case IPTC_AUTHOR:                   tag = info->Author;                  break;
        case IPTC_HEADLINE:                 tag = info->Headline;                break;
        case IPTC_SPECIAL_INSTRUCTIONS:     tag = info->SpecialInstructions;     break;
        case IPTC_CATEGORY:                 tag = info->Category;                break;
        case IPTC_BYLINE:                   tag = info->Byline;                  break;
        case IPTC_BYLINE_TITLE:             tag = info->BylineTitle;             break;
        case IPTC_CREDIT:                   tag = info->Credit;                  break;
        case IPTC_SOURCE:                   tag = info->Source;                  break;
        case IPTC_COPYRIGHT_NOTICE:         tag = info->CopyrightNotice;         break;
        case IPTC_OBJECT_NAME:              tag = info->ObjectName;              break;
        case IPTC_CITY:                     tag = info->City;                    break;
        case IPTC_STATE:                    tag = info->State;                   break;
        case IPTC_COUNTRY:                  tag = info->Country;                 break;
        case IPTC_TRANSMISSION_REFERENCE:   tag = info->TransmissionReference;   break;
        case IPTC_DATE:                     tag = info->Date;                    break;
        case IPTC_URGENCY:                  tag = info->Urgency;                 break;
        case IPTC_REFERENCE_SERVICE:        tag = info->ReferenceService;        break;
        case IPTC_COUNTRY_CODE:             tag = info->CountryCode;             break;
        case IPTC_TIME_CREATED:             tag = info->TimeCreated;             break;
        case IPTC_SUB_LOCATION:             tag = info->SubLocation;             break;
        case IPTC_IMAGE_TYPE:               tag = info->ImageType;               break;
        default:
          printf("IptcParse: Unrecognised IPTC tag: 0x%02x", type);
          break;
      }
    }

    if (tag)
    {
      if (type != IPTC_KEYWORDS || *tag == 0)
      {
        strncpy(tag, pos, min(length, MAX_IPTC_STRING - 1));
        tag[min(length, MAX_IPTC_STRING - 1)] = 0;
      }
      else if (type == IPTC_KEYWORDS)
      {
        // there may be multiple keywords - lets join them
        size_t maxLen = MAX_IPTC_STRING - strlen(tag);
        if (maxLen > 2)
        {
          strcat(tag, ", ");
          strncat(tag, pos, min(length, maxLen - 3));
        }
      }
/*      if (id == SLIDE_IPTC_CAPTION)
      {
        CExifParse::FixComment(m_IptcInfo[id]);     // Ensure comment is printable
      }*/
    }
    pos += length;
  }
  return true;
}

