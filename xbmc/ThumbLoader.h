/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifndef THUMBLOADER_H
#define THUMBLOADER_H
#include "BackgroundInfoLoader.h"
#include "utils/JobManager.h"
#include "FileItem.h"

class CStreamDetails;
class IStreamDetailsObserver;

/*!
 \ingroup thumbs,jobs
 \brief Thumb extractor job class

 Used by the CVideoThumbLoader to perform asynchronous generation of thumbs

 \sa CVideoThumbLoader and CJob
 */
class CThumbExtractor : public CJob
{
public:
  CThumbExtractor(const CFileItem& item, const CStdString& listpath, bool thumb, const CStdString& strTarget="");
  virtual ~CThumbExtractor();

  /*!
   \brief Work function that extracts thumb.
   */
  virtual bool DoWork();

  virtual const char* GetType() const
  {
    return "mediaflags";
  }

  virtual bool operator==(const CJob* job) const;

  CStdString m_path; ///< path of video to extract thumb from
  CStdString m_target; ///< thumbpath
  CStdString m_listpath; ///< path used in fileitem list
  CFileItem  m_item;
  bool       m_thumb; ///< extract thumb?
};

class CThumbLoader : public CBackgroundInfoLoader
{
public:
  CThumbLoader(int nThreads=-1);
  virtual ~CThumbLoader();

  bool LoadRemoteThumb(CFileItem *pItem);

  /*! \brief Checks whether the given item has a thumb that needs caching, and if so caches it.
   \param item CFileItem to check for a cachable thumb.
   \return true if we successfully cache a thumb, false otherwise.
   \sa GetCachedThumb
   */
  static bool CheckAndCacheThumb(CFileItem &item);

  /*! \brief Checks whether the given item has a thumb listed in the texture database
   \param item CFileItem to check for a thumb
   \return the thumb associated with this item
   \sa CheckAndCacheThumb
   */
  static CStdString GetCachedThumb(const CFileItem &item);
};

class CVideoThumbLoader : public CThumbLoader, public CJobQueue
{
public:
  CVideoThumbLoader();
  virtual ~CVideoThumbLoader();
  virtual bool LoadItem(CFileItem* pItem);
  void SetStreamDetailsObserver(IStreamDetailsObserver *pObs) { m_pStreamDetailsObs = pObs; }

  /*!
   \brief Callback from CThumbExtractor on completion of a generated image

   Performs the callbacks and updates the GUI.

   \sa CImageLoader, IJobCallback
   */
  virtual void OnJobComplete(unsigned int jobID, bool success, CJob *job);
  static void SetWatchedOverlay(CFileItem *item);

protected:
  virtual void OnLoaderStart() ;
  virtual void OnLoaderFinish() ;

  IStreamDetailsObserver *m_pStreamDetailsObs;
};

class CProgramThumbLoader : public CThumbLoader
{
public:
  CProgramThumbLoader();
  virtual ~CProgramThumbLoader();
  virtual bool LoadItem(CFileItem* pItem);

  /*! \brief Fill the thumb of a programs item
   First uses a cached thumb from a previous run, then checks for a local thumb
   and caches it for the next run
   \param item the CFileItem object to fill
   \return true if we fill the thumb, false otherwise
   \sa GetLocalThumb
   */
  static bool FillThumb(CFileItem &item);

  /*! \brief Get a local thumb for a programs item
   Shortcuts are checked, then we check for a file or folder thumb
   \param item the CFileItem object to check
   \return the local thumb (if it exists)
   \sa FillThumb
   */
  static CStdString GetLocalThumb(const CFileItem &item);
};

class CMusicThumbLoader : public CThumbLoader
{
public:
  CMusicThumbLoader();
  virtual ~CMusicThumbLoader();
  virtual bool LoadItem(CFileItem* pItem);
};
#endif
