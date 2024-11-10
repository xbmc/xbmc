/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/CriticalSection.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

class CVideoDatabase;

namespace PVR
{
class CPVRClient;
class CPVREpgInfoTag;
class CPVRMediaTag;
class CPVRMediaTagUid;
class CPVRMediaPath;

class CPVRMedia
{
public:
  CPVRMedia();
  virtual ~CPVRMedia();

  /*!
   * @brief Update all media from the given PVR clients.
   * @param clients The PVR clients data should be loaded for. Leave empty for all clients.
   * @return True on success, false otherwise.
   */
  bool Update(const std::vector<std::shared_ptr<CPVRClient>>& clients);

  /*!
   * @brief unload all media.
   */
  void Unload();

  /*!
   * @brief Update data with media from the given clients, sync with local data.
   * @param clients The clients to fetch data from. Leave empty to fetch data from all created clients.
   * @return True on success, false otherwise.
   */
  bool UpdateFromClients(const std::vector<std::shared_ptr<CPVRClient>>& clients);

  /*!
   * @brief client has delivered a new/updated media tag.
   * @param tag The media tag
   * @param client The client the media tag belongs to.
   */
  void UpdateFromClient(const std::shared_ptr<CPVRMediaTag>& tag, const CPVRClient& client);

  // /*!
  //  * @brief refresh the size of any in progress media from the clients.
  //  */
  // void UpdateInProgressSize();

  int GetNumTVMedia() const;
  int GetNumRadioMedia() const;

  /*!
   * @brief Set a media tag's watched state
   * @param mediaTag The media tag
   * @param bWatched True to set watched, false to set unwatched state
   * @return True on success, false otherwise
   */
  bool MarkWatched(const std::shared_ptr<CPVRMediaTag>& mediaTag, bool bWatched);

  /*!
   * @brief Reset a media tag's resume point, if any
   * @param mediaTag The media tag
   * @return True on success, false otherwise
   */
  bool ResetResumePoint(const std::shared_ptr<CPVRMediaTag>& mediaTag);

  /*!
   * @brief Get a list of all media
   * @return the list of all media
   */
  std::vector<std::shared_ptr<CPVRMediaTag>> GetAll() const;

  std::shared_ptr<CPVRMediaTag> GetByPath(const std::string& path) const;
  std::shared_ptr<CPVRMediaTag> GetById(int iClientId, const std::string& strMediaTagId) const;
  std::shared_ptr<CPVRMediaTag> GetById(unsigned int iId) const;

  /*!
   * @brief Erase stale texture db entries and image files.
   * @return number of cleaned up images.
   */
  int CleanupCachedImages();

private:
  /*!
   * @brief Get/Open the video database.
   * @return A reference to the video database.
   */
  CVideoDatabase& GetVideoDatabase();

  /*!
   * @brief Set a media tag's play count
   * @param mediaTag The media tag
   * @param count The new play count
   * @return True on success, false otherwise
   */
  bool SetMediaPlayCount(const std::shared_ptr<CPVRMediaTag>& mediaTag, int count);

  /*!
   * @brief Increment a media tag's play count
   * @param mediaTag The media tag
   * @return True on success, false otherwise
   */
  bool IncrementMediaPlayCount(const std::shared_ptr<CPVRMediaTag>& mediaTag);

  /*!
   * @brief special value for parameter count of method ChangeMediaPlayCount
   */
  static const int INCREMENT_PLAY_COUNT = -1;

  /*!
   * @brief change the play count of the given media tag
   * @param mediaTag The media tag
   * @param count The new play count or INCREMENT_PLAY_COUNT to denote that the current play count is to be incremented by one
   * @return true if the play count was changed successfully
   */
  bool ChangeMediaPlayCount(const std::shared_ptr<CPVRMediaTag>& mediaTag, int count);

  mutable CCriticalSection m_critSection;
  bool m_bIsUpdating = false;
  std::map<CPVRMediaTagUid, std::shared_ptr<CPVRMediaTag>> m_media;
  unsigned int m_iLastId = 0;
  std::unique_ptr<CVideoDatabase> m_database;
  unsigned int m_iTVMedia = 0;
  unsigned int m_iRadioMedia = 0;
};
} // namespace PVR
