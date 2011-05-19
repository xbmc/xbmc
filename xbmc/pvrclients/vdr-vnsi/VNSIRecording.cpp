/*
 *      Copyright (C) 2010 Alwin Esch (Team XBMC)
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
#include "VNSIRecording.h"
#include "responsepacket.h"
#include "requestpacket.h"
#include "vnsicommand.h"

#define SEEK_POSSIBLE 0x10 // flag used to check if protocol allows seeks

cVNSIRecording::cVNSIRecording()
{
}

cVNSIRecording::~cVNSIRecording()
{
  Close();
}

bool cVNSIRecording::OpenRecording(const PVR_RECORDING& recinfo)
{
  m_recinfo = recinfo;

  if(!cVNSISession::Open(g_szHostname, g_iPort, "XBMC RecordingStream Receiver"))
    return false;

  if(!cVNSISession::Login())
    return false;

  cRequestPacket vrp;
  if (!vrp.init(VNSI_RECSTREAM_OPEN) ||
      !vrp.add_U32(recinfo.iClientIndex))
  {
    return false;
  }

  cResponsePacket* vresp = ReadResult(&vrp);
  if (!vresp)
    return false;

  uint32_t returnCode = vresp->extract_U32();
  if (returnCode == VNSI_RET_OK)
  {
    m_currentPlayingRecordFrames    = vresp->extract_U32();
    m_currentPlayingRecordBytes     = vresp->extract_U64();
    m_currentPlayingRecordPosition  = 0;
  }
  else
    XBMC->Log(LOG_ERROR, "%s - Can't open recording '%s'", __FUNCTION__, recinfo.strTitle);

  delete vresp;
  return (returnCode == VNSI_RET_OK);
}

void cVNSIRecording::Close()
{
  if(!IsOpen())
    return;

  cRequestPacket vrp;
  vrp.init(VNSI_RECSTREAM_CLOSE);
  ReadSuccess(&vrp);
  cVNSISession::Close();
}

int cVNSIRecording::Read(unsigned char* buf, uint32_t buf_size)
{
  if (ConnectionLost() && !TryReconnect())
  {
    *buf = 0;
    SleepMs(100);
    return 1;
  }

  if (m_currentPlayingRecordPosition >= m_currentPlayingRecordBytes)
    return 0;

  cRequestPacket vrp;
  if (!vrp.init(VNSI_RECSTREAM_GETBLOCK) ||
      !vrp.add_U64(m_currentPlayingRecordPosition) ||
      !vrp.add_U32(buf_size))
  {
    return 0;
  }

  cResponsePacket* vresp = ReadResult(&vrp);
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

long long cVNSIRecording::Seek(long long pos, uint32_t whence)
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
      if (m_currentPlayingRecordBytes)
        nextPos = m_currentPlayingRecordBytes - pos;
      else
        return -1;

      break;

    case SEEK_POSSIBLE:
      return 1;

    default:
      return -1;
  }

  if (nextPos >= m_currentPlayingRecordBytes)
  {
    return 0;
  }

  m_currentPlayingRecordPosition = nextPos;

  return m_currentPlayingRecordPosition;
}

long long cVNSIRecording::Position(void)
{
  return m_currentPlayingRecordPosition;
}

long long cVNSIRecording::Length(void)
{
  return m_currentPlayingRecordBytes;
}

void cVNSIRecording::OnReconnect()
{
  OpenRecording(m_recinfo);
}
