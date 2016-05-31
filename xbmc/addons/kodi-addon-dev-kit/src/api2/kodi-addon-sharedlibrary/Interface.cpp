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

#include "Interface.h"

#include <stdio.h>

ADDON::AddonCB* CKODIAddonInterface::m_Handle = nullptr;
CB_AddOnLib* CKODIAddonInterface::m_interface = nullptr;

int CKODIAddonInterface::InitLibAddon(void* hdl)
{
  m_Handle = static_cast<ADDON::AddonCB*>(hdl);
  if (m_Handle == nullptr)
  {
    KODI_API_lasterror = API_ERR_BUFFER;
    return KODI_API_lasterror;
  }

  m_interface = static_cast<CB_AddOnLib*>(m_Handle->interface);
  if (m_interface == nullptr)
  {
    KODI_API_lasterror = API_ERR_BUFFER;
    return KODI_API_lasterror;
  }

  KODI_API_lasterror = API_SUCCESS;
  return KODI_API_lasterror;
}

int CKODIAddonInterface::Finalize()
{
  KODI_API_lasterror = API_SUCCESS;
  return KODI_API_lasterror;
}
