/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoDatabase.h"

#include <map>
#include <memory>
#include <string>

#include "video/VideoInfoTag.h"
#include "addons/AddonManager.h"
#include "ServiceBroker.h"
#include "dbwrappers/dataset.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogProgress.h"
#include "FileItem.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "interfaces/AnnouncementManager.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "TextureCache.h"
#include "Util.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "utils/XMLUtils.h"
#include "video/VideoThumbLoader.h"
#include "video/VideoInfoScanner.h"

using namespace dbiplus;
using namespace XFILE;
using namespace VIDEO;
using namespace ADDON;
using namespace KODI::MESSAGING;

void CVideoDatabase::ExportToXML(const std::string &path,
                                 bool singleFile /* = true */,
                                 bool images /* = false */,
                                 bool actorThumbs /* false */,
                                 bool overwrite /*=false*/)
{
  int iFailCount = 0;
  CGUIDialogProgress *progress=NULL;
  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;
    if (NULL == m_pDS2.get()) return;

    // create a 3rd dataset as well as GetEpisodeDetails() etc. uses m_pDS2, and we need to do 3 nested queries on tv shows
    std::unique_ptr<Dataset> pDS;
    pDS.reset(m_pDB->CreateDataset());
    if (NULL == pDS.get()) return;

    std::unique_ptr<Dataset> pDS2;
    pDS2.reset(m_pDB->CreateDataset());
    if (NULL == pDS2.get()) return;

    // if we're exporting to a single folder, we export thumbs as well
    std::string exportRoot = URIUtils::AddFileToFolder(path, "kodi_videodb_" + CDateTime::GetCurrentDateTime().GetAsDBDate());
    std::string xmlFile = URIUtils::AddFileToFolder(exportRoot, "videodb.xml");
    std::string actorsDir = URIUtils::AddFileToFolder(exportRoot, "actors");
    std::string moviesDir = URIUtils::AddFileToFolder(exportRoot, "movies");
    std::string musicvideosDir = URIUtils::AddFileToFolder(exportRoot, "musicvideos");
    std::string tvshowsDir = URIUtils::AddFileToFolder(exportRoot, "tvshows");
    if (singleFile)
    {
      images = true;
      overwrite = false;
      actorThumbs = true;
      CDirectory::Remove(exportRoot);
      CDirectory::Create(exportRoot);
      CDirectory::Create(actorsDir);
      CDirectory::Create(moviesDir);
      CDirectory::Create(musicvideosDir);
      CDirectory::Create(tvshowsDir);
    }

    progress = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogProgress>(WINDOW_DIALOG_PROGRESS);
    // find all movies
    std::string sql = "select * from movie_view";

    m_pDS->query(sql);

    if (progress)
    {
      progress->SetHeading(CVariant{647});
      progress->SetLine(0, CVariant{650});
      progress->SetLine(1, CVariant{""});
      progress->SetLine(2, CVariant{""});
      progress->SetPercentage(0);
      progress->Open();
      progress->ShowProgressBar(true);
    }

    int total = m_pDS->num_rows();
    int current = 0;

    // create our xml document
    CXBMCTinyXML xmlDoc;
    TiXmlDeclaration decl("1.0", "UTF-8", "yes");
    xmlDoc.InsertEndChild(decl);
    TiXmlNode *pMain = NULL;
    if (!singleFile)
      pMain = &xmlDoc;
    else
    {
      TiXmlElement xmlMainElement("videodb");
      pMain = xmlDoc.InsertEndChild(xmlMainElement);
      XMLUtils::SetInt(pMain,"version", GetExportVersion());
    }

    while (!m_pDS->eof())
    {
      CVideoInfoTag movie = GetDetailsForMovie(m_pDS, VideoDbDetailsAll);
      // strip paths to make them relative
      if (StringUtils::StartsWith(movie.m_strTrailer, movie.m_strPath))
        movie.m_strTrailer = movie.m_strTrailer.substr(movie.m_strPath.size());
      std::map<std::string, std::string> artwork;
      if (GetArtForItem(movie.m_iDbId, movie.m_type, artwork) && singleFile)
      {
        TiXmlElement additionalNode("art");
        for (const auto &i : artwork)
          XMLUtils::SetString(&additionalNode, i.first.c_str(), i.second);
        movie.Save(pMain, "movie", true, &additionalNode);
      }
      else
        movie.Save(pMain, "movie", singleFile);

      // reset old skip state
      bool bSkip = false;

      if (progress)
      {
        progress->SetLine(1, CVariant{movie.m_strTitle});
        progress->SetPercentage(current * 100 / total);
        progress->Progress();
        if (progress->IsCanceled())
        {
          progress->Close();
          m_pDS->close();
          return;
        }
      }

      CFileItem item(movie.m_strFileNameAndPath,false);
      if (!singleFile && CUtil::SupportsWriteFileOperations(movie.m_strFileNameAndPath))
      {
        if (!item.Exists(false))
        {
          CLog::Log(LOGINFO, "%s - Not exporting item %s as it does not exist", __FUNCTION__, movie.m_strFileNameAndPath.c_str());
          bSkip = true;
        }
        else
        {
          std::string nfoFile(URIUtils::ReplaceExtension(item.GetTBNFile(), ".nfo"));

          if (item.IsOpticalMediaFile())
          {
            nfoFile = URIUtils::AddFileToFolder(
                                    URIUtils::GetParentPath(nfoFile),
                                    URIUtils::GetFileName(nfoFile));
          }

          if (overwrite || !CFile::Exists(nfoFile, false))
          {
            if(!xmlDoc.SaveFile(nfoFile))
            {
              CLog::Log(LOGERROR, "%s: Movie nfo export failed! ('%s')", __FUNCTION__, nfoFile.c_str());
              CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error, g_localizeStrings.Get(20302), nfoFile);
              iFailCount++;
            }
          }
        }
      }
      if (!singleFile)
      {
        xmlDoc.Clear();
        TiXmlDeclaration decl("1.0", "UTF-8", "yes");
        xmlDoc.InsertEndChild(decl);
      }

      if (images && !bSkip)
      {
        if (singleFile)
        {
          std::string strFileName(movie.m_strTitle);
          if (movie.HasYear())
            strFileName += StringUtils::Format("_%i", movie.GetYear());
          item.SetPath(GetSafeFile(moviesDir, strFileName) + ".avi");
        }
        for (const auto &i : artwork)
        {
          std::string savedThumb = item.GetLocalArt(i.first, false);
          CTextureCache::GetInstance().Export(i.second, savedThumb, overwrite);
        }
        if (actorThumbs)
          ExportActorThumbs(actorsDir, movie, !singleFile, overwrite);
      }
      m_pDS->next();
      current++;
    }
    m_pDS->close();

    // find all musicvideos
    sql = "select * from musicvideo_view";

    m_pDS->query(sql);

    total = m_pDS->num_rows();
    current = 0;

    while (!m_pDS->eof())
    {
      CVideoInfoTag movie = GetDetailsForMusicVideo(m_pDS, VideoDbDetailsAll);
      std::map<std::string, std::string> artwork;
      if (GetArtForItem(movie.m_iDbId, movie.m_type, artwork) && singleFile)
      {
        TiXmlElement additionalNode("art");
        for (const auto &i : artwork)
          XMLUtils::SetString(&additionalNode, i.first.c_str(), i.second);
        movie.Save(pMain, "musicvideo", true, &additionalNode);
      }
      else
        movie.Save(pMain, "musicvideo", singleFile);

      // reset old skip state
      bool bSkip = false;

      if (progress)
      {
        progress->SetLine(1, CVariant{movie.m_strTitle});
        progress->SetPercentage(current * 100 / total);
        progress->Progress();
        if (progress->IsCanceled())
        {
          progress->Close();
          m_pDS->close();
          return;
        }
      }

      CFileItem item(movie.m_strFileNameAndPath,false);
      if (!singleFile && CUtil::SupportsWriteFileOperations(movie.m_strFileNameAndPath))
      {
        if (!item.Exists(false))
        {
          CLog::Log(LOGINFO, "%s - Not exporting item %s as it does not exist", __FUNCTION__, movie.m_strFileNameAndPath.c_str());
          bSkip = true;
        }
        else
        {
          std::string nfoFile(URIUtils::ReplaceExtension(item.GetTBNFile(), ".nfo"));

          if (overwrite || !CFile::Exists(nfoFile, false))
          {
            if(!xmlDoc.SaveFile(nfoFile))
            {
              CLog::Log(LOGERROR, "%s: Musicvideo nfo export failed! ('%s')", __FUNCTION__, nfoFile.c_str());
              CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error, g_localizeStrings.Get(20302), nfoFile);
              iFailCount++;
            }
          }
        }
      }
      if (!singleFile)
      {
        xmlDoc.Clear();
        TiXmlDeclaration decl("1.0", "UTF-8", "yes");
        xmlDoc.InsertEndChild(decl);
      }
      if (images && !bSkip)
      {
        if (singleFile)
        {
          std::string strFileName(StringUtils::Join(movie.m_artist, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator) + "." + movie.m_strTitle);
          if (movie.HasYear())
            strFileName += StringUtils::Format("_%i", movie.GetYear());
          item.SetPath(GetSafeFile(musicvideosDir, strFileName) + ".avi");
        }
        for (const auto &i : artwork)
        {
          std::string savedThumb = item.GetLocalArt(i.first, false);
          CTextureCache::GetInstance().Export(i.second, savedThumb, overwrite);
        }
      }
      m_pDS->next();
      current++;
    }
    m_pDS->close();

    // repeat for all tvshows
    sql = "SELECT * FROM tvshow_view";
    m_pDS->query(sql);

    total = m_pDS->num_rows();
    current = 0;

    while (!m_pDS->eof())
    {
      CVideoInfoTag tvshow = GetDetailsForTvShow(m_pDS, VideoDbDetailsAll);
      GetTvShowNamedSeasons(tvshow.m_iDbId, tvshow.m_namedSeasons);

      std::map<int, std::map<std::string, std::string> > seasonArt;
      GetTvShowSeasonArt(tvshow.m_iDbId, seasonArt);

      std::map<std::string, std::string> artwork;
      if (GetArtForItem(tvshow.m_iDbId, tvshow.m_type, artwork) && singleFile)
      {
        TiXmlElement additionalNode("art");
        for (const auto &i : artwork)
          XMLUtils::SetString(&additionalNode, i.first.c_str(), i.second);
        for (const auto &i : seasonArt)
        {
          TiXmlElement seasonNode("season");
          seasonNode.SetAttribute("num", i.first);
          for (const auto &j : i.second)
            XMLUtils::SetString(&seasonNode, j.first.c_str(), j.second);
          additionalNode.InsertEndChild(seasonNode);
        }
        tvshow.Save(pMain, "tvshow", true, &additionalNode);
      }
      else
        tvshow.Save(pMain, "tvshow", singleFile);

      // reset old skip state
      bool bSkip = false;

      if (progress)
      {
        progress->SetLine(1, CVariant{tvshow.m_strTitle});
        progress->SetPercentage(current * 100 / total);
        progress->Progress();
        if (progress->IsCanceled())
        {
          progress->Close();
          m_pDS->close();
          return;
        }
      }

      CFileItem item(tvshow.m_strPath, true);
      if (!singleFile && CUtil::SupportsWriteFileOperations(tvshow.m_strPath))
      {
        if (!item.Exists(false))
        {
          CLog::Log(LOGINFO, "%s - Not exporting item %s as it does not exist", __FUNCTION__, tvshow.m_strPath.c_str());
          bSkip = true;
        }
        else
        {
          std::string nfoFile = URIUtils::AddFileToFolder(tvshow.m_strPath, "tvshow.nfo");

          if (overwrite || !CFile::Exists(nfoFile, false))
          {
            if(!xmlDoc.SaveFile(nfoFile))
            {
              CLog::Log(LOGERROR, "%s: TVShow nfo export failed! ('%s')", __FUNCTION__, nfoFile.c_str());
              CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error, g_localizeStrings.Get(20302), nfoFile);
              iFailCount++;
            }
          }
        }
      }
      if (!singleFile)
      {
        xmlDoc.Clear();
        TiXmlDeclaration decl("1.0", "UTF-8", "yes");
        xmlDoc.InsertEndChild(decl);
      }
      if (images && !bSkip)
      {
        if (singleFile)
          item.SetPath(GetSafeFile(tvshowsDir, tvshow.m_strTitle));

        for (const auto &i : artwork)
        {
          std::string savedThumb = item.GetLocalArt(i.first, true);
          CTextureCache::GetInstance().Export(i.second, savedThumb, overwrite);
        }

        if (actorThumbs)
          ExportActorThumbs(actorsDir, tvshow, !singleFile, overwrite);

        // export season thumbs
        for (const auto &i : seasonArt)
        {
          std::string seasonThumb;
          if (i.first == -1)
            seasonThumb = "season-all";
          else if (i.first == 0)
            seasonThumb = "season-specials";
          else
            seasonThumb = StringUtils::Format("season%02i", i.first);
          for (const auto &j : i.second)
          {
            std::string savedThumb(item.GetLocalArt(seasonThumb + "-" + j.first, true));
            if (!i.second.empty())
              CTextureCache::GetInstance().Export(j.second, savedThumb, overwrite);
          }
        }
      }

      // now save the episodes from this show
      sql = PrepareSQL("select * from episode_view where idShow=%i order by strFileName, idEpisode",tvshow.m_iDbId);
      pDS->query(sql);
      std::string showDir(item.GetPath());

      while (!pDS->eof())
      {
        CVideoInfoTag episode = GetDetailsForEpisode(pDS, VideoDbDetailsAll);
        std::map<std::string, std::string> artwork;
        if (GetArtForItem(episode.m_iDbId, MediaTypeEpisode, artwork) && singleFile)
        {
          TiXmlElement additionalNode("art");
          for (const auto &i : artwork)
            XMLUtils::SetString(&additionalNode, i.first.c_str(), i.second);
          episode.Save(pMain->LastChild(), "episodedetails", true, &additionalNode);
        }
        else if (singleFile)
          episode.Save(pMain->LastChild(), "episodedetails", singleFile);
        else
          episode.Save(pMain, "episodedetails", singleFile);
        pDS->next();
        // multi-episode files need dumping to the same XML
        while (!singleFile && !pDS->eof() &&
               episode.m_iFileId == pDS->fv("idFile").get_asInt())
        {
          episode = GetDetailsForEpisode(pDS, VideoDbDetailsAll);
          episode.Save(pMain, "episodedetails", singleFile);
          pDS->next();
        }

        // reset old skip state
        bool bSkip = false;

        CFileItem item(episode.m_strFileNameAndPath, false);
        if (!singleFile && CUtil::SupportsWriteFileOperations(episode.m_strFileNameAndPath))
        {
          if (!item.Exists(false))
          {
            CLog::Log(LOGINFO, "%s - Not exporting item %s as it does not exist", __FUNCTION__, episode.m_strFileNameAndPath.c_str());
            bSkip = true;
          }
          else
          {
            std::string nfoFile(URIUtils::ReplaceExtension(item.GetTBNFile(), ".nfo"));

            if (overwrite || !CFile::Exists(nfoFile, false))
            {
              if(!xmlDoc.SaveFile(nfoFile))
              {
                CLog::Log(LOGERROR, "%s: Episode nfo export failed! ('%s')", __FUNCTION__, nfoFile.c_str());
                CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error, g_localizeStrings.Get(20302), nfoFile);
                iFailCount++;
              }
            }
          }
        }
        if (!singleFile)
        {
          xmlDoc.Clear();
          TiXmlDeclaration decl("1.0", "UTF-8", "yes");
          xmlDoc.InsertEndChild(decl);
        }

        if (images && !bSkip)
        {
          if (singleFile)
          {
            std::string epName = StringUtils::Format("s%02ie%02i.avi", episode.m_iSeason, episode.m_iEpisode);
            item.SetPath(URIUtils::AddFileToFolder(showDir, epName));
          }
          for (const auto &i : artwork)
          {
            std::string savedThumb = item.GetLocalArt(i.first, false);
            CTextureCache::GetInstance().Export(i.second, savedThumb, overwrite);
          }
          if (actorThumbs)
            ExportActorThumbs(actorsDir, episode, !singleFile, overwrite);
        }
      }
      pDS->close();
      m_pDS->next();
      current++;
    }
    m_pDS->close();

    if (!singleFile && progress)
    {
      progress->SetPercentage(100);
      progress->Progress();
    }

    if (singleFile)
    {
      // now dump path info
      std::set<std::string> paths;
      GetPaths(paths);
      TiXmlElement xmlPathElement("paths");
      TiXmlNode *pPaths = pMain->InsertEndChild(xmlPathElement);
      for (const auto &i : paths)
      {
        bool foundDirectly = false;
        SScanSettings settings;
        ScraperPtr info = GetScraperForPath(i, settings, foundDirectly);
        if (info && foundDirectly)
        {
          TiXmlElement xmlPathElement2("path");
          TiXmlNode *pPath = pPaths->InsertEndChild(xmlPathElement2);
          XMLUtils::SetString(pPath,"url", i);
          XMLUtils::SetInt(pPath,"scanrecursive", settings.recurse);
          XMLUtils::SetBoolean(pPath,"usefoldernames", settings.parent_name);
          XMLUtils::SetString(pPath,"content", TranslateContent(info->Content()));
          XMLUtils::SetString(pPath,"scraperpath", info->ID());
        }
      }
      xmlDoc.SaveFile(xmlFile);
    }
    CVariant data;
    if (singleFile)
    {
      data["root"] = exportRoot;
      data["file"] = xmlFile;
      if (iFailCount > 0)
        data["failcount"] = iFailCount;
    }
    CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::VideoLibrary, "xbmc", "OnExport", data);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
    iFailCount++;
  }

  if (progress)
    progress->Close();

  if (iFailCount > 0)
    HELPERS::ShowOKDialogText(CVariant{647}, CVariant{StringUtils::Format(g_localizeStrings.Get(15011).c_str(), iFailCount)});
}

void CVideoDatabase::ExportActorThumbs(const std::string &strDir,
                                       const CVideoInfoTag &tag,
                                       bool singleFiles, bool overwrite /*=false*/)
{
  std::string strPath(strDir);
  if (singleFiles)
  {
    strPath = URIUtils::AddFileToFolder(tag.m_strPath, ".actors");
    if (!CDirectory::Exists(strPath))
    {
      CDirectory::Create(strPath);
      CFile::SetHidden(strPath, true);
    }
  }

  for (const auto &i : tag.m_cast)
  {
    CFileItem item;
    item.SetLabel(i.strName);
    if (!i.thumb.empty())
    {
      std::string thumbFile(GetSafeFile(strPath, i.strName));
      CTextureCache::GetInstance().Export(i.thumb, thumbFile, overwrite);
    }
  }
}

void CVideoDatabase::ImportFromXML(const std::string &path)
{
  CGUIDialogProgress *progress=NULL;
  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    CXBMCTinyXML xmlDoc;
    if (!xmlDoc.LoadFile(URIUtils::AddFileToFolder(path, "videodb.xml")))
      return;

    TiXmlElement *root = xmlDoc.RootElement();
    if (!root) return;

    progress = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogProgress>(WINDOW_DIALOG_PROGRESS);
    if (progress)
    {
      progress->SetHeading(CVariant{648});
      progress->SetLine(0, CVariant{649});
      progress->SetLine(1, CVariant{330});
      progress->SetLine(2, CVariant{""});
      progress->SetPercentage(0);
      progress->Open();
      progress->ShowProgressBar(true);
    }

    int iVersion = 0;
    XMLUtils::GetInt(root, "version", iVersion);

    CLog::Log(LOGINFO, "%s: Starting import (export version = %i)", __FUNCTION__, iVersion);

    TiXmlElement *movie = root->FirstChildElement();
    int current = 0;
    int total = 0;
    // first count the number of items...
    while (movie)
    {
      if (strnicmp(movie->Value(), MediaTypeMovie, 5)==0 ||
          strnicmp(movie->Value(), MediaTypeTvShow, 6)==0 ||
          strnicmp(movie->Value(), MediaTypeMusicVideo,10)==0 )
        total++;
      movie = movie->NextSiblingElement();
    }

    std::string actorsDir(URIUtils::AddFileToFolder(path, "actors"));
    std::string moviesDir(URIUtils::AddFileToFolder(path, "movies"));
    std::string musicvideosDir(URIUtils::AddFileToFolder(path, "musicvideos"));
    std::string tvshowsDir(URIUtils::AddFileToFolder(path, "tvshows"));
    CVideoInfoScanner scanner;
    // add paths first (so we have scraper settings available)
    TiXmlElement *path = root->FirstChildElement("paths");
    path = path->FirstChildElement();
    while (path)
    {
      std::string strPath;
      if (XMLUtils::GetString(path,"url",strPath) && !strPath.empty())
        AddPath(strPath);

      std::string content;
      if (XMLUtils::GetString(path,"content", content) && !content.empty())
      { // check the scraper exists, if so store the path
        AddonPtr addon;
        std::string id;
        XMLUtils::GetString(path,"scraperpath",id);
        if (CServiceBroker::GetAddonMgr().GetAddon(id, addon))
        {
          SScanSettings settings;
          ScraperPtr scraper = std::dynamic_pointer_cast<CScraper>(addon);
          // FIXME: scraper settings are not exported?
          scraper->SetPathSettings(TranslateContent(content), "");
          XMLUtils::GetInt(path,"scanrecursive",settings.recurse);
          XMLUtils::GetBoolean(path,"usefoldernames",settings.parent_name);
          SetScraperForPath(strPath,scraper,settings);
        }
      }
      path = path->NextSiblingElement();
    }
    movie = root->FirstChildElement();
    while (movie)
    {
      CVideoInfoTag info;
      if (strnicmp(movie->Value(), MediaTypeMovie, 5) == 0)
      {
        info.Load(movie);
        CFileItem item(info);
        bool useFolders = info.m_basePath.empty() ? LookupByFolders(item.GetPath()) : false;
        std::string filename = info.m_strTitle;
        if (info.HasYear())
          filename += StringUtils::Format("_%i", info.GetYear());
        CFileItem artItem(item);
        artItem.SetPath(GetSafeFile(moviesDir, filename) + ".avi");
        scanner.GetArtwork(&artItem, CONTENT_MOVIES, useFolders, true, actorsDir);
        item.SetArt(artItem.GetArt());
        scanner.AddVideo(&item, CONTENT_MOVIES, useFolders, true, NULL, true);
        current++;
      }
      else if (strnicmp(movie->Value(), MediaTypeMusicVideo, 10) == 0)
      {
        info.Load(movie);
        CFileItem item(info);
        bool useFolders = info.m_basePath.empty() ? LookupByFolders(item.GetPath()) : false;
        std::string filename = StringUtils::Join(info.m_artist, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator) + "." + info.m_strTitle;
        if (info.HasYear())
          filename += StringUtils::Format("_%i", info.GetYear());
        CFileItem artItem(item);
        artItem.SetPath(GetSafeFile(musicvideosDir, filename) + ".avi");
        scanner.GetArtwork(&artItem, CONTENT_MUSICVIDEOS, useFolders, true, actorsDir);
        item.SetArt(artItem.GetArt());
        scanner.AddVideo(&item, CONTENT_MUSICVIDEOS, useFolders, true, NULL, true);
        current++;
      }
      else if (strnicmp(movie->Value(), MediaTypeTvShow, 6) == 0)
      {
        // load the TV show in.  NOTE: This deletes all episodes under the TV Show, which may not be
        // what we desire.  It may make better sense to only delete (or even better, update) the show information
        info.Load(movie);
        URIUtils::AddSlashAtEnd(info.m_strPath);
        DeleteTvShow(info.m_strPath);
        CFileItem showItem(info);
        bool useFolders = info.m_basePath.empty() ? LookupByFolders(showItem.GetPath(), true) : false;
        CFileItem artItem(showItem);
        std::string artPath(GetSafeFile(tvshowsDir, info.m_strTitle));
        artItem.SetPath(artPath);
        scanner.GetArtwork(&artItem, CONTENT_TVSHOWS, useFolders, true, actorsDir);
        showItem.SetArt(artItem.GetArt());
        int showID = scanner.AddVideo(&showItem, CONTENT_TVSHOWS, useFolders, true, NULL, true);
        // season artwork
        std::map<int, std::map<std::string, std::string> > seasonArt;
        artItem.GetVideoInfoTag()->m_strPath = artPath;
        scanner.GetSeasonThumbs(*artItem.GetVideoInfoTag(), seasonArt, CVideoThumbLoader::GetArtTypes(MediaTypeSeason), true);
        for (const auto &i : seasonArt)
        {
          int seasonID = AddSeason(showID, i.first);
          SetArtForItem(seasonID, MediaTypeSeason, i.second);
        }
        current++;
        // now load the episodes
        TiXmlElement *episode = movie->FirstChildElement("episodedetails");
        while (episode)
        {
          // no need to delete the episode info, due to the above deletion
          CVideoInfoTag info;
          info.Load(episode);
          CFileItem item(info);
          std::string filename = StringUtils::Format("s%02ie%02i.avi", info.m_iSeason, info.m_iEpisode);
          CFileItem artItem(item);
          artItem.SetPath(GetSafeFile(artPath, filename));
          scanner.GetArtwork(&artItem, CONTENT_TVSHOWS, useFolders, true, actorsDir);
          item.SetArt(artItem.GetArt());
          scanner.AddVideo(&item,CONTENT_TVSHOWS, false, false, showItem.GetVideoInfoTag(), true);
          episode = episode->NextSiblingElement("episodedetails");
        }
      }
      movie = movie->NextSiblingElement();
      if (progress && total)
      {
        progress->SetPercentage(current * 100 / total);
        progress->SetLine(2, CVariant{info.m_strTitle});
        progress->Progress();
        if (progress->IsCanceled())
        {
          progress->Close();
          return;
        }
      }
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  if (progress)
    progress->Close();
}

void CVideoDatabase::DumpToDummyFiles(const std::string &path)
{
  // get all tvshows
  CFileItemList items;
  GetTvShowsByWhere("videodb://tvshows/titles/", CDatabase::Filter(), items);
  std::string showPath = URIUtils::AddFileToFolder(path, "shows");
  CDirectory::Create(showPath);
  for (int i = 0; i < items.Size(); i++)
  {
    // create a folder in this directory
    std::string showName = CUtil::MakeLegalFileName(items[i]->GetVideoInfoTag()->m_strShowTitle);
    std::string TVFolder = URIUtils::AddFileToFolder(showPath, showName);
    if (CDirectory::Create(TVFolder))
    { // right - grab the episodes and dump them as well
      CFileItemList episodes;
      Filter filter(PrepareSQL("idShow=%i", items[i]->GetVideoInfoTag()->m_iDbId));
      GetEpisodesByWhere("videodb://tvshows/titles/", filter, episodes);
      for (int i = 0; i < episodes.Size(); i++)
      {
        CVideoInfoTag *tag = episodes[i]->GetVideoInfoTag();
        std::string episode = StringUtils::Format("%s.s%02de%02d.avi", showName.c_str(), tag->m_iSeason, tag->m_iEpisode);
        // and make a file
        std::string episodePath = URIUtils::AddFileToFolder(TVFolder, episode);
        CFile file;
        if (file.OpenForWrite(episodePath))
          file.Close();
      }
    }
  }
  // get all movies
  items.Clear();
  GetMoviesByWhere("videodb://movies/titles/", CDatabase::Filter(), items);
  std::string moviePath = URIUtils::AddFileToFolder(path, "movies");
  CDirectory::Create(moviePath);
  for (int i = 0; i < items.Size(); i++)
  {
    CVideoInfoTag *tag = items[i]->GetVideoInfoTag();
    std::string movie = StringUtils::Format("%s.avi", tag->m_strTitle.c_str());
    CFile file;
    if (file.OpenForWrite(URIUtils::AddFileToFolder(moviePath, movie)))
      file.Close();
  }
}
