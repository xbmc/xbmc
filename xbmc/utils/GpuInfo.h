/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/Temperature.h"

#include <memory>
#include <string>

/*! \brief Class to concentrate all methods related to GPU information
* \details This is used by the Info interface to obtain the current GPU temperature
*/
class CGPUInfo
{
public:
  CGPUInfo() = default;
  virtual ~CGPUInfo() = default;

  /*! \brief Getter from the specific platform GPUInfo
   \return the platform specific implementation of GPUInfo
   */
  static std::unique_ptr<CGPUInfo> GetGPUInfo();

  /*! \brief Get the temperature of the GPU
   \param[in,out] temperature - the temperature to fill with the result
   \return true if it was possible to obtain the GPU temperature, false otherwise
   */
  bool GetTemperature(CTemperature& temperature) const;

protected:
  /*! \brief Checks if the specific platform implementation supports obtaining the GPU temperature
    via the execution of a custom command line command
    \note this is false on the base class but may be overridden by the specific platform implementation.
    Custom GPU command is defined in advancedsettings.
    \return true if the implementation supports obtaining the GPU temperature from a custom command, false otherwise
  */
  virtual bool SupportsCustomTemperatureCommand() const { return false; }

  /*! \brief Checks if the specific platform implementation supports obtaining the GPU temperature
    from the platform SDK itself
    \note this is false on the base class but may be overridden by the specific platform implementation.
    \return true if the implementation supports obtaining the GPU temperature from the platform SDK, false otherwise
  */
  virtual bool SupportsPlatformTemperature() const { return false; }

  /*! \brief Get the GPU temperature from the platform SDK
    \note platform implementations must override this. For this to take effect SupportsPlatformTemperature must be true.
    \param[in,out] temperature - the temperature to fill with the result
    \return true if obtaining the GPU temperature succeeded, false otherwise
  */
  virtual bool GetGPUPlatformTemperature(CTemperature& temperature) const = 0;

  /*! \brief Get the GPU temperature from a user provided command (advanced settings)
    \note platform implementations must override this. For this to take effect SupportsCustomTemperatureCommand must be true.
    \param[in,out] temperature - the temperature to fill with the result
    \return true if obtaining the GPU temperature succeeded, false otherwise
  */
  virtual bool GetGPUTemperatureFromCommand(CTemperature& temperature,
                                            const std::string& cmd) const = 0;
};
