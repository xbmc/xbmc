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

#include "InterProcess.h"
#include "kodi/api2/audioengine/General.hpp"

namespace V2
{
namespace KodiAPI
{

namespace AudioEngine
{

  namespace General
  {

    void AddDSPMenuHook(AE_DSP_MENUHOOK* hook)
    {
      return g_interProcess.AddDSPMenuHook(hook);
    }

    void RemoveDSPMenuHook(AE_DSP_MENUHOOK* hook)
    {
      return g_interProcess.RemoveDSPMenuHook(hook);
    }
    /*\_________________________________________________________________________
    \*/
    void RegisterDSPMode(AE_DSP_MODES::AE_DSP_MODE* mode)
    {
      return g_interProcess.RegisterDSPMode(mode);
    }

    void UnregisterDSPMode(AE_DSP_MODES::AE_DSP_MODE* mode)
    {
      return g_interProcess.UnregisterDSPMode(mode);
    }
    /*\_________________________________________________________________________
    \*/
    bool GetCurrentSinkFormat(AudioEngineFormat &SinkFormat)
    {
      return g_interProcess.GetCurrentSinkFormat(SinkFormat);
    }

  }; /* namespace General */

}; /* namespace AudioEngine */

}; /* namespace KodiAPI */
}; /* namespace V2 */
