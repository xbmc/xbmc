/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "TextureCacheJob.h"
#include "dbwrappers/Database.h"
#include "dbwrappers/DatabaseQuery.h"

#include <string>
#include <vector>

class CVariant;

class CTextureRule : public CDatabaseQueryRule
{
public:
  CTextureRule() = default;
  ~CTextureRule() override = default;

  static void GetAvailableFields(std::vector<std::string> &fieldList);
protected:
  int                 TranslateField(const char *field) const override;
  std::string         TranslateField(int field) const override;
  std::string         GetField(int field, const std::string& type) const override;
  FIELD_TYPE          GetFieldType(int field) const override;
  std::string         FormatParameter(const std::string &negate,
                                              const std::string &oper,
                                              const CDatabase &db,
                                              const std::string &type) const override;
};

class CTextureUtils
{
public:
  /*! \brief retrieve a wrapped URL for a image file
   \param image name of the file
   \param type signifies a special type of image (eg embedded video thumb, picture folder thumb)
   \param options which options we need (eg size=thumb)
   \return full wrapped URL of the image file
   */
  static std::string GetWrappedImageURL(const std::string &image, const std::string &type = "", const std::string &options = "");
  static std::string GetWrappedThumbURL(const std::string &image);

  /*! \brief Unwrap an image://<url_encoded_path> style URL
   Such urls are used for art over the webserver or other users of the VFS
   \param image url of the image
   \return the unwrapped URL, or the original URL if unwrapping is inappropriate.
   */
  static std::string UnwrapImageURL(const std::string &image);
};

class CTextureDatabase : public CDatabase, public IDatabaseQueryRuleFactory
{
public:
  CTextureDatabase();
  ~CTextureDatabase() override;
  bool Open() override;

  bool GetCachedTexture(const std::string &originalURL, CTextureDetails &details);
  bool AddCachedTexture(const std::string &originalURL, const CTextureDetails &details);
  bool SetCachedTextureValid(const std::string &originalURL, bool updateable);
  bool ClearCachedTexture(const std::string &originalURL, std::string &cacheFile);
  bool ClearCachedTexture(int textureID, std::string &cacheFile);
  bool IncrementUseCount(const CTextureDetails &details);

  /*! \brief Invalidate a previously cached texture
   Invalidates the texture hash, and sets the texture update time to the current time so that
   next texture load it will be re-cached.
   \param url texture path
   */
  bool InvalidateCachedTexture(const std::string &originalURL);

  /*! \brief Get a texture associated with the given path
   Used for retrieval of previously discovered images to save
   stat() on the filesystem all the time
   \param url path that may be associated with a texture
   \param type type of image to look for
   \return URL of the texture associated with the given path
   */
  std::string GetTextureForPath(const std::string &url, const std::string &type);

  /*! \brief Set a texture associated with the given path
   Used for setting of previously discovered images to save
   stat() on the filesystem all the time. Should be used to set
   the actual image path, not the cached image path (the image will be
   cached at load time.)
   \param url path that was used to find the texture
   \param type type of image to associate
   \param texture URL of the texture to associate with the path
   */
  void SetTextureForPath(const std::string &url, const std::string &type, const std::string &texture);

  /*! \brief Clear a texture associated with the given path
   \param url path that was used to find the texture
   \param type type of image to associate
   \param texture URL of the texture to associate with the path
   \sa GetTextureForPath, SetTextureForPath
   */
  void ClearTextureForPath(const std::string &url, const std::string &type);

  bool GetTextures(CVariant &items, const Filter &filter);

  // rule creation
  CDatabaseQueryRule *CreateRule() const override;
  CDatabaseQueryRuleCombination *CreateCombination() const override;
protected:
  /*! \brief retrieve a hash for the given url
   Computes a hash of the current url to use for lookups in the database
   \param url url to hash
   \return a hash for this url
   */
  unsigned int GetURLHash(const std::string &url) const;

  void CreateTables() override;
  void CreateAnalytics() override;
  void UpdateTables(int version) override;
  int GetSchemaVersion() const override { return 13; };
  const char *GetBaseDBName() const override { return "Textures"; };
};
