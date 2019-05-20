/*
 *  Copyright (C) 2012-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "URL.h"

class CFileItemList;

namespace PVR
{

class CPVRGUIDirectory
{
public:
  /*!
   * @brief PVR GUI directory ctor.
   * @param url The directory's URL.
   */
  explicit CPVRGUIDirectory(const CURL& url) : m_url(url) {}

  /*!
   * @brief PVR GUI directory ctor.
   * @param path The directory's path.
   */
  explicit CPVRGUIDirectory(const std::string& path) : m_url(path) {}

  /*!
   * @brief PVR GUI directory dtor.
   */
  virtual ~CPVRGUIDirectory() = default;

  /*!
   * @brief Check existense of this directory.
   * @return True if the directory exists, false otherwise.
   */
  bool Exists() const;

  /*!
   * @brief Obtain the directory listing.
   * @param results The list to fill with the results.
   * @return True on success, false otherwise.
   */
  bool GetDirectory(CFileItemList& results) const;

  /*!
   * @brief Check if this directory supports file write operations.
   * @return True if the directory supports file write operations, false otherwise.
   */
  bool SupportsWriteFileOperations() const;

  /*!
   * @brief Check if any TV recordings are existing.
   * @return True if TV recordings exists, false otherwise.
   */
  static bool HasTVRecordings();

  /*!
   * @brief Check if any deleted TV recordings are existing.
   * @return True if deleted TV recordings exists, false otherwise.
   */
  static bool HasDeletedTVRecordings();

  /*!
   * @brief Check if any radio recordings are existing.
   * @return True if radio recordings exists, false otherwise.
   */
  static bool HasRadioRecordings();

  /*!
   * @brief Check if any deleted radio recordings are existing.
   * @return True if deleted radio recordings exists, false otherwise.
   */
  static bool HasDeletedRadioRecordings();

private:

  bool FilterDirectory(CFileItemList& results) const;

  bool GetChannelGroupsDirectory(bool bRadio, CFileItemList& results) const;
  bool GetChannelsDirectory(CFileItemList& results) const;
  bool GetTimersDirectory(CFileItemList& results) const;
  bool GetRecordingsDirectory(CFileItemList& results) const;

  const CURL m_url;
};

}
