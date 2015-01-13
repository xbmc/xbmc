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

#include <string>
#include "threads/Thread.h"
#include "TextureDatabase.h"
#include <string>

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
   \param originalUrl [in/out] the url that we think the oldCachedFile came from. May be set if it's empty and an oldCachedFile exists.
   \param oldCachedFile the old cached file
   \param label the label of the item for logging
   \param type [out] the type of art (poster/banner/thumb)
   */
  bool CacheTexture(std::string &originalUrl, const std::string &cachedFile, const std::string &label, std::string &type);
  bool CacheTexture(std::string &originalUrl, const std::string &oldCachedFile, const std::string &label);

  std::string GetCachedActorThumb(const CFileItem &item);
  std::string GetCachedSeasonThumb(int season, const std::string &path);
  std::string GetCachedEpisodeThumb(const CFileItem &item);
  std::string GetCachedVideoThumb(const CFileItem &item);
  std::string GetCachedFanart(const CFileItem &item);
  std::string GetThumb(const std::string &path, const std::string &path2, bool split /* = false */);

  CTextureDatabase m_textureDB;
};
