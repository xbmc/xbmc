/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#pragma once

#include "Database.h"

class CTextureDatabase : public CDatabase
{
public:
  CTextureDatabase();
  virtual ~CTextureDatabase();
  virtual bool Open();

  bool GetCachedTexture(const CStdString &originalURL, CStdString &cacheFile, CStdString &imageHash);
  bool AddCachedTexture(const CStdString &originalURL, const CStdString &cachedFile, const CStdString &imageHash = "");
  bool ClearCachedTexture(const CStdString &originalURL, CStdString &cacheFile);

  /*! \brief Get a texture associated with the given path
   Used for retrieval of previously discovered (and cached) images to save
   stat() on the filesystem all the time
   \param url path that may be associated with a texture
   \return URL of the texture associated with the given path
   */
  CStdString GetTextureForPath(const CStdString &url);

  /*! \brief Set a texture associated with the given path
   Used for setting of previously discovered (and cached) images to save
   stat() on the filesystem all the time
   \param url path that was used to find the texture
   \param texture URL of the texture to associate with the path
   */
  void SetTextureForPath(const CStdString &url, const CStdString &texture);

protected:
  /*! \brief retrieve a hash for the given url
   Computes a hash of the current url to use for lookups in the database
   \param url url to hash
   \return a hash for this url
   */
  unsigned int GetURLHash(const CStdString &url) const;

  virtual bool CreateTables();
  virtual bool UpdateOldVersion(int version);
  virtual int GetMinVersion() const { return 6; };
  const char *GetDefaultDBName() const { return "Textures"; };
};
