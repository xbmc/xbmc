/*
 *      Copyright (C) 2005-2017 Team Kodi
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

#include "DialogTextViewer.h"
#include "addons/kodi-addon-dev-kit/include/kodi/gui/DialogTextViewer.h"

#include "addons/AddonDll.h"
#include "dialogs/GUIDialogTextViewer.h"
#include "guilib/GUIWindowManager.h"
#include "utils/log.h"

extern "C"
{
namespace ADDON
{

void Interface_GUIDialogTextViewer::Init(AddonGlobalInterface* addonInterface)
{
  addonInterface->toKodi->kodi_gui->dialogTextViewer = static_cast<AddonToKodiFuncTable_kodi_gui_dialogTextViewer*>(malloc(sizeof(AddonToKodiFuncTable_kodi_gui_dialogTextViewer)));

  addonInterface->toKodi->kodi_gui->dialogTextViewer->open = open;
}

void Interface_GUIDialogTextViewer::DeInit(AddonGlobalInterface* addonInterface)
{
  free(addonInterface->toKodi->kodi_gui->dialogTextViewer);
}

void Interface_GUIDialogTextViewer::open(void* kodiBase, const char *heading, const char *text)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogTextViewer::%s - invalid data", __FUNCTION__);
    return;
  }

  CGUIDialogTextViewer* dialog = g_windowManager.GetWindow<CGUIDialogTextViewer>(WINDOW_DIALOG_TEXT_VIEWER);
  if (!heading || !text || !dialog)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogTextViewer::%s - invalid handler data (heading='%p', text='%p', dialog='%p') on addon '%s'", __FUNCTION__,
                            heading, text, dialog, addon->ID().c_str());
    return;
  }

  dialog->SetHeading(heading);
  dialog->SetText(text);
  dialog->Open();
}

} /* namespace ADDON */
} /* extern "C" */
