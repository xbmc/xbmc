/*
 *      Copyright (C) 2012-2016 Team Kodi
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

#include "Addon_InputStream.h"

#include "addons/Addon.h"
#include "addons/binary/ExceptionHandling.h"
#include "addons/binary/interfaces/api2/AddonInterfaceBase.h"
#include "addons/kodi-addon-dev-kit/include/kodi/api2/.internal/AddonLib_internal.hpp"
#include "cores/VideoPlayer/DVDDemuxers/DVDDemuxUtils.h"
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

namespace InputStream
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

void CAddOnInputStream::Init(struct CB_AddOnLib *interfaces)
{
  interfaces->InputStream.get_codec_by_name      = V2::KodiAPI::InputStream::CAddOnInputStream::get_codec_by_name;
  interfaces->InputStream.free_demux_packet      = V2::KodiAPI::InputStream::CAddOnInputStream::free_demux_packet;
  interfaces->InputStream.allocate_demux_packet  = V2::KodiAPI::InputStream::CAddOnInputStream::allocate_demux_packet;
}

void CAddOnInputStream::get_codec_by_name(void* addonData, const char* strCodecName, kodi_codec* codec)
{
  try
  {
    if (!addonData || !strCodecName)
      throw ADDON::WrongValueException("CAddOnCodec - %s - invalid data (addonData='%p', strCodecName='%p')",
                                        __FUNCTION__, addonData, strCodecName);

    kodi_codec retCodec = CCodecIds::GetInstance().GetCodecByName(strCodecName);
    codec->codec_type = retCodec.codec_type;
    codec->codec_id   = retCodec.codec_id;
    return;
  }
  HANDLE_ADDON_EXCEPTION

  codec->codec_type = KODI_CODEC_TYPE_UNKNOWN;
  codec->codec_id   = 0;
}

void CAddOnInputStream::free_demux_packet(void *addonData, DemuxPacket* pPacket)
{
  CDVDDemuxUtils::FreeDemuxPacket(pPacket);
}

DemuxPacket* CAddOnInputStream::allocate_demux_packet(void *addonData, int iDataSize)
{
  return CDVDDemuxUtils::AllocateDemuxPacket(iDataSize);
}

} /* extern "C" */
} /* namespace InputStream */

} /* namespace KodiAPI */
} /* namespace V2 */
