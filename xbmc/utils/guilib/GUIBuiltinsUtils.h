/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>

class CExecString;
class CFileItem;

namespace KODI::UTILS::GUILIB
{
class CGUIBuiltinsUtils
{
public:
  /*! \brief Execute the action given by exec string and item.
   \param execute The action to execute
   \param item The item associated with the action to execute
   \return True on success, false otherwise
   */
  static bool ExecuteAction(const CExecString& execute, const std::shared_ptr<CFileItem>& item);

  /*! \brief Execute the action given by exec string and item.
   \param execute The action to execute
   \param item The item associated with the action to execute
   \return True on success, false otherwise
   */
  static bool ExecuteAction(const std::string& execute, const std::shared_ptr<CFileItem>& item);

  /*! \brief Execute "PlayMedia" for the given item, resume of possible, else play from beginning.
   \param item The item associated with the action to execute
   \return True on success, false otherwise
   */
  static bool ExecutePlayMediaTryResume(const std::shared_ptr<CFileItem>& item);

  /*! \brief Execute "PlayMedia" for the given item, force playback from the beginning.
   \param item The item associated with the action to execute
   \return True on success, false otherwise
   */
  static bool ExecutePlayMediaNoResume(const std::shared_ptr<CFileItem>& item);

  /*! \brief Execute "PlayMedia" for the given item, ask whether to resume if possible, else play from beginning.
   \param item The item associated with the action to execute
   \return True on success, false otherwise
   */
  static bool ExecutePlayMediaAskResume(const std::shared_ptr<CFileItem>& item);

  /*! \brief Execute "QueueMedia" for the given item.
   \param item The item associated with the action to execute
   \return True on success, false otherwise
   */
  static bool ExecuteQueueMedia(const std::shared_ptr<CFileItem>& item);
};
} // namespace KODI::UTILS::GUILIB
