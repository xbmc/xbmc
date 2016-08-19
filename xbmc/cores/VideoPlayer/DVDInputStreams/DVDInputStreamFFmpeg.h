#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include "DVDInputStream.h"

class CDVDInputStreamFFmpeg
  : public CDVDInputStream
{
public:
  CDVDInputStreamFFmpeg(const CFileItem& fileitem);
  virtual ~CDVDInputStreamFFmpeg();
  virtual bool Open() override;
  virtual void Close() override;
  virtual int Read(uint8_t* buf, int buf_size) override;
  virtual int64_t Seek(int64_t offset, int whence) override;
  virtual bool Pause(double dTime) override { return false; };
  virtual bool IsEOF() override;
  virtual int64_t GetLength() override;
  std::string GetFileName() override;

  virtual void  Abort() override { m_aborted = true;  }
  bool Aborted() { return m_aborted;  }

  const CFileItem& GetItem() const { return m_item; }

  bool CanSeek() override { return m_can_seek; }
  bool CanPause() override { return m_can_pause; }

  std::string GetProxyType() const;
  std::string GetProxyHost() const;
  uint16_t GetProxyPort() const;
  std::string GetProxyUser() const;
  std::string GetProxyPassword() const;

private:
  CURL GetM3UBestBandwidthStream(const CURL &url, size_t bandwidth);

protected:
  bool m_can_pause;
  bool m_can_seek;
  bool m_aborted;
};
