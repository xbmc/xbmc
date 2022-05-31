/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <vector>

class CAction;
class CCriticalSection;
class IActionListener;

/*!
 * \brief Class handling application support for action listeners.
 */

class CApplicationActionListeners
{
public:
  CApplicationActionListeners(CCriticalSection& sect);

  /*!
   \brief Register an action listener.
   \param listener The listener to register
   */
  void RegisterActionListener(IActionListener* listener);
  /*!
   \brief Unregister an action listener.
   \param listener The listener to unregister
   */
  void UnregisterActionListener(IActionListener* listener);

protected:
  /*!
   \brief Delegates the action to all registered action handlers.
   \param action The action
   \return true, if the action was taken by one of the action listener.
   */
  bool NotifyActionListeners(const CAction& action) const;

  std::vector<IActionListener*> m_actionListeners;

  CCriticalSection& m_critSection;
};
