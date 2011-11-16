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

#include "MusicInfoTagLoaderMP3.h"
#include "APEv2Tag.h"
#include "Id3Tag.h"
#include "settings/AdvancedSettings.h"
#include "filesystem/File.h"
#include "utils/log.h"

using namespace MUSIC_INFO;

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

}

CMusicInfoTagLoaderMP3::~CMusicInfoTagLoaderMP3()
{
}

bool CMusicInfoTagLoaderMP3::Load(const CStdString& strFileName, CMusicInfoTag& tag)
{
  try
  {
    // retrieve the ID3 Tag info from strFileName
    // and put it in tag
    CID3Tag id3tag;
    if (id3tag.Read(strFileName))
    {
      id3tag.GetMusicInfoTag(tag);
      m_replayGainInfo=id3tag.GetReplayGain();
    }

    // Check for an APEv2 tag
    CAPEv2Tag apeTag;
    if (PrioritiseAPETags() && apeTag.ReadTag(strFileName.c_str()))
    { // found - let's copy over the additional info (if any)
      if (apeTag.GetArtist().size())
      {
        tag.SetArtist(apeTag.GetArtist());
        tag.SetLoaded();
      }
      if (apeTag.GetAlbum().size())
      {
        tag.SetAlbum(apeTag.GetAlbum());
        tag.SetLoaded();
      }
      if (apeTag.GetAlbumArtist().size())
      {
        tag.SetAlbumArtist(apeTag.GetAlbumArtist());
        tag.SetLoaded();
      }
      if (apeTag.GetTitle().size())
      {
        tag.SetTitle(apeTag.GetTitle());
        tag.SetLoaded();
      }
      if (apeTag.GetGenre().size())
        tag.SetGenre(apeTag.GetGenre());

      if (apeTag.GetLyrics().size())
        tag.SetLyrics(apeTag.GetLyrics());

      if (apeTag.GetYear().size())
      {
        SYSTEMTIME time;
        ZeroMemory(&time, sizeof(SYSTEMTIME));
        time.wYear = atoi(apeTag.GetYear().c_str());
        tag.SetReleaseDate(time);
      }
      if (apeTag.GetTrackNum())
        tag.SetTrackNumber(apeTag.GetTrackNum());
      if (apeTag.GetDiscNum())
        tag.SetPartOfSet(apeTag.GetDiscNum());
      if (apeTag.GetComment().size())
        tag.SetComment(apeTag.GetComment());
      if (apeTag.GetReplayGain().iHasGainInfo)
        m_replayGainInfo = apeTag.GetReplayGain();
      if (apeTag.GetRating() > '0')
        tag.SetRating(apeTag.GetRating());
    }

    tag.SetDuration(ReadDuration(strFileName));

    return tag.Loaded();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Tag loader mp3: exception in file %s", strFileName.c_str());
  }

  tag.SetLoaded(false);
  return false;
}

bool CMusicInfoTagLoaderMP3::ReadSeekAndReplayGainInfo(const CStdString &strFileName)
{
  // First check for an APEv2 tag
  CAPEv2Tag apeTag;
  if (apeTag.ReadTag(strFileName.c_str()))
  { // found - let's copy over the additional info (if any)
    if (apeTag.GetReplayGain().iHasGainInfo)
      m_replayGainInfo = apeTag.GetReplayGain();
  }

  if (!m_replayGainInfo.iHasGainInfo)
  { // Nothing found query id3 tag
    CID3Tag id3tag;
    if (id3tag.Read(strFileName))
    {
      if (id3tag.GetReplayGain().iHasGainInfo)
        m_replayGainInfo = id3tag.GetReplayGain();
    }
  }

  // now read the duration
  int duration = ReadDuration(strFileName);

  return duration>0 ? true : false;
}

/* check if 'head' is a valid mp3 frame header and return the framesize if it is (0 otherwise) */
int CMusicInfoTagLoaderMP3::IsMp3FrameHeader(unsigned long head)
{
  const long freqs[9] = { 44100, 48000, 32000,
                          22050, 24000, 16000 ,
                          11025 , 12000 , 8000 };

 const int tabsel_123[2][3][16] = {
  { {128,32,64,96,128,160,192,224,256,288,320,352,384,416,448,},
    {128,32,48,56, 64, 80, 96,112,128,160,192,224,256,320,384,},
    {128,32,40,48, 56, 64, 80, 96,112,128,160,192,224,256,320,} },

  { {128,32,48,56,64,80,96,112,128,144,160,176,192,224,256,},
    {128,8,16,24,32,40,48,56,64,80,96,112,128,144,160,},
    {128,8,16,24,32,40,48,56,64,80,96,112,128,144,160,} }
};


  if ((head & SYNC_MASK) != (unsigned long)SYNC_MASK) /* bad sync? */
    return 0;
  if ((head & VERSION_MASK) == (1 << 19)) /* bad version? */
    return 0;
  if (!(head & LAYER_MASK)) /* no layer? */
    return 0;
  if ((head & BITRATE_MASK) == BITRATE_MASK) /* bad bitrate? */
    return 0;
  if (!(head & BITRATE_MASK)) /* no bitrate? */
    return 0;
  if ((head & SAMPLERATE_MASK) == SAMPLERATE_MASK) /* bad sample rate? */
    return 0;
  if (((head >> 19) & 1) == 1 &&
      ((head >> 17) & 3) == 3 &&
      ((head >> 16) & 1) == 1)
    return 0;
  if ((head & 0xffff0000) == 0xfffe0000)
    return 0;

  int srate = 0;
  if(!((head >> 20) &  1))
    srate = 6 + ((head>>10)&0x3);
  else
    srate = ((head>>10)&0x3) + ((1-((head >> 19) &  1)) * 3);

  int framesize = tabsel_123[1 - ((head >> 19) &  1)][(4-((head>>17)&3))-1][((head>>12)&0xf)]*144000/(freqs[srate]<<(1 - ((head >> 19) &  1)))+((head>>9)&0x1);
  return framesize;
}

//TODO: merge duplicate, but slitely different implemented) code and consts in IsMp3FrameHeader(above) and ReadDuration (below).

// Inspired by http://rockbox.haxx.se/ and http://www.xs4all.nl/~rwvtveer/scilla
int CMusicInfoTagLoaderMP3::ReadDuration(const CStdString& strFileName)
{
#define SCANSIZE  8192
#define CHECKNUMFRAMES 5
#define ID3V2HEADERSIZE 10

  unsigned char* xing;
  unsigned char* vbri;
  unsigned char buffer[SCANSIZE + 1];

  const int freqtab[][4] =
    {
      {11025, 12000, 8000, 0}
      ,   /* MPEG version 2.5 */
      {44100, 48000, 32000, 0},  /* MPEG Version 1 */
      {22050, 24000, 16000, 0},  /* MPEG version 2 */
    };


  CFile file;
  if (!file.Open(strFileName))
    return 0;

  /* Check if the file has an ID3v1 tag */
  file.Seek(file.GetLength()-128, SEEK_SET);
  file.Read(buffer, 3);

  bool hasid3v1=false;
  if (buffer[0] == 'T' &&
      buffer[1] == 'A' &&
      buffer[2] == 'G')
  {
    hasid3v1=true;
  }

  /* Check if the file has an ID3v2 tag (or multiple tags) */
  unsigned int id3v2Size = 0;
  file.Seek(0, SEEK_SET);
  file.Read(buffer, ID3V2HEADERSIZE);
  unsigned int size = IsID3v2Header(buffer, ID3V2HEADERSIZE);
  while (size)
  {
    id3v2Size += size;
    if (id3v2Size != file.Seek(id3v2Size, SEEK_SET))
      return 0;
    if (ID3V2HEADERSIZE != file.Read(buffer, ID3V2HEADERSIZE))
      return 0;
    size = IsID3v2Header(buffer, ID3V2HEADERSIZE);
  }

  //skip any padding
  //already read ID3V2HEADERSIZE bytes so take it into account
  int iScanSize = file.Read(buffer + ID3V2HEADERSIZE, SCANSIZE - ID3V2HEADERSIZE) + ID3V2HEADERSIZE;
  int iBufferDataStart;
  do
  {
    iBufferDataStart = -1;
    for(int i = 0; i < iScanSize; i++)
    {
      //all 0x00's after id3v2 tag until first mpeg-frame are padding
      if (buffer[i] != 0)
      {
        iBufferDataStart = i;
        break;
      }
    }
    if (iBufferDataStart == -1)
    {
      id3v2Size += iScanSize;
      iScanSize = file.Read(buffer, SCANSIZE);
    }
    else
    {
      id3v2Size += iBufferDataStart;
    }
  } while (iBufferDataStart == -1 && iScanSize > 0);
  if (iScanSize <= 0)
    return 0;
  if (iBufferDataStart > 0)
  {
    //move data to front of buffer
    iScanSize -= iBufferDataStart;
    memcpy(buffer, buffer + iBufferDataStart, iScanSize);
    //fill remainder of buffer with new data
    iScanSize += file.Read(buffer + iScanSize, SCANSIZE - iScanSize);
  }

  int firstFrameOffset = id3v2Size;


  //raw mp3Data = FileSize - ID3v1 tag - ID3v2 tag
  int nMp3DataSize = (int)file.GetLength() - id3v2Size;
  if (hasid3v1)
    nMp3DataSize -= 128;

  //*** find the first frame in the buffer, we do this by checking if the calculated framesize leads to the next frame a couple of times.
  //the first frame that leads to a valid next frame a couple of times is where we should start decoding.
  int firstValidFrameLocation = 0;
  for(int i = 0; i < iScanSize; i++)
  {
    int j = i;
    int framesize = 1;
    int numFramesCheck = 0;

    for (numFramesCheck = 0; (numFramesCheck < CHECKNUMFRAMES) && framesize; numFramesCheck++)
    {
      unsigned long mpegheader = (unsigned long)(
        ( (buffer[j] & 255) << 24) |
        ( (buffer[j + 1] & 255) << 16) |
        ( (buffer[j + 2] & 255) << 8) |
        ( (buffer[j + 3] & 255) )
      );
      framesize = IsMp3FrameHeader(mpegheader);

      j += framesize;

      if ((j + 4) >= iScanSize)
      {
        //no valid frame found in buffer
        firstValidFrameLocation = -1;
        break;
      }
    }

    if (numFramesCheck == CHECKNUMFRAMES)
    { //found it
      firstValidFrameLocation = i;
      break;
    }
  }

  if (firstValidFrameLocation != -1)
  {
    firstFrameOffset += firstValidFrameLocation;
    nMp3DataSize -= firstValidFrameLocation;
  }
  //*** done finding first valid frame

  //find lame/xing info
  int frequency = 0, bitrate = 0, bittable = 0;
  int frame_count = 0;
  double tpf = 0.0, bpf = 0.0;
  for (int i = 0; i < iScanSize; i++)
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
            offset[j] = (float)buffer[iOffset + j]/256.0f * nMp3DataSize + firstFrameOffset;
          offset[100] = (float)nMp3DataSize + firstFrameOffset;
          m_seekInfo.SetOffsets(100, offset);
          delete[] offset;
        }
      }
    }
    /*else if (buffer[i] == 'I' && buffer[i + 1] == 'n' && buffer[i + 2] == 'f' && buffer[i + 3] == 'o') //Info is used when CBR
    {
      //should we do something with this?
    }*/

    if (
      (i == firstValidFrameLocation) ||
      (
        (firstValidFrameLocation == -1) &&
        (IsMp3FrameHeader(mpegheader))
      )
    )
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
              offset[j] = (float)xing[iOffset + j]/256.0f * nMp3DataSize + firstFrameOffset;
            //first offset should be the location of the first frame, usually it is but some files have seektables that are a little off.
            offset[0]   = (float)firstFrameOffset;
            offset[100] = (float)nMp3DataSize + firstFrameOffset;
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
          int64_t lastSample = (int64_t)frame_count * (int64_t)tpfbs[layer] - m_seekInfo.GetFirstSample() - m_seekInfo.GetLastSample();
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
        int iSeekOffsets = (((vbri[18] & 0xFF) << 8) | (vbri[19] & 0xFF)) + 1;
        float *offset = new float[iSeekOffsets + 1];
        int iScaleFactor = ((vbri[20] & 0xFF) << 8) | (vbri[21] & 0xFF);
        int iOffsetSize = ((vbri[22] & 0xFF) << 8) | (vbri[23] & 0xFF);
        offset[0] = (float)firstFrameOffset;
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
        offset[iSeekOffsets] = (float)firstFrameOffset + nMp3DataSize;
        m_seekInfo.SetOffsets(iSeekOffsets, offset);
        delete[] offset;
      }
      // We are done!
      break;
    }
  }

  if (m_seekInfo.GetNumOffsets() == 0)
  {
    float offset[2];
    offset[0] = (float)firstFrameOffset;
    offset[1] = (float)(firstFrameOffset + nMp3DataSize);
    m_seekInfo.SetOffsets(1, offset);
  }

  // Calculate duration if we have a Xing/VBRI VBR file
  if (frame_count > 0)
  {
    double d = tpf * frame_count;
    m_seekInfo.SetDuration((float)d);
    return (int)d;
  }

  // Normal mp3 with constant bitrate duration
  // Now song length is (filesize without id3v1/v2 tag)/((bitrate)/(8))
  double d = 0;
  if (bitrate > 0)
   d = (double)(nMp3DataSize / ((bitrate * 1000) / 8));
  m_seekInfo.SetDuration((float)d);
  return (int)d;
}

void CMusicInfoTagLoaderMP3::GetSeekInfo(CVBRMP3SeekHelper &info) const
{
  info.SetDuration(m_seekInfo.GetDuration());
  info.SetOffsets(m_seekInfo.GetNumOffsets(), m_seekInfo.GetOffsets());
  info.SetSampleRange(m_seekInfo.GetFirstSample(), m_seekInfo.GetLastSample());
  return;
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

bool CMusicInfoTagLoaderMP3::GetReplayGain(CReplayGain &info) const
{
  if (!m_replayGainInfo.iHasGainInfo)
    return false;
  info = m_replayGainInfo;
  return true;
}

bool CMusicInfoTagLoaderMP3::PrioritiseAPETags() const
{
  return g_advancedSettings.m_prioritiseAPEv2tags;
}

// \brief Check to see if the specified buffer contains an ID3v2 tag header
// \param pBuf Pointer to the buffer to be examined. Must be at least 10 bytes long.
// \param bufLen Size of the buffer pointer to by pBuf. Must be at least 10.
// \return The length of the ID3v2 tag (including the header) if one is present, otherwise 0
unsigned int CMusicInfoTagLoaderMP3::IsID3v2Header(unsigned char* pBuf, size_t bufLen)
{
  unsigned int tagLen = 0;
  if (bufLen < 10 || pBuf[0] != 'I' || pBuf[1] != 'D' || pBuf[2] != '3')
    return 0; // Buffer is too small for complete header, or no header signature detected

  // Retrieve the tag size (including this header)
  tagLen = UNSYNC(pBuf[6], pBuf[7], pBuf[8], pBuf[9]) + 10;

  if (pBuf[5] & 0x10) // Header is followed by a footer
    tagLen += 10; //Add footer size

  return tagLen;
}
