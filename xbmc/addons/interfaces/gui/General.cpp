/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "General.h"

#include "ListItem.h"
#include "ServiceBroker.h"
#include "Window.h"
#include "addons/binary-addons/AddonDll.h"
#include "addons/kodi-addon-dev-kit/include/kodi/gui/General.h"
#include "controls/Button.h"
#include "controls/Edit.h"
#include "controls/FadeLabel.h"
#include "controls/Image.h"
#include "controls/Label.h"
#include "controls/Progress.h"
#include "controls/RadioButton.h"
#include "controls/Rendering.h"
#include "controls/SettingsSlider.h"
#include "controls/Slider.h"
#include "controls/Spin.h"
#include "controls/TextBox.h"
#include "dialogs/ContextMenu.h"
#include "dialogs/ExtendedProgressBar.h"
#include "dialogs/FileBrowser.h"
#include "dialogs/Keyboard.h"
#include "dialogs/Numeric.h"
#include "dialogs/OK.h"
#include "dialogs/Progress.h"
#include "dialogs/Select.h"
#include "dialogs/TextViewer.h"
#include "dialogs/YesNo.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "utils/log.h"

using namespace kodi; // addon-dev-kit namespace
using namespace kodi::gui; // addon-dev-kit namespace

namespace ADDON
{
int Interface_GUIGeneral::m_iAddonGUILockRef = 0;
};

extern "C"
{

namespace ADDON
{

void Interface_GUIGeneral::Init(AddonGlobalInterface* addonInterface)
{
  addonInterface->toKodi->kodi_gui = static_cast<AddonToKodiFuncTable_kodi_gui*>(malloc(sizeof(AddonToKodiFuncTable_kodi_gui)));

  Interface_GUIControlButton::Init(addonInterface);
  Interface_GUIControlEdit::Init(addonInterface);
  Interface_GUIControlFadeLabel::Init(addonInterface);
  Interface_GUIControlImage::Init(addonInterface);
  Interface_GUIControlLabel::Init(addonInterface);
  Interface_GUIControlProgress::Init(addonInterface);
  Interface_GUIControlRadioButton::Init(addonInterface);
  Interface_GUIControlAddonRendering::Init(addonInterface);
  Interface_GUIControlSettingsSlider::Init(addonInterface);
  Interface_GUIControlSlider::Init(addonInterface);
  Interface_GUIControlSpin::Init(addonInterface);
  Interface_GUIControlTextBox::Init(addonInterface);
  Interface_GUIDialogContextMenu::Init(addonInterface);
  Interface_GUIDialogExtendedProgress::Init(addonInterface);
  Interface_GUIDialogFileBrowser::Init(addonInterface);
  Interface_GUIDialogKeyboard::Init(addonInterface);
  Interface_GUIDialogNumeric::Init(addonInterface);
  Interface_GUIDialogOK::Init(addonInterface);
  Interface_GUIDialogProgress::Init(addonInterface);
  Interface_GUIDialogSelect::Init(addonInterface);
  Interface_GUIDialogTextViewer::Init(addonInterface);
  Interface_GUIDialogYesNo::Init(addonInterface);
  Interface_GUIListItem::Init(addonInterface);
  Interface_GUIWindow::Init(addonInterface);

  addonInterface->toKodi->kodi_gui->general = static_cast<AddonToKodiFuncTable_kodi_gui_general*>(malloc(sizeof(AddonToKodiFuncTable_kodi_gui_general)));

  addonInterface->toKodi->kodi_gui->general->lock = lock;
  addonInterface->toKodi->kodi_gui->general->unlock = unlock;
  addonInterface->toKodi->kodi_gui->general->get_screen_height = get_screen_height;
  addonInterface->toKodi->kodi_gui->general->get_screen_width = get_screen_width;
  addonInterface->toKodi->kodi_gui->general->get_video_resolution = get_video_resolution;
  addonInterface->toKodi->kodi_gui->general->get_current_window_dialog_id = get_current_window_dialog_id;
  addonInterface->toKodi->kodi_gui->general->get_current_window_id = get_current_window_id;
  addonInterface->toKodi->kodi_gui->general->get_hw_context = get_hw_context;
}

void Interface_GUIGeneral::DeInit(AddonGlobalInterface* addonInterface)
{
  if (addonInterface->toKodi && /* <-- needed as long as the old addon way is used */
      addonInterface->toKodi->kodi_gui)
  {
    Interface_GUIControlButton::DeInit(addonInterface);
    Interface_GUIControlEdit::DeInit(addonInterface);
    Interface_GUIControlFadeLabel::DeInit(addonInterface);
    Interface_GUIControlImage::DeInit(addonInterface);
    Interface_GUIControlLabel::DeInit(addonInterface);
    Interface_GUIControlProgress::DeInit(addonInterface);
    Interface_GUIControlRadioButton::DeInit(addonInterface);
    Interface_GUIControlAddonRendering::DeInit(addonInterface);
    Interface_GUIControlSettingsSlider::DeInit(addonInterface);
    Interface_GUIControlSlider::DeInit(addonInterface);
    Interface_GUIControlSpin::DeInit(addonInterface);
    Interface_GUIControlTextBox::DeInit(addonInterface);
    Interface_GUIDialogContextMenu::DeInit(addonInterface);
    Interface_GUIDialogExtendedProgress::DeInit(addonInterface);
    Interface_GUIDialogFileBrowser::DeInit(addonInterface);
    Interface_GUIDialogKeyboard::DeInit(addonInterface);
    Interface_GUIDialogNumeric::DeInit(addonInterface);
    Interface_GUIDialogOK::DeInit(addonInterface);
    Interface_GUIDialogProgress::DeInit(addonInterface);
    Interface_GUIDialogSelect::DeInit(addonInterface);
    Interface_GUIDialogTextViewer::DeInit(addonInterface);
    Interface_GUIDialogYesNo::DeInit(addonInterface);
    Interface_GUIListItem::DeInit(addonInterface);
    Interface_GUIWindow::DeInit(addonInterface);

    free(addonInterface->toKodi->kodi_gui->general);
    free(addonInterface->toKodi->kodi_gui);
    addonInterface->toKodi->kodi_gui = nullptr;
  }
}

//@{
void Interface_GUIGeneral::lock()
{
  if (m_iAddonGUILockRef == 0)
    CServiceBroker::GetWinSystem()->GetGfxContext().lock();
  ++m_iAddonGUILockRef;
}

void Interface_GUIGeneral::unlock()
{
  if (m_iAddonGUILockRef > 0)
  {
    --m_iAddonGUILockRef;
    if (m_iAddonGUILockRef == 0)
      CServiceBroker::GetWinSystem()->GetGfxContext().unlock();
  }
}
//@}

//@{
int Interface_GUIGeneral::get_screen_height(void* kodiBase)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "kodi::gui::%s - invalid data", __FUNCTION__);
    return -1;
  }

  return CServiceBroker::GetWinSystem()->GetGfxContext().GetHeight();
}

int Interface_GUIGeneral::get_screen_width(void* kodiBase)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "kodi::gui::%s - invalid data", __FUNCTION__);
    return -1;
  }

  return CServiceBroker::GetWinSystem()->GetGfxContext().GetWidth();
}

int Interface_GUIGeneral::get_video_resolution(void* kodiBase)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "kodi::gui::%s - invalid data", __FUNCTION__);
    return -1;
  }

  return (int)CServiceBroker::GetWinSystem()->GetGfxContext().GetVideoResolution();
}
//@}

//@{
int Interface_GUIGeneral::get_current_window_dialog_id(void* kodiBase)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "kodi::gui::%s - invalid data", __FUNCTION__);
    return -1;
  }

  CSingleLock gl(CServiceBroker::GetWinSystem()->GetGfxContext());
  return CServiceBroker::GetGUI()->GetWindowManager().GetTopmostModalDialog();
}

int Interface_GUIGeneral::get_current_window_id(void* kodiBase)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "kodi::gui::%s - invalid data", __FUNCTION__);
    return -1;
  }

  CSingleLock gl(CServiceBroker::GetWinSystem()->GetGfxContext());
  return CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow();
}

void* Interface_GUIGeneral::get_hw_context(void* kodiBase)
{
  return CServiceBroker::GetWinSystem()->GetHWContext();
}

//@}

} /* namespace ADDON */
} /* extern "C" */
