/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "dialogs/GUIDialogSelect.h"
#include "threads/CriticalSection.h"
#include "utils/Observer.h"
#include "FileItem.h"

namespace PERIPHERALS
{
class CPeripherals;

class CGUIDialogPeripherals : public CGUIDialogSelect,
                              protected Observer
{
public:
  CGUIDialogPeripherals();
  ~CGUIDialogPeripherals() override;

  void RegisterPeripheralManager(CPeripherals &manager);
  void UnregisterPeripheralManager();

  CFileItemPtr GetItem(unsigned int pos) const;

  static void Show(CPeripherals &manager);

  // implementation of CGUIControl via CGUIDialogSelect
  bool OnMessage(CGUIMessage& message) override;

  // implementation of Observer
  void Notify(const Observable &obs, const ObservableMessage msg) override;

private:
  // implementation of CGUIWindow via CGUIDialogSelect
  void OnInitWindow() override;

  void ShowInternal();
  void UpdatePeripheralsAsync();
  void UpdatePeripheralsSync();

  CPeripherals *m_manager = nullptr;
  CFileItemList m_peripherals;
  mutable CCriticalSection m_peripheralsMutex;
};
}
