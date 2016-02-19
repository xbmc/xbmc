/*
 *      Copyright (C) 2015 Team KODI
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

#include "AddonCB_Codec.h"

#include "addons/Addon.h"
#include "addons/binary/ExceptionHandling.h"
#include "addons/binary/callbacks/api2/AddonCallbacksBase.h"
#include "cores/VideoPlayer/DVDDemuxers/DVDDemuxUtils.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "utils/StringUtils.h"

extern "C" {
#include "libavcodec/avcodec.h"
}

#include <map>
#include <utility>

using namespace ADDON;

namespace V2
{
namespace KodiAPI
{

namespace AddOn
{
extern "C"
{

class CCodecIds
{
public:
  virtual ~CCodecIds(void) {}

  static CCodecIds& GetInstance()
  {
    static CCodecIds _instance;
    return _instance;
  }

  kodi_codec GetCodecByName(const char* strCodecName)
  {
    kodi_codec retVal = KODI_INVALID_CODEC;
    if (strlen(strCodecName) == 0)
      return retVal;

    std::string strUpperCodecName = strCodecName;
    StringUtils::ToUpper(strUpperCodecName);

    std::map<std::string, kodi_codec>::const_iterator it = m_lookup.find(strUpperCodecName);
    if (it != m_lookup.end())
      retVal = it->second;

    return retVal;
  }

private:
  CCodecIds(void)
  {
    // get ids and names
    AVCodec* codec = nullptr;
    kodi_codec tmp;
    while ((codec = av_codec_next(codec)))
    {
      if (av_codec_is_decoder(codec))
      {
        tmp.codec_type = (kodi_codec_type)codec->type;
        tmp.codec_id   = codec->id;

        std::string strUpperCodecName = codec->name;
        StringUtils::ToUpper(strUpperCodecName);

        m_lookup.insert(std::make_pair(strUpperCodecName, tmp));
      }
    }

    // teletext is not returned by av_codec_next. we got our own decoder
    tmp.codec_type = KODI_CODEC_TYPE_SUBTITLE;
    tmp.codec_id   = AV_CODEC_ID_DVB_TELETEXT;
    m_lookup.insert(std::make_pair("TELETEXT", tmp));

    // rds is not returned by av_codec_next. we got our own decoder
    tmp.codec_type = KODI_CODEC_TYPE_RDS;
    tmp.codec_id   = AV_CODEC_ID_NONE;
    m_lookup.insert(std::make_pair("RDS", tmp));
  }

  std::map<std::string, kodi_codec> m_lookup;
};

CAddOnCodec::CAddOnCodec()
{

}
void CAddOnCodec::Init(V2::KodiAPI::CB_AddOnLib *callbacks)
{
  /* write Kodi specific add-on function addresses to the callback table */
  callbacks->Codec.get_codec_by_name      = get_codec_by_name;
  callbacks->Codec.allocate_demux_packet  = allocate_demux_packet;
  callbacks->Codec.free_demux_packet      = free_demux_packet;
}

kodi_codec CAddOnCodec::get_codec_by_name(void* addonData, const char* strCodecName)
{
  try
  {
    if (!addonData || !strCodecName)
      throw ADDON::WrongValueException("CAddOnCodec - %s - invalid data (addonData='%p', strCodecName='%p')",
                                        __FUNCTION__, addonData, strCodecName);

    return CCodecIds::GetInstance().GetCodecByName(strCodecName);
  }
  HANDLE_ADDON_EXCEPTION

  return KODI_INVALID_CODEC;
}

DemuxPacket* CAddOnCodec::allocate_demux_packet(void *addonData, int iDataSize)
{
  try
  {
    return CDVDDemuxUtils::AllocateDemuxPacket(iDataSize);
  }
  HANDLE_ADDON_EXCEPTION

  return nullptr;
}

void CAddOnCodec::free_demux_packet(void *addonData, DemuxPacket* pPacket)
{
  try
  {
    if (!addonData || !pPacket)
      throw ADDON::WrongValueException("CAddOnCodec - %s - invalid data (addonData='%p', pPacket='%p')",
                                        __FUNCTION__, addonData, pPacket);

    CDVDDemuxUtils::FreeDemuxPacket(pPacket);
  }
  HANDLE_ADDON_EXCEPTION
}

}; /* extern "C" */
}; /* namespace AddOn */

}; /* namespace KodiAPI */
}; /* namespace V2 */
