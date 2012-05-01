/*
 *      Copyright (C) 2010 Alwin Esch (Team XBMC)
 *      Copyright (C) 2011 Alexander Pipelka
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

#include <limits.h>
#include "XVDRRecording.h"
#include "XVDRResponsePacket.h"
#include "requestpacket.h"
#include "xvdrcommand.h"

#define SEEK_POSSIBLE 0x10 // flag used to check if protocol allows seeks

using namespace ADDON;

cXVDRRecording::cXVDRRecording()
{
}

cXVDRRecording::~cXVDRRecording()
{
  Close();
}

bool cXVDRRecording::OpenRecording(const std::string& hostname, const PVR_RECORDING& recinfo)
{
  m_recinfo = recinfo;

  if(!cXVDRSession::Open(hostname, "XBMC RecordingStream Receiver"))
    return false;

  if(!cXVDRSession::Login())
    return false;

  cRequestPacket vrp;
  if (!vrp.init(XVDR_RECSTREAM_OPEN) ||
      !vrp.add_String(recinfo.strRecordingId))
  {
    return false;
  }

  cXVDRResponsePacket* vresp = ReadResult(&vrp);
  if (!vresp)
    return false;

  uint32_t returnCode = vresp->extract_U32();
  if (returnCode == XVDR_RET_OK)
  {
    m_currentPlayingRecordFrames    = vresp->extract_U32();
    m_currentPlayingRecordBytes     = vresp->extract_U64();
    m_currentPlayingRecordPosition  = 0;
  }
  else
    XBMC->Log(LOG_ERROR, "%s - Can't open recording '%s'", __FUNCTION__, recinfo.strTitle);

  delete vresp;
  return (returnCode == XVDR_RET_OK);
}

void cXVDRRecording::Close()
{
  if(!IsOpen())
    return;

  cRequestPacket vrp;
  vrp.init(XVDR_RECSTREAM_CLOSE);
  ReadSuccess(&vrp);
  cXVDRSession::Close();
}

int cXVDRRecording::Read(unsigned char* buf, uint32_t buf_size)
{
  if (ConnectionLost() && !TryReconnect())
  {
    *buf = 0;
    SleepMs(100);
    return 1;
  }

  if (m_currentPlayingRecordPosition >= m_currentPlayingRecordBytes)
    return 0;

  cXVDRResponsePacket* vresp = NULL;

  cRequestPacket vrp1;
  if (vrp1.init(XVDR_RECSTREAM_UPDATE) && ((vresp = ReadResult(&vrp1)) != NULL))
  {
    uint32_t frames = vresp->extract_U32();
    uint64_t bytes  = vresp->extract_U64();

    if(frames != m_currentPlayingRecordFrames || bytes != m_currentPlayingRecordBytes)
    {
      m_currentPlayingRecordFrames = frames;
      m_currentPlayingRecordBytes  = bytes;
      XBMC->Log(LOG_DEBUG, "Size of recording changed: %lu bytes (%u frames)", bytes, frames);
    }
    delete vresp;
  }

  cRequestPacket vrp2;
  if (!vrp2.init(XVDR_RECSTREAM_GETBLOCK) ||
      !vrp2.add_U64(m_currentPlayingRecordPosition) ||
      !vrp2.add_U32(buf_size))
  {
    return 0;
  }

  vresp = ReadResult(&vrp2);
  if (!vresp)
    return -1;

  uint32_t length = vresp->getUserDataLength();
  uint8_t *data   = vresp->getUserData();
  if (length > buf_size)
  {
    XBMC->Log(LOG_ERROR, "%s: PANIC - Received more bytes as requested", __FUNCTION__);
    free(data);
    delete vresp;
    return 0;
  }

  memcpy(buf, data, length);
  m_currentPlayingRecordPosition += length;
  free(data);
  delete vresp;
  return length;
}

long long cXVDRRecording::Seek(long long pos, uint32_t whence)
{
  uint64_t nextPos = m_currentPlayingRecordPosition;

  switch (whence)
  {
    case SEEK_SET:
      nextPos = pos;
      break;

    case SEEK_CUR:
      nextPos += pos;
      break;

    case SEEK_END:
      nextPos = m_currentPlayingRecordBytes + pos;
      break;

    case SEEK_POSSIBLE:
      return 1;

    default:
      return -1;
  }

  if (nextPos > m_currentPlayingRecordBytes)
    return -1;

  m_currentPlayingRecordPosition = nextPos;

  return m_currentPlayingRecordPosition;
}

long long cXVDRRecording::Position(void)
{
  return m_currentPlayingRecordPosition;
}

long long cXVDRRecording::Length(void)
{
  return m_currentPlayingRecordBytes;
}

void cXVDRRecording::OnReconnect()
{
  OpenRecording(m_hostname, m_recinfo);
}
