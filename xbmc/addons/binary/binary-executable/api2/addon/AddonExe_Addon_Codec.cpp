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

#include "AddonExe_Addon_Codec.h"
#include "addons/Addon.h"
#include "addons/binary/callbacks/api2/Addon/AddonCB_Codec.h"

namespace V2
{
namespace KodiAPI
{

bool CAddonExeCB_Addon_Codec::GetCodecByName(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  bool retValue = API_SUCCESS;
  char* codecName;
  req.pop(API_STRING, &codecName);
  kodi_codec codec = AddOn::CAddOnCodec::get_codec_by_name(addon, codecName);
  resp.init(req.getRequestID());
  resp.push(API_UINT32_T, &retValue);
  resp.push(API_INT,  &codec.codec_type);
  resp.push(API_UNSIGNED_INT, &codec.codec_id);
  resp.finalise();
  return true;
}

bool CAddonExeCB_Addon_Codec::AllocateDemuxPacket(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{

}

bool CAddonExeCB_Addon_Codec::FreeDemuxPacket(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{

}

}; /* namespace KodiAPI */
}; /* namespace V2 */
