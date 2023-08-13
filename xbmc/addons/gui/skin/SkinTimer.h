/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIAction.h"
#include "interfaces/info/InfoExpression.h"
#include "utils/Stopwatch.h"

#include <memory>
#include <string>

class TiXmlElement;

/*! \brief Skin timers are skin objects that dependent on time and can be fully controlled from skins either using boolean
 * conditions or builtin functions. This class represents the Skin Timer object.
 * \sa Skin_Timers
 */
class CSkinTimer : public CStopWatch
{
public:
  /*! \brief Skin timer constructor
  * \param name - the name of the timer
  * \param startCondition - the boolean info expression to start the timer (may be null)
  * \param resetCondition - the boolean info expression to reset the timer (may be null)
  * \param stopCondition - the boolean info expression to stop the timer (may be null)
  * \param startActions - the builtin functions to execute on timer start (actions may be empty)
  * \param stopActions - the builtin functions to execute on timer stop (actions may be empty)
  * \param resetOnStart - if the timer should be reset when started (i.e. start from zero if true or resumed if false)
  */
  CSkinTimer(const std::string& name,
             const INFO::InfoPtr& startCondition,
             const INFO::InfoPtr& resetCondition,
             const INFO::InfoPtr& stopCondition,
             const CGUIAction& startActions,
             const CGUIAction& stopActions,
             bool resetOnStart);

  /*! \brief Default skin timer destructor */
  virtual ~CSkinTimer() = default;

  /*! \brief Start the skin timer */
  void Start();

  /*! \brief Resets the skin timer so that the elapsed time of the timer is 0 */
  void Reset();

  /*! \brief stops the skin timer */
  void Stop();

  /*! \brief Getter for the timer name
   * \return the timer name
  */
  const std::string& GetName() const;

  /*! \brief Getter for the timer start boolean condition/expression
  * \return the start boolean condition/expression (may be null)
  */
  INFO::InfoPtr GetStartCondition() const;

  /*! \brief Getter for the timer reset boolean condition/expression
  * \return the reset boolean condition/expression (may be null)
  */
  INFO::InfoPtr GetResetCondition() const;

  /*! \brief Getter for the timer start boolean condition/expression
  * \return the start boolean condition/expression (may be null)
  */
  INFO::InfoPtr GetStopCondition() const;

  /*! \brief Checks if a given timer is configured to reset every time it starts
   * \return true if the timer is configured to reset on start, false otherwise
  */
  bool ResetsOnStart() const;

  /*! \brief Gets the start actions of this skin timer
   * \return the actions configured to start when the timer is started
  */
  const CGUIAction& GetStartActions() const;

  /*! \brief Gets the stop actions of this skin timer
   * \return the actions configured to start when the timer is stopped
  */
  const CGUIAction& GetStopActions() const;

  /*! \brief Evaluates the timer start boolean info expression returning the respective result.
  * \details Called from the skin timer manager to check if the timer should be started
  * \return true if the condition is true, false otherwise
  */
  bool VerifyStartCondition() const;

  /*! \brief Evaluates the timer reset boolean info expression returning the respective result.
  * \details Called from the skin timer manager to check if the timer should be reset to 0
  * \return true if the condition is true, false otherwise
  */
  bool VerifyResetCondition() const;

  /*! \brief Evaluates the timer stop boolean info expression returning the respective result.
  * \details Called from the skin timer manager to check if the timer should be stopped
  * \return true if the condition is true, false otherwise
  */
  bool VerifyStopCondition() const;

private:
  /*! \brief Called when this timer is started */
  void OnStart();

  /*! \brief Called when this timer is stopped */
  void OnStop();

  /*! The name of the skin timer */
  std::string m_name;
  /*! The info boolean expression that automatically starts the timer if evaluated true */
  INFO::InfoPtr m_startCondition;
  /*! The info boolean expression that automatically resets the timer if evaluated true */
  INFO::InfoPtr m_resetCondition;
  /*! The info boolean expression that automatically stops the timer if evaluated true */
  INFO::InfoPtr m_stopCondition;
  /*! The builtin functions to be executed when the timer is started */
  CGUIAction m_startActions;
  /*! The builtin functions to be executed when the timer is stopped */
  CGUIAction m_stopActions;
  /*! if the timer should be reset on start (or just resumed) */
  bool m_resetOnStart{false};
};
