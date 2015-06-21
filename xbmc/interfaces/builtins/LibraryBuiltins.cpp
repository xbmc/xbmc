/*
 *      Copyright (C) 2005-2015 Team XBMC
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

#include "LibraryBuiltins.h"

#include "Application.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/GUIWindowManager.h"
#include "GUIUserMessages.h"
#include "MediaSource.h"
#include "messaging/helpers/DialogHelper.h"
#include "music/MusicDatabase.h"
#include "storage/MediaManager.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "video/VideoDatabase.h"

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
  if (!params.size() || StringUtils::EqualsNoCase(params[0], "video"))
  {
    if (!g_application.IsVideoScanning())
      g_application.StartVideoCleanup(userInitiated);
    else
      CLog::Log(LOGERROR, "CleanLibrary is not possible while scanning or cleaning");
  }
  else if (StringUtils::EqualsNoCase(params[0], "music"))
  {
    if (!g_application.IsMusicScanning())
      g_application.StartMusicCleanup(userInitiated);
    else
      CLog::Log(LOGERROR, "CleanLibrary is not possible while scanning for media info");
  }

  return 0;
}

/*! \brief Export a library.
 *  \param params The parameters.
 *  \details params[0] = "video" or "music".
 *           params[1] = "true" to export to a single file (optional).
 *           params[2] = "true" to export thumbs (optional).
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
  g_mediaManager.GetLocalDrives(shares);
  g_mediaManager.GetNetworkLocations(shares);
  g_mediaManager.GetRemovableDrives(shares);
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
    cancelled = result == HELPERS::DialogResponse::CANCELLED;
    singleFile = result != HELPERS::DialogResponse::YES;
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
      cancelled = result == HELPERS::DialogResponse::CANCELLED;
      thumbs = result == HELPERS::DialogResponse::YES;
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
      cancelled = result == HELPERS::DialogResponse::CANCELLED;
      actorThumbs = result == HELPERS::DialogResponse::YES;
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
      cancelled = result == HELPERS::DialogResponse::CANCELLED;
      overwrite = result == HELPERS::DialogResponse::YES;
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
      if (URIUtils::HasSlashAtEnd(path))
        path = URIUtils::AddFileToFolder(path, "musicdb.xml");
      CMusicDatabase musicdatabase;
      musicdatabase.Open();
      musicdatabase.ExportToXML(path, singleFile, thumbs, overwrite);
      musicdatabase.Close();
    }
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
    if (g_application.IsMusicScanning())
      g_application.StopMusicScan();
    else
      g_application.StartMusicScan(params.size() > 1 ? params[1] : "", userInitiated);
  }
  else if (StringUtils::EqualsNoCase(params[0], "video"))
  {
    if (g_application.IsVideoScanning())
      g_application.StopVideoScan();
    else
      g_application.StartVideoScan(params.size() > 1 ? params[1] : "", userInitiated);
  }

  return 0;
}

/*! \brief Open a video library search.
 *  \param params (ignored)
 */
static int SearchVideoLibrary(const std::vector<std::string>& params)
{
  CGUIMessage msg(GUI_MSG_SEARCH, 0, 0, 0);
  g_windowManager.SendMessage(msg, WINDOW_VIDEO_NAV);

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
///     @param[in] type                  "video" or "music".
///   }
///   \table_row2_l{
///     <b>`exportlibrary(type [\, exportSingeFile\, exportThumbs\, overwrite\, exportActorThumbs])`</b>
///     ,
///     Export the video/music library
///     @param[in] type                  "video" or "music".
///     @param[in] exportSingleFile      Add "true" to export to a single file (optional).
///     @param[in] exportThumbs          Add "true" to export thumbs (optional).
///     @param[in] overwrite             Add "true" to overwrite existing files (optional).
///     @param[in] exportActorThumbs     Add "true" to export actor thumbs (optional).
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
          {"updatelibrary",       {"Update the selected library (music or video)", 1, UpdateLibrary}},
          {"videolibrary.search", {"Brings up a search dialog which will search the library", 0, SearchVideoLibrary}}
         };
}
