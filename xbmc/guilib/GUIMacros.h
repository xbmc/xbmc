/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#define CONTROL_SELECT(controlID) \
  do \
  { \
    CGUIMessage _msg(GUI_MSG_SET_SELECTED, GetID(), controlID); \
    OnMessage(_msg); \
  } while (0)

#define CONTROL_DESELECT(controlID) \
  do \
  { \
    CGUIMessage _msg(GUI_MSG_SET_DESELECTED, GetID(), controlID); \
    OnMessage(_msg); \
  } while (0)

#define CONTROL_ENABLE(controlID) \
  do \
  { \
    CGUIMessage _msg(GUI_MSG_ENABLED, GetID(), controlID); \
    OnMessage(_msg); \
  } while (0)

#define CONTROL_DISABLE(controlID) \
  do \
  { \
    CGUIMessage _msg(GUI_MSG_DISABLED, GetID(), controlID); \
    OnMessage(_msg); \
  } while (0)

#define CONTROL_ENABLE_ON_CONDITION(controlID, bCondition) \
  do \
  { \
    CGUIMessage _msg(bCondition ? GUI_MSG_ENABLED : GUI_MSG_DISABLED, GetID(), controlID); \
    OnMessage(_msg); \
  } while (0)

#define CONTROL_SELECT_ITEM(controlID, iItem) \
  do \
  { \
    CGUIMessage _msg(GUI_MSG_ITEM_SELECT, GetID(), controlID, iItem); \
    OnMessage(_msg); \
  } while (0)

#define SET_CONTROL_LABEL(controlID, label) \
  do \
  { \
    CGUIMessage _msg(GUI_MSG_LABEL_SET, GetID(), controlID); \
    _msg.SetLabel(label); \
    OnMessage(_msg); \
  } while (0)

/*!
 * \brief Set the label of the current control
 */
#define SET_CONTROL_LABEL_THREAD_SAFE(controlID, label) \
  { \
    CGUIMessage _msg(GUI_MSG_LABEL_SET, GetID(), controlID); \
    _msg.SetLabel(label); \
    if (CServiceBroker::GetAppMessenger()->IsProcessThread()) \
      OnMessage(_msg); \
    else \
      CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(_msg, GetID()); \
  }

/*!
 * \brief Set the second label of the current control
 */
#define SET_CONTROL_LABEL2(controlID, label) \
  do \
  { \
    CGUIMessage _msg(GUI_MSG_LABEL2_SET, GetID(), controlID); \
    _msg.SetLabel(label); \
    OnMessage(_msg); \
  } while (0)

/*!
 * \brief Set a bunch of labels on the given control
 */
#define SET_CONTROL_LABELS(controlID, defaultValue, labels) \
  do \
  { \
    CGUIMessage _msg(GUI_MSG_SET_LABELS, GetID(), controlID, defaultValue); \
    _msg.SetPointer(labels); \
    OnMessage(_msg); \
  } while (0)

/*!
 * \brief Set the label of the current control
 */
#define SET_CONTROL_FILENAME(controlID, label) \
  do \
  { \
    CGUIMessage _msg(GUI_MSG_SET_FILENAME, GetID(), controlID); \
    _msg.SetLabel(label); \
    OnMessage(_msg); \
  } while (0)

#define SET_CONTROL_HIDDEN(controlID) \
  do \
  { \
    CGUIMessage _msg(GUI_MSG_HIDDEN, GetID(), controlID); \
    OnMessage(_msg); \
  } while (0)

#define SET_CONTROL_FOCUS(controlID, dwParam) \
  do \
  { \
    CGUIMessage _msg(GUI_MSG_SETFOCUS, GetID(), controlID, dwParam); \
    OnMessage(_msg); \
  } while (0)

#define SET_CONTROL_VISIBLE(controlID) \
  do \
  { \
    CGUIMessage _msg(GUI_MSG_VISIBLE, GetID(), controlID); \
    OnMessage(_msg); \
  } while (0)

#define SET_CONTROL_SELECTED(dwSenderId, controlID, bSelect) \
  do \
  { \
    CGUIMessage _msg(bSelect ? GUI_MSG_SET_SELECTED : GUI_MSG_SET_DESELECTED, dwSenderId, \
                     controlID); \
    OnMessage(_msg); \
  } while (0)

/*!
 * \brief Click message sent from controls to windows.
 */
#define SEND_CLICK_MESSAGE(id, parentID, action) \
  do \
  { \
    CGUIMessage _msg(GUI_MSG_CLICKED, id, parentID, action); \
    SendWindowMessage(_msg); \
  } while (0)
