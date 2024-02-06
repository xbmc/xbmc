/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ActionTranslator.h"

#include "ActionIDs.h"
#include "interfaces/builtins/Builtins.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <map>

using namespace KODI;
using namespace ACTION;

namespace
{
using ActionName = std::string;
using ActionID = unsigned int;

static const std::map<ActionName, ActionID> ActionMappings = {
    {"left", ACTION_MOVE_LEFT},
    {"right", ACTION_MOVE_RIGHT},
    {"up", ACTION_MOVE_UP},
    {"down", ACTION_MOVE_DOWN},
    {"pageup", ACTION_PAGE_UP},
    {"pagedown", ACTION_PAGE_DOWN},
    {"select", ACTION_SELECT_ITEM},
    {"highlight", ACTION_HIGHLIGHT_ITEM},
    {"parentdir", ACTION_NAV_BACK}, // backward compatibility
    {"parentfolder", ACTION_PARENT_DIR},
    {"back", ACTION_NAV_BACK},
    {"menu", ACTION_MENU},
    {"previousmenu", ACTION_PREVIOUS_MENU},
    {"info", ACTION_SHOW_INFO},
    {"pause", ACTION_PAUSE},
    {"stop", ACTION_STOP},
    {"skipnext", ACTION_NEXT_ITEM},
    {"skipprevious", ACTION_PREV_ITEM},
    {"fullscreen", ACTION_SHOW_GUI},
    {"aspectratio", ACTION_ASPECT_RATIO},
    {"stepforward", ACTION_STEP_FORWARD},
    {"stepback", ACTION_STEP_BACK},
    {"bigstepforward", ACTION_BIG_STEP_FORWARD},
    {"bigstepback", ACTION_BIG_STEP_BACK},
    {"chapterorbigstepforward", ACTION_CHAPTER_OR_BIG_STEP_FORWARD},
    {"chapterorbigstepback", ACTION_CHAPTER_OR_BIG_STEP_BACK},
    {"osd", ACTION_SHOW_OSD},
    {"showsubtitles", ACTION_SHOW_SUBTITLES},
    {"nextsubtitle", ACTION_NEXT_SUBTITLE},
    {"browsesubtitle", ACTION_BROWSE_SUBTITLE},
    {"cyclesubtitle", ACTION_CYCLE_SUBTITLE},
    {"playerdebug", ACTION_PLAYER_DEBUG},
    {"playerdebugvideo", ACTION_PLAYER_DEBUG_VIDEO},
    {"codecinfo", ACTION_PLAYER_PROCESS_INFO},
    {"playerprocessinfo", ACTION_PLAYER_PROCESS_INFO},
    {"playerprogramselect", ACTION_PLAYER_PROGRAM_SELECT},
    {"playerresolutionselect", ACTION_PLAYER_RESOLUTION_SELECT},
    {"nextpicture", ACTION_NEXT_PICTURE},
    {"previouspicture", ACTION_PREV_PICTURE},
    {"zoomout", ACTION_ZOOM_OUT},
    {"zoomin", ACTION_ZOOM_IN},
    {"playlist", ACTION_SHOW_PLAYLIST},
    {"queue", ACTION_QUEUE_ITEM},
    {"playnext", ACTION_QUEUE_ITEM_NEXT},
    {"zoomnormal", ACTION_ZOOM_LEVEL_NORMAL},
    {"zoomlevel1", ACTION_ZOOM_LEVEL_1},
    {"zoomlevel2", ACTION_ZOOM_LEVEL_2},
    {"zoomlevel3", ACTION_ZOOM_LEVEL_3},
    {"zoomlevel4", ACTION_ZOOM_LEVEL_4},
    {"zoomlevel5", ACTION_ZOOM_LEVEL_5},
    {"zoomlevel6", ACTION_ZOOM_LEVEL_6},
    {"zoomlevel7", ACTION_ZOOM_LEVEL_7},
    {"zoomlevel8", ACTION_ZOOM_LEVEL_8},
    {"zoomlevel9", ACTION_ZOOM_LEVEL_9},
    {"nextcalibration", ACTION_CALIBRATE_SWAP_ARROWS},
    {"resetcalibration", ACTION_CALIBRATE_RESET},
    {"analogmove", ACTION_ANALOG_MOVE},
    {"analogmovexleft", ACTION_ANALOG_MOVE_X_LEFT},
    {"analogmovexright", ACTION_ANALOG_MOVE_X_RIGHT},
    {"analogmoveyup", ACTION_ANALOG_MOVE_Y_UP},
    {"analogmoveydown", ACTION_ANALOG_MOVE_Y_DOWN},
    {"rotate", ACTION_ROTATE_PICTURE_CW},
    {"rotateccw", ACTION_ROTATE_PICTURE_CCW},
    {"close", ACTION_NAV_BACK}, // backwards compatibility
    {"subtitledelayminus", ACTION_SUBTITLE_DELAY_MIN},
    {"subtitledelay", ACTION_SUBTITLE_DELAY},
    {"subtitledelayplus", ACTION_SUBTITLE_DELAY_PLUS},
    {"audiodelayminus", ACTION_AUDIO_DELAY_MIN},
    {"audiodelay", ACTION_AUDIO_DELAY},
    {"audiodelayplus", ACTION_AUDIO_DELAY_PLUS},
    {"subtitleshiftup", ACTION_SUBTITLE_VSHIFT_UP},
    {"subtitleshiftdown", ACTION_SUBTITLE_VSHIFT_DOWN},
    {"subtitlealign", ACTION_SUBTITLE_ALIGN},
    {"audionextlanguage", ACTION_AUDIO_NEXT_LANGUAGE},
    {"verticalshiftup", ACTION_VSHIFT_UP},
    {"verticalshiftdown", ACTION_VSHIFT_DOWN},
    {"nextresolution", ACTION_CHANGE_RESOLUTION},
    {"audiotoggledigital", ACTION_TOGGLE_DIGITAL_ANALOG},
    {"number0", REMOTE_0},
    {"number1", REMOTE_1},
    {"number2", REMOTE_2},
    {"number3", REMOTE_3},
    {"number4", REMOTE_4},
    {"number5", REMOTE_5},
    {"number6", REMOTE_6},
    {"number7", REMOTE_7},
    {"number8", REMOTE_8},
    {"number9", REMOTE_9},
    {"smallstepback", ACTION_SMALL_STEP_BACK},
    {"fastforward", ACTION_PLAYER_FORWARD},
    {"rewind", ACTION_PLAYER_REWIND},
    {"tempoup", ACTION_PLAYER_INCREASE_TEMPO},
    {"tempodown", ACTION_PLAYER_DECREASE_TEMPO},
    {"play", ACTION_PLAYER_PLAY},
    {"playpause", ACTION_PLAYER_PLAYPAUSE},
    {"switchplayer", ACTION_SWITCH_PLAYER},
    {"delete", ACTION_DELETE_ITEM},
    {"copy", ACTION_COPY_ITEM},
    {"move", ACTION_MOVE_ITEM},
    {"screenshot", ACTION_TAKE_SCREENSHOT},
    {"rename", ACTION_RENAME_ITEM},
    {"togglewatched", ACTION_TOGGLE_WATCHED},
    {"scanitem", ACTION_SCAN_ITEM},
    {"reloadkeymaps", ACTION_RELOAD_KEYMAPS},
    {"volumeup", ACTION_VOLUME_UP},
    {"volumedown", ACTION_VOLUME_DOWN},
    {"mute", ACTION_MUTE},
    {"backspace", ACTION_BACKSPACE},
    {"scrollup", ACTION_SCROLL_UP},
    {"scrolldown", ACTION_SCROLL_DOWN},
    {"analogfastforward", ACTION_ANALOG_FORWARD},
    {"analogrewind", ACTION_ANALOG_REWIND},
    {"moveitemup", ACTION_MOVE_ITEM_UP},
    {"moveitemdown", ACTION_MOVE_ITEM_DOWN},
    {"contextmenu", ACTION_CONTEXT_MENU},
    {"shift", ACTION_SHIFT},
    {"symbols", ACTION_SYMBOLS},
    {"cursorleft", ACTION_CURSOR_LEFT},
    {"cursorright", ACTION_CURSOR_RIGHT},
    {"showtime", ACTION_SHOW_OSD_TIME},
    {"analogseekforward", ACTION_ANALOG_SEEK_FORWARD},
    {"analogseekback", ACTION_ANALOG_SEEK_BACK},
    {"showpreset", ACTION_VIS_PRESET_SHOW},
    {"nextpreset", ACTION_VIS_PRESET_NEXT},
    {"previouspreset", ACTION_VIS_PRESET_PREV},
    {"lockpreset", ACTION_VIS_PRESET_LOCK},
    {"randompreset", ACTION_VIS_PRESET_RANDOM},
    {"increasevisrating", ACTION_VIS_RATE_PRESET_PLUS},
    {"decreasevisrating", ACTION_VIS_RATE_PRESET_MINUS},
    {"showvideomenu", ACTION_SHOW_VIDEOMENU},
    {"enter", ACTION_ENTER},
    {"increaserating", ACTION_INCREASE_RATING},
    {"decreaserating", ACTION_DECREASE_RATING},
    {"setrating", ACTION_SET_RATING},
    {"togglefullscreen", ACTION_TOGGLE_FULLSCREEN},
    {"nextscene", ACTION_NEXT_SCENE},
    {"previousscene", ACTION_PREV_SCENE},
    {"nextletter", ACTION_NEXT_LETTER},
    {"prevletter", ACTION_PREV_LETTER},
    {"jumpsms2", ACTION_JUMP_SMS2},
    {"jumpsms3", ACTION_JUMP_SMS3},
    {"jumpsms4", ACTION_JUMP_SMS4},
    {"jumpsms5", ACTION_JUMP_SMS5},
    {"jumpsms6", ACTION_JUMP_SMS6},
    {"jumpsms7", ACTION_JUMP_SMS7},
    {"jumpsms8", ACTION_JUMP_SMS8},
    {"jumpsms9", ACTION_JUMP_SMS9},
    {"filter", ACTION_FILTER},
    {"filterclear", ACTION_FILTER_CLEAR},
    {"filtersms2", ACTION_FILTER_SMS2},
    {"filtersms3", ACTION_FILTER_SMS3},
    {"filtersms4", ACTION_FILTER_SMS4},
    {"filtersms5", ACTION_FILTER_SMS5},
    {"filtersms6", ACTION_FILTER_SMS6},
    {"filtersms7", ACTION_FILTER_SMS7},
    {"filtersms8", ACTION_FILTER_SMS8},
    {"filtersms9", ACTION_FILTER_SMS9},
    {"firstpage", ACTION_FIRST_PAGE},
    {"lastpage", ACTION_LAST_PAGE},
    {"guiprofile", ACTION_GUIPROFILE_BEGIN},
    {"red", ACTION_TELETEXT_RED},
    {"green", ACTION_TELETEXT_GREEN},
    {"yellow", ACTION_TELETEXT_YELLOW},
    {"blue", ACTION_TELETEXT_BLUE},
    {"increasepar", ACTION_INCREASE_PAR},
    {"decreasepar", ACTION_DECREASE_PAR},
    {"volampup", ACTION_VOLAMP_UP},
    {"volampdown", ACTION_VOLAMP_DOWN},
    {"volumeamplification", ACTION_VOLAMP},
    {"createbookmark", ACTION_CREATE_BOOKMARK},
    {"createepisodebookmark", ACTION_CREATE_EPISODE_BOOKMARK},
    {"settingsreset", ACTION_SETTINGS_RESET},
    {"settingslevelchange", ACTION_SETTINGS_LEVEL_CHANGE},
    {"togglefont", ACTION_TOGGLE_FONT},
    {"videonextstream", ACTION_VIDEO_NEXT_STREAM},

    // 3D movie playback/GUI
    {"stereomode", ACTION_STEREOMODE_SELECT}, // cycle 3D modes, for now an alias for next
    {"nextstereomode", ACTION_STEREOMODE_NEXT},
    {"previousstereomode", ACTION_STEREOMODE_PREVIOUS},
    {"togglestereomode", ACTION_STEREOMODE_TOGGLE},
    {"stereomodetomono", ACTION_STEREOMODE_TOMONO},

    // HDR display support
    {"hdrtoggle", ACTION_HDR_TOGGLE},

    // Tone mapping
    {"cycletonemapmethod", ACTION_CYCLE_TONEMAP_METHOD},

    // PVR actions
    {"channelup", ACTION_CHANNEL_UP},
    {"channeldown", ACTION_CHANNEL_DOWN},
    {"previouschannelgroup", ACTION_PREVIOUS_CHANNELGROUP},
    {"nextchannelgroup", ACTION_NEXT_CHANNELGROUP},
    {"playpvr", ACTION_PVR_PLAY},
    {"playpvrtv", ACTION_PVR_PLAY_TV},
    {"playpvrradio", ACTION_PVR_PLAY_RADIO},
    {"record", ACTION_RECORD},
    {"togglecommskip", ACTION_TOGGLE_COMMSKIP},
    {"showtimerrule", ACTION_PVR_SHOW_TIMER_RULE},
    {"channelnumberseparator", ACTION_CHANNEL_NUMBER_SEP},

    // Mouse actions
    {"leftclick", ACTION_MOUSE_LEFT_CLICK},
    {"rightclick", ACTION_MOUSE_RIGHT_CLICK},
    {"middleclick", ACTION_MOUSE_MIDDLE_CLICK},
    {"doubleclick", ACTION_MOUSE_DOUBLE_CLICK},
    {"longclick", ACTION_MOUSE_LONG_CLICK},
    {"wheelup", ACTION_MOUSE_WHEEL_UP},
    {"wheeldown", ACTION_MOUSE_WHEEL_DOWN},
    {"mousedrag", ACTION_MOUSE_DRAG},
    {"mousedragend", ACTION_MOUSE_DRAG_END},
    {"mousemove", ACTION_MOUSE_MOVE},

    // Touch
    {"tap", ACTION_TOUCH_TAP},
    {"longpress", ACTION_TOUCH_LONGPRESS},
    {"pangesture", ACTION_GESTURE_PAN},
    {"zoomgesture", ACTION_GESTURE_ZOOM},
    {"rotategesture", ACTION_GESTURE_ROTATE},
    {"swipeleft", ACTION_GESTURE_SWIPE_LEFT},
    {"swiperight", ACTION_GESTURE_SWIPE_RIGHT},
    {"swipeup", ACTION_GESTURE_SWIPE_UP},
    {"swipedown", ACTION_GESTURE_SWIPE_DOWN},

    // Voice
    {"voicerecognizer", ACTION_VOICE_RECOGNIZE},

    // Do nothing / error action
    {"error", ACTION_ERROR},
    {"noop", ACTION_NOOP}};
} // namespace

void CActionTranslator::GetActions(std::vector<std::string>& actionList)
{
  actionList.reserve(ActionMappings.size());
  for (auto& actionMapping : ActionMappings)
    actionList.push_back(actionMapping.first);
}

bool CActionTranslator::IsAnalog(unsigned int actionID)
{
  switch (actionID)
  {
    case ACTION_ANALOG_SEEK_FORWARD:
    case ACTION_ANALOG_SEEK_BACK:
    case ACTION_SCROLL_UP:
    case ACTION_SCROLL_DOWN:
    case ACTION_ANALOG_FORWARD:
    case ACTION_ANALOG_REWIND:
    case ACTION_ANALOG_MOVE:
    case ACTION_ANALOG_MOVE_X_LEFT:
    case ACTION_ANALOG_MOVE_X_RIGHT:
    case ACTION_ANALOG_MOVE_Y_UP:
    case ACTION_ANALOG_MOVE_Y_DOWN:
    case ACTION_CURSOR_LEFT:
    case ACTION_CURSOR_RIGHT:
    case ACTION_VOLUME_UP:
    case ACTION_VOLUME_DOWN:
    case ACTION_ZOOM_IN:
    case ACTION_ZOOM_OUT:
      return true;
    default:
      return false;
  }
}

bool CActionTranslator::TranslateString(std::string strAction, unsigned int& actionId)
{
  actionId = ACTION_NONE;

  if (strAction.empty())
    return false;

  StringUtils::ToLower(strAction);

  auto it = ActionMappings.find(strAction);
  if (it != ActionMappings.end())
    actionId = it->second;
  else if (CBuiltins::GetInstance().HasCommand(strAction))
    actionId = ACTION_BUILT_IN_FUNCTION;

  if (actionId == ACTION_NONE)
  {
    CLog::Log(LOGERROR, "Keymapping error: no such action '{}' defined", strAction);
    return false;
  }

  return true;
}
