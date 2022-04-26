/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "SkinTimer.h"
#include "threads/Thread.h"

#include <map>
#include <memory>
#include <string>

/*! \brief CSkinTimerManager is the container and manager for Skin timers. Its role is that of
 * checking if the timer boolean conditions are valid, start or stop timers and execute the respective
 * builtin actions linked to the timer lifecycle
 * \sa Skin_Timers
 * \sa CSkinTimer
 */
class CSkinTimerManager : public CThread
{
public:
  /*! \brief Skin timer manager constructor */
  CSkinTimerManager();

  /*! \brief Default skin timer manager destructor */
  ~CSkinTimerManager() override = default;

  /*! \brief Loads all the skin timers
  * \param path - the path for the skin Timers.xml file
  */
  void LoadTimers(const std::string& path);

  /*! \brief Starts the manager */
  void Start();

  /*! \brief Stops the manager */
  void Stop();

  /*! \brief Checks if the timer with name `timer` is running
   \param timer the name of the skin timer
   \return true if the given timer exists and is running, false otherwise
   */
  bool TimerIsRunning(const std::string& timer) const;

  /*! \brief Get the elapsed seconds since the timer with name `timer` was started
   \param timer the name of the skin timer
   \return the elapsed time in seconds the given timer is running (0 if not running or if it does not exist)
   */
  float GetTimerElapsedSeconds(const std::string& timer) const;

  /*! \brief Starts/Enables a given skin timer
   \param timer the name of the skin timer
   */
  void TimerStart(const std::string& timer) const;

  /*! \brief Stops/Disables a given skin timer
   \param timer the name of the skin timer
   */
  void TimerStop(const std::string& timer) const;

  // CThread methods

  /*! \brief Start and run the main manager loop */
  void Process() override;

  /*! \brief Stop the manager thread
   \param bWait - If the callee should wait for the thread to exit (default is true)
   */
  void StopThread(bool bWait = true) override;

private:
  /*! \brief Loads a specific timer
  * \note Called internally from LoadTimers
  * \param node - the XML representation of a skin timer object
  */
  void LoadTimerInternal(const TiXmlElement* node);

  /*! Container for the skin timers */
  std::map<std::string, std::unique_ptr<CSkinTimer>> m_timers;

  mutable CCriticalSection m_timerCriticalSection;
};
