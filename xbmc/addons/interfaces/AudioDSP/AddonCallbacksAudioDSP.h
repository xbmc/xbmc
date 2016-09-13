#pragma once
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

#include "addons/interfaces/AddonInterfaces.h"
#include "addons/kodi-addon-dev-kit/include/kodi/libKODI_adsp.h"

namespace ActiveAE
{
  class CActiveAEDSPAddon;
}

namespace ADDON
{
  class CAddon;
}

namespace KodiAPI
{
namespace V1
{
namespace AudioDSP
{

/*!
 * Callbacks for a audio DSP add-on to KODI.
 *
 * Also translates the addon's C structures to KODI's C++ structures.
 */
class CAddonCallbacksADSP : public ADDON::IAddonInterface
{
public:
  CAddonCallbacksADSP(ADDON::CAddon* addon);
  virtual ~CAddonCallbacksADSP(void);

  /*!
   * @return The callback table.
   */
  CB_ADSPLib *GetCallbacks() { return m_callbacks; }

  /*!
   * @brief Add or replace a menu hook for the menu for this add-on
   * @param addonData A pointer to the add-on.
   * @param hook The hook to add.
   */
  static void ADSPAddMenuHook(void* addonData, AE_DSP_MENUHOOK* hook);

  /*!
   * @brief Remove a menu hook for the menu for this add-on
   * @param addonData A pointer to the add-on.
   * @param hook The hook to remove.
   */
  static void ADSPRemoveMenuHook(void* addonData, AE_DSP_MENUHOOK* hook);

  /*!
   * @brief Add or replace master mode information inside audio dsp database.
   * Becomes identifier written inside mode to iModeID if it was 0 (undefined)
   * @param mode The master mode to add or update inside database
   */
  static void ADSPRegisterMode(void* addonData, AE_DSP_MODES::AE_DSP_MODE* mode);

  /*!
   * @brief Remove a master mode from audio dsp database
   * @param mode The Mode to remove
   */
  static void ADSPUnregisterMode(void* addonData, AE_DSP_MODES::AE_DSP_MODE* mode);

  /*! @name KODI background sound play handling */
  //@{
  /*!
   * @brief Get a handle to open a gui sound wave playback
   * @param addonData A pointer to the add-on.
   * @param filename the related wave file to open
   * @return pointer to sound play handle
   */
  static ADSPHANDLE ADSPSoundPlay_GetHandle(void *addonData, const char *filename);

  /*!
   * @brief Release the selected handle on KODI
   * @param addonData A pointer to the add-on.
   * @param handle pointer to sound play handle
   */
  static void ADSPSoundPlay_ReleaseHandle(void *addonData, ADSPHANDLE handle);

  /*!
   * @brief Start the wave file playback
   * @param addonData A pointer to the add-on.
   * @param handle pointer to sound play handle
   */
  static void ADSPSoundPlay_Play(void *addonData, ADSPHANDLE handle);

  /*!
   * @brief Stop the wave file playback
   * @param addonData A pointer to the add-on.
   * @param handle pointer to sound play handle
   */
  static void ADSPSoundPlay_Stop(void *addonData, ADSPHANDLE handle);

  /*!
   * @brief Ask that the playback of wave is in progress
   * @param addonData A pointer to the add-on.
   * @param handle pointer to sound play handle
   * @return true if playing
   */
  static bool ADSPSoundPlay_IsPlaying(void *addonData, ADSPHANDLE handle);

  /*!
   * @brief Set the optional playback channel position, if not used it is on all channels
   * @param addonData A pointer to the add-on.
   * @param handle pointer to sound play handle
   * @param used channel pointer for the playback, AE_DSP_CH_INVALID for on all
   */
  static void ADSPSoundPlay_SetChannel(void *addonData, ADSPHANDLE handle, AE_DSP_CHANNEL channel);

  /*!
   * @brief Get the selected playback channel position
   * @param addonData A pointer to the add-on.
   * @param handle pointer to sound play handle
   * @return the used channel pointer for the playback, AE_DSP_CH_INVALID for on all
   */
  static AE_DSP_CHANNEL ADSPSoundPlay_GetChannel(void *addonData, ADSPHANDLE handle);

  /*!
   * @brief Set the volume for the wave file playback
   * @param addonData A pointer to the add-on.
   * @param handle pointer to sound play handle
   * @param volume for the file playback
   */
  static void ADSPSoundPlay_SetVolume(void *addonData, ADSPHANDLE handle, float volume);

  /*!
   * @brief Get the volume which is set for the playback
   * @param addonData A pointer to the add-on.
   * @param handle pointer to sound play handle
   * @return current volume for the file playback
   */
  static float ADSPSoundPlay_GetVolume(void *addonData, ADSPHANDLE handle);
  //@}

private:
  static ::ActiveAE::CActiveAEDSPAddon* GetAudioDSPAddon(void* addonData);

  CB_ADSPLib   *m_callbacks; /*!< callback addresses */
};

} /* namespace AudioDSP */
} /* namespace V1 */
} /* namespace KodiAPI */
