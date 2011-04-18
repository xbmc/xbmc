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

#include "DVDAudioCodecPassthrough.h"
#include "DVDCodecs/DVDCodecs.h"
#include "DVDStreamInfo.h"
#include "settings/GUISettings.h"
#include "settings/Settings.h"
#include "utils/log.h"

#include "cores/AudioEngine/AEFactory.h"

CDVDAudioCodecPassthrough::CDVDAudioCodecPassthrough(void) :
  m_buffer    (NULL),
  m_bufferSize(0),
  m_trueHDPos (0),
  m_trueHD    (NULL),
  m_dataSize  (0)
{
}

CDVDAudioCodecPassthrough::~CDVDAudioCodecPassthrough(void)
{
  Dispose();
}

bool CDVDAudioCodecPassthrough::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  /* dont open if AE doesnt support RAW */
  if (!AE.SupportsRaw())
    return false;

  bool bSupportsAC3Out    = false;
  bool bSupportsDTSOut    = false;
  bool bSupportsTrueHDOut = false;
  int audioMode = g_guiSettings.GetInt("audiooutput.mode");
  if (AUDIO_IS_BITSTREAM(audioMode))
  {
    bSupportsAC3Out = g_guiSettings.GetBool("audiooutput.ac3passthrough");
    bSupportsDTSOut = g_guiSettings.GetBool("audiooutput.dtspassthrough");
  }

  if (audioMode == AUDIO_HDMI)
  {
    bSupportsTrueHDOut = g_guiSettings.GetBool("audiooutput.truehdpassthrough");
  }

  m_bufferSize = 0;

  if (
    (hints.codec == CODEC_ID_AC3  && bSupportsAC3Out) ||
    (hints.codec == CODEC_ID_DTS  && bSupportsDTSOut) ||
    (
      audioMode == AUDIO_HDMI &&
      (
        (hints.codec == CODEC_ID_EAC3   && bSupportsAC3Out   ) ||
        (hints.codec == CODEC_ID_TRUEHD && bSupportsTrueHDOut)
      )
    )
  )
    return true;

  return false;
}

int CDVDAudioCodecPassthrough::GetSampleRate()
{
  int rate = m_info.GetSampleRate();

  /* TrueHD uses a base sample rate of 96 or 88.2 kHz across 8 channels (96 kHz * 8 = 768, 88.2 kHz * 8 = 705.6 kHz) */
  if(m_info.GetDataType() == CAEStreamInfo::STREAM_TYPE_TRUEHD)
  {
    if (rate == 48000 || rate == 96000 || rate == 192000)
      return 96000;
    else
      return 88200;
  }

  return m_info.GetSampleRate();
}

enum AEDataFormat CDVDAudioCodecPassthrough::GetDataFormat()
{
  /* TrueHD needs 8 channel RAW */
  if(m_info.GetDataType() == CAEStreamInfo::STREAM_TYPE_TRUEHD)
    return AE_FMT_RAW8;

  return AE_FMT_RAW;
}

void CDVDAudioCodecPassthrough::Dispose()
{
  if (m_buffer)
  {
    delete[] m_buffer;
    m_buffer = NULL;
  }

  if (m_trueHD)
  {
    delete[] m_trueHD;
    m_trueHD = NULL;
  }

  m_bufferSize = 0;
  m_trueHDPos  = 0;
}

#define TRUEHD_FRAME_OFFSET     2560
#define MAT_MIDDLE_CODE_OFFSET -4
#define MAT_FRAME_SIZE          61424

int CDVDAudioCodecPassthrough::Decode(BYTE* pData, int iSize)
{
  if (iSize <= 0) return 0;

  unsigned int size = m_bufferSize;
  unsigned int used = m_info.AddData(pData, iSize, &m_buffer, &size);
  m_bufferSize = std::max(m_bufferSize, size);

  /* if we have a frame */
  if (size)
  {
    /* FIXME: this should be moved into a MAT packer class */
    /* we need to pack 24 TrueHD audio units into the unknown MAT format before packing into IEC958 */
    if (m_info.GetDataType() == CAEStreamInfo::STREAM_TYPE_TRUEHD)
    {
      /* magic MAT format values, meaning is unknown at this point */
      const uint8_t mat_start_code [20] = { 0x07, 0x9E, 0x00, 0x03, 0x84, 0x01, 0x01, 0x01, 0x80, 0x00, 0x56, 0xA5, 0x3B, 0xF4, 0x81, 0x83, 0x49, 0x80, 0x77, 0xE0 };
      const uint8_t mat_middle_code[12] = { 0xC3, 0xC1, 0x42, 0x49, 0x3B, 0xFA, 0x82, 0x83, 0x49, 0x80, 0x77, 0xE0 };
      const uint8_t mat_end_code   [16] = { 0xC3, 0xC2, 0xC0, 0xC4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x97, 0x11 };

      /* if the buffer has not been setup yet */
      if (!m_trueHD)
      {
        /* create the buffer and copy in the MAT codes */
        m_trueHD = new uint8_t[MAT_FRAME_SIZE];
        memcpy(m_trueHD, mat_start_code, sizeof(mat_start_code));
        memcpy(m_trueHD + (12 * TRUEHD_FRAME_OFFSET) + MAT_MIDDLE_CODE_OFFSET, mat_middle_code, sizeof(mat_middle_code));
        memcpy(m_trueHD + MAT_FRAME_SIZE - sizeof(mat_end_code), mat_end_code, sizeof(mat_end_code));
      }

           if (m_trueHDPos ==  0) memcpy(m_trueHD + (m_trueHDPos * TRUEHD_FRAME_OFFSET) + sizeof(mat_start_code), m_buffer, size);
      else if (m_trueHDPos == 12) memcpy(m_trueHD + (m_trueHDPos * TRUEHD_FRAME_OFFSET) + sizeof(mat_middle_code) + MAT_MIDDLE_CODE_OFFSET, m_buffer, size);
      else                        memcpy(m_trueHD + (m_trueHDPos * TRUEHD_FRAME_OFFSET), m_buffer, size);

      /* if we have a full frame */
      if (++m_trueHDPos == 24)
      {
        m_trueHDPos = 0;
        m_dataSize = CAEPackIEC958::PackTrueHD(m_trueHD, MAT_FRAME_SIZE, m_packedBuffer);
      }
    }
    else
    {
      /* pack the data into an IEC958 frame */
      CAEPackIEC958::PackFunc pack = m_info.GetPackFunc();
      if (pack)
        m_dataSize = pack(m_buffer, size, m_packedBuffer);
    }
  }

  return used;
}

int CDVDAudioCodecPassthrough::GetData(BYTE** dst)
{
  int size     = m_dataSize;
  m_dataSize   = 0;
  *dst         = m_packedBuffer;
  return size;
}

void CDVDAudioCodecPassthrough::Reset()
{
  m_dataSize = 0;
}

