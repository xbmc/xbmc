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

#include "EncoderFlac.h"
#include "utils/log.h"
#include "filesystem/File.h"
#include "settings/GUISettings.h"

CEncoderFlac::CEncoderFlac() : m_encoder(0), m_samplesBuf(new FLAC__int32[SAMPLES_BUF_SIZE])
{
  m_metadata[0] = 0;
  m_metadata[1] = 0;
}

CEncoderFlac::~CEncoderFlac()
{
  delete [] m_samplesBuf;
}

bool CEncoderFlac::Init(const char* strFile, int iInChannels, int iInRate, int iInBits)
{
  // we only accept 2 / 44100 / 16 atm
  if (iInChannels != 2 || iInRate != 44100 || iInBits != 16)
    return false;

  // set input stream information and open the file
  if (!CEncoder::Init(strFile, iInChannels, iInRate, iInBits))
    return false;

  // load the flac dll
  if (!m_dll.Load())
    return false;

  // allocate libFLAC encoder
  m_encoder = m_dll.FLAC__stream_encoder_new();
  if (!m_encoder)
  {
    CLog::Log(LOGERROR, "Error: FLAC__stream_encoder_new() failed");
    return false;
  }

  FLAC__bool ok = 1;

  ok &= m_dll.FLAC__stream_encoder_set_verify(m_encoder, true);
  ok &= m_dll.FLAC__stream_encoder_set_channels(m_encoder, 2);
  ok &= m_dll.FLAC__stream_encoder_set_bits_per_sample(m_encoder, 16);
  ok &= m_dll.FLAC__stream_encoder_set_sample_rate(m_encoder, 44100);
  ok &= m_dll.FLAC__stream_encoder_set_total_samples_estimate(m_encoder, m_iTrackLength / 4);
  ok &= m_dll.FLAC__stream_encoder_set_compression_level(m_encoder, g_guiSettings.GetInt("audiocds.compressionlevel"));

  // now add some metadata
  FLAC__StreamMetadata_VorbisComment_Entry entry;
  if (ok)
  {
    if (
      (m_metadata[0] = m_dll.FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT)) == NULL ||
      (m_metadata[1] = m_dll.FLAC__metadata_object_new(FLAC__METADATA_TYPE_PADDING)) == NULL ||
      !m_dll.FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "ARTIST", m_strArtist.c_str()) ||
      !m_dll.FLAC__metadata_object_vorbiscomment_append_comment(m_metadata[0], entry, false) ||
      !m_dll.FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "ALBUM", m_strAlbum.c_str()) ||
      !m_dll.FLAC__metadata_object_vorbiscomment_append_comment(m_metadata[0], entry, false) ||
      !m_dll.FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "ALBUMARTIST", m_strAlbumArtist.c_str()) || 
      !m_dll.FLAC__metadata_object_vorbiscomment_append_comment(m_metadata[0], entry, false) || 
      !m_dll.FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "TITLE", m_strTitle.c_str()) ||
      !m_dll.FLAC__metadata_object_vorbiscomment_append_comment(m_metadata[0], entry, false) ||
      !m_dll.FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "GENRE", m_strGenre.c_str()) ||
      !m_dll.FLAC__metadata_object_vorbiscomment_append_comment(m_metadata[0], entry, false) ||
      !m_dll.FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "TRACKNUMBER", m_strTrack.c_str()) ||
      !m_dll.FLAC__metadata_object_vorbiscomment_append_comment(m_metadata[0], entry, false) ||
      !m_dll.FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "DATE", m_strYear.c_str()) ||
      !m_dll.FLAC__metadata_object_vorbiscomment_append_comment(m_metadata[0], entry, false) ||
      !m_dll.FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "COMMENT", m_strComment.c_str()) ||
      !m_dll.FLAC__metadata_object_vorbiscomment_append_comment(m_metadata[0], entry, false)
      ) 
    {
      CLog::Log(LOGERROR, "ERROR: FLAC out of memory or tag error\n");
      ok = false;
    }
    else
    {
      m_metadata[1]->length = 4096;
      ok = m_dll.FLAC__stream_encoder_set_metadata(m_encoder, m_metadata, 2);
    }
  }

  // initialize encoder in stream mode
  if (ok)
  {
    FLAC__StreamEncoderInitStatus init_status;
    init_status = m_dll.FLAC__stream_encoder_init_stream(m_encoder, write_callback, NULL, NULL, NULL, this);
    if (init_status != FLAC__STREAM_ENCODER_INIT_STATUS_OK)
	  {
      CLog::Log(LOGERROR, "FLAC encoder initializing error");
      ok = false;
    }
  }

  if (!ok)
  {
    CLog::Log(LOGERROR, "Error: FLAC intialization failed");
    return false;
  }

  return true;
}

int CEncoderFlac::Encode(int nNumBytesRead, BYTE* pbtStream)
{
  int nLeftSamples = nNumBytesRead / 2; // each sample takes 2 bytes (16 bits per sample)
  while (nLeftSamples > 0)
  {
    int nSamples = nLeftSamples > SAMPLES_BUF_SIZE ? SAMPLES_BUF_SIZE : nLeftSamples;

    // convert the packed little-endian 16-bit PCM samples into an interleaved FLAC__int32 buffer for libFLAC
    for (int i = 0; i < nSamples; i++)
    { // inefficient but simple and works on big- or little-endian machines.
      m_samplesBuf[i] = (FLAC__int32)(((FLAC__int16)(FLAC__int8)pbtStream[2*i+1] << 8) | (FLAC__int16)pbtStream[2*i]);
    }

    // feed samples to encoder
    if (!m_dll.FLAC__stream_encoder_process_interleaved(m_encoder, m_samplesBuf, nSamples / 2))
    {
      CLog::Log(LOGERROR, "FLAC__stream_encoder_process_interleaved error");
      return 0;
    }

    nLeftSamples -= nSamples;
    pbtStream += nSamples * 2; // skip processed samples
  }

  return 1;
}

bool CEncoderFlac::Close()
{
  FLAC__bool ok = 0;

  if (m_encoder)
  {
    // finish encoding
    ok = m_dll.FLAC__stream_encoder_finish(m_encoder);
    if (!ok)
      CLog::Log(LOGERROR, "FLAC encoder finish error");

    // now that encoding is finished, the metadata can be freed
    if (m_metadata[0])
      m_dll.FLAC__metadata_object_delete(m_metadata[0]);
    if (m_metadata[1])
      m_dll.FLAC__metadata_object_delete(m_metadata[1]);

    // delete encoder
    m_dll.FLAC__stream_encoder_delete(m_encoder);
  }

  FlushStream();
  FileClose();

  // unload the flac dll
  m_dll.Unload();

  return ok ? true : false;
}

FLAC__StreamEncoderWriteStatus CEncoderFlac::write_callback(const FLAC__StreamEncoder *encoder, const FLAC__byte buffer[], size_t bytes, unsigned samples, unsigned current_frame, void *client_data)
{
  CEncoderFlac *pThis = (CEncoderFlac *)client_data;
  if (pThis->FileWrite(buffer, bytes) != (int)bytes)
    return FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;
  return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
}
