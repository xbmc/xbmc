/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>
#include <unordered_map>

class CVideoDatabase;

namespace dbiplus
{
class Dataset;
}

namespace KODI::VIDEO
{

/*!
 \brief The actor table, held in memory

 CVideoDatabase::AddActor() matches names case-insensitively, which the actor table's index cannot
 serve, so every cast member, director and writer otherwise costs a scan of that table - tens of
 thousands of scans over a full scrape, against a table growing to a similar size. Reading the
 table once answers all of them from memory instead.

 While loaded the cache stands in for the whole table: a name it does not hold is taken to be new.
 Nothing else may add actors meanwhile, and it must be released before anything removes them.

 \sa CActorCacheScope, which is how a caller should hold one
 */
class CActorCache
{
public:
  struct Actor
  {
    int id{-1};
    std::string artUrls;
  };

  /*! \brief Read the actor table
   Failure leaves the cache unloaded rather than partial, so lookups simply go back to the database
   \param dataset the dataset to query on
   */
  void Load(dbiplus::Dataset* dataset);

  //! \brief Discard the cache, so that lookups go back to the database
  void Release();

  //! \brief Whether the cache is loaded and may be treated as the whole actor table
  bool IsLoaded() const { return m_loaded; }

  /*! \brief A name as the actor table holds it
   Trimmed and truncated the way CVideoDatabase::AddActor() writes it, so that a name looked up
   here matches the row it would have created
   \param name the actor's name, as scraped
   \return the name the actor table would hold
   */
  static std::string StoredName(const std::string& name);

  /*! \brief Find an actor by name
   \param storedName the name as the actor table holds it - see StoredName()
   \return the actor, or nullptr if the cache doesn't hold that name
   */
  const Actor* Find(const std::string& storedName) const;

  /*! \brief Record an actor, replacing anything held for that name
   \param storedName the name as the actor table holds it - trimmed and truncated
   \param id the actor's id
   \param artUrls the actor's art urls, as last written
   */
  void Set(const std::string& storedName, int id, const std::string& artUrls);

private:
  /*! \brief The key a name is matched on
   Folded to lower case in ASCII only, which is what the LIKE comparison it stands in for does
   */
  static std::string Key(const std::string& storedName);

  std::unordered_map<std::string, Actor> m_actors;
  bool m_loaded{false};
};

/*!
 \brief Holds a video database's actor cache for as long as the object lives

 Loading is worthwhile for a scan and wrong to leave in place afterwards, so ownership is explicit.
 */
class CActorCacheScope
{
public:
  explicit CActorCacheScope(CVideoDatabase& db);
  ~CActorCacheScope();

  CActorCacheScope(const CActorCacheScope&) = delete;
  CActorCacheScope& operator=(const CActorCacheScope&) = delete;

private:
  CVideoDatabase& m_db;
};

} // namespace KODI::VIDEO
