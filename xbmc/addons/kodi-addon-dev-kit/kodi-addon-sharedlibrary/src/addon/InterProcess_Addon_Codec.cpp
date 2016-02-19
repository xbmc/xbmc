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

#include "InterProcess_Addon_Codec.h"
#include "InterProcess.h"

extern "C"
{

  KodiAPI::kodi_codec CKODIAddon_InterProcess_Addon_Codec::GetCodecByName(const std::string &strCodecName)
  {
    return g_interProcess.m_Callbacks->Codec.get_codec_by_name(g_interProcess.m_Handle, strCodecName.c_str());
  }

  DemuxPacket* CKODIAddon_InterProcess_Addon_Codec::AllocateDemuxPacket(int iDataSize)
  {
    return g_interProcess.m_Callbacks->Codec.allocate_demux_packet(g_interProcess.m_Handle, iDataSize);
  }

  void CKODIAddon_InterProcess_Addon_Codec::FreeDemuxPacket(DemuxPacket* pPacket)
  {
    return g_interProcess.m_Callbacks->Codec.free_demux_packet(g_interProcess.m_Handle, pPacket);
  }

}; /* extern "C" */
