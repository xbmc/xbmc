#pragma once
/*
 *      Copyright (C) 2012 Team XBMC
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

#include <string>
#include "threads/Thread.h"
#include "TextureDatabase.h"

class CFileItem;

class CEdenVideoArtUpdater : CThread
{
public:
  CEdenVideoArtUpdater();
  ~CEdenVideoArtUpdater();

  static void Start();

  virtual void Process();

private:
  /*! \brief Caches the texture from oldCachedFile as if it came from originalUrl into the texture cache.
   \param originalUrl the url that we think the oldCachedFile came from.
   \param oldCachedFile the old cached file
   \param type [out] the type of art (poster/banner/thumb)
   */
  bool CacheTexture(const std::string &originalUrl, const std::string &cachedFile, std::string &type);
  bool CacheTexture(const std::string &originalUrl, const std::string &oldCachedFile);

  CStdString GetCachedActorThumb(const CFileItem &item);
  CStdString GetCachedSeasonThumb(int season, const CStdString &path);
  CStdString GetCachedEpisodeThumb(const CFileItem &item);
  CStdString GetCachedVideoThumb(const CFileItem &item);
  CStdString GetCachedFanart(const CFileItem &item);
  CStdString GetThumb(const CStdString &path, const CStdString &path2, bool split /* = false */);

  CTextureDatabase m_textureDB;
};
