/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoDatabase.h"

#include <string>
#include <vector>

#include "video/VideoInfoTag.h"
#include "dbwrappers/dataset.h"
#include "FileItem.h"
#include "GUIPassword.h"
#include "profiles/ProfileManager.h"
#include "ServiceBroker.h"
#include "settings/AdvancedSettings.h"
#include "settings/MediaSourceSettings.h"
#include "settings/SettingsComponent.h"
#include "threads/SystemClock.h"
#include "utils/log.h"

using namespace dbiplus;

extern DWORD movieTime;
extern DWORD castTime;

int CVideoDatabase::GetMusicVideoId(const std::string& strFilenameAndPath)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    int idFile = GetFileId(strFilenameAndPath);
    if (idFile < 0)
      return -1;

    std::string strSQL=PrepareSQL("select idMVideo from musicvideo where idFile=%i", idFile);

    CLog::Log(LOGDEBUG, LOGDATABASE, "%s (%s), query = %s", __FUNCTION__, CURL::GetRedacted(strFilenameAndPath).c_str(), strSQL.c_str());
    m_pDS->query(strSQL);
    int idMVideo=-1;
    if (m_pDS->num_rows() > 0)
      idMVideo = m_pDS->fv("idMVideo").get_asInt();
    m_pDS->close();

    return idMVideo;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
  return -1;
}

int CVideoDatabase::AddMusicVideo(const std::string& strFilenameAndPath)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    int idMVideo = GetMusicVideoId(strFilenameAndPath);
    if (idMVideo < 0)
    {
      int idFile = AddFile(strFilenameAndPath);
      if (idFile < 0)
        return -1;
      UpdateFileDateAdded(idFile, strFilenameAndPath);
      std::string strSQL=PrepareSQL("insert into musicvideo (idMVideo, idFile) values (NULL, %i)", idFile);
      m_pDS->exec(strSQL);
      idMVideo = (int)m_pDS->lastinsertid();
    }

    return idMVideo;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
  return -1;
}

bool CVideoDatabase::HasMusicVideoInfo(const std::string& strFilenameAndPath)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    int idMVideo = GetMusicVideoId(strFilenameAndPath);
    return (idMVideo > 0); // index of zero is also invalid
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
  return false;
}

void CVideoDatabase::GetMusicVideosByArtist(const std::string& strArtist, CFileItemList& items)
{
  try
  {
    items.Clear();
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    std::string strSQL;
    if (strArtist.empty())  //! @todo SMARTPLAYLISTS what is this here for???
      strSQL=PrepareSQL("select distinct * from musicvideo_view join actor_link on actor_link.media_id=musicvideo_view.idMVideo AND actor_link.media_type='musicvideo' join actor on actor.actor_id=actor_link.actor_id");
    else
      strSQL=PrepareSQL("select * from musicvideo_view join actor_link on actor_link.media_id=musicvideo_view.idMVideo AND actor_link.media_type='musicvideo' join actor on actor.actor_id=actor_link.actor_id where actor.name='%s'", strArtist.c_str());
    m_pDS->query( strSQL );

    while (!m_pDS->eof())
    {
      CVideoInfoTag tag = GetDetailsForMusicVideo(m_pDS);
      CFileItemPtr pItem(new CFileItem(tag));
      pItem->SetLabel(StringUtils::Join(tag.m_artist, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator));
      items.Add(pItem);
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strArtist.c_str());
  }
}

bool CVideoDatabase::GetMusicVideoInfo(const std::string& strFilenameAndPath, CVideoInfoTag& details, int idMVideo /* = -1 */, int getDetails /* = VideoDbDetailsAll */)
{
  try
  {
    if (m_pDB == nullptr || m_pDS == nullptr)
      return false;

    if (idMVideo < 0)
      idMVideo = GetMusicVideoId(strFilenameAndPath);
    if (idMVideo < 0) return false;

    std::string sql = PrepareSQL("select * from musicvideo_view where idMVideo=%i", idMVideo);
    if (!m_pDS->query(sql))
      return false;
    details = GetDetailsForMusicVideo(m_pDS, getDetails);
    return !details.IsEmpty();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
  return false;
}

int CVideoDatabase::SetDetailsForMusicVideo(const std::string& strFilenameAndPath, const CVideoInfoTag& details,
    const std::map<std::string, std::string> &artwork, int idMVideo /* = -1 */)
{
  try
  {
    BeginTransaction();

    if (idMVideo < 0)
      idMVideo = GetMusicVideoId(strFilenameAndPath);

    if (idMVideo > -1)
      DeleteMusicVideo(idMVideo, true); // Keep id
    else
    {
      // only add a new musicvideo if we don't already have a valid idMVideo
      // (DeleteMusicVideo is called with bKeepId == true so the musicvideo won't
      // be removed from the musicvideo table)
      idMVideo = AddMusicVideo(strFilenameAndPath);
      if (idMVideo < 0)
      {
        RollbackTransaction();
        return -1;
      }
    }

    // update dateadded if it's set
    if (details.m_dateAdded.IsValid())
    {
      int idFile = details.m_iFileId;
      if (idFile <= 0)
        idFile = GetFileId(strFilenameAndPath);

      UpdateFileDateAdded(idFile, strFilenameAndPath, details.m_dateAdded);
    }

    AddActorLinksToItem(idMVideo, MediaTypeMusicVideo, "actor", details.m_artist);
    AddActorLinksToItem(idMVideo, MediaTypeMusicVideo, "director", details.m_director);
    AddLinksToItem(idMVideo, MediaTypeMusicVideo, "genre", details.m_genre);
    AddLinksToItem(idMVideo, MediaTypeMusicVideo, "studio", details.m_studio);
    AddLinksToItem(idMVideo, MediaTypeMusicVideo, "tag", details.m_tags);

    if (details.HasStreamDetails())
      SetStreamDetailsForFileId(details.m_streamDetails, GetFileId(strFilenameAndPath));

    SetArtForItem(idMVideo, MediaTypeMusicVideo, artwork);

    // update our movie table (we know it was added already above)
    // and insert the new row
    std::string sql = "UPDATE musicvideo SET " + GetValueString(details, VIDEODB_ID_MUSICVIDEO_MIN, VIDEODB_ID_MUSICVIDEO_MAX, DbMusicVideoOffsets);
    if (details.m_iUserRating > 0 && details.m_iUserRating < 11)
      sql += PrepareSQL(", userrating = %i", details.m_iUserRating);
    else
      sql += ", userrating = NULL";
    if (details.HasPremiered())
      sql += PrepareSQL(", premiered = '%s'", details.GetPremiered().GetAsDBDate().c_str());
    else
      sql += PrepareSQL(", premiered = '%i'", details.GetYear());
    sql += PrepareSQL(" where idMVideo=%i", idMVideo);
    m_pDS->exec(sql);
    CommitTransaction();

    return idMVideo;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
  RollbackTransaction();
  return -1;
}

void CVideoDatabase::DeleteMusicVideo(const std::string& strFilenameAndPath, bool bKeepId /* = false */)
{
  int idMVideo = GetMusicVideoId(strFilenameAndPath);
  if (idMVideo > -1)
    DeleteMusicVideo(idMVideo, bKeepId);
}

void CVideoDatabase::DeleteMusicVideo(int idMVideo, bool bKeepId /* = false */)
{
  if (idMVideo < 0)
    return;

  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    BeginTransaction();

    // keep the music video table entry and bookmarks so we can update data in place
    // the ancillary tables are still purged
    if (!bKeepId)
    {
      int idFile = GetDbId(PrepareSQL("SELECT idFile FROM musicvideo WHERE idMVideo=%i", idMVideo));
      std::string path = GetSingleValue(PrepareSQL("SELECT strPath FROM path JOIN files ON files.idPath=path.idPath WHERE files.idFile=%i", idFile));
      if (!path.empty())
        InvalidatePathHash(path);

      std::string strSQL = PrepareSQL("delete from musicvideo where idMVideo=%i", idMVideo);
      m_pDS->exec(strSQL);
    }

    //! @todo move this below CommitTransaction() once UPnP doesn't rely on this anymore
    if (!bKeepId)
      AnnounceRemove(MediaTypeMusicVideo, idMVideo);

    CommitTransaction();

  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
    RollbackTransaction();
  }
}

CVideoInfoTag CVideoDatabase::GetDetailsForMusicVideo(std::unique_ptr<Dataset> &pDS, int getDetails /* = VideoDbDetailsNone */)
{
  return GetDetailsForMusicVideo(pDS->get_sql_record(), getDetails);
}

CVideoInfoTag CVideoDatabase::GetDetailsForMusicVideo(const dbiplus::sql_record* const record, int getDetails /* = VideoDbDetailsNone */)
{
  CVideoInfoTag details;

  if (record == nullptr)
    return details;

  unsigned int time = XbmcThreads::SystemClockMillis();
  int idMVideo = record->at(0).get_asInt();

  GetDetailsFromDB(record, VIDEODB_ID_MUSICVIDEO_MIN, VIDEODB_ID_MUSICVIDEO_MAX, DbMusicVideoOffsets, details);
  details.m_iDbId = idMVideo;
  details.m_type = MediaTypeMusicVideo;

  details.m_iFileId = record->at(VIDEODB_DETAILS_FILEID).get_asInt();
  details.m_strPath = record->at(VIDEODB_DETAILS_MUSICVIDEO_PATH).get_asString();
  std::string strFileName = record->at(VIDEODB_DETAILS_MUSICVIDEO_FILE).get_asString();
  ConstructPath(details.m_strFileNameAndPath,details.m_strPath,strFileName);
  details.SetPlayCount(record->at(VIDEODB_DETAILS_MUSICVIDEO_PLAYCOUNT).get_asInt());
  details.m_lastPlayed.SetFromDBDateTime(record->at(VIDEODB_DETAILS_MUSICVIDEO_LASTPLAYED).get_asString());
  details.m_dateAdded.SetFromDBDateTime(record->at(VIDEODB_DETAILS_MUSICVIDEO_DATEADDED).get_asString());
  details.SetResumePoint(record->at(VIDEODB_DETAILS_MUSICVIDEO_RESUME_TIME).get_asInt(),
                         record->at(VIDEODB_DETAILS_MUSICVIDEO_TOTAL_TIME).get_asInt(),
                         record->at(VIDEODB_DETAILS_MUSICVIDEO_PLAYER_STATE).get_asString());
  details.m_iUserRating = record->at(VIDEODB_DETAILS_MUSICVIDEO_USER_RATING).get_asInt();
  std::string premieredString = record->at(VIDEODB_DETAILS_MUSICVIDEO_PREMIERED).get_asString();
  if (premieredString.size() == 4)
    details.SetYear(record->at(VIDEODB_DETAILS_MUSICVIDEO_PREMIERED).get_asInt());
  else
    details.SetPremieredFromDBDate(premieredString);

  movieTime += XbmcThreads::SystemClockMillis() - time; time = XbmcThreads::SystemClockMillis();

  if (getDetails)
  {
    if (getDetails & VideoDbDetailsTag)
      GetTags(details.m_iDbId, MediaTypeMusicVideo, details.m_tags);

    details.m_strPictureURL.Parse();

    if (getDetails & VideoDbDetailsStream)
      GetStreamDetails(details);

    details.m_parsedDetails = getDetails;
  }
  return details;
}

bool CVideoDatabase::GetMusicVideoAlbumsNav(const std::string& strBaseDir, CFileItemList& items, int idArtist /* = -1 */, const Filter &filter /* = Filter() */, bool countOnly /* = false */)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CVideoDbUrl videoUrl;
    if (!videoUrl.FromString(strBaseDir))
      return false;

    std::string strSQL = "select %s from musicvideo_view ";
    Filter extFilter = filter;
    extFilter.fields = PrepareSQL("musicvideo_view.c%02d, musicvideo_view.idMVideo, actor.name", VIDEODB_ID_MUSICVIDEO_ALBUM);
    extFilter.AppendJoin(PrepareSQL("JOIN actor_link ON actor_link.media_id=musicvideo_view.idMVideo AND actor_link.media_type='musicvideo'"));
    extFilter.AppendJoin(PrepareSQL("JOIN actor ON actor.actor_id = actor_link.actor_id"));
    if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
    {
      extFilter.fields += ", path.strPath";
      extFilter.AppendJoin("join files on files.idFile = musicvideo_view.idFile join path on path.idPath = files.idPath");
    }

    if (idArtist > -1)
      videoUrl.AddOption("artistid", idArtist);

    extFilter.AppendGroup(PrepareSQL("musicvideo_view.c%02d", VIDEODB_ID_MUSICVIDEO_ALBUM));

    if (countOnly)
    {
      extFilter.fields = "COUNT(1)";
      extFilter.group.clear();
      extFilter.order.clear();
    }
    strSQL = StringUtils::Format(strSQL.c_str(), !extFilter.fields.empty() ? extFilter.fields.c_str() : "*");

    if (!BuildSQL(videoUrl.ToString(), strSQL, extFilter, strSQL, videoUrl))
      return false;

    int iRowsFound = RunQuery(strSQL);
    if (iRowsFound <= 0)
      return iRowsFound == 0;

    if (countOnly)
    {
      CFileItemPtr pItem(new CFileItem());
      pItem->SetProperty("total", iRowsFound == 1 ? m_pDS->fv(0).get_asInt() : iRowsFound);
      items.Add(pItem);

      m_pDS->close();
      return true;
    }

    if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
    {
      std::map<int, std::pair<std::string,std::string> > mapAlbums;
      while (!m_pDS->eof())
      {
        int lidMVideo = m_pDS->fv(1).get_asInt();
        std::string strAlbum = m_pDS->fv(0).get_asString();
        auto it = mapAlbums.find(lidMVideo);
        // was this genre already found?
        if (it == mapAlbums.end())
        {
          // check path
          if (g_passwordManager.IsDatabasePathUnlocked(m_pDS->fv("path.strPath").get_asString(),*CMediaSourceSettings::GetInstance().GetSources("video")))
            mapAlbums.insert(make_pair(lidMVideo, make_pair(strAlbum,m_pDS->fv(2).get_asString())));
        }
        m_pDS->next();
      }
      m_pDS->close();

      for (const auto &i : mapAlbums)
      {
        if (!i.second.first.empty())
        {
          CFileItemPtr pItem(new CFileItem(i.second.first));

          CVideoDbUrl itemUrl = videoUrl;
          std::string path = StringUtils::Format("%i/", i.first);
          itemUrl.AppendPath(path);
          pItem->SetPath(itemUrl.ToString());

          pItem->m_bIsFolder=true;
          pItem->SetLabelPreformatted(true);
          if (!items.Contains(pItem->GetPath()))
          {
            pItem->GetVideoInfoTag()->m_artist.push_back(i.second.second);
            items.Add(pItem);
          }
        }
      }
    }
    else
    {
      while (!m_pDS->eof())
      {
        if (!m_pDS->fv(0).get_asString().empty())
        {
          CFileItemPtr pItem(new CFileItem(m_pDS->fv(0).get_asString()));

          CVideoDbUrl itemUrl = videoUrl;
          std::string path = StringUtils::Format("%i/", m_pDS->fv(1).get_asInt());
          itemUrl.AppendPath(path);
          pItem->SetPath(itemUrl.ToString());

          pItem->m_bIsFolder=true;
          pItem->SetLabelPreformatted(true);
          if (!items.Contains(pItem->GetPath()))
          {
            pItem->GetVideoInfoTag()->m_artist.emplace_back(m_pDS->fv(2).get_asString());
            items.Add(pItem);
          }
        }
        m_pDS->next();
      }
      m_pDS->close();
    }

//    CLog::Log(LOGDEBUG, __FUNCTION__" Time: %d ms", XbmcThreads::SystemClockMillis() - time);
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CVideoDatabase::GetMusicVideosNav(const std::string& strBaseDir, CFileItemList& items, int idGenre, int idYear, int idArtist, int idDirector, int idStudio, int idAlbum, int idTag /* = -1 */, const SortDescription &sortDescription /* = SortDescription() */, int getDetails /* = VideoDbDetailsNone */)
{
  CVideoDbUrl videoUrl;
  if (!videoUrl.FromString(strBaseDir))
    return false;

  if (idGenre != -1)
    videoUrl.AddOption("genreid", idGenre);
  else if (idStudio != -1)
    videoUrl.AddOption("studioid", idStudio);
  else if (idDirector != -1)
    videoUrl.AddOption("directorid", idDirector);
  else if (idYear !=-1)
    videoUrl.AddOption("year", idYear);
  else if (idArtist != -1)
    videoUrl.AddOption("artistid", idArtist);
  else if (idTag != -1)
    videoUrl.AddOption("tagid", idTag);
  if (idAlbum != -1)
    videoUrl.AddOption("albumid", idAlbum);

  Filter filter;
  return GetMusicVideosByWhere(videoUrl.ToString(), filter, items, true, sortDescription, getDetails);
}

bool CVideoDatabase::GetRecentlyAddedMusicVideosNav(const std::string& strBaseDir, CFileItemList& items, unsigned int limit /* = 0 */, int getDetails /* = VideoDbDetailsNone */)
{
  Filter filter;
  filter.order = "dateAdded desc, idMVideo desc";
  filter.limit = PrepareSQL("%u", limit ? limit : CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_iVideoLibraryRecentlyAddedItems);
  return GetMusicVideosByWhere(strBaseDir, filter, items, true, SortDescription(), getDetails);
}

std::string CVideoDatabase::GetMusicVideoAlbumById(int id)
{
  return GetSingleValue("musicvideo", PrepareSQL("c%02d", VIDEODB_ID_MUSICVIDEO_ALBUM), PrepareSQL("idMVideo=%i", id));
}

void CVideoDatabase::GetMusicVideoArtistsByName(const std::string& strSearch, CFileItemList& items)
{
  std::string strSQL;

  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    std::string strLike;
    if (!strSearch.empty())
      strLike = "and actor.name like '%%%s%%'";
    if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL=PrepareSQL("SELECT actor.actor_id, actor.name, path.strPath FROM actor INNER JOIN actor_link ON actor_link.actor_id=actor.actor_id INNER JOIN musicvideo ON actor_link.media_id=musicvideo.idMVideo INNER JOIN files ON files.idFile=musicvideo.idFile INNER JOIN path ON path.idPath=files.idPath WHERE actor_link.media_type='musicvideo' "+strLike, strSearch.c_str());
    else
      strSQL=PrepareSQL("SELECT DISTINCT actor.actor_id, actor.name from actor INNER JOIN actor_link ON actor_link.actor_id=actor.actor_id WHERE actor_link.media_type='musicvideo' "+strLike,strSearch.c_str());
    m_pDS->query( strSQL );

    while (!m_pDS->eof())
    {
      if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(m_pDS->fv("path.strPath").get_asString(),*CMediaSourceSettings::GetInstance().GetSources("video")))
        {
          m_pDS->next();
          continue;
        }

      CFileItemPtr pItem(new CFileItem(m_pDS->fv(1).get_asString()));
      std::string strDir = StringUtils::Format("%i/", m_pDS->fv(0).get_asInt());
      pItem->SetPath("videodb://musicvideos/artists/"+ strDir);
      pItem->m_bIsFolder=true;
      items.Add(pItem);
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strSQL.c_str());
  }
}

void CVideoDatabase::GetMusicVideoGenresByName(const std::string& strSearch, CFileItemList& items)
{
  std::string strSQL;

  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL=PrepareSQL("SELECT genre.genre_id, genre.name, path.strPath FROM genre INNER JOIN genre_link ON genre_link.genre_id=genre.genre_id INNER JOIN musicvideo ON genre_link.media_id=musicvideo.idMVideo INNER JOIN files ON files.idFile=musicvideo.idFile INNER JOIN path ON path.idPath=files.idPath WHERE genre_link.media_type='musicvideo' AND genre.name LIKE '%%%s%%'", strSearch.c_str());
    else
      strSQL=PrepareSQL("SELECT DISTINCT genre.genre_id, genre.name FROM genre INNER JOIN genre_link ON genre_link.genre_id=genre.genre_id WHERE genre_link.media_type='musicvideo' AND genre.name LIKE '%%%s%%'", strSearch.c_str());
    m_pDS->query( strSQL );

    while (!m_pDS->eof())
    {
      if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(m_pDS->fv("path.strPath").get_asString(),*CMediaSourceSettings::GetInstance().GetSources("video")))
        {
          m_pDS->next();
          continue;
        }

      CFileItemPtr pItem(new CFileItem(m_pDS->fv(1).get_asString()));
      std::string strDir = StringUtils::Format("%i/", m_pDS->fv(0).get_asInt());
      pItem->SetPath("videodb://musicvideos/genres/"+ strDir);
      pItem->m_bIsFolder=true;
      items.Add(pItem);
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strSQL.c_str());
  }
}

void CVideoDatabase::GetMusicVideoAlbumsByName(const std::string& strSearch, CFileItemList& items)
{
  std::string strSQL;

  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    strSQL = StringUtils::Format("SELECT DISTINCT"
                                 "  musicvideo.c%02d,"
                                 "  musicvideo.idMVideo,"
                                 "  path.strPath"
                                 " FROM"
                                 "  musicvideo"
                                 " JOIN files ON"
                                 "  files.idFile=musicvideo.idFile"
                                 " JOIN path ON"
                                 "  path.idPath=files.idPath", VIDEODB_ID_MUSICVIDEO_ALBUM);
    if (!strSearch.empty())
      strSQL += PrepareSQL(" WHERE musicvideo.c%02d like '%%%s%%'",VIDEODB_ID_MUSICVIDEO_ALBUM, strSearch.c_str());

    m_pDS->query( strSQL );

    while (!m_pDS->eof())
    {
      if (m_pDS->fv(0).get_asString().empty())
      {
        m_pDS->next();
        continue;
      }

      if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(m_pDS->fv(2).get_asString(),*CMediaSourceSettings::GetInstance().GetSources("video")))
        {
          m_pDS->next();
          continue;
        }

      CFileItemPtr pItem(new CFileItem(m_pDS->fv(0).get_asString()));
      std::string strDir = StringUtils::Format("%i", m_pDS->fv(1).get_asInt());
      pItem->SetPath("videodb://musicvideos/titles/"+ strDir);
      pItem->m_bIsFolder=false;
      items.Add(pItem);
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strSQL.c_str());
  }
}

void CVideoDatabase::GetMusicVideosByAlbum(const std::string& strSearch, CFileItemList& items)
{
  std::string strSQL;

  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL = PrepareSQL("SELECT musicvideo.idMVideo, musicvideo.c%02d,musicvideo.c%02d, path.strPath FROM musicvideo INNER JOIN files ON files.idFile=musicvideo.idFile INNER JOIN path ON path.idPath=files.idPath WHERE musicvideo.c%02d LIKE '%%%s%%'", VIDEODB_ID_MUSICVIDEO_ALBUM, VIDEODB_ID_MUSICVIDEO_TITLE, VIDEODB_ID_MUSICVIDEO_ALBUM, strSearch.c_str());
    else
      strSQL = PrepareSQL("select musicvideo.idMVideo,musicvideo.c%02d,musicvideo.c%02d from musicvideo where musicvideo.c%02d like '%%%s%%'",VIDEODB_ID_MUSICVIDEO_ALBUM,VIDEODB_ID_MUSICVIDEO_TITLE,VIDEODB_ID_MUSICVIDEO_ALBUM,strSearch.c_str());
    m_pDS->query( strSQL );

    while (!m_pDS->eof())
    {
      if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(m_pDS->fv("path.strPath").get_asString(),*CMediaSourceSettings::GetInstance().GetSources("video")))
        {
          m_pDS->next();
          continue;
        }

      CFileItemPtr pItem(new CFileItem(m_pDS->fv(1).get_asString()+" - "+m_pDS->fv(2).get_asString()));
      std::string strDir = StringUtils::Format("3/2/%i",m_pDS->fv("musicvideo.idMVideo").get_asInt());

      pItem->SetPath("videodb://"+ strDir);
      pItem->m_bIsFolder=false;
      items.Add(pItem);
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strSQL.c_str());
  }
}

bool CVideoDatabase::GetMusicVideosByWhere(const std::string &baseDir, const Filter &filter, CFileItemList &items, bool checkLocks /*= true*/, const SortDescription &sortDescription /* = SortDescription() */, int getDetails /* = VideoDbDetailsNone */)
{
  try
  {
    movieTime = 0;
    castTime = 0;

    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    int total = -1;

    std::string strSQL = "select %s from musicvideo_view ";
    CVideoDbUrl videoUrl;
    std::string strSQLExtra;
    Filter extFilter = filter;
    SortDescription sorting = sortDescription;
    if (!BuildSQL(baseDir, strSQLExtra, extFilter, strSQLExtra, videoUrl, sorting))
      return false;

    // Apply the limiting directly here if there's no special sorting but limiting
    if (extFilter.limit.empty() &&
      sorting.sortBy == SortByNone &&
      (sorting.limitStart > 0 || sorting.limitEnd > 0))
    {
      total = (int)strtol(GetSingleValue(PrepareSQL(strSQL, "COUNT(1)") + strSQLExtra, m_pDS).c_str(), NULL, 10);
      strSQLExtra += DatabaseUtils::BuildLimitClause(sorting.limitEnd, sorting.limitStart);
    }

    strSQL = PrepareSQL(strSQL, !extFilter.fields.empty() ? extFilter.fields.c_str() : "*") + strSQLExtra;

    int iRowsFound = RunQuery(strSQL);
    if (iRowsFound <= 0)
      return iRowsFound == 0;

    // store the total value of items as a property
    if (total < iRowsFound)
      total = iRowsFound;
    items.SetProperty("total", total);

    DatabaseResults results;
    results.reserve(iRowsFound);
    if (!SortUtils::SortFromDataset(sorting, MediaTypeMusicVideo, m_pDS, results))
      return false;

    // get data from returned rows
    items.Reserve(results.size());
    // get songs from returned subtable
    const query_data &data = m_pDS->get_result_set().records;
    for (const auto &i : results)
    {
      unsigned int targetRow = (unsigned int)i.at(FieldRow).asInteger();
      const dbiplus::sql_record* const record = data.at(targetRow);

      CVideoInfoTag musicvideo = GetDetailsForMusicVideo(record, getDetails);
      if (!checkLocks || m_profileManager.GetMasterProfile().getLockMode() == LOCK_MODE_EVERYONE || g_passwordManager.bMasterUser ||
          g_passwordManager.IsDatabasePathUnlocked(musicvideo.m_strPath, *CMediaSourceSettings::GetInstance().GetSources("video")))
      {
        CFileItemPtr item(new CFileItem(musicvideo));

        CVideoDbUrl itemUrl = videoUrl;
        std::string path = StringUtils::Format("%i", record->at(0).get_asInt());
        itemUrl.AppendPath(path);
        item->SetPath(itemUrl.ToString());

        item->SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED, musicvideo.GetPlayCount() > 0);
        items.Add(item);
      }
    }

    // cleanup
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

unsigned int CVideoDatabase::GetRandomMusicVideoIDs(const std::string& strWhere, std::vector<std::pair<int,int> > &songIDs)
{
  try
  {
    if (NULL == m_pDB.get()) return 0;
    if (NULL == m_pDS.get()) return 0;

    std::string strSQL = "select distinct idMVideo from musicvideo_view";
    if (!strWhere.empty())
      strSQL += " where " + strWhere;
    strSQL += PrepareSQL(" ORDER BY RANDOM()");

    if (!m_pDS->query(strSQL)) return 0;
    songIDs.clear();
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      return 0;
    }
    songIDs.reserve(m_pDS->num_rows());
    while (!m_pDS->eof())
    {
      songIDs.push_back(std::make_pair<int,int>(2,m_pDS->fv(0).get_asInt()));
      m_pDS->next();
    }    // cleanup
    m_pDS->close();
    return songIDs.size();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strWhere.c_str());
  }
  return 0;
}

int CVideoDatabase::GetMatchingMusicVideo(const std::string& strArtist, const std::string& strAlbum, const std::string& strTitle)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    std::string strSQL;
    if (strAlbum.empty() && strTitle.empty())
    { // we want to return matching artists only
      if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        strSQL=PrepareSQL("SELECT DISTINCT actor.actor_id, path.strPath FROM actor INNER JOIN actor_link ON actor_link.actor_id=actor.actor_id INNER JOIN musicvideo ON actor_link.media_id=musicvideo.idMVideo INNER JOIN files ON files.idFile=musicvideo.idFile INNER JOIN path ON path.idPath=files.idPath WHERE actor_link.media_type='musicvideo' AND actor.name like '%s'", strArtist.c_str());
      else
        strSQL=PrepareSQL("SELECT DISTINCT actor.actor_id FROM actor INNER JOIN actor_link ON actor_link.actor_id=actor.actor_id WHERE actor_link.media_type='musicvideo' AND actor.name LIKE '%s'", strArtist.c_str());
    }
    else
    { // we want to return the matching musicvideo
      if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        strSQL = PrepareSQL("SELECT musicvideo.idMVideo FROM actor INNER JOIN actor_link ON actor_link.actor_id=actor.actor_id INNER JOIN musicvideo ON actor_link.media_id=musicvideo.idMVideo INNER JOIN files ON files.idFile=musicvideo.idFile INNER JOIN path ON path.idPath=files.idPath WHERE actor_link.media_type='musicvideo' AND musicvideo.c%02d LIKE '%s' AND musicvideo.c%02d LIKE '%s' AND actor.name LIKE '%s'", VIDEODB_ID_MUSICVIDEO_ALBUM, strAlbum.c_str(), VIDEODB_ID_MUSICVIDEO_TITLE, strTitle.c_str(), strArtist.c_str());
      else
        strSQL = PrepareSQL("select musicvideo.idMVideo from musicvideo join actor_link on actor_link.media_id=musicvideo.idMVideo AND actor_link.media_type='musicvideo' join actor on actor.actor_id=actor_link.actor_id where musicvideo.c%02d like '%s' and musicvideo.c%02d like '%s' and actor.name like '%s'",VIDEODB_ID_MUSICVIDEO_ALBUM,strAlbum.c_str(),VIDEODB_ID_MUSICVIDEO_TITLE,strTitle.c_str(),strArtist.c_str());
    }
    m_pDS->query( strSQL );

    if (m_pDS->eof())
      return -1;

    if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      if (!g_passwordManager.IsDatabasePathUnlocked(m_pDS->fv("path.strPath").get_asString(),*CMediaSourceSettings::GetInstance().GetSources("video")))
      {
        m_pDS->close();
        return -1;
      }

    int lResult = m_pDS->fv(0).get_asInt();
    m_pDS->close();
    return lResult;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return -1;
}

void CVideoDatabase::GetMusicVideosByName(const std::string& strSearch, CFileItemList& items)
{
// Alternative searching - not quite as fast though due to
// retrieving all information
//  Filter filter(PrepareSQL("c%02d like '%s%%' or c%02d like '%% %s%%'", VIDEODB_ID_MUSICVIDEO_TITLE, strSearch.c_str(), VIDEODB_ID_MUSICVIDEO_TITLE, strSearch.c_str()));
//  GetMusicVideosByWhere("videodb://musicvideos/titles/", filter, items);
  std::string strSQL;

  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL = PrepareSQL("SELECT musicvideo.idMVideo, musicvideo.c%02d, path.strPath FROM musicvideo INNER JOIN files ON files.idFile=musicvideo.idFile INNER JOIN path ON path.idPath=files.idPath WHERE musicvideo.c%02d LIKE '%%%s%%'", VIDEODB_ID_MUSICVIDEO_TITLE, VIDEODB_ID_MUSICVIDEO_TITLE, strSearch.c_str());
    else
      strSQL = PrepareSQL("select musicvideo.idMVideo,musicvideo.c%02d from musicvideo where musicvideo.c%02d like '%%%s%%'",VIDEODB_ID_MUSICVIDEO_TITLE,VIDEODB_ID_MUSICVIDEO_TITLE,strSearch.c_str());
    m_pDS->query( strSQL );

    while (!m_pDS->eof())
    {
      if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(m_pDS->fv("path.strPath").get_asString(),*CMediaSourceSettings::GetInstance().GetSources("video")))
        {
          m_pDS->next();
          continue;
        }

      CFileItemPtr pItem(new CFileItem(m_pDS->fv(1).get_asString()));
      std::string strDir = StringUtils::Format("3/2/%i",m_pDS->fv("musicvideo.idMVideo").get_asInt());

      pItem->SetPath("videodb://"+ strDir);
      pItem->m_bIsFolder=false;
      items.Add(pItem);
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strSQL.c_str());
  }
}

void CVideoDatabase::GetMusicVideoDirectorsByName(const std::string& strSearch, CFileItemList& items)
{
  std::string strSQL;

  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL = PrepareSQL("SELECT DISTINCT director_link.actor_id, actor.name, path.strPath FROM actor INNER JOIN director_link ON director_link.actor_id=actor.actor_id INNER JOIN musicvideo ON director_link.media_id=musicvideo.idMVideo INNER JOIN files ON files.idFile=musicvideo.idFile INNER JOIN path ON path.idPath=files.idPath WHERE director_link.media_type='musicvideo' AND actor.name LIKE '%%%s%%'", strSearch.c_str());
    else
      strSQL = PrepareSQL("SELECT DISTINCT director_link.actor_id, actor.name FROM actor INNER JOIN director_link ON director_link.actor_id=actor.actor_id INNER JOIN musicvideo ON director_link.media_id=musicvideo.idMVideo WHERE director_link.media_type='musicvideo' AND actor.name LIKE '%%%s%%'", strSearch.c_str());

    m_pDS->query( strSQL );

    while (!m_pDS->eof())
    {
      if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(m_pDS->fv("path.strPath").get_asString(),*CMediaSourceSettings::GetInstance().GetSources("video")))
        {
          m_pDS->next();
          continue;
        }

      std::string strDir = StringUtils::Format("%i/", m_pDS->fv(0).get_asInt());
      CFileItemPtr pItem(new CFileItem(m_pDS->fv(1).get_asString()));

      pItem->SetPath("videodb://musicvideos/albums/"+ strDir);
      pItem->m_bIsFolder=true;
      items.Add(pItem);
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strSQL.c_str());
  }
}
