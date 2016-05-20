/*
 *      Copyright (C) 2016 Team KODI
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "InterProcess.h"
#include KITINCLUDE(ADDON_API_LEVEL, inputstream/InputStream.hpp)

#include <string>
#include <stdarg.h>

API_NAMESPACE

namespace KodiAPI
{
namespace InputStream
{

  kodi_codec GetCodecByName(const std::string &strCodecName)
  {
    kodi_codec codec;
    g_interProcess.m_Callbacks->InputStream.get_codec_by_name(g_interProcess.m_Handle, strCodecName.c_str(), &codec);
    return codec;
  }

  DemuxPacket* AllocateDemuxPacket(int iDataSize)
  {
    return g_interProcess.m_Callbacks->InputStream.allocate_demux_packet(g_interProcess.m_Handle, iDataSize);
  }

  void FreeDemuxPacket(DemuxPacket* pPacket)
  {
    g_interProcess.m_Callbacks->InputStream.free_demux_packet(g_interProcess.m_Handle, pPacket);
  }

} /* namespace InputStream */
} /* namespace KodiAPI */

END_NAMESPACE()
