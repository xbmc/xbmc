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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string>
#include "addons/binary/interfaces/api1/Codec/AddonCallbacksCodec.h"
#include "addons/kodi-addon-dev-kit/include/kodi/libXBMC_codec.h"

#ifdef _WIN32
#include <windows.h>
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

using namespace std;
using namespace V1::KodiAPI::Codec;

extern "C"
{

DLLEXPORT void* CODEC_register_me(void *hdl)
{
  CB_CodecLib *cb = NULL;
  if (!hdl)
    fprintf(stderr, "libXBMC_codec-ERROR: %s is called with NULL handle\n", __FUNCTION__);
  else
  {
    cb = (CB_CodecLib*)((AddonCB*)hdl)->CodecLib_RegisterMe(((AddonCB*)hdl)->addonData);
    if (!cb)
      fprintf(stderr, "libXBMC_codec-ERROR: %s can't get callback table from XBMC\n", __FUNCTION__);
  }
  return cb;
}

DLLEXPORT void CODEC_unregister_me(void *hdl, void* cb)
{
  if (hdl && cb)
    ((AddonCB*)hdl)->CodecLib_UnRegisterMe(((AddonCB*)hdl)->addonData, (CB_CodecLib*)cb);
}

DLLEXPORT xbmc_codec_t CODEC_get_codec_by_name(void *hdl, void* cb, const char* strCodecName)
{
  xbmc_codec_t retVal;
  retVal.codec_id   = XBMC_INVALID_CODEC_ID;
  retVal.codec_type = XBMC_CODEC_TYPE_UNKNOWN;

  if (cb != NULL)
    retVal = ((CB_CodecLib*)cb)->GetCodecByName(((AddonCB*)hdl)->addonData, strCodecName);

  return retVal;
}

};
