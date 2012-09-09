#pragma once

/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif
#include <vorbis/vorbisenc.h>
#include "utils/log.h"
#include "DynamicDll.h"

class DllVorbisEncInterface
{
public:
  virtual int vorbis_encode_init(vorbis_info *vi, long channels, long rate, long max_bitrate, long nominal_bitrate, long min_bitrate)=0;
  virtual int vorbis_encode_init_vbr(vorbis_info *vi, long channels, long rate, float base_quality)=0;
  virtual ~DllVorbisEncInterface() {}
};

class DllVorbisEnc : public DllDynamic, DllVorbisEncInterface
{
  DECLARE_DLL_WRAPPER(DllVorbisEnc, DLL_PATH_VORBIS_ENC)
  DEFINE_METHOD6(int, vorbis_encode_init, (vorbis_info *p1, long p2, long p3, long p4, long p5, long p6))
  DEFINE_METHOD4(int, vorbis_encode_init_vbr, (vorbis_info *p1, long p2, long p3, float p4))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(vorbis_encode_init)
    RESOLVE_METHOD(vorbis_encode_init_vbr)
  END_METHOD_RESOLVE()
};
