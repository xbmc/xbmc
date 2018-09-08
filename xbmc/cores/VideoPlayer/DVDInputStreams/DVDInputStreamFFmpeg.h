/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DVDInputStream.h"

class CDVDInputStreamFFmpeg
  : public CDVDInputStream
{
public:
  explicit CDVDInputStreamFFmpeg(const CFileItem& fileitem);
  ~CDVDInputStreamFFmpeg() override;
  bool Open() override;
  void Close() override;
  int Read(uint8_t* buf, int buf_size) override;
  int64_t Seek(int64_t offset, int whence) override;
  bool Pause(double dTime) override { return false; };
  bool IsEOF() override;
  int64_t GetLength() override;
  std::string GetFileName() override;

  void  Abort() override { m_aborted = true;  }
  bool Aborted() { return m_aborted;  }

  const CFileItem& GetItem() const { return m_item; }

  std::string GetProxyType() const;
  std::string GetProxyHost() const;
  uint16_t GetProxyPort() const;
  std::string GetProxyUser() const;
  std::string GetProxyPassword() const;

protected:
  bool m_aborted = false;
};
