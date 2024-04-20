/*
 *  Copyright (C) 2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ControllerSelect.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "dialogs/GUIDialogSelect.h"
#include "games/controllers/Controller.h"
#include "games/controllers/ControllerLayout.h"
#include "games/controllers/ControllerTypes.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/WindowIDs.h"
#include "utils/Variant.h"
#include "utils/log.h"

using namespace KODI;
using namespace GAME;

CControllerSelect::CControllerSelect() : CThread("ControllerSelect")
{
}

CControllerSelect::~CControllerSelect() = default;

void CControllerSelect::Initialize(ControllerVector controllers,
                                   ControllerPtr defaultController,
                                   bool showDisconnect,
                                   const std::function<void(ControllerPtr)>& callback)
{
  // Validate parameters
  if (!callback)
    return;

  // Stop thread and reset state
  Deinitialize();

  // Initialize state
  m_controllers = std::move(controllers);
  m_defaultController = std::move(defaultController);
  m_showDisconnect = showDisconnect;
  m_callback = callback;

  // Create thread
  Create(false);
}

void CControllerSelect::Deinitialize()
{
  // Stop thread
  StopThread(true);

  // Reset state
  m_controllers.clear();
  m_defaultController.reset();
  m_showDisconnect = true;
  m_callback = nullptr;
}

void CControllerSelect::Process()
{
  // Select first controller by default
  unsigned int initialSelected = 0;

  CGUIComponent* gui = CServiceBroker::GetGUI();
  if (gui == nullptr)
    return;

  CGUIWindowManager& windowManager = gui->GetWindowManager();

  auto pSelectDialog = windowManager.GetWindow<CGUIDialogSelect>(WINDOW_DIALOG_SELECT);
  if (pSelectDialog == nullptr)
    return;

  CLog::Log(LOGDEBUG, "Controller select: Showing dialog for {} controllers", m_controllers.size());

  CFileItemList items;
  for (const ControllerPtr& controller : m_controllers)
  {
    CFileItemPtr item(new CFileItem(controller->Layout().Label()));
    item->SetArt("icon", controller->Layout().ImagePath());
    items.Add(std::move(item));

    // Check if a specified controller should be selected by default
    if (m_defaultController && m_defaultController->ID() == controller->ID())
      initialSelected = items.Size() - 1;
  }

  if (m_showDisconnect)
  {
    // Add a button to disconnect the port
    CFileItemPtr item(new CFileItem(g_localizeStrings.Get(13298))); // "Disconnected"
    item->SetArt("icon", "DefaultAddonNone.png");
    items.Add(std::move(item));

    // Check if the disconnect button should be selected by default
    if (!m_defaultController)
      initialSelected = items.Size() - 1;
  }

  pSelectDialog->Reset();
  pSelectDialog->SetHeading(CVariant{35113}); // "Select a Controller"
  pSelectDialog->SetUseDetails(true);
  pSelectDialog->EnableButton(false, 186); // "OK""
  pSelectDialog->SetButtonFocus(false);
  for (const auto& it : items)
    pSelectDialog->Add(*it);
  pSelectDialog->SetSelected(static_cast<int>(initialSelected));
  pSelectDialog->Open();

  // If the thread was stopped, exit early
  if (m_bStop)
    return;

  if (pSelectDialog->IsConfirmed())
  {
    ControllerPtr resultController;

    const int selected = pSelectDialog->GetSelectedItem();
    if (0 <= selected && selected < static_cast<int>(m_controllers.size()))
      resultController = m_controllers.at(selected);

    // Fire a callback with the result
    m_callback(resultController);
  }
}
