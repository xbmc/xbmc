/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIContainerBuiltins.h"

#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "utils/StringUtils.h"

/*! \brief Change sort method.
 *  \param params (ignored)
 *
 *  Set the Dir template parameter to 1 to switch to next sort method
 *  or -1 to switch to previous sort method.
 */
  template<int Dir>
static int ChangeSortMethod(const std::vector<std::string>& params)
{
  CGUIMessage message(GUI_MSG_CHANGE_SORT_METHOD, CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow(), 0, 0, Dir);
  CServiceBroker::GetGUI()->GetWindowManager().SendMessage(message);

  return 0;
}

/*! \brief Change view mode.
 *  \param params (ignored)
 *
 *  Set the Dir template parameter to 1 to switch to next view mode
 *  or -1 to switch to previous view mode.
 */
  template<int Dir>
static int ChangeViewMode(const std::vector<std::string>& params)
{
  CGUIMessage message(GUI_MSG_CHANGE_VIEW_MODE, CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow(), 0, 0, Dir);
  CServiceBroker::GetGUI()->GetWindowManager().SendMessage(message);

  return 0;
}

/*! \brief Refresh a media window.
 *  \param params The parameters.
 *  \details params[0] = The URL to refresh window at.
 */
static int Refresh(const std::vector<std::string>& params)
{ // NOTE: These messages require a media window, thus they're sent to the current activewindow.
  //       This shouldn't stop a dialog intercepting it though.
  CGUIMessage message(GUI_MSG_NOTIFY_ALL, CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow(), 0, GUI_MSG_UPDATE, 1); // 1 to reset the history
  message.SetStringParam(!params.empty() ? params[0] : "");
  CServiceBroker::GetGUI()->GetWindowManager().SendMessage(message);

  return 0;
}

/*! \brief Set sort method.
 *  \param params The parameters.
 *  \details params[0] = ID of sort method.
 */
static int SetSortMethod(const std::vector<std::string>& params)
{
  CGUIMessage message(GUI_MSG_CHANGE_SORT_METHOD, CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow(), 0, atoi(params[0].c_str()));
  CServiceBroker::GetGUI()->GetWindowManager().SendMessage(message);

  return 0;
}

/*! \brief Set view mode.
 *  \param params The parameters.
 *  \details params[0] = ID of view mode.
 */
static int SetViewMode(const std::vector<std::string>& params)
{
  CGUIMessage message(GUI_MSG_CHANGE_VIEW_MODE, CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow(), 0, atoi(params[0].c_str()));
  CServiceBroker::GetGUI()->GetWindowManager().SendMessage(message);

  return 0;
}

/*! \brief Toggle sort direction.
 *  \param params (ignored)
 */
static int ToggleSortDirection(const std::vector<std::string>& params)
{
  CGUIMessage message(GUI_MSG_CHANGE_SORT_DIRECTION, CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow(), 0, 0);
  CServiceBroker::GetGUI()->GetWindowManager().SendMessage(message);

  return 0;
}

/*! \brief Update a listing in a media window.
 *  \param params The parameters.
 *  \details params[0] = The URL to update listing at.
 *           params[1] = "replace" to reset history (optional).
 */
static int Update(const std::vector<std::string>& params)
{
  CGUIMessage message(GUI_MSG_NOTIFY_ALL, CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow(), 0, GUI_MSG_UPDATE, 0);
  message.SetStringParam(params[0]);
  if (params.size() > 1 && StringUtils::EqualsNoCase(params[1], "replace"))
    message.SetParam2(1); // reset the history
  CServiceBroker::GetGUI()->GetWindowManager().SendMessage(message);

  return 0;
}

// Note: For new Texts with comma add a "\" before!!! Is used for table text.
//
/// \page page_List_of_built_in_functions
/// \section built_in_functions_6 GUI container built-in's
///
/// -----------------------------------------------------------------------------
///
/// \table_start
///   \table_h2_l{
///     Function,
///     Description }
///   \table_row2_l{
///     <b>`Container.NextSortMethod`</b>
///     ,
///     Change to the next sort method.
///   }
///   \table_row2_l{
///     <b>`Container.NextViewMode`</b>
///     ,
///     Select the next view mode.
///   }
///   \table_row2_l{
///     <b>`Container.PreviousSortMethod`</b>
///     ,
///     Change to the previous sort method.
///   }
///   \table_row2_l{
///     <b>`Container.PreviousViewMode`</b>
///     ,
///     Select the previous view mode.
///   }
///   \table_row2_l{
///     <b>`Container.Refresh(url)`</b>
///     ,
///     Refresh current listing
///     @param[in] url                   The URL to refresh window at.
///   }
///   \table_row2_l{
///     <b>`Container.SetSortMethod(id)`</b>
///     ,
///     Change to the specified sort method. (For list of ID's \ref SortBy "see List" of sort methods below)
///     @param[in] id                    ID of sort method.
///   }
///   \table_row2_l{
///     <b>`Container.SetViewMode(id)`</b>
///     ,
///     Set the current view mode (list\, icons etc.) to the given container id.
///     @param[in] id                    ID of view mode.
///   }
///   \table_row2_l{
///     <b>`Container.SortDirection`</b>
///     ,
///     Toggle the sort direction
///   }
///   \table_row2_l{
///     <b>`Container.Update(url\,[replace])`</b>
///     ,
///     Update current listing. Send `Container.Update(path\,replace)` to reset the path history.
///     @param[in] url                   The URL to update listing at.
///     @param[in] replace               "replace" to reset history (optional).
///   }
/// \table_end
///

CBuiltins::CommandMap CGUIContainerBuiltins::GetOperations() const
{
  return {
           {"container.nextsortmethod",     {"Change to the next sort method", 0, ChangeSortMethod<1>}},
           {"container.nextviewmode",       {"Move to the next view type (and refresh the listing)", 0, ChangeViewMode<1>}},
           {"container.previoussortmethod", {"Change to the previous sort method", 0, ChangeSortMethod<-1>}},
           {"container.previousviewmode",   {"Move to the previous view type (and refresh the listing)", 0, ChangeViewMode<-1>}},
           {"container.refresh",            {"Refresh current listing", 0, Refresh}},
           {"container.setsortdirection",   {"Toggle the sort direction", 0, ToggleSortDirection}},
           {"container.setsortmethod",      {"Change to the specified sort method", 1, SetSortMethod}},
           {"container.setviewmode",        {"Move to the view with the given id", 1, SetViewMode}},
           {"container.update",             {"Update current listing. Send Container.Update(path,replace) to reset the path history", 1, Update}}
         };
}
