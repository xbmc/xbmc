/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIControlBuiltins.h"

#include "ServiceBroker.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "input/WindowTranslator.h"
#include "utils/StringUtils.h"

/*! \brief Send a move event to a GUI control.
 *  \param params The parameters.
 *  \details params[0] = ID of control.
 *           params[1] = Offset of move.
 */
static int ControlMove(const std::vector<std::string>& params)
{
  CGUIMessage message(GUI_MSG_MOVE_OFFSET, CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindowOrDialog(),
                      atoi(params[0].c_str()), atoi(params[1].c_str()));
  CServiceBroker::GetGUI()->GetWindowManager().SendMessage(message);

  return 0;
}

/*! \brief Send a click event to a GUI control.
 *  \param params The parameters.
 *  \details params[0] = ID of control.
 *           params[1] = ID for window with control (optional).
 */
static int SendClick(const std::vector<std::string>& params)
{
  if (params.size() == 2)
  {
    // have a window - convert it
    int windowID = CWindowTranslator::TranslateWindow(params[0]);
    CGUIMessage message(GUI_MSG_CLICKED, atoi(params[1].c_str()), windowID);
    CServiceBroker::GetGUI()->GetWindowManager().SendMessage(message);
  }
  else
  { // single param - assume you meant the focused window
    CGUIMessage message(GUI_MSG_CLICKED, atoi(params[0].c_str()), CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindowOrDialog());
    CServiceBroker::GetGUI()->GetWindowManager().SendMessage(message);
  }

  return 0;
}

/*! \brief Send a message to a control.
 *  \param params The parameters.
 *  \details params[0] = ID of control.
 *           params[1] = Action name.
 *           \params[2] = ID of window with control (optional).
 */
static int SendMessage(const std::vector<std::string>& params)
{
  int controlID = atoi(params[0].c_str());
  int windowID = (params.size() == 3) ? CWindowTranslator::TranslateWindow(params[2]) : CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow();
  if (params[1] == "moveup")
    CServiceBroker::GetGUI()->GetWindowManager().SendMessage(GUI_MSG_MOVE_OFFSET, windowID, controlID, 1);
  else if (params[1] == "movedown")
    CServiceBroker::GetGUI()->GetWindowManager().SendMessage(GUI_MSG_MOVE_OFFSET, windowID, controlID, -1);
  else if (params[1] == "pageup")
    CServiceBroker::GetGUI()->GetWindowManager().SendMessage(GUI_MSG_PAGE_UP, windowID, controlID);
  else if (params[1] == "pagedown")
    CServiceBroker::GetGUI()->GetWindowManager().SendMessage(GUI_MSG_PAGE_DOWN, windowID, controlID);
  else if (params[1] == "click")
    CServiceBroker::GetGUI()->GetWindowManager().SendMessage(GUI_MSG_CLICKED, controlID, windowID);

  return 0;
}

/*! \brief Give a control focus.
 *  \param params The parameters.
 *  \details params[0] = ID of control.
 *           params[1] = ID of subitem of control (optional).
 *           params[2] = "absolute" to focus the absolute position instead of the relative one (optional).
 */
static int SetFocus(const std::vector<std::string>& params)
{
  int controlID = atol(params[0].c_str());
  int subItem = (params.size() > 1) ? atol(params[1].c_str())+1 : 0;
  int absID = 0;
  if (params.size() > 2 && StringUtils::EqualsNoCase(params[2].c_str(), "absolute"))
    absID = 1;
  CGUIMessage msg(GUI_MSG_SETFOCUS, CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindowOrDialog(), controlID, subItem, absID);
  CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);

  return 0;
}

/*! \brief Set a control to visible.
 *  \param params The parameters.
 *  \details params[0] = ID of control.
 */
static int SetVisible(const std::vector<std::string>& params)
{
  int controlID = std::stol(params[0]);
  CGUIMessage msg{GUI_MSG_VISIBLE,
                  CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindowOrDialog(),
                  controlID};
  CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);

  return 0;
}

/*! \brief Set a control to hidden.
 *  \param params The parameters.
 *  \details params[0] = ID of control.
 */
static int SetHidden(const std::vector<std::string>& params)
{
  int controlID = std::stol(params[0]);
  CGUIMessage msg{GUI_MSG_HIDDEN,
                  CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindowOrDialog(),
                  controlID};
  CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);

  return 0;
}

/*! \brief Shift page in a control.
 *  \param params The parameters.
 *  \details params[0] = ID of control
 *
 *  Set Message template parameter to GUI_MSG_PAGE_DOWN/GUI_MSG_PAGE_UP.
 */
  template<int Message>
static int ShiftPage(const std::vector<std::string>& params)
{
  int id = atoi(params[0].c_str());
  CGUIMessage message(Message, CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindowOrDialog(), id);
  CServiceBroker::GetGUI()->GetWindowManager().SendMessage(message);

  return 0;
}

// Note: For new Texts with comma add a "\" before!!! Is used for table text.
//
/// \page page_List_of_built_in_functions
/// \section built_in_functions_7 GUI control built-in's
///
/// -----------------------------------------------------------------------------
///
/// \table_start
///   \table_h2_l{
///     Function,
///     Description }
///   \table_row2_l{
///     <b>`control.message(controlId\, action[\, windowId])`</b>
///     ,
///     Send a given message to a control within a given window
///     @param[in] controlId             ID of control.
///     @param[in] action                Action name.
///     @param[in] windowId              ID of window with control (optional).
///   }
///   \table_row2_l{
///     <b>`control.move(id\, offset)`</b>
///     ,
///     Tells the specified control to 'move' to another entry specified by offset
///     @param[in] id                    ID of control.
///     @param[in] offset                Offset of move.
///   }
///   \table_row2_l{
///     <b>`control.setfocus(controlId[\, subitemId])`</b>
///     ,
///     Change current focus to a different control id
///     @param[in] controlId             ID of control.
///     @param[in] subitemId             ID of subitem of control (optional).
///     @param[in] absolute              "absolute" to focus the absolute position instead of the relative one (optional).
///   }
///   \table_row2_l{
///     <b>`control.setvisible(controlId)`</b>
///     \anchor Builtin_SetVisible,
///     Set the control id to visible
///     @param[in] controlId             ID of control.
///     <p><hr>
///     @skinning_v20 **[New builtin]** \link Builtin_SetVisible `SetVisible(id)`\endlink
///     <p>
///   }
///   \table_row2_l{
///     <b>`control.sethidden(controlId)`</b>
///     \anchor Builtin_SetHidden,
///     Set the control id to hidden
///     @param[in] controlId             ID of control.
///     <p><hr>
///     @skinning_v20 **[New builtin]** \link Builtin_SetHidden `SetHidden(id)`\endlink
///     <p>
///   }
///   \table_row2_l{
///     <b>`pagedown(controlId)`</b>
///     ,
///     Send a page down event to the pagecontrol with given id
///     @param[in] controlId             ID of control.
///   }
///   \table_row2_l{
///     <b>`pageup(controlId)`</b>
///     ,
///     Send a page up event to the pagecontrol with given id
///     @param[in] controlId             ID of control.
///   }
///   \table_row2_l{
///     <b>`sendclick(controlId [\, windowId])`</b>
///     ,
///     Send a click message from the given control to the given window
///     @param[in] controlId             ID of control.
///     @param[in] windowId              ID for window with control (optional).
///   }
///   \table_row2_l{
///     <b>`setfocus`</b>
///     ,
///     Change current focus to a different control id
///     @param[in] controlId             ID of control.
///     @param[in] subitemId             ID of subitem of control (optional).
///     @param[in] absolute              "absolute" to focus the absolute position instead of the relative one (optional).
///   }
///  \table_end
///

CBuiltins::CommandMap CGUIControlBuiltins::GetOperations() const
{
  return {
           {"control.message",  {"Send a given message to a control within a given window", 2, SendMessage}},
           {"control.move",     {"Tells the specified control to 'move' to another entry specified by offset", 2, ControlMove}},
           {"control.setfocus", {"Change current focus to a different control id", 1, SetFocus}},
           {"control.setvisible", {"Set the control id to visible", 1, SetVisible}},
           {"control.sethidden", {"Set the control id to Hidden", 1, SetHidden}},
           {"pagedown",         {"Send a page down event to the pagecontrol with given id", 1, ShiftPage<GUI_MSG_PAGE_DOWN>}},
           {"pageup",           {"Send a page up event to the pagecontrol with given id", 1, ShiftPage<GUI_MSG_PAGE_UP>}},
           {"sendclick",        {"Send a click message from the given control to the given window", 1, SendClick}},
           {"setfocus",         {"Change current focus to a different control id", 1, SetFocus}},
         };
}
