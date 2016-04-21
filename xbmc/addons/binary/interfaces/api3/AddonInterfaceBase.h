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

//#include "addons/kodi-addon-dev-kit/include/kodi/api3/.internal/AddonLib_internal.hpp"
#include "addons/binary/interfaces/IAddonInterface.h"

namespace ADDON { class CAddon; }

namespace V3
{
namespace KodiAPI
{
extern "C"
{

  struct CB_AddOnLib;

  class CAddonInterfaceAddon
    : public ADDON::IAddonInterface
  {
  public:
    CAddonInterfaceAddon(ADDON::CAddon* addon);
    virtual ~CAddonInterfaceAddon();

    static void addon_log_msg(
          void*                     hdl,
          const int                 addonLogLevel,
          const char*               strMessage);

    static void free_string(
          void*                     hdl,
          char*                     str);

    /*!
     * @return The callback table.
     */
    CB_AddOnLib *GetCallbacks() { return m_callbacks; }

  private:
    CB_AddOnLib  *m_callbacks; /*!< callback addresses */
  };

} /* extern "C" */
} /* namespace KodiAPI */
} /* namespace V3 */
