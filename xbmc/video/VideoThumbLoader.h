/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <map>
#include <vector>
#include "ThumbLoader.h"
#include "utils/JobManager.h"
#include "FileItem.h"

class CStreamDetails;
class CVideoDatabase;
class EmbeddedArt;

using ArtMap = std::map<std::string, std::string>;
using ArtCache = std::map<std::pair<MediaType, int>, ArtMap>;

/*!
 \ingroup thumbs,jobs
 \brief Thumb extractor job class

 Used by the CVideoThumbLoader to perform asynchronous generation of thumbs

 \sa CVideoThumbLoader and CJob
 */
class CThumbExtractor : public CJob
{
public:
  CThumbExtractor(const CFileItem& item, const std::string& listpath, bool thumb, const std::string& strTarget="", int64_t pos = -1, bool fillStreamDetails = true);
  ~CThumbExtractor() override;

  /*!
   \brief Work function that extracts thumb.
   */
  bool DoWork() override;

  const char* GetType() const override
  {
    return kJobTypeMediaFlags;
  }

  bool operator==(const CJob* job) const override;

  std::string m_target; ///< thumbpath
  std::string m_listpath; ///< path used in fileitem list
  CFileItem  m_item;
  bool       m_thumb; ///< extract thumb?
  int64_t    m_pos; ///< position to extract thumb from
  bool m_fillStreamDetails; ///< fill in stream details?
};

class CVideoThumbLoader : public CThumbLoader, public CJobQueue
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

  /*!
   \brief Callback from CThumbExtractor on completion of a generated image

   Performs the callbacks and updates the GUI.

   \sa CImageLoader, IJobCallback
   */
  void OnJobComplete(unsigned int jobID, bool success, CJob *job) override;

  /*! \brief set the artwork map for an item
   In addition, sets the standard fallbacks.
   \param item the item on which to set art.
   \param artwork the artwork map.
   */
  static void SetArt(CFileItem &item, const std::map<std::string, std::string> &artwork);

  static bool GetEmbeddedThumb(const std::string& path,
                               const std::string& type,
                               EmbeddedArt& art);

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
