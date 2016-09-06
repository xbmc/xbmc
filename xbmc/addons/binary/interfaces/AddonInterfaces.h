#pragma once
/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      Copyright (C) 2015-2016 Team KODI
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

#include "IAddonInterface.h"
#include "addons/kodi-addon-dev-kit/include/kodi/kodi_peripheral_callbacks.h"

#include <stdint.h>

namespace KODI
{
namespace MESSAGING
{
  class ThreadMessage;
}
}

typedef void* (*KODIAddOnLib_RegisterMe)(void *addonData);
typedef void (*KODIAddOnLib_UnRegisterMe)(void *addonData, void *cbTable);
typedef void* (*KODIAudioEngineLib_RegisterMe)(void *addonData);
typedef void (*KODIAudioEngineLib_UnRegisterMe)(void *addonData, void *cbTable);
typedef void* (*KODIGUILib_RegisterMe)(void *addonData);
typedef void (*KODIGUILib_UnRegisterMe)(void *addonData, void *cbTable);
typedef void* (*KODIPVRLib_RegisterMe)(void *addonData);
typedef void (*KODIPVRLib_UnRegisterMe)(void *addonData, void *cbTable);
typedef void* (*KODIADSPLib_RegisterMe)(void *addonData);
typedef void (*KODIADSPLib_UnRegisterMe)(void *addonData, void *cbTable);
typedef void* (*KODICodecLib_RegisterMe)(void *addonData);
typedef void (*KODICodecLib_UnRegisterMe)(void *addonData, void *cbTable);
typedef void* (*KODIINPUTSTREAMLib_RegisterMe)(void *addonData);
typedef void (*KODIINPUTSTREAMLib_UnRegisterMe)(void *addonData, void *cbTable);
typedef CB_PeripheralLib* (*KODIPeripheralLib_RegisterMe)(void *addonData);
typedef void (*KODIPeripheralLib_UnRegisterMe)(void *addonData, CB_PeripheralLib *cbTable);

typedef struct AddonCB
{
  const char* libBasePath;  ///< Never, never change this!!!
  void*       addonData;
  KODIAddOnLib_RegisterMe           AddOnLib_RegisterMe;
  KODIAddOnLib_UnRegisterMe         AddOnLib_UnRegisterMe;
  KODIAudioEngineLib_RegisterMe     AudioEngineLib_RegisterMe;
  KODIAudioEngineLib_UnRegisterMe   AudioEngineLib_UnRegisterMe;
  KODICodecLib_RegisterMe           CodecLib_RegisterMe;
  KODICodecLib_UnRegisterMe         CodecLib_UnRegisterMe;
  KODIGUILib_RegisterMe             GUILib_RegisterMe;
  KODIGUILib_UnRegisterMe           GUILib_UnRegisterMe;
  KODIPVRLib_RegisterMe             PVRLib_RegisterMe;
  KODIPVRLib_UnRegisterMe           PVRLib_UnRegisterMe;
  KODIADSPLib_RegisterMe            ADSPLib_RegisterMe;
  KODIADSPLib_UnRegisterMe          ADSPLib_UnRegisterMe;
  KODIINPUTSTREAMLib_RegisterMe     INPUTSTREAMLib_RegisterMe;
  KODIINPUTSTREAMLib_UnRegisterMe   INPUTSTREAMLib_UnRegisterMe;
  KODIPeripheralLib_RegisterMe      PeripheralLib_RegisterMe;
  KODIPeripheralLib_UnRegisterMe    PeripheralLib_UnRegisterMe;
} AddonCB;


namespace ADDON
{

  class CAddon;

  class CAddonInterfaces
  {
  public:
    CAddonInterfaces(CAddon* addon);
    ~CAddonInterfaces();

    AddonCB* GetCallbacks()        { return m_callbacks; }
    CAddon *GetAddon()             { return m_addon; }
    const CAddon *GetAddon() const { return m_addon; }
    /*\_________________________________________________________________________
    \*/
    static void*        AddOnLib_RegisterMe            (void* addonData);
    static void         AddOnLib_UnRegisterMe          (void* addonData, void* cbTable);
    void*               AddOnLib_GetHelper()          { return m_helperAddOn; }
    /*\_________________________________________________________________________
    \*/
    static void*        AudioEngineLib_RegisterMe      (void* addonData);
    static void         AudioEngineLib_UnRegisterMe    (void* addonData, void* cbTable);
    void*               AudioEngineLib_GetHelper()    { return m_helperAudioEngine; }
    /*\__________________________________________________________________________________________
    \*/
    static void*        GUILib_RegisterMe              (void* addonData);
    static void         GUILib_UnRegisterMe            (void* addonData, void* cbTable);
    void*               GUILib_GetHelper()            { return m_helperGUI; }
    /*\_________________________________________________________________________
    \*/
    static void*        PVRLib_RegisterMe              (void* addonData);
    static void         PVRLib_UnRegisterMe            (void* addonData, void* cbTable);
    void*               PVRLib_GetHelper()            { return m_helperPVR; }
    /*\_________________________________________________________________________
    \*/
    static void*        CodecLib_RegisterMe            (void* addonData);
    static void         CodecLib_UnRegisterMe          (void* addonData, void* cbTable);
    void*               GetHelperCODEC()              { return m_helperCODEC; }
    /*\_________________________________________________________________________
    \*/
    static void*        ADSPLib_RegisterMe             (void* addonData);
    static void         ADSPLib_UnRegisterMe           (void* addonData, void* cbTable);
    void*               GetHelperADSP()               { return m_helperADSP; }
    /*\_________________________________________________________________________
    \*/
    static void*        INPUTSTREAMLib_RegisterMe      (void *addonData);
    static void         INPUTSTREAMLib_UnRegisterMe    (void *addonData, void* cbTable);
    void*               GetHelperInputStream()        { return m_helperInputStream; }
    /*\_________________________________________________________________________
    \*/
    static CB_PeripheralLib* PeripheralLib_RegisterMe  (void *addonData);
    static void         PeripheralLib_UnRegisterMe     (void *addonData, CB_PeripheralLib* cbTable);
    void*               GetHelperPeripheral()         { return m_helperPeripheral; }
    /*
     * API level independent functions for Kodi
     */
    static void OnApplicationMessage(KODI::MESSAGING::ThreadMessage* pMsg);

  private:
    AddonCB*  m_callbacks;
    CAddon*   m_addon;

    void*     m_helperAddOn;
    void*     m_helperAudioEngine;
    void*     m_helperGUI;
    void*     m_helperPVR;
    void*     m_helperADSP;
    void*     m_helperCODEC;
    void*     m_helperInputStream;
    void*     m_helperPeripheral;
  };

} /* namespace ADDON */
