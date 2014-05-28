/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://xbmc.org
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

#ifndef ADDONS_APPLICATION_H_INCLUDED
#define ADDONS_APPLICATION_H_INCLUDED
#include "Application.h"
#endif

#ifndef ADDONS_ADDON_H_INCLUDED
#define ADDONS_ADDON_H_INCLUDED
#include "Addon.h"
#endif

#ifndef ADDONS_ADDONCALLBACKSCODEC_H_INCLUDED
#define ADDONS_ADDONCALLBACKSCODEC_H_INCLUDED
#include "AddonCallbacksCodec.h"
#endif

#ifndef ADDONS_DLLAVCODEC_H_INCLUDED
#define ADDONS_DLLAVCODEC_H_INCLUDED
#include "DllAvCodec.h"
#endif

#ifndef ADDONS_DLLAVFORMAT_H_INCLUDED
#define ADDONS_DLLAVFORMAT_H_INCLUDED
#include "DllAvFormat.h"
#endif

#ifndef ADDONS_UTILS_STRINGUTILS_H_INCLUDED
#define ADDONS_UTILS_STRINGUTILS_H_INCLUDED
#include "utils/StringUtils.h"
#endif


namespace ADDON
{
class CCodecIds
{
public:
  virtual ~CCodecIds(void) {}

  static CCodecIds& Get(void)
  {
    static CCodecIds _instance;
    return _instance;
  }

  xbmc_codec_t GetCodecByName(const char* strCodecName)
  {
    xbmc_codec_t retVal = XBMC_INVALID_CODEC;
    if (strlen(strCodecName) == 0)
      return retVal;

    std::string strUpperCodecName = strCodecName;
    StringUtils::ToUpper(strUpperCodecName);

    std::map<std::string, xbmc_codec_t>::const_iterator it = m_lookup.find(strUpperCodecName);
    if (it != m_lookup.end())
      retVal = it->second;

    return retVal;
  }

private:
  CCodecIds(void)
  {
    DllAvCodec  dllAvCodec;
    DllAvFormat dllAvFormat;

    // load ffmpeg and register formats
    if (!dllAvCodec.Load() || !dllAvFormat.Load())
    {
      CLog::Log(LOGWARNING, "failed to load ffmpeg");
      return;
    }
    dllAvFormat.av_register_all();

    // get ids and names
    AVCodec* codec = NULL;
    xbmc_codec_t tmp;
    while ((codec = dllAvCodec.av_codec_next(codec)))
    {
      if (dllAvCodec.av_codec_is_decoder(codec))
      {
        tmp.codec_type = (xbmc_codec_type_t)codec->type;
        tmp.codec_id   = codec->id;

        std::string strUpperCodecName = codec->name;
        StringUtils::ToUpper(strUpperCodecName);

        m_lookup.insert(std::make_pair(strUpperCodecName, tmp));
      }
    }

    // teletext is not returned by av_codec_next. we got our own decoder
    tmp.codec_type = XBMC_CODEC_TYPE_SUBTITLE;
    tmp.codec_id   = AV_CODEC_ID_DVB_TELETEXT;
    m_lookup.insert(std::make_pair("TELETEXT", tmp));
  }

  std::map<std::string, xbmc_codec_t> m_lookup;
};

CAddonCallbacksCodec::CAddonCallbacksCodec(CAddon* addon)
{
  m_addon     = addon;
  m_callbacks = new CB_CODECLib;

  /* write XBMC addon-on specific add-on function addresses to the callback table */
  m_callbacks->GetCodecByName   = GetCodecByName;
}

CAddonCallbacksCodec::~CAddonCallbacksCodec()
{
  /* delete the callback table */
  delete m_callbacks;
}

xbmc_codec_t CAddonCallbacksCodec::GetCodecByName(const void* addonData, const char* strCodecName)
{
  (void)addonData;
  return CCodecIds::Get().GetCodecByName(strCodecName);
}

}; /* namespace ADDON */

