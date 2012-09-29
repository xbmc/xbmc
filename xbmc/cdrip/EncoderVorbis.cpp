/*
 *      Copyright (C) 2005-2012 Team XBMC
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

#include "EncoderVorbis.h"
#include "settings/GUISettings.h"
#include "utils/log.h"

CEncoderVorbis::CEncoderVorbis()
{
  m_pBuffer = NULL;
}

bool CEncoderVorbis::Init(const char* strFile, int iInChannels, int iInRate, int iInBits)
{
  // we only accept 2 / 44100 / 16 atm
  if (iInChannels != 2 || iInRate != 44100 || iInBits != 16) return false;

  // set input stream information and open the file
  if (!CEncoder::Init(strFile, iInChannels, iInRate, iInBits)) return false;

  float fQuality = 0.5f;
  if (g_guiSettings.GetInt("audiocds.quality") == CDDARIP_QUALITY_MEDIUM) fQuality = 0.4f;
  if (g_guiSettings.GetInt("audiocds.quality") == CDDARIP_QUALITY_STANDARD) fQuality = 0.5f;
  if (g_guiSettings.GetInt("audiocds.quality") == CDDARIP_QUALITY_EXTREME) fQuality = 0.7f;

  if (!m_VorbisEncDll.Load() || !m_OggDll.Load() || !m_VorbisDll.Load())
  {
    // failed loading the dll's, unload it all
    CLog::Log(LOGERROR, "CEncoderVorbis::Init() Error while loading ogg.dll and or vorbis.dll");
    return false;
  }

  m_VorbisDll.vorbis_info_init(&m_sVorbisInfo);
  if (g_guiSettings.GetInt("audiocds.quality") == CDDARIP_QUALITY_CBR)
  {
    // not realy cbr, but abr in this case
    int iBitRate = g_guiSettings.GetInt("audiocds.bitrate") * 1000;
    m_VorbisEncDll.vorbis_encode_init(&m_sVorbisInfo, m_iInChannels, m_iInSampleRate, -1, iBitRate, -1);
  }
  else
  {
    if (m_VorbisEncDll.vorbis_encode_init_vbr(&m_sVorbisInfo, m_iInChannels, m_iInSampleRate, fQuality)) return false;
  }

  /* add a comment */
  m_VorbisDll.vorbis_comment_init(&m_sVorbisComment);
  m_VorbisDll.vorbis_comment_add_tag(&m_sVorbisComment, (char*)"comment", (char*)m_strComment.c_str());
  m_VorbisDll.vorbis_comment_add_tag(&m_sVorbisComment, (char*)"artist", (char*)m_strArtist.c_str());
  m_VorbisDll.vorbis_comment_add_tag(&m_sVorbisComment, (char*)"title", (char*)m_strTitle.c_str());
  m_VorbisDll.vorbis_comment_add_tag(&m_sVorbisComment, (char*)"album", (char*)m_strAlbum.c_str());
  m_VorbisDll.vorbis_comment_add_tag(&m_sVorbisComment, (char*)"albumartist", (char*)m_strAlbumArtist.c_str());
  m_VorbisDll.vorbis_comment_add_tag(&m_sVorbisComment, (char*)"genre", (char*)m_strGenre.c_str());
  m_VorbisDll.vorbis_comment_add_tag(&m_sVorbisComment, (char*)"tracknumber", (char*)m_strTrack.c_str());
  m_VorbisDll.vorbis_comment_add_tag(&m_sVorbisComment, (char*)"date", (char*)m_strYear.c_str());

  /* set up the analysis state and auxiliary encoding storage */
  m_VorbisDll.vorbis_analysis_init(&m_sVorbisDspState, &m_sVorbisInfo);

  m_VorbisDll.vorbis_block_init(&m_sVorbisDspState, &m_sVorbisBlock);

  /* set up our packet->stream encoder */
  /* pick a random serial number; that way we can more likely build
  chained streams just by concatenation */
  srand(time(NULL));
  m_OggDll.ogg_stream_init(&m_sOggStreamState, rand());

  {
    ogg_packet header;
    ogg_packet header_comm;
    ogg_packet header_code;

    m_VorbisDll.vorbis_analysis_headerout(&m_sVorbisDspState, &m_sVorbisComment,
                              &header, &header_comm, &header_code);

    m_OggDll.ogg_stream_packetin(&m_sOggStreamState, &header);
    m_OggDll.ogg_stream_packetin(&m_sOggStreamState, &header_comm);
    m_OggDll.ogg_stream_packetin(&m_sOggStreamState, &header_code);

    /* This ensures the actual
    * audio data will start on a new page, as per spec
    */
    while (1)
    {
      int result = m_OggDll.ogg_stream_flush(&m_sOggStreamState, &m_sOggPage);
      if (result == 0)break;
      FileWrite(m_sOggPage.header, m_sOggPage.header_len);
      FileWrite(m_sOggPage.body, m_sOggPage.body_len);
    }
  }
  m_pBuffer = new BYTE[4096];

  return true;
}

int CEncoderVorbis::Encode(int nNumBytesRead, BYTE* pbtStream)
{
  int eos = 0;

  /* data to encode */
  LONG nBlocks = (int)(nNumBytesRead / 4096);
  LONG nBytesleft = nNumBytesRead - nBlocks * 4096;
  LONG block = 4096;

  for (int a = 0; a <= nBlocks; a++)
  {
    if (a == nBlocks)
    {
      // no more blocks of 4096 bytes to write, just write the last bytes
      block = nBytesleft;
    }

    /* expose the buffer to submit data */
    float **buffer = m_VorbisDll.vorbis_analysis_buffer(&m_sVorbisDspState, 1024);

    /* uninterleave samples */
    memcpy(m_pBuffer, pbtStream, block);
    pbtStream += 4096;
    LONG iSamples = block / (2 * 2);
    signed char* buf = (signed char*) m_pBuffer;
    for (int i = 0; i < iSamples; i++)
    {
      int j = i << 2; // j = i * 4
      buffer[0][i] = (((long)buf[j + 1] << 8) | (0x00ff & (int)buf[j])) / 32768.0f;
      buffer[1][i] = (((long)buf[j + 3] << 8) | (0x00ff & (int)buf[j + 2])) / 32768.0f;
    }

    /* tell the library how much we actually submitted */
    m_VorbisDll.vorbis_analysis_wrote(&m_sVorbisDspState, iSamples);

    /* vorbis does some data preanalysis, then divvies up blocks for
    more involved (potentially parallel) processing.  Get a single
    block for encoding now */
    while (m_VorbisDll.vorbis_analysis_blockout(&m_sVorbisDspState, &m_sVorbisBlock) == 1)
    {
      /* analysis, assume we want to use bitrate management */
      m_VorbisDll.vorbis_analysis(&m_sVorbisBlock, NULL);
      m_VorbisDll.vorbis_bitrate_addblock(&m_sVorbisBlock);

      while (m_VorbisDll.vorbis_bitrate_flushpacket(&m_sVorbisDspState, &m_sOggPacket))
      {
        /* weld the packet into the bitstream */
        m_OggDll.ogg_stream_packetin(&m_sOggStreamState, &m_sOggPacket);

        /* write out pages (if any) */
        while (!eos)
        {
          int result = m_OggDll.ogg_stream_pageout(&m_sOggStreamState, &m_sOggPage);
          if (result == 0)break;
          WriteStream(m_sOggPage.header, m_sOggPage.header_len);
          WriteStream(m_sOggPage.body, m_sOggPage.body_len);

          /* this could be set above, but for illustrative purposes, I do
          it here (to show that vorbis does know where the stream ends) */
          if (m_OggDll.ogg_page_eos(&m_sOggPage)) eos = 1;
        }
      }
    }
  }

  return 1;
}

bool CEncoderVorbis::Close()
{
  int eos = 0;
  // tell vorbis we are encoding the end of the stream
  m_VorbisDll.vorbis_analysis_wrote(&m_sVorbisDspState, 0);
  while (m_VorbisDll.vorbis_analysis_blockout(&m_sVorbisDspState, &m_sVorbisBlock) == 1)
  {
    /* analysis, assume we want to use bitrate management */
    m_VorbisDll.vorbis_analysis(&m_sVorbisBlock, NULL);
    m_VorbisDll.vorbis_bitrate_addblock(&m_sVorbisBlock);

    while (m_VorbisDll.vorbis_bitrate_flushpacket(&m_sVorbisDspState, &m_sOggPacket))
    {
      /* weld the packet into the bitstream */
      m_OggDll.ogg_stream_packetin(&m_sOggStreamState, &m_sOggPacket);

      /* write out pages (if any) */
      while (!eos)
      {
        int result = m_OggDll.ogg_stream_pageout(&m_sOggStreamState, &m_sOggPage);
        if (result == 0)break;
        WriteStream(m_sOggPage.header, m_sOggPage.header_len);
        WriteStream(m_sOggPage.body, m_sOggPage.body_len);

        /* this could be set above, but for illustrative purposes, I do
        it here (to show that vorbis does know where the stream ends) */
        if (m_OggDll.ogg_page_eos(&m_sOggPage)) eos = 1;
      }
    }
  }

  /* clean up and exit.  vorbis_info_clear() must be called last */
  m_OggDll.ogg_stream_clear(&m_sOggStreamState);
  m_VorbisDll.vorbis_block_clear(&m_sVorbisBlock);
  m_VorbisDll.vorbis_dsp_clear(&m_sVorbisDspState);
  m_VorbisDll.vorbis_comment_clear(&m_sVorbisComment);
  m_VorbisDll.vorbis_info_clear(&m_sVorbisInfo);

  /* ogg_page and ogg_packet structs always point to storage in
     libvorbis.  They're never freed or manipulated directly */
  FlushStream();
  FileClose();

  delete []m_pBuffer;
  m_pBuffer = NULL;

  m_VorbisEncDll.Unload();

  m_OggDll.Unload();

  m_VorbisDll.Unload();

  return true;
}
