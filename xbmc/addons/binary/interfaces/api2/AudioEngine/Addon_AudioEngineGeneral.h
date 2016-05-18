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

namespace ActiveAE { class CActiveAEDSPAddon; }

struct AE_DSP_MENUHOOK;

namespace V2
{
namespace KodiAPI
{

struct CB_AddOnLib;
struct AudioEngineFormat;

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
    static void Init(struct CB_AddOnLib *callbacks);

    static void add_dsp_menu_hook(
          void*                       hdl,
          AE_DSP_MENUHOOK*            hook);

    static void remove_dsp_menu_hook(
          void*                       hdl,
          AE_DSP_MENUHOOK*            hook);

    static void register_dsp_mode(
          void*                       hdl,
          void*                       mode);

    static void unregister_dsp_mode(
          void*                       hdl,
          void*                       mode);

    static void* make_stream(
          void*                       hdl,
          AudioEngineFormat           Format,
          unsigned int                Options);

    static void free_stream(
          void*                       hdl,
          void*                       StreamHandle);

    static bool get_current_sink_format(
          void*                       hdl,
          AudioEngineFormat*          SinkFormat);

  private:
    static ActiveAE::CActiveAEDSPAddon *GetAudioDSPAddon(void *hdl);
  };

} /* extern "C" */
} /* namespace AudioEngine */

} /* namespace KodiAPI */
} /* namespace V2 */
