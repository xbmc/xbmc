#pragma once
/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <map>
#include "ThumbLoader.h"
#include "utils/JobManager.h"
#include "FileItem.h"

class CStreamDetails;
class CVideoDatabase;

/*!
 \ingroup thumbs,jobs
 \brief Thumb extractor job class

 Used by the CVideoThumbLoader to perform asynchronous generation of thumbs

 \sa CVideoThumbLoader and CJob
 */
class CThumbExtractor : public CJob
{
public:
  CThumbExtractor(const CFileItem& item, const std::string& listpath, bool thumb, const std::string& strTarget="");
  virtual ~CThumbExtractor();

  /*!
   \brief Work function that extracts thumb.
   */
  virtual bool DoWork();

  virtual const char* GetType() const
  {
    return kJobTypeMediaFlags;
  }

  virtual bool operator==(const CJob* job) const;

  std::string m_target; ///< thumbpath
  std::string m_listpath; ///< path used in fileitem list
  CFileItem  m_item;
  bool       m_thumb; ///< extract thumb?
};

class CVideoThumbLoader : public CThumbLoader, public CJobQueue
{
public:
  CVideoThumbLoader();
  virtual ~CVideoThumbLoader();

  virtual void OnLoaderStart();
  virtual void OnLoaderFinish();

  virtual bool LoadItem(CFileItem* pItem);
  virtual bool LoadItemCached(CFileItem* pItem);
  virtual bool LoadItemLookup(CFileItem* pItem);

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
 virtual bool FillLibraryArt(CFileItem &item);

  /*!
   \brief Callback from CThumbExtractor on completion of a generated image

   Performs the callbacks and updates the GUI.

   \sa CImageLoader, IJobCallback
   */
  virtual void OnJobComplete(unsigned int jobID, bool success, CJob *job);

  /*! \brief set the artwork map for an item
   In addition, sets the standard fallbacks.
   \param item the item on which to set art.
   \param artwork the artwork map.
   */
  static void SetArt(CFileItem &item, const std::map<std::string, std::string> &artwork);

protected:
  CVideoDatabase *m_videoDatabase;
  typedef std::map<int, std::map<std::string, std::string> > ArtCache;
  ArtCache m_showArt;

  /*! \brief Tries to detect missing data/info from a file and adds those
   \param item The CFileItem to process
   \return void
   */
  void DetectAndAddMissingItemData(CFileItem &item);
};
