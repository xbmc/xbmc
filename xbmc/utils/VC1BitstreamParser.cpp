/*
*      Copyright (C) 2017 Team XBMC
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
*  along with XBMC; see the file COPYING.  If not, see
*  <http://www.gnu.org/licenses/>.
*
*/

#include "VC1BitstreamParser.h"
#include "BitstreamReader.h"

enum
{
  VC1_PROFILE_SIMPLE,
  VC1_PROFILE_MAIN,
  VC1_PROFILE_RESERVED,
  VC1_PROFILE_ADVANCED,
  VC1_PROFILE_NOPROFILE
};

enum
{
  VC1_END_OF_SEQ = 0x0A,
  VC1_SLICE = 0x0B,
  VC1_FIELD = 0x0C,
  VC1_FRAME = 0x0D,
  VC1_ENTRYPOINT = 0x0E,
  VC1_SEQUENCE = 0x0F,
  VC1_SLICE_USER = 0x1B,
  VC1_FIELD_USER = 0x1C,
  VC1_FRAME_USER = 0x1D,
  VC1_ENTRY_POINT_USER = 0x1E,
  VC1_SEQUENCE_USER = 0x1F
};

enum
{
  VC1_FRAME_PROGRESSIVE = 0x0,
  VC1_FRAME_INTERLACE = 0x10,
  VC1_FIELD_INTERLACE = 0x11
};

CVC1BitstreamParser::CVC1BitstreamParser()
{
  Reset();
}

void CVC1BitstreamParser::Reset()
{
  m_Profile = VC1_PROFILE_NOPROFILE;
}

bool CVC1BitstreamParser::IsRecoveryPoint(const uint8_t *buf, int buf_size)
{
  return vc1_parse_frame(buf, buf + buf_size, true);
};

bool CVC1BitstreamParser::IsIFrame(const uint8_t *buf, int buf_size)
{
  return vc1_parse_frame(buf, buf + buf_size, false);
};

bool CVC1BitstreamParser::vc1_parse_frame(const uint8_t *buf, const uint8_t *buf_end, bool sequence_only)
{
  uint32_t state = -1;
  for (;;)
  {
    buf = find_start_code(buf, buf_end, &state);
    if (buf >= buf_end)
      break;
    if (buf[-1] == VC1_SEQUENCE)
    {
      if (m_Profile != VC1_PROFILE_NOPROFILE)
        return false;
      CBitstreamReader br(buf, buf_end - buf);
      // Read the profile
      m_Profile = static_cast<uint8_t>(br.ReadBits(2));
      if (m_Profile == VC1_PROFILE_ADVANCED)
      {
        br.SkipBits(39);
        m_AdvInterlace = br.ReadBits(1);
      }
      else
      {
        br.SkipBits(22);

        m_SimpleSkipBits = 2;
        if (br.ReadBits(1)) //rangered
          ++m_SimpleSkipBits;

        m_MaxBFrames = br.ReadBits(3);

        br.SkipBits(2); // quantizer
        if (br.ReadBits(1)) //finterpflag
          ++m_SimpleSkipBits;
      }
      if (sequence_only)
        return true;
    }
    else if (buf[-1] == VC1_FRAME)
    {
      CBitstreamReader br(buf, buf_end - buf);

      if (sequence_only)
        return false;
      if (m_Profile == VC1_PROFILE_ADVANCED)
      {
        uint8_t fcm;
        if (m_AdvInterlace) {
          fcm = br.ReadBits(1);
          if (fcm)
            fcm = br.ReadBits(1) + 1;
        }
        else
          fcm = VC1_FRAME_PROGRESSIVE;
        if (fcm == VC1_FIELD_INTERLACE) {
          uint8_t pic = br.ReadBits(3);
          return pic == 0x00 || pic == 0x01;
        }
        else
        {
          uint8_t pic(0);
          while (pic < 4 && br.ReadBits(1))++pic;
          return pic == 2;
        }
        return false;
      }
      else if (m_Profile != VC1_PROFILE_NOPROFILE)
      {
        br.SkipBits(m_SimpleSkipBits); // quantizer
        uint8_t pic(br.ReadBits(1));
        if (m_MaxBFrames) {
          if (!pic) {
            pic = br.ReadBits(1);
            return pic != 0;
          }
          else
            return false;
        }
        else
          return pic != 0;
      }
      else
        break;
    }
  }
  return false;
}
