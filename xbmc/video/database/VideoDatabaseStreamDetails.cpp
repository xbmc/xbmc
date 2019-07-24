/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoDatabase.h"

#include <memory>
#include <string>
#include <vector>

#include "video/VideoInfoTag.h"
#include "dbwrappers/dataset.h"
#include "FileItem.h"
#include "utils/log.h"

using namespace dbiplus;

void CVideoDatabase::SetStreamDetailsForFile(const CStreamDetails& details, const std::string &strFileNameAndPath)
{
  // AddFile checks to make sure the file isn't already in the DB first
  int idFile = AddFile(strFileNameAndPath);
  if (idFile < 0)
    return;
  SetStreamDetailsForFileId(details, idFile);
}

void CVideoDatabase::SetStreamDetailsForFileId(const CStreamDetails& details, int idFile)
{
  if (idFile < 0)
    return;

  try
  {
    BeginTransaction();
    m_pDS->exec(PrepareSQL("DELETE FROM streamdetails WHERE idFile = %i", idFile));

    for (int i=1; i<=details.GetVideoStreamCount(); i++)
    {
      m_pDS->exec(PrepareSQL("INSERT INTO streamdetails "
        "(idFile, iStreamType, strVideoCodec, fVideoAspect, iVideoWidth, iVideoHeight, iVideoDuration, strStereoMode, strVideoLanguage) "
        "VALUES (%i,%i,'%s',%f,%i,%i,%i,'%s','%s')",
        idFile, (int)CStreamDetail::VIDEO,
        details.GetVideoCodec(i).c_str(), details.GetVideoAspect(i),
        details.GetVideoWidth(i), details.GetVideoHeight(i), details.GetVideoDuration(i),
        details.GetStereoMode(i).c_str(),
        details.GetVideoLanguage(i).c_str()));
    }
    for (int i=1; i<=details.GetAudioStreamCount(); i++)
    {
      m_pDS->exec(PrepareSQL("INSERT INTO streamdetails "
        "(idFile, iStreamType, strAudioCodec, iAudioChannels, strAudioLanguage) "
        "VALUES (%i,%i,'%s',%i,'%s')",
        idFile, (int)CStreamDetail::AUDIO,
        details.GetAudioCodec(i).c_str(), details.GetAudioChannels(i),
        details.GetAudioLanguage(i).c_str()));
    }
    for (int i=1; i<=details.GetSubtitleStreamCount(); i++)
    {
      m_pDS->exec(PrepareSQL("INSERT INTO streamdetails "
        "(idFile, iStreamType, strSubtitleLanguage) "
        "VALUES (%i,%i,'%s')",
        idFile, (int)CStreamDetail::SUBTITLE,
        details.GetSubtitleLanguage(i).c_str()));
    }

    // update the runtime information, if empty
    if (details.GetVideoDuration())
    {
      std::vector<std::pair<std::string, int> > tables;
      tables.emplace_back("movie", VIDEODB_ID_RUNTIME);
      tables.emplace_back("episode", VIDEODB_ID_EPISODE_RUNTIME);
      tables.emplace_back("musicvideo", VIDEODB_ID_MUSICVIDEO_RUNTIME);
      for (const auto &i : tables)
      {
        std::string sql = PrepareSQL("update %s set c%02d=%d where idFile=%d and c%02d=''",
                                    i.first.c_str(), i.second, details.GetVideoDuration(), idFile, i.second);
        m_pDS->exec(sql);
      }
    }

    CommitTransaction();
  }
  catch (...)
  {
    RollbackTransaction();
    CLog::Log(LOGERROR, "%s (%i) failed", __FUNCTION__, idFile);
  }
}

void CVideoDatabase::DeleteStreamDetails(int idFile)
{
  m_pDS->exec(PrepareSQL("DELETE FROM streamdetails WHERE idFile = %i", idFile));
}

bool CVideoDatabase::GetStreamDetails(CFileItem& item)
{
  // Note that this function (possibly) creates VideoInfoTags for items that don't have one yet!
  int fileId = -1;

  if (item.HasVideoInfoTag())
    fileId = item.GetVideoInfoTag()->m_iFileId;

  if (fileId < 0)
    fileId = GetFileId(item);

  if (fileId < 0)
    return false;

  // Have a file id, get stream details if available (creates tag either way)
  item.GetVideoInfoTag()->m_iFileId = fileId;
  return GetStreamDetails(*item.GetVideoInfoTag());
}

bool CVideoDatabase::GetStreamDetails(CVideoInfoTag& tag) const
{
  if (tag.m_iFileId < 0)
    return false;

  bool retVal = false;

  CStreamDetails& details = tag.m_streamDetails;
  details.Reset();

  std::unique_ptr<Dataset> pDS(m_pDB->CreateDataset());
  try
  {
    std::string strSQL = PrepareSQL("SELECT * FROM streamdetails WHERE idFile = %i", tag.m_iFileId);
    pDS->query(strSQL);

    while (!pDS->eof())
    {
      CStreamDetail::StreamType e = (CStreamDetail::StreamType)pDS->fv(1).get_asInt();
      switch (e)
      {
      case CStreamDetail::VIDEO:
        {
          CStreamDetailVideo *p = new CStreamDetailVideo();
          p->m_strCodec = pDS->fv(2).get_asString();
          p->m_fAspect = pDS->fv(3).get_asFloat();
          p->m_iWidth = pDS->fv(4).get_asInt();
          p->m_iHeight = pDS->fv(5).get_asInt();
          p->m_iDuration = pDS->fv(10).get_asInt();
          p->m_strStereoMode = pDS->fv(11).get_asString();
          p->m_strLanguage = pDS->fv(12).get_asString();
          details.AddStream(p);
          retVal = true;
          break;
        }
      case CStreamDetail::AUDIO:
        {
          CStreamDetailAudio *p = new CStreamDetailAudio();
          p->m_strCodec = pDS->fv(6).get_asString();
          if (pDS->fv(7).get_isNull())
            p->m_iChannels = -1;
          else
            p->m_iChannels = pDS->fv(7).get_asInt();
          p->m_strLanguage = pDS->fv(8).get_asString();
          details.AddStream(p);
          retVal = true;
          break;
        }
      case CStreamDetail::SUBTITLE:
        {
          CStreamDetailSubtitle *p = new CStreamDetailSubtitle();
          p->m_strLanguage = pDS->fv(9).get_asString();
          details.AddStream(p);
          retVal = true;
          break;
        }
      }

      pDS->next();
    }

    pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%i) failed", __FUNCTION__, tag.m_iFileId);
  }
  details.DetermineBestStreams();

  if (details.GetVideoDuration() > 0)
    tag.SetDuration(details.GetVideoDuration());

  return retVal;
}
