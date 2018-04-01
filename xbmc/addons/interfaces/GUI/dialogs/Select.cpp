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

#include "Select.h"
#include "addons/kodi-addon-dev-kit/include/kodi/gui/dialogs/Select.h"

#include "addons/binary-addons/AddonDll.h"
#include "dialogs/GUIDialogSelect.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "utils/log.h"
#include "utils/Variant.h"
#include "ServiceBroker.h"

namespace ADDON
{
extern "C"
{

void Interface_GUIDialogSelect::Init(AddonGlobalInterface* addonInterface)
{
  addonInterface->toKodi->kodi_gui->dialogSelect = static_cast<AddonToKodiFuncTable_kodi_gui_dialogSelect*>(malloc(sizeof(AddonToKodiFuncTable_kodi_gui_dialogSelect)));

  addonInterface->toKodi->kodi_gui->dialogSelect->open = open;
  addonInterface->toKodi->kodi_gui->dialogSelect->open_multi_select = open_multi_select;
}

void Interface_GUIDialogSelect::DeInit(AddonGlobalInterface* addonInterface)
{
  free(addonInterface->toKodi->kodi_gui->dialogSelect);
}

int Interface_GUIDialogSelect::open(void* kodiBase, const char *heading, const char *entries[], unsigned int size, int selected, unsigned int autoclose)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogSelect::%s - invalid data", __FUNCTION__);
    return -1;
  }

  CGUIDialogSelect* dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(WINDOW_DIALOG_SELECT);
  if (!heading || !entries || !dialog)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIDialogSelect::%s - invalid handler data (heading='%p', entries='%p', "
              "dialog='%p') on addon '%s'",
              __FUNCTION__, heading, static_cast<const void*>(entries), static_cast<void*>(dialog),
              addon->ID().c_str());
    return -1;
  }

  dialog->Reset();
  dialog->SetHeading(CVariant{heading});

  for (unsigned int i = 0; i < size; ++i)
    dialog->Add(entries[i]);

  if (selected > 0)
    dialog->SetSelected(selected);
  if (autoclose > 0)
    dialog->SetAutoClose(autoclose);

  dialog->Open();
  return dialog->GetSelectedItem();
}


bool Interface_GUIDialogSelect::open_multi_select(void* kodiBase, const char *heading, const char *entryIDs[], const char *entryNames[],
                                                  bool entriesSelected[], unsigned int size, unsigned int autoclose)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogMultiSelect::%s - invalid data", __FUNCTION__);
    return false;
  }

  CGUIDialogSelect* dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(WINDOW_DIALOG_SELECT);
  if (!heading || !entryIDs || !entryNames || !entriesSelected || !dialog)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIDialogMultiSelect::%s - invalid handler data (heading='%p', "
              "entryIDs='%p', entryNames='%p', entriesSelected='%p', dialog='%p') on addon '%s'",
              __FUNCTION__, heading, static_cast<const void*>(entryIDs),
              static_cast<const void*>(entryNames), static_cast<void*>(entriesSelected),
              static_cast<void*>(dialog), addon->ID().c_str());
    return false;
  }

  dialog->Reset();
  dialog->SetMultiSelection(true);
  dialog->SetHeading(CVariant{heading});

  std::vector<int> selectedIndexes;

  for (unsigned int i = 0; i < size; ++i)
  {
    dialog->Add(entryNames[i]);
    if (entriesSelected[i])
      selectedIndexes.push_back(i);
  }

  dialog->SetSelected(selectedIndexes);
  if (autoclose > 0)
    dialog->SetAutoClose(autoclose);

  dialog->Open();
  if (dialog->IsConfirmed())
  {
    for (unsigned int i = 0; i < size; ++i)
      entriesSelected[i] = false;

    selectedIndexes = dialog->GetSelectedItems();

    for (unsigned int i = 0; i < selectedIndexes.size(); ++i)
    {
      if (selectedIndexes[i])
        entriesSelected[selectedIndexes[i]] = true;
    }
  }

  return true;
}

} /* extern "C" */
} /* namespace ADDON */
