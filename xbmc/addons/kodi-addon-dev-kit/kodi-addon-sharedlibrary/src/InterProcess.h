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

#include "addon/InterProcess_Addon_General.h"
#include "addon/InterProcess_Addon_Codec.h"
#include "addon/InterProcess_Addon_Network.h"
#include "addon/InterProcess_Addon_SoundPlay.h"
#include "addon/InterProcess_Addon_VFSUtils.h"
#include "audioengine/InterProcess_AudioEngine_General.h"
#include "audioengine/InterProcess_AudioEngine_Stream.h"
#include "gui/InterProcess_GUI_General.h"
#include "gui/InterProcess_GUI_ControlButton.h"
#include "gui/InterProcess_GUI_ControlEdit.h"
#include "gui/InterProcess_GUI_ControlFadeLabel.h"
#include "gui/InterProcess_GUI_ControlImage.h"
#include "gui/InterProcess_GUI_ControlLabel.h"
#include "gui/InterProcess_GUI_ControlProgress.h"
#include "gui/InterProcess_GUI_ControlRadioButton.h"
#include "gui/InterProcess_GUI_ControlRendering.h"
#include "gui/InterProcess_GUI_ControlSettingsSlider.h"
#include "gui/InterProcess_GUI_ControlSlider.h"
#include "gui/InterProcess_GUI_ControlSpin.h"
#include "gui/InterProcess_GUI_ControlTextBox.h"
#include "gui/InterProcess_GUI_DialogExtendedProgress.h"
#include "gui/InterProcess_GUI_DialogFileBrowser.h"
#include "gui/InterProcess_GUI_DialogKeyboard.h"
#include "gui/InterProcess_GUI_DialogNumeric.h"
#include "gui/InterProcess_GUI_DialogOK.h"
#include "gui/InterProcess_GUI_DialogProgress.h"
#include "gui/InterProcess_GUI_DialogSelect.h"
#include "gui/InterProcess_GUI_DialogTextViewer.h"
#include "gui/InterProcess_GUI_DialogYesNo.h"
#include "gui/InterProcess_GUI_ListItem.h"
#include "gui/InterProcess_GUI_Window.h"
#include "pvr/InterProcess_PVR_General.h"
#include "pvr/InterProcess_PVR_Transfer.h"
#include "pvr/InterProcess_PVR_Trigger.h"
#include "player/InterProcess_Player_InfoTagMusic.h"
#include "player/InterProcess_Player_InfoTagVideo.h"
#include "player/InterProcess_Player_Player.h"
#include "player/InterProcess_Player_PlayList.h"

#include "kodi/api2/AddonLib.hpp"
#include "kodi/api2/.internal/AddonLib_LibFunc_Base.hpp"

#include <string>

#ifdef TARGET_WINDOWS
#include <windows.h>
#else
#ifndef __cdecl
#define __cdecl
#endif
#ifndef __declspec
#define __declspec(X)
#endif
#endif

extern "C"
{

  #ifdef BUILD_KODI_ADDON
  namespace ADDON
  {
    typedef struct AddonCB
    {
      const char* libBasePath;  ///< Never, never change this!!!
      void*       addonData;
    } AddonCB;
  }
  #endif

  class CKODIAddon_InterProcess
   : public CKODIAddon_InterProcess_Addon_General,
     public CKODIAddon_InterProcess_Addon_Codec,
     public CKODIAddon_InterProcess_Addon_Network,
     public CKODIAddon_InterProcess_Addon_SoundPlay,
     public CKODIAddon_InterProcess_Addon_VFSUtils,
     public CKODIAddon_InterProcess_AudioEngine_General,
     public CKODIAddon_InterProcess_AudioEngine_Stream,
     public CKODIAddon_InterProcess_GUI_General,
     public CKODIAddon_InterProcess_GUI_ControlButton,
     public CKODIAddon_InterProcess_GUI_ControlEdit,
     public CKODIAddon_InterProcess_GUI_ControlFadeLabel,
     public CKODIAddon_InterProcess_GUI_ControlImage,
     public CKODIAddon_InterProcess_GUI_ControlLabel,
     public CKODIAddon_InterProcess_GUI_ControlProgress,
     public CKODIAddon_InterProcess_GUI_ControlRadioButton,
     public CKODIAddon_InterProcess_GUI_ControlRendering,
     public CKODIAddon_InterProcess_GUI_ControlSettingsSlider,
     public CKODIAddon_InterProcess_GUI_ControlSlider,
     public CKODIAddon_InterProcess_GUI_ControlSpin,
     public CKODIAddon_InterProcess_GUI_ControlTextBox,
     public CKODIAddon_InterProcess_GUI_DialogExtendedProgress,
     public CKODIAddon_InterProcess_GUI_DialogFileBrowser,
     public CKODIAddon_InterProcess_GUI_DialogKeyboard,
     public CKODIAddon_InterProcess_GUI_DialogNumeric,
     public CKODIAddon_InterProcess_GUI_DialogOK,
     public CKODIAddon_InterProcess_GUI_DialogProgress,
     public CKODIAddon_InterProcess_GUI_DialogSelect,
     public CKODIAddon_InterProcess_GUI_DialogTextViewer,
     public CKODIAddon_InterProcess_GUI_DialogYesNo,
     public CKODIAddon_InterProcess_GUI_ListItem,
     public CKODIAddon_InterProcess_GUI_Window,
     public CKODIAddon_InterProcess_PVR_General,
     public CKODIAddon_InterProcess_PVR_Transfer,
     public CKODIAddon_InterProcess_PVR_Trigger,
     public CKODIAddon_InterProcess_Player_InfoTagMusic,
     public CKODIAddon_InterProcess_Player_InfoTagVideo,
     public CKODIAddon_InterProcess_Player_Player,
     public CKODIAddon_InterProcess_Player_PlayList
  {
  public:
    CKODIAddon_InterProcess();
    virtual ~CKODIAddon_InterProcess();

    int Init(int argc, char *argv[], addon_properties* props, const std::string &hostname);
    int InitThread();
    int InitLibAddon(void* hdl);
    int Finalize();
    void Log(const addon_log loglevel, const char* string);

    ADDON::AddonCB* m_Handle;
    CB_AddOnLib*    m_Callbacks;

  protected:
    _register_level*  KODI_register;
    _unregister_me*   KODI_unregister;

  private:
    void* m_libKODI_addon;
    struct cb_array
    {
      const char* libPath;
    };
  };

  extern CKODIAddon_InterProcess g_interProcess;

}; /* extern "C" */
