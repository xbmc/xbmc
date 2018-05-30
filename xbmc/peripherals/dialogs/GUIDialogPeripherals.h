/*
 *      Copyright (C) 2017 Team Kodi
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
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
  CCriticalSection m_peripheralsMutex;
};
}
