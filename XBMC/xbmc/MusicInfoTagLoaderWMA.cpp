/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
 *
 *      Based on MediaInfo by Jérôme Martinez, Zen@MediaArea.net
 *      http://sourceforge.net/projects/mediainfo/
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

#include "stdafx.h"
#include "MusicInfoTagLoaderWMA.h"
#include "Util.h"
#include "Picture.h"

using namespace XFILE;
using namespace MUSIC_INFO;

CStdString fixString(CStdString &ansiString)
// ucs2CharsetToStringCharset is always called even when not required resulting in some strings
// twice the length they should be. This function is a quick fix to the problem. The correct
// solution would be to call ucs2CharsetToStringCharset only when necessary.
{
  int halfLen = ansiString.length() / 2 - 1;
  CStdString out = "";

  if (halfLen > 0)
    if (*(ansiString.Mid(halfLen, 1).c_str()) == 0 &&
        *(ansiString.Mid(halfLen + 1, 1).c_str()) == 0)
      out = ansiString.Left(halfLen);
  if (out == "")
    return ansiString ;
  else
    return out ;
}


// WMA metadata attribut types
// http://msdn.microsoft.com/library/en-us/wmform/htm/attributelist.asp
typedef enum WMT_ATTR_DATATYPE
{
  WMT_TYPE_STRING = 0,
  WMT_TYPE_BINARY = 1,
  WMT_TYPE_BOOL = 2,
  WMT_TYPE_DWORD = 3,
  WMT_TYPE_QWORD = 4,
  WMT_TYPE_WORD = 5,
} WMT_ATTR_DATATYPE;

// Data item for the WM/Picture metadata attribute
// http://msdn.microsoft.com/library/default.asp?url=/library/en-us/wmform/htm/wm_picture.asp
typedef struct _WMPicture
{
  CStdString pwszMIMEType;
  BYTE bPictureType;
  CStdStringW pwszDescription;
  DWORD dwDataLen;
  BYTE* pbData;
}
WM_PICTURE;

CMusicInfoTagLoaderWMA::CMusicInfoTagLoaderWMA(void)
{}

CMusicInfoTagLoaderWMA::~CMusicInfoTagLoaderWMA()
{}

// Based on MediaInfo
// by Jérôme Martinez, Zen@MediaArea.net
// http://sourceforge.net/projects/mediainfo/
bool CMusicInfoTagLoaderWMA::Load(const CStdString& strFileName, CMusicInfoTag& tag)
{
  try
  {
    tag.SetLoaded(false);
    CFile file;
    if (!file.Open(strFileName)) return false;

    tag.SetURL(strFileName);

    auto_aptr<unsigned char> pData(new unsigned char[65536]);
    file.Read(pData.get(), 65536);
    file.Close();

    int iOffset = 0;
    unsigned int* pDataI;

    //Play time
    iOffset = 0;
    pDataI = (unsigned int*)pData.get();
    while (!(pDataI[0] == 0x75B22630 && pDataI[1] == 0x11CF668E && pDataI[2] == 0xAA00D9A6 && pDataI[3] == 0x6CCE6200) && iOffset <= 65536 - 4)
    {
      iOffset++;
      pDataI = (unsigned int*)(pData.get() + iOffset);
    }
    if (iOffset > 65536 - 4)
      return false;

    //Play time
    iOffset = 0;
    pDataI = (unsigned int*)pData.get();
    while (!(pDataI[0] == 0x8CABDCA1 && pDataI[1] == 0x11CFA947 && pDataI[2] == 0xC000E48E && pDataI[3] == 0x6553200C) && iOffset <= 65536 - 4)
    {
      iOffset++;
      pDataI = (unsigned int*)(pData.get() + iOffset);
    }
    if (iOffset <= 65536 - 4)
    {
      iOffset += 64;
      pDataI = (unsigned int*)(pData.get() + iOffset);
      float F1 = (float)pDataI[1];
      F1 = F1 * 0x10000 * 0x10000 + pDataI[0];
      tag.SetDuration((long)((F1 / 10000) / 1000)); // from milliseconds to seconds
    }

    //Description  Title
    iOffset = 0;
    pDataI = (unsigned int*)pData.get();
    while (!(pDataI[0] == 0x75B22633 && pDataI[1] == 0x11CF668E && pDataI[2] == 0xAA00D9A6 && pDataI[3] == 0x6CCE6200) && iOffset <= 65536 - 4)
    {
      iOffset++;
      pDataI = (unsigned int*)(pData.get() + iOffset);
    }
    if (iOffset <= 65536 - 4)
    {
      iOffset += 24;
      int nTitleSize = pData[iOffset + 0] + pData[iOffset + 1] * 0x100;
      //int nAuthorSize = pData[iOffset + 2] + pData[iOffset + 3] * 0x100;
      //int nCopyrightSize = pData[iOffset + 4] + pData[iOffset + 5] * 0x100;
      //int nCommentsSize = pData[iOffset + 6] + pData[iOffset + 7] * 0x100;

      iOffset += 10;

      // TODO: UTF-8 Do we need to "fixString" these strings at all?
      CStdString utf8String = "";
      CStdStringW wString = "";
      g_charsetConverter.utf16LEtoW((const char*) (pData.get() + iOffset), wString);
      g_charsetConverter.wToUTF8(wString, utf8String);
      tag.SetTitle(utf8String);

      utf8String = "";
      g_charsetConverter.utf16LEtoW((char*) (pData.get() + iOffset + nTitleSize), wString);
      g_charsetConverter.wToUTF8(wString, utf8String);
      tag.SetArtist(utf8String);

      //General(ZT("Copyright"))=(LPWSTR)(pData.get()+iOffset+(nTitleSize+nAuthorSize));
      //General(ZT("Comments"))=(LPWSTR)(pData.get()+iOffset+(nTitleSize+nAuthorSize+nCopyrightSize));
    }

    // Maybe these information can be usefull in the future

    //Info audio
    //iOffset=0;
    //pDataI=(unsigned int*)pData;
    //while (!(pDataI[0]==0xF8699E40 && pDataI[1]==0x11CF5B4D && pDataI[2]==0x8000FDA8 && pDataI[3]==0x2B445C5F) && iOffset<=65536-4)
    //{
    //iOffset++;
    //pDataI=(unsigned int*)(pData+iOffset);
    //}
    //if (iOffset<=65536-4)
    //{
    //iOffset+=54;
    ////Codec
    //TCHAR C1[30];
    //_itoa(pData[iOffset]+pData[iOffset+1]*0x100, C1, 16);
    //CStdString Codec=C1;
    //while (Codec.size()<4)
    //  Codec='0'+Codec;
    //Audio[0](ZT("Codec"))=Codec;
    //Audio[0](ZT("Channels"))=pData[iOffset+2]; //2 octets
    //pDataI=(unsigned int*)(pData+iOffset);
    //Audio[0](ZT("SamplingRate"))=pDataI[1];
    //Audio[0](ZT("BitRate"))=pDataI[2]*8;
    //}

    //Info video
    //iOffset=0;
    //pDataI=(unsigned int*)pData;
    //while (!(pDataI[0]==0xBC19EFC0 && pDataI[1]==0x11CF5B4D && pDataI[2]==0x8000FDA8 && pDataI[3]==0x2B445C5F) && iOffset<=65536-4)
    //{
    //iOffset++;
    //pDataI=(unsigned int*)(pData+iOffset);
    //}
    //if (iOffset<=65536-4)
    //{
    //iOffset+=54;
    //iOffset+=15;
    //pDataI=(unsigned int*)(pData+iOffset);
    //Video[0](ZT("Width"))=pDataI[0];
    //Video[0](ZT("Height"))=pDataI[1];
    //Codec
    //unsigned char C1[5]; C1[4]='\0';
    //C1[0]=pData[iOffset+12+0]; C1[1]=pData[iOffset+12+1]; C1[2]=pData[iOffset+12+2]; C1[3]=pData[iOffset+12+3];
    //Video[0](ZT("Codec"))=wxString((char*)C1,wxConvUTF8).c_str();
    //}


    //Read extended metadata
    iOffset = 0;
    pDataI = (unsigned int*)pData.get();
    while (!(pDataI[0] == 0xD2D0A440 && pDataI[1] == 0x11D2E307 && pDataI[2] == 0xA000F097 && pDataI[3] == 0x50A85EC9) && iOffset <= 65536 - 4)
    {
      iOffset++;
      pDataI = (unsigned int*)(pData.get() + iOffset);
    }

    if (iOffset <= 65536 - 4)
    {
      iOffset += 24;

      // Walk through all frames in the file
      int iNumOfFrames = pData[iOffset] + pData[iOffset + 1] * 0x100;
      iOffset += 2;
      for (int Pos = 0; Pos < iNumOfFrames; Pos++)
      {
        int iFrameNameSize = pData[iOffset] + (pData[iOffset + 1] * 0x100);
        iOffset += 2;

        // Get frame name
        CStdString strFrameName = "";
        CStdStringW wString = "";
        g_charsetConverter.utf16LEtoW((char*)(pData.get() + iOffset), wString);
        g_charsetConverter.wToUTF8(wString, strFrameName);
        //CStdString strFrameName((LPWSTR)(pData.get() + iOffset));
        iOffset += iFrameNameSize;

        // Get datatype of frame
        int iFrameType = pData[iOffset] + pData[iOffset + 1];
        iOffset += 2;

        // Size of frame value
        int iValueSize = pData[iOffset] + (pData[iOffset + 1] * 0x100);
        iOffset += 2;

        // Parse frame value and fill
        // tag with extended metadata
        if (iFrameType == WMT_TYPE_STRING && iValueSize > 0)
        {
          //LPWSTR pwszValue = (LPWSTR)(pData.get() + iOffset);
          // TODO: UTF-8: Do we need to "fixString" these utf8 strings?
          CStdString utf8String = "";
          CStdStringW wString = "";
          g_charsetConverter.utf16LEtoW((char*)(pData.get() + iOffset), wString);
          g_charsetConverter.wToUTF8(wString, utf8String);
                
          SetTagValueString(strFrameName, utf8String, tag);
        }
        else if (iFrameType == WMT_TYPE_BINARY && iValueSize > 0)
        {
          BYTE* pValue = (BYTE*)(pData.get() + iOffset); // Raw data
          SetTagValueBinary(strFrameName, pValue, tag);
        }
        else if (iFrameType == WMT_TYPE_BOOL && iValueSize > 0)
        {
          BOOL bValue = (BOOL)pData[iOffset];
          SetTagValueBool(strFrameName, bValue, tag);
        }
        else if (iFrameType == WMT_TYPE_DWORD && iValueSize > 0)
        {
          DWORD dwValue = pData[iOffset] + pData[iOffset + 1] * 0x100 + pData[iOffset + 2] * 0x10000 + pData[iOffset + 3] * 0x1000000;
          SetTagValueDWORD(strFrameName, dwValue, tag);
        }
        else if (iFrameType == WMT_TYPE_QWORD && iValueSize > 0)
        {
          //DWORD qwValue = pData[iOffset] + pData[iOffset + 1] * 0x100 + pData[iOffset + 2] * 0x10000 + pData[iOffset + 3] * 0x1000000;
        }
        else if (iFrameType == WMT_TYPE_WORD && iValueSize > 0)
        {
          //WORD wValue = pData[iOffset] + pData[iOffset + 1] * 0x100;
        }

        // parse next frame
        iOffset += iValueSize;
      }
    }

    //Read extended metadata 2
    iOffset = 0;
    pDataI = (unsigned int*)pData.get();
    while (!(pDataI[0] == 0x44231C94 && pDataI[1] == 0x49D19498 && pDataI[2] == 0x131D41A1 && pDataI[3] == 0x5470454E) && iOffset <= 65536 - 4)
    {
      iOffset++;
      pDataI = (unsigned int*)(pData.get() + iOffset);
    }

    if (iOffset <= 65536 - 4)
    {
      iOffset += 24;

      // Walk through all frames in the file
      int iNumOfFrames = pData[iOffset] + pData[iOffset + 1] * 0x100;
      iOffset += 6;
      for (int Pos = 0; Pos < iNumOfFrames; Pos++)
      {
        int iFrameNameSize = pData[iOffset] + (pData[iOffset + 1] * 0x100);
        iOffset += 2;

        // Get datatype of frame
        int iFrameType = pData[iOffset] + pData[iOffset + 1];
        iOffset += 2;

        // Size of frame value
        int iValueSize = pData[iOffset] + (pData[iOffset + 1] * 0x100);
        iOffset += 4;

        // Get frame name
        CStdString strFrameName = "";
        CStdStringW wString = "";
        g_charsetConverter.utf16LEtoW((char*)(pData.get() + iOffset), wString);
        g_charsetConverter.wToUTF8(wString, strFrameName);
        // CStdString strFrameName((LPWSTR)(pData.get() + iOffset));
        iOffset += iFrameNameSize;

        // Parse frame value and fill
        // tag with extended metadata
        if (iFrameType == WMT_TYPE_STRING && iValueSize > 0)
        {
          //LPWSTR pwszValue = (LPWSTR)(pData.get() + iOffset);
          // TODO: UTF-8: Do we need to "fixString" these utf8 strings?
          CStdString utf8String = "";
          CStdStringW wString = "";
          g_charsetConverter.utf16LEtoW((char*)(pData.get() + iOffset), wString);
          g_charsetConverter.wToUTF8(wString, utf8String);
          SetTagValueString(strFrameName, utf8String, tag);
        }
        else if (iFrameType == WMT_TYPE_BINARY && iValueSize > 0)
        {
          BYTE* pValue = (BYTE*)(pData.get() + iOffset); // Raw data
          SetTagValueBinary(strFrameName, pValue, tag);
        }
        else if (iFrameType == WMT_TYPE_BOOL && iValueSize > 0)
        {
          BOOL bValue = (BOOL)pData[iOffset];
          SetTagValueBool(strFrameName, bValue, tag);
        }
        else if (iFrameType == WMT_TYPE_DWORD && iValueSize > 0)
        {
          DWORD dwValue = pData[iOffset] + pData[iOffset + 1] * 0x100 + pData[iOffset + 2] * 0x10000 + pData[iOffset + 3] * 0x1000000;
          SetTagValueDWORD(strFrameName, dwValue, tag);
        }
        else if (iFrameType == WMT_TYPE_QWORD && iValueSize > 0)
        {
          //DWORD qwValue = pData[iOffset] + pData[iOffset + 1] * 0x100 + pData[iOffset + 2] * 0x10000 + pData[iOffset + 3] * 0x1000000;
        }
        else if (iFrameType == WMT_TYPE_WORD && iValueSize > 0)
        {
          //WORD wValue = pData[iOffset] + pData[iOffset + 1] * 0x100;
        }

        // parse next frame
        iOffset += iValueSize + 4;
      }
    }

    tag.SetLoaded(true);
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Tag loader wma: exception in file %s", strFileName.c_str());
  }

  tag.SetLoaded(false);
  return false;
}

void CMusicInfoTagLoaderWMA::SetTagValueString(const CStdString& strFrameName, const CStdString& strValue, CMusicInfoTag& tag)
{
  if (strFrameName == "WM/AlbumTitle")
  {
    tag.SetAlbum(strValue);
  }
  else if (strFrameName == "WM/AlbumArtist")
  {
    if (tag.GetAlbumArtist().IsEmpty()) tag.SetAlbumArtist(strValue);
  }
  else if (strFrameName == "Author")
  {
    // Multiple artists are stored in multiple "Author" tags we have get them
    // separatly and merge them to our system
    if (tag.GetArtist().IsEmpty())
      tag.SetArtist(strValue);
    else
      tag.SetArtist(tag.GetArtist() + g_advancedSettings.m_musicItemSeparator + strValue);
  }
  else if (strFrameName == "WM/TrackNumber")
  {
    if (tag.GetTrackNumber() <= 0) tag.SetTrackNumber(atoi(strValue.c_str()));
  }
  else if (strFrameName == "WM/PartOfSet")
  {
    tag.SetPartOfSet(atoi(strValue.c_str()));
  }
  //else if (strFrameName=="WM/Track") // Old Tracknumber, should not be used anymore
  else if (strFrameName == "WM/Year")
  {
    SYSTEMTIME dateTime;
    dateTime.wYear = atoi(strValue.c_str());
    tag.SetReleaseDate(dateTime);
  }
  else if (strFrameName == "WM/Genre")
  {
    // Multiple genres are stared in multiple "WM/Genre" tags we have to get them
    // separatly and merge them to our system
    if (tag.GetGenre().IsEmpty())
      tag.SetGenre(strValue);
    else
      tag.SetGenre(tag.GetGenre() + g_advancedSettings.m_musicItemSeparator + strValue);
  }
  else if (strFrameName == "WM/Lyrics")
  {
    tag.SetLyrics(strValue);
  }
  //else if (strFrameName=="WM/DRM")
  //{
  // // File is DRM protected
  // pwszValue;
  //}
  //else if (strFrameName=="WM/Codec")
  //{
  // pwszValue;
  //}
  //else if (strFrameName=="WM/BeatsPerMinute")
  //{
  // pwszValue;
  //}
  //else if (strFrameName=="WM/Mood")
  //{
  // pwszValue;
  //}
  //else if (strFrameName=="WM/RadioStationName")
  //{
  // pwszValue;
  //}
  //else if (strFrameName=="WM/RadioStationOwner")
  //{
  // pwszValue;
  //}
}

void CMusicInfoTagLoaderWMA::SetTagValueDWORD(const CStdString& strFrameName, DWORD dwValue, CMusicInfoTag& tag)
{
  if (strFrameName == "WM/TrackNumber")
  {
    if (tag.GetTrackNumber() <= 0)
      tag.SetTrackNumber(dwValue);
  }
}

void CMusicInfoTagLoaderWMA::SetTagValueBinary(const CStdString& strFrameName, const LPBYTE pValue, CMusicInfoTag& tag)
{
  if (strFrameName == "WM/Picture")
  {
    WM_PICTURE picture;
    int iPicOffset = 0;

    // Picture types: http://msdn.microsoft.com/library/en-us/wmform/htm/wm_picture.asp
    picture.bPictureType = (BYTE)pValue[iPicOffset];
    iPicOffset += 1;

    picture.dwDataLen = (DWORD)pValue[iPicOffset] + (pValue[iPicOffset + 1] * 0x100);
    iPicOffset += 4;

    CStdStringW wString;
    g_charsetConverter.utf16LEtoW((char*) (pValue + iPicOffset), wString);
    g_charsetConverter.wToUTF8(wString, picture.pwszMIMEType);
    iPicOffset += (wString.length() * 2);
    iPicOffset += 2;

    g_charsetConverter.utf16LEtoW((char*) (pValue + iPicOffset), picture.pwszDescription);
    iPicOffset += (picture.pwszDescription.length() * 2);
    iPicOffset += 2;

    picture.pbData = (pValue + iPicOffset);

    // many wma's don't have the bPictureType specified.  For now, just take
    // Cover Front (3) or Other (0) as the cover.
    if (picture.bPictureType == 3 || picture.bPictureType == 0) // Cover Front
    {
      CStdString strExtension(picture.pwszMIMEType);
      // if we don't have an album tag, cache with the full file path so that
      // other non-tagged files don't get this album image
      CStdString strCoverArt;
      if (!tag.GetAlbum().IsEmpty() && (!tag.GetAlbumArtist().IsEmpty() || !tag.GetArtist().IsEmpty()))
        strCoverArt = CUtil::GetCachedAlbumThumb(tag.GetAlbum(), tag.GetAlbumArtist().IsEmpty() ? tag.GetArtist() : tag.GetAlbumArtist());
      else
        strCoverArt = CUtil::GetCachedMusicThumb(tag.GetURL());
      if (!CUtil::ThumbExists(strCoverArt))
      {
        int nPos = strExtension.Find('/');
        if (nPos > -1)
          strExtension.Delete(0, nPos + 1);

        if (picture.pbData != NULL && picture.dwDataLen > 0)
        {
          CPicture pic;
          if (pic.CreateThumbnailFromMemory(picture.pbData, picture.dwDataLen, strExtension, strCoverArt))
          {
            CUtil::ThumbCacheAdd(strCoverArt, true);
          }
          else
          {
            CUtil::ThumbCacheAdd(strCoverArt, false);
            CLog::Log(LOGERROR, "Tag loader wma: Unable to create album art for %s (extension=%s, size=%lu)", tag.GetURL().c_str(), strExtension.c_str(), picture.dwDataLen);
          }
        }
      }
    }
  }
}

void CMusicInfoTagLoaderWMA::SetTagValueBool(const CStdString& strFrameName, BOOL bValue, CMusicInfoTag& tag)
{
  //else if (strFrameName=="isVBR")
  //{
  //}
}
