/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "SkinTimer.h"

#include <map>
#include <memory>
#include <string>

class CGUIInfoManager;
namespace tinyxml2
{
class XMLNode;
}

/*! \brief CSkinTimerManager is the container and manager for Skin timers. Its role is that of
 * checking if the timer boolean conditions are valid, start or stop timers and execute the respective
 * builtin actions linked to the timer lifecycle
 * \note This component should only be called by the main/rendering thread
 * \sa Skin_Timers
 * \sa CSkinTimer
 */
class CSkinTimerManager
{
public:
  /*! \brief Skin timer manager constructor
   *  \param infoMgr reference to the infomanager
   */
  CSkinTimerManager(CGUIInfoManager& infoMgr);
  CSkinTimerManager() = delete;

  /*! \brief Default skin timer manager destructor */
  ~CSkinTimerManager() = default;

  /*! \brief Loads all the skin timers
  * \param path - the path for the skin Timers.xml file
  */
  void LoadTimers(const std::string& path);

  /*! \brief Stops the manager */
  void Stop();

  /*! \brief Gets the total number of timers registered in the manager
     *  \return the timer count
    */
  size_t GetTimerCount() const;

  /*! \brief Checks if the timer with name `timer` exists
   *  \param timer the name of the skin timer
   *  \return true if the given timer exists, false otherwise
  */
  bool TimerExists(const std::string& timer) const;

  /*! \brief Move a given timer, removing it from the manager
   *  \param timer the name of the skin timer
   *  \return the timer (moved), nullptr if it doesn't exist
  */
  std::unique_ptr<CSkinTimer> GrabTimer(const std::string& timer);

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

  /*! \brief Run the main manager processing loop */
  void Process();

private:
  /*! \brief Loads a specific timer
  * \note Called internally from LoadTimers
  * \param node - the XML representation of a skin timer object
  */
  void LoadTimerInternal(const tinyxml2::XMLNode* node);

  /*! Container for the skin timers */
  std::map<std::string, std::unique_ptr<CSkinTimer>> m_timers;
  CGUIInfoManager& m_infoMgr;
};
