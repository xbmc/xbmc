/*
 *      Copyright (C) 2014-2016 Team KODI
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

#include "Addon_AudioEngineGeneral.h"
#include "addons/binary/interfaces/api3/AddonInterfaceBase.h"
#include "addons/binary/interfaces/api2/AudioEngine/Addon_AudioEngineGeneral.h"
#include "addons/kodi-addon-dev-kit/include/kodi/api3/.internal/AddonLib_internal.hpp"

namespace V3
{
namespace KodiAPI
{

namespace AudioEngine
{
extern "C"
{

void CAddOnAEGeneral::Init(struct CB_AddOnLib *interfaces)
{
  interfaces->AudioEngine.add_dsp_menu_hook        = V2::KodiAPI::AudioEngine::CAddOnAEGeneral::add_dsp_menu_hook;
  interfaces->AudioEngine.remove_dsp_menu_hook     = V2::KodiAPI::AudioEngine::CAddOnAEGeneral::remove_dsp_menu_hook;

  interfaces->AudioEngine.register_dsp_mode        = V2::KodiAPI::AudioEngine::CAddOnAEGeneral::register_dsp_mode;
  interfaces->AudioEngine.unregister_dsp_Mode      = V2::KodiAPI::AudioEngine::CAddOnAEGeneral::unregister_dsp_mode;

  interfaces->AudioEngine.get_current_sink_format  = (bool (*)(void*, AudioEngineFormat*))V2::KodiAPI::AudioEngine::CAddOnAEGeneral::get_current_sink_format;

  interfaces->AudioEngine.make_stream              = (void* (*)(void*, AudioEngineFormat, unsigned int))V2::KodiAPI::AudioEngine::CAddOnAEGeneral::make_stream;
  interfaces->AudioEngine.free_stream              = V2::KodiAPI::AudioEngine::CAddOnAEGeneral::free_stream;
}

} /* extern "C" */
} /* namespace AudioEngine */

} /* namespace KodiAPI */
} /* namespace V3 */
