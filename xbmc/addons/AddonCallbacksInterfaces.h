#pragma once
/*
 *      Copyright (C) 2014 Team KODI
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

#include "AddonCallbacks.h"
#include "include/kodi_interfaces_types.h"

namespace ADDON
{

/*!
 * Callbacks for Kodi's AudioEngine.
 */
class CAddonCallbacksInterfaces
{
public:
  CAddonCallbacksInterfaces(CAddon* Addon);
  ~CAddonCallbacksInterfaces();

  /*!
   * @return The callback table.
   */
  CB_InterfacesLib *GetCallbacks() { return m_callbacks; }

  /*!
   * \brief Executes the given script asynchronously in a separate thread.
   *
   * \param script Path to the script to be executed
   * \param addon (Optional) Addon to which the script belongs
   * \param arguments (Optional) List of arguments passed to the script
   * \return -1 if an error occurred, otherwise the ID of the script
   */
  static int ExecuteScriptSync(void *AddonData, const char *AddonName, const char *Script, const char **Arguments, uint32_t TimeoutMs, bool WaitShutdown);

  /*!
   * \brief Executes the given script synchronously.
   *
   * \details The script is actually executed asynchronously but the calling
   * thread is blocked until either the script has finished or the given timeout
   * has expired. If the given timeout has expired the script's execution is
   * stopped and depending on the specified wait behaviour we wait for the
   * script's execution to finish or not.
   *
   * \param script Path to the script to be executed
   * \param addon (Optional) Addon to which the script belongs
   * \param arguments (Optional) List of arguments passed to the script
   * \param timeout (Optional) Timeout (in milliseconds) for the script's execution
   * \param waitShutdown (Optional) Whether to wait when having to forcefully stop the script's execution or not.
   * \return -1 if an error occurred, 0 if the script terminated or ETIMEDOUT if the given timeout expired
   */
  static int ExecuteScriptAsync(void *AddonData, const char *AddonName, const char *Script, const char **Arguments);

  static int GetPythonInterpreter(void *AddonData);
  static bool ActivatePythonInterpreter(void *AddonData, int InterpreterId);
  static bool DeactivatePythonInterpreter(void *AddonData, int InterpreterId);

private:
  CB_InterfacesLib    *m_callbacks; /*!< callback addresses */
  CAddon              *m_addon;     /*!< the addon */
};

}; /* namespace ADDON */
