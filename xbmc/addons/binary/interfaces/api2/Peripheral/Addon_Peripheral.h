#pragma once
/*
 *      Copyright (C) 2014-2016 Team Kodi
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

namespace PERIPHERALS { class CPeripheralAddon; }

namespace V2
{
namespace KodiAPI
{

struct CB_AddOnLib;

namespace Peripheral
{
extern "C"
{

  /*!
   * Callbacks for a peripheral add-on to Kodi
   */
  class CAddOnPeripheral
  {
  public:
    static void Init(struct CB_AddOnLib *interfaces);

    static void trigger_scan(void* hdl);
    static void refresh_button_maps(void* hdl, const char* deviceName, const char* controllerId);

  private:
    static PERIPHERALS::CPeripheralAddon* GetPeripheralAddon(void* addonData, const char* strFunction);
  };

} /* extern "C" */
} /* namespace InputStream */

} /* namespace KoidAPI */
} /* namespace V2 */
