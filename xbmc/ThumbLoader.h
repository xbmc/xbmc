#pragma once
/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "BackgroundInfoLoader.h"
#include "utils/StdString.h"

class CThumbLoader : public CBackgroundInfoLoader
{
public:
  CThumbLoader(int nThreads=-1);
  virtual ~CThumbLoader();

  virtual void Initialize() { };

  /*! \brief helper function to fill the art for a library item
   \param item a CFileItem
   \return true if we fill art, false otherwise
   */
  virtual bool FillLibraryArt(CFileItem &item) { return false; }

  /*! \brief Checks whether the given item has an image listed in the texture database
   \param item CFileItem to check
   \param type the type of image to retrieve
   \return the image associated with this item
   */
  static CStdString GetCachedImage(const CFileItem &item, const CStdString &type);

  /*! \brief Associate an image with the given item in the texture database
   \param item CFileItem to associate the image with
   \param type the type of image
   \param image the URL of the image
   */
  static void SetCachedImage(const CFileItem &item, const CStdString &type, const CStdString &image);
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
