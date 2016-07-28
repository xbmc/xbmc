/*
 *      Copyright (C) 2015 Team Kodi
 *      http://kodi.tv
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "InputStreamAddon.h"
#include "addons/InputStream.h"
#include "cores/VideoPlayer/DVDClock.h"

CInputStreamAddon::CInputStreamAddon(const CFileItem& fileitem, std::shared_ptr<ADDON::CInputStream> inputStream)
: CDVDInputStream(DVDSTREAM_TYPE_ADDON, fileitem), m_addon(inputStream)
{
  m_hasDemux = false;
}

CInputStreamAddon::~CInputStreamAddon()
{
  Close();
  m_addon->Stop();
  m_addon.reset();
}

bool CInputStreamAddon::Open()
{
  bool ret = false;
  if (m_addon)
    ret = m_addon->Open(m_item);
  if (ret)
  {
    m_hasDemux = m_addon->HasDemux();
    m_hasDisplayTime = m_addon->HasDisplayTime();
    m_hasPosTime = m_addon->HasPosTime();
    m_canPause = m_addon->CanPause();
    m_canSeek = m_addon->CanSeek();
  }
  return ret;
}

void CInputStreamAddon::Close()
{
  if (m_addon)
    return m_addon->Close();
}

bool CInputStreamAddon::IsEOF()
{
  return false;
}

int CInputStreamAddon::Read(uint8_t* buf, int buf_size)
{
  if (!m_addon)
    return -1;

  return m_addon->ReadStream(buf, buf_size);
}

int64_t CInputStreamAddon::Seek(int64_t offset, int whence)
{
  if (!m_addon)
    return -1;

  return m_addon->SeekStream(offset, whence);
}

int64_t CInputStreamAddon::GetLength()
{
  if (!m_addon)
    return -1;

  return m_addon->LengthStream();
}

bool CInputStreamAddon::Pause(double dTime)
{
  if (!m_addon)
    return false;

  m_addon->PauseStream(dTime);
  return true;
}

bool CInputStreamAddon::CanSeek()
{
  return m_canSeek;
}

bool CInputStreamAddon::CanPause()
{
  return m_canPause;
}

// IDisplayTime
CDVDInputStream::IDisplayTime* CInputStreamAddon::GetIDisplayTime()
{
  if (!m_addon)
    return nullptr;
  if (!m_hasDisplayTime)
    return nullptr;

  return this;
}

int CInputStreamAddon::GetTotalTime()
{
  if (!m_addon)
    return 0;

  return m_addon->GetTotalTime();
}

int CInputStreamAddon::GetTime()
{
  if (!m_addon)
    return 0;

  return m_addon->GetTime();
}

// IPosTime
CDVDInputStream::IPosTime* CInputStreamAddon::GetIPosTime()
{
  if (!m_addon)
    return nullptr;
  if (!m_hasPosTime)
    return nullptr;

  return this;
}

bool CInputStreamAddon::PosTime(int ms)
{
  if (!m_addon)
    return false;

  return m_addon->PosTime(ms);
}

// IDemux
CDVDInputStream::IDemux* CInputStreamAddon::GetIDemux()
{
  if (!m_addon)
    return nullptr;
  if (!m_hasDemux)
    return nullptr;

  return this;
}

bool CInputStreamAddon::OpenDemux()
{
  if (m_hasDemux)
    return true;
  else
    return false;
}

DemuxPacket* CInputStreamAddon::ReadDemux()
{
  if (!m_addon)
    return nullptr;

  return m_addon->ReadDemux();
}

std::vector<CDemuxStream*> CInputStreamAddon::GetStreams() const
{
  std::vector<CDemuxStream*> streams;

  if (!m_addon)
    return streams;

  return m_addon->GetStreams();
}

CDemuxStream* CInputStreamAddon::GetStream(int iStreamId) const
{
  if (!m_addon)
    return nullptr;

  return m_addon->GetStream(iStreamId);
}

void CInputStreamAddon::EnableStream(int iStreamId, bool enable)
{
  if (!m_addon)
    return;

  return m_addon->EnableStream(iStreamId, enable);
}

int CInputStreamAddon::GetNrOfStreams() const
{
  if (!m_addon)
    return 0;

  int count = m_addon->GetNrOfStreams();
  return count;
}

void CInputStreamAddon::SetSpeed(int iSpeed)
{
  if (!m_addon)
    return;

  m_addon->SetSpeed(iSpeed);
}

bool CInputStreamAddon::SeekTime(int time, bool backward, double* startpts)
{
  if (!m_addon)
    return false;

  if (m_hasPosTime)
  {
    if (!PosTime(time))
      return false;

    FlushDemux();

    if(startpts)
      *startpts = DVD_NOPTS_VALUE;
    return true;
  }

  return m_addon->SeekTime(time, backward, startpts);
}

void CInputStreamAddon::AbortDemux()
{
  if (!m_addon)
    return;

  m_addon->AbortDemux();
}

void CInputStreamAddon::FlushDemux()
{
  if (!m_addon)
    return;

  m_addon->FlushDemux();
}

void CInputStreamAddon::SetVideoResolution(int width, int height)
{
  if (!m_addon)
    return;

  m_addon->SetVideoResolution(width, height);
}
