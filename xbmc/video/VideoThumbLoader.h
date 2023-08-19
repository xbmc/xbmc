/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "FileItem.h"
#include "ThumbLoader.h"

#include <map>
#include <vector>

class CStreamDetails;
class CVideoDatabase;
class EmbeddedArt;

using ArtMap = std::map<std::string, std::string>;
using ArtCache = std::map<std::pair<MediaType, int>, ArtMap>;

class CVideoThumbLoader : public CThumbLoader
{
public:
  CVideoThumbLoader();
  ~CVideoThumbLoader() override;

  void OnLoaderStart() override;
  void OnLoaderFinish() override;

  bool LoadItem(CFileItem* pItem) override;
  bool LoadItemCached(CFileItem* pItem) override;
  bool LoadItemLookup(CFileItem* pItem) override;

  /*! \brief Fill the thumb of a video item
   First uses a cached thumb from a previous run, then checks for a local thumb
   and caches it for the next run
   \param item the CFileItem object to fill
   \return true if we fill the thumb, false otherwise
   */
  virtual bool FillThumb(CFileItem &item);

  /*! \brief Find a particular art type for a given item, optionally checking at the folder level
   \param item the CFileItem to search.
   \param type the type of art to look for.
   \param checkFolder whether to also check the folder level for files. Defaults to false.
   \return the art file (if found), else empty.
   */
  static std::string GetLocalArt(const CFileItem &item, const std::string &type, bool checkFolder = false);

  /*! \brief return the available art types for a given media type
   \param type the type of media.
   \return a vector of art types.
   \sa GetLocalArt
   */
  static std::vector<std::string> GetArtTypes(const std::string &type);

  static bool IsValidArtType(const std::string& potentialArtType);

  static bool IsArtTypeInWhitelist(const std::string& artType, const std::vector<std::string>& whitelist, bool exact);

  /*! \brief helper function to retrieve a thumb URL for embedded video thumbs
   \param item a video CFileItem.
   \return a URL for the embedded thumb.
   */
  static std::string GetEmbeddedThumbURL(const CFileItem &item);

  /*! \brief helper function to fill the art for a video library item
   \param item a video CFileItem
   \return true if we fill art, false otherwise
   */
 bool FillLibraryArt(CFileItem &item) override;

protected:
  CVideoDatabase *m_videoDatabase;
  ArtCache m_artCache;

  /*! \brief Tries to detect missing data/info from a file and adds those
   \param item The CFileItem to process
   \return void
   */
  void DetectAndAddMissingItemData(CFileItem &item);

  const ArtMap& GetArtFromCache(const std::string &mediaType, const int id);
};
