/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "video/dialogs/GUIDialogVideoManager.h"

#include <cstdint>
#include <memory>
#include <string>

class CFileItem;

enum class VideoDbContentType;
enum class VideoAssetType;
enum class MediaRole;
enum class VersionConversionResult : uint8_t;

class CGUIDialogVideoManagerVersions : public CGUIDialogVideoManager
{
public:
  CGUIDialogVideoManagerVersions();
  ~CGUIDialogVideoManagerVersions() override = default;

  void SetVideoAsset(const std::shared_ptr<CFileItem>& item) override;

  /*!
   * \brief Find a movie in the library similar to dbId and, if confirmed, convert dbId into an
   * additional version of it.
   * \param[in] itemType content type of dbId
   * \param[in] dbId id of the movie to convert
   * \param[in] targetDbId when >= 0, skip searching the library for a similar movie and
   * reuse this movie id as the merge target instead - used to apply a merge decision already
   * made for an earlier, related item (e.g. another bluray playlist on the same disc) without
   * re-prompting for the same target.
   * \return a pair containing the result of the conversion and the id of the movie ends up attached to as a new version, -1 if not converted
   */
  static std::pair<VersionConversionResult, int> ProcessVideoVersion(VideoDbContentType itemType,
                                                                     int dbId,
                                                                     int targetDbId = -1);

  /*!
   * \brief Strip a trailing part/disc number from a movie's title, if present, and persist the
   * change. Used when a movie ends up spanning several bluray playlists/discs or folders, where
   * the number no longer makes sense as part of the (now shared) title.
   * \param[in] dbId id of the movie
   * \param[in] itemType content type of the movie
   * \param[in] db database connection to use
   */
  static void RemovePartNumberFromTitle(int dbId, VideoDbContentType itemType, CVideoDatabase& db);

  /*!
   * \brief Open the Manage Versions dialog for a video
   * \param item video to manage
   * \return true: the video or another item was modified, a containing list should be refreshed.
   * false: no changes
   */
  static bool ManageVideoVersions(const std::shared_ptr<CFileItem>& item);

protected:
  bool OnMessage(CGUIMessage& message) override;

  VideoAssetType GetVideoAssetType() override;
  int GetHeadingId() override { return 40024; } // Versions:

  void Clear() override;
  void Refresh() override;
  void UpdateButtons() override;
  void Remove() override;

private:
  enum class ReplaceExistingFile : bool
  {
    NO,
    YES
  };

  void SetDefaultVideoVersion(const CFileItem& version);
  /*!
   * \brief Prompt the user to select a file / movie to add as version
   * \return true if a version was added, false otherwise.
   */
  bool AddVideoVersion();
  void SetDefault();
  void UpdateDefaultVideoVersionSelection();

  enum class Mode
  {
    INTERACTIVE,
    NON_INTERACTIVE,
  };

  /*!
   * \brief Ask the user to choose an item from the list of items, the version type of the item,
   * and perform the version conversion according to the role parameter.
   * \param[in] items The items for the user to choose from
   * \param[in] itemType Type of the item being chosen
   * \param[in] dbId id of the video being added if role is NewVersion, id of the video being added
   * to if role is Parent
   * \param[in] videoDb Database connection
   * \param[in] role NewVersion: dbId will be converted to a version of the movie chosen by
   *                 the user from the whole library.
   * \param[in] mode INTERACTIVE: ask the user to choose and confirm as needed.
   *                 NON_INTERACTIVE: no user interaction allowed, use heuristics in
   *                 place of user input
   * Parent: dbId will have another movie chosen by the user from the whole library as a new version.
   *
   * \return A pair containing the result of the conversion and the id of the movie ends up attached to as a new version, -1 if not converted
   */
  static std::pair<VersionConversionResult, int> ChooseVideoAndConvertToVideoVersion(
      CFileItemList& items,
      VideoDbContentType itemType,
      int dbId,
      CVideoDatabase& videoDb,
      MediaRole role,
      Mode mode);

  /*!
   * \brief Convert dbId into a new version of the already-chosen selectedItem: confirm (as
   * needed), ask for the version type (as needed), and perform the conversion.
   * \param[in] selectedItem the movie dbId is to become a version of (role NewVersion) or that is
   * to become a version of dbId (role Parent)
   * \param[in] itemType Type of the item being chosen
   * \param[in] dbId id of the video being added if role is NewVersion, id of the video being
   * added to if role is Parent
   * \param[in] videoDb Database connection
   * \param[in] role see ChooseVideoAndConvertToVideoVersion
   * \param[in] mode see ChooseVideoAndConvertToVideoVersion
   * \return A pair containing the result of the conversion and the id of the movie ends up attached to as a new version, -1 if not converted
   */
  static std::pair<VersionConversionResult, int> ConvertToVideoVersion(
      const std::shared_ptr<CFileItem>& selectedItem,
      VideoDbContentType itemType,
      int dbId,
      CVideoDatabase& videoDb,
      MediaRole role,
      Mode mode);

  /*!
   * \brief Use a file picker to select a file to add as a new version of a movie.
   * \return True when a version was added, false otherwise
   */
  bool AddVideoVersionFilePicker();

  /*!
   * \brief Populates a list of movies of the library that are similar to the item provided as
   * parameter.
   * \param[in] item The reference item
   * \param[out] list List to populate
   * \param[in] videoDb Database connection
   * \return True for success, false otherwise
   */
  static bool GetSimilarMovies(const std::shared_ptr<CFileItem>& item,
                               CFileItemList& list,
                               CVideoDatabase& videoDb);

  /*!
   * \brief Convert the movie into a version
   * \param itemMovie Movie to convert
   * \return True for success, false otherwise
   */
  bool AddSimilarMovieAsVersion(const std::shared_ptr<CFileItem>& itemMovie);

  /*!
   * \brief Populates a list with all movies of the library, excluding the item provided as parameter.
   * \param[in] item The item that will be excluded from the list
   * \param[out] list List to populate
   * \param[in] videoDb Database connection
   * \return True for success, false otherwise.
   */
  static bool GetAllOtherMovies(const std::shared_ptr<CFileItem>& item,
                                CFileItemList& list,
                                CVideoDatabase& videoDb);

  /*!
   * \brief Shared post processing of lists after extraction and before display
   * \param[in,out] list the list of movies
   * \param[in] dbId item to remove from the list
   */
  static void PostProcessList(CFileItemList& list, int dbId);

  /*!
   * \brief Prompts the user to choose a playlist from the current disc
   * \param item the current CFileItem
   * \param replaceExistingFile whether to replace the existing playlist in the database
   * \return true for success, false otherwise.
   */
  bool ChoosePlaylist(const std::shared_ptr<CFileItem>& item,
                      ReplaceExistingFile replaceExistingFile);

  std::shared_ptr<CFileItem> m_defaultVideoVersion;
};
