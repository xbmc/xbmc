#pragma once
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

#include "AddonHelpers_local.h"
#include "include/xbmc_pvr_types.h"

namespace ADDON
{

class CAddon;

class CAddonHelpers_PVR
{
public:
  CAddonHelpers_PVR(CAddon* addon, AddonCB* cbTable);
  ~CAddonHelpers_PVR();

  static void PVRTransferEpgEntry(void *addonData, const PVRHANDLE handle, const PVR_PROGINFO *epgentry);
  static void PVRTransferChannelEntry(void *addonData, const PVRHANDLE handle, const PVR_CHANNEL *channel);
  static void PVRTransferTimerEntry(void *addonData, const PVRHANDLE handle, const PVR_TIMERINFO *timer);
  static void PVRTransferRecordingEntry(void *addonData, const PVRHANDLE handle, const PVR_RECORDINGINFO *recording);
  static void PVRAddMenuHook(void *addonData, PVR_MENUHOOK *hook);

private:
  CAddon* m_addon;
};

}; /* namespace ADDON */
