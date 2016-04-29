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

#include "Addon/Addon_Audio.h"
#include "Addon/Addon_Directory.h"
#include "Addon/Addon_File.h"
#include "Addon/Addon_General.h"
#include "Addon/Addon_Network.h"
#include "AudioEngine/Addon_AudioEngineGeneral.h"
#include "AudioEngine/Addon_AudioEngineStream.h"
#include "GUI/Addon_GUIGeneral.h"
#include "GUI/Addon_GUIControlButton.h"
#include "GUI/Addon_GUIControlEdit.h"
#include "GUI/Addon_GUIControlFadeLabel.h"
#include "GUI/Addon_GUIControlImage.h"
#include "GUI/Addon_GUIControlLabel.h"
#include "GUI/Addon_GUIControlProgress.h"
#include "GUI/Addon_GUIControlRadioButton.h"
#include "GUI/Addon_GUIControlRendering.h"
#include "GUI/Addon_GUIControlSettingsSlider.h"
#include "GUI/Addon_GUIControlSlider.h"
#include "GUI/Addon_GUIControlSpin.h"
#include "GUI/Addon_GUIControlTextBox.h"
#include "GUI/Addon_GUIDialogExtendedProgressBar.h"
#include "GUI/Addon_GUIDialogFileBrowser.h"
#include "GUI/Addon_GUIDialogKeyboard.h"
#include "GUI/Addon_GUIDialogNumeric.h"
#include "GUI/Addon_GUIDialogOK.h"
#include "GUI/Addon_GUIDialogProgress.h"
#include "GUI/Addon_GUIDialogSelect.h"
#include "GUI/Addon_GUIDialogTextViewer.h"
#include "GUI/Addon_GUIDialogYesNo.h"
#include "GUI/Addon_GUIListItem.h"
#include "GUI/Addon_GUIWindow.h"
#include "InputStream/Addon_InputStream.h"
#include "PVR/Addon_PVR.h"
#include "Peripheral/Addon_Peripheral.h"
#include "Player/Addon_InfoTagMusic.h"
#include "Player/Addon_InfoTagVideo.h"
#include "Player/Addon_PlayList.h"
#include "Player/Addon_Player.h"

#include "Application.h"
#include "addons/Addon.h"
#include "addons/binary/ExceptionHandling.h"
#include "addons/binary/interfaces/AddonInterfaces.h"
#include "addons/kodi-addon-dev-kit/include/kodi/api3/.internal/AddonLib_internal.hpp"
#include "addons/kodi-addon-dev-kit/src/api3/version.h"
#include "utils/log.h"

using namespace ADDON;

namespace V3
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
  m_callbacks->free_string   = free_string;

  AddOn::CAddOnGeneral::Init(m_callbacks);
  AddOn::CAddOnAudio::Init(m_callbacks);
  AddOn::CAddOnDirectory::Init(m_callbacks);
  AddOn::CAddOnFile::Init(m_callbacks);
  AddOn::CAddOnNetwork::Init(m_callbacks);
  AudioEngine::CAddOnAEGeneral::Init(m_callbacks);
  AudioEngine::CAddOnAEStream::Init(m_callbacks);
  GUI::CAddOnGUIGeneral::Init(m_callbacks);
  GUI::CAddOnControl_Button::Init(m_callbacks);
  GUI::CAddOnControl_Edit::Init(m_callbacks);
  GUI::CAddOnControl_FadeLabel::Init(m_callbacks);
  GUI::CAddOnControl_Image::Init(m_callbacks);
  GUI::CAddOnControl_Label::Init(m_callbacks);
  GUI::CAddOnControl_Progress::Init(m_callbacks);
  GUI::CAddOnControl_RadioButton::Init(m_callbacks);
  GUI::CAddOnControl_Rendering::Init(m_callbacks);
  GUI::CAddOnControl_SettingsSlider::Init(m_callbacks);
  GUI::CAddOnControl_Slider::Init(m_callbacks);
  GUI::CAddOnControl_Spin::Init(m_callbacks);
  GUI::CAddOnControl_TextBox::Init(m_callbacks);
  GUI::CAddOnDialog_ExtendedProgress::Init(m_callbacks);
  GUI::CAddOnDialog_FileBrowser::Init(m_callbacks);
  GUI::CAddOnDialog_Keyboard::Init(m_callbacks);
  GUI::CAddOnDialog_Numeric::Init(m_callbacks);
  GUI::CAddOnDialog_OK::Init(m_callbacks);
  GUI::CAddOnDialog_Progress::Init(m_callbacks);
  GUI::CAddOnDialog_Select::Init(m_callbacks);
  GUI::CAddOnDialog_TextViewer::Init(m_callbacks);
  GUI::CAddOnDialog_YesNo::Init(m_callbacks);
  GUI::CAddOnListItem::Init(m_callbacks);
  GUI::CAddOnWindow::Init(m_callbacks);
  InputStream::CAddOnInputStream::Init(m_callbacks);
  PVR::CAddonInterfacesPVR::Init(m_callbacks);
  Peripheral::CAddOnPeripheral::Init(m_callbacks);
  Player::CAddOnPlayList::Init(m_callbacks);
  Player::CAddOnPlayer::Init(m_callbacks);
  Player::CAddOnInfoTagMusic::Init(m_callbacks);
  Player::CAddOnInfoTagVideo::Init(m_callbacks);
}

CAddonInterfaceAddon::~CAddonInterfaceAddon()
{
  delete m_callbacks;
}

void CAddonInterfaceAddon::addon_log_msg(
        void*                     hdl,
        const int                 addonLogLevel,
        const char*               strMessage)
{
  try
  {
    CAddonInterfaces* addon = static_cast<CAddonInterfaces*>(static_cast<AddonCB*>(hdl)->addonData);
    if (addon == nullptr || strMessage == nullptr)
      throw ADDON::WrongValueException("CAddonCB_General - %s - invalid data (addon='%p', strMessage='%p')",
                                        __FUNCTION__, addon, strMessage);

    CAddonInterfaceAddon* addonHelper = static_cast<CAddonInterfaceAddon*>(addon->AddOnLib_GetHelper());
    if (addonHelper == nullptr)
    {
      throw ADDON::WrongValueException("CAddonCB_General - %s - invalid data (addonHelper='%p')",
                                        __FUNCTION__, addonHelper);
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

    CLog::Log(logLevel, "AddOnLog: %s: %s", addonHelper->GetAddon()->Name().c_str(), strMessage);
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
} /* namespace V3 */
