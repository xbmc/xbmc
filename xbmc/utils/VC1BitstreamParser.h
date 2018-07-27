/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <stdint.h>

class CVC1BitstreamParser
{
public:
  CVC1BitstreamParser();
  ~CVC1BitstreamParser() = default;

  void Reset();

  inline bool IsRecoveryPoint(const uint8_t *buf, int buf_size);
  inline bool IsIFrame(const uint8_t *buf, int buf_size);

protected:
  bool vc1_parse_frame(const uint8_t *buf, const uint8_t *buf_end, bool sequenceOnly);
private:
  uint8_t m_Profile;
  uint8_t m_MaxBFrames;
  uint8_t m_SimpleSkipBits;
  uint8_t m_AdvInterlace;
};
