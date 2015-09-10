/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "network/Network.h"
#include "system.h"
#include "utils/AlarmClock.h"
#include "utils/Screenshot.h"
#include "utils/SeekHandler.h"
#include "Application.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/helpers/DialogHelper.h"
#include "Autorun.h"
#include "Builtins.h"
#include "input/ButtonTranslator.h"
#include "input/InputManager.h"
#include "FileItem.h"
#include "addons/GUIDialogAddonSettings.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "guilib/GUIKeyboardFactory.h"
#include "input/Key.h"
#include "guilib/StereoscopicsManager.h"
#include "guilib/GUIAudioManager.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogNumeric.h"
#include "dialogs/GUIDialogProgress.h"
#include "GUIUserMessages.h"
#include "windows/GUIWindowLoginScreen.h"
#include "video/windows/GUIWindowVideoBase.h"
#include "addons/GUIWindowAddonBrowser.h"
#include "addons/Addon.h" // for TranslateType, TranslateContent
#include "addons/AddonInstaller.h"
#include "addons/AddonManager.h"
#include "addons/PluginSource.h"
#include "addons/RepositoryUpdater.h"
#include "addons/Skin.h"
#include "interfaces/generic/ScriptInvocationManager.h"
#include "interfaces/AnnouncementManager.h"
#include "network/NetworkServices.h"
#include "utils/log.h"
#include "storage/MediaManager.h"
#include "utils/RssManager.h"
#include "utils/JSONVariantParser.h"
#include "PartyModeManager.h"
#include "profiles/ProfilesManager.h"
#include "settings/DisplaySettings.h"
#include "settings/Settings.h"
#include "settings/MediaSettings.h"
#include "settings/MediaSourceSettings.h"
#include "settings/SkinSettings.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "video/VideoLibraryQueue.h"
#include "Util.h"
#include "URL.h"
#include "music/MusicDatabase.h"
#include "cores/IPlayer.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/recordings/PVRRecording.h"
#include "pvr/PVRManager.h"

#include "filesystem/PluginDirectory.h"
#ifdef HAS_FILESYSTEM_RAR
#include "filesystem/RarManager.h"
#endif
#include "filesystem/ZipManager.h"

#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"

#ifdef HAS_LIRC
#include "input/linux/LIRC.h"
#endif
#ifdef HAS_IRSERVERSUITE

  #include "input/windows/IRServerSuite.h"

#endif

#if defined(TARGET_DARWIN)
#include "filesystem/SpecialProtocol.h"
#include "osx/CocoaInterface.h"
#endif

#ifdef HAS_CDDA_RIPPER
#include "cdrip/CDDARipper.h"
#endif

#include <vector>
#include "settings/AdvancedSettings.h"
#include "settings/DisplaySettings.h"
#include "powermanagement/PowerManager.h"
#include "filesystem/Directory.h"

#include "AddonBuiltins.h"
#include "CECBuiltins.h"
#include "GUIBuiltins.h"
#include "GUIControlBuiltins.h"
#include "GUIContainerBuiltins.h"
#include "LibraryBuiltins.h"
#include "ProfileBuiltins.h"
#include "PVRBuiltins.h"
#include "SkinBuiltins.h"
#include "SystemBuiltins.h"
#include "WeatherBuiltins.h"

using namespace XFILE;
using namespace ADDON;
using namespace KODI::MESSAGING;

#ifdef HAS_DVD_DRIVE
using namespace MEDIA_DETECT;
#endif

using KODI::MESSAGING::HELPERS::DialogResponse;

typedef struct
{
  const char* command;
  bool needsParameters;
  const char* description;
} BUILT_IN;

const BUILT_IN commands[] = {
  { "Help",                       false,  "This help message" },
  { "NotifyAll",                  true,   "Notify all connected clients" },
  { "Extract",                    true,   "Extracts the specified archive" },
  { "PlayMedia",                  true,   "Play the specified media file (or playlist)" },
  { "Seek",                       true,   "Performs a seek in seconds on the current playing media file" },
  { "ShowPicture",                true,   "Display a picture by file path" },
  { "SlideShow",                  true,   "Run a slideshow from the specified directory" },
  { "RecursiveSlideShow",         true,   "Run a slideshow from the specified directory, including all subdirs" },
  { "PlayerControl",              true,   "Control the music or video player" },
  { "Playlist.PlayOffset",        true,   "Start playing from a particular offset in the playlist" },
  { "Playlist.Clear",             false,  "Clear the current playlist" },
  { "EjectTray",                  false,  "Close or open the DVD tray" },
  { "PlayDVD",                    false,  "Plays the inserted CD or DVD media from the DVD-ROM Drive!" },
  { "RipCD",                      false,  "Rip the currently inserted audio CD"},
  { "Mute",                       false,  "Mute the player" },
  { "SetVolume",                  true,   "Set the current volume" },
  { "PlayWith",                   true,   "Play the selected item with the specified core" },
  { "WakeOnLan",                  true,   "Sends the wake-up packet to the broadcast address for the specified MAC address" },
  { "ToggleDPMS",                 false,  "Toggle DPMS mode manually"},
  { "ToggleDebug",                false,  "Enables/disables debug mode" },
#if defined(TARGET_ANDROID)
  { "StartAndroidActivity",       true,   "Launch an Android native app with the given package name.  Optional parms (in order): intent, dataType, dataURI." },
#endif
};

CBuiltins::CBuiltins()
{
  RegisterCommands<CAddonBuiltins>();
  RegisterCommands<CGUIBuiltins>();
  RegisterCommands<CGUIContainerBuiltins>();
  RegisterCommands<CGUIControlBuiltins>();
  RegisterCommands<CLibraryBuiltins>();
  RegisterCommands<CProfileBuiltins>();
  RegisterCommands<CPVRBuiltins>();
  RegisterCommands<CSkinBuiltins>();
  RegisterCommands<CSystemBuiltins>();
  RegisterCommands<CWeatherBuiltins>();
}

CBuiltins::~CBuiltins()
{
}

CBuiltins& CBuiltins::GetInstance()
{
  static CBuiltins sBuiltins;
  return sBuiltins;
}

bool CBuiltins::HasCommand(const std::string& execString)
{
  std::string function;
  std::vector<std::string> parameters;
  CUtil::SplitExecFunction(execString, function, parameters);
  StringUtils::ToLower(function);
  const auto& it = m_command.find(function);
  if (it != m_command.end())
  {
    if (it->second.parameters == 0 || it->second.parameters <= parameters.size())
      return true;
  }
  else
  {
    for (unsigned int i = 0; i < sizeof(commands)/sizeof(BUILT_IN); i++)
    {
      if (StringUtils::EqualsNoCase(function, commands[i].command) && (!commands[i].needsParameters || parameters.size()))
        return true;
    }
  }

  return false;
}

bool CBuiltins::IsSystemPowerdownCommand(const std::string& execString)
{
  std::string execute;
  std::vector<std::string> params;
  CUtil::SplitExecFunction(execString, execute, params);
  StringUtils::ToLower(execute);

  // Check if action is resulting in system powerdown.
  if (execute == "reboot"    ||
      execute == "restart"   ||
      execute == "reset"     ||
      execute == "powerdown" ||
      execute == "hibernate" ||
      execute == "suspend" )
  {
    return true;
  }
  else if (execute == "shutdown")
  {
    switch (CSettings::GetInstance().GetInt(CSettings::SETTING_POWERMANAGEMENT_SHUTDOWNSTATE))
    {
      case POWERSTATE_SHUTDOWN:
      case POWERSTATE_SUSPEND:
      case POWERSTATE_HIBERNATE:
        return true;

      default:
        return false;
    }
  }
  return false;
}

void CBuiltins::GetHelp(std::string &help)
{
  help.clear();

  for (const auto& it : m_command)
  {
    help += it.first;
    help += "\t";
    help += it.second.description;
    help += "\n";
  }

  for (unsigned int i = 0; i < sizeof(commands)/sizeof(BUILT_IN); i++)
  {
    help += commands[i].command;
    help += "\t";
    help += commands[i].description;
    help += "\n";
  }
}

int CBuiltins::Execute(const std::string& execString)
{
  // Deprecated. Get the text after the "XBMC."
  std::string execute;
  std::vector<std::string> params;
  CUtil::SplitExecFunction(execString, execute, params);
  StringUtils::ToLower(execute);
  std::string parameter = params.size() ? params[0] : "";
  std::string paramlow(parameter);
  StringUtils::ToLower(paramlow);

  const auto& it = m_command.find(execute);
  if (it != m_command.end())
  {
    if (it->second.parameters == 0 || params.size() >= it->second.parameters)
      return it->second.Execute(params);
    else
    {
      CLog::Log(LOGERROR, "%s called with invalid number of parameters (should be: %" PRIdS ", is %" PRIdS")",
                          execute.c_str(), it->second.parameters, params.size());
      return -1;
    }
  }

  if (execute == "extract" && params.size())
  {
    // Detects if file is zip or rar then extracts
    std::string strDestDirect;
    if (params.size() < 2)
      strDestDirect = URIUtils::GetDirectory(params[0]);
    else
      strDestDirect = params[1];

    URIUtils::AddSlashAtEnd(strDestDirect);

    if (URIUtils::IsZIP(params[0]))
      g_ZipManager.ExtractArchive(params[0],strDestDirect);
#ifdef HAS_FILESYSTEM_RAR
    else if (URIUtils::IsRAR(params[0]))
      g_RarManager.ExtractArchive(params[0],strDestDirect);
#endif
    else
      CLog::Log(LOGERROR, "Extract, No archive given");
  }
  else if (execute == "notifyall")
  {
    if (params.size() > 1)
    {
      CVariant data;
      if (params.size() > 2)
        data = CJSONVariantParser::Parse((const unsigned char *)params[2].c_str(), params[2].size());

      ANNOUNCEMENT::CAnnouncementManager::GetInstance().Announce(ANNOUNCEMENT::Other, params[0].c_str(), params[1].c_str(), data);
    }
    else
      CLog::Log(LOGERROR, "NotifyAll needs two parameters");
  }
  else if (execute == "playmedia")
  {
    if (!params.size())
    {
      CLog::Log(LOGERROR, "PlayMedia called with empty parameter");
      return -3;
    }

    CFileItem item(params[0], false);
    if (URIUtils::HasSlashAtEnd(params[0]))
      item.m_bIsFolder = true;

    // restore to previous window if needed
    if( g_windowManager.GetActiveWindow() == WINDOW_SLIDESHOW ||
        g_windowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO ||
        g_windowManager.GetActiveWindow() == WINDOW_VISUALISATION )
        g_windowManager.PreviousWindow();

    // reset screensaver
    g_application.ResetScreenSaver();
    g_application.WakeUpScreenSaverAndDPMS();

    // ask if we need to check guisettings to resume
    bool askToResume = true;
    int playOffset = 0;
    for (unsigned int i = 1 ; i < params.size() ; i++)
    {
      if (StringUtils::EqualsNoCase(params[i], "isdir"))
        item.m_bIsFolder = true;
      else if (params[i] == "1") // set fullscreen or windowed
        CMediaSettings::GetInstance().SetVideoStartWindowed(true);
      else if (StringUtils::EqualsNoCase(params[i], "resume"))
      {
        // force the item to resume (if applicable) (see CApplication::PlayMedia)
        item.m_lStartOffset = STARTOFFSET_RESUME;
        askToResume = false;
      }
      else if (StringUtils::EqualsNoCase(params[i], "noresume"))
      {
        // force the item to start at the beginning (m_lStartOffset is initialized to 0)
        askToResume = false;
      }
      else if (StringUtils::StartsWithNoCase(params[i], "playoffset=")) {
        playOffset = atoi(params[i].substr(11).c_str()) - 1;
        item.SetProperty("playlist_starting_track", playOffset);
      }
    }

    if (!item.m_bIsFolder && item.IsPlugin())
      item.SetProperty("IsPlayable", true);

    if ( askToResume == true )
    {
      if ( CGUIWindowVideoBase::ShowResumeMenu(item) == false )
        return false;
    }
    if (item.m_bIsFolder)
    {
      CFileItemList items;
      std::string extensions = g_advancedSettings.m_videoExtensions + "|" + g_advancedSettings.GetMusicExtensions();
      CDirectory::GetDirectory(item.GetPath(),items,extensions);

      bool containsMusic = false, containsVideo = false;
      for (int i = 0; i < items.Size(); i++)
      {
        bool isVideo = items[i]->IsVideo();
        containsMusic |= !isVideo;
        containsVideo |= isVideo;
        
        if (containsMusic && containsVideo)
          break;
      }

      std::unique_ptr<CGUIViewState> state(CGUIViewState::GetViewState(containsVideo ? WINDOW_VIDEO_NAV : WINDOW_MUSIC, items));
      if (state.get())
        items.Sort(state->GetSortMethod());
      else
        items.Sort(SortByLabel, SortOrderAscending);

      int playlist = containsVideo? PLAYLIST_VIDEO : PLAYLIST_MUSIC;
      if (containsMusic && containsVideo) //mixed content found in the folder
      {
        for (int i = items.Size() - 1; i >= 0; i--) //remove music entries
        {
          if (!items[i]->IsVideo())
            items.Remove(i);
        }
      }
      
      g_playlistPlayer.ClearPlaylist(playlist);
      g_playlistPlayer.Add(playlist, items);
      g_playlistPlayer.SetCurrentPlaylist(playlist);
      g_playlistPlayer.Play(playOffset);
    }
    else
    {
      int playlist = item.IsAudio() ? PLAYLIST_MUSIC : PLAYLIST_VIDEO;
      g_playlistPlayer.ClearPlaylist(playlist);
      g_playlistPlayer.SetCurrentPlaylist(playlist);

      // play media
      if (!g_application.PlayMedia(item, playlist))
      {
        CLog::Log(LOGERROR, "PlayMedia could not play media: %s", params[0].c_str());
        return false;
      }
    }
  }
  else if (execute == "seek")
  {
    if (!params.size())
    {
      CLog::Log(LOGERROR, "Seek called with empty parameter");
      return -3;
    }
    if (g_application.m_pPlayer->IsPlaying())
      CSeekHandler::GetInstance().SeekSeconds(atoi(params[0].c_str()));
  }
  else if (execute == "showpicture")
  {
    if (!params.size())
    {
      CLog::Log(LOGERROR, "ShowPicture called with empty parameter");
      return -2;
    }
    CGUIMessage msg(GUI_MSG_SHOW_PICTURE, 0, 0);
    msg.SetStringParam(params[0]);
    CGUIWindow *pWindow = g_windowManager.GetWindow(WINDOW_SLIDESHOW);
    if (pWindow) pWindow->OnMessage(msg);
  }
  else if (execute == "slideshow" || execute == "recursiveslideshow")
  {
    if (!params.size())
    {
      CLog::Log(LOGERROR, "SlideShow called with empty parameter");
      return -2;
    }
    std::string beginSlidePath;
    // leave RecursiveSlideShow command as-is
    unsigned int flags = 0;
    if (execute == "recursiveslideshow")
      flags |= 1;

    // SlideShow(dir[,recursive][,[not]random][,pause][,beginslide="/path/to/start/slide.jpg"])
    // the beginslide value need be escaped (for '"' or '\' in it, by backslash)
    // and then quoted, or not. See CUtil::SplitParams()
    else
    {
      for (unsigned int i = 1 ; i < params.size() ; i++)
      {
        if (StringUtils::EqualsNoCase(params[i], "recursive"))
          flags |= 1;
        else if (StringUtils::EqualsNoCase(params[i], "random")) // set fullscreen or windowed
          flags |= 2;
        else if (StringUtils::EqualsNoCase(params[i], "notrandom"))
          flags |= 4;
        else if (StringUtils::EqualsNoCase(params[i], "pause"))
          flags |= 8;
        else if (StringUtils::StartsWithNoCase(params[i], "beginslide="))
          beginSlidePath = params[i].substr(11);
      }
    }

    CGUIMessage msg(GUI_MSG_START_SLIDESHOW, 0, 0, flags);
    std::vector<std::string> strParams;
    strParams.push_back(params[0]);
    strParams.push_back(beginSlidePath);
    msg.SetStringParams(strParams);
    CGUIWindow *pWindow = g_windowManager.GetWindow(WINDOW_SLIDESHOW);
    if (pWindow) pWindow->OnMessage(msg);
  }
  else if (execute == "playercontrol")
  {
    g_application.ResetScreenSaver();
    g_application.WakeUpScreenSaverAndDPMS();
    if (!params.size())
    {
      CLog::Log(LOGERROR, "PlayerControl called with empty parameter");
      return -3;
    }
    if (paramlow ==  "play")
    { // play/pause
      // either resume playing, or pause
      if (g_application.m_pPlayer->IsPlaying())
      {
        if (g_application.m_pPlayer->GetPlaySpeed() != 1)
          g_application.m_pPlayer->SetPlaySpeed(1, g_application.IsMutedInternal());
        else
          g_application.m_pPlayer->Pause();
      }
    }
    else if (paramlow == "stop")
    {
      g_application.StopPlaying();
    }
    else if (paramlow =="rewind" || paramlow == "forward")
    {
      if (g_application.m_pPlayer->IsPlaying() && !g_application.m_pPlayer->IsPaused())
      {
        int iPlaySpeed = g_application.m_pPlayer->GetPlaySpeed();
        if (paramlow == "rewind" && iPlaySpeed == 1) // Enables Rewinding
          iPlaySpeed *= -2;
        else if (paramlow ==  "rewind" && iPlaySpeed > 1) //goes down a notch if you're FFing
          iPlaySpeed /= 2;
        else if (paramlow == "forward" && iPlaySpeed < 1) //goes up a notch if you're RWing
        {
          iPlaySpeed /= 2;
          if (iPlaySpeed == -1) iPlaySpeed = 1;
        }
        else
          iPlaySpeed *= 2;

        if (iPlaySpeed > 32 || iPlaySpeed < -32)
          iPlaySpeed = 1;

        g_application.m_pPlayer->SetPlaySpeed(iPlaySpeed, g_application.IsMutedInternal());
      }
    }
    else if (paramlow == "next")
    {
      g_application.OnAction(CAction(ACTION_NEXT_ITEM));
    }
    else if (paramlow == "previous")
    {
      g_application.OnAction(CAction(ACTION_PREV_ITEM));
    }
    else if (paramlow == "bigskipbackward")
    {
      if (g_application.m_pPlayer->IsPlaying())
        g_application.m_pPlayer->Seek(false, true);
    }
    else if (paramlow == "bigskipforward")
    {
      if (g_application.m_pPlayer->IsPlaying())
        g_application.m_pPlayer->Seek(true, true);
    }
    else if (paramlow == "smallskipbackward")
    {
      if (g_application.m_pPlayer->IsPlaying())
        g_application.m_pPlayer->Seek(false, false);
    }
    else if (paramlow == "smallskipforward")
    {
      if (g_application.m_pPlayer->IsPlaying())
        g_application.m_pPlayer->Seek(true, false);
    }
    else if (StringUtils::StartsWithNoCase(parameter, "seekpercentage"))
    {
      std::string offset;
      if (parameter.size() == 14)
        CLog::Log(LOGERROR,"PlayerControl(seekpercentage(n)) called with no argument");
      else if (parameter.size() < 17) // arg must be at least "(N)"
        CLog::Log(LOGERROR,"PlayerControl(seekpercentage(n)) called with invalid argument: \"%s\"", parameter.substr(14).c_str());
      else
      {
        // Don't bother checking the argument: an invalid arg will do seek(0)
        offset = parameter.substr(15);
        StringUtils::TrimRight(offset, ")");
        float offsetpercent = (float) atof(offset.c_str());
        if (offsetpercent < 0 || offsetpercent > 100)
          CLog::Log(LOGERROR,"PlayerControl(seekpercentage(n)) argument, %f, must be 0-100", offsetpercent);
        else if (g_application.m_pPlayer->IsPlaying())
          g_application.SeekPercentage(offsetpercent);
      }
    }
    else if (paramlow == "showvideomenu")
    {
      if( g_application.m_pPlayer->IsPlaying() )
        g_application.m_pPlayer->OnAction(CAction(ACTION_SHOW_VIDEOMENU));
    }
    else if (paramlow == "record")
    {
      if( g_application.m_pPlayer->IsPlaying() && g_application.m_pPlayer->CanRecord())
        g_application.m_pPlayer->Record(!g_application.m_pPlayer->IsRecording());
    }
    else if (StringUtils::StartsWithNoCase(parameter, "partymode"))
    {
      std::string strXspPath;
      //empty param=music, "music"=music, "video"=video, else xsp path
      PartyModeContext context = PARTYMODECONTEXT_MUSIC;
      if (parameter.size() > 9)
      {
        if (parameter.size() == 16 && StringUtils::EndsWithNoCase(parameter, "video)"))
          context = PARTYMODECONTEXT_VIDEO;
        else if (parameter.size() != 16 || !StringUtils::EndsWithNoCase(parameter, "music)"))
        {
          strXspPath = parameter.substr(10);
          StringUtils::TrimRight(strXspPath, ")");
          context = PARTYMODECONTEXT_UNKNOWN;
        }
      }
      if (g_partyModeManager.IsEnabled())
        g_partyModeManager.Disable();
      else
        g_partyModeManager.Enable(context, strXspPath);
    }
    else if (paramlow == "random" || paramlow == "randomoff" || paramlow == "randomon")
    {
      // get current playlist
      int iPlaylist = g_playlistPlayer.GetCurrentPlaylist();

      // reverse the current setting
      bool shuffled = g_playlistPlayer.IsShuffled(iPlaylist);
      if ((shuffled && paramlow == "randomon") || (!shuffled && paramlow == "randomoff"))
        return 0;

      // check to see if we should notify the user
      bool notify = (params.size() == 2 && StringUtils::EqualsNoCase(params[1], "notify"));
      g_playlistPlayer.SetShuffle(iPlaylist, !shuffled, notify);

      // save settings for now playing windows
      switch (iPlaylist)
      {
      case PLAYLIST_MUSIC:
        CMediaSettings::GetInstance().SetMusicPlaylistShuffled(g_playlistPlayer.IsShuffled(iPlaylist));
        CSettings::GetInstance().Save();
        break;
      case PLAYLIST_VIDEO:
        CMediaSettings::GetInstance().SetVideoPlaylistShuffled(g_playlistPlayer.IsShuffled(iPlaylist));
        CSettings::GetInstance().Save();
      }

      // send message
      CGUIMessage msg(GUI_MSG_PLAYLISTPLAYER_RANDOM, 0, 0, iPlaylist, g_playlistPlayer.IsShuffled(iPlaylist));
      g_windowManager.SendThreadMessage(msg);

    }
    else if (StringUtils::StartsWithNoCase(parameter, "repeat"))
    {
      // get current playlist
      int iPlaylist = g_playlistPlayer.GetCurrentPlaylist();
      PLAYLIST::REPEAT_STATE previous_state = g_playlistPlayer.GetRepeat(iPlaylist);

      PLAYLIST::REPEAT_STATE state;
      if (paramlow == "repeatall")
        state = PLAYLIST::REPEAT_ALL;
      else if (paramlow == "repeatone")
        state = PLAYLIST::REPEAT_ONE;
      else if (paramlow == "repeatoff")
        state = PLAYLIST::REPEAT_NONE;
      else if (previous_state == PLAYLIST::REPEAT_NONE)
        state = PLAYLIST::REPEAT_ALL;
      else if (previous_state == PLAYLIST::REPEAT_ALL)
        state = PLAYLIST::REPEAT_ONE;
      else
        state = PLAYLIST::REPEAT_NONE;

      if (state == previous_state)
        return 0;

      // check to see if we should notify the user
      bool notify = (params.size() == 2 && StringUtils::EqualsNoCase(params[1], "notify"));
      g_playlistPlayer.SetRepeat(iPlaylist, state, notify);

      // save settings for now playing windows
      switch (iPlaylist)
      {
      case PLAYLIST_MUSIC:
        CMediaSettings::GetInstance().SetMusicPlaylistRepeat(state == PLAYLIST::REPEAT_ALL);
        CSettings::GetInstance().Save();
        break;
      case PLAYLIST_VIDEO:
        CMediaSettings::GetInstance().SetVideoPlaylistRepeat(state == PLAYLIST::REPEAT_ALL);
        CSettings::GetInstance().Save();
      }

      // send messages so now playing window can get updated
      CGUIMessage msg(GUI_MSG_PLAYLISTPLAYER_REPEAT, 0, 0, iPlaylist, (int)state);
      g_windowManager.SendThreadMessage(msg);
    }
    else if (StringUtils::StartsWithNoCase(parameter, "resumelivetv"))
    {
      CFileItem& fileItem(g_application.CurrentFileItem());
      PVR::CPVRChannelPtr channel = fileItem.HasPVRRecordingInfoTag() ? fileItem.GetPVRRecordingInfoTag()->Channel() : PVR::CPVRChannelPtr();

      if (channel)
      {
        CFileItem playItem(channel);
        if (!g_application.PlayMedia(playItem, channel->IsRadio() ? PLAYLIST_MUSIC : PLAYLIST_VIDEO))
        {
          CLog::Log(LOGERROR, "ResumeLiveTv could not play channel: %s", channel->ChannelName().c_str());
          return false;
        }
      }
    }
  }
  else if (execute == "playwith")
  {
    g_application.m_eForcedNextPlayer = CPlayerCoreFactory::GetInstance().GetPlayerCore(parameter);
    g_application.OnAction(CAction(ACTION_PLAYER_PLAY));
  }
  else if (execute == "mute")
  {
    g_application.ToggleMute();
  }
  else if (execute == "setvolume")
  {
    float oldVolume = g_application.GetVolume();
    float volume = (float)strtod(parameter.c_str(), NULL);

    g_application.SetVolume(volume);
    if(oldVolume != volume)
    {
      if(params.size() > 1 && StringUtils::EqualsNoCase(params[1], "showVolumeBar"))
      {
        CApplicationMessenger::GetInstance().PostMsg(TMSG_VOLUME_SHOW, oldVolume < volume ? ACTION_VOLUME_UP : ACTION_VOLUME_DOWN);
      }
    }
  }
  else if (execute == "playlist.playoffset")
  {
    // playlist.playoffset(offset)
    // playlist.playoffset(music|video,offset)
    std::string strPos = parameter;
    if (params.size() > 1)
    {
      // ignore any other parameters if present
      std::string strPlaylist = params[0];
      strPos = params[1];

      int iPlaylist = PLAYLIST_NONE;
      if (paramlow == "music")
        iPlaylist = PLAYLIST_MUSIC;
      else if (paramlow == "video")
        iPlaylist = PLAYLIST_VIDEO;

      // unknown playlist
      if (iPlaylist == PLAYLIST_NONE)
      {
        CLog::Log(LOGERROR,"Playlist.PlayOffset called with unknown playlist: %s", strPlaylist.c_str());
        return false;
      }

      // user wants to play the 'other' playlist
      if (iPlaylist != g_playlistPlayer.GetCurrentPlaylist())
      {
        g_application.StopPlaying();
        g_playlistPlayer.Reset();
        g_playlistPlayer.SetCurrentPlaylist(iPlaylist);
      }
    }
    // play the desired offset
    int pos = atol(strPos.c_str());
    // playlist is already playing
    if (g_application.m_pPlayer->IsPlaying())
      g_playlistPlayer.PlayNext(pos);
    // we start playing the 'other' playlist so we need to use play to initialize the player state
    else
      g_playlistPlayer.Play(pos);
  }
  else if (execute == "playlist.clear")
  {
    g_playlistPlayer.Clear();
  }
#ifdef HAS_DVD_DRIVE
  else if (execute == "ejecttray")
  {
    g_mediaManager.ToggleTray();
  }
#endif
  else if (execute == "playdvd")
  {
#ifdef HAS_DVD_DRIVE
    bool restart = false;
    if (params.size() > 0 && StringUtils::EqualsNoCase(params[0], "restart"))
      restart = true;
    CAutorun::PlayDisc(g_mediaManager.GetDiscPath(), true, restart);
#endif
  }
  else if (execute == "ripcd")
  {
#ifdef HAS_CDDA_RIPPER
    CCDDARipper::GetInstance().RipCD();
#endif
  }
  else if (execute == "wakeonlan")
  {
    g_application.getNetwork().WakeOnLan((char*)params[0].c_str());
  }
  else if (execute == "toggledpms")
  {
    g_application.ToggleDPMS(true);
  }
  else if (execute == "toggledebug")
  {
    bool debug = CSettings::GetInstance().GetBool(CSettings::SETTING_DEBUG_SHOWLOGINFO);
    CSettings::GetInstance().SetBool(CSettings::SETTING_DEBUG_SHOWLOGINFO, !debug);
    g_advancedSettings.SetDebugMode(!debug);
  }
  else if (execute == "startandroidactivity" && !params.empty())
  {
    CApplicationMessenger::GetInstance().PostMsg(TMSG_START_ANDROID_ACTIVITY, -1, -1, static_cast<void*>(&params));
  }
  else
    return CInputManager::GetInstance().ExecuteBuiltin(execute, params);
  return 0;
}
