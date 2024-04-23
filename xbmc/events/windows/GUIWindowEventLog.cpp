/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIWindowEventLog.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "events/EventLog.h"
#include "filesystem/EventsDirectory.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/WindowIDs.h"
#include "input/actions/ActionIDs.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "view/ViewStateSettings.h"

#define CONTROL_BUTTON_CLEAR          20
#define CONTROL_BUTTON_LEVEL          21
#define CONTROL_BUTTON_LEVEL_ONLY     22

CGUIWindowEventLog::CGUIWindowEventLog()
  : CGUIMediaWindow(WINDOW_EVENT_LOG, "EventLog.xml")
{ }

CGUIWindowEventLog::~CGUIWindowEventLog() = default;

bool CGUIWindowEventLog::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_CLICKED:
  {
    int iControl = message.GetSenderId();

    // check if we should clear all items
    if (iControl == CONTROL_BUTTON_CLEAR)
    {
      auto eventLog = CServiceBroker::GetEventLog();
      if (eventLog)
        eventLog->Clear(CViewStateSettings::GetInstance().GetEventLevel(),
                        CViewStateSettings::GetInstance().ShowHigherEventLevels());

      // refresh the list
      Refresh(true);
      return true;
    }

    // check if we should change the level
    if (iControl == CONTROL_BUTTON_LEVEL)
    {
      // update the event level
      CViewStateSettings::GetInstance().CycleEventLevel();
      CServiceBroker::GetSettingsComponent()->GetSettings()->Save();

      // update the listing
      Refresh();
      return true;
    }

    // check if we should change the level
    if (iControl == CONTROL_BUTTON_LEVEL_ONLY)
    {
      // update whether to show higher event levels
      CViewStateSettings::GetInstance().ToggleShowHigherEventLevels();
      CServiceBroker::GetSettingsComponent()->GetSettings()->Save();

      // update the listing
      Refresh();
      return true;
    }

    // check if the user interacted with one of the events
    if (m_viewControl.HasControl(iControl))
    {
      // get selected item
      int itemIndex = m_viewControl.GetSelectedItem();
      if (itemIndex < 0 || itemIndex >= m_vecItems->Size())
        break;

      CFileItemPtr item = m_vecItems->Get(itemIndex);
      int actionId = message.GetParam1();

      if (actionId == ACTION_DELETE_ITEM)
        return OnDelete(item);
    }

    break;
  }

  case GUI_MSG_NOTIFY_ALL:
  {
    CFileItemPtr item = std::dynamic_pointer_cast<CFileItem>(message.GetItem());
    if (item == nullptr)
      break;

    switch (message.GetParam1())
    {
    case GUI_MSG_EVENT_ADDED:
      OnEventAdded(item);
      return true;

    case GUI_MSG_EVENT_REMOVED:
      OnEventRemoved(item);
      return true;

    default:
      break;
    }
  }

  default:
    break;
  }

  return CGUIMediaWindow::OnMessage(message);
}

bool CGUIWindowEventLog::OnSelect(int item)
{
  if (item < 0 || item >= m_vecItems->Size())
    return false;

  return OnSelect(m_vecItems->Get(item));
}

void CGUIWindowEventLog::GetContextButtons(int itemNumber, CContextButtons &buttons)
{
  if (itemNumber < 0 && itemNumber >= m_vecItems->Size())
    return;

  CFileItemPtr item = m_vecItems->Get(itemNumber);
  if (item == nullptr)
    return;

  std::string eventIdentifier = item->GetProperty(PROPERTY_EVENT_IDENTIFIER).asString();
  if (eventIdentifier.empty())
    return;

  auto eventLog = CServiceBroker::GetEventLog();
  if (!eventLog)
    return;

  EventPtr eventPtr = eventLog->Get(eventIdentifier);
  if (eventPtr == nullptr)
    return;

  buttons.Add(CONTEXT_BUTTON_DELETE, g_localizeStrings.Get(1210));
}

bool CGUIWindowEventLog::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  if (itemNumber < 0 && itemNumber >= m_vecItems->Size())
    return false;

  CFileItemPtr item = m_vecItems->Get(itemNumber);
  if (item == nullptr)
    return false;

  switch (button)
  {
  case CONTEXT_BUTTON_DELETE:
    return OnDelete(item);

  default:
    break;
  }

  return CGUIMediaWindow::OnContextButton(itemNumber, button);
}

void CGUIWindowEventLog::UpdateButtons()
{
  // only enable the "clear" button if there is something to clear
  CONTROL_ENABLE_ON_CONDITION(CONTROL_BUTTON_CLEAR, m_vecItems->GetObjectCount() > 0);

  EventLevel eventLevel = CViewStateSettings::GetInstance().GetEventLevel();
  // set the label of the "level" button
  SET_CONTROL_LABEL(CONTROL_BUTTON_LEVEL,
                    StringUtils::Format(g_localizeStrings.Get(14119),
                                        g_localizeStrings.Get(14115 + (int)eventLevel)));

  // set the label, value and enabled state of the "level only" button
  SET_CONTROL_LABEL(CONTROL_BUTTON_LEVEL_ONLY, 14120);
  SET_CONTROL_SELECTED(GetID(), CONTROL_BUTTON_LEVEL_ONLY, CViewStateSettings::GetInstance().ShowHigherEventLevels());
  CONTROL_ENABLE_ON_CONDITION(CONTROL_BUTTON_LEVEL_ONLY, eventLevel < EventLevel::Error);

  CGUIMediaWindow::UpdateButtons();
}

bool CGUIWindowEventLog::GetDirectory(const std::string &strDirectory, CFileItemList &items)
{
  bool result = CGUIMediaWindow::GetDirectory(strDirectory, items);

  EventLevel currentLevel = CViewStateSettings::GetInstance().GetEventLevel();
  bool showHigherLevels = CViewStateSettings::GetInstance().ShowHigherEventLevels();

  CFileItemList filteredItems(items.GetPath());
  for (int i = 0; i < items.Size(); i++)
  {
    CFileItemPtr item = items.Get(i);
    if (item->IsParentFolder())
    {
      filteredItems.Add(item);
      continue;
    }

    if (!item->HasProperty(PROPERTY_EVENT_LEVEL))
      continue;

    EventLevel level = CEventLog::EventLevelFromString(item->GetProperty(PROPERTY_EVENT_LEVEL).asString());
    if (level == currentLevel ||
      (level > currentLevel && showHigherLevels))
      filteredItems.Add(item);
  }

  items.ClearItems();
  items.Append(filteredItems);

  return result;
}

bool CGUIWindowEventLog::OnSelect(const CFileItemPtr& item)
{
  if (item == nullptr)
    return false;

  OnExecute(item);
  return true;
}

bool CGUIWindowEventLog::OnDelete(const CFileItemPtr& item)
{
  if (item == nullptr)
    return false;

  std::string eventIdentifier = item->GetProperty(PROPERTY_EVENT_IDENTIFIER).asString();
  if (eventIdentifier.empty())
    return false;

  auto eventLog = CServiceBroker::GetEventLog();
  if (!eventLog)
    return false;

  eventLog->Remove(eventIdentifier);
  return true;
}

bool CGUIWindowEventLog::OnExecute(const CFileItemPtr& item)
{
  if (item == nullptr)
    return false;

  std::string eventIdentifier = item->GetProperty(PROPERTY_EVENT_IDENTIFIER).asString();
  if (eventIdentifier.empty())
    return false;

  auto eventLog = CServiceBroker::GetEventLog();
  if (!eventLog)
    return false;

  const EventPtr eventPtr = eventLog->Get(eventIdentifier);
  if (eventPtr == nullptr)
    return false;

  if (!eventPtr->CanExecute())
    return true;

  return eventPtr->Execute();
}

void CGUIWindowEventLog::OnEventAdded(const CFileItemPtr& item)
{
  if (!IsActive())
    return;

  Refresh(true);
}

void CGUIWindowEventLog::OnEventRemoved(const CFileItemPtr& item)
{
  if (!IsActive())
    return;

  int selectedItemIndex = -1;
  if (item != nullptr)
  {
    selectedItemIndex = m_viewControl.GetSelectedItem();
    // only update the selected item index when the deleted item is focused
    if (m_vecItems->Get(selectedItemIndex)->GetProperty(PROPERTY_EVENT_IDENTIFIER).asString() != item->GetProperty(PROPERTY_EVENT_IDENTIFIER).asString())
      selectedItemIndex = -1;
  }

  Refresh(true);

  // update the selected item
  if (selectedItemIndex >= 0)
    m_viewControl.SetSelectedItem(selectedItemIndex);
}
