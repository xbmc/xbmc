#pragma once

/*
*      Copyright (C) 2017 Team Kodi
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
