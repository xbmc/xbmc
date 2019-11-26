/*
 *  Copyright (C) 2018 Arthur Liberman
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DVDInputStreamFFmpeg.h"

namespace PVR
{
  class CPVRClient;
}

class CDVDInputStreamFFmpegArchive
  : public CDVDInputStreamFFmpeg
  , public CDVDInputStream::ITimes
{
public:
  explicit CDVDInputStreamFFmpegArchive(const CFileItem& fileitem);

  bool IsStreamType(DVDStreamType type) const override;

  int64_t GetLength() override;
  int64_t Seek(int64_t offset, int whence) override;
  CURL GetURL() override;
  CDVDInputStream::ITimes* GetITimes() override { return this; }
  bool GetTimes(Times &times) override;

protected:
  std::shared_ptr<PVR::CPVRClient> m_client;
};
