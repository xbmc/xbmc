/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
/*
#include "Addon.h"
#include "AddonHelpers_local.h"
#include "AddonHelpers_Addon.h"
#include "AddonHelpers_GUI.h"
#include "AddonHelpers_PVR.h"
#include "AddonHelpers_Vis.h"

namespace ADDON
{

CAddonHelpers::CAddonHelpers(CAddon* addon)
{
  m_callbacks   = new AddonCB;
  m_helperAddon = new CAddonHelpers_Addon(addon, m_callbacks);
  m_helperGUI   = new CAddonHelpers_GUI(addon, m_callbacks);
  if (addon->Type() == ADDON_PVRDLL)
    m_helperPVR = new CAddonHelpers_PVR(addon, m_callbacks);
  else
    m_helperPVR = NULL;
  if (addon->Type() == ADDON_VIZ)
    m_helperVis = new CAddonHelpers_Vis(addon, m_callbacks);
  else
    m_helperVis = NULL;

  m_callbacks->addonData = this;
}

CAddonHelpers::~CAddonHelpers()
{
  delete m_helperAddon;
  m_helperAddon = NULL;
  delete m_helperGUI;
  m_helperGUI = NULL;
  delete m_helperPVR;
  m_helperPVR = NULL;
  delete m_helperVis;
  m_helperVis = NULL;
  delete m_callbacks;
  m_callbacks = NULL;
}

};*/ /* namespace ADDON */
