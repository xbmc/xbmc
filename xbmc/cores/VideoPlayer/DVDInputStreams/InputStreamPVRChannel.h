/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "InputStreamPVRBase.h"

class CInputStreamPVRChannel : public CInputStreamPVRBase
{
public:
  CInputStreamPVRChannel(IVideoPlayer* pPlayer, const CFileItem& fileitem);
  ~CInputStreamPVRChannel() override;

  CDVDInputStream::IDemux* GetIDemux() override;

protected:
  bool OpenPVRStream() override;
  void ClosePVRStream() override;
  int ReadPVRStream(uint8_t* buf, int buf_size) override;
  int64_t SeekPVRStream(int64_t offset, int whence) override;
  int64_t GetPVRStreamLength() override;
  ENextStream NextPVRStream() override;
  bool CanPausePVRStream() override;
  bool CanSeekPVRStream() override;

private:
  bool m_bDemuxActive = false;
};
