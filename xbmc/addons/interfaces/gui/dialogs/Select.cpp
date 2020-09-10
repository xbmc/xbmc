/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Select.h"

#include "ServiceBroker.h"
#include "addons/binary-addons/AddonDll.h"
#include "addons/kodi-dev-kit/include/kodi/gui/dialogs/Select.h"
#include "dialogs/GUIDialogSelect.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "utils/Variant.h"
#include "utils/log.h"

namespace ADDON
{

void Interface_GUIDialogSelect::Init(AddonGlobalInterface* addonInterface)
{
  addonInterface->toKodi->kodi_gui->dialogSelect = new AddonToKodiFuncTable_kodi_gui_dialogSelect();

  addonInterface->toKodi->kodi_gui->dialogSelect->open = open;
  addonInterface->toKodi->kodi_gui->dialogSelect->open_multi_select = open_multi_select;
}

void Interface_GUIDialogSelect::DeInit(AddonGlobalInterface* addonInterface)
{
  delete addonInterface->toKodi->kodi_gui->dialogSelect;
}

int Interface_GUIDialogSelect::open(KODI_HANDLE kodiBase,
                                    const char* heading,
                                    const char* entries[],
                                    unsigned int size,
                                    int selected,
                                    unsigned int autoclose)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogSelect::{} - invalid data", __func__);
    return -1;
  }

  CGUIDialogSelect* dialog =
      CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(
          WINDOW_DIALOG_SELECT);
  if (!heading || !entries || !dialog)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIDialogSelect::{} - invalid handler data (heading='{}', entries='{}', "
              "dialog='{}') on addon '{}'",
              __func__, static_cast<const void*>(heading), static_cast<const void*>(entries),
              static_cast<void*>(dialog), addon->ID());
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


bool Interface_GUIDialogSelect::open_multi_select(KODI_HANDLE kodiBase,
                                                  const char* heading,
                                                  const char* entryIDs[],
                                                  const char* entryNames[],
                                                  bool entriesSelected[],
                                                  unsigned int size,
                                                  unsigned int autoclose)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogMultiSelect::{} - invalid data", __func__);
    return false;
  }

  CGUIDialogSelect* dialog =
      CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(
          WINDOW_DIALOG_SELECT);
  if (!heading || !entryIDs || !entryNames || !entriesSelected || !dialog)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIDialogMultiSelect::{} - invalid handler data (heading='{}', "
              "entryIDs='{}', entryNames='{}', entriesSelected='{}', dialog='{}') on addon '{}'",
              __func__, static_cast<const void*>(heading), static_cast<const void*>(entryIDs),
              static_cast<const void*>(entryNames), static_cast<void*>(entriesSelected),
              static_cast<void*>(dialog), addon->ID());
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

} /* namespace ADDON */
