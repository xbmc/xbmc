#pragma once
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

#include "system.h"

#include "addons/binary/callbacks/IAddonCallback.h"
#include "addons/binary/callbacks/AddonCallbacks.h"
#include "addons/kodi-addon-dev-kit/include/kodi/api2/.internal/AddonLib_LibFunc_Base.hpp"

namespace ActiveAE
{
  class CActiveAEDSPAddon;
}

namespace V2
{
namespace KodiAPI
{

namespace AudioEngine
{
extern "C"
{

/*!
 * Callbacks for Kodi's AudioEngine.
 */
class CAddOnAEGeneral
{
public:
  CAddOnAEGeneral();

  static void Init(CB_AddOnLib *callbacks);

  static void add_dsp_menu_hook(
        void*                       hdl,
        AE_DSP_MENUHOOK*            hook);

  static void remove_dsp_menu_hook(
        void*                       hdl,
        AE_DSP_MENUHOOK*            hook);

  static void register_dsp_mode(
        void*                       hdl,
        AE_DSP_MODES::AE_DSP_MODE*  mode);

  static void unregister_dsp_mode(
        void*                       hdl,
        AE_DSP_MODES::AE_DSP_MODE*  mode);

  static AEStreamHandle* make_stream(
        void*                       hdl,
        AudioEngineFormat           Format,
        unsigned int                Options);

  static void free_stream(
        void*                       hdl,
        AEStreamHandle*             StreamHandle);

  static bool get_current_sink_format(
        void*                       hdl,
        AudioEngineFormat*          SinkFormat);

private:
  static ActiveAE::CActiveAEDSPAddon *GetAudioDSPAddon(void *hdl);
};

}; /* extern "C" */
}; /* namespace AudioEngine */

}; /* namespace KodiAPI */
}; /* namespace V2 */
