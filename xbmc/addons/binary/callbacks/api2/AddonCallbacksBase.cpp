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

#include "AddonCallbacksBase.h"

#include "Application.h"
#include "addons/Addon.h"
#include "addons/binary/ExceptionHandling.h"
#include "utils/log.h"

#include <string>

using namespace ADDON;

namespace V2
{
namespace KodiAPI
{
extern "C"
{

CAddonCallbacksAddon::CAddonCallbacksAddon(CAddon* addon)
  : ADDON::IAddonCallback(addon, KODI_API_Level, KODI_API_Version),
    m_callbacks(new CB_AddOnLib)
{
  m_callbacks->addon_log_msg = addon_log_msg;
  m_callbacks->free_string   = free_string;

  AddOn::CAddOnGeneral                  ::Init(m_callbacks);
  AddOn::CAddOnAudio                    ::Init(m_callbacks);
  AddOn::CAddOnCodec                    ::Init(m_callbacks);
  AddOn::CAddOnDirectory                ::Init(m_callbacks);
  AddOn::CAddOnFile                     ::Init(m_callbacks);
  AddOn::CAddOnNetwork                  ::Init(m_callbacks);

  AudioEngine::CAddOnAEGeneral          ::Init(m_callbacks);
  AudioEngine::CAddOnAEStream           ::Init(m_callbacks);

  GUI::CAddOnGUIGeneral                 ::Init(m_callbacks);
  GUI::CAddOnControl_Button             ::Init(m_callbacks);
  GUI::CAddOnControl_Edit               ::Init(m_callbacks);
  GUI::CAddOnControl_FadeLabel          ::Init(m_callbacks);
  GUI::CAddOnControl_Image              ::Init(m_callbacks);
  GUI::CAddOnControl_Label              ::Init(m_callbacks);
  GUI::CAddOnControl_Progress           ::Init(m_callbacks);
  GUI::CAddOnControl_RadioButton        ::Init(m_callbacks);
  GUI::CAddOnControl_Rendering          ::Init(m_callbacks);
  GUI::CAddOnControl_SettingsSlider     ::Init(m_callbacks);
  GUI::CAddOnControl_Slider             ::Init(m_callbacks);
  GUI::CAddOnControl_Spin               ::Init(m_callbacks);
  GUI::CAddOnControl_TextBox            ::Init(m_callbacks);
  GUI::CAddOnDialog_ExtendedProgress    ::Init(m_callbacks);
  GUI::CAddOnDialog_FileBrowser         ::Init(m_callbacks);
  GUI::CAddOnDialog_Keyboard            ::Init(m_callbacks);
  GUI::CAddOnDialog_Numeric             ::Init(m_callbacks);
  GUI::CAddOnDialog_OK                  ::Init(m_callbacks);
  GUI::CAddOnDialog_Progress            ::Init(m_callbacks);
  GUI::CAddOnDialog_Select              ::Init(m_callbacks);
  GUI::CAddOnDialog_TextViewer          ::Init(m_callbacks);
  GUI::CAddOnDialog_YesNo               ::Init(m_callbacks);
  GUI::CAddOnListItem                   ::Init(m_callbacks);
  GUI::CAddOnWindow                     ::Init(m_callbacks);

  PVR::CAddonCallbacksPVR               ::Init(m_callbacks);

  Player::CAddOnPlayList                ::Init(m_callbacks);
  Player::CAddOnPlayer                  ::Init(m_callbacks);
  Player::CAddOnInfoTagMusic            ::Init(m_callbacks);
  Player::CAddOnInfoTagVideo            ::Init(m_callbacks);
}

CAddonCallbacksAddon::~CAddonCallbacksAddon()
{
  delete m_callbacks;
}

void CAddonCallbacksAddon::addon_log_msg(
        void*                     hdl,
        const addon_log           addonLogLevel,
        const char*               strMessage)
{
  try
  {
    CAddonCallbacks* addon = static_cast<CAddonCallbacks*>(static_cast<AddonCB*>(hdl)->addonData);
    if (addon == nullptr || strMessage == nullptr)
      throw ADDON::WrongValueException("CAddonCB_General - %s - invalid data (addon='%p', strMessage='%p')",
                                        __FUNCTION__, addon, strMessage);

    CAddonCallbacksAddon* addonHelper = static_cast<CAddonCallbacksAddon*>(addon->AddOnLib_GetHelper());
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

void CAddonCallbacksAddon::free_string(void* hdl, char* str)
{
  try
  {
    if (!hdl || !str)
      throw ADDON::WrongValueException("CAddonCB_General - %s - invalid data (handle='%p', str='%p')", __FUNCTION__, hdl, str);

    free(str);
  }
  HANDLE_ADDON_EXCEPTION
}

}; /* extern "C" */
}; /* namespace KodiAPI */
}; /* namespace V2 */
