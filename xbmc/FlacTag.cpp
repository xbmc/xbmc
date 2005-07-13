
#include "stdafx.h"
#include "xtl.h"
#include "flactag.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define BYTES_TO_CHECK_FOR_BAD_TAGS 8192

using namespace MUSIC_INFO;

#define CHUNK_SIZE 8192  // should suffice for most tags

CFlacTag::CFlacTag()
{

}

CFlacTag::~CFlacTag()
{}

// overridden from COggTag
bool CFlacTag::ReadTag(const CStdString& strFile)
{
  CFile file;

  m_musicInfoTag.SetURL(strFile);

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
  unsigned char buffer[4];
  m_file->Seek(iPos, SEEK_SET); // past the fLaC header and STREAMINFO buffer (compulsory)
  // see what type it is:
  bool bFound(false);
  do
  {
    m_file->Read(buffer, 4); // read the next METABLOCK header
    if ((buffer[0]&0x7F) == 4) // found a VORBIS_COMMENT tag
    {
      bFound = true;
      break;
    }
    if (buffer[0]&0x80)  // break if it's the last one
      break;
    iPos += ((int)buffer[1] << 16) + ((int)buffer[2] << 8) + int(buffer[3]) + 4;
    m_file->Seek(iPos, SEEK_SET);
  }
  while (!bFound);

  if (bFound)  // Yay, we've found the vorbis_comment section - seek to the right place
  {
    m_file->Seek(iPos + 4, SEEK_SET);
    // now read in a chunk of data
    char pBuffer[8192];
    m_file->Read((void*)pBuffer, 8192);
    // Process this tag info
    ProcessVorbisComment(pBuffer);
  }
  return bFound;
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
  __int64 iNumSamples = (__int64(buffer[3] & 0x0F) << 32) | (__int64(buffer[4]) << 24) | (buffer[5] << 16) | (buffer[6] << 8) | buffer[7];
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

void CFlacTag::ProcessVorbisComment(const char *pBuffer)
{
  int Pos = 0;      // position in the buffer
  int *I1 = (int*)(pBuffer + Pos); // length of vendor string
  Pos += I1[0] + 4;     // just pass the vendor string
  I1 = (int*)(pBuffer + Pos);   // number of comments
  int Count = I1[0];
  Pos += 4;    // Start of the first comment
  char C1[CHUNK_SIZE];
  for (int I2 = 0; I2 < Count; I2++) // Run through the comments
  {
    I1 = (int*)(pBuffer + Pos);   // Length of comment
    strncpy(C1, pBuffer + Pos + 4, I1[0]);
    C1[I1[0]] = '\0';
    CStdString strItem;
    g_charsetConverter.utf8ToStringCharset(C1, strItem);  // convert UTF-8 to charset string
    // Parse the tag entry
    ParseTagEntry( strItem );
    // Increment our position in the file buffer
    Pos += I1[0] + 4;
  }
}
