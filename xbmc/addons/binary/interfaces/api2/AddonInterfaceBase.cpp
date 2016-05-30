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

#include "AddonInterfaceBase.h"

#include "Application.h"
#include "addons/Addon.h"
#include "addons/binary/ExceptionHandling.h"
#include "addons/binary/interfaces/AddonInterfaces.h"
#include "addons/kodi-addon-dev-kit/include/kodi/api2/internal/AddonLib_internal.hpp"
#include "addons/kodi-addon-dev-kit/src/api2/version.h"
#include "utils/log.h"

using namespace ADDON;

namespace V2
{
namespace KodiAPI
{
extern "C"
{

CAddonInterfaceAddon::CAddonInterfaceAddon(CAddon* addon)
  : ADDON::IAddonInterface(addon, ADDON_API_LEVEL, ADDON_API_VERSION),
    m_callbacks(new CB_AddOnLib)
{
  m_callbacks->addon_log_msg = addon_log_msg;
  m_callbacks->free_string = free_string;
}

CAddonInterfaceAddon::~CAddonInterfaceAddon()
{
  delete m_callbacks;
}

void CAddonInterfaceAddon::addon_log_msg(void* hdl, const int addonLogLevel, const char* strMessage)
{
  try
  {
    CAddonInterfaces* addon = static_cast<CAddonInterfaces*>(static_cast<AddonCB*>(hdl)->addonData);
    if (addon == nullptr || strMessage == nullptr)
      throw ADDON::WrongValueException("CAddonCB_General - %s - invalid data (addon='%p', strMessage='%p')",
                                        __FUNCTION__, addon, strMessage);

    CAddonInterfaceAddon* interface = static_cast<CAddonInterfaceAddon*>(addon->GetInterface());
    if (interface == nullptr)
    {
      throw ADDON::WrongValueException("CAddonCB_General - %s - invalid data (interface='%p')",
                                        __FUNCTION__, interface);
    }

    int logLevel = LOGNONE;
    switch (addonLogLevel)
    {
      case ADDON_LOG_FATAL:
        logLevel = LOGFATAL;
        break;
      case ADDON_LOG_SEVERE:
        logLevel = LOGSEVERE;
        break;
      case ADDON_LOG_ERROR:
        logLevel = LOGERROR;
        break;
      case ADDON_LOG_WARNING:
        logLevel = LOGWARNING;
        break;
      case ADDON_LOG_NOTICE:
        logLevel = LOGNOTICE;
        break;
      case ADDON_LOG_INFO:
        logLevel = LOGINFO;
        break;
      case ADDON_LOG_DEBUG:
        logLevel = LOGDEBUG;
        break;
      default:
        break;
    }

    CLog::Log(logLevel, "AddOnLog: %s: %s", interface->GetAddon()->Name().c_str(), strMessage);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddonInterfaceAddon::free_string(void* hdl, char* str)
{
  try
  {
    if (!hdl || !str)
      throw ADDON::WrongValueException("CAddonCB_General - %s - invalid data (handle='%p', str='%p')", __FUNCTION__, hdl, str);

    free(str);
  }
  HANDLE_ADDON_EXCEPTION
}

} /* extern "C" */
} /* namespace KodiAPI */
} /* namespace V2 */
