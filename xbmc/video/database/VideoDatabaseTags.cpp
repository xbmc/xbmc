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

#include "dbwrappers/dataset.h"
#include "FileItem.h"
#include "utils/log.h"

using namespace dbiplus;

int CVideoDatabase::AddTag(const std::string& name)
{
  if (name.empty())
    return -1;

  return AddToTable("tag", "tag_id", "name", name);
}

void CVideoDatabase::AddTagToItem(int media_id, int tag_id, const std::string &type)
{
  if (type.empty())
    return;

  AddToLinkTable(media_id, type, "tag", tag_id);
}

void CVideoDatabase::RemoveTagFromItem(int media_id, int tag_id, const std::string &type)
{
  if (type.empty())
    return;

  RemoveFromLinkTable(media_id, type, "tag", tag_id);
}

void CVideoDatabase::RemoveTagsFromItem(int media_id, const std::string &type)
{
  if (type.empty())
    return;

  m_pDS2->exec(PrepareSQL("DELETE FROM tag_link WHERE media_id=%d AND media_type='%s'", media_id, type.c_str()));
}

void CVideoDatabase::DeleteTag(int idTag, VIDEODB_CONTENT_TYPE mediaType)
{
  try
  {
    if (m_pDB.get() == NULL || m_pDS.get() == NULL)
      return;

    std::string type;
    if (mediaType == VIDEODB_CONTENT_MOVIES)
      type = MediaTypeMovie;
    else if (mediaType == VIDEODB_CONTENT_TVSHOWS)
      type = MediaTypeTvShow;
    else if (mediaType == VIDEODB_CONTENT_MUSICVIDEOS)
      type = MediaTypeMusicVideo;
    else
      return;

    std::string strSQL = PrepareSQL("DELETE FROM tag_link WHERE tag_id = %i AND media_type = '%s'", idTag, type.c_str());
    m_pDS->exec(strSQL);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%i) failed", __FUNCTION__, idTag);
  }
}

void CVideoDatabase::GetTags(int media_id, const std::string &media_type, std::vector<std::string> &tags)
{
  try
  {
    if (!m_pDB.get()) return;
    if (!m_pDS2.get()) return;

    std::string sql = PrepareSQL("SELECT tag.name FROM tag INNER JOIN tag_link ON tag_link.tag_id = tag.tag_id WHERE tag_link.media_id = %i AND tag_link.media_type = '%s' ORDER BY tag.tag_id", media_id, media_type.c_str());
    m_pDS2->query(sql);
    while (!m_pDS2->eof())
    {
      tags.emplace_back(m_pDS2->fv(0).get_asString());
      m_pDS2->next();
    }
    m_pDS2->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%i,%s) failed", __FUNCTION__, media_id, media_type.c_str());
  }
}

bool CVideoDatabase::GetTagsNav(const std::string& strBaseDir, CFileItemList& items, int idContent /* = -1 */, const Filter &filter /* = Filter() */, bool countOnly /* = false */)
{
  return GetNavCommon(strBaseDir, items, "tag", idContent, filter, countOnly);
}

std::string CVideoDatabase::GetTagById(int id)
{
  return GetSingleValue("tag", "name", PrepareSQL("tag_id = %i", id));
}
