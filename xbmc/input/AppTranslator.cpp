/*
 *      Copyright (C) 2017 Team Kodi
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "AppTranslator.h"
#include "Key.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

#include <map>

namespace
{

using ActionName = std::string;
using CommandID = uint32_t;

#ifdef TARGET_WINDOWS
static const std::map<ActionName, CommandID> AppCommands =
{
    { "browser_back"             , APPCOMMAND_BROWSER_BACKWARD },
    { "browser_forward"          , APPCOMMAND_BROWSER_FORWARD },
    { "browser_refresh"          , APPCOMMAND_BROWSER_REFRESH },
    { "browser_stop"             , APPCOMMAND_BROWSER_STOP },
    { "browser_search"           , APPCOMMAND_BROWSER_SEARCH },
    { "browser_favorites"        , APPCOMMAND_BROWSER_FAVORITES },
    { "browser_home"             , APPCOMMAND_BROWSER_HOME },
    { "volume_mute"              , APPCOMMAND_VOLUME_MUTE },
    { "volume_down"              , APPCOMMAND_VOLUME_DOWN },
    { "volume_up"                , APPCOMMAND_VOLUME_UP },
    { "next_track"               , APPCOMMAND_MEDIA_NEXTTRACK },
    { "prev_track"               , APPCOMMAND_MEDIA_PREVIOUSTRACK },
    { "stop"                     , APPCOMMAND_MEDIA_STOP },
    { "play_pause"               , APPCOMMAND_MEDIA_PLAY_PAUSE },
    { "launch_mail"              , APPCOMMAND_LAUNCH_MAIL },
    { "launch_media_select"      , APPCOMMAND_LAUNCH_MEDIA_SELECT },
    { "launch_app1"              , APPCOMMAND_LAUNCH_APP1 },
    { "launch_app2"              , APPCOMMAND_LAUNCH_APP2 },
    { "play"                     , APPCOMMAND_MEDIA_PLAY },
    { "pause"                    , APPCOMMAND_MEDIA_PAUSE },
    { "fastforward"              , APPCOMMAND_MEDIA_FAST_FORWARD },
    { "rewind"                   , APPCOMMAND_MEDIA_REWIND },
    { "channelup"                , APPCOMMAND_MEDIA_CHANNEL_UP },
    { "channeldown"              , APPCOMMAND_MEDIA_CHANNEL_DOWN }
};
#endif

} // anonymous namespace

uint32_t CAppTranslator::TranslateAppCommand(const std::string &szButton)
{
#ifdef TARGET_WINDOWS
  std::string strAppCommand = szButton;
  StringUtils::ToLower(strAppCommand);

  auto it = AppCommands.find(strAppCommand);
  if (it != AppCommands.end())
    return it->second | KEY_APPCOMMAND;

  CLog::Log(LOGERROR, "%s: Can't find appcommand %s", __FUNCTION__, szButton.c_str());
#endif

  return 0;
}
