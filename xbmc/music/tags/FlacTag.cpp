/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "system.h"
#include "FlacTag.h"
#include "Util.h"
#include "pictures/Picture.h"
#include "filesystem/File.h"
#include "utils/log.h"
#include "utils/EndianSwap.h"
#include "ThumbnailCache.h"


#define BYTES_TO_CHECK_FOR_BAD_TAGS 16384

using namespace XFILE;
using namespace MUSIC_INFO;
using namespace std;

#define CHUNK_SIZE 8192  // should suffice for most tags

CFlacTag::CFlacTag()
{

}

CFlacTag::~CFlacTag()
{}

// overridden from COggTag
bool CFlacTag::Read(const CStdString& strFile)
{
  CVorbisTag::Read(strFile);

  CFile file;
  if (!file.Open(strFile))
    return false;

  m_file = &file;

  // format is:
  // fLaC METABLOCK ... METABLOCK
  // METABLOCK has format:
  // <1> Bool for last metablock
  // <7> blocktype (0 = STREAMINFO, 1 = PADDING, 3 = SEEKTABLE, 4 = VORBIS_COMMENT
  // <24> length of metablock to follow (not including this 4 byte header)
  //
  // first find our FLAC header
  int iPos = ReadFlacHeader(); // position in the file
  if (!iPos) return false;
  // Find vorbis header
  m_file->Seek(iPos, SEEK_SET); // past the fLaC header and STREAMINFO buffer (compulsory)
  // see what type it is:
  bool foundTag = false;
  unsigned int cover = 0;
  unsigned int second_cover = 0;
  unsigned int third_cover = 0;
  do
  {
    unsigned int metaBlock = ReadUnsigned();
    if ((metaBlock & 0x7F000000) == 0x4000000) // found a VORBIS_COMMENT tag
    { // read it in
      unsigned int size = (metaBlock & 0xffffff);
      char *tag = new char[size];
      if (tag)
      {
        m_file->Read((void*)tag, size);
        // Process this tag info
        ProcessVorbisComment(tag,size);
        foundTag = true;
        delete[] tag;
      }
    }
    else if ((metaBlock & 0x7F000000) == 0x6000000) // found a PICTURE tag - see if it's the cover
    {
      // read the type of the image
      unsigned int picType = ReadUnsigned();
      if (picType == 3 && !cover)  // 3 == Cover (front)
        cover = iPos + 8;
      else if (picType == 0 && !second_cover) // 0 == Other
        second_cover = iPos + 8;
      else
        third_cover = iPos + 8;
    }
    else if (metaBlock & 0x80000000)  // break if it's the last one
      break;
    iPos += (metaBlock & 0xffffff) + 4;
    m_file->Seek(iPos, SEEK_SET);
  }
  while (true);

  if (!cover)
    cover = second_cover;
  if (!cover)
    cover = third_cover;

  CStdString strCoverArt;
  if (!m_musicInfoTag.GetAlbum().IsEmpty() && (!m_musicInfoTag.GetAlbumArtist().IsEmpty() || !m_musicInfoTag.GetArtist().IsEmpty()))
    strCoverArt = CThumbnailCache::GetAlbumThumb(&m_musicInfoTag);
  else
    strCoverArt = CThumbnailCache::GetMusicThumb(m_musicInfoTag.GetURL());

  if (cover && !CUtil::ThumbExists(strCoverArt))
  {
    char info[1024];
    m_file->Seek(cover, SEEK_SET);

    // read the mime type
    unsigned int size = ReadUnsigned();
    m_file->Read(info, min(size, (unsigned int) 1023));
    info[min(size, (unsigned int) 1023)] = 0;
    if (size > 1023)
      m_file->Seek(size - 1023, SEEK_CUR);
    CStdString mimeType = info;

    // now the description
    size = ReadUnsigned();
    m_file->Read(info, min(size, (unsigned int) 1023));
    info[min(size, (unsigned int) 1023)] = 0;
    if (size > 1023)
      m_file->Seek(size - 1023, SEEK_CUR);

    int nPos = mimeType.Find('/');
    if (nPos > -1)
      mimeType.Delete(0, nPos + 1);

    // and now our actual image info
    unsigned int picInfo[4];
    m_file->Read(picInfo, 16);

    unsigned int picSize = ReadUnsigned();
    BYTE *picData = new BYTE[picSize];
    if (picData)
    {
      m_file->Read(picData, picSize);
      if (CPicture::CreateThumbnailFromMemory(picData, picSize, mimeType, strCoverArt))
      {
        CUtil::ThumbCacheAdd(strCoverArt, true);
      }
      else
      {
        CUtil::ThumbCacheAdd(strCoverArt, false);
        CLog::Log(LOGERROR, "%s Unable to create album art for %s (extension=%s, size=%d)", __FUNCTION__, m_musicInfoTag.GetURL().c_str(), mimeType.c_str(), picSize);
      }
      delete[] picData;
    }
  }
  return foundTag;
}

// read the duration information from the STREAM_INFO metadata block
int CFlacTag::ReadFlacHeader(void)
{
  unsigned char buffer[8];
  // Check to see if we have a STREAM_INFO header:
  int iPos = FindFlacHeader();
  if (!iPos) return 0;
  // Okay, we have found the correct start of a fLaC file
  m_file->Seek(iPos, SEEK_SET);  // seek to right after the "fLaC" header string
  m_file->Read(buffer, 4);    // read the header bit
  if ((buffer[0]&0x7F) != 0) return 0; // no Flac header details at all!
  // get details out of the stream
  m_file->Seek(iPos + 14, SEEK_SET);  // seek to the frequency and duration data
  m_file->Read(buffer, 8);    // read 64 bits of data
  int iFreq = (buffer[0] << 12) | (buffer[1] << 4) | (buffer[2] >> 4);
  int64_t iNumSamples = ( (int64_t) (buffer[3] & 0x0F) << 32) | ( (int64_t) buffer[4] << 24) | (buffer[5] << 16) | (buffer[6] << 8) | buffer[7];
  if (iFreq != 0)
    m_musicInfoTag.SetDuration((int)((iNumSamples) / iFreq));
  return iPos + 38;
}

// runs through the file and finds the occurence of the word "fLaC" which SHOULD
// be the first 4 bytes, but sometimes ID3 tags etc. have been incorrectly added
// so we should at least make a (small) effort to check these cases out.
// We check the first BYTES_TO_CHECK_FOR_BAD_TAGS bytes.
// returns the file offset

int CFlacTag::FindFlacHeader(void)
{
  char tag[BYTES_TO_CHECK_FOR_BAD_TAGS];
  m_file->Read( (void*) tag, BYTES_TO_CHECK_FOR_BAD_TAGS );

  // Find flac header "fLaC"
  int i = 0;
  while ( i < BYTES_TO_CHECK_FOR_BAD_TAGS )
  {
    if ( tag[i] == 'f' && tag[i + 1] == 'L' && tag[i + 2] == 'a' && tag[i + 3] == 'C')
    {
      return i + 4;
    }
    i++;
  }

  return 0;
}

void CFlacTag::ProcessVorbisComment(const char *pBuffer, size_t bufsize)
{
  unsigned int Pos = 0;      // position in the buffer
  unsigned int I1 = Endian_SwapLE32(*(unsigned int*)(pBuffer + Pos)); // length of vendor string
  Pos += I1 + 4;     // just pass the vendor string
  unsigned int Count = Endian_SwapLE32(*(unsigned int*)(pBuffer + Pos)); // number of comments
  Pos += 4;    // Start of the first comment
  char C1[CHUNK_SIZE];
  for (unsigned int I2 = 0; I2 < Count; I2++) // Run through the comments
  {
    if (Pos >= bufsize)
    {
      CLog::Log(LOGWARNING,"flac tag overflow");
      return;
    }
    I1 = Endian_SwapLE32(*(unsigned int*)(pBuffer + Pos));   // Length of comment
    if (I1 < CHUNK_SIZE)
    {
      strncpy(C1, pBuffer + Pos + 4, I1);
      C1[I1] = '\0';
      CStdString strItem;
      strItem=C1;
      // Parse the tag entry
      ParseTagEntry( strItem );
    }
    // Increment our position in the file buffer
    Pos += I1 + 4;
  }
}

unsigned int CFlacTag::ReadUnsigned()
{
  unsigned char size[4];
  m_file->Read(size, 4);
  return ((unsigned int)size[0] << 24) + ((unsigned int)size[1] << 16) + ((unsigned int)size[2] << 8) + (unsigned int)size[3];
}

