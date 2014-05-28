#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#ifndef PICTURES_UTILS_STDSTRING_H_INCLUDED
#define PICTURES_UTILS_STDSTRING_H_INCLUDED
#include "utils/StdString.h"
#endif

#ifndef PICTURES_UTILS_JOBMANAGER_H_INCLUDED
#define PICTURES_UTILS_JOBMANAGER_H_INCLUDED
#include "utils/JobManager.h"
#endif

#ifndef PICTURES_THUMBLOADER_H_INCLUDED
#define PICTURES_THUMBLOADER_H_INCLUDED
#include "ThumbLoader.h"
#endif


class CPictureThumbLoader : public CThumbLoader, public CJobQueue
{
public:
  CPictureThumbLoader();
  virtual ~CPictureThumbLoader();

  virtual bool LoadItem(CFileItem* pItem);
  virtual bool LoadItemCached(CFileItem* pItem);
  virtual bool LoadItemLookup(CFileItem* pItem);
  void SetRegenerateThumbs(bool regenerate) { m_regenerateThumbs = regenerate; };
  static void ProcessFoldersAndArchives(CFileItem *pItem);

  /*!
   \brief Callback from CThumbExtractor on completion of a generated image

   Performs the callbacks and updates the GUI.

   \sa CImageLoader, IJobCallback
   */
  virtual void OnJobComplete(unsigned int jobID, bool success, CJob *job);

protected:
  virtual void OnLoaderFinish();

private:
  bool m_regenerateThumbs;
};
