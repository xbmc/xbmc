
#include "stdafx.h"
#include "musicinfotagloadermp3.h"
#include "Util.h"
#include "picture.h"
#include "lib/libID3/misc_support.h"
#include "apev2tag.h"
#include "cores/paplayer/aaccodec.h"

using namespace MUSIC_INFO;

const uchar* ID3_GetPictureBufferOfPicType(ID3_Tag* tag, ID3_PictureType pictype, size_t* pBufSize )
{
  if (NULL == tag)
    return NULL;
  else
  {
    ID3_Frame* frame = NULL;
    ID3_Tag::Iterator* iter = tag->CreateIterator();

    while (NULL != (frame = iter->GetNext() ))
    {
      if (frame->GetID() == ID3FID_PICTURE)
      {
        if (frame->GetField(ID3FN_PICTURETYPE)->Get() == (uint32)pictype)
          break;
      }
    }
    delete iter;

    if (frame != NULL)
    {
      ID3_Field* myField = frame->GetField(ID3FN_DATA);
      if (myField != NULL)
      {
        *pBufSize = myField->Size();
        return myField->GetRawBinary();
      }
      else return NULL;
    }
    else return NULL;
  }
}

#define BYTES2INT(b1,b2,b3,b4) (((b1 & 0xFF) << (3*8)) | \
                                ((b2 & 0xFF) << (2*8)) | \
                                ((b3 & 0xFF) << (1*8)) | \
                                ((b4 & 0xFF) << (0*8)))

#define UNSYNC(b1,b2,b3,b4) (((b1 & 0x7F) << (3*7)) | \
                             ((b2 & 0x7F) << (2*7)) | \
                             ((b3 & 0x7F) << (1*7)) | \
                             ((b4 & 0x7F) << (0*7)))

#define MPEG_VERSION2_5 0
#define MPEG_VERSION1   1
#define MPEG_VERSION2   2

/* Xing header information */
#define VBR_FRAMES_FLAG 0x01
#define VBR_BYTES_FLAG  0x02
#define VBR_TOC_FLAG    0x04

// mp3 header flags
#define SYNC_MASK (0x7ff << 21)
#define VERSION_MASK (3 << 19)
#define LAYER_MASK (3 << 17)
#define PROTECTION_MASK (1 << 16)
#define BITRATE_MASK (0xf << 12)
#define SAMPLERATE_MASK (3 << 10)
#define PADDING_MASK (1 << 9)
#define PRIVATE_MASK (1 << 8)
#define CHANNELMODE_MASK (3 << 6)
#define MODE_EXT_MASK (3 << 4)
#define COPYRIGHT_MASK (1 << 3)
#define ORIGINAL_MASK (1 << 2)
#define EMPHASIS_MASK 3

using namespace MUSIC_INFO;
using namespace XFILE;

CMusicInfoTagLoaderMP3::CMusicInfoTagLoaderMP3(void)
{
  m_iID3v2Size=0;
}

CMusicInfoTagLoaderMP3::~CMusicInfoTagLoaderMP3()
{
}

char* CMusicInfoTagLoaderMP3::GetString(const ID3_Frame *frame, ID3_FieldID fldName)
{
  char *text = NULL;

  ID3_Field* fld;
  if (NULL != frame && NULL != (fld = frame->GetField(fldName)))
  {
    ID3_TextEnc enc = fld->GetEncoding();

    if (enc == ID3TE_ISO8859_1)
    {
      size_t nText = fld->Size();
      text = LEAKTESTNEW(char[nText + 1]);
      fld->Get(text, nText + 1);
      text[nText] = '\0';
    }
    else if (enc == ID3TE_UTF16 || enc == ID3TE_UTF16BE)
    {
      size_t nText = fld->Size()/2;
      unicode_t* textW = LEAKTESTNEW(unicode_t[nText + 1]);
      fld->Get(textW, nText);
      textW[nText] = '\0';

      CStdStringW s((wchar_t*) textW, nText);
      CStdStringA ansiString;
      g_charsetConverter.ucs2CharsetToStringCharset(s, ansiString, true);
      delete [] textW;

      nText = strlen(ansiString.c_str());
      text = LEAKTESTNEW(char[nText + 1]);
      strncpy(text, ansiString.c_str(), nText);
      text[nText] = '\0';
    }
    else if (enc == ID3TE_UTF8)
    {
      size_t nText = fld->Size();
      text = LEAKTESTNEW(char[nText + 1]);
      fld->Get(text, nText);
      text[nText] = '\0';

      CStdStringA s(text, nText);
      CStdStringA ansiString;
      g_charsetConverter.utf8ToStringCharset(s, ansiString);

      nText = strlen(ansiString.c_str());
      strncpy(text, ansiString.c_str(), nText);
      text[nText] = '\0';
    }
  }
  return text;
}

char* CMusicInfoTagLoaderMP3::GetArtist(const ID3_Tag *tag)
{
  char *sArtist = NULL;
  if (NULL == tag)
  {
    return sArtist;
  }

  ID3_Frame *frame = NULL;
  if ((frame = tag->Find(ID3FID_LEADARTIST)) ||
      (frame = tag->Find(ID3FID_BAND)) ||
      (frame = tag->Find(ID3FID_CONDUCTOR)) ||
      (frame = tag->Find(ID3FID_COMPOSER)))
  {
    sArtist = GetString(frame, ID3FN_TEXT);
  }
  return sArtist;
}

char* CMusicInfoTagLoaderMP3::GetAlbum(const ID3_Tag *tag)
{
  char *sAlbum = NULL;
  if (NULL == tag)
  {
    return sAlbum;
  }

  ID3_Frame *frame = tag->Find(ID3FID_ALBUM);
  if (frame != NULL)
  {
    sAlbum = GetString(frame, ID3FN_TEXT);
  }
  return sAlbum;
}

char* CMusicInfoTagLoaderMP3::GetTitle(const ID3_Tag *tag)
{
  char *sTitle = NULL;
  if (NULL == tag)
  {
    return sTitle;
  }

  ID3_Frame *frame = tag->Find(ID3FID_TITLE);
  if (frame != NULL)
  {
    sTitle = GetString(frame, ID3FN_TEXT);
  }
  return sTitle;
}

char* CMusicInfoTagLoaderMP3::GetUniqueFileID(const ID3_Tag *tag, const CStdString& strUfidOwner)
{
  if (NULL == tag)
    return NULL;

  //  Iterate through all frames, there can be more 
  //  then one ID3FID_UNIQUEFILEID frame
  ID3_Tag::ConstIterator* itTag=tag->CreateIterator();
  const ID3_Frame *frame=NULL;
  while (frame=itTag->GetNext())
  {
    ID3_FrameID frameid=frame->GetID();
    if (frameid!=ID3FID_UNIQUEFILEID)
      continue;

    //  Extract the owner of this file id
    ID3_Field* fld=frame->GetField(ID3FN_OWNER);
    if (fld != NULL)
    {
      CStdString strIdentifier=fld->GetRawText();
      if (strIdentifier!=strUfidOwner)
        continue;
    }

    //  Extract the owner data of this file id
    fld=frame->GetField(ID3FN_DATA);
    if (fld != NULL)
    {
      size_t nText = fld->Size();
      char* text = LEAKTESTNEW(char[nText+1]);
      memset(text, 0, nText+1);
      memcpy(text, fld->GetRawBinary(), nText);
      return text;
    }
  }

  delete itTag;

  return NULL;
}

char* CMusicInfoTagLoaderMP3::GetUserText(const ID3_Tag *tag, const CStdString& strDescription)
{
  if (NULL == tag)
    return NULL;

  //  Iterate through all frames, there can be more 
  //  then one ID3FID_UNIQUEFILEID frame
  ID3_Tag::ConstIterator* itTag=tag->CreateIterator();
  const ID3_Frame *frame=NULL;
  while (frame=itTag->GetNext())
  {
    ID3_FrameID frameid=frame->GetID();
    if (frameid!=ID3FID_USERTEXT)
      continue;

    //  Extract the owner of this file id
    ID3_Field* fld=frame->GetField(ID3FN_DESCRIPTION);
    if (fld != NULL)
    {
      CStdString strIdentifier=fld->GetRawText();
      if (strIdentifier!=strDescription)
        continue;
    }

    //  Extract the owner data of this file id
    fld=frame->GetField(ID3FN_TEXT);
    if (fld != NULL)
    {
      size_t nText = fld->Size();
      char* text = LEAKTESTNEW(char[nText+1]);
      memset(text, 0, nText+1);
      memcpy(text, fld->GetRawText(), nText);
      return text;
    }
  }

  delete itTag;

  return NULL;
}

char* CMusicInfoTagLoaderMP3::GetMusicBrainzTrackID(const ID3_Tag *tag)
{
  return GetUniqueFileID(tag, "http://musicbrainz.org");
}

char* CMusicInfoTagLoaderMP3::GetMusicBrainzArtistID(const ID3_Tag *tag)
{
  return GetUserText(tag, "MusicBrainz Artist Id");
}

char* CMusicInfoTagLoaderMP3::GetMusicBrainzAlbumID(const ID3_Tag *tag)
{
  return GetUserText(tag, "MusicBrainz Album Id");
}

char* CMusicInfoTagLoaderMP3::GetMusicBrainzAlbumArtistID(const ID3_Tag *tag)
{
  return GetUserText(tag, "MusicBrainz Album Artist Id");
}

char* CMusicInfoTagLoaderMP3::GetMusicBrainzTRMID(const ID3_Tag *tag)
{
  return GetUserText(tag, "MusicBrainz TRM Id");
}

bool CMusicInfoTagLoaderMP3::ReadTag( ID3_Tag& id3tag, CMusicInfoTag& tag )
{
  bool bResult = false;

  SYSTEMTIME dateTime;
  auto_aptr<char>pYear    (ID3_GetYear(&id3tag));
  auto_aptr<char>pTitle   (GetTitle(&id3tag));
  auto_aptr<char>pArtist  (GetArtist(&id3tag));
  auto_aptr<char>pAlbum   (GetAlbum(&id3tag));
  auto_aptr<char>pGenre   (ID3_GetGenre(&id3tag));
  auto_aptr<char>pMBTID   (GetMusicBrainzTrackID(&id3tag));
  auto_aptr<char>pMBAID   (GetMusicBrainzArtistID(&id3tag));
  auto_aptr<char>pMBBID   (GetMusicBrainzAlbumID(&id3tag));
  auto_aptr<char>pMBABID  (GetMusicBrainzAlbumArtistID(&id3tag));
  auto_aptr<char>pMBTRMID (GetMusicBrainzTRMID(&id3tag));

  GetReplayGainInfo(&id3tag);

  int nTrackNum = ID3_GetTrackNum(&id3tag);

  tag.SetTrackNumber(nTrackNum);

  if (NULL != pGenre.get())
  {
    tag.SetGenre(ParseMP3Genre(pGenre.get()));
  }
  if (NULL != pTitle.get())
  {
    if (strlen(pTitle.get()))
    {
      tag.SetLoaded(true);
      bResult = true;
    }
    tag.SetTitle(pTitle.get());
  }
  if (NULL != pArtist.get())
  {
    tag.SetArtist(pArtist.get());
  }
  if (NULL != pAlbum.get())
  {
    tag.SetAlbum(pAlbum.get());
  }
  if (NULL != pYear.get())
  {
    dateTime.wYear = atoi(pYear.get());
    tag.SetReleaseDate(dateTime);
  }
  if (NULL != pMBTID.get())
  {
    tag.SetMusicBrainzTrackID(pMBTID.get());
  }
  if (NULL != pMBAID.get())
  {
    tag.SetMusicBrainzArtistID(pMBAID.get());
  }
  if (NULL != pMBBID.get())
  {
    tag.SetMusicBrainzAlbumID(pMBBID.get());
  }
  if (NULL != pMBABID.get())
  {
    tag.SetMusicBrainzAlbumArtistID(pMBABID.get());
  }
  if (NULL != pMBTRMID.get())
  {
    tag.SetMusicBrainzTRMID(pMBTRMID.get());
  }

  // extract Cover Art and save as album thumb
  if (ID3_HasPicture(&id3tag))
  {
    ID3_PictureType nPicTyp = ID3PT_COVERFRONT;
    CStdString strExtension;
    bool bFound = false;
    auto_aptr<char>pMimeTyp (ID3_GetMimeTypeOfPicType(&id3tag, nPicTyp));
    if (pMimeTyp.get() == NULL)
    {
      nPicTyp = ID3PT_OTHER;
      auto_aptr<char>pMimeTyp (ID3_GetMimeTypeOfPicType(&id3tag, nPicTyp));
      if (pMimeTyp.get() != NULL)
      {
        strExtension = pMimeTyp.get();
        bFound = true;
      }
    }
    else
    {
      strExtension = pMimeTyp.get();
      bFound = true;
    }

    CStdString strCoverArt, strPath;
    CUtil::GetDirectory(tag.GetURL(), strPath);
    CUtil::GetAlbumThumb(tag.GetAlbum(), strPath, strCoverArt, true);
    if (bFound)
    {
      if (!CUtil::ThumbExists(strCoverArt))
      {
        int nPos = strExtension.Find('/');
        if (nPos > -1)
          strExtension.Delete(0, nPos + 1);

        size_t nBufSize = 0;
        const BYTE* pPic = ID3_GetPictureBufferOfPicType(&id3tag, nPicTyp, &nBufSize );

        if (pPic != NULL && nBufSize > 0)
        {
          CPicture pic;
          if (pic.CreateAlbumThumbnailFromMemory(pPic, nBufSize, strExtension, strCoverArt))
          {
            CUtil::ThumbCacheAdd(strCoverArt, true);
          }
          else
          {
            CUtil::ThumbCacheAdd(strCoverArt, false);
            CLog::Log(LOGERROR, "Tag loader mp3: Unable to create album art for %s (extension=%s, size=%d)", tag.GetURL().c_str(), strExtension.c_str(), nBufSize);
          }
        }
      }
    }
    else
    {
      // id3 has no cover, so add to cache
      // that it does not exist
      CUtil::ThumbCacheAdd(strCoverArt, false);
    }
  }

  return bResult;
}

bool CMusicInfoTagLoaderMP3::Load(const CStdString& strFileName, CMusicInfoTag& tag)
{
  try
  {
    // retrieve the ID3 Tag info from strFileName
    // and put it in tag
    bool bResult = false;
    // CSectionLoader::Load("LIBID3");
    tag.SetURL(strFileName);
    if ( m_file.Open( strFileName.c_str() ) )
    {
      // Do not use ID3TT_ALL, because
      // id3lib reads the ID3V1 tag first
      // then ID3V2 tag is blocked.
      ID3_XIStreamReader reader( m_file );
      if ( m_id3tag.Link(reader, ID3TT_ID3V2) >= 0)
      {
        if ( !(bResult = ReadTag( m_id3tag, tag )) )
        {
          m_id3tag.Clear();
          if ( m_id3tag.Link(reader, ID3TT_ID3V1 ) >= 0 )
          {
            bResult = ReadTag( m_id3tag, tag );
          }
        }
      }
      // Check for an APEv2 tag
      CAPEv2Tag apeTag;
      if (apeTag.ReadTag(strFileName.c_str()))
      { // found - let's copy over the additional info (if any)
        if (apeTag.GetArtist().size())
          tag.SetArtist(apeTag.GetArtist());
        if (apeTag.GetAlbum().size())
          tag.SetAlbum(apeTag.GetAlbum());
        if (apeTag.GetTitle().size())
        {
          bResult = true;
          tag.SetTitle(apeTag.GetTitle());
          tag.SetLoaded();
        }
        if (apeTag.GetGenre().size())
          tag.SetGenre(apeTag.GetGenre());
        if (apeTag.GetYear().size())
        {
          SYSTEMTIME time;
          ZeroMemory(&time, sizeof(SYSTEMTIME));
          time.wYear = atoi(apeTag.GetYear().c_str());
          tag.SetReleaseDate(time);
        }
        if (apeTag.GetTrackNum())
          tag.SetTrackNumber(apeTag.GetTrackNum());
        if (apeTag.GetReplayGain().iHasGainInfo)
          m_replayGainInfo = apeTag.GetReplayGain();
      }

      tag.SetDuration(ReadDuration(strFileName));
      m_file.Close();
    }

    // CSectionLoader::Unload("LIBID3");
    return bResult;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Tag loader mp3: exception in file %s", strFileName.c_str());
  }

  tag.SetLoaded(false);
  return false;
}

/* check if 'head' is a valid mp3 frame header */
bool CMusicInfoTagLoaderMP3::IsMp3FrameHeader(unsigned long head)
{
  if ((head & SYNC_MASK) != (unsigned long)SYNC_MASK) /* bad sync? */
    return false;
  if ((head & VERSION_MASK) == (1 << 19)) /* bad version? */
    return false;
  if (!(head & LAYER_MASK)) /* no layer? */
    return false;
  if ((head & BITRATE_MASK) == BITRATE_MASK) /* bad bitrate? */
    return false;
  if (!(head & BITRATE_MASK)) /* no bitrate? */
    return false;
  if ((head & SAMPLERATE_MASK) == SAMPLERATE_MASK) /* bad sample rate? */
    return false;
  if (((head >> 19) & 1) == 1 &&
      ((head >> 17) & 3) == 3 &&
      ((head >> 16) & 1) == 1)
    return false;
  if ((head & 0xffff0000) == 0xfffe0000)
    return false;

  return true;
}

// Inspired by http://rockbox.haxx.se/ and http://www.xs4all.nl/~rwvtveer/scilla
int CMusicInfoTagLoaderMP3::ReadDuration(const CStdString& strFileName)
{
  int nDuration = 0;
  int nPrependedBytes = 0;
  unsigned char* xing;
  unsigned char* vbri;
  unsigned char buffer[8193];

  /* Make sure file has a ID3v2 tag */
  m_file.Seek(0, SEEK_SET);
  m_file.Read(buffer, 6);

  if (buffer[0] == 'I' &&
      buffer[1] == 'D' &&
      buffer[2] == '3')
  {
    /* Now check what the ID3v2 size field says */
    m_file.Read(buffer, 4);
    nPrependedBytes = UNSYNC(buffer[0], buffer[1], buffer[2], buffer[3]) + 10;
    m_iID3v2Size=nPrependedBytes;
  }

  //raw mp3Data = FileSize - ID3v1 tag - ID3v2 tag
  int nMp3DataSize = (int)m_file.GetLength() - nPrependedBytes;
  if (m_id3tag.HasV1Tag())
    nMp3DataSize -= m_id3tag.GetAppendedBytes();

  const int freqtab[][4] =
    {
      {11025, 12000, 8000, 0}
      ,   /* MPEG version 2.5 */
      {44100, 48000, 32000, 0},  /* MPEG Version 1 */
      {22050, 24000, 16000, 0},  /* MPEG version 2 */
    };

  // Skip ID3V2 tag when reading mp3 data
  m_file.Seek(nPrependedBytes, SEEK_SET);
  m_file.Read(buffer, 8192);

  int frequency = 0, bitrate = 0, bittable = 0;
  int frame_count = 0;
  double tpf = 0.0, bpf = 0.0;
  for (int i = 0; i < 8192; i++)
  {
    unsigned long mpegheader = (unsigned long)(
                                 ( (buffer[i] & 255) << 24) |
                                 ( (buffer[i + 1] & 255) << 16) |
                                 ( (buffer[i + 2] & 255) << 8) |
                                 ( (buffer[i + 3] & 255) )
                               );

    // Do we have a Xing header before the first mpeg frame?
    if (buffer[i ] == 'X' &&
        buffer[i + 1] == 'i' &&
        buffer[i + 2] == 'n' &&
        buffer[i + 3] == 'g')
    {
      if (buffer[i + 7] & VBR_FRAMES_FLAG) /* Is the frame count there? */
      {
        frame_count = BYTES2INT(buffer[i + 8], buffer[i + 8 + 1], buffer[i + 8 + 2], buffer[i + 8 + 3]);
        if (buffer[i + 7] & VBR_TOC_FLAG)
        {
          int iOffset = i + 12;
          if (buffer[i + 7] & VBR_BYTES_FLAG)
          {
            nMp3DataSize = BYTES2INT(buffer[i + 12], buffer[i + 12 + 1], buffer[i + 12 + 2], buffer[i + 12 + 3]);
            iOffset += 4;
          }
          float *offset = new float[101];
          for (int j = 0; j < 100; j++)
            offset[j] = (float)buffer[iOffset + j]/256.0f * nMp3DataSize + nPrependedBytes;
          offset[100] = (float)nMp3DataSize + nPrependedBytes;
          m_seekInfo.SetDuration((float)(frame_count * tpf));
          m_seekInfo.SetOffsets(100, offset);
          delete[] offset;
        }
      }
    }

    if (IsMp3FrameHeader(mpegheader))
    {
      // skip mpeg header
      i += 4;
      int version = 0;
      /* MPEG Audio Version */
      switch (mpegheader & VERSION_MASK)
      {
      case 0:
        /* MPEG version 2.5 is not an official standard */
        version = MPEG_VERSION2_5;
        bittable = MPEG_VERSION2 - 1; /* use the V2 bit rate table */
        break;

      case (1 << 19):
              return 0;

      case (2 << 19):
              /* MPEG version 2 (ISO/IEC 13818-3) */
              version = MPEG_VERSION2;
        bittable = MPEG_VERSION2 - 1;
        break;

      case (3 << 19):
              /* MPEG version 1 (ISO/IEC 11172-3) */
              version = MPEG_VERSION1;
        bittable = MPEG_VERSION1 - 1;
        break;
      }

      int layer = 0;
      switch (mpegheader & LAYER_MASK)
      {
      case (3 << 17):  // LAYER_I
        layer = 1;
        break;
      case (2 << 17):  // LAYER_II
        layer = 2;
        break;
      case (1 << 17):  // LAYER_III
        layer = 3;
        break;
      }

      /* Table of bitrates for MP3 files, all values in kilo.
      * Indexed by version, layer and value of bit 15-12 in header.
      */
      const int bitrate_table[2][4][16] =
        {
          {
            {0},
            {0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, 0},
            {0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 0},
            {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0}
          },
          {
            {0},
            {0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256, 0},
            {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0},
            {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0}
          }
        };

      /* Bitrate */
      int bitindex = (mpegheader & 0xf000) >> 12;
      int freqindex = (mpegheader & 0x0C00) >> 10;
      bitrate = bitrate_table[bittable][layer][bitindex];

      /* Calculate bytes per frame, calculation depends on layer */
      switch (layer)
      {
      case 1:
        bpf = bitrate;
        bpf *= 48000;
        bpf /= freqtab[version][freqindex] << (version - 1);
        break;
      case 2:
      case 3:
        bpf = bitrate;
        bpf *= 144000;
        bpf /= freqtab[version][freqindex] << (version - 1);
        break;
      default:
        bpf = 1;
      }
      double tpfbs[] = { 0, 384.0f, 1152.0f, 1152.0f };
      frequency = freqtab[version][freqindex];
      tpf = tpfbs[layer] / (double) frequency;
      if (version == MPEG_VERSION2_5 && version == MPEG_VERSION2)
        tpf /= 2;

      if (frequency == 0)
        return 0;

      /* Channel mode (stereo/mono) */
      int chmode = (mpegheader & 0xc0) >> 6;
      /* calculate position of Xing VBR header */
      if (version == MPEG_VERSION1)
      {
        if (chmode == 3) /* mono */
          xing = buffer + i + 17;
        else
          xing = buffer + i + 32;
      }
      else
      {
        if (chmode == 3) /* mono */
          xing = buffer + i + 9;
        else
          xing = buffer + i + 17;
      }

      /* calculate position of VBRI header */
      vbri = buffer + i + 32;

      // Do we have a Xing header
      if (xing[0] == 'X' &&
          xing[1] == 'i' &&
          xing[2] == 'n' &&
          xing[3] == 'g')
      {
        if (xing[7] & VBR_FRAMES_FLAG) /* Is the frame count there? */
        {
          frame_count = BYTES2INT(xing[8], xing[8 + 1], xing[8 + 2], xing[8 + 3]);
          if (xing[7] & VBR_TOC_FLAG)
          {
            int iOffset = 12;
            if (xing[7] & VBR_BYTES_FLAG)
            {
              nMp3DataSize = BYTES2INT(xing[12], xing[12 + 1], xing[12 + 2], xing[12 + 3]);
              iOffset += 4;
            }
            float *offset = new float[101];
            for (int j = 0; j < 100; j++)
              offset[j] = (float)xing[iOffset + j]/256.0f * nMp3DataSize + nPrependedBytes;
            offset[100] = (float)nMp3DataSize + nPrependedBytes;
            m_seekInfo.SetDuration((float)(frame_count * tpf));
            m_seekInfo.SetOffsets(100, offset);
            delete[] offset;
          }
        }
      }
      // Get the info from the Lame header (if any)
      if ((xing[0] == 'X' && xing[1] == 'i' && xing[2] == 'n' && xing[3] == 'g') ||
          (xing[0] == 'I' && xing[1] == 'n' && xing[2] == 'f' && xing[3] == 'o'))
      {
        if (ReadLAMETagInfo(xing - 0x24))
        {
          // calculate new (more accurate) duration:
          __int64 lastSample = (__int64)frame_count * (__int64)tpfbs[layer] - m_seekInfo.GetFirstSample() - m_seekInfo.GetLastSample();
          m_seekInfo.SetDuration((float)lastSample / frequency);
        }
      }
      if (vbri[0] == 'V' &&
          vbri[1] == 'B' &&
          vbri[2] == 'R' &&
          vbri[3] == 'I')
      {
        frame_count = BYTES2INT(vbri[14], vbri[14 + 1],
                                vbri[14 + 2], vbri[14 + 3]);
        nMp3DataSize = BYTES2INT(vbri[10], vbri[10 + 1], vbri[10 + 2], vbri[10 + 3]);
        int iSeekOffsets = ((vbri[18] & 0xFF) << 8) | (vbri[19] & 0xFF) + 1;
        float *offset = new float[iSeekOffsets + 1];
        int iScaleFactor = ((vbri[20] & 0xFF) << 8) | (vbri[21] & 0xFF);
        int iOffsetSize = ((vbri[22] & 0xFF) << 8) | (vbri[23] & 0xFF);
        offset[0] = (float)nPrependedBytes;
        for (int j = 0; j < iSeekOffsets; j++)
        {
          DWORD dwOffset = 0;
          for (int k = 0; k < iOffsetSize; k++)
          {
            dwOffset = dwOffset << 8;
            dwOffset += vbri[26 + j*iOffsetSize + k];
          }
          offset[j] += (float)dwOffset * iScaleFactor;
          offset[j + 1] = offset[j];
        }
        offset[iSeekOffsets] = (float)nPrependedBytes + nMp3DataSize;
        m_seekInfo.SetDuration((float)(frame_count * tpf));
        m_seekInfo.SetOffsets(iSeekOffsets, offset);
        delete[] offset;
      }
      // We are done!
      break;
    }
  }

  // Calculate duration if we have a Xing/VBRI VBR file
  if (frame_count > 0)
  {
    double d = tpf * frame_count;
    return (int)d;
  }

  // Normal mp3 with constant bitrate duration
  // Now song length is (filesize without id3v1/v2 tag)/((bitrate)/(8))
  double d = 0;
  if (bitrate > 0)
   d = (double)(nMp3DataSize / ((bitrate * 1000) / 8));
  m_seekInfo.SetDuration((float)d);
  float offset[2];
  offset[0] = (float)nPrependedBytes;
  offset[1] = (float)nPrependedBytes + nMp3DataSize;
  m_seekInfo.SetOffsets(1, offset);
  return (int)d;
}

void CMusicInfoTagLoaderMP3::GetSeekInfo(CVBRMP3SeekHelper &info)
{
  info.SetDuration(m_seekInfo.GetDuration());
  info.SetOffsets(m_seekInfo.GetNumOffsets(), m_seekInfo.GetOffsets());
  info.SetSampleRange(m_seekInfo.GetFirstSample(), m_seekInfo.GetLastSample());
  return;
}

CStdString CMusicInfoTagLoaderMP3::ParseMP3Genre (const CStdString& str)
{
  CStdString strTemp = str;
  //vector<CStdString> vecGenres;
  set<CStdString> setGenres;

  while (!strTemp.IsEmpty())
  {
    // remove any leading spaces
    int i = strTemp.find_first_not_of(" ");
    if (i > 0) strTemp.erase(0, i);

    // pull off the first character
    char p = strTemp[0];

    // start off looking for (something)
    if (p == '(')
    {
      strTemp.erase(0, 1);

      // now look for ((something))
      p = strTemp[0];
      if (p == '(')
      {
        // remove ((something))
        i = strTemp.find_first_of("))");
        strTemp.erase(0, i + 2);
      }
    }

    // no parens, so we have a start of a string
    // push chars into temp string until valid terminator found
    // valid terminators are ) or , or ;
    else
    {
      CStdString t;
      while ((!strTemp.IsEmpty()) && (p != ')') && (p != ',') && (p != ';'))
      {
        strTemp.erase(0, 1);
        t.push_back(p);
        p = strTemp[0];
      }
      // loop exits when terminator is found
      // be sure to remove the terminator
      strTemp.erase(0, 1);

      // remove any leading or trailing white space
      // from temp string
      t.Trim();
      if (!t.size()) continue;

      // if the temp string is natural number try to convert it to a genre string
      if (CUtil::IsNaturalNumber(t))
      {
        char * pEnd;
        long l = strtol(t.c_str(), &pEnd, 0);
        if (l < ID3_NR_OF_V1_GENRES)
        {
          // convert to genre string
          t = ID3_v1_genre_description[l];
        }
      }

      // convert RX to Remix as per ID3 V2.3 spec
      else if ((t == "RX") || (t == "Rx") || (t == "rX") || (t == "rx"))
      {
        t = "Remix";
      }

      // convert CR to Cover as per ID3 V2.3 spec
      else if ((t == "CR") || (t == "Cr") || (t == "cR") || (t == "cr"))
      {
        t = "Cover";
      }

      // insert genre name in set
      setGenres.insert(t);
    }

  }

  // return a " / " seperated string
  CStdString strGenre;
  set<CStdString>::iterator it;
  for (it = setGenres.begin(); it != setGenres.end(); it++)
  {
    CStdString strTemp = *it;
    if (!strGenre.IsEmpty())
      strGenre += " / ";
    strGenre += strTemp;
  }
  return strGenre;
}

bool CMusicInfoTagLoaderMP3::ReadLAMETagInfo(BYTE *b)
{
  if (b[0x9c] != 'L' ||
      b[0x9d] != 'A' ||
      b[0x9e] != 'M' ||
      b[0x9f] != 'E')
    return false;

  // Found LAME tag - extract the start and end offsets
  int iDelay = ((b[0xb1] & 0xFF) << 4) + ((b[0xb2] & 0xF0) >> 4);
  iDelay += 1152; // This header is going to be decoded as a silent frame
  int iPadded = ((b[0xb2] & 0x0F) << 8) + (b[0xb3] & 0xFF);
  m_seekInfo.SetSampleRange(iDelay, iPadded);

  /* Don't read replaygain information from here as no other player respects this.

  // Now do the ReplayGain stuff
  if (!m_replayGainInfo.iHasGainInfo)
  { // haven't found the gain info - let's test here for it
    BYTE *p = b + 0xA7;
    #define REPLAY_GAIN_RADIO 1
    #define REPLAY_GAIN_AUDIOPHILE 2
    m_replayGainInfo.fTrackPeak = *(float *)p;
    if (m_replayGainInfo.fTrackPeak != 0.0f) m_replayGainInfo.iHasGainInfo |= REPLAY_GAIN_HAS_TRACK_PEAK;
    for (int i = 0; i <= 6; i+=2)
    {
      BYTE gainType = (*(p+i) & 0xE0) >> 5;
      BYTE gainFrom = (*(p+i) & 0x1C) >> 2;  // where the gain is from
      if (gainFrom && (gainType == REPLAY_GAIN_RADIO || gainType == REPLAY_GAIN_AUDIOPHILE))
      { // have some replay gain stuff
        int sign = (*(p+i) & 0x02) ? -1 : 1;
        int gainLevel = ((*(p+i) & 0x01) << 8) | (*(p+i+1) & 0xFF);
        gainLevel *= sign;
        if (gainType == REPLAY_GAIN_RADIO)
        {
          m_replayGainInfo.iTrackGain = gainLevel*10;
          m_replayGainInfo.iHasGainInfo |= REPLAY_GAIN_HAS_TRACK_INFO;
        }
        if (gainType == REPLAY_GAIN_AUDIOPHILE)
        {
          m_replayGainInfo.iAlbumGain = gainLevel*10;
          m_replayGainInfo.iHasGainInfo |= REPLAY_GAIN_HAS_ALBUM_INFO;
        }
      }
    }
  }*/
  return true;
}

void CMusicInfoTagLoaderMP3::GetReplayGainInfo(const ID3_Tag *tag)
{
  char *szGain = GetUserText(tag, "replaygain_track_gain");
  if (szGain)
  {
    m_replayGainInfo.iTrackGain = (int)(atof(szGain) * 100 + 0.5);
    m_replayGainInfo.iHasGainInfo |= REPLAY_GAIN_HAS_TRACK_INFO;
    delete[] szGain;
  }
  szGain = GetUserText(tag, "replaygain_album_gain");
  if (szGain)
  {
    m_replayGainInfo.iAlbumGain = (int)(atof(szGain) * 100 + 0.5);
    m_replayGainInfo.iHasGainInfo |= REPLAY_GAIN_HAS_ALBUM_INFO;
    delete[] szGain;
  }
  szGain = GetUserText(tag, "replaygain_track_peak");
  if (szGain)
  {
    m_replayGainInfo.fTrackPeak = (float)atof(szGain);
    m_replayGainInfo.iHasGainInfo |= REPLAY_GAIN_HAS_TRACK_PEAK;
    delete[] szGain;
  }
  szGain = GetUserText(tag, "replaygain_album_peak");
  if (szGain)
  {
    m_replayGainInfo.fAlbumPeak = (float)atof(szGain);
    m_replayGainInfo.iHasGainInfo |= REPLAY_GAIN_HAS_ALBUM_PEAK;
    delete[] szGain;
  }
}

bool CMusicInfoTagLoaderMP3::GetReplayGain(CReplayGain &info)
{
  if (!m_replayGainInfo.iHasGainInfo)
    return false;
  info = m_replayGainInfo;
  return true;
}