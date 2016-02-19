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

#include "system.h"
#include "addons/kodi-addon-dev-kit/include/kodi/api2/.internal/AddonLib_LibFunc_Base.hpp"

#include "Addon/AddonCB_Audio.h"
#include "Addon/AddonCB_Codec.h"
#include "Addon/AddonCB_Directory.h"
#include "Addon/AddonCB_File.h"
#include "Addon/AddonCB_General.h"
#include "Addon/AddonCB_Network.h"
#include "AudioEngine/AudioEngineCB_General.h"
#include "AudioEngine/AudioEngineCB_Stream.h"
#include "GUI/AddonGUIGeneral.h"
#include "GUI/AddonGUIControlButton.h"
#include "GUI/AddonGUIControlEdit.h"
#include "GUI/AddonGUIControlFadeLabel.h"
#include "GUI/AddonGUIControlImage.h"
#include "GUI/AddonGUIControlLabel.h"
#include "GUI/AddonGUIControlProgress.h"
#include "GUI/AddonGUIControlRadioButton.h"
#include "GUI/AddonGUIControlRendering.h"
#include "GUI/AddonGUIControlSettingsSlider.h"
#include "GUI/AddonGUIControlSlider.h"
#include "GUI/AddonGUIControlSpin.h"
#include "GUI/AddonGUIControlTextBox.h"
#include "GUI/AddonGUIDialogExtendedProgressBar.h"
#include "GUI/AddonGUIDialogFileBrowser.h"
#include "GUI/AddonGUIDialogKeyboard.h"
#include "GUI/AddonGUIDialogNumeric.h"
#include "GUI/AddonGUIDialogOK.h"
#include "GUI/AddonGUIDialogProgress.h"
#include "GUI/AddonGUIDialogSelect.h"
#include "GUI/AddonGUIDialogTextViewer.h"
#include "GUI/AddonGUIDialogYesNo.h"
#include "GUI/AddonGUIListItem.h"
#include "GUI/AddonGUIWindow.h"
#include "Player/PlayerCB_AddonPlayList.h"
#include "Player/PlayerCB_AddonPlayer.h"
#include "Player/PlayerCB_InfoTagMusic.h"
#include "Player/PlayerCB_InfoTagVideo.h"
#include "PVR/AddonCallbacksPVR.h"

#include "addons/binary/callbacks/IAddonCallback.h"
#include "addons/binary/callbacks/AddonCallbacks.h"

namespace ADDON
{
  class CAddon;
};

namespace V2
{
namespace KodiAPI
{
extern "C"
{

  class CAddonCallbacksAddon
    : public ADDON::IAddonCallback
  {
  public:
    CAddonCallbacksAddon(ADDON::CAddon* addon);
    virtual ~CAddonCallbacksAddon();

    static int APILevel()         { return KODI_API_Level; }
    static std::string Version()  { return KODI_API_Version;  }

    static void addon_log_msg(
          void*                     hdl,
          const addon_log           addonLogLevel,
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

}; /* extern "C" */
}; /* namespace KodiAPI */
}; /* namespace V2 */
