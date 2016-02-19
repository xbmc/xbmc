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
#include "RequestPacket.h"
#include "ResponsePacket.h"

#include <p8-platform/util/StringUtils.h>
#include <iostream>       // std::cerr

using namespace P8PLATFORM;

extern "C"
{

  KodiAPI::kodi_codec CKODIAddon_InterProcess_Addon_Codec::GetCodecByName(const std::string &strCodecName)
  {
    try
    {
      KodiAPI::kodi_codec kodiCodec;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AddOn_Codec_GetCodecByName, session);
      vrp.push(API_STRING, strCodecName.c_str());
      uint32_t retCode;
      CLockObject lock(session->m_callMutex);
      std::unique_ptr<CResponsePacket> vresp(session->ReadResult(&vrp));
      if (!vresp)
        throw API_ERR_BUFFER;
      vresp->pop(API_UINT32_T,     &retCode);
      vresp->pop(API_INT,          &kodiCodec.codec_type);
      vresp->pop(API_UNSIGNED_INT, &kodiCodec.codec_id);
      if (retCode != API_SUCCESS)
        throw retCode;
      return kodiCodec;
    }
    PROCESS_HANDLE_EXCEPTION;

    return KODI_INVALID_CODEC;
  }

  DemuxPacket* CKODIAddon_InterProcess_Addon_Codec::AllocateDemuxPacket(int iDataSize)
  {

  }

  void CKODIAddon_InterProcess_Addon_Codec::FreeDemuxPacket(DemuxPacket* pPacket)
  {

  }

}; /* extern "C" */
