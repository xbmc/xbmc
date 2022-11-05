/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "LibraryBuiltins.h"

#include "GUIUserMessages.h"
#include "MediaSource.h"
#include "ServiceBroker.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "messaging/helpers/DialogHelper.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "music/MusicLibraryQueue.h"
#include "music/infoscanner/MusicInfoScanner.h"
#include "settings/LibExportSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "storage/MediaManager.h"
#include "utils/StringUtils.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"
#include "video/VideoLibraryQueue.h"

using namespace KODI::MESSAGING;

/*! \brief Clean a library.
 *  \param params The parameters.
 *  \details params[0] = "video" or "music".
 */
static int CleanLibrary(const std::vector<std::string>& params)
{
  bool userInitiated = true;
  if (params.size() > 1)
    userInitiated = StringUtils::EqualsNoCase(params[1], "true");
  if (!params.size() || StringUtils::EqualsNoCase(params[0], "video")
                     || StringUtils::EqualsNoCase(params[0], "movies")
                     || StringUtils::EqualsNoCase(params[0], "tvshows")
                     || StringUtils::EqualsNoCase(params[0], "musicvideos"))
  {
    if (!CVideoLibraryQueue::GetInstance().IsScanningLibrary())
    {
      if (userInitiated && CVideoLibraryQueue::GetInstance().IsRunning())
        HELPERS::ShowOKDialogText(CVariant{700}, CVariant{703});
      else
      {
        const std::string content = (params.empty() || params[0] == "video") ? "" : params[0];
        const std::string directory = params.size() > 2 ? params[2] : "";

        std::set<int> paths;
        if (!content.empty() || !directory.empty())
        {
          CVideoDatabase db;
          std::set<std::string> contentPaths;
          if (db.Open())
          {
            if (!directory.empty())
              contentPaths.insert(directory);
            else
              db.GetPaths(contentPaths);
            for (const std::string& path : contentPaths)
            {
              if (db.GetContentForPath(path) == content)
              {
                paths.insert(db.GetPathId(path));
                std::vector<std::pair<int, std::string>> sub;
                if (db.GetSubPaths(path, sub))
                {
                  for (const auto& it : sub)
                    paths.insert(it.first);
                }
              }
            }
          }
          if (paths.empty())
            return 0;
        }

        if (userInitiated)
          CVideoLibraryQueue::GetInstance().CleanLibraryModal(paths);
        else
          CVideoLibraryQueue::GetInstance().CleanLibrary(paths, true);
      }
    }
    else
      CLog::Log(LOGERROR, "CleanLibrary is not possible while scanning or cleaning");
  }
  else if (StringUtils::EqualsNoCase(params[0], "music"))
  {
    if (!CMusicLibraryQueue::GetInstance().IsScanningLibrary())
    {
      if (!(userInitiated && CMusicLibraryQueue::GetInstance().IsRunning()))
        CMusicLibraryQueue::GetInstance().CleanLibrary(userInitiated);
    }
    else
      CLog::Log(LOGERROR, "CleanLibrary is not possible while scanning for media info");
  }
  else
    CLog::Log(LOGERROR, "Unknown content type '{}' passed to CleanLibrary, ignoring", params[0]);

  return 0;
}

/*! \brief Export a library.
 *  \param params The parameters.
 *  \details params[0] = "video" or "music".
 *           params[1] = "true" to export to separate files (optional).
 *           params[2] = "true" to export thumbs (optional) or the file path for export to singlefile.
 *           params[3] = "true" to overwrite existing files (optional).
 *           params[4] = "true" to export actor thumbs (optional).
 */
static int ExportLibrary(const std::vector<std::string>& params)
{
  int iHeading = 647;
  if (StringUtils::EqualsNoCase(params[0], "music"))
    iHeading = 20196;
  std::string path;
  VECSOURCES shares;
  CServiceBroker::GetMediaManager().GetLocalDrives(shares);
  CServiceBroker::GetMediaManager().GetNetworkLocations(shares);
  CServiceBroker::GetMediaManager().GetRemovableDrives(shares);
  bool singleFile;
  bool thumbs=false;
  bool actorThumbs=false;
  bool overwrite=false;
  bool cancelled=false;

  if (params.size() > 1)
    singleFile = StringUtils::EqualsNoCase(params[1], "false");
  else
  {
    HELPERS::DialogResponse result = HELPERS::ShowYesNoDialogText(CVariant{iHeading}, CVariant{20426}, CVariant{20428}, CVariant{20429});
    cancelled = result == HELPERS::DialogResponse::CHOICE_CANCELLED;
    singleFile = result != HELPERS::DialogResponse::CHOICE_YES;
  }

  if (cancelled)
    return -1;

  if (!singleFile)
  {
    if (params.size() > 2)
      thumbs = StringUtils::EqualsNoCase(params[2], "true");
    else
    {
      HELPERS::DialogResponse result = HELPERS::ShowYesNoDialogText(CVariant{iHeading}, CVariant{20430});
      cancelled = result == HELPERS::DialogResponse::CHOICE_CANCELLED;
      thumbs = result == HELPERS::DialogResponse::CHOICE_YES;
    }
  }

  if (cancelled)
    return -1;

  if (thumbs && !singleFile && StringUtils::EqualsNoCase(params[0], "video"))
  {
    std::string movieSetsInfoPath = CServiceBroker::GetSettingsComponent()->GetSettings()->
        GetString(CSettings::SETTING_VIDEOLIBRARY_MOVIESETSFOLDER);
    if (movieSetsInfoPath.empty())
    {
      auto result = HELPERS::ShowYesNoDialogText(CVariant{iHeading}, CVariant{36301});
      cancelled = result != HELPERS::DialogResponse::CHOICE_YES;
    }
  }

  if (cancelled)
    return -1;

  if (thumbs && StringUtils::EqualsNoCase(params[0], "video"))
  {
    if (params.size() > 4)
      actorThumbs = StringUtils::EqualsNoCase(params[4], "true");
    else
    {
      HELPERS::DialogResponse result = HELPERS::ShowYesNoDialogText(CVariant{iHeading}, CVariant{20436});
      cancelled = result == HELPERS::DialogResponse::CHOICE_CANCELLED;
      actorThumbs = result == HELPERS::DialogResponse::CHOICE_YES;
    }
  }

  if (cancelled)
    return -1;

  if (!singleFile)
  {
    if (params.size() > 3)
      overwrite = StringUtils::EqualsNoCase(params[3], "true");
    else
    {
      HELPERS::DialogResponse result = HELPERS::ShowYesNoDialogText(CVariant{iHeading}, CVariant{20431});
      cancelled = result == HELPERS::DialogResponse::CHOICE_CANCELLED;
      overwrite = result == HELPERS::DialogResponse::CHOICE_YES;
    }
  }

  if (cancelled)
    return -1;

  if (params.size() > 2)
    path=params[2];
  if (!singleFile || !path.empty() ||
      CGUIDialogFileBrowser::ShowAndGetDirectory(shares, g_localizeStrings.Get(661),
                                                 path, true))
  {
    if (StringUtils::EqualsNoCase(params[0], "video"))
    {
      CVideoDatabase videodatabase;
      videodatabase.Open();
      videodatabase.ExportToXML(path, singleFile, thumbs, actorThumbs, overwrite);
      videodatabase.Close();
    }
    else
    {
      CLibExportSettings settings;
      // ELIBEXPORT_SINGLEFILE, ELIBEXPORT_ALBUMS + ELIBEXPORT_ALBUMARTISTS by default
      settings.m_strPath = path;
      if (!singleFile)
        settings.SetExportType(ELIBEXPORT_TOLIBRARYFOLDER);
      settings.m_artwork = thumbs;
      settings.m_overwrite = overwrite;
      // Export music library (not showing progress dialog)
      CMusicLibraryQueue::GetInstance().ExportLibrary(settings, false);
    }
  }

  return 0;
}

/*! \brief Export a library with extended parameters
Avoiding breaking change to original ExportLibrary routine parameters
*  \param params The parameters.
*  \details params[0] = "video" or "music".
*           params[1] = export type "singlefile", "separate", or "library".
*           params[2] = path of destination folder.
*           params[3,...] = "unscraped" to include unscraped items
*           params[3,...] = "overwrite" to overwrite existing files.
*           params[3,...] = "artwork" to include images such as thumbs and fanart.
*           params[3,...] = "skipnfo" to not include nfo files (just art).
*           params[3,...] = "ablums" to include albums.
*           params[3,...] = "albumartists" to include album artists.
*           params[3,...] = "songartists" to include song artists.
*           params[3,...] = "otherartists" to include other artists.
*/
static int ExportLibrary2(const std::vector<std::string>& params)
{
  CLibExportSettings settings;
  if (params.size() < 3)
    return -1;
  settings.m_strPath = params[2];
  settings.SetExportType(ELIBEXPORT_SINGLEFILE);
  if (StringUtils::EqualsNoCase(params[1], "separate"))
    settings.SetExportType(ELIBEXPORT_SEPARATEFILES);
  else if (StringUtils::EqualsNoCase(params[1], "library"))
  {
    settings.SetExportType(ELIBEXPORT_TOLIBRARYFOLDER);
    settings.m_strPath.clear();
  }
  settings.ClearItems();

  for (unsigned int i = 2; i < params.size(); i++)
  {
    if (StringUtils::EqualsNoCase(params[i], "artwork"))
      settings.m_artwork = true;
    else if (StringUtils::EqualsNoCase(params[i], "overwrite"))
      settings.m_overwrite = true;
    else if (StringUtils::EqualsNoCase(params[i], "unscraped"))
      settings.m_unscraped = true;
    else if (StringUtils::EqualsNoCase(params[i], "skipnfo"))
      settings.m_skipnfo = true;
    else if (StringUtils::EqualsNoCase(params[i], "albums"))
      settings.AddItem(ELIBEXPORT_ALBUMS);
    else if (StringUtils::EqualsNoCase(params[i], "albumartists"))
      settings.AddItem(ELIBEXPORT_ALBUMARTISTS);
    else if (StringUtils::EqualsNoCase(params[i], "songartists"))
      settings.AddItem(ELIBEXPORT_SONGARTISTS);
    else if (StringUtils::EqualsNoCase(params[i], "otherartists"))
      settings.AddItem(ELIBEXPORT_OTHERARTISTS);
    else if (StringUtils::EqualsNoCase(params[i], "actorthumbs"))
      settings.AddItem(ELIBEXPORT_ACTORTHUMBS);
  }
  if (StringUtils::EqualsNoCase(params[0], "music"))
  {
    // Export music library (not showing progress dialog)
    CMusicLibraryQueue::GetInstance().ExportLibrary(settings, false);
  }
  else
  {
    CVideoDatabase videodatabase;
    videodatabase.Open();
    videodatabase.ExportToXML(settings.m_strPath, settings.IsSingleFile(),
      settings.m_artwork, settings.IsItemExported(ELIBEXPORT_ACTORTHUMBS), settings.m_overwrite);
    videodatabase.Close();
  }
  return 0;
}


/*! \brief Update a library.
 *  \param params The parameters.
 *  \details params[0] = "video" or "music".
 *           params[1] = "true" to suppress dialogs (optional).
 */
static int UpdateLibrary(const std::vector<std::string>& params)
{
  bool userInitiated = true;
  if (params.size() > 2)
    userInitiated = StringUtils::EqualsNoCase(params[2], "true");
  if (StringUtils::EqualsNoCase(params[0], "music"))
  {
    if (CMusicLibraryQueue::GetInstance().IsScanningLibrary())
      CMusicLibraryQueue::GetInstance().StopLibraryScanning();
    else
      CMusicLibraryQueue::GetInstance().ScanLibrary(params.size() > 1 ? params[1] : "",
                                                    MUSIC_INFO::CMusicInfoScanner::SCAN_NORMAL,
                                                    userInitiated);
  }
  else if (StringUtils::EqualsNoCase(params[0], "video"))
  {
    if (CVideoLibraryQueue::GetInstance().IsScanningLibrary())
      CVideoLibraryQueue::GetInstance().StopLibraryScanning();
    else
      CVideoLibraryQueue::GetInstance().ScanLibrary(params.size() > 1 ? params[1] : "", false,
                                                    userInitiated);
  }

  return 0;
}

/*! \brief Open a video library search.
 *  \param params (ignored)
 */
static int SearchVideoLibrary(const std::vector<std::string>& params)
{
  CGUIMessage msg(GUI_MSG_SEARCH, 0, 0, 0);
  CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg, WINDOW_VIDEO_NAV);

  return 0;
}

// Note: For new Texts with comma add a "\" before!!! Is used for table text.
//
/// \page page_List_of_built_in_functions
/// \section built_in_functions_8 Library built-in's
///
/// -----------------------------------------------------------------------------
///
/// \table_start
///   \table_h2_l{
///     Function,
///     Description }
///   \table_row2_l{
///     <b>`cleanlibrary(type)`</b>
///     ,
///      Clean the video/music library
///     @param[in] type                  "video"\, "movies"\, "tvshows"\, "musicvideos" or "music".
///   }
///   \table_row2_l{
///     <b>`exportlibrary(type [\, exportSingeFile\, exportThumbs\, overwrite\, exportActorThumbs])`</b>
///     ,
///     Export the video/music library
///     @param[in] type                  "video" or "music".
///     @param[in] exportSingleFile      Add "true" to export to separate files (optional).
///     @param[in] exportThumbs          Add "true" to export thumbs (optional).
///     @param[in] overwrite             Add "true" to overwrite existing files (optional).
///     @param[in] exportActorThumbs     Add "true" to export actor thumbs (optional).
///   }
///   \table_row2_l{
///     <b>`exportlibrary2(library\, exportFiletype\, path [\, unscraped][\, overwrite][\, artwork][\, skipnfo]
///     [\, albums][\, albumartists][\, songartists][\, otherartists][\, actorthumbs])`</b>
///     ,
///     Export the video/music library with extended parameters
///     @param[in] library               "video" or "music".
///     @param[in] exportFiletype        "singlefile"\, "separate" or "library".
///     @param[in] path                  Path to destination folder.
///     @param[in] unscraped             Add "unscraped" to include unscraped items.
///     @param[in] overwrite             Add "overwrite" to overwrite existing files.
///     @param[in] artwork               Add "artwork" to include images such as thumbs and fanart.
///     @param[in] skipnfo               Add "skipnfo" to not include nfo files(just art).
///     @param[in] albums                Add "ablums" to include albums.
///     @param[in] albumartists          Add "albumartists" to include album artists.
///     @param[in] songartists           Add "songartists" to include song artists.
///     @param[in] otherartists          Add "otherartists" to include other artists.
///     @param[in] actorthumbs           Add "actorthumbs" to include other actor thumbs.
///   }
///   \table_row2_l{
///     <b>`updatelibrary([type\, suppressDialogs])`</b>
///     ,
///     Update the selected library (music or video)
///     @param[in] type                  "video" or "music".
///     @param[in] suppressDialogs       Add "true" to suppress dialogs (optional).
///   }
///   \table_row2_l{
///     <b>`videolibrary.search`</b>
///     ,
///     Brings up a search dialog which will search the library
///   }
///  \table_end
///

CBuiltins::CommandMap CLibraryBuiltins::GetOperations() const
{
  return {
          {"cleanlibrary",        {"Clean the video/music library", 1, CleanLibrary}},
          {"exportlibrary",       {"Export the video/music library", 1, ExportLibrary}},
          {"exportlibrary2",      {"Export the video/music library", 1, ExportLibrary2}},
          {"updatelibrary",       {"Update the selected library (music or video)", 1, UpdateLibrary}},
          {"videolibrary.search", {"Brings up a search dialog which will search the library", 0, SearchVideoLibrary}}
         };
}
