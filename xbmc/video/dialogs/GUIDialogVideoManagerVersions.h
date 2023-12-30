/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "video/dialogs/GUIDialogVideoManager.h"

#include <memory>
#include <string>
#include <tuple>

class CFileItem;

enum class VideoDbContentType;
enum class VideoAssetType;

class CGUIDialogVideoManagerVersions : public CGUIDialogVideoManager
{
public:
  CGUIDialogVideoManagerVersions();
  ~CGUIDialogVideoManagerVersions() override = default;

  void SetVideoAsset(const std::shared_ptr<CFileItem>& item) override;

  static std::tuple<int, std::string> NewVideoVersion();
  static bool ProcessVideoVersion(VideoDbContentType itemType, int dbId);
  static void ManageVideoVersions(const std::shared_ptr<CFileItem>& item);
  static int ManageVideoVersionContextMenu(const std::shared_ptr<CFileItem>& version);

protected:
  bool OnMessage(CGUIMessage& message) override;

  VideoAssetType GetVideoAssetType() override;
  int GetHeadingId() override { return 40024; } // Versions:

  void Refresh() override;
  void UpdateButtons() override;
  void Remove() override;

private:
  void SetDefaultVideoVersion(const CFileItem& version);
  /*!
   * \brief Prompt the user to select a file / movie to add as version
   * \return true if a version was added, false otherwise.
   */
  bool AddVideoVersion();
  void SetDefault();
  void UpdateDefaultVideoVersionSelection();

  /*!
   * \brief 
   * \param items The items for the user to choose from
   * \param itemType Type of the item being chosen
   * \param mediaType "movie" ?
   * \param dbId id of the video being added
   * \param videoDb Databse connection
   * \return 
  */
  static bool ChooseVideoAndConvertToVideoVersion(CFileItemList& items,
                                                  VideoDbContentType itemType,
                                                  const std::string& mediaType,
                                                  int dbId,
                                                  CVideoDatabase& videoDb);
  /*!
   * \brief Use a file picker to select a file to add as a new version of a movie.
   * \return True when a version was added, false otherwise
   */
  bool AddVideoVersionFilePicker();

  /*!
   * \brief Populates a list of movies of the library that are similar to the video asset of the
   * dialog
   * \param[out] list The list of movies
   * \return True for success, false otherwise
   */
  bool GetSimilarMovies(CFileItemList& list);

  /*!
   * \brief Convert the movie into a version
   * \param itemMovie Movie to convert
   * \return True for success, false otherwse
   */
  bool AddSimilarMovieAsVersion(const std::shared_ptr<CFileItem> itemMovie);

  std::shared_ptr<CFileItem> m_defaultVideoVersion;
};
