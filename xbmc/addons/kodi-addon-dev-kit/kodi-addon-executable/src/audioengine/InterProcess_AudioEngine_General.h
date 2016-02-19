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

#include "kodi/api2/.internal/AddonLib_internal.hpp"

#include <string>

extern "C"
{

  struct CKODIAddon_InterProcess_AudioEngine_General
  {
    void AddDSPMenuHook(AE_DSP_MENUHOOK* hook);
    void RemoveDSPMenuHook(AE_DSP_MENUHOOK* hook);
    void RegisterDSPMode(AE_DSP_MODES::AE_DSP_MODE* mode);
    void UnregisterDSPMode(AE_DSP_MODES::AE_DSP_MODE* mode);
    bool GetCurrentSinkFormat(AudioEngineFormat &SinkFormat);
  };

}; /* extern "C" */
