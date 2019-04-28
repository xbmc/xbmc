/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIInfoManager.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <iterator>
#include <memory>

#include "Application.h"
#include "FileItem.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "Util.h"
#include "cores/DataCacheCore.h"
#include "filesystem/File.h"
#include "guilib/guiinfo/GUIInfo.h"
#include "guilib/guiinfo/GUIInfoHelper.h"
#include "guilib/guiinfo/GUIInfoLabels.h"
#include "input/WindowTranslator.h"
#include "interfaces/AnnouncementManager.h"
#include "interfaces/info/InfoExpression.h"
#include "messaging/ApplicationMessenger.h"
#include "settings/SkinSettings.h"
#include "utils/CharsetConverter.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

using namespace KODI::GUILIB;
using namespace KODI::GUILIB::GUIINFO;
using namespace INFO;

bool InfoBoolComparator(const InfoPtr &right, const InfoPtr &left)
{
  return *right < *left;
}

CGUIInfoManager::CGUIInfoManager(void)
: m_currentFile(new CFileItem),
  m_bools(&InfoBoolComparator)
{
}

CGUIInfoManager::~CGUIInfoManager(void)
{
  delete m_currentFile;
}

void CGUIInfoManager::Initialize()
{
  KODI::MESSAGING::CApplicationMessenger::GetInstance().RegisterReceiver(this);
}

/// \brief Translates a string as given by the skin into an int that we use for more
/// efficient retrieval of data. Can handle combined strings on the form
/// Player.Caching + VideoPlayer.IsFullscreen (Logical and)
/// Player.HasVideo | Player.HasAudio (Logical or)
int CGUIInfoManager::TranslateString(const std::string &condition)
{
  // translate $LOCALIZE as required
  std::string strCondition(CGUIInfoLabel::ReplaceLocalize(condition));
  return TranslateSingleString(strCondition);
}

typedef struct
{
  const char *str;
  int  val;
} infomap;

/// \page modules__infolabels_boolean_conditions Infolabels and Boolean conditions
/// \tableofcontents
///
/// \section modules__infolabels_boolean_conditions_Description Description
/// Skins can use boolean conditions with the <b>\<visible\></b> tag or with condition
/// attributes. Scripts can read boolean conditions with
/// <b>xbmc.getCondVisibility(condition)</b>.
///
/// Skins can use infolabels with <b>$INFO[infolabel]</b> or the <b>\<info\></b> tag. Scripts
/// can read infolabels with <b>xbmc.getInfoLabel('infolabel')</b>.
/// 
/// @todo [docs] Improve the description and create links for functions
/// @todo [docs] Separate boolean conditions from infolabels
/// @todo [docs] Order items alphabetically within subsections for a better search experience
/// @todo [docs] Order subsections alphabetically
/// @todo [docs] Use links instead of bold values for infolabels/bools
/// so we can use a link to point users when providing help
/// 


/// \page modules__infolabels_boolean_conditions
/// \section modules_list_infolabels_booleans List of Infolabels and Boolean conditions
/// \subsection modules__infolabels_boolean_conditions_GlobalBools Global
/// \table_start
///   \table_h3{ Labels, Type, Description }
///   \table_row3{   <b>`true`</b>,
///                  \anchor Global_True
///                  _boolean_,
///     @return Always evaluates to **true**.
///     <p>
///   }
///   \table_row3{   <b>`false`</b>,
///                  \anchor Global_False
///                  _boolean_,
///     @return Always evaluates to **false**.
///     <p>
///   }
///   \table_row3{   <b>`yes`</b>,
///                  \anchor Global_Yes
///                  _boolean_,
///     @return same as \link Global_True `true` \endlink.
///     <p>
///   }
///   \table_row3{   <b>`no`</b>,
///                  \anchor Global_No
///                  _boolean_,
///     @return same as \link Global_False `false` \endlink.
///     <p>
///   }
/// \table_end
///
/// -----------------------------------------------------------------------------


/// \page modules__infolabels_boolean_conditions
/// \subsection modules__infolabels_boolean_conditions_String String
/// \table_start
///   \table_h3{ Labels, Type, Description }
///   \table_row3{   <b>`String.IsEmpty(info)`</b>,
///                  \anchor String_IsEmpty
///                  _boolean_,
///     @return **True** if the info is empty.
///     @param info - infolabel
///     @note **Example of info:** \link ListItem_Title `ListItem.Title` \endlink \, 
///     \link ListItem_Genre `ListItem.Genre` \endlink.
///     Please note that string can also be a `$LOCALIZE[]`.
///     Also note that in a panelview or similar this only works on the focused item
///     <p><hr>
///     @skinning_v17 **[New Boolean Condition]** \link String_IsEmpty `String.IsEmpty(info)`\endlink
///     <p>
///   }
///   \table_row3{   <b>`String.IsEqual(info\,string)`</b>,
///                  \anchor String_IsEqual
///                  _boolean_,
///     @return **True** if the info is equal to the given string.
///     @param info - infolabel
///     @param string - comparison string
///     @note **Example of info:** \link ListItem_Title `ListItem.Title` \endlink \, 
///     \link ListItem_Genre `ListItem.Genre` \endlink.
///     Please note that string can also be a `$LOCALIZE[]`.
///     Also note that in a panelview or similar this only works on the focused item
///     <p><hr>
///     @skinning_v17 **[New Boolean Condition]** \link String_IsEqual `String.IsEqual(info\,string)`\endlink
///     <p>
///   }
///   \table_row3{   <b>`String.StartsWith(info\,substring)`</b>,
///                  \anchor String_StartsWith
///                  _boolean_,
///     @return **True** if the info starts with the given substring.
///     @param info - infolabel
///     @param substring - substring to check
///     @note **Example of info:** \link ListItem_Title `ListItem.Title` \endlink \, 
///     \link ListItem_Genre `ListItem.Genre` \endlink.
///     Please note that string can also be a `$LOCALIZE[]`.
///     Also note that in a panelview or similar this only works on the focused item
///     <p><hr>
///     @skinning_v17 **[New Boolean Condition]** \link String_StartsWith `String.StartsWith(info\,substring)`\endlink
///     <p>
///   }
///   \table_row3{   <b>`String.EndsWith(info\,substring)`</b>,
///                  \anchor String_EndsWith
///                  _boolean_,
///     @return **True** if the info ends with the given substring.
///     @param info - infolabel
///     @param substring - substring to check
///     @note **Example of info:** \link ListItem_Title `ListItem.Title` \endlink \, 
///     \link ListItem_Genre `ListItem.Genre` \endlink.
///     Please note that string can also be a `$LOCALIZE[]`.
///     Also note that in a panelview or similar this only works on the focused item
///     <p><hr>
///     @skinning_v17 **[New Boolean Condition]** \link String_EndsWith `String.EndsWith(info\,substring)`\endlink
///     <p>
///   }
///   \table_row3{   <b>`String.Contains(info\,substring)`</b>,
///                  \anchor String_Contains
///                  _boolean_,
///     @return **True** if the info contains the given substring.
///     @param info - infolabel
///     @param substring - substring to check
///     @note **Example of info:** \link ListItem_Title `ListItem.Title` \endlink \, 
///     \link ListItem_Genre `ListItem.Genre` \endlink.
///     Please note that string can also be a `$LOCALIZE[]`.
///     Also note that in a panelview or similar this only works on the focused item
///     <p><hr>
///     @skinning_v17 **[New Boolean Condition]** \link String_Contains `String.Contains(info\,substring)`\endlink
///     <p>
///   }
/// \table_end
///
/// -----------------------------------------------------------------------------


const infomap string_bools[] =   {{ "isempty",          STRING_IS_EMPTY },
                                  { "isequal",          STRING_IS_EQUAL },
                                  { "startswith",       STRING_STARTS_WITH },
                                  { "endswith",         STRING_ENDS_WITH },
                                  { "contains",         STRING_CONTAINS }};


/// \page modules__infolabels_boolean_conditions
/// \subsection modules__infolabels_boolean_conditions_Integer Integer
/// \table_start
///   \table_h3{ Labels, Type, Description }
///   \table_row3{   <b>`Integer.IsEqual(info\,number)`</b>,
///                  \anchor Integer_IsEqual
///                  _boolean_,
///     @return **True** if the value of the infolabel is equal to the supplied number.
///     @param info - infolabel
///     @param number - number to compare
///     @note **Example:** `Integer.IsEqual(ListItem.Year\,2000)`
///     <p><hr>
///     @skinning_v17 **[New Boolean Condition]** \link Integer_IsEqual `Integer.IsEqual(info\,number)`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Integer.IsGreater(info\,number)`</b>,
///                  \anchor Integer_IsGreater
///                  _boolean_,
///     @return **True** if the value of the infolabel is greater than to the supplied number.
///     @param info - infolabel
///     @param number - number to compare
///     @note **Example:** `Integer.IsGreater(ListItem.Year\,2000)`
///     <p><hr>
///     @skinning_v17 **[New Boolean Condition]** \link Integer_IsGreater `Integer.IsGreater(info\,number)`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Integer.IsGreaterOrEqual(info\,number)`</b>,
///                  \anchor Integer_IsGreaterOrEqual
///                  _boolean_,
///     @return **True** if the value of the infolabel is greater or equal to the supplied number.
///     @param info - infolabel
///     @param number - number to compare
///     @note **Example:** `Integer.IsGreaterOrEqual(ListItem.Year\,2000)`
///     <p><hr>
///     @skinning_v17 **[New Boolean Condition]** \link Integer_IsGreaterOrEqual `Integer.IsGreaterOrEqual(info\,number)`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Integer.IsLess(info\,number)`</b>,
///                  \anchor Integer_IsLess
///                  _boolean_,
///     @return **True** if the value of the infolabel is less than the supplied number.
///     @param info - infolabel
///     @param number - number to compare
///     @note **Example:** `Integer.IsLess(ListItem.Year\,2000)`
///     <p><hr>
///     @skinning_v17 **[New Boolean Condition]** \link Integer_IsLess `Integer.IsLess(info\,number)`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Integer.IsLessOrEqual(info\,number)`</b>,
///                  \anchor Integer_IsLessOrEqual
///                  _boolean_,
///     @return **True** if the value of the infolabel is less or equal to the supplied number.
///     @param info - infolabel
///     @param number - number to compare
///     @note **Example:** `Integer.IsLessOrEqual(ListItem.Year\,2000)`
///     <p><hr>
///     @skinning_v17 **[New Boolean Condition]** \link Integer_IsLessOrEqual `Integer.IsLessOrEqual(info\,number)`\endlink
///     <p>
///   }
/// \table_end
///
/// -----------------------------------------------------------------------------


const infomap integer_bools[] =  {{ "isequal",          INTEGER_IS_EQUAL },
                                  { "isgreater",        INTEGER_GREATER_THAN },
                                  { "isgreaterorequal", INTEGER_GREATER_OR_EQUAL },
                                  { "isless",           INTEGER_LESS_THAN },
                                  { "islessorequal",    INTEGER_LESS_OR_EQUAL }};


/// \page modules__infolabels_boolean_conditions
/// \subsection modules__infolabels_boolean_conditions_Player Player
/// \table_start
///   \table_h3{ Labels, Type, Description }
///   \table_row3{   <b>`Player.HasAudio`</b>,
///                  \anchor Player_HasAudio
///                  _boolean_,
///     @return **True** if the player has an audio file.
///     <p>
///   }
///   \table_row3{   <b>`Player.HasGame`</b>,
///                  \anchor Player_HasGame
///                  _boolean_,
///     @return **True** if the player has a game file (RETROPLAYER).
///     <p><hr>
///     @skinning_v18 **[New Boolean Condition]** \link Player_HasGame `Player.HasGame`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Player.HasMedia`</b>,
///                  \anchor Player_HasMedia
///                  _boolean_,
///     @return **True** if the player has an audio or video file.
///     <p>
///   }
///   \table_row3{   <b>`Player.HasVideo`</b>,
///                  \anchor Player_HasVideo
///                  _boolean_,
///     @return **True** if the player has a video file.
///     <p>
///   }
///   \table_row3{   <b>`Player.Paused`</b>,
///                  \anchor Player_Paused
///                  _boolean_,
///     @return **True** if the player is paused.
///     <p>
///   }
///   \table_row3{   <b>`Player.Playing`</b>,
///                  \anchor Player_Playing
///                  _boolean_,
///     @return **True** if the player is currently playing (i.e. not ffwding\,
///     rewinding or paused.)
///     <p>
///   }
///   \table_row3{   <b>`Player.Rewinding`</b>,
///                  \anchor Player_Rewinding
///                  _boolean_,
///     @return **True** if the player is rewinding.
///     <p>
///   }
///   \table_row3{   <b>`Player.Rewinding2x`</b>,
///                  \anchor Player_Rewinding2x
///                  _boolean_,
///     @return **True** if the player is rewinding at 2x.
///     <p>
///   }
///   \table_row3{   <b>`Player.Rewinding4x`</b>,
///                  \anchor Player_Rewinding4x
///                  _boolean_,
///     @return **True** if the player is rewinding at 4x.
///     <p>
///   }
///   \table_row3{   <b>`Player.Rewinding8x`</b>,
///                  \anchor Player_Rewinding8x
///                  _boolean_,
///     @return **True** if the player is rewinding at 8x.
///     <p>
///   }
///   \table_row3{   <b>`Player.Rewinding16x`</b>,
///                  \anchor Player_Rewinding16x
///                  _boolean_,
///     @return **True** if the player is rewinding at 16x.
///     <p>
///   }
///   \table_row3{   <b>`Player.Rewinding32x`</b>,
///                  \anchor Player_Rewinding32x
///                  _boolean_,
///     @return **True** if the player is rewinding at 32x.
///     <p>
///   }
///   \table_row3{   <b>`Player.Forwarding`</b>,
///                  \anchor Player_Forwarding
///                  _boolean_,
///     @return **True** if the player is fast forwarding.
///     <p>
///   }
///   \table_row3{   <b>`Player.Forwarding2x`</b>,
///                  \anchor Player_Forwarding2x
///                  _boolean_,
///     @return **True** if the player is fast forwarding at 2x.
///     <p>
///   }
///   \table_row3{   <b>`Player.Forwarding4x`</b>,
///                  \anchor Player_Forwarding4x
///                  _boolean_,
///     @return **True** if the player is fast forwarding at 4x.
///     <p>
///   }
///   \table_row3{   <b>`Player.Forwarding8x`</b>,
///                  \anchor Player_Forwarding8x
///                  _boolean_,
///     @return **True** if the player is fast forwarding at 8x.
///     <p>
///   }
///   \table_row3{   <b>`Player.Forwarding16x`</b>,
///                  \anchor Player_Forwarding16x
///                  _boolean_,
///     @return **True** if the player is fast forwarding at 16x.
///     <p>
///   }
///   \table_row3{   <b>`Player.Forwarding32x`</b>,
///                  \anchor Player_Forwarding32x
///                  _boolean_,
///     @return **True** if the player is fast forwarding at 32x.
///     <p>
///   }
///   \table_row3{   <b>`Player.Caching`</b>,
///                  \anchor Player_Caching
///                  _boolean_,
///     @return **True** if the player is current re-caching data (internet based
///     video playback).
///     <p>
///   }
///   \table_row3{   <b>`Player.DisplayAfterSeek`</b>,
///                  \anchor Player_DisplayAfterSeek
///                  _boolean_,
///     @return **True** for the first 2.5 seconds after a seek.
///     <p>
///   }
///   \table_row3{   <b>`Player.Seekbar`</b>,
///                  \anchor Player_Seekbar
///                  _integer_,
///     @return The percentage of one seek to other position.
///     <p>
///   }
///   \table_row3{   <b>`Player.Seeking`</b>,
///                  \anchor Player_Seeking
///                  _boolean_,
///     @return **True** if a seek is in progress.
///     <p>
///   }
///   \table_row3{   <b>`Player.ShowTime`</b>,
///                  \anchor Player_ShowTime
///                  _boolean_,
///     @return **True** if the user has requested the time to show (occurs in video
///     fullscreen).
///     <p>
///   }
///   \table_row3{   <b>`Player.ShowInfo`</b>,
///                  \anchor Player_ShowInfo
///                  _boolean_,
///     @return **True** if the user has requested the song info to show (occurs in
///     visualisation fullscreen and slideshow).
///     <p>
///   }
///   \table_row3{   <b>`Player.Title`</b>,
///                  \anchor Player_Title
///                  _string_,
///     @return The Musicplayer title for audio and the Videoplayer title for
///     video.
///     <p>
///   }
///   \table_row3{   <b>`Player.Muted`</b>,
///                  \anchor Player_Muted
///                  _boolean_,
///     @return **True** if the volume is muted.
///     <p>
///   }
///   \table_row3{   <b>`Player.HasDuration`</b>,
///                  \anchor Player_HasDuration
///                  _boolean_,
///     @return **True** if Media is not a true stream.
///     <p>
///   }
///   \table_row3{   <b>`Player.Passthrough`</b>,
///                  \anchor Player_Passthrough
///                  _boolean_,
///     @return **True** if the player is using audio passthrough.
///     <p>
///   }
///   \table_row3{   <b>`Player.CacheLevel`</b>,
///                  \anchor Player_CacheLevel
///                  _string_,
///     @return The used cache level as a string with an integer number.
///     <p>
///   }
///   \table_row3{   <b>`Player.Progress`</b>,
///                  \anchor Player_Progress
///                  _integer_,
///     @return The progress position as percentage.
///     <p>
///   }
///   \table_row3{   <b>`Player.ProgressCache`</b>,
///                  \anchor Player_ProgressCache
///                  _integer_,
///     @return How much of the file is cached above current play percentage
///     <p>
///   }
///   \table_row3{   <b>`Player.Volume`</b>,
///                  \anchor Player_Volume
///                  _string_,
///     @return The current player volume with the format `%2.1f` dB
///     <p>
///   }
///   \table_row3{   <b>`Player.SubtitleDelay`</b>,
///                  \anchor Player_SubtitleDelay
///                  _string_,
///     @return The used subtitle delay with the format `%2.3f` s
///     <p>
///   }
///   \table_row3{   <b>`Player.AudioDelay`</b>,
///                  \anchor Player_AudioDelay
///                  _string_,
///     @return The used audio delay with the format `%2.3f` s
///     <p>
///   }
///   \table_row3{   <b>`Player.Chapter`</b>,
///                  \anchor Player_Chapter
///                  _integer_,
///     @return The current chapter of current playing media.
///     <p>
///   }
///   \table_row3{   <b>`Player.ChapterCount`</b>,
///                  \anchor Player_ChapterCount
///                  _integer_,
///     @return The total number of chapters of current playing media.
///     <p>
///   }
///   \table_row3{   <b>`Player.ChapterName`</b>,
///                  \anchor Player_ChapterName
///                  _string_,
///     @return The name of currently used chapter if available.
///     <p>
///   }
///   \table_row3{   <b>`Player.Folderpath`</b>,
///                  \anchor Player_Folderpath
///                  _string_,
///     @return The full path of the currently playing song or movie
///     <p>
///   }
///   \table_row3{   <b>`Player.FilenameAndPath`</b>,
///                  \anchor Player_FilenameAndPath
///                  _string_,
///     @return The full path with filename of the currently 
///     playing song or movie
///     <p>
///   }
///   \table_row3{   <b>`Player.Filename`</b>,
///                  \anchor Player_Filename
///                  _string_,
///     @return The filename of the currently playing media.
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link Player_Filename `Player.Filename`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Player.IsInternetStream`</b>,
///                  \anchor Player_IsInternetStream
///                  _boolean_,
///     @return **True** if the player is playing an internet stream.
///     <p>
///   }
///   \table_row3{   <b>`Player.PauseEnabled`</b>,
///                  \anchor Player_PauseEnabled
///                  _boolean_,
///     @return **True** if played stream is paused.
///     <p>
///   }
///   \table_row3{   <b>`Player.SeekEnabled`</b>,
///                  \anchor Player_SeekEnabled
///                  _boolean_,
///     @return **True** if seek on playing is enabled.
///     <p>
///   }
///   \table_row3{   <b>`Player.ChannelPreviewActive`</b>,
///                  \anchor Player_ChannelPreviewActive
///                  _boolean_,
///     @return **True** if PVR channel preview is active (used 
///     channel tag different from played tag)
///     <p>
///   }
///   \table_row3{   <b>`Player.TempoEnabled`</b>,
///                  \anchor Player_TempoEnabled
///                  _boolean_,
///     @return **True** if player supports tempo (i.e. speed up/down normal 
///     playback speed)
///     <p><hr>
///     @skinning_v17 **[New Boolean Condition]** \link Player_TempoEnabled `Player.TempoEnabled`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Player.IsTempo`</b>,
///                  \anchor Player_IsTempo
///                  _boolean_,
///     @return **True** if player has tempo (i.e. is playing with a playback speed higher or
///     lower than normal playback speed)
///     <p><hr>
///     @skinning_v17 **[New Boolean Condition]** \link Player_IsTempo `Player.IsTempo`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Player.PlaySpeed`</b>,
///                  \anchor Player_PlaySpeed
///                  _string_,
///     @return The player playback speed with the format `%1.2f` (1.00 means normal 
///     playback speed).
///     @note For Tempo\, the default range is 0.80 - 1.50 (it can be changed 
///     in advanced settings). If \ref Player_PlaySpeed "Player.PlaySpeed" returns a value different from 1.00
///     and \ref Player_IsTempo "Player.IsTempo" is false it means the player is in ff/rw mode.
///     <p>
///   }
///   \table_row3{   <b>`Player.HasResolutions`</b>,
///                  \anchor Player_HasResolutions
///                  _boolean_,
///     @return **True** if the player is allowed to switch resolution and refresh rate 
///     (i.e. if whitelist modes are configured in Kodi's System/Display settings)
///     <p><hr>
///     @skinning_v18 **[New Boolean Condition]** \link Player_HasResolutions `Player.HasResolutions`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Player.HasPrograms`</b>,
///                  \anchor Player_HasPrograms
///                  _boolean_,
///     @return **True** if the media file being played has programs\, i.e. groups of streams. 
///     @note Ex: if a media file has multiple streams (quality\, channels\, etc) a program represents
///     a particular stream combo.
///     <p>
///   }
///   \table_row3{   <b>`Player.FrameAdvance`</b>,
///                  \anchor Player_FrameAdvance
///                  _boolean_,
///     @return **True** if player is in frame advance mode.
///     @note Skins should hide seek bar in this mode
///     <p><hr>
///     @skinning_v18 **[New Boolean Condition]** \link Player_FrameAdvance `Player.FrameAdvance`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Player.Icon`</b>,
///                  \anchor Player_Icon
///                  _string_,
///     @return The thumbnail of the currently playing item. If no thumbnail image exists\,
///     the icon will be returned\, if available.
///     <p><hr>
///     @skinning_v18 **[New Infolabel]** \link Player_Icon `Player.Icon`\endlink
///     <p>  
///   }
///   \table_row3{   <b>`Player.Cutlist`</b>,
///                  \anchor Player_Cutlist
///                  _string_,
///     @return The cutlist of the currently playing item as csv in the format start1\,end1\,start2\,end2\,...
///     Tokens must have values in the range from 0.0 to 100.0. end token must be less or equal than start token.
///     <p><hr>
///     @skinning_v19 **[New Infolabel]** \link Player_Cutlist `Player.Cutlist`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Player.Chapters`</b>,
///                  \anchor Player_Chapters
///                  _string_,
///     @return The chapters of the currently playing item as csv in the format start1\,end1\,start2\,end2\,...
///     Tokens must have values in the range from 0.0 to 100.0. end token must be less or equal than start token.
///     <p><hr>
///     @skinning_v19 **[New Infolabel]** \link Player_Chapters `Player.Chapters`\endlink
///     <p>
///   }
const infomap player_labels[] =  {{ "hasmedia",         PLAYER_HAS_MEDIA },
                                  { "hasaudio",         PLAYER_HAS_AUDIO },
                                  { "hasvideo",         PLAYER_HAS_VIDEO },
                                  { "hasgame",          PLAYER_HAS_GAME },
                                  { "playing",          PLAYER_PLAYING },
                                  { "paused",           PLAYER_PAUSED },
                                  { "rewinding",        PLAYER_REWINDING },
                                  { "forwarding",       PLAYER_FORWARDING },
                                  { "rewinding2x",      PLAYER_REWINDING_2x },
                                  { "rewinding4x",      PLAYER_REWINDING_4x },
                                  { "rewinding8x",      PLAYER_REWINDING_8x },
                                  { "rewinding16x",     PLAYER_REWINDING_16x },
                                  { "rewinding32x",     PLAYER_REWINDING_32x },
                                  { "forwarding2x",     PLAYER_FORWARDING_2x },
                                  { "forwarding4x",     PLAYER_FORWARDING_4x },
                                  { "forwarding8x",     PLAYER_FORWARDING_8x },
                                  { "forwarding16x",    PLAYER_FORWARDING_16x },
                                  { "forwarding32x",    PLAYER_FORWARDING_32x },
                                  { "displayafterseek", PLAYER_DISPLAY_AFTER_SEEK },
                                  { "caching",          PLAYER_CACHING },
                                  { "seekbar",          PLAYER_SEEKBAR },
                                  { "seeking",          PLAYER_SEEKING },
                                  { "showtime",         PLAYER_SHOWTIME },
                                  { "showinfo",         PLAYER_SHOWINFO },
                                  { "muted",            PLAYER_MUTED },
                                  { "hasduration",      PLAYER_HASDURATION },
                                  { "passthrough",      PLAYER_PASSTHROUGH },
                                  { "cachelevel",       PLAYER_CACHELEVEL },
                                  { "title",            PLAYER_TITLE },
                                  { "progress",         PLAYER_PROGRESS },
                                  { "progresscache",    PLAYER_PROGRESS_CACHE },
                                  { "volume",           PLAYER_VOLUME },
                                  { "subtitledelay",    PLAYER_SUBTITLE_DELAY },
                                  { "audiodelay",       PLAYER_AUDIO_DELAY },
                                  { "chapter",          PLAYER_CHAPTER },
                                  { "chaptercount",     PLAYER_CHAPTERCOUNT },
                                  { "chaptername",      PLAYER_CHAPTERNAME },
                                  { "folderpath",       PLAYER_PATH },
                                  { "filenameandpath",  PLAYER_FILEPATH },
                                  { "filename",         PLAYER_FILENAME },
                                  { "isinternetstream", PLAYER_ISINTERNETSTREAM },
                                  { "pauseenabled",     PLAYER_CAN_PAUSE },
                                  { "seekenabled",      PLAYER_CAN_SEEK },
                                  { "channelpreviewactive", PLAYER_IS_CHANNEL_PREVIEW_ACTIVE },
                                  { "tempoenabled",     PLAYER_SUPPORTS_TEMPO },
                                  { "istempo",          PLAYER_IS_TEMPO },
                                  { "playspeed",        PLAYER_PLAYSPEED },
                                  { "hasprograms",      PLAYER_HAS_PROGRAMS },
                                  { "hasresolutions",   PLAYER_HAS_RESOLUTIONS },
                                  { "frameadvance",     PLAYER_FRAMEADVANCE },
                                  { "icon",             PLAYER_ICON },
                                  { "cutlist",          PLAYER_CUTLIST },
                                  { "chapters",         PLAYER_CHAPTERS }};

/// \page modules__infolabels_boolean_conditions
///   \table_row3{   <b>`Player.Art(type)`</b>,
///                  \anchor Player_Art_type
///                  _string_,
///     @return The Image for the defined art type for the current playing ListItem.
///     @param type - The art type. The type is defined by scripts and scrappers and can have any value.
///     Common example values for type are:
///       - fanart
///       - thumb
///       - poster
///       - banner
///       - clearlogo
///       - tvshow.poster
///       - tvshow.banner
///       - etc
///     @todo get a way of centralize all random art strings used in core so we can point users to them
///     while still making it clear they can have any value.
///     <p>
///   }


const infomap player_param[] =   {{ "art",              PLAYER_ITEM_ART }};

/// \page modules__infolabels_boolean_conditions
///   \table_row3{   <b>`Player.SeekTime`</b>,
///                  \anchor Player_SeekTime
///                  _string_,
///     @return The time to which the user is seeking.
///     <p>
///   }
///   \table_row3{   <b>`Player.SeekOffset([format])`</b>,
///                  \anchor Player_SeekOffset_format
///                  _string_,
///     @return The seek offset after a seek press in a given format.
///     @param format [opt] The format of the return time value.
///     See \ref TIME_FORMAT for the list of possible values.
///     <p>
///     @note **Example:** user presses BigStepForward\, player.seekoffset returns +10:00
///     <p>
///   }
///   \table_row3{   <b>`Player.SeekStepSize`</b>,
///                  \anchor Player_SeekStepSize
///                  _string_,
///     @return The seek step size.
///     <p>
///     <hr>
///     @skinning_v15 **[New Infolabel]** \link Player_SeekStepSize `Player.SeekStepSize`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Player.TimeRemaining([format])`</b>,
///                  \anchor Player_TimeRemaining_format
///                  _string_,
///     @return The remaining time of current playing media in a given format.
///     @param format [opt] The format of the return time value.
///     See \ref TIME_FORMAT for the list of possible values.
///     <p>
///   }
///   \table_row3{   <b>`Player.TimeSpeed`</b>,
///                  \anchor Player_TimeSpeed
///                  _string_,
///     @return The time and the playspeed formatted: "1:23 (2x)".
///     <p>
///   }
///   \table_row3{   <b>`Player.Time([format])`</b>,
///                  \anchor Player_Time_format
///                  _string_,
///     @return The elapsed time of current playing media in a given format.
///     @param format [opt] The format of the return time value.
///     See \ref TIME_FORMAT for the list of possible values.
///     <p>
///   }
///   \table_row3{   <b>`Player.Duration([format])`</b>,
///                  \anchor Player_Duration_format
///                  _string_,
///     @return The total duration of the current playing media in a given format.
///     @param format [opt] The format of the return time value.
///     See \ref TIME_FORMAT for the list of possible values.
///     <p>
///   }
///   \table_row3{   <b>`Player.FinishTime([format])`</b>,
///                  \anchor Player_FinishTime_format
///                  _string_,
///     @return The time at which the playing media will end (in a specified format).
///     @param format [opt] The format of the return time value.
///     See \ref TIME_FORMAT for the list of possible values.
///     <p>
///   }
///   \table_row3{   <b>`Player.StartTime([format])`</b>,
///                  \anchor Player_StartTime_format
///                  _string_,
///     @return The time at which the playing media began (in a specified format).
///     @param format [opt] The format of the return time value.
///     See \ref TIME_FORMAT for the list of possible values.
///     <p>
///   }
///   \table_row3{   <b>`Player.SeekNumeric([format])`</b>,
///                  \anchor Player_SeekNumeric_format
///                  _string_,
///     @return The time at which the playing media began (in a specified format).
///     @param format [opt] The format of the return time value.
///     See \ref TIME_FORMAT for the list of possible values.
///     <p>
///   }
const infomap player_times[] =   {{ "seektime",         PLAYER_SEEKTIME },
                                  { "seekoffset",       PLAYER_SEEKOFFSET },
                                  { "seekstepsize",     PLAYER_SEEKSTEPSIZE },
                                  { "timeremaining",    PLAYER_TIME_REMAINING },
                                  { "timespeed",        PLAYER_TIME_SPEED },
                                  { "time",             PLAYER_TIME },
                                  { "duration",         PLAYER_DURATION },
                                  { "finishtime",       PLAYER_FINISH_TIME },
                                  { "starttime",        PLAYER_START_TIME },
                                  { "seeknumeric",      PLAYER_SEEKNUMERIC } };


/// \page modules__infolabels_boolean_conditions
///   \table_row3{   <b>`Player.Process(videohwdecoder)`</b>,
///                  \anchor Player_Process_videohwdecoder
///                  _boolean_,
///     @return **True** if the currently playing video is decoded in hardware.
///     <p><hr>
///     @skinning_v17 **[New Boolean Condition]** \link Player_Process_videohwdecoder `Player.Process(videohwdecoder)`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Player.Process(videodecoder)`</b>,
///                  \anchor Player_Process_videodecoder
///                  _string_,
///     @return The videodecoder name of the currently playing video.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link Player_Process_videodecoder `Player.Process(videodecoder)`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Player.Process(deintmethod)`</b>,
///                  \anchor Player_Process_deintmethod
///                  _string_,
///     @return The deinterlace method of the currently playing video.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link Player_Process_deintmethod `Player.Process(deintmethod)`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Player.Process(pixformat)`</b>,
///                  \anchor Player_Process_pixformat
///                  _string_,
///     @return The pixel format of the currently playing video.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link Player_Process_pixformat `Player.Process(pixformat)`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Player.Process(videowidth)`</b>,
///                  \anchor Player_Process_videowidth
///                  _string_,
///     @return The width of the currently playing video.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link Player_Process_videowidth `Player.Process(videowidth)`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Player.Process(videoheight)`</b>,
///                  \anchor Player_Process_videoheight
///                  _string_,
///     @return The width of the currently playing video.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link Player_Process_videoheight `Player.Process(videoheight)`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Player.Process(videofps)`</b>,
///                  \anchor Player_Process_videofps
///                  _string_,
///     @return The video framerate of the currently playing video.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link Player_Process_videofps `Player.Process(videofps)`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Player.Process(videodar)`</b>,
///                  \anchor Player_Process_videodar
///                  _string_,
///     @return The display aspect ratio of the currently playing video.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link Player_Process_videodar `Player.Process(videodar)`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Player.Process(audiodecoder)`</b>,
///                  \anchor Player_Process_audiodecoder
///                  _string_,
///     @return The audiodecoder name of the currently playing item.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link Player_Process_videodar `Player.Process(audiodecoder)`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Player.Process(audiochannels)`</b>,
///                  \anchor Player_Process_audiochannels
///                  _string_,
///     @return The audiodecoder name of the currently playing item.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link Player_Process_audiochannels `Player.Process(audiochannels)`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Player.Process(audiosamplerate)`</b>,
///                  \anchor Player_Process_audiosamplerate
///                  _string_,
///     @return The samplerate of the currently playing item.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link Player_Process_audiosamplerate `Player.Process(audiosamplerate)`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Player.Process(audiobitspersample)`</b>,
///                  \anchor Player_Process_audiobitspersample
///                  _string_,
///     @return The bits per sample of the currently playing item.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link Player_Process_audiobitspersample `Player.Process(audiobitspersample)`\endlink
///     <p>
///   }
/// \table_end
///
/// -----------------------------------------------------------------------------

const infomap player_process[] =
{
  { "videodecoder", PLAYER_PROCESS_VIDEODECODER },
  { "deintmethod", PLAYER_PROCESS_DEINTMETHOD },
  { "pixformat", PLAYER_PROCESS_PIXELFORMAT },
  { "videowidth", PLAYER_PROCESS_VIDEOWIDTH },
  { "videoheight", PLAYER_PROCESS_VIDEOHEIGHT },
  { "videofps", PLAYER_PROCESS_VIDEOFPS },
  { "videodar", PLAYER_PROCESS_VIDEODAR },
  { "videohwdecoder", PLAYER_PROCESS_VIDEOHWDECODER },
  { "audiodecoder", PLAYER_PROCESS_AUDIODECODER },
  { "audiochannels", PLAYER_PROCESS_AUDIOCHANNELS },
  { "audiosamplerate", PLAYER_PROCESS_AUDIOSAMPLERATE },
  { "audiobitspersample", PLAYER_PROCESS_AUDIOBITSPERSAMPLE }
};

/// \page modules__infolabels_boolean_conditions
/// \subsection modules__infolabels_boolean_conditions_Weather Weather
/// \table_start
///   \table_h3{ Labels, Type, Description }
///   \table_row3{   <b>`Weather.IsFetched`</b>,
///                  \anchor Weather_IsFetched
///                  _boolean_,
///     @return **True** if the weather data has been downloaded.
///     <p>
///   }
///   \table_row3{   <b>`Weather.Conditions`</b>,
///                  \anchor Weather_Conditions
///                  _string_,
///     @return The current weather conditions as textual description.
///     @note This is looked up in a background process.
///     <p>
///   }
///   \table_row3{   <b>`Weather.ConditionsIcon`</b>,
///                  \anchor Weather_ConditionsIcon
///                  _string_,
///     @return The current weather conditions as an icon.
///     @note This is looked up in a background process.
///     <p>
///   }
///   \table_row3{   <b>`Weather.Temperature`</b>,
///                  \anchor Weather_Temperature
///                  _string_,
///     @return The current weather temperature.
///     <p>
///   }
///   \table_row3{   <b>`Weather.Location`</b>,
///                  \anchor Weather_Location
///                  _string_,
///     @return The city/town which the above two items are for.
///     <p>
///   }
///   \table_row3{   <b>`Weather.Fanartcode`</b>,
///                  \anchor Weather_fanartcode
///                  _string_,
///     @return The current weather fanartcode.
///     <p>
///   }
///   \table_row3{   <b>`Weather.Plugin`</b>,
///                  \anchor Weather_plugin
///                  _string_,
///     @return The current weather plugin.
///     <p>
///   }
/// \table_end
///
/// -----------------------------------------------------------------------------
const infomap weather[] =        {{ "isfetched",        WEATHER_IS_FETCHED },
                                  { "conditions",       WEATHER_CONDITIONS_TEXT },         // labels from here
                                  { "temperature",      WEATHER_TEMPERATURE },
                                  { "location",         WEATHER_LOCATION },
                                  { "fanartcode",       WEATHER_FANART_CODE },
                                  { "plugin",           WEATHER_PLUGIN },
                                  { "conditionsicon",   WEATHER_CONDITIONS_ICON }};

/// \page modules__infolabels_boolean_conditions
/// \subsection modules__infolabels_boolean_conditions_System System
/// @todo some values are hardcoded in the middle of the code  - refactor to make it easier to track
/// \table_start
///   \table_h3{ Labels, Type, Description }
///   \table_row3{   <b>`System.AlarmLessOrEqual(alarmname\,seconds)`</b>,
///                  \anchor System_AlarmLessOrEqual
///                  _boolean_,
///     @return **True** if the alarm with `alarmname` has less or equal to `seconds` left.
///     @param alarmname - The name of the alarm. It can be one of the following:
///       - shutdowntimer
///     @param seconds - Time in seconds to compare with the alarm trigger event
///     @note **Example:** `System.Alarmlessorequal(shutdowntimer\,119)`\,
///     will return true when the shutdowntimer has less then 2 minutes
///     left.
///     <p>
///   }
///   \table_row3{   <b>`System.HasNetwork`</b>,
///                  \anchor System_HasNetwork
///                  _boolean_,
///     @return **True** if the Kodi host has a network available.
///     <p>
///   }
///   \table_row3{   <b>`System.HasMediadvd`</b>,
///                  \anchor System_HasMediadvd
///                  _boolean_,
///     @return **True** if there is a CD or DVD in the DVD-ROM drive.
///     <p>
///   }
///   \table_row3{   <b>`System.HasMediaAudioCD`</b>,
///                  \anchor System_HasMediaAudioCD
///                  _boolean_,
///     @return **True** if there is an audio CD in the optical drive. **False** if no drive available\, empty drive or other medium.
///   <p><hr>
///   @skinning_v18 **[New Boolean Condition]** \link System_HasMediaAudioCD `System.HasMediaAudioCD` \endlink
///   <p>
///   }
///   \table_row3{   <b>`System.DVDReady`</b>,
///                  \anchor System_DVDReady
///                  _boolean_,
///     @return **True** if the disc is ready to use.
///     <p>
///   }
///   \table_row3{   <b>`System.TrayOpen`</b>,
///                  \anchor System_TrayOpen
///                  _boolean_,
///     @return **True** if the disc tray is open.
///     <p>
///   }
///   \table_row3{   <b>`System.HasLocks`</b>,
///                  \anchor System_HasLocks
///                  _boolean_,
///     @return **True** if the system has an active lock mode.
///     <p>
///   }
///   \table_row3{   <b>`System.IsMaster`</b>,
///                  \anchor System_IsMaster
///                  _boolean_,
///     @return **True** if the system is in master mode.
///     <p>
///   }
///   \table_row3{   <b>`System.ShowExitButton`</b>,
///                  \anchor System_ShowExitButton
///                  _boolean_,
///     @return **True** if the exit button should be shown (configurable via advanced settings).
///     <p>
///   }
///   \table_row3{   <b>`System.DPMSActive`</b>,
///                  \anchor System_DPMSActive
///                  _boolean_,
///     @return **True** if DPMS (VESA Display Power Management Signaling) mode is active.
///     <p>
///   }
///   \table_row3{   <b>`System.IsStandalone`</b>,
///                  \anchor System_IsStandalone
///                  _boolean_,
///     @return **True** if Kodi is running in standalone mode.
///     <p>
///   }
///   \table_row3{   <b>`System.IsFullscreen`</b>,
///                  \anchor System_IsFullscreen
///                  _boolean_,
///     @return **True** if Kodi is running fullscreen.
///     <p>
///   }
///   \table_row3{   <b>`System.LoggedOn`</b>,
///                  \anchor System_LoggedOn
///                  _boolean_,
///     @return **True** if a user is currently logged on under a profile.
///     <p>
///   }
///   \table_row3{   <b>`System.HasLoginScreen`</b>,
///                  \anchor System_HasLoginScreen
///                  _boolean_,
///     @return **True** if the profile login screen is enabled.
///     <p>
///   }
///   \table_row3{   <b>`System.HasPVR`</b>,
///                  \anchor System_HasPVR
///                  _boolean_,
///     @return **True** if PVR is supported from Kodi.
///     @note normally always true
///     
///   }
///   \table_row3{   <b>`System.HasPVRAddon`</b>,
///                  \anchor System_HasPVRAddon
///                  _boolean_,
///     @return **True** if at least one pvr client addon is installed and enabled.
///     @param id - addon id of the PVR addon
///     <p><hr>
///     @skinning_v17 **[New Boolean Condition]** \link System_HasPVRAddon `System.HasPVRAddon`\endlink
///     <p>
///   }
///   \table_row3{   <b>`System.HasCMS`</b>,
///                  \anchor System_HasCMS
///                  _boolean_,
///     @return **True** if colour management is supported from Kodi.
///     @note currently only supported for OpenGL
///     <p><hr>
///     @skinning_v17 **[New Boolean Condition]** \link System_HasCMS `System.HasCMS`\endlink
///     <p>
///   }
///   \table_row3{   <b>`System.HasActiveModalDialog`</b>,
///                  \anchor System_HasActiveModalDialog
///                  _boolean_,
///     @return **True** if a modal dialog is active.
///     <p><hr>
///     @skinning_v18 **[New Boolean Condition]** \link System_HasActiveModalDialog `System.HasActiveModalDialog`\endlink
///     <p>
///   }
///   \table_row3{   <b>`System.HasVisibleModalDialog`</b>,
///                  \anchor System_HasVisibleModalDialog
///                  _boolean_,
///     @return **True** if a modal dialog is visible.
///     <p><hr>
///     @skinning_v18 **[New Boolean Condition]** \link System_HasVisibleModalDialog `System.HasVisibleModalDialog`\endlink
///     <p>
///   }
///   \table_row3{   <b>`System.Platform.Linux`</b>,
///                  \anchor System_PlatformLinux
///                  _boolean_,
///     @return **True** if Kodi is running on a linux/unix based computer.
///     <p>
///   }
///   \table_row3{   <b>`System.Platform.Linux.RaspberryPi`</b>,
///                  \anchor System_PlatformLinuxRaspberryPi
///                  _boolean_,
///     @return **True** if Kodi is running on a Raspberry Pi.
///     <p><hr>
///     @skinning_v13 **[New Boolean Condition]** \link System_PlatformLinuxRaspberryPi `System.Platform.Linux.RaspberryPi`\endlink
///     <p>
///   }
///   \table_row3{   <b>`System.Platform.Windows`</b>,
///                  \anchor System_PlatformWindows
///                  _boolean_,
///     @return **True** if Kodi is running on a windows based computer.
///     <p>
///   }
///   \table_row3{   <b>`System.Platform.UWP`</b>,
///                  \anchor System_PlatformUWP
///                  _boolean_,
///     @return **True** if Kodi is running on Universal Windows Platform (UWP).
///     <p><hr>
///     @skinning_v18 **[New Boolean Condition]** \link System_PlatformUWP `System.Platform.UWP`\endlink
///     <p>
///   }
///   \table_row3{   <b>`System.Platform.OSX`</b>,
///                  \anchor System_PlatformOSX
///                  _boolean_,
///     @return **True** if Kodi is running on an OSX based computer.
///     <p>
///   }
///   \table_row3{   <b>`System.Platform.IOS`</b>,
///                  \anchor System_PlatformIOS
///                  _boolean_,
///     @return **True** if Kodi is running on an IOS device.
///     <p>
///   }
///   \table_row3{   <b>`System.Platform.Darwin`</b>,
///                  \anchor System_PlatformDarwin
///                  _boolean_,
///     @return **True** if Kodi is running on an OSX or IOS system.
///     <p>
///   }
///   \table_row3{   <b>`System.Platform.Android`</b>,
///                  \anchor System_PlatformAndroid
///                  _boolean_,
///     @return **True** if Kodi is running on an android device.
///     <p>
///   }
///   \table_row3{   <b>`System.CanPowerDown`</b>,
///                  \anchor System_CanPowerDown
///                  _boolean_,
///     @return **True** if Kodi can powerdown the system.
///     <p>
///   }
///   \table_row3{   <b>`System.CanSuspend`</b>,
///                  \anchor System_CanSuspend
///                  _boolean_,
///     @return **True** if Kodi can suspend the system.
///     <p>
///   }
///   \table_row3{   <b>`System.CanHibernate`</b>,
///                  \anchor System_CanHibernate
///                  _boolean_,
///     @return **True** if Kodi can hibernate the system.
///     <p>
///   }
///   \table_row3{   <b>`System.HasHiddenInput`</b>,
///                  \anchor System_HasHiddenInput
///                  _boolean_,
///     @return **True** when to osd keyboard/numeric dialog requests a
///     password/pincode.
///     <p><hr>
///     @skinning_v16 **[New Boolean Condition]** \link System_HasHiddenInput `System.HasHiddenInput`\endlink
///     <p>
///   }
///   \table_row3{   <b>`System.CanReboot`</b>,
///                  \anchor System_CanReboot
///                  _boolean_,
///     @return **True** if Kodi can reboot the system.
///     <p>
///   }
///   \table_row3{   <b>`System.ScreenSaverActive`</b>,
///                  \anchor System_ScreenSaverActive
///                  _boolean_,
///     @return **True** if ScreenSaver is active.
///     <p>
///   }
///   \table_row3{   <b>`System.IsInhibit`</b>,
///                  \anchor System_IsInhibit
///                  _boolean_,
///     @return **True** when shutdown on idle is disabled.
///     <p>
///   }
///   \table_row3{   <b>`System.HasShutdown`</b>,
///                  \anchor System_HasShutdown
///                  _boolean_,
///     @return **True** when shutdown on idle is enabled.
///     <p>
///   }
///   \table_row3{   <b>`System.Time`</b>,
///                  \anchor System_Time
///                  _string_,
///     @return The current time.
///     <p>
///   }
///   \table_row3{   <b>`System.Time(format)`</b>,
///                  \anchor System_Time_format
///                  _string_,
///     @return The current time in a specified format.
///     @param format [opt] The format of the return time value.
///     See \ref TIME_FORMAT for the list of possible values.
///     <p>
///   }
///   \table_row3{   <b>`System.Time(startTime[\,endTime])`</b>,
///                  \anchor System_Time
///                  _boolean_,
///     @return **True** if the current system time is >= `startTime` and < `endTime` (if defined).
///     @param startTime - Start time
///     @param endTime - [opt] End time
///     <p>
///     @note Time must be specified in the format HH:mm\, using
///     a 24 hour clock.
///     <p>
///   }
///   \table_row3{   <b>`System.Date`</b>,
///                  \anchor System_Date
///                  _string_,
///     @return The current date.
///     <p><hr>
///     @skinning_v16 **[Infolabel Updated]** \link System_Date `System.Date`\endlink
///     will now return the full day and month names. old: sat\, jul 18 2015
///     new: saturday\, july 18 2015
///     <p>
///   }
///   \table_row3{   <b>`System.Date(format)`</b>,
///                  \anchor System_Date_format
///                  _string_,
///     @return The current date using a specified format.
///     @param format - the format for the date. It can be one of the following
///     values:
///       - **d** - day of month (1-31)
///       - **dd** - day of month (01-31)
///       - **ddd** - short day of the week Mon-Sun
///       - **DDD** - long day of the week Monday-Sunday
///       - **m** - month (1-12)
///       - **mm** - month (01-12)
///       - **mmm** - short month name Jan-Dec
///       - **MMM** - long month name January-December
///       - **yy** - 2-digit year
///       - **yyyy** - 4-digit year
///     <p>
///   }
///   \table_row3{   <b>`System.Date(startDate[\,endDate])`</b>,
///                  \anchor System_Date
///                  _boolean_,
///     @return **True** if the current system date is >= `startDate` and < `endDate` (if defined).
///     @param startDate - The start date
///     @param endDate - [opt] The end date
///     @note Date must be specified in the format MM-DD or YY-MM-DD.
///     <p>
///   }
///   \table_row3{   <b>`System.AlarmPos`</b>,
///                  \anchor System_AlarmPos
///                  _string_,
///     @return The shutdown Timer position.
///     <p>
///   }
///   \table_row3{   <b>`System.BatteryLevel`</b>,
///                  \anchor System_BatteryLevel
///                  _string_,
///     @return The remaining battery level in range 0-100.
///     <p>
///   }
///   \table_row3{   <b>`System.FreeSpace`</b>,
///                  \anchor System_FreeSpace
///                  _string_,
///     @return The total Freespace on the drive.
///     <p>
///   }
///   \table_row3{   <b>`System.UsedSpace`</b>,
///                  \anchor System_UsedSpace
///                  _string_,
///     @return The total Usedspace on the drive.
///     <p>
///   }
///   \table_row3{   <b>`System.TotalSpace`</b>,
///                  \anchor System_TotalSpace
///                  _string_,
///     @return The total space on the drive.
///     <p>
///   }
///   \table_row3{   <b>`System.UsedSpacePercent`</b>,
///                  \anchor System_UsedSpacePercent
///                  _string_,
///     @return The total Usedspace Percent on the drive.
///     <p>
///   }
///   \table_row3{   <b>`System.FreeSpacePercent`</b>,
///                  \anchor System_FreeSpacePercent
///                  _string_,
///     @return The total Freespace Percent on the drive.
///     <p>
///   }
///   \table_row3{   <b>`System.CPUTemperature`</b>,
///                  \anchor System_CPUTemperature
///                  _string_,
///     @return The current CPU temperature.
///     <p>
///   }
///   \table_row3{   <b>`System.CpuUsage`</b>,
///                  \anchor System_CpuUsage
///                  _string_,
///     @return The the cpu usage for each individual cpu core.
///     <p>
///   }
///   \table_row3{   <b>`System.GPUTemperature`</b>,
///                  \anchor System_GPUTemperature
///                  _string_,
///     @return The current GPU temperature.
///     <p>
///   }
///   \table_row3{   <b>`System.FanSpeed`</b>,
///                  \anchor System_FanSpeed
///                  _string_,
///     @return The current fan speed.
///     <p>
///   }
///   \table_row3{   <b>`System.BuildVersion`</b>,
///                  \anchor System_BuildVersion
///                  _string_,
///     @return The version of build.
///     <p>
///   }
///   \table_row3{   <b>`System.BuildVersionShort`</b>,
///                  \anchor System_BuildVersionShort
///                  _string_,
///     @return The shorter string with version of build.
///     <p>
///   }
///   \table_row3{   <b>`System.BuildDate`</b>,
///                  \anchor System_BuildDate
///                  _string_,
///     @return The date of build.
///     <p>
///   }
///   \table_row3{   <b>`System.FriendlyName`</b>,
///                  \anchor System_FriendlyName
///                  _string_,
///     @return The Kodi instance name. 
///     @note It will auto append (%hostname%) in case
///     the device name was not changed. eg. "Kodi (htpc)"
///     <p>
///   }
///   \table_row3{   <b>`System.FPS`</b>,
///                  \anchor System_FPS
///                  _string_,
///     @return The current rendering speed (frames per second).
///     <p>
///   }
///   \table_row3{   <b>`System.FreeMemory`</b>,
///                  \anchor System_FreeMemory
///                  _string_,
///     @return The amount of free memory in Mb.
///     <p>
///   }
///   \table_row3{   <b>`System.ScreenMode`</b>,
///                  \anchor System_ScreenMode
///                  _string_,
///     @return The screenmode (eg windowed / fullscreen).
///     <p>
///   }
///   \table_row3{   <b>`System.ScreenWidth`</b>,
///                  \anchor System_ScreenWidth
///                  _string_,
///     @return The width of screen in pixels.
///     <p>
///   }
///   \table_row3{   <b>`System.ScreenHeight`</b>,
///                  \anchor System_ScreenHeight
///                  _string_,
///     @return The height of screen in pixels.
///     <p>
///   }
///   \table_row3{   <b>`System.StartupWindow`</b>,
///                  \anchor System_StartupWindow
///                  _string_,
///     @return The Window Kodi will load on startup.
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link System_StartupWindow `System.StartupWindow`\endlink
///     <p>
///   }
///   \table_row3{   <b>`System.CurrentWindow`</b>,
///                  \anchor System_CurrentWindow
///                  _string_,
///     @return The current Window in use.
///     <p>
///   }
///   \table_row3{   <b>`System.CurrentControl`</b>,
///                  \anchor System_CurrentControl
///                  _string_,
///     @return The current focused control
///     <p>
///   }
///   \table_row3{   <b>`System.CurrentControlId`</b>,
///                  \anchor System_CurrentControlId
///                  _string_,
///     @return The ID of the currently focused control.
///     <p>
///   }
///   \table_row3{   <b>`System.DVDLabel`</b>,
///                  \anchor System_DVDLabel
///                  _string_,
///     @return the label of the disk in the DVD-ROM drive.
///     <p>
///   }
///   \table_row3{   <b>`System.KernelVersion`</b>,
///                  \anchor System_KernelVersion
///                  _string_,
///     @return The System kernel version.
///     <p>
///   }
///   \table_row3{   <b>`System.OSVersionInfo`</b>,
///                  \anchor System_OSVersionInfo
///                  _string_,
///     @return The system name + kernel version.
///     <p>
///   }
///   \table_row3{   <b>`System.Uptime`</b>,
///                  \anchor System_Uptime
///                  _string_,
///     @return The system current uptime.
///     <p>
///   }
///   \table_row3{   <b>`System.TotalUptime`</b>,
///                  \anchor System_TotalUptime
///                  _string_,
///     @return The system total uptime.
///     <p>
///   }
///   \table_row3{   <b>`System.CpuFrequency`</b>,
///                  \anchor System_CpuFrequency
///                  _string_,
///     @return The system cpu frequency.
///     <p>
///   }
///   \table_row3{   <b>`System.ScreenResolution`</b>,
///                  \anchor System_ScreenResolution
///                  _string_,
///     @return The screen resolution.
///     <p>
///   }
///   \table_row3{   <b>`System.VideoEncoderInfo`</b>,
///                  \anchor System_VideoEncoderInfo
///                  _string_,
///     @return The video encoder info.
///     <p>
///   }
///   \table_row3{   <b>`System.InternetState`</b>,
///                  \anchor System_InternetState
///                  _string_,
///     @return The internet state: connected or not connected.
///     @warning Do not use to check status in a pythonscript since it is threaded.
///     <p>
///   }
///   \table_row3{   <b>`System.Language`</b>,
///                  \anchor System_Language
///                  _string_,
///     @return the current language.
///     <p>
///   }
///   \table_row3{   <b>`System.ProfileName`</b>,
///                  \anchor System_ProfileName
///                  _string_,
///     @return The user name of the currently logged in Kodi user
///     <p>
///   }
///   \table_row3{   <b>`System.ProfileThumb`</b>,
///                  \anchor System_ProfileThumb
///                  _string_,
///     @return The thumbnail image of the currently logged in Kodi user
///     <p>
///   }
///   \table_row3{   <b>`System.ProfileCount`</b>,
///                  \anchor System_ProfileCount
///                  _string_,
///     @return The number of defined profiles.
///     <p>
///   }
///   \table_row3{   <b>`System.ProfileAutoLogin`</b>,
///                  \anchor System_ProfileAutoLogin
///                  _string_,
///     @return The profile Kodi will auto login to.
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link System_ProfileAutoLogin `System.ProfileAutoLogin`\endlink
///     <p>
///   }
///   \table_row3{   <b>`System.StereoscopicMode`</b>,
///                  \anchor System_StereoscopicMode
///                  _string_,
///     @return The prefered stereoscopic mode.
///     @note Configured in settings > video > playback).
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link System_StereoscopicMode `System.StereoscopicMode`\endlink
///     <p>
///   }
///   \table_row3{   <b>`System.TemperatureUnits`</b>,
///                  \anchor System_TemperatureUnits
///                  _string_,
///     @return the Celsius or the Fahrenheit symbol.
///     <p>
///   }
///   \table_row3{   <b>`System.Progressbar`</b>,
///                  \anchor System_Progressbar
///                  _string_,
///     @return The percentage of the currently active progress.
///     <p>
///   }
///   \table_row3{   <b>`System.GetBool(boolean)`</b>,
///                  \anchor System_GetBool
///                  _string_,
///     @return The value of any standard system boolean setting. 
///     @note Will not work with settings in advancedsettings.xml
///     <p>
///   }
///   \table_row3{   <b>`System.Memory(type)`</b>,
///                  \anchor System_Memory
///                  _string_,
///     @return The memory value depending on the requested type.
///     @param type - Can be one of the following:
///       - <b>free</b>
///       - <b>free.percent</b>
///       - <b>used</b>
///       - <b>used.percent</b>
///       - <b>total</b>
///     <p>
///   }
///   \table_row3{   <b>`System.AddonTitle(id)`</b>,
///                  \anchor System_AddonTitle
///                  _string_,
///     @return The title of the addon with the given id
///     @param id - the addon id
///     <p>
///   }
///   \table_row3{   <b>`System.AddonVersion(id)`</b>,
///                  \anchor System_AddonVersion
///                  _string_,
///     @return The version of the addon with the given id.
///     @param id - the addon id
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link System_AddonVersion `System.AddonVersion(id)`\endlink
///     <p>
///   }
///   \table_row3{   <b>`System.AddonIcon(id)`</b>,
///                  \anchor System_AddonVersion
///                  _string_,
///     @return The icon of the addon with the given id.
///     @param id - the addon id
///     <p>
///   }
///   \table_row3{   <b>`System.IdleTime(time)`</b>,
///                  \anchor System_IdleTime
///                  _boolean_,
///     @return **True** if Kodi has had no input for `time` amount of seconds.
///     @param time - elapsed seconds to check for idle activity.
///     <p>
///   }
///   \table_row3{   <b>`System.PrivacyPolicy`</b>,
///                  \anchor System_PrivacyPolicy
///                  _string_,
///     @return The official Kodi privacy policy.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link System_PrivacyPolicy `System.PrivacyPolicy`\endlink
///     <p>
///   }
const infomap system_labels[] =  {{ "hasnetwork",       SYSTEM_ETHERNET_LINK_ACTIVE },
                                  { "hasmediadvd",      SYSTEM_MEDIA_DVD },
                                  { "hasmediaaudiocd",  SYSTEM_MEDIA_AUDIO_CD },
                                  { "dvdready",         SYSTEM_DVDREADY },
                                  { "trayopen",         SYSTEM_TRAYOPEN },
                                  { "haslocks",         SYSTEM_HASLOCKS },
                                  { "hashiddeninput",   SYSTEM_HAS_INPUT_HIDDEN },
                                  { "hasloginscreen",   SYSTEM_HAS_LOGINSCREEN },
                                  { "hasactivemodaldialog",   SYSTEM_HAS_ACTIVE_MODAL_DIALOG },
                                  { "hasvisiblemodaldialog",   SYSTEM_HAS_VISIBLE_MODAL_DIALOG },
                                  { "ismaster",         SYSTEM_ISMASTER },
                                  { "isfullscreen",     SYSTEM_ISFULLSCREEN },
                                  { "isstandalone",     SYSTEM_ISSTANDALONE },
                                  { "loggedon",         SYSTEM_LOGGEDON },
                                  { "showexitbutton",   SYSTEM_SHOW_EXIT_BUTTON },
                                  { "canpowerdown",     SYSTEM_CAN_POWERDOWN },
                                  { "cansuspend",       SYSTEM_CAN_SUSPEND },
                                  { "canhibernate",     SYSTEM_CAN_HIBERNATE },
                                  { "canreboot",        SYSTEM_CAN_REBOOT },
                                  { "screensaveractive",SYSTEM_SCREENSAVER_ACTIVE },
                                  { "dpmsactive",       SYSTEM_DPMS_ACTIVE },
                                  { "cputemperature",   SYSTEM_CPU_TEMPERATURE },     // labels from here
                                  { "cpuusage",         SYSTEM_CPU_USAGE },
                                  { "gputemperature",   SYSTEM_GPU_TEMPERATURE },
                                  { "fanspeed",         SYSTEM_FAN_SPEED },
                                  { "freespace",        SYSTEM_FREE_SPACE },
                                  { "usedspace",        SYSTEM_USED_SPACE },
                                  { "totalspace",       SYSTEM_TOTAL_SPACE },
                                  { "usedspacepercent", SYSTEM_USED_SPACE_PERCENT },
                                  { "freespacepercent", SYSTEM_FREE_SPACE_PERCENT },
                                  { "buildversion",     SYSTEM_BUILD_VERSION },
                                  { "buildversionshort",SYSTEM_BUILD_VERSION_SHORT },
                                  { "builddate",        SYSTEM_BUILD_DATE },
                                  { "fps",              SYSTEM_FPS },
                                  { "freememory",       SYSTEM_FREE_MEMORY },
                                  { "language",         SYSTEM_LANGUAGE },
                                  { "temperatureunits", SYSTEM_TEMPERATURE_UNITS },
                                  { "screenmode",       SYSTEM_SCREEN_MODE },
                                  { "screenwidth",      SYSTEM_SCREEN_WIDTH },
                                  { "screenheight",     SYSTEM_SCREEN_HEIGHT },
                                  { "currentwindow",    SYSTEM_CURRENT_WINDOW },
                                  { "currentcontrol",   SYSTEM_CURRENT_CONTROL },
                                  { "currentcontrolid", SYSTEM_CURRENT_CONTROL_ID },
                                  { "dvdlabel",         SYSTEM_DVD_LABEL },
                                  { "internetstate",    SYSTEM_INTERNET_STATE },
                                  { "osversioninfo",    SYSTEM_OS_VERSION_INFO },
                                  { "kernelversion",    SYSTEM_OS_VERSION_INFO }, // old, not correct name
                                  { "uptime",           SYSTEM_UPTIME },
                                  { "totaluptime",      SYSTEM_TOTALUPTIME },
                                  { "cpufrequency",     SYSTEM_CPUFREQUENCY },
                                  { "screenresolution", SYSTEM_SCREEN_RESOLUTION },
                                  { "videoencoderinfo", SYSTEM_VIDEO_ENCODER_INFO },
                                  { "profilename",      SYSTEM_PROFILENAME },
                                  { "profilethumb",     SYSTEM_PROFILETHUMB },
                                  { "profilecount",     SYSTEM_PROFILECOUNT },
                                  { "profileautologin", SYSTEM_PROFILEAUTOLOGIN },
                                  { "progressbar",      SYSTEM_PROGRESS_BAR },
                                  { "batterylevel",     SYSTEM_BATTERY_LEVEL },
                                  { "friendlyname",     SYSTEM_FRIENDLY_NAME },
                                  { "alarmpos",         SYSTEM_ALARM_POS },
                                  { "isinhibit",        SYSTEM_ISINHIBIT },
                                  { "hasshutdown",      SYSTEM_HAS_SHUTDOWN },
                                  { "haspvr",           SYSTEM_HAS_PVR },
                                  { "startupwindow",    SYSTEM_STARTUP_WINDOW },
                                  { "stereoscopicmode", SYSTEM_STEREOSCOPIC_MODE },
                                  { "hascms",           SYSTEM_HAS_CMS },
                                  { "privacypolicy",    SYSTEM_PRIVACY_POLICY },
                                  { "haspvraddon",      SYSTEM_HAS_PVR_ADDON }};

/// \page modules__infolabels_boolean_conditions
///   \table_row3{   <b>`System.HasAddon(id)`</b>,
///                  \anchor System_HasAddon
///                  _boolean_,
///     @return **True** if the specified addon is installed on the system.
///     @param id - the addon id
///     <p>
///   }
///   \table_row3{   <b>`System.HasCoreId(id)`</b>,
///                  \anchor System_HasCoreId
///                  _boolean_,
///     @return **True** if the CPU core with the given 'id' exists.
///     @param id - the id of the CPU core
///     <p>
///   }
///   \table_row3{   <b>`System.HasAlarm(alarm)`</b>,
///                  \anchor System_HasAlarm
///                  _boolean_,
///     @return **True** if the system has the `alarm` alarm set.
///     @param alarm - the name of the alarm
///     <p>
///   }
///   \table_row3{   <b>`System.CoreUsage(id)`</b>,
///                  \anchor System_CoreUsage
///                  _string_,
///     @return the usage of the CPU core with the given 'id'
///     @param id - the id of the CPU core
///     <p>
///   }
///   \table_row3{   <b>`System.Setting(hidewatched)`</b>,
///                  \anchor System_Setting
///                  _boolean_,
///     @return **True** if 'hide watched items' is selected.
///     <p>
///   }
/// \table_end
///
/// -----------------------------------------------------------------------------
const infomap system_param[] =   {{ "hasalarm",         SYSTEM_HAS_ALARM },
                                  { "hascoreid",        SYSTEM_HAS_CORE_ID },
                                  { "setting",          SYSTEM_SETTING },
                                  { "hasaddon",         SYSTEM_HAS_ADDON },
                                  { "coreusage",        SYSTEM_GET_CORE_USAGE }};

/// \page modules__infolabels_boolean_conditions
/// \subsection modules__infolabels_boolean_conditions_Network Network
/// \table_start
///   \table_h3{ Labels, Type, Description }
///   \table_row3{   <b>`Network.IsDHCP`</b>,
///                  \anchor Network_IsDHCP
///                  _boolean_,
///     @return **True** if the network type is DHCP.
///     @note Network type can be either DHCP or FIXED
///     <p>
///   }
///   \table_row3{   <b>`Network.IPAddress`</b>,
///                  \anchor Network_IPAddress
///                  _string_,
///     @return The system's IP Address. e.g. 192.168.1.15
///     <p>
///   }
///   \table_row3{   <b>`Network.LinkState`</b>,
///                  \anchor Network_LinkState
///                  _string_,
///     @return The network linkstate e.g. 10mbit/100mbit etc.
///     <p>
///   }
///   \table_row3{   <b>`Network.MacAddress`</b>,
///                  \anchor Network_MacAddress
///                  _string_,
///     @return The system's MAC address.
///     <p>
///   }
///   \table_row3{   <b>`Network.SubnetMask`</b>,
///                  \anchor Network_SubnetMask
///                  _string_,
///     @return The network subnet mask.
///     <p>
///   }
///   \table_row3{   <b>`Network.GatewayAddress`</b>,
///                  \anchor Network_GatewayAddress
///                  _string_,
///     @return The network gateway address.
///     <p>
///   }
///   \table_row3{   <b>`Network.DNS1Address`</b>,
///                  \anchor Network_DNS1Address
///                  _string_,
///     @return The network DNS 1 address.
///     <p>
///   }
///   \table_row3{   <b>`Network.DNS2Address`</b>,
///                  \anchor Network_DNS2Address
///                  _string_,
///     @return The network DNS 2 address.
///     <p>
///   }
///   \table_row3{   <b>`Network.DHCPAddress`</b>,
///                  \anchor Network_DHCPAddress
///                  _string_,
///     @return The DHCP IP address.
///     <p>
///   }
/// \table_end
///
/// -----------------------------------------------------------------------------
const infomap network_labels[] = {{ "isdhcp",            NETWORK_IS_DHCP },
                                  { "ipaddress",         NETWORK_IP_ADDRESS }, //labels from here
                                  { "linkstate",         NETWORK_LINK_STATE },
                                  { "macaddress",        NETWORK_MAC_ADDRESS },
                                  { "subnetmask",        NETWORK_SUBNET_MASK },
                                  { "gatewayaddress",    NETWORK_GATEWAY_ADDRESS },
                                  { "dns1address",       NETWORK_DNS1_ADDRESS },
                                  { "dns2address",       NETWORK_DNS2_ADDRESS },
                                  { "dhcpaddress",       NETWORK_DHCP_ADDRESS }};

/// \page modules__infolabels_boolean_conditions
/// \subsection modules__infolabels_boolean_conditions_musicpartymode Music party mode
/// \table_start
///   \table_h3{ Labels, Type, Description }
///   \table_row3{   <b>`MusicPartyMode.Enabled`</b>,
///                  \anchor MusicPartyMode_Enabled
///                  _boolean_,
///     @return **True** if Party Mode is enabled.
///     <p>
///   }
///   \table_row3{   <b>`MusicPartyMode.SongsPlayed`</b>,
///                  \anchor MusicPartyMode_SongsPlayed
///                  _string_,
///     @return The number of songs played during Party Mode.
///     <p>
///   }
///   \table_row3{   <b>`MusicPartyMode.MatchingSongs`</b>,
///                  \anchor MusicPartyMode_MatchingSongs
///                  _string_,
///     @return The number of songs available to Party Mode.
///     <p>
///   }
///   \table_row3{   <b>`MusicPartyMode.MatchingSongsPicked`</b>,
///                  \anchor MusicPartyMode_MatchingSongsPicked
///                  _string_,
///     @return The number of songs picked already for Party Mode.
///     <p>
///   }
///   \table_row3{   <b>`MusicPartyMode.MatchingSongsLeft`</b>,
///                  \anchor MusicPartyMode_MatchingSongsLeft
///                  _string_,
///     @return The number of songs left to be picked from for Party Mode.
///     <p>
///   }
///   \table_row3{   <b>`MusicPartyMode.RelaxedSongsPicked`</b>,
///                  \anchor MusicPartyMode_RelaxedSongsPicked
///                  _string_,
///     @todo Not currently used
///     <p>
///   }
///   \table_row3{   <b>`MusicPartyMode.RandomSongsPicked`</b>,
///                  \anchor MusicPartyMode_RandomSongsPicked
///                  _string_,
///     @return The number of unique random songs picked during Party Mode.
///     <p>
///   }
/// \table_end
///
/// -----------------------------------------------------------------------------
const infomap musicpartymode[] = {{ "enabled",           MUSICPM_ENABLED },
                                  { "songsplayed",       MUSICPM_SONGSPLAYED },
                                  { "matchingsongs",     MUSICPM_MATCHINGSONGS },
                                  { "matchingsongspicked", MUSICPM_MATCHINGSONGSPICKED },
                                  { "matchingsongsleft", MUSICPM_MATCHINGSONGSLEFT },
                                  { "relaxedsongspicked", MUSICPM_RELAXEDSONGSPICKED },
                                  { "randomsongspicked", MUSICPM_RANDOMSONGSPICKED }};

/// \page modules__infolabels_boolean_conditions
/// \subsection modules__infolabels_boolean_conditions_MusicPlayer Music player
/// \table_start
///   \table_h3{ Labels, Type, Description }
///   \table_row3{   <b>`MusicPlayer.Offset(number).Exists`</b>,
///                  \anchor MusicPlayer_Offset
///                  _boolean_,
///     @return **True** if the music players playlist has a song queued in
///     position (number).
///     @param number - song position
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.Title`</b>,
///                  \anchor MusicPlayer_Title
///                  _string_,
///     @return The title of the currently playing song.
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.offset(number).Title`</b>,
///                  \anchor MusicPlayer_Offset_Title
///                  _string_,
///     @return The title of the song which has an offset `number` with respect to the
///     current playing song.
///     @param number - the offset number with respect to the current playing song
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.Position(number).Title`</b>,
///                  \anchor MusicPlayer_Position_Title
///                  _string_,
///     @return The title of the song which as an offset `number` with respect to the
///     start of the playlist.
///     @param number - the offset number with respect to the start of the playlist
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.Album`</b>,
///                  \anchor MusicPlayer_Album
///                  _string_,
///     @return The album from which the current song is from.
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.offset(number).Album`</b>,
///                  \anchor MusicPlayer_OffSet_Album
///                  _string_,
///     @return The album from which the song with offset `number` with respect to
///     the current song is from.
///     @param number - the offset number with respect to the current playing song
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.Position(number).Album`</b>,
///                  \anchor MusicPlayer_Position_Album
///                  _string_,
///     @return The album from which the song with offset `number` with respect to
///     the start of the playlist is from.
///     @param number - the offset number with respect to the start of the playlist
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.Property(Album_Mood)`</b>,
///                  \anchor MusicPlayer_Property_Album_Mood
///                  _string_,
///     @return The moods of the currently playing Album
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.Property(Role.Composer)`</b>,
///                  \anchor MusicPlayer_Property_Role_Composer
///                  _string_,
///     @return The name of the person who composed the selected song.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link MusicPlayer_Property_Role_Composer `MusicPlayer.Property(Role.Composer)`\endlink
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.Property(Role.Conductor)`</b>,
///                  \anchor MusicPlayer_Property_Role_Conductor
///                  _string_,
///     @return The name of the person who conducted the selected song.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link MusicPlayer_Property_Role_Conductor `MusicPlayer.Property(Role.Conductor)`\endlink
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.Property(Role.Orchestra)`</b>,
///                  \anchor MusicPlayer_Property_Role_Orchestra
///                  _string_,
///     @return The name of the orchestra performing the selected song.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link MusicPlayer_Property_Role_Orchestra `MusicPlayer.Property(Role.Orchestra)`\endlink
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.Property(Role.Lyricist)`</b>,
///                  \anchor MusicPlayer_Property_Role_Lyricist
///                  _string_,
///     @return The name of the person who wrote the lyrics of the selected song.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link MusicPlayer_Property_Role_Lyricist `MusicPlayer.Property(Role.Lyricist)`\endlink
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.Property(Role.Remixer)`</b>,
///                  \anchor MusicPlayer_Property_Role_Remixer
///                  _string_,
///     @return The name of the person who remixed the selected song.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link MusicPlayer_Property_Role_Remixer `MusicPlayer.Property(Role.Remixer)`\endlink
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.Property(Role.Arranger)`</b>,
///                  \anchor MusicPlayer_Property_Role_Arranger
///                  _string_,
///     @return The name of the person who arranged the selected song.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link MusicPlayer_Property_Role_Arranger `MusicPlayer.Property(Role.Arranger)`\endlink
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.Property(Role.Engineer)`</b>,
///                  \anchor MusicPlayer_Property_Role_Engineer
///                  _string_,
///     @return The name of the person who was the engineer of the selected song.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link MusicPlayer_Property_Role_Engineer `MusicPlayer.Property(Role.Engineer)`\endlink
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.Property(Role.Producer)`</b>,
///                  \anchor MusicPlayer_Property_Role_Producer
///                  _string_,
///     @return The name of the person who produced the selected song.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link MusicPlayer_Property_Role_Producer `MusicPlayer.Property(Role.Producer)`\endlink
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.Property(Role.DJMixer)`</b>,
///                  \anchor MusicPlayer_Property_Role_DJMixer
///                  _string_,
///     @return The name of the dj who remixed the selected song.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link MusicPlayer_Property_Role_DJMixer `MusicPlayer.Property(Role.DJMixer)`\endlink
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.Property(Role.Mixer)`</b>,
///                  \anchor MusicPlayer_Property_Role_Mixer
///                  _string_,
///     @return The name of the dj who remixed the selected song.
///     @todo So maybe rather than a row each have one entry for Role.XXXXX with composer\, arranger etc. as listed values
///     @note MusicPlayer.Property(Role.any_custom_role) also works\, 
///     where any_custom_role could be an instrument violin or some other production activity e.g. sound engineer.
///     The roles listed (composer\, arranger etc.) are standard ones but there are many possible. 
///     Music file tagging allows for the musicians and all other people involved in the recording to be added\, Kodi
///     will gathers and stores that data\, and it is availlable to GUI.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link MusicPlayer_Property_Role_Mixer `MusicPlayer.Property(Role.Mixer)`\endlink
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.Property(Album_Mood)`</b>,
///                  \anchor MusicPlayer_Property_Album_Mood
///                  _string_,
///     @return the moods of the currently playing Album
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.Property(Album_Style)`</b>,
///                  \anchor MusicPlayer_Property_Album_Style
///                  _string_,
///     @return the styles of the currently playing Album.
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.Property(Album_Theme)`</b>,
///                  \anchor MusicPlayer_Property_Album_Theme
///                  _string_,
///     @return The themes of the currently playing Album
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.Property(Album_Type)`</b>,
///                  \anchor MusicPlayer_Property_Album_Type
///                  _string_,
///     @return The album type (e.g. compilation\, enhanced\, explicit lyrics) of the
///     currently playing album.
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.Property(Album_Label)`</b>,
///                  \anchor MusicPlayer_Property_Album_Label
///                  _string_,
///     @return The record label of the currently playing album.
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.Property(Album_Description)`</b>,
///                  \anchor MusicPlayer_Property_Album_Description
///                  _string_,
///     @return A review of the currently playing album
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.Artist`</b>,
///                  \anchor MusicPlayer_Artist
///                  _string_,
///     @return Artist(s) of current song.
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.offset(number).Artist`</b>,
///                  \anchor MusicPlayer_Offset_Artist
///                  _string_,
///     @return Artist(s) of the song which has an offset `number` with respect
///     to the current playing song.
///     @param number - the offset of the song with respect to the current
///     playing song
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.Position(number).Artist`</b>,
///                  \anchor MusicPlayer_Position_Artist
///                  _string_,
///     @return Artist(s) of the song which has an offset `number` with respect
///     to the start of the playlist.
///     @param number - the offset of the song with respect to 
///     the start of the playlist
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.AlbumArtist`</b>,
///                  \anchor MusicPlayer_AlbumArtist
///                  _string_,
///     @return The album artist of the currently playing song.
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.Cover`</b>,
///                  \anchor MusicPlayer_Cover
///                  _string_,
///     @return The album cover of currently playing song.
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.Property(Artist_Sortname)`</b>,
///                  \anchor MusicPlayer_Property_Artist_Sortname
///                  _string_,
///     @return The sortname of the currently playing Artist.
///     <p><hr>
///     @skinning_v18 **[New Infolabel]** \link MusicPlayer_Property_Artist_Sortname `MusicPlayer.Property(Artist_Sortname)`\endlink
///     <p> 
///   }
///   \table_row3{   <b>`MusicPlayer.Property(Artist_Type)`</b>,
///                  \anchor MusicPlayer_Property_Artist_Type
///                  _string_,
///     @return The type of the currently playing Artist - person\,
///     group\, orchestra\, choir etc.
///     <p><hr>
///     @skinning_v18 **[New Infolabel]** \link MusicPlayer_Property_Artist_Type `MusicPlayer.Property(Artist_Type)`\endlink
///     <p>  
///   }
///   \table_row3{   <b>`MusicPlayer.Property(Artist_Gender)`</b>,
///                  \anchor MusicPlayer_Property_Artist_Gender
///                  _string_,
///     @return The gender of the currently playing Artist - male\,
///     female\, other.
///     <p><hr>
///     @skinning_v18 **[New Infolabel]** \link MusicPlayer_Property_Artist_Gender `MusicPlayer.Property(Artist_Gender)`\endlink
///     <p>  
///   }
///   \table_row3{   <b>`MusicPlayer.Property(Artist_Disambiguation)`</b>,
///                  \anchor MusicPlayer_Property_Artist_Disambiguation
///                  _string_,
///     @return A brief description of the currently playing Artist that differentiates them
///     from others with the same name.
///     <p><hr>
///     @skinning_v18 **[New Infolabel]** \link MusicPlayer_Property_Artist_Disambiguation `MusicPlayer.Property(Artist_Disambiguation)`\endlink
///     <p>  
///   }
///   \table_row3{   <b>`MusicPlayer.Property(Artist_Born)`</b>,
///                  \anchor MusicPlayer_Property_Artist_Born
///                  _string_,
///     @return The date of Birth of the currently playing Artist.
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.Property(Artist_Died)`</b>,
///                  \anchor MusicPlayer_Property_Artist_Died
///                  _string_,
///     @return The date of Death of the currently playing Artist.
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.Property(Artist_Formed)`</b>,
///                  \anchor MusicPlayer_Property_Artist_Formed
///                  _string_,
///     @return The Formation date of the currently playing Artist/Band.
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.Property(Artist_Disbanded)`</b>,
///                  \anchor MusicPlayer_Property_Artist_Disbanded
///                  _string_,
///     @return The disbanding date of the currently playing Artist/Band.
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.Property(Artist_YearsActive)`</b>,
///                  \anchor MusicPlayer_Property_Artist_YearsActive
///                  _string_,
///     @return The years the currently Playing artist has been active.
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.Property(Artist_Instrument)`</b>,
///                  \anchor MusicPlayer_Property_Artist_Instrument
///                  _string_,
///     @return The instruments played by the currently playing artist.
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.Property(Artist_Description)`</b>,
///                  \anchor MusicPlayer_Property_Artist_Description
///                  _string_,
///     @return A biography of the currently playing artist.
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.Property(Artist_Mood)`</b>,
///                  \anchor MusicPlayer_Property_Artist_Mood
///                  _string_,
///     @return The moods of the currently playing artist.
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.Property(Artist_Style)`</b>,
///                  \anchor MusicPlayer_Property_Artist_Style
///                  _string_,
///     @return The styles of the currently playing artist.
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.Property(Artist_Genre)`</b>,
///                  \anchor MusicPlayer_Property_Artist_Genre
///                  _string_,
///     @return The genre of the currently playing artist.
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.Genre`</b>,
///                  \anchor MusicPlayer_Genre
///                  _string_,
///     @return The genre(s) of current song.
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.offset(number).Genre`</b>,
///                  \anchor MusicPlayer_OffSet_Genre
///                  _string_,
///     @return The genre(s) of the song with an offset `number` with respect
///     to the current playing song.
///     @param number - the offset song number with respect to the current playing
///     song.
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.Position(number).Genre`</b>,
///                  \anchor MusicPlayer_Position_Genre
///                  _string_,
///     @return The genre(s) of the song with an offset `number` with respect
///     to the start of the playlist.
///     @param number - the offset song number with respect to the start of the
///     playlist
///     song.
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.Lyrics`</b>,
///                  \anchor MusicPlayer_Lyrics
///                  _string_,
///     @return The lyrics of current song stored in ID tag info.
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.Year`</b>,
///                  \anchor MusicPlayer_Year
///                  _string_,
///     @return The year of release of current song.
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.offset(number).Year`</b>,
///                  \anchor MusicPlayer_Offset_Year
///                  _string_,
///     @return The year of release of the song with an offset `number` with
///     respect to the current playing song.
///     @param number - the offset numbet with respect to the current song.
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.Position(number).Year`</b>,
///                  \anchor MusicPlayer_Position_Year
///                  _string_,
///     @return The year of release of the song with an offset `number` with
///     respect to the start of the playlist.
///     @param number - the offset numbet with respect to the start of the
///     playlist.
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.Rating`</b>,
///                  \anchor MusicPlayer_Rating
///                  _string_,
///     @return The numeric Rating of current song (1-10).
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.offset(number).Rating`</b>,
///                  \anchor MusicPlayer_OffSet_Rating
///                  _string_,
///     @return The numeric Rating of song with an offset `number` with
///     respect to the current playing song.
///     @param number - the offset with respect to the current playing song
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.Position(number).Rating`</b>,
///                  \anchor MusicPlayer_Position_Rating
///                  _string_,
///     @return The numeric Rating of song with an offset `number` with
///     respect to the start of the playlist.
///     @param number - the offset with respect to the start of the playlist
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.RatingAndVotes`</b>,
///                  \anchor MusicPlayer_RatingAndVotes
///                  _string_,
///     @return The scraped rating and votes of currently playing song\, if it's in the database.
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.UserRating`</b>,
///                  \anchor MusicPlayer_UserRating
///                  _string_,
///     @return The scraped rating of the currently playing song (1-10).
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link MusicPlayer_UserRating `MusicPlayer.UserRating`\endlink
///     <p>  
///   }
///   \table_row3{   <b>`MusicPlayer.Votes`</b>,
///                  \anchor MusicPlayer_Votes
///                  _string_,
///     @return The scraped votes of currently playing song\, if it's in the database.
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.DiscNumber`</b>,
///                  \anchor MusicPlayer_DiscNumber
///                  _string_,
///     @return The Disc Number of current song stored in ID tag info.
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.offset(number).DiscNumber`</b>,
///                  \anchor MusicPlayer_Offset_DiscNumber
///                  _string_,
///     @return The Disc Number of current song stored in ID tag info for the
///     song with an offset `number` with respect to the playing song.
///     @param number - The offset value for the song with respect to the
///     playing song.
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.Position(number).DiscNumber`</b>,
///                  \anchor MusicPlayer_Position_DiscNumber
///                  _string_,
///     @return The Disc Number of current song stored in ID tag info for the
///     song with an offset `number` with respect to the start of the playlist.
///     @param number - The offset value for the song with respect to the
///     start of the playlist.
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.Comment`</b>,
///                  \anchor MusicPlayer_Comment
///                  _string_,
///     @return The Comment of current song stored in ID tag info.
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.offset(number).Comment`</b>,
///                  \anchor MusicPlayer_Offset_Comment
///                  _string_,
///     @return The Comment of current song stored in ID tag info for the
///     song with an offset `number` with respect to the playing song.
///     @param number - The offset value for the song with respect to the
///     playing song.
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.Position(number).Comment`</b>,
///                  \anchor MusicPlayer_Position_Comment
///                  _string_,
///     @return The Comment of current song stored in ID tag info for the
///     song with an offset `number` with respect to the start of the playlist.
///     @param number - The offset value for the song with respect to the
///     start of the playlist.
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.Contributors`</b>,
///                  \anchor MusicPlayer_Contributors
///                  _string_,
///     @return The list of all people who've contributed to the currently playing song
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link MusicPlayer_Contributors `MusicPlayer.Contributors`\endlink
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.ContributorAndRole`</b>,
///                  \anchor MusicPlayer_ContributorAndRole
///                  _string_,
///     @return The list of all people and their role who've contributed to the currently playing song.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link MusicPlayer_ContributorAndRole `MusicPlayer.ContributorAndRole`\endlink
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.Mood`</b>,
///                  \anchor MusicPlayer_Mood
///                  _string_,
///     @return The mood of the currently playing song.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link MusicPlayer_Mood `MusicPlayer.Mood`\endlink
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.PlaylistPlaying`</b>,
///                  \anchor MusicPlayer_PlaylistPlaying
///                  _boolean_,
///     @return **True** if a playlist is currently playing.
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.Exists(relative\,position)`</b>,
///                  \anchor MusicPlayer_Exists
///                  _boolean_,
///     @return **True** if the currently playing playlist has a song queued at the given position.
///     @param relative - bool - If the position is relative
///     @param position - int - The position of the song
///     @note It is possible to define whether the position is relative or not\, default is false.
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.HasPrevious`</b>,
///                  \anchor MusicPlayer_HasPrevious
///                  _boolean_,
///     @return **True** if the music player has a a Previous Song in the Playlist.
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.HasNext`</b>,
///                  \anchor MusicPlayer_HasNext
///                  _boolean_,
///     @return **True** if the music player has a next song queued in the Playlist.
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.PlayCount`</b>,
///                  \anchor MusicPlayer_PlayCount
///                  _integer_,
///     @return The play count of currently playing song\, if it's in the database.
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.LastPlayed`</b>,
///                  \anchor MusicPlayer_LastPlayed
///                  _string_,
///     @return The last play date of currently playing song\, if it's in the database.
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.TrackNumber`</b>,
///                  \anchor MusicPlayer_TrackNumber
///                  _string_,
///     @return The track number of current song.
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.offset(number).TrackNumber`</b>,
///                  \anchor MusicPlayer_Offset_TrackNumber
///                  _string_,
///     @return The track number of the song with an offset `number`
///     with respect to the current playing song.
///     @param number - The offset number of the song with respect to the
///     playing song
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.Position(number).TrackNumber`</b>,
///                  \anchor MusicPlayer_Position_TrackNumber
///                  _string_,
///     @return The track number of the song with an offset `number`
///     with respect to start of the playlist.
///     @param number - The offset number of the song with respect 
///     to start of the playlist
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.Duration`</b>,
///                  \anchor MusicPlayer_Duration
///                  _string_,
///     @return The duration of the current song.
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.offset(number).Duration`</b>,
///                  \anchor MusicPlayer_Offset_Duration
///                  _string_,
///     @return The duration of the song with an offset `number`
///     with respect to the current playing song.
///     @param number - the offset number of the song with respect
///     to the current playing song
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.Position(number).Duration`</b>,
///                  \anchor MusicPlayer_Position_Duration
///                  _string_,
///     @return The duration of the song with an offset `number`
///     with respect to the start of the playlist.
///     @param number - the offset number of the song with respect
///     to the start of the playlist
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.BitRate`</b>,
///                  \anchor MusicPlayer_BitRate
///                  _string_,
///     @return The bitrate of current song.
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.Channels`</b>,
///                  \anchor MusicPlayer_Channels
///                  _string_,
///     @return The number of channels of current song.
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.BitsPerSample`</b>,
///                  \anchor MusicPlayer_BitsPerSample
///                  _string_,
///     @return The number of bits per sample of current song.
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.SampleRate`</b>,
///                  \anchor MusicPlayer_SampleRate
///                  _string_,
///     @return The samplerate of current playing song.
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.Codec`</b>,
///                  \anchor MusicPlayer_Codec
///                  _string_,
///     @return The codec of current playing song.
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.PlaylistPosition`</b>,
///                  \anchor MusicPlayer_PlaylistPosition
///                  _string_,
///     @return The position of the current song in the current music playlist.
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.PlaylistLength`</b>,
///                  \anchor MusicPlayer_PlaylistLength
///                  _string_,
///     @return The total size of the current music playlist.
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.ChannelName`</b>,
///                  \anchor MusicPlayer_ChannelName
///                  _string_,
///     @return The channel name of the radio programme that's currently playing (PVR).
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.ChannelNumberLabel`</b>,
///                  \anchor MusicPlayer_ChannelNumberLabel
///                  _string_,
///     @return The channel and subchannel number of the radio channel that's currently
///     playing (PVR).
///     <p><hr>
///     @skinning_v14 **[New Infolabel]** \link MusicPlayer_ChannelNumberLabel `MusicPlayer.ChannelNumberLabel`\endlink
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.ChannelGroup`</b>,
///                  \anchor MusicPlayer_ChannelGroup
///                  _string_,
///     @return The channel group of the radio programme that's currently playing (PVR).
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.Property(propname)`</b>,
///                  \anchor MusicPlayer_Property_Propname
///                  _string_,
///     @return The requested property value of the currently playing item.
///     @param propname - The requested property
///     <p>
///   }
///   \table_row3{   <b>`MusicPlayer.DBID`</b>,
///                  \anchor MusicPlayer_DBID
///                  _string_,
///     @return The database id of the currently playing song.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link MusicPlayer_DBID `MusicPlayer.DBID`\endlink
///     <p>
///   }
/// \table_end
///
/// -----------------------------------------------------------------------------
const infomap musicplayer[] =    {{ "title",            MUSICPLAYER_TITLE },
                                  { "album",            MUSICPLAYER_ALBUM },
                                  { "artist",           MUSICPLAYER_ARTIST },
                                  { "albumartist",      MUSICPLAYER_ALBUM_ARTIST },
                                  { "year",             MUSICPLAYER_YEAR },
                                  { "genre",            MUSICPLAYER_GENRE },
                                  { "duration",         MUSICPLAYER_DURATION },
                                  { "tracknumber",      MUSICPLAYER_TRACK_NUMBER },
                                  { "cover",            MUSICPLAYER_COVER },
                                  { "bitrate",          MUSICPLAYER_BITRATE },
                                  { "playlistlength",   MUSICPLAYER_PLAYLISTLEN },
                                  { "playlistposition", MUSICPLAYER_PLAYLISTPOS },
                                  { "channels",         MUSICPLAYER_CHANNELS },
                                  { "bitspersample",    MUSICPLAYER_BITSPERSAMPLE },
                                  { "samplerate",       MUSICPLAYER_SAMPLERATE },
                                  { "codec",            MUSICPLAYER_CODEC },
                                  { "discnumber",       MUSICPLAYER_DISC_NUMBER },
                                  { "rating",           MUSICPLAYER_RATING },
                                  { "ratingandvotes",   MUSICPLAYER_RATING_AND_VOTES },
                                  { "userrating",       MUSICPLAYER_USER_RATING },
                                  { "votes",            MUSICPLAYER_VOTES },
                                  { "comment",          MUSICPLAYER_COMMENT },
                                  { "mood",             MUSICPLAYER_MOOD },
                                  { "contributors",     MUSICPLAYER_CONTRIBUTORS },
                                  { "contributorandrole", MUSICPLAYER_CONTRIBUTOR_AND_ROLE },
                                  { "lyrics",           MUSICPLAYER_LYRICS },
                                  { "playlistplaying",  MUSICPLAYER_PLAYLISTPLAYING },
                                  { "exists",           MUSICPLAYER_EXISTS },
                                  { "hasprevious",      MUSICPLAYER_HASPREVIOUS },
                                  { "hasnext",          MUSICPLAYER_HASNEXT },
                                  { "playcount",        MUSICPLAYER_PLAYCOUNT },
                                  { "lastplayed",       MUSICPLAYER_LASTPLAYED },
                                  { "channelname",      MUSICPLAYER_CHANNEL_NAME },
                                  { "channelnumberlabel", MUSICPLAYER_CHANNEL_NUMBER },
                                  { "channelgroup",     MUSICPLAYER_CHANNEL_GROUP },
                                  { "dbid",             MUSICPLAYER_DBID },
                                  { "property",         MUSICPLAYER_PROPERTY },
};

/// \page modules__infolabels_boolean_conditions
/// \subsection modules__infolabels_boolean_conditions_Videoplayer Video player
/// \table_start
///   \table_h3{ Labels, Type, Description }
///   \table_row3{   <b>`VideoPlayer.UsingOverlays`</b>,
///                  \anchor VideoPlayer_UsingOverlays
///                  _boolean_,
///     @return **True** if the video player is using the hardware overlays render
///     method.
///     @note This is useful\, as with hardware overlays you have no alpha blending to
///     the video image\, so shadows etc. need redoing\, or disabling.
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.IsFullscreen`</b>,
///                  \anchor VideoPlayer_IsFullscreen
///                  _boolean_,
///     @return **True** if the video player is in fullscreen mode.
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.HasMenu`</b>,
///                  \anchor VideoPlayer_HasMenu
///                  _boolean_,
///     @return **True** if the video player has a menu (ie is playing a DVD).
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.HasInfo`</b>,
///                  \anchor VideoPlayer_HasInfo
///                  _boolean_,
///     @return **True** if the current playing video has information from the
///     library or from a plugin (eg director/plot etc.)
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.Content(parameter)`</b>,
///                  \anchor VideoPlayer_Content
///                  _boolean_,
///     @return **True** if the current Video you are playing is contained in
///     corresponding Video Library sections. The following values are accepted:
///     - <b>files</b>
///     - <b>movies</b>
///     - <b>episodes</b>
///     - <b>musicvideos</b>
///     - <b>livetv</b>
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.HasSubtitles`</b>,
///                  \anchor VideoPlayer_HasSubtitles
///                  _boolean_,
///     @return **True** if there are subtitles available for video.
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.HasTeletext`</b>,
///                  \anchor VideoPlayer_HasTeletext
///                  _boolean_,
///     @return **True** if teletext is usable on played TV channel.
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.IsStereoscopic`</b>,
///                  \anchor VideoPlayer_IsStereoscopic
///                  _boolean_,
///     @return **True** when the currently playing video is a 3D (stereoscopic)
///     video.
///     <p><hr>
///     @skinning_v13 **[New Boolean Condition]** \link VideoPlayer_IsStereoscopic `VideoPlayer.IsStereoscopic`\endlink
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.SubtitlesEnabled`</b>,
///                  \anchor VideoPlayer_SubtitlesEnabled
///                  _boolean_,
///     @return **True** if subtitles are turned on for video.
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.HasEpg`</b>,
///                  \anchor VideoPlayer_HasEpg
///                  _boolean_,
///     @return **True** if epg information is available for the currently playing
///     programme (PVR).
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.CanResumeLiveTV`</b>,
///                  \anchor VideoPlayer_CanResumeLiveTV
///                  _boolean_,
///     @return **True** if a in-progress PVR recording is playing an the respective
///     live TV channel is available.
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.Title`</b>,
///                  \anchor VideoPlayer_Title
///                  _string_,
///     @return The title of currently playing video.
///     @note If it's in the database it will return the database title\, else the filename.
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.OriginalTitle`</b>,
///                  \anchor VideoPlayer_OriginalTitle
///                  _string_,
///     @return The original title of currently playing video. If it's in the database.
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.TVShowTitle`</b>,
///                  \anchor VideoPlayer_TVShowTitle
///                  _string_,
///     @return The title of currently playing episode's tvshow name.
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.Season`</b>,
///                  \anchor VideoPlayer_Season
///                  _string_,
///     @return The season number of the currently playing episode\, if it's in the database.
///     <p><hr>
///     @skinning_v15 **[Infolabel Updated]** \link VideoPlayer_Season `VideoPlayer.Season`\endlink
///     also supports EPG.
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.Episode`</b>,
///                  \anchor VideoPlayer_Episode
///                  _string_,
///     @return The episode number of the currently playing episode.
///     <p><hr>
///     @skinning_v15 **[Infolabel Updated]** \link VideoPlayer_Episode `VideoPlayer.Episode`\endlink
///     also supports EPG.
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.Genre`</b>,
///                  \anchor VideoPlayer_Genre
///                  _string_,
///     @return The genre(s) of current movie\, if it's in the database.
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.Director`</b>,
///                  \anchor VideoPlayer_Director
///                  _string_,
///     @return The director of current movie\, if it's in the database.
///     <p><hr>
///     @skinning_v15 **[Infolabel Updated]** \link VideoPlayer_Director `VideoPlayer.Director`\endlink
///     also supports EPG.
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.Country`</b>,
///                  \anchor VideoPlayer_Country
///                  _string_,
///     @return The production country of current movie\, if it's in the database.
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.Year`</b>,
///                  \anchor VideoPlayer_Year
///                  _string_,
///     @return The year of release of current movie\, if it's in the database.
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.Cover`</b>,
///                  \anchor VideoPlayer_Cover
///                  _string_,
///     @return The cover of currently playing movie.
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.Rating`</b>,
///                  \anchor VideoPlayer_Rating
///                  _string_,
///     @return The scraped rating of current movie\, if it's in the database.
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.UserRating`</b>,
///                  \anchor VideoPlayer_UserRating
///                  _string_,
///     @return The user rating of the currently playing item.
///     <p><hr>
///     @skinning_v16 **[New Infolabel]** \link VideoPlayer_UserRating `VideoPlayer.UserRating`\endlink
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.Votes`</b>,
///                  \anchor VideoPlayer_Votes
///                  _string_,
///     @return The scraped votes of current movie\, if it's in the database.
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link VideoPlayer_Votes `VideoPlayer.Votes`\endlink
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.RatingAndVotes`</b>,
///                  \anchor VideoPlayer_RatingAndVotes
///                  _string_,
///     @return The scraped rating and votes of current movie\, if it's in the database
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.mpaa`</b>,
///                  \anchor VideoPlayer_mpaa
///                  _string_,
///     @return The MPAA rating of current movie\, if it's in the database.
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.IMDBNumber`</b>,
///                  \anchor VideoPlayer_IMDBNumber
///                  _string_,
///     @return The IMDb ID of the current movie\, if it's in the database.
///     <p><hr>
///     @skinning_v15 **[New Infolabel]** \link VideoPlayer_IMDBNumber `VideoPlayer.IMDBNumber`\endlink
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.Top250`</b>,
///                  \anchor VideoPlayer_Top250
///                  _string_,
///     @return The IMDb Top250 position of the currently playing movie\, if it's in the database.
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.EpisodeName`</b>,
///                  \anchor VideoPlayer_EpisodeName
///                  _string_,
///     @return The name of the episode if the playing video is a TV Show\,
///     if it's in the database (PVR).
///     <p><hr>
///     @skinning_v15 **[New Infolabel]** \link VideoPlayer_EpisodeName `VideoPlayer.EpisodeName`\endlink
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.PlaylistPosition`</b>,
///                  \anchor VideoPlayer_PlaylistPosition
///                  _string_,
///     @return The position of the current song in the current video playlist.
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.PlaylistLength`</b>,
///                  \anchor VideoPlayer_PlaylistLength
///                  _string_,
///     @return The total size of the current video playlist.
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.Cast`</b>,
///                  \anchor VideoPlayer_Cast
///                  _string_,
///     @return A concatenated string of cast members of the current movie\, if it's in
///     the database.
///     <p><hr>
///     @skinning_v15 **[Infolabel Updated]** \link VideoPlayer_Cast `VideoPlayer.Cast`\endlink
///     also supports EPG.
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.CastAndRole`</b>,
///                  \anchor VideoPlayer_CastAndRole
///                  _string_,
///     @return A concatenated string of cast members and roles of the current movie\,
///     if it's in the database.
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.Album`</b>,
///                  \anchor VideoPlayer_Album
///                  _string_,
///     @return The album from which the current Music Video is from\, if it's in the database.
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.Artist`</b>,
///                  \anchor VideoPlayer_Artist
///                  _string_,
///     @return The artist(s) of current Music Video\, if it's in the database.
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.Studio`</b>,
///                  \anchor VideoPlayer_Studio
///                  _string_,
///     @return The studio of current Music Video\, if it's in the database.
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.Writer`</b>,
///                  \anchor VideoPlayer_Writer
///                  _string_,
///     @return The name of Writer of current playing Video\, if it's in the database.
///     <p><hr>
///     @skinning_v15 **[Infolabel Updated]** \link VideoPlayer_Writer `VideoPlayer.Writer`\endlink
///     also supports EPG.
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.Tagline`</b>,
///                  \anchor VideoPlayer_Tagline
///                  _string_,
///     @return The small Summary of current playing Video\, if it's in the database.
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.PlotOutline`</b>,
///                  \anchor VideoPlayer_PlotOutline
///                  _string_,
///     @return The small Summary of current playing Video\, if it's in the database.
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.Plot`</b>,
///                  \anchor VideoPlayer_Plot
///                  _string_,
///     @return The complete Text Summary of current playing Video\, if it's in the database.
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.Premiered`</b>,
///                  \anchor VideoPlayer_Premiered
///                  _string_,
///     @return The release or aired date of the currently playing episode\, show\, movie or EPG item\,
///     if it's in the database.
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.Trailer`</b>,
///                  \anchor VideoPlayer_Trailer
///                  _string_,
///     @return The path to the trailer of the currently playing movie\, if it's in the database.
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.LastPlayed`</b>,
///                  \anchor VideoPlayer_LastPlayed
///                  _string_,
///     @return The last play date of current playing Video\, if it's in the database.
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.PlayCount`</b>,
///                  \anchor VideoPlayer_PlayCount
///                  _string_,
///     @return The playcount of current playing Video\, if it's in the database.
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.VideoCodec`</b>,
///                  \anchor VideoPlayer_VideoCodec
///                  _string_,
///     @return The video codec of the currently playing video (common values: see
///     \ref ListItem_VideoCodec "ListItem.VideoCodec").
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.VideoResolution`</b>,
///                  \anchor VideoPlayer_VideoResolution
///                  _string_,
///     @return The video resolution of the currently playing video (possible
///     values: see \ref ListItem_VideoResolution "ListItem.VideoResolution").
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.VideoAspect`</b>,
///                  \anchor VideoPlayer_VideoAspect
///                  _string_,
///     @return The aspect ratio of the currently playing video (possible values:
///     see \ref ListItem_VideoAspect "ListItem.VideoAspect").
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.AudioCodec`</b>,
///                  \anchor VideoPlayer_AudioCodec
///                  _string_,
///     @return The audio codec of the currently playing video\, optionally 'n'
///     defines the number of the audiostream (common values: see
///     \ref ListItem_AudioCodec "ListItem.AudioCodec").
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.AudioChannels`</b>,
///                  \anchor VideoPlayer_AudioChannels
///                  _string_,
///     @return The number of audio channels of the currently playing video
///     (possible values: see \ref ListItem_AudioChannels "ListItem.AudioChannels").
///     <p><hr>
///     @skinning_v16 **[Infolabel Updated]** \link VideoPlayer_AudioChannels `VideoPlayer.AudioChannels`\endlink
///     if a video contains no audio\, these infolabels will now return empty.
///     (they used to return 0)
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.AudioLanguage`</b>,
///                  \anchor VideoPlayer_AudioLanguage
///                  _string_,
///     @return The language of the audio of the currently playing video(possible
///     values: see \ref ListItem_AudioLanguage "ListItem.AudioLanguage").
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link VideoPlayer_AudioLanguage `VideoPlayer.AudioLanguage`\endlink
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.SubtitlesLanguage`</b>,
///                  \anchor VideoPlayer_SubtitlesLanguage
///                  _string_,
///     @return The language of the subtitle of the currently playing video
///     (possible values: see \ref ListItem_SubtitleLanguage "ListItem.SubtitleLanguage").
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link VideoPlayer_SubtitlesLanguage `VideoPlayer.SubtitlesLanguage`\endlink
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.StereoscopicMode`</b>,
///                  \anchor VideoPlayer_StereoscopicMode
///                  _string_,
///     @return The stereoscopic mode of the currently playing video (possible
///     values: see \ref ListItem_StereoscopicMode "ListItem.StereoscopicMode").
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link VideoPlayer_StereoscopicMode `VideoPlayer.StereoscopicMode`\endlink
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.StartTime`</b>,
///                  \anchor VideoPlayer_StartTime
///                  _string_,
///     @return The start date and time of the currently playing epg event or recording (PVR).
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.EndTime`</b>,
///                  \anchor VideoPlayer_EndTime
///                  _string_,
///     @return The end date and time of the currently playing epg event or recording (PVR).
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.NextTitle`</b>,
///                  \anchor VideoPlayer_NextTitle
///                  _string_,
///     @return The title of the programme that will be played next (PVR).
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.NextGenre`</b>,
///                  \anchor VideoPlayer_NextGenre
///                  _string_,
///     @return The genre of the programme that will be played next (PVR).
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.NextPlot`</b>,
///                  \anchor VideoPlayer_NextPlot
///                  _string_,
///     @return The plot of the programme that will be played next (PVR).
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.NextPlotOutline`</b>,
///                  \anchor VideoPlayer_NextPlotOutline
///                  _string_,
///     @return The plot outline of the programme that will be played next (PVR).
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.NextStartTime`</b>,
///                  \anchor VideoPlayer_NextStartTime
///                  _string_,
///     @return The start time of the programme that will be played next (PVR).
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.NextEndTime`</b>,
///                  \anchor VideoPlayer_NextEndTime
///                  _string_,
///     @return The end time of the programme that will be played next (PVR).
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.NextDuration`</b>,
///                  \anchor VideoPlayer_NextDuration
///                  _string_,
///     @return The duration of the programme that will be played next (PVR).
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.ChannelName`</b>,
///                  \anchor VideoPlayer_ChannelName
///                  _string_,
///     @return The name of the currently tuned channel (PVR).
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.ChannelNumberLabel`</b>,
///                  \anchor VideoPlayer_ChannelNumberLabel
///                  _string_,
///     @return The channel and subchannel number of the tv channel that's currently playing (PVR).
///     <p><hr>
///     @skinning_v14 **[New Infolabel]** \link VideoPlayer_ChannelNumberLabel `VideoPlayer.ChannelNumberLabel`\endlink
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.ChannelGroup`</b>,
///                  \anchor VideoPlayer_ChannelGroup
///                  _string_,
///     @return The group of the currently tuned channel (PVR).
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.ParentalRating`</b>,
///                  \anchor VideoPlayer_ParentalRating
///                  _string_,
///     @return The parental rating of the currently playing programme (PVR).
///     <p>
///   }
///   \table_row3{   <b>`VideoPlayer.DBID`</b>,
///                  \anchor VideoPlayer_DBID
///                  _string_,
///     @return The database id of the currently playing video
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link VideoPlayer_DBID `VideoPlayer.DBID`\endlink
///     <p>
///   }
/// \table_end
///
/// -----------------------------------------------------------------------------
const infomap videoplayer[] =    {{ "title",            VIDEOPLAYER_TITLE },
                                  { "genre",            VIDEOPLAYER_GENRE },
                                  { "country",          VIDEOPLAYER_COUNTRY },
                                  { "originaltitle",    VIDEOPLAYER_ORIGINALTITLE },
                                  { "director",         VIDEOPLAYER_DIRECTOR },
                                  { "year",             VIDEOPLAYER_YEAR },
                                  { "cover",            VIDEOPLAYER_COVER },
                                  { "usingoverlays",    VIDEOPLAYER_USING_OVERLAYS },
                                  { "isfullscreen",     VIDEOPLAYER_ISFULLSCREEN },
                                  { "hasmenu",          VIDEOPLAYER_HASMENU },
                                  { "playlistlength",   VIDEOPLAYER_PLAYLISTLEN },
                                  { "playlistposition", VIDEOPLAYER_PLAYLISTPOS },
                                  { "plot",             VIDEOPLAYER_PLOT },
                                  { "plotoutline",      VIDEOPLAYER_PLOT_OUTLINE },
                                  { "episode",          VIDEOPLAYER_EPISODE },
                                  { "season",           VIDEOPLAYER_SEASON },
                                  { "rating",           VIDEOPLAYER_RATING },
                                  { "ratingandvotes",   VIDEOPLAYER_RATING_AND_VOTES },
                                  { "userrating",       VIDEOPLAYER_USER_RATING },
                                  { "votes",            VIDEOPLAYER_VOTES },
                                  { "tvshowtitle",      VIDEOPLAYER_TVSHOW },
                                  { "premiered",        VIDEOPLAYER_PREMIERED },
                                  { "studio",           VIDEOPLAYER_STUDIO },
                                  { "mpaa",             VIDEOPLAYER_MPAA },
                                  { "top250",           VIDEOPLAYER_TOP250 },
                                  { "cast",             VIDEOPLAYER_CAST },
                                  { "castandrole",      VIDEOPLAYER_CAST_AND_ROLE },
                                  { "artist",           VIDEOPLAYER_ARTIST },
                                  { "album",            VIDEOPLAYER_ALBUM },
                                  { "writer",           VIDEOPLAYER_WRITER },
                                  { "tagline",          VIDEOPLAYER_TAGLINE },
                                  { "hasinfo",          VIDEOPLAYER_HAS_INFO },
                                  { "trailer",          VIDEOPLAYER_TRAILER },
                                  { "videocodec",       VIDEOPLAYER_VIDEO_CODEC },
                                  { "videoresolution",  VIDEOPLAYER_VIDEO_RESOLUTION },
                                  { "videoaspect",      VIDEOPLAYER_VIDEO_ASPECT },
                                  { "videobitrate",     VIDEOPLAYER_VIDEO_BITRATE },
                                  { "audiocodec",       VIDEOPLAYER_AUDIO_CODEC },
                                  { "audiochannels",    VIDEOPLAYER_AUDIO_CHANNELS },
                                  { "audiobitrate",     VIDEOPLAYER_AUDIO_BITRATE },
                                  { "audiolanguage",    VIDEOPLAYER_AUDIO_LANG },
                                  { "hasteletext",      VIDEOPLAYER_HASTELETEXT },
                                  { "lastplayed",       VIDEOPLAYER_LASTPLAYED },
                                  { "playcount",        VIDEOPLAYER_PLAYCOUNT },
                                  { "hassubtitles",     VIDEOPLAYER_HASSUBTITLES },
                                  { "subtitlesenabled", VIDEOPLAYER_SUBTITLESENABLED },
                                  { "subtitleslanguage",VIDEOPLAYER_SUBTITLES_LANG },
                                  { "starttime",        VIDEOPLAYER_STARTTIME },
                                  { "endtime",          VIDEOPLAYER_ENDTIME },
                                  { "nexttitle",        VIDEOPLAYER_NEXT_TITLE },
                                  { "nextgenre",        VIDEOPLAYER_NEXT_GENRE },
                                  { "nextplot",         VIDEOPLAYER_NEXT_PLOT },
                                  { "nextplotoutline",  VIDEOPLAYER_NEXT_PLOT_OUTLINE },
                                  { "nextstarttime",    VIDEOPLAYER_NEXT_STARTTIME },
                                  { "nextendtime",      VIDEOPLAYER_NEXT_ENDTIME },
                                  { "nextduration",     VIDEOPLAYER_NEXT_DURATION },
                                  { "channelname",      VIDEOPLAYER_CHANNEL_NAME },
                                  { "channelnumberlabel", VIDEOPLAYER_CHANNEL_NUMBER },
                                  { "channelgroup",     VIDEOPLAYER_CHANNEL_GROUP },
                                  { "hasepg",           VIDEOPLAYER_HAS_EPG },
                                  { "parentalrating",   VIDEOPLAYER_PARENTAL_RATING },
                                  { "isstereoscopic",   VIDEOPLAYER_IS_STEREOSCOPIC },
                                  { "stereoscopicmode", VIDEOPLAYER_STEREOSCOPIC_MODE },
                                  { "canresumelivetv",  VIDEOPLAYER_CAN_RESUME_LIVE_TV },
                                  { "imdbnumber",       VIDEOPLAYER_IMDBNUMBER },
                                  { "episodename",      VIDEOPLAYER_EPISODENAME },
                                  { "dbid",             VIDEOPLAYER_DBID }
};

/// \page modules__infolabels_boolean_conditions
/// \subsection modules__infolabels_boolean_conditions_RetroPlayer RetroPlayer
/// \table_start
///   \table_h3{ Labels, Type, Description }
///   \table_row3{   <b>`RetroPlayer.VideoFilter`</b>,
///                  \anchor RetroPlayer_VideoFilter
///                  _string_,
///     @return The video filter of the currently-playing game.
///     The following values are possible:
///       - nearest (Nearest neighbor\, i.e. pixelate)
///       - linear (Bilinear filtering\, i.e. smooth blur)
///     <p><hr>
///     @skinning_v18 **[New Infolabel]** \link RetroPlayer_VideoFilter `RetroPlayer.VideoFilter`\endlink
///     <p>
///   }
///   \table_row3{   <b>`RetroPlayer.StretchMode`</b>,
///                  \anchor RetroPlayer_StretchMode
///                  _string_,
///     @return The stretch mode of the currently-playing game.
///     The following values are possible:
///       - normal (Show the game normally)
///       - 4:3 (Stretch to a 4:3 aspect ratio)
///       - fullscreen (Stretch to the full viewing area)
///       - original (Shrink to the original resolution)
///     <p><hr>
///     @skinning_v18 **[New Infolabel]** \link RetroPlayer_StretchMode `RetroPlayer.StretchMode`\endlink
///     <p>  
///   }
///   \table_row3{   <b>`RetroPlayer.VideoRotation`</b>,
///                  \anchor RetroPlayer_VideoRotation
///                  _integer_,
///     @return The video rotation of the currently-playing game
///     in degrees counter-clockwise.
///     The following values are possible:
///       - 0
///       - 90 (Shown in the GUI as 270 degrees)
///       - 180
///       - 270 (Shown in the GUI as 90 degrees)
///     <p><hr>
///     @skinning_v18 **[New Infolabel]** \link RetroPlayer_VideoRotation `RetroPlayer.VideoRotation`\endlink
///     <p>  
///   }
/// \table_end
///
/// -----------------------------------------------------------------------------
const infomap retroplayer[] =
{
  { "videofilter",            RETROPLAYER_VIDEO_FILTER},
  { "stretchmode",            RETROPLAYER_STRETCH_MODE},
  { "videorotation",          RETROPLAYER_VIDEO_ROTATION},
};

/// \page modules__infolabels_boolean_conditions
/// \subsection modules__infolabels_boolean_conditions_Container Container
/// \table_start
///   \table_h3{ Labels, Type, Description }
///   \table_row3{   <b>`Container(id).HasFiles`</b>,
///                  \anchor Container_HasFiles
///                  _boolean_,
///     @return **True** if the container contains files (or current container if
///     id is omitted).
///     <p>
///   }
///   \table_row3{   <b>`Container(id).HasFolders`</b>,
///                  \anchor Container_HasFolders
///                  _boolean_,
///     @return **True** if the container contains folders (or current container if
///     id is omitted).
///     <p>
///   }
///   \table_row3{   <b>`Container(id).IsStacked`</b>,
///                  \anchor Container_IsStacked
///                  _boolean_,
///     @return **True** if the container is currently in stacked mode (or current
///     container if id is omitted).
///     <p>
///   }
///   \table_row3{   <b>`Container.FolderPath`</b>,
///                  \anchor Container_FolderPath
///                  _string_,
///     @return The complete path of currently displayed folder.
///     <p>
///   }
///   \table_row3{   <b>`Container.FolderName`</b>,
///                  \anchor Container_FolderName
///                  _string_,
///     @return The top most folder in currently displayed folder.
///     <p>
///   }
///   \table_row3{   <b>`Container.PluginName`</b>,
///                  \anchor Container_PluginName
///                  _string_,
///     @return The current plugins base folder name.
///     <p>
///   }
///   \table_row3{   <b>`Container.PluginCategory`</b>,
///                  \anchor Container_PluginCategory
///                  _string_,
///     @return The current plugins category (set by the scripter).
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link Container_PluginCategory `Container.PluginCategory`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Container.Viewmode`</b>,
///                  \anchor Container_Viewmode
///                  _string_,
///     @return The current viewmode (list\, icons etc).
///     <p>
///   }
///   \table_row3{   <b>`Container.ViewCount`</b>,
///                  \anchor Container_ViewCount
///                  _integer_,
///     @return The number of available skin view modes for the current container listing.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link Container_ViewCount `Container.ViewCount`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Container(id).Totaltime`</b>,
///                  \anchor Container_Totaltime
///                  _string_,
///     @return The total time of all items in the current container.
///     <p>
///   }
///   \table_row3{   <b>`Container(id).TotalWatched`</b>,
///                  \anchor Container_TotalWatched
///                  _string_,
///     @return The number of watched items in the container.
///     @param id - [opt] if not supplied the current container will be used.
///     <p><hr>
///     @skinning_v16 **[New Infolabel]** \link Container_TotalWatched `Container(id).TotalWatched`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Container(id).TotalUnWatched`</b>,
///                  \anchor Container_TotalUnWatched
///                  _string_,
///     @return The number of unwatched items in the container.
///     @param id - [opt] if not supplied the current container will be used.
///     <p><hr>
///     @skinning_v16 **[New Infolabel]** \link Container_TotalUnWatched `Container(id).TotalUnWatched`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Container.HasThumb`</b>,
///                  \anchor Container_HasThumb
///                  _boolean_,
///     @return **True** if the current container you are in has a thumb assigned
///     to it.
///     <p>
///   }
///   \table_row3{   <b>`Container.SortMethod`</b>,
///                  \anchor Container_SortMethod
///                  _boolean_,
///     @return **True** the current sort method (name\, year\, rating\, etc).
///     <p>
///   }
///   \table_row3{   <b>`Container.SortOrder`</b>,
///                  \anchor Container_SortOrder
///                  _string_,
///     @return The current sort order (Ascending/Descending).
///     <p><hr>
///     @skinning_v16 **[New Infolabel]** \link Container_SortOrder `Container.SortOrder`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Container.ShowPlot`</b>,
///                  \anchor Container_ShowPlot
///                  _string_,
///     @return The TV Show plot of the current container and can be used at
///     season and episode level.
///     <p>
///   }
///   \table_row3{   <b>`Container.ShowTitle`</b>,
///                  \anchor Container_ShowTitle
///                  _string_,
///     @return The TV Show title of the current container and can be used at
///     season and episode level.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link Container_ShowTitle `Container.ShowTitle`\endlink
///     <p>
///   }
const infomap mediacontainer[] = {{ "hasfiles",         CONTAINER_HASFILES },
                                  { "hasfolders",       CONTAINER_HASFOLDERS },
                                  { "isstacked",        CONTAINER_STACKED },
                                  { "folderpath",       CONTAINER_FOLDERPATH },
                                  { "foldername",       CONTAINER_FOLDERNAME },
                                  { "pluginname",       CONTAINER_PLUGINNAME },
                                  { "plugincategory",   CONTAINER_PLUGINCATEGORY },
                                  { "viewmode",         CONTAINER_VIEWMODE },
                                  { "viewcount",        CONTAINER_VIEWCOUNT },
                                  { "totaltime",        CONTAINER_TOTALTIME },
                                  { "totalwatched",     CONTAINER_TOTALWATCHED },
                                  { "totalunwatched",   CONTAINER_TOTALUNWATCHED },
                                  { "hasthumb",         CONTAINER_HAS_THUMB },
                                  { "sortmethod",       CONTAINER_SORT_METHOD },
                                  { "sortorder",        CONTAINER_SORT_ORDER },
                                  { "showplot",         CONTAINER_SHOWPLOT },
                                  { "showtitle",        CONTAINER_SHOWTITLE }};

/// \page modules__infolabels_boolean_conditions
///   \table_row3{   <b>`Container(id).OnNext`</b>,
///                  \anchor Container_OnNext
///                  _boolean_,
///     @return **True** if the container with id (or current container if id is
///     omitted) is moving to the next item. Allows views to be
///     custom-designed (such as 3D coverviews etc.)
///     <p>
///   }
///   \table_row3{   <b>`Container(id).OnScrollNext`</b>,
///                  \anchor Container_OnScrollNext
///                  _boolean_,
///     @return **True** if the container with id (or current container if id is
///     omitted) is scrolling to the next item. Differs from \ref Container_OnNext "OnNext" in that
///     \ref Container_OnNext "OnNext" triggers on movement even if there is no scroll involved.
///     <p>
///   }
///   \table_row3{   <b>`Container(id).OnPrevious`</b>,
///                  \anchor Container_OnPrevious
///                  _boolean_,
///     @return **True** if the container with id (or current container if id is
///     omitted) is moving to the previous item. Allows views to be
///     custom-designed (such as 3D coverviews etc).
///     <p>
///   }
///   \table_row3{   <b>`Container(id).OnScrollPrevious`</b>,
///                  \anchor Container_OnScrollPrevious
///                  _boolean_,
///     @return **True** if the container with id (or current container if id is
///     omitted) is scrolling to the previous item. Differs from \ref Container_OnPrevious "OnPrevious" in
///     that \ref Container_OnPrevious "OnPrevious" triggers on movement even if there is no scroll involved.
///     <p>
///   }
///   \table_row3{   <b>`Container(id).NumPages`</b>,
///                  \anchor Container_NumPages
///                  _integer_,
///     @return The number of pages in the container with given id. If no id is specified it
///     grabs the current container.
///     <p>
///   }
///   \table_row3{   <b>`Container(id).NumItems`</b>,
///                  \anchor Container_NumItems
///                  _integer_,
///     @return The number of items in the container or grouplist with given id excluding parent folder item. 
///     @note If no id is specified it grabs the current container.
///     <p>
///   }
///   \table_row3{   <b>`Container(id).NumAllItems`</b>,
///                  \anchor Container_NumAllItems
///                  _integer_,
///     @return The number of all items in the container or grouplist with given id including parent folder item. 
///     @note If no id is specified it grabs the current container.
///     <p><hr>
///     @skinning_v18 **[New Infolabel]** \link Container_NumAllItems `Container(id).NumAllItems`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Container(id).NumNonFolderItems`</b>,
///                  \anchor Container_NumNonFolderItems
///                  _integer_,
///     @return The Number of items in the container or grouplist with given id excluding all folder items.
///     @note **Example:** pvr recordings folders\, parent ".." folder). 
///     If no id is specified it grabs the current container.
///     <p><hr>
///     @skinning_v18 **[New Infolabel]** \link Container_NumNonFolderItems `Container(id).NumNonFolderItems`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Container(id).CurrentPage`</b>,
///                  \anchor Container_CurrentPage
///                  _string_,
///     @return THe current page in the container with given id.
///     @note If no id is specified it grabs the current container.
///     <p>
///   }
///   \table_row3{   <b>`Container(id).Scrolling`</b>,
///                  \anchor Container_Scrolling
///                  _boolean_,
///     @return **True** if the user is currently scrolling through the container
///     with id (or current container if id is omitted).
///     @note This is slightly delayed from the actual scroll start. Use
///     \ref Container_OnScrollNext "Container(id).OnScrollNext" or 
///     \ref Container_OnScrollPrevious "Container(id).OnScrollPrevious" to trigger animations
///     immediately on scroll.
///     <p>
///   }
///   \table_row3{   <b>`Container(id).HasNext`</b>,
///                  \anchor Container_HasNext
///                  _boolean_,
///     @return **True** if the container or textbox with id (id) has a next page.
///     <p>
///   }
///   \table_row3{   <b>`Container.HasParent`</b>,
///                  \anchor Container_HasParent
///                  _boolean_,
///     @return **True** when the container contains a parent ('..') item.
///     <p><hr>
///     @skinning_v16 **[New Boolean Condition]** \link Container_HasParent `Container.HasParent`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Container(id).HasPrevious`</b>,
///                  \anchor Container_HasPrevious
///                  _boolean_,
///     @return **True** if the container or textbox with id (id) has a previous page.
///     <p>
///   }
///   \table_row3{   <b>`Container.CanFilter`</b>,
///                  \anchor Container_CanFilter
///                  _boolean_,
///     @return **True** when the current container can be filtered.
///     <p>
///   }
///   \table_row3{   <b>`Container.CanFilterAdvanced`</b>,
///                  \anchor Container_CanFilterAdvanced
///                  _boolean_,
///     @return **True** when advanced filtering can be applied to the current container.
///     <p>
///   }
///   \table_row3{   <b>`Container.Filtered`</b>,
///                  \anchor Container_Filtered
///                  _boolean_,
///     @return **True** when a mediafilter is applied to the current container.
///     <p>
///   }
///   \table_row3{   <b>`Container(id).IsUpdating`</b>,
///                  \anchor Container_IsUpdating
///                  _boolean_,
///     @return **True** if the container with dynamic list content is currently updating.
///   }
const infomap container_bools[] ={{ "onnext",           CONTAINER_MOVE_NEXT },
                                  { "onprevious",       CONTAINER_MOVE_PREVIOUS },
                                  { "onscrollnext",     CONTAINER_SCROLL_NEXT },
                                  { "onscrollprevious", CONTAINER_SCROLL_PREVIOUS },
                                  { "numpages",         CONTAINER_NUM_PAGES },
                                  { "numitems",         CONTAINER_NUM_ITEMS },
                                  { "numnonfolderitems", CONTAINER_NUM_NONFOLDER_ITEMS },
                                  { "numallitems",      CONTAINER_NUM_ALL_ITEMS },
                                  { "currentpage",      CONTAINER_CURRENT_PAGE },
                                  { "scrolling",        CONTAINER_SCROLLING },
                                  { "hasnext",          CONTAINER_HAS_NEXT },
                                  { "hasparent",        CONTAINER_HAS_PARENT_ITEM },
                                  { "hasprevious",      CONTAINER_HAS_PREVIOUS },
                                  { "canfilter",        CONTAINER_CAN_FILTER },
                                  { "canfilteradvanced",CONTAINER_CAN_FILTERADVANCED },
                                  { "filtered",         CONTAINER_FILTERED },
                                  { "isupdating",       CONTAINER_ISUPDATING }};

/// \page modules__infolabels_boolean_conditions
///   \table_row3{   <b>`Container(id).Row`</b>,
///                  \anchor Container_Row
///                  _integer_,
///     @return The row number of the focused position in a panel container.
///     <p><hr>
///     @skinning_v16 **[New Infolabel]** \link Container_Row `Container(id).Row`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Container(id).Column`</b>,
///                  \anchor Container_Column
///                  _integer_,
///     @return The column number of the focused position in a panel container.
///     <p><hr>
///     @skinning_v16 **[New Infolabel]** \link Container_Column `Container(id).Column`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Container(id).Position`</b>,
///                  \anchor Container_Position
///                  _integer_,
///     @return The current focused position of container / grouplist (id) as a
///     numeric label.
///     <p><hr>
///     @skinning_v16 **[Infolabel Updated]** \link Container_Position `Container(id).Position`\endlink
///     now also returns the position for items inside a grouplist.
///     <p>
///   }
///   \table_row3{   <b>`Container(id).CurrentItem`</b>,
///                  \anchor Container_CurrentItem
///                  _integer_,
///     @return The current item in the container or grouplist with given id. 
///     @note If no id is specified it grabs the current container.
///     <p><hr>
///     @skinning_v15 **[New Infolabel]** \link Container_CurrentItem `Container(id).CurrentItem`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Container(id).SubItem`</b>,
///                  \anchor Container_SubItem
///                  _integer_,
///     @return Sub-item in the container or grouplist with given id.
///     @note If no id is specified it grabs the current container.
///     <p>
///   }
///   \table_row3{   <b>`Container(id).HasFocus(item_number)`</b>,
///                  \anchor Container_HasFocus
///                  _boolean_,
///     @return **True** if the container with id (or current container if id is
///     omitted) has static content and is focused on the item with id
///     item_number.
///     <p>
///   }
const infomap container_ints[] = {{ "row",              CONTAINER_ROW },
                                  { "column",           CONTAINER_COLUMN },
                                  { "position",         CONTAINER_POSITION },
                                  { "currentitem",      CONTAINER_CURRENT_ITEM },
                                  { "subitem",          CONTAINER_SUBITEM },
                                  { "hasfocus",         CONTAINER_HAS_FOCUS }};

/// \page modules__infolabels_boolean_conditions
///   \table_row3{   <b>`Container.Property(addoncategory)`</b>,
///                  \anchor Container_Property_addoncategory
///                  _string_,
///     @return The current add-on category.
///     <p>
///   }
///   \table_row3{   <b>`Container.Property(reponame)`</b>,
///                  \anchor Container_Property_reponame
///                  _string_,
///     @return The current add-on repository name.
///     <p>
///   }
///   \table_row3{   <b>`Container.Content`</b>,
///                  \anchor Container_Content
///                  _string_,
///     @return The content of the current container.
///     <p><hr>
///     @skinning_v16 **[New Infolabel]** \link Container_Content `Container.Content`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Container(id).ListItem(offset).Property`</b>,
///                  \anchor Container_ListItem_property
///                  _string_,
///     @return the property of the ListItem with a given offset.
///     @param offset - The offset for the listitem.
///     @note `Property` has to be replaced with `Label`\, `Label2`\, `Icon` etc.
///     @note **Example:** `Container(50).Listitem(2).Label `
///     <p>
///   }
///   \table_row3{   <b>`Container(id).ListItemNoWrap(offset).Property`</b>,
///                  \anchor Container_ListItemNoWrap
///                  _string_,
///     @return the same as \link Container_ListItem_property `Container(id).ListItem(offset).Property` \endlink
///     but it won't wrap.
///     @param offset - The offset for the listitem.
///     @note That means if the last item of a list is focused\, `ListItemNoWrap(1)` 
///     will be empty while `ListItem(1)` will return the first item of the list. 
///     `Property` has to be replaced with `Label`\, `Label2`\, `Icon` etc.
///     @note **Example:** `Container(50).ListitemNoWrap(1).Plot`
///     <p>
///   }
///   \table_row3{   <b>`Container(id).ListItemPosition(x).[infolabel]`</b>,
///                  \anchor Container_ListItemPosition
///                  _string_,
///     @return The infolabel for an item in a Container.
///     @param x - the position in the container relative to the cursor position.
///     @note **Example:** `Container(50).ListItemPosition(4).Genre`
///     <p>
///   }
///   \table_row3{   <b>`Container(id).ListItemAbsolute(x).[infolabel]`</b>,
///                  \anchor Container_ListItemAbsolute
///                  _string_,
///     @return The infolabel for an item in a Container.
///     @param x - the absolute position in the container.
///     @note **Example:** `Container(50).ListItemAbsolute(4).Genre`
///     <p><hr>
///     @skinning_v16 **[New Infolabel]** \link Container_ListItemAbsolute `Container(id).ListItemAbsolute(x).[infolabel]`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Container.Content(parameter)`</b>,
///                  \anchor Container_Content_parameter
///                  _string_,
///     @return **True** if the current container you are in contains the following:
///       - <b>files</b> 
///       - <b>songs</b> 
///       - <b>artists</b>
///       - <b>albums</b> 
///       - <b>movies</b>
///       - <b>tvshows</b>
///       - <b>seasons</b>
///       - <b>episodes</b> 
///       - <b>musicvideos</b> 
///       - <b>genres</b>
///       - <b>years</b>
///       - <b>actors</b>
///       - <b>playlists</b>
///       - <b>plugins</b>
///       - <b>studios</b>
///       - <b>directors</b>
///       - <b>sets</b>
///       - <b>tags</b>
///     @note These currently only work in the Video and Music
///     Library or unless a Plugin has set the value) also available are
///     Addons true when a list of add-ons is shown LiveTV true when a
///     htsp (tvheadend) directory is shown
///     <p>
///   }
///   \table_row3{   <b>`Container.Art(type)`</b>,
///                  \anchor Container_Art
///                  _string_,
///     @return The path to the art image file for the given type of the current container.
///     @param type - the art type to request.
///     @todo List of all art types
///     <p><hr>
///     @skinning_v16 **[Infolabel Updated]** \link Container_Art `Container.Art(type)`\endlink
///     <b>set.fanart</b> as possible type value.
///     @skinning_v15 **[New Infolabel]** \link Container_Art `Container.Art(type)`\endlink
///     <p>
///   }
///
const infomap container_str[]  = {{ "property",         CONTAINER_PROPERTY },
                                  { "content",          CONTAINER_CONTENT },
                                  { "art",              CONTAINER_ART }};

/// \page modules__infolabels_boolean_conditions
///   \table_row3{   <b>`Container.SortDirection(direction)`</b>,
///                  \anchor Container_SortDirection
///                  _boolean_,
///     @return **True** if the sort direction of a container equals direction.
///     @param direction - The direction to check. It can be:
///       - <b>ascending</b>
///       - <b>descending</b>
///     <p>
///   }
/// \table_end
///
/// -----------------------------------------------------------------------------

/// \page modules__infolabels_boolean_conditions
/// \subsection modules__infolabels_boolean_conditions_ListItem ListItem
/// \table_start
///   \table_h3{ Labels, Type, Description }
///   \table_row3{   <b>`ListItem.Thumb`</b>,
///                  \anchor ListItem_Thumb
///                  _string_,
///     @return The thumbnail (if it exists) of the currently selected item
///     in a list or thumb control.
///     @deprecated but still available\, returns
///     the same as \ref ListItem_Art_Type "ListItem.Art(thumb)"
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Icon`</b>,
///                  \anchor ListItem_Icon
///                  _string_,
///     @return The thumbnail (if it exists) of the currently selected item in a list or thumb control. 
///     @note If no thumbnail image exists\, it will show the icon.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.ActualIcon`</b>,
///                  \anchor ListItem_ActualIcon
///                  _string_,
///     @return The icon of the currently selected item in a list or thumb control.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Overlay`</b>,
///                  \anchor ListItem_Overlay
///                  _string_,
///     @return The overlay icon status of the currently selected item in a list or thumb control.
///       - compressed file -- OverlayRAR.png
///       - watched -- OverlayWatched.png
///       - unwatched -- OverlayUnwatched.png
///       - locked -- OverlayLocked.png
///     <p>
///   }
///   \table_row3{   <b>`ListItem.IsFolder`</b>,
///                  \anchor ListItem_IsFolder
///                  _boolean_,
///     @return **True** if the current ListItem is a folder.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.IsPlaying`</b>,
///                  \anchor ListItem_IsPlaying
///                  _boolean_,
///     @return **True** if the current ListItem.* info labels and images are
///     currently Playing media.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.IsResumable`</b>,
///                  \anchor ListItem_IsResumable
///                  _boolean_,
///     @return **True** when the current ListItem has been partially played.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.IsCollection`</b>,
///                  \anchor ListItem_IsCollection
///                  _boolean_,
///     @return **True** when the current ListItem is a movie set.
///     <p><hr>
///     @skinning_v15 **[New Boolean Condition]** \link ListItem_IsCollection `ListItem.IsCollection`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.IsSelected`</b>,
///                  \anchor ListItem_IsSelected
///                  _boolean_,
///     @return **True** if the current ListItem is selected (f.e. currently playing
///     in playlist window).
///     <p>
///   }
///   \table_row3{   <b>`ListItem.HasEpg`</b>,
///                  \anchor ListItem_HasEpg
///                  _boolean_,
///     @return **True** when the selected programme has epg info (PVR).
///     <p>
///   }
///   \table_row3{   <b>`ListItem.HasTimer`</b>,
///                  \anchor ListItem_HasTimer
///                  _boolean_,
///     @return **True** when a recording timer has been set for the selected
///     programme (PVR).
///     <p>
///   }
///   \table_row3{   <b>`ListItem.IsRecording`</b>,
///                  \anchor ListItem_IsRecording
///                  _boolean_,
///     @return **True** when the selected programme is being recorded (PVR).
///     <p>
///   }
///   \table_row3{   <b>`ListItem.IsPlayable`</b>,
///                  \anchor ListItem_IsPlayable
///                  _boolean_,
///     @return **True** when the selected programme can be played (PVR)
///     <p><hr>
///     @skinning_v19 **[New Boolean Condition]** \link ListItem_IsPlayable `ListItem.IsPlayable`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.HasArchive`</b>,
///                  \anchor ListItem_HasArchive
///                  _boolean_,
///     @return **True** when the selected channel has a server-side back buffer (PVR)
///     <p><hr>
///     @skinning_v19 **[New Boolean Condition]** \link ListItem_HasArchive `ListItem.HasArchive`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.IsEncrypted`</b>,
///                  \anchor ListItem_IsEncrypted
///                  _boolean_,
///     @return **True** when the selected programme is encrypted (PVR).
///     <p>
///   }
///   \table_row3{   <b>`ListItem.IsStereoscopic`</b>,
///                  \anchor ListItem_IsStereoscopic
///                  _boolean_,
///     @return **True** when the selected video is a 3D (stereoscopic) video.
///     <p><hr>
///     @skinning_v13 **[New Boolean Condition]** \link ListItem_IsStereoscopic `ListItem.IsStereoscopic`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Property(IsSpecial)`</b>,
///                  \anchor ListItem_Property_IsSpecial
///                  _boolean_,
///     @return **True** if the current Season/Episode is a Special.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Property(DateLabel)`</b>,
///                  \anchor ListItem_Property_DateLabel
///                  _boolean_,
///     @return **True** if the item is a date label\, returns false if the item is a time label.
///     @note Can be used in the rulerlayout of the epggrid control.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Property(Addon.IsEnabled)`</b>,
///                  \anchor ListItem_Property_AddonIsEnabled
///                  _boolean_,
///     @return **True** when the selected addon is enabled (for use in the addon
///     info dialog only).
///     <p><hr>
///     @skinning_v17 **[Boolean Condition Updated]** \link ListItem_Property_AddonIsEnabled `ListItem.Property(Addon.IsEnabled)`\endlink
///     replaces `ListItem.Property(Addon.Enabled)`.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Property(Addon.IsInstalled)`</b>,
///                  \anchor ListItem_Property_AddonIsInstalled
///                  _boolean_,
///     @return **True** when the selected addon is installed (for use in the addon
///     info dialog only).
///     <p><hr>
///     @skinning_v17 **[Boolean Condition Updated]** \link ListItem_Property_AddonIsInstalled `ListItem.Property(Addon.IsInstalled)`\endlink
///     replaces `ListItem.Property(Addon.Installed)`.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Property(Addon.HasUpdate)`</b>,
///                  \anchor ListItem_Property_AddonHasUpdate
///                  _boolean_,
///     @return **True** when there's an update available for the selected addon.
///     <p><hr>
///     @skinning_v17 **[Boolean Condition Updated]** \link ListItem_Property_AddonHasUpdate `ListItem.Property(Addon.HasUpdate)`\endlink
///     replaces `ListItem.Property(Addon.UpdateAvail)`.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Label`</b>,
///                  \anchor ListItem_Label
///                  _string_,
///     @return The left label of the currently selected item in a container.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Label2`</b>,
///                  \anchor ListItem_Label2
///                  _string_,
///     @return The right label of the currently selected item in a container.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Title`</b>,
///                  \anchor ListItem_Title
///                  _string_,
///     @return The title of the currently selected song\, movie\, game in a container.
///     <p><hr>
///     @skinning_v18 **[Infolabel Updated]** \link ListItem_Title `ListItem.Title`\endlink extended
///     to support games
///     <p>
///   }
///   \table_row3{   <b>`ListItem.OriginalTitle`</b>,
///                  \anchor ListItem_OriginalTitle
///                  _string_,
///     @return The original title of the currently selected movie in a container.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.SortLetter`</b>,
///                  \anchor ListItem_SortLetter
///                  _string_,
///     @return The first letter of the current file in a container.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.TrackNumber`</b>,
///                  \anchor ListItem_TrackNumber
///                  _string_,
///     @return The track number of the currently selected song in a container.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Artist`</b>,
///                  \anchor ListItem_Artist
///                  _string_,
///     @return The artist of the currently selected song in a container.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.AlbumArtist`</b>,
///                  \anchor ListItem_AlbumArtist
///                  _string_,
///     @return The artist of the currently selected album in a list.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Property(Artist_Sortname)`</b>,
///                  \anchor ListItem_Property_Artist_Sortname
///                  _string_,
///     @return The sortname of the currently selected Artist.
///     <p><hr>
///     @skinning_v18 **[New Infolabel]** \link ListItem_Property_Artist_Sortname `ListItem.Property(Artist_Sortname)`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Property(Artist_Type)`</b>,
///                  \anchor ListItem_Property_Artist_Type
///                  _string_,
///     @return The type of the currently selected Artist - person\, group\, orchestra\, choir etc.
///     <p><hr>
///     @skinning_v18 **[New Infolabel]** \link ListItem_Property_Artist_Type `ListItem.Property(Artist_Type)`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Property(Artist_Gender)`</b>,
///                  \anchor ListItem_Property_Artist_Gender
///                  _string_,
///     @return The Gender of the currently selected Artist - male\, female\, other.
///     <p><hr>
///     @skinning_v18 **[New Infolabel]** \link ListItem_Property_Artist_Gender `ListItem.Property(Artist_Gender)`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Property(Artist_Disambiguation)`</b>,
///                  \anchor ListItem_Property_Artist_Disambiguation
///                  _string_,
///     @return A Brief description of the currently selected Artist that differentiates them
///     from others with the same name.
///     <p><hr>
///     @skinning_v18 **[New Infolabel]** \link ListItem_Property_Artist_Disambiguation `ListItem.Property(Artist_Disambiguation)`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Property(Artist_Born)`</b>,
///                  \anchor ListItem_Property_Artist_Born
///                  _string_,
///     @return The date of Birth of the currently selected Artist.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Property(Artist_Died)`</b>,
///                  \anchor ListItem_Property_Artist_Died
///                  _string_,
///     @return The date of Death of the currently selected Artist.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Property(Artist_Formed)`</b>,
///                  \anchor ListItem_Property_Artist_Formed
///                  _string_,
///     @return The formation date of the currently selected Band.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Property(Artist_Disbanded)`</b>,
///                  \anchor ListItem_Property_Artist_Disbanded
///                  _string_,
///     @return The disbanding date of the currently selected Band.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Property(Artist_YearsActive)`</b>,
///                  \anchor ListItem_Property_Artist_YearsActive
///                  _string_,
///     @return The years the currently selected artist has been active.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Property(Artist_Instrument)`</b>,
///                  \anchor ListItem_Property_Artist_Instrument
///                  _string_,
///     @return The instruments played by the currently selected artist.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Property(Artist_Description)`</b>,
///                  \anchor ListItem_Property_Artist_Description
///                  _string_,
///     @return A biography of the currently selected artist.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Property(Artist_Mood)`</b>,
///                  \anchor ListItem_Property_Artist_Mood
///                  _string_,
///     @return The moods of the currently selected artist.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Property(Artist_Style)`</b>,
///                  \anchor ListItem_Property_Artist_Style
///                  _string_,
///     @return The styles of the currently selected artist.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Property(Artist_Genre)`</b>,
///                  \anchor ListItem_Property_Artist_Genre
///                  _string_,
///     @return The genre of the currently selected artist.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Album`</b>,
///                  \anchor ListItem_Album
///                  _string_,
///     @return The album of the currently selected song in a container.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Property(Album_Mood)`</b>,
///                  \anchor ListItem_Property_Album_Mood
///                  _string_,
///     @return The moods of the currently selected Album.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Property(Album_Style)`</b>,
///                  \anchor ListItem_Property_Album_Style
///                  _string_,
///     @return The styles of the currently selected Album.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Property(Album_Theme)`</b>,
///                  \anchor ListItem_Property_Album_Theme
///                  _string_,
///     @return The themes of the currently selected Album.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Property(Album_Type)`</b>,
///                  \anchor ListItem_Property_Album_Type
///                  _string_,
///     @return The Album Type (e.g. compilation\, enhanced\, explicit lyrics) of
///     the currently selected Album.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Property(Album_Label)`</b>,
///                  \anchor ListItem_Property_Album_Label
///                  _string_,
///     @return The record label of the currently selected Album.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Property(Album_Description)`</b>,
///                  \anchor ListItem_Property_Album_Description
///                  _string_,
///     @return A review of the currently selected Album.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.DiscNumber`</b>,
///                  \anchor ListItem_DiscNumber
///                  _string_,
///     @return The disc number of the currently selected song in a container.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Year`</b>,
///                  \anchor ListItem_Year
///                  _string_,
///     @return The year of the currently selected song\, album\, movie\, game  in a
///     container.
///     <p><hr>
///     @skinning_v18 **[Infolabel Updated]** \link ListItem_Title `ListItem.Title`\endlink extended
///     to support games
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Premiered`</b>,
///                  \anchor ListItem_Premiered
///                  _string_,
///     @return The release/aired date of the currently selected episode\, show\,
///     movie or EPG item in a container.
///     <p><hr>
///     @skinning_v15 **[Infolabel Updated]** \link ListItem_Premiered `ListItem.Premiered`\endlink
///     now also available for EPG items.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Genre`</b>,
///                  \anchor ListItem_Genre
///                  _string_,
///     @return The genre of the currently selected song\, album or movie in a
///     container.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Contributors`</b>,
///                  \anchor ListItem_Contributors
///                  _string_,
///     @return The list of all people who've contributed to the selected song.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link ListItem_Contributors `ListItem.Contributors`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.ContributorAndRole`</b>,
///                  \anchor ListItem_ContributorAndRole
///                  _string_,
///     @return The list of all people and their role who've contributed to the selected song.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link ListItem_ContributorAndRole `ListItem.ContributorAndRole`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Director`</b>,
///                  \anchor ListItem_Director
///                  _string_,
///     @return The director of the currently selected movie in a container.
///     <p><hr>
///     @skinning_v15 **[Infolabel Updated]** \link ListItem_Director `ListItem.Director`\endlink
///     also supports EPG.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Country`</b>,
///                  \anchor ListItem_Country
///                  _string_,
///     @return The production country of the currently selected movie in a
///     container.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Episode`</b>,
///                  \anchor ListItem_Episode
///                  _string_,
///     @return The episode number value for the currently selected episode. It
///     also returns the number of total\, watched or unwatched episodes for the
///     currently selected tvshow or season\, based on the the current watched
///     filter.
///     <p><hr>
///     @skinning_v15 **[Infolabel Updated]** \link ListItem_Episode `ListItem.Episode`\endlink
///     also supports EPG.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Season`</b>,
///                  \anchor ListItem_Season
///                  _string_,
///     @return The season value for the currently selected tvshow.
///     <p><hr>
///     @skinning_v15 **[Infolabel Updated]** \link ListItem_Season `ListItem.Season`\endlink
///     also supports EPG.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.TVShowTitle`</b>,
///                  \anchor ListItem_TVShowTitle
///                  _string_,
///     @return The name value for the currently selected tvshow in the season and
///     episode depth of the video library.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Property(TotalSeasons)`</b>,
///                  \anchor ListItem_Property_TotalSeasons
///                  _string_,
///     @return The total number of seasons for the currently selected tvshow.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Property(TotalEpisodes)`</b>,
///                  \anchor ListItem_Property_TotalEpisodes
///                  _string_,
///     @return the total number of episodes for the currently selected tvshow or
///     season.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Property(WatchedEpisodes)`</b>,
///                  \anchor ListItem_Property_WatchedEpisodes
///                  _string_,
///     @return The number of watched episodes for the currently selected tvshow
///     or season.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Property(UnWatchedEpisodes)`</b>,
///                  \anchor ListItem_Property_UnWatchedEpisodes
///                  _string_,
///     @return The number of unwatched episodes for the currently selected tvshow
///     or season.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Property(NumEpisodes)`</b>,
///                  \anchor ListItem_Property_NumEpisodes
///                  _string_,
///     @return The number of total\, watched or unwatched episodes for the
///     currently selected tvshow or season\, based on the the current watched filter.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.PictureAperture`</b>,
///                  \anchor ListItem_PictureAperture
///                  _string_,
///     @return The F-stop used to take the selected picture.
///     @note This is the value of the EXIF FNumber tag (hex code 0x829D).
///     <p>
///   }
///   \table_row3{   <b>`ListItem.PictureAuthor`</b>,
///                  \anchor ListItem_PictureAuthor
///                  _string_,
///     @return The name of the person involved in writing about the selected picture.
///     @note This is the value of the IPTC Writer tag (hex code 0x7A).
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link ListItem_PictureAuthor `ListItem.PictureAuthor`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.PictureByline`</b>,
///                  \anchor ListItem_PictureByline
///                  _string_,
///     @return The name of the person who created the selected picture.
///     @note This is the value of the IPTC Byline tag (hex code 0x50).
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link ListItem_PictureByline `ListItem.PictureByline`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.PictureBylineTitle`</b>,
///                  \anchor ListItem_PictureBylineTitle
///                  _string_,
///     @return The title of the person who created the selected picture.
///     @note This is the value of the IPTC BylineTitle tag (hex code 0x55).
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link ListItem_PictureBylineTitle `ListItem.PictureBylineTitle`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.PictureCamMake`</b>,
///                  \anchor ListItem_PictureCamMake
///                  _string_,
///     @return The manufacturer of the camera used to take the selected picture.
///     @note This is the value of the EXIF Make tag (hex code 0x010F).
///     <p>
///   }
///   \table_row3{   <b>`ListItem.PictureCamModel`</b>,
///                  \anchor ListItem_PictureCamModel
///                  _string_,
///     @return The manufacturer's model name or number of the camera used to take
///     the selected picture.
///     @note This is the value of the EXIF Model tag (hex code 0x0110).
///     <p>
///   }
///   \table_row3{   <b>`ListItem.PictureCaption`</b>,
///                  \anchor ListItem_PictureCaption
///                  _string_,
///     @return A description of the selected picture.
///     @note This is the value of the IPTC Caption tag (hex code 0x78).
///     <p>
///   }
///   \table_row3{   <b>`ListItem.PictureCategory`</b>,
///                  \anchor ListItem_PictureCategory
///                  _string_,
///     @return The subject of the selected picture as a category code.
///     @note This is the value of the IPTC Category tag (hex code 0x0F).
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link ListItem_PictureCategory `ListItem.PictureCategory`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.PictureCCDWidth`</b>,
///                  \anchor ListItem_PictureCCDWidth
///                  _string_,
///     @return The width of the CCD in the camera used to take the selected
///     picture.
///     @note This is calculated from three EXIF tags (0xA002 * 0xA210 / 0xA20e).
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link ListItem_PictureCCDWidth `ListItem.PictureCCDWidth`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.PictureCity`</b>,
///                  \anchor ListItem_PictureCity
///                  _string_,
///     @return The city where the selected picture was taken.
///     @note This is the value of the IPTC City tag (hex code 0x5A).
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link ListItem_PictureCity `ListItem.PictureCity`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.PictureColour`</b>,
///                  \anchor ListItem_PictureColour
///                  _string_,
///     @return Whether the selected picture is "Colour" or "Black and White".
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link ListItem_PictureColour `ListItem.PictureColour`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.PictureComment`</b>,
///                  \anchor ListItem_PictureComment
///                  _string_,
///     @return A description of the selected picture.
///     @note This is the value of the
///     EXIF User Comment tag (hex code 0x9286). This is the same value as
///     \ref Slideshow_SlideComment "Slideshow.SlideComment".
///     <p>
///   }
///   \table_row3{   <b>`ListItem.PictureCopyrightNotice`</b>,
///                  \anchor ListItem_PictureCopyrightNotice
///                  _string_,
///     @return The copyright notice of the selected picture.
///     @note This is the value of the IPTC Copyright tag (hex code 0x74).
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link ListItem_PictureCopyrightNotice `ListItem.PictureCopyrightNotice`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.PictureCountry`</b>,
///                  \anchor ListItem_PictureCountry
///                  _string_,
///     @return The full name of the country where the selected picture was taken.
///     @note This is the value of the IPTC CountryName tag (hex code 0x65).
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link ListItem_PictureCountry `ListItem.PictureCountry`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.PictureCountryCode`</b>,
///                  \anchor ListItem_PictureCountryCode
///                  _string_,
///     @return The country code of the country where the selected picture was
///     taken.
///     @note This is the value of the IPTC CountryCode tag (hex code 0x64).
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link ListItem_PictureCountryCode `ListItem.PictureCountryCode`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.PictureCredit`</b>,
///                  \anchor ListItem_PictureCredit
///                  _string_,
///     @return Who provided the selected picture.
///     @note This is the value of the IPTC Credit tag (hex code 0x6E).
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link ListItem_PictureCredit `ListItem.PictureCredit`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.PictureDate`</b>,
///                  \anchor ListItem_PictureDate
///                  _string_,
///     @return The localized date of the selected picture. The short form of the
///     date is used.
///     @note The value of the EXIF DateTimeOriginal tag (hex code 0x9003)
///     is preferred. If the DateTimeOriginal tag is not found\, the value of
///     DateTimeDigitized (hex code 0x9004) or of DateTime (hex code 0x0132) might
///     be used.
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link ListItem_PictureDate `ListItem.PictureDate`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.PictureDatetime`</b>,
///                  \anchor ListItem_PictureDatetime
///                  _string_,
///     @return The date/timestamp of the selected picture. The localized short form
///     of the date and time is used.
///     @note The value of the EXIF DateTimeOriginal tag (hex code 0x9003) is preferred.
///     If the DateTimeOriginal tag is not found\, the value of DateTimeDigitized
///     (hex code 0x9004) or of DateTime (hex code 0x0132) might be used.
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link ListItem_PictureDatetime `ListItem.PictureDatetime`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.PictureDesc`</b>,
///                  \anchor ListItem_PictureDesc
///                  _string_,
///     @return A short description of the selected picture. The SlideComment\,
///     EXIFComment\, or Caption values might contain a longer description.
///     @note This is the value of the EXIF ImageDescription tag (hex code 0x010E).
///     <p>
///   }
///   \table_row3{   <b>`ListItem.PictureDigitalZoom`</b>,
///                  \anchor ListItem_PictureDigitalZoom
///                  _string_,
///     @return The digital zoom ratio when the selected picture was taken.
///     @note This is the value of the EXIF DigitalZoomRatio tag (hex code 0xA404).
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link ListItem_PictureDigitalZoom `ListItem.PictureDigitalZoom`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.PictureExpMode`</b>,
///                  \anchor ListItem_PictureExpMode
///                  _string_,
///     @return The exposure mode of the selected picture.
///     The possible values are:
///       - <b>"Automatic"</b>
///       - <b>"Manual"</b>
///       - <b>"Auto bracketing"</b>
///     @note This is the value of the EXIF ExposureMode tag (hex code 0xA402).
///     <p>
///   }
///   \table_row3{   <b>`ListItem.PictureExposure`</b>,
///                  \anchor ListItem_PictureExposure
///                  _string_,
///     @return The class of the program used by the camera to set exposure when
///     the selected picture was taken. Values include:
///      -  <b>"Manual"</b>
///      -  <b>"Program (Auto)"</b>
///      -  <b>"Aperture priority (Semi-Auto)"</b>
///      -  <b>"Shutter priority (semi-auto)"</b>
///      -  etc
///     @note This is the value of the EXIF ExposureProgram tag (hex code 0x8822).
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link ListItem_PictureExposure `ListItem.PictureExposure`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.PictureExposureBias`</b>,
///                  \anchor ListItem_PictureExposureBias
///                  _string_,
///     @return The exposure bias of the selected picture.
///     Typically this is a number between -99.99 and 99.99.
///     @note This is the value of the EXIF ExposureBiasValue tag (hex code 0x9204).
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link ListItem_PictureExposureBias `ListItem.PictureExposureBias`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.PictureExpTime`</b>,
///                  \anchor ListItem_PictureExpTime
///                  _string_,
///     @return The exposure time of the selected picture\, in seconds. 
///     @note This is the value of the EXIF ExposureTime tag (hex code 0x829A).
///     If the ExposureTime tag is not found\, the ShutterSpeedValue tag (hex code 0x9201) 
///     might be used.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.PictureFlashUsed`</b>,
///                  \anchor ListItem_PictureFlashUsed
///                  _string_,
///     @return The status of flash when the selected picture was taken. The value
///     will be either "Yes" or "No"\, and might include additional information.
///     @note This is the value of the EXIF Flash tag (hex code 0x9209).
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link ListItem_PictureFlashUsed `ListItem.PictureFlashUsed`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.PictureFocalLen`</b>,
///                  \anchor ListItem_PictureFocalLen
///                  _string_,
///     @return The lens focal length of the selected picture.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.PictureFocusDist`</b>,
///                  \anchor ListItem_PictureFocusDist
///                  _string_,
///     @return The focal length of the lens\, in mm. 
///     @note This is the value of the EXIF FocalLength tag (hex code 0x920A).
///   }
///   \table_row3{   <b>`ListItem.PictureGPSLat`</b>,
///                  \anchor ListItem_PictureGPSLat
///                  _string_,
///     @return The latitude where the selected picture was taken (degrees\,
///     minutes\, seconds North or South).
///     @note This is the value of the EXIF GPSInfo.GPSLatitude and GPSInfo.GPSLatitudeRef tags.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.PictureGPSLon`</b>,
///                  \anchor ListItem_PictureGPSLon
///                  _string_,
///     @return The longitude where the selected picture was taken (degrees\,
///     minutes\, seconds East or West). 
///     @note This is the value of the EXIF GPSInfo.GPSLongitude and GPSInfo.GPSLongitudeRef tags.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.PictureGPSAlt`</b>,
///                  \anchor ListItem_PictureGPSAlt
///                  _string_,
///     @return The altitude in meters where the selected picture was taken.
///     @note This is the value of the EXIF GPSInfo.GPSAltitude tag.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.PictureHeadline`</b>,
///                  \anchor ListItem_PictureHeadline
///                  _string_,
///     @return A synopsis of the contents of the selected picture.
///     @note This is the value of the IPTC Headline tag (hex code 0x69).
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link ListItem_PictureHeadline `ListItem.PictureHeadline`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.PictureImageType`</b>,
///                  \anchor ListItem_PictureImageType
///                  _string_,
///     @return The color components of the selected picture.
///     @note This is the value of the IPTC ImageType tag (hex code 0x82).
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link ListItem_PictureImageType `ListItem.PictureImageType`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.PictureIPTCDate`</b>,
///                  \anchor ListItem_PictureIPTCDate
///                  _string_,
///     @return The date when the intellectual content of the selected picture was
///     created\, rather than when the picture was created.
///     @note This is the value of the IPTC DateCreated tag (hex code 0x37).
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link ListItem_PictureIPTCDate `ListItem.PictureIPTCDate`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.PictureIPTCTime`</b>,
///                  \anchor ListItem_PictureIPTCTime
///                  _string_,
///     @return The time when the intellectual content of the selected picture was
///     created\, rather than when the picture was created.
///     @note This is the value of the IPTC TimeCreated tag (hex code 0x3C).
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link ListItem_PictureIPTCTime `ListItem.PictureIPTCTime`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.PictureISO`</b>,
///                  \anchor ListItem_PictureISO
///                  _string_,
///     @return The ISO speed of the camera when the selected picture was taken.
///     @note This is the value of the EXIF ISOSpeedRatings tag (hex code 0x8827).
///     <p>
///   }
///   \table_row3{   <b>`ListItem.PictureKeywords`</b>,
///                  \anchor ListItem_PictureKeywords
///                  _string_,
///     @return The keywords assigned to the selected picture.
///     @note This is the value of the IPTC Keywords tag (hex code 0x19).
///     <p>
///   }
///   \table_row3{   <b>`ListItem.PictureLightSource`</b>,
///                  \anchor ListItem_PictureLightSource
///                  _string_,
///     @return The kind of light source when the picture was taken. Possible
///     values include:
///       - <b>"Daylight"</b>
///       - <b>"Fluorescent"</b>
///       - <b>"Incandescent</b>
///       - etc
///     @note This is the value of the EXIF LightSource tag (hex code 0x9208).
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link ListItem_PictureLightSource `ListItem.PictureLightSource`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.PictureLongDate`</b>,
///                  \anchor ListItem_PictureLongDate
///                  _string_,
///     @return Only the localized date of the selected picture. The long form of
///     the date is used. 
///     @note The value of the EXIF DateTimeOriginal tag (hex code
///     0x9003) is preferred. If the DateTimeOriginal tag is not found\, the
///     value of DateTimeDigitized (hex code 0x9004) or of DateTime (hex code
///     0x0132) might be used.
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link ListItem_PictureLongDate `ListItem.PictureLongDate`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.PictureLongDatetime`</b>,
///                  \anchor ListItem_PictureLongDatetime
///                  _string_,
///     @return The date/timestamp of the selected picture. The localized long
///     form of the date and time is used. 
///     @note The value of the EXIF DateTimeOriginal
///     tag (hex code 0x9003) is preferred. if the DateTimeOriginal tag is not
///     found\, the value of DateTimeDigitized (hex code 0x9004) or of DateTime
///     (hex code 0x0132) might be used.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.PictureMeteringMode`</b>,
///                  \anchor ListItem_PictureMeteringMode
///                  _string_,
///     @return The metering mode used when the selected picture was taken. The
///     possible values are:
///      - <b>"Center weight"</b>
///      - <b>"Spot"</b>
///      - <b>"Matrix"</b> 
///     @note This is the value of the EXIF MeteringMode tag (hex code 0x9207).
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link ListItem_PictureMeteringMode `ListItem.PictureMeteringMode`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.PictureObjectName`</b>,
///                  \anchor ListItem_PictureObjectName
///                  _string_,
///     @return A shorthand reference for the selected picture.
///     @note This is the value of the IPTC ObjectName tag (hex code 0x05).
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link ListItem_PictureObjectName `ListItem.PictureObjectName`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.PictureOrientation`</b>,
///                  \anchor ListItem_PictureOrientation
///                  _string_,
///     @return The orientation of the selected picture. Possible values are:
///       - <b>"Top Left"</b> 
///       - <b>"Top Right"</b>
///       - <b>"Left Top"</b>
///       - <b>"Right Bottom"</b>
///       - etc 
///     @note This is the value of the EXIF Orientation tag (hex code 0x0112).
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link ListItem_PictureOrientation `ListItem.PictureOrientation`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.PicturePath`</b>,
///                  \anchor ListItem_PicturePath
///                  _string_,
///     @return The filename and path of the selected picture.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.PictureProcess`</b>,
///                  \anchor ListItem_PictureProcess
///                  _string_,
///     @return The process used to compress the selected picture.
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link ListItem_PictureProcess `ListItem.PictureProcess`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.PictureReferenceService`</b>,
///                  \anchor ListItem_PictureReferenceService
///                  _string_,
///     @return The Service Identifier of a prior envelope to which the selected
///     picture refers.
///     @note This is the value of the IPTC ReferenceService tag (hex code 0x2D).
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link ListItem_PictureReferenceService `ListItem.PictureReferenceService`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.PictureResolution`</b>,
///                  \anchor ListItem_PictureResolution
///                  _string_,
///     @return The dimensions of the selected picture.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.PictureSource`</b>,
///                  \anchor ListItem_PictureSource
///                  _string_,
///     @return The original owner of the selected picture.
///     @note This is the value of the IPTC Source tag (hex code 0x73).
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link ListItem_PictureSource `ListItem.PictureSource`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.PictureSpecialInstructions`</b>,
///                  \anchor ListItem_PictureSpecialInstructions
///                  _string_,
///     @return Other editorial instructions concerning the use of the selected
///     picture.
///     @note This is the value of the IPTC SpecialInstructions tag (hex code 0x28).
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link ListItem_PictureSpecialInstructions `ListItem.PictureSpecialInstructions`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.PictureState`</b>,
///                  \anchor ListItem_PictureState
///                  _string_,
///     @return The State/Province where the selected picture was taken.
///     @note This is the value of the IPTC ProvinceState tag (hex code 0x5F).
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link ListItem_PictureState `ListItem.PictureState`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.PictureSublocation`</b>,
///                  \anchor ListItem_PictureSublocation
///                  _string_,
///     @return The location within a city where the selected picture was taken -
///     might indicate the nearest landmark.
///     @note This is the value of the IPTC SubLocation tag (hex code 0x5C).
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link ListItem_PictureSublocation `ListItem.PictureSublocation`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.PictureSupplementalCategories`</b>,
///                  \anchor ListItem_PictureSupplementalCategories
///                  _string_,
///     @return A supplemental category codes to further refine the subject of the
///     selected picture.
///     @note This is the value of the IPTC SuppCategory tag (hex code 0x14).
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link ListItem_PictureSupplementalCategories `ListItem.PictureSupplementalCategories`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.PictureTransmissionReference`</b>,
///                  \anchor ListItem_PictureTransmissionReference
///                  _string_,
///     @return A code representing the location of original transmission of the
///     selected picture.
///     @note This is the value of the IPTC TransmissionReference tag (hex code 0x67).
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link ListItem_PictureTransmissionReference `ListItem.PictureTransmissionReference`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.PictureUrgency`</b>,
///                  \anchor ListItem_PictureUrgency
///                  _string_,
///     @return The urgency of the selected picture. Values are 1-9.
///     @note The "1" is most urgent. Some image management programs use urgency to indicate
///     picture rating\, where urgency "1" is 5 stars and urgency "5" is 1 star.
///     Urgencies 6-9 are not used for rating. This is the value of the IPTC
///     Urgency tag (hex code 0x0A).
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link ListItem_PictureUrgency `ListItem.PictureUrgency`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.PictureWhiteBalance`</b>,
///                  \anchor ListItem_PictureWhiteBalance
///                  _string_,
///     @return The white balance mode set when the selected picture was taken.
///     The possible values are:
///       - <b>"Manual"</b>
///       - <b>"Auto"</b>
///     @note This is the value of the EXIF WhiteBalance tag (hex code 0xA403).
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link ListItem_PictureWhiteBalance `ListItem.PictureWhiteBalance`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.FileName`</b>,
///                  \anchor ListItem_FileName
///                  _string_,
///     @return The filename of the currently selected song or movie in a container.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Path`</b>,
///                  \anchor ListItem_Path
///                  _string_,
///     @return The complete path of the currently selected song or movie in a
///     container.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.FolderName`</b>,
///                  \anchor ListItem_FolderName
///                  _string_,
///     @return The top most folder of the path of the currently selected song or
///     movie in a container.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.FolderPath`</b>,
///                  \anchor ListItem_FolderPath
///                  _string_,
///     @return The complete path of the currently selected song or movie in a
///     container (without user details).
///     <p>
///   }
///   \table_row3{   <b>`ListItem.FileNameAndPath`</b>,
///                  \anchor ListItem_FileNameAndPath
///                  _string_,
///     @return The full path with filename of the currently selected song or
///     movie in a container.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.FileExtension`</b>,
///                  \anchor ListItem_FileExtension
///                  _string_,
///     @return The file extension (without leading dot) of the currently selected
///     item in a container.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Date`</b>,
///                  \anchor ListItem_Date
///                  _string_,
///     @return The file date of the currently selected song or movie in a
///     container / Aired date of an episode / Day\, start time and end time of
///     current selected TV programme (PVR).
///     <p>
///   }
///   \table_row3{   <b>`ListItem.DateTime`</b>,
///                  \anchor ListItem_DateTime
///                  _string_,
///     @return The date and time a certain event happened (event log).
///     <p><hr>
///     @skinning_v16 **[New Infolabel]** \link ListItem_DateTime `ListItem.DateTime`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.DateAdded`</b>,
///                  \anchor ListItem_DateAdded
///                  _string_,
///     @return The date the currently selected item was added to the
///     library / Date and time of an event in the EventLog window.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Size`</b>,
///                  \anchor ListItem_Size
///                  _string_,
///     @return The file size of the currently selected song or movie in a
///     container.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Rating([name])`</b>,
///                  \anchor ListItem_Rating
///                  _string_,
///     @return The scraped rating of the currently selected item in a container (1-10). 
///     @param name - [opt] you can specify the name of the scraper to retrieve a specific rating\, 
///     for use in dialogvideoinfo.xml.
///     <p><hr>
///     @skinning_v18 **[Infolabel Updated]** \link ListItem_Rating `ListItem.Rating([name])`\endlink replaces
///     the old `ListItem.Ratings([name])` infolabel.
///     @skinning_v17 **[New Infolabel]** \link ListItem_Rating `ListItem.Ratings([name])`\endlink
///     @skinning_v17 **[Infolabel Updated]** \link ListItem_Rating `ListItem.Ratings`\endlink
///     for songs it's now the scraped rating.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Set`</b>,
///                  \anchor ListItem_Set
///                  _string_,
///     @return The name of the set the movie is part of.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link ListItem_Set `ListItem.Set`\endlink
///     <p>  
///   }
///   \table_row3{   <b>`ListItem.SetId`</b>,
///                  \anchor ListItem_SetId
///                  _string_,
///     @return The id of the set the movie is part of.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link ListItem_SetId `ListItem.SetId`\endlink
///     <p>  
///   }
///   \table_row3{   <b>`ListItem.Status`</b>,
///                  \anchor ListItem_Status
///                  _string_,
///     @return One of the following status:
///       - <b>"returning series"</b>
///       - <b>"in production"</b>
///       - <b>"planned"</b>
///       - <b>"cancelled"</b>
///       - <b>"ended"</b>
///     <p>
///     @note For use with tv shows.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link ListItem_Status `ListItem.Status`\endlink
///     <p>  
///   }
///   \table_row3{   <b>`ListItem.EndTimeResume`</b>,
///                  \anchor ListItem_EndTimeResume
///                  _string_,
///     @return Returns the time a video will end if you resume it\, instead of playing it from the beginning.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link ListItem_EndTimeResume `ListItem.EndTimeResume`\endlink
///     <p>  
///   }
///   \table_row3{   <b>`ListItem.UserRating`</b>,
///                  \anchor ListItem_UserRating
///                  _string_,
///     @return The user rating of the currently selected item in a container (1-10).
///     <p><hr>
///     @skinning_v17 **[Infolabel Updated]** \link ListItem_UserRating `ListItem.UserRating`\endlink
///     now available for albums/songs.
///     @skinning_v16 **[New Infolabel]** \link ListItem_UserRating `ListItem.UserRating`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Votes([name])`</b>,
///                  \anchor ListItem_Votes
///                  _string_,
///     @return The scraped votes of the currently selected movie in a container.
///     @param name - [opt] you can specify the name of the scraper to retrieve specific votes\,
///     for use in `dialogvideoinfo.xml`.
///     <p><hr>
///     @skinning_v17 **[Infolabel Updated]** \link ListItem_Votes `ListItem.Votes([name])`\endlink
///     add optional param <b>name</b> to specify the scrapper.
///     @skinning_v13 **[New Infolabel]** \link ListItem_Votes `ListItem.Votes`\endlink
///     <p>  
///   }
///   \table_row3{   <b>`ListItem.RatingAndVotes([name])`</b>,
///                  \anchor ListItem_RatingAndVotes
///                  _string_,
///     @return The scraped rating and votes of the currently selected movie in a
///     container (1-10).
///     @param name - [opt] you can specify the name of the scraper to retrieve specific votes\,
///     for use in `dialogvideoinfo.xml`.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link ListItem_RatingAndVotes `ListItem.RatingAndVotes([name])`\endlink
///     @skinning_v17 **[Infolabel Updated]** \link ListItem_RatingAndVotes `ListItem.RatingAndVotes`\endlink
///     now available for albums/songs.
///     <p>  
///   }
///   \table_row3{   <b>`ListItem.Mood`</b>,
///                  \anchor ListItem_Mood
///                  _string_,
///     @return The mood of the selected song.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link ListItem_Mood `ListItem.Mood`\endlink
///     <p>  
///   }
///   \table_row3{   <b>`ListItem.Mpaa`</b>,
///                  \anchor ListItem_Mpaa
///                  _string_,
///     @return The MPAA rating of the currently selected movie in a container.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.ProgramCount`</b>,
///                  \anchor ListItem_ProgramCount
///                  _string_,
///     @return The number of times an xbe has been run from "my programs".
///     @todo description might be outdated
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Duration`</b>,
///                  \anchor ListItem_Duration
///                  _string_,
///     @return The duration of the currently selected item in a container
///     in the format <b>hh:mm:ss</b>.
///     @note <b>hh:</b> will be omitted if hours value is zero.
///     <p><hr>
///     @skinning_v18 **[Infolabel Updated]** \link ListItem_Duration `ListItem.Duration`\endlink will
///     return <b>hh:mm:ss</b> instead of the duration in minutes.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Duration(format)`</b>,
///                  \anchor ListItem_Duration_format
///                  _string_,
///     @return The duration of the currently selected item in a container in
///     different formats.
///     @param format [opt] The format of the return time value.
///     See \ref TIME_FORMAT for the list of possible values.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.DBTYPE`</b>,
///                  \anchor ListItem_DBTYPE
///                  _string_,
///     @return The database type of the \ref ListItem_DBID "ListItem.DBID" for videos (movie\, set\,
///     genre\, actor\, tvshow\, season\, episode). It does not return any value
///     for the music library. 
///     @note Beware with season\, the "*all seasons" entry does
///     give a DBTYPE "season" and a DBID\, but you can't get the details of that
///     entry since it's a virtual entry in the Video Library.
///     <p><hr>
///     @skinning_v17 **[Infolabel Updated]** \link ListItem_DBTYPE `ListItem.DBTYPE`\endlink
///     now available in the music library.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.DBID`</b>,
///                  \anchor ListItem_DBID
///                  _string_,
///     @return The database id of the currently selected listitem in a container.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Appearances`</b>,
///                  \anchor ListItem_Appearances
///                  _string_,
///     @return The number of movies featuring the selected actor / directed by the selected director.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link ListItem_Appearances `ListItem.Appearances`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Cast`</b>,
///                  \anchor ListItem_Cast
///                  _string_,
///     @return A concatenated string of cast members of the currently selected
///     movie\, for use in dialogvideoinfo.xml.
///     <p><hr>
///     @skinning_v15 **[Infolabel Updated]** \link ListItem_Cast `ListItem.Cast`\endlink
///     also supports EPG.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.CastAndRole`</b>,
///                  \anchor ListItem_CastAndRole
///                  _string_,
///     @return A concatenated string of cast members and roles of the currently
///     selected movie\, for use in dialogvideoinfo.xml.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Studio`</b>,
///                  \anchor ListItem_Studio
///                  _string_,
///     @return The studio of current selected Music Video in a container.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Top250`</b>,
///                  \anchor ListItem_Top250
///                  _string_,
///     @return The IMDb top250 position of the currently selected listitem in a
///     container.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Trailer`</b>,
///                  \anchor ListItem_Trailer
///                  _string_,
///     @return The full trailer path with filename of the currently selected
///     movie in a container.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Writer`</b>,
///                  \anchor ListItem_Writer
///                  _string_,
///     @return The name of Writer of current Video in a container.
///     <p><hr>
///     @skinning_v15 **[Infolabel Updated]** \link ListItem_Writer `ListItem.Writer`\endlink
///     also supports EPG.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Tag`</b>,
///                  \anchor ListItem_Tag
///                  _string_,
///     @return The summary of current Video in a container.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link ListItem_Tag `ListItem.Tag`\endlink
///     <p>  
///   }
///   \table_row3{   <b>`ListItem.Tagline`</b>,
///                  \anchor ListItem_Tagline
///                  _string_,
///     @return A Small Summary of current Video in a container.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.PlotOutline`</b>,
///                  \anchor ListItem_PlotOutline
///                  _string_,
///     @return A small Summary of current Video in a container.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Plot`</b>,
///                  \anchor ListItem_Plot
///                  _string_,
///     @return The complete Text Summary of Video in a container.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.IMDBNumber`</b>,
///                  \anchor ListItem_IMDBNumber
///                  _string_,
///     @return The IMDb ID of the selected Video in a container.
///     <p><hr>
///     @skinning_v15 **[New Infolabel]** \link ListItem_IMDBNumber `ListItem.IMDBNumber`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.EpisodeName`</b>,
///                  \anchor ListItem_EpisodeName
///                  _string_,
///     @return The name of the episode if the selected EPG item is a TV Show (PVR).
///     <p><hr>
///     @skinning_v15 **[New Infolabel]** \link ListItem_EpisodeName `ListItem.EpisodeName`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.PercentPlayed`</b>,
///                  \anchor ListItem_PercentPlayed
///                  _string_,
///     @return The percentage value [0-100] of how far the selected video has been
///     played.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.LastPlayed`</b>,
///                  \anchor ListItem_LastPlayed
///                  _string_,
///     @return The last play date of Video in a container.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.PlayCount`</b>,
///                  \anchor ListItem_PlayCount
///                  _string_,
///     @return The playcount of Video in a container.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.ChannelName`</b>,
///                  \anchor ListItem_ChannelName
///                  _string_,
///     @return The name of current selected TV channel in a container.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.VideoCodec`</b>,
///                  \anchor ListItem_VideoCodec
///                  _string_,
///     @return The video codec of the currently selected video. Common values:
///      - <b>3iv2</b>
///      - <b>avc1</b>
///      - <b>div2</b>
///      - <b>div3</b>
///      - <b>divx</b>
///      - <b>divx 4</b>
///      - <b>dx50</b>
///      - <b>flv</b>
///      - <b>h264</b>
///      - <b>microsoft</b>
///      - <b>mp42</b>
///      - <b>mp43</b>
///      - <b>mp4v</b>
///      - <b>mpeg1video</b>
///      - <b>mpeg2video</b>
///      - <b>mpg4</b>
///      - <b>rv40</b>
///      - <b>svq1</b>
///      - <b>svq3</b>
///      - <b>theora</b>
///      - <b>vp6f</b>
///      - <b>wmv2</b>
///      - <b>wmv3</b>
///      - <b>wvc1</b>
///      - <b>xvid</b>
///      - etc
///     <p>
///   }
///   \table_row3{   <b>`ListItem.VideoResolution`</b>,
///                  \anchor ListItem_VideoResolution
///                  _string_,
///     @return The resolution of the currently selected video. Possible values:
///       - <b>480</b> 
///       - <b>576</b>
///       - <b>540</b>
///       - <b>720</b>
///       - <b>1080</b>
///       - <b>4K</b>
///       - <b>8K</b>
///     @note 540 usually means a widescreen
///     format (around 960x540) while 576 means PAL resolutions (normally
///     720x576)\, therefore 540 is actually better resolution than 576.
///     <p><hr>
///     @skinning_v18 **[Updated Infolabel]** \link ListItem_VideoResolution ListItem.VideoResolution\endlink
///     added <b>8K</b> as a possible value.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.VideoAspect`</b>,
///                  \anchor ListItem_VideoAspect
///                  _string_,
///     @return The aspect ratio of the currently selected video. Possible values:
///      - <b>1.33</b>
///      - <b>1.37</b>
///      - <b>1.66</b>
///      - <b>1.78</b>
///      - <b>1.85</b>
///      - <b>2.20</b>
///      - <b>2.35</b>
///      - <b>2.40</b>
///      - <b>2.55</b>
///      - <b>2.76</b>
///     <p>
///   }
///   \table_row3{   <b>`ListItem.AudioCodec`</b>,
///                  \anchor ListItem_AudioCodec
///                  _string_,
///     @return The audio codec of the currently selected video. Common values:
///       - <b>aac</b>
///       - <b>ac3</b>
///       - <b>cook</b>
///       - <b>dca</b>
///       - <b>dtshd_hra</b>
///       - <b>dtshd_ma</b>
///       - <b>eac3</b>
///       - <b>mp1</b>
///       - <b>mp2</b>
///       - <b>mp3</b>
///       - <b>pcm_s16be</b>
///       - <b>pcm_s16le</b>
///       - <b>pcm_u8</b>
///       - <b>truehd</b>
///       - <b>vorbis</b>
///       - <b>wmapro</b>
///       - <b>wmav2</b>
///     <p>
///   }
///   \table_row3{   <b>`ListItem.AudioChannels`</b>,
///                  \anchor ListItem_AudioChannels
///                  _string_,
///     @return The number of audio channels of the currently selected video. Possible values:
///       - <b>1</b>
///       - <b>2</b>
///       - <b>4</b>
///       - <b>5</b>
///       - <b>6</b>
///       - <b>8</b>
///       - <b>10</b>
///     <p><hr>
///     @skinning_v16 **[Infolabel Updated]** \link ListItem_AudioChannels `ListItem.AudioChannels`\endlink
///     if a video contains no audio\, these infolabels will now return empty.
///     (they used to return 0)
///     <p>
///   }
///   \table_row3{   <b>`ListItem.AudioLanguage`</b>,
///                  \anchor ListItem_AudioLanguage
///                  _string_,
///     @return The audio language of the currently selected video (an
///     ISO 639-2 three character code: e.g. eng\, epo\, deu)
///     <p>
///   }
///   \table_row3{   <b>`ListItem.SubtitleLanguage`</b>,
///                  \anchor ListItem_SubtitleLanguage
///                  _string_,
///     @return The subtitle language of the currently selected video (an
///     ISO 639-2 three character code: e.g. eng\, epo\, deu)
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Property(AudioCodec.[n])`</b>,
///                  \anchor ListItem_Property_AudioCodec
///                  _string_,
///     @return The audio codec of the currently selected video
///     @param n - the number of the audiostream (values: see \ref ListItem_AudioCodec "ListItem.AudioCodec")
///     <p><hr>
///     @skinning_v16 **[New Infolabel]** \link ListItem_Property_AudioCodec `ListItem.Property(AudioCodec.[n])`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Property(AudioChannels.[n])`</b>,
///                  \anchor ListItem_Property_AudioChannels
///                  _string_,
///     @return The number of audio channels of the currently selected video
///     @param n - the number of the audiostream (values: see
///     \ref ListItem_AudioChannels "ListItem.AudioChannels")
///     <p><hr>
///     @skinning_v16 **[New Infolabel]** \link ListItem_Property_AudioChannels `ListItem.Property(AudioChannels.[n])`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Property(AudioLanguage.[n])`</b>,
///                  \anchor ListItem_Property_AudioLanguage
///                  _string_,
///     @return The audio language of the currently selected video
///     @param n - the number of the audiostream (values: see \ref ListItem_AudioLanguage "ListItem.AudioLanguage")
///     <p><hr>
///     @skinning_v16 **[New Infolabel]** \link ListItem_Property_AudioLanguage `ListItem.Property(AudioLanguage.[n])`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Property(SubtitleLanguage.[n])`</b>,
///                  \anchor ListItem_Property_SubtitleLanguage
///                  _string_,
///     @return The subtitle language of the currently selected video 
///     @param n - the number of the subtitle (values: see \ref ListItem_SubtitleLanguage "ListItem.SubtitleLanguage")
///     <p><hr>
///     @skinning_v16 **[New Infolabel]** \link ListItem_Property_SubtitleLanguage `ListItem.Property(SubtitleLanguage.[n])`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Property(Addon.Disclaimer)`</b>,
///                  \anchor ListItem_Property_AddonDisclaimer
///                  _string_,
///     @return The disclaimer of the currently selected addon.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Property(Addon.Changelog)`</b>,
///                  \anchor ListItem_Property_AddonChangelog
///                  _string_,
///     @return The changelog of the currently selected addon.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Property(Addon.ID)`</b>,
///                  \anchor ListItem_Property_AddonID
///                  _string_,
///     @return The identifier of the currently selected addon.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Property(Addon.Status)`</b>,
///                  \anchor ListItem_Property_AddonStatus
///                  _string_,
///     @return The status of the currently selected addon.
///     @todo missing reference in GuiInfoManager.cpp making it hard to track.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Property(Addon.Orphaned)`</b>,
///                  \anchor ListItem_Property_AddonOrphaned
///                  _boolean_,
///     @return **True** if the Addon is orphanad.
///     @todo missing reference in GuiInfoManager.cpp making it hard to track.
///     <p><hr>
///     @skinning_v17 **[New Boolean Condition]** \link ListItem_Property_AddonOrphaned `ListItem.Property(Addon.Orphaned)`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Property(Addon.Path)`</b>,
///                  \anchor ListItem_Property_AddonPath
///                  _string_,
///     @return The path of the currently selected addon.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.StartTime`</b>,
///                  \anchor ListItem_StartTime
///                  _string_,
///     @return The start time of current selected TV programme in a container.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.EndTime`</b>,
///                  \anchor ListItem_EndTime
///                  _string_,
///     @return The end time of current selected TV programme in a container.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.StartDate`</b>,
///                  \anchor ListItem_StartDate
///                  _string_,
///     @return The start date of current selected TV programme in a container.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.EndDate`</b>,
///                  \anchor ListItem_EndDate
///                  _string_,
///     @return The end date of current selected TV programme in a container.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.NextTitle`</b>,
///                  \anchor ListItem_NextTitle
///                  _string_,
///     @return The title of the next item (PVR).
///     <p>
///   }
///   \table_row3{   <b>`ListItem.NextGenre`</b>,
///                  \anchor ListItem_NextGenre
///                  _string_,
///     @return The genre of the next item (PVR).
///     <p>
///   }
///   \table_row3{   <b>`ListItem.NextPlot`</b>,
///                  \anchor ListItem_NextPlot
///                  _string_,
///     @return The plot of the next item (PVR).
///     <p>
///   }
///   \table_row3{   <b>`ListItem.NextPlotOutline`</b>,
///                  \anchor ListItem_NextPlotOutline
///                  _string_,
///     @return The plot outline of the next item (PVR).
///     <p>
///   }
///   \table_row3{   <b>`ListItem.NextStartTime`</b>,
///                  \anchor ListItem_NextStartTime
///                  _string_,
///     @return The start time of the next item (PVR).
///     <p>
///   }
///   \table_row3{   <b>`ListItem.NextEndTime`</b>,
///                  \anchor ListItem_NextEndTime
///                  _string_,
///     @return The end of the next item (PVR).
///     <p>
///   }
///   \table_row3{   <b>`ListItem.NextStartDate`</b>,
///                  \anchor ListItem_NextStartDate
///                  _string_,
///     @return The start date of the next item (PVR).
///     <p>
///   }
///   \table_row3{   <b>`ListItem.NextEndDate`</b>,
///                  \anchor ListItem_NextEndDate
///                  _string_,
///     @return The end date of the next item (PVR).
///     <p>
///   }
///   \table_row3{   <b>`ListItem.NextDuration`</b>,
///                  \anchor ListItem_NextDuration
///                  _string_,
///     @return The duration of the next item (PVR) in the format <b>hh:mm:ss</b>.
///     @note <b>hh:</b> will be omitted if hours value is zero.
///     <p><hr>
///     @skinning_v18 **[New Infolabel]** \link ListItem_NextDuration `ListItem.NextDuration`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.NextDuration(format)`</b>,
///                  \anchor ListItem_NextDuration_format
///                  _string_,
///     @return The duration of the next item (PVR) in different formats.
///     @param format [opt] The format of the return time value.
///     See \ref TIME_FORMAT for the list of possible values.
///     <p><hr>
///     @skinning_v18 **[New Infolabel]** \link ListItem_NextDuration_format `ListItem.NextDuration(format)`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.ChannelGroup`</b>,
///                  \anchor ListItem_ChannelGroup
///                  _string_,
///     @return The channel group of the selected item (PVR).
///     <p>
///   }
///   \table_row3{   <b>`ListItem.ChannelNumberLabel`</b>,
///                  \anchor ListItem_ChannelNumberLabel
///                  _string_,
///     @return The channel and subchannel number of the currently selected channel that's
///     currently playing (PVR).
///     <p><hr>
///     @skinning_v14 **[New Infolabel]** \link ListItem_ChannelNumberLabel `ListItem.ChannelNumberLabel`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Progress`</b>,
///                  \anchor ListItem_Progress
///                  _string_,
///     @return The part of the programme that's been played (PVR).
///     <p>
///   }
///   \table_row3{   <b>`ListItem.StereoscopicMode`</b>,
///                  \anchor ListItem_StereoscopicMode
///                  _string_,
///     @return The stereomode of the selected video:
///       - <b>mono</b>
///       - <b>split_vertical</b>
///       - <b>split_horizontal</b>
///       - <b>row_interleaved</b>
///       - <b>anaglyph_cyan_red</b>
///       - <b>anaglyph_green_magenta</b>
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link ListItem_StereoscopicMode `ListItem.StereoscopicMode`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.HasTimerSchedule`</b>,
///                  \anchor ListItem_HasTimerSchedule
///                  _boolean_,
///     @return **True** if the item was scheduled by a timer rule (PVR).
///     <p><hr>
///     @skinning_v16 **[New Boolean Condition]** \ref ListItem_HasTimerSchedule "ListItem.HasTimerSchedule"
///     <p>
///   }
///   \table_row3{   <b>`ListItem.HasReminder`</b>,
///                  \anchor ListItem_HasReminder
///                  _boolean_,
///     @return **True** if the item has a reminder set (PVR).
///     <p><hr>
///     @skinning_v19 **[New Boolean Condition]** \ref ListItem_HasReminder "ListItem.HasReminder"
///     <p>
///   }
///   \table_row3{   <b>`ListItem.HasReminderRule`</b>,
///                  \anchor ListItem_ListItem.HasReminderRule
///                  _boolean_,
///     @return **True** if the item was scheduled by a reminder timer rule (PVR).
///     <p><hr>
///     @skinning_v19 **[New Boolean Condition]** \ref ListItem_HasReminderRule "ListItem.HasReminderRule"
///     <p>
///   }
///   \table_row3{   <b>`ListItem.HasRecording`</b>,
///                  \anchor ListItem_HasRecording
///                  _boolean_,
///     @return **True** if a given epg tag item currently gets recorded or has been recorded.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.TimerHasError`</b>,
///                  \anchor ListItem_TimerHasError
///                  _boolean_,
///     @return **True** if the item has a timer and it won't be recorded because of an error (PVR).
///     <p><hr>
///     @skinning_v17 **[New Boolean Condition]** \ref ListItem_TimerHasError "ListItem.TimerHasError"
///     <p>
///   }
///   \table_row3{   <b>`ListItem.TimerHasConflict`</b>,
///                  \anchor ListItem_TimerHasConflict
///                  _boolean_,
///     @return **True** if the item has a timer and it won't be recorded because of a conflict (PVR).
///     <p><hr>
///     @skinning_v17 **[New Boolean Condition]** \ref ListItem_TimerHasConflict "ListItem.TimerHasConflict"
///     <p>
///   }
///   \table_row3{   <b>`ListItem.TimerIsActive`</b>,
///                  \anchor ListItem_TimerIsActive
///                  _boolean_,
///     @return **True** if the item has a timer that will be recorded\, i.e. the timer is enabled (PVR).
///     <p><hr>
///     @skinning_v17 **[New Boolean Condition]** \ref ListItem_TimerIsActive "ListItem.TimerIsActive"
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Comment`</b>,
///                  \anchor ListItem_Comment
///                  _string_,
///     @return The comment assigned to the item (PVR/MUSIC).
///     <p>
///   }
///   \table_row3{   <b>`ListItem.TimerType`</b>,
///                  \anchor ListItem_TimerType
///                  _string_,
///     @return The type of the PVR timer / timer rule item as a human readable string.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.EpgEventTitle`</b>,
///                  \anchor ListItem_EpgEventTitle
///                  _string_,
///     @return The title of the epg event associated with the item\, if any.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.EpgEventIcon`</b>,
///                  \anchor ListItem_EpgEventIcon
///                  _string_,
///     @return The thumbnail for the EPG event associated with the item (if it exists).
///     <p><hr>
///     @skinning_v18 **[New Infolabel]** \link ListItem_EpgEventIcon `ListItem.EpgEventIcon`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.InProgress`</b>,
///                  \anchor ListItem_InProgress
///                  _boolean_,
///     @return **True** if the EPG event item is currently active (time-wise).
///     <p>
///   }
///   \table_row3{   <b>`ListItem.IsParentFolder`</b>,
///                  \anchor ListItem_IsParentFolder
///                  _boolean_,
///     @return **True** if the current list item is the goto parent folder '..'.
///     <p><hr>
///     @skinning_v17 **[New Boolean Condition]** \link ListItem_IsParentFolder `ListItem.IsParentFolder`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.AddonName`</b>,
///                  \anchor ListItem_AddonName
///                  _string_,
///     @return The name of the currently selected addon.
///     <p><hr>
///     @skinning_v17 **[Infolabel Updated]** \link ListItem_AddonName `ListItem.AddonName`\endlink
///     replaces `ListItem.Property(Addon.Name)`.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.AddonVersion`</b>,
///                  \anchor ListItem_AddonVersion
///                  _string_,
///     @return The version of the currently selected addon.
///     <p><hr>
///     @skinning_v17 **[Infolabel Updated]** \link ListItem_AddonVersion `ListItem.AddonVersion`\endlink
///     replaces `ListItem.Property(Addon.Version)`.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.AddonCreator`</b>,
///                  \anchor ListItem_AddonCreator
///                  _string_,
///     @return The name of the author the currently selected addon.
///     <p><hr>
///     @skinning_v17 **[Infolabel Updated]** \link ListItem_AddonCreator `ListItem.AddonCreator`\endlink
///     replaces `ListItem.Property(Addon.Creator)`.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.AddonSummary`</b>,
///                  \anchor ListItem_AddonSummary
///                  _string_,
///     @return A short description of the currently selected addon.
///     <p><hr>
///     @skinning_v17 **[Infolabel Updated]** \link ListItem_AddonSummary `ListItem.AddonSummary`\endlink
///     replaces `ListItem.Property(Addon.Summary)`.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.AddonDescription`</b>,
///                  \anchor ListItem_AddonDescription
///                  _string_,
///     @return The full description of the currently selected addon.
///     <p><hr>
///     @skinning_v17 **[Infolabel Updated]** \link ListItem_AddonDescription `ListItem.AddonDescription`\endlink
///     replaces `ListItem.Property(Addon.Description)`.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.AddonDisclaimer`</b>,
///                  \anchor ListItem_AddonDisclaimer
///                  _string_,
///     @return The disclaimer of the currently selected addon.
///     <p><hr>
///     @skinning_v17 **[Infolabel Updated]** \link ListItem_AddonDisclaimer `ListItem.AddonDisclaimer`\endlink
///     replaces `ListItem.Property(Addon.Disclaimer)`.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.AddonBroken`</b>,
///                  \anchor ListItem_AddonBroken
///                  _string_,
///     @return A message when the addon is marked as broken in the repo.
///     <p><hr>
///     @skinning_v17 **[Infolabel Updated]** \link ListItem_AddonBroken `ListItem.AddonBroken`\endlink
///     replaces `ListItem.Property(Addon.Broken)`.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.AddonType`</b>,
///                  \anchor ListItem_AddonType
///                  _string_,
///     @return The type (screensaver\, script\, skin\, etc...) of the currently selected addon.
///     <p><hr>
///     @skinning_v17 **[Infolabel Updated]** \link ListItem_AddonType `ListItem.AddonType`\endlink
///     replaces `ListItem.Property(Addon.Type)`.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.AddonInstallDate`</b>,
///                  \anchor ListItem_AddonInstallDate
///                  _string_,
///     @return The date the addon was installed.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link ListItem_AddonInstallDate `ListItem.AddonInstallDate`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.AddonLastUpdated`</b>,
///                  \anchor ListItem_AddonLastUpdated
///                  _string_,
///     @return The date the addon was last updated.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link ListItem_AddonLastUpdated `ListItem.AddonLastUpdated`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.AddonLastUsed`</b>,
///                  \anchor ListItem_AddonLastUsed
///                  _string_,
///     @return The date the addon was used last.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link ListItem_AddonLastUsed `ListItem.AddonLastUsed`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.AddonNews`</b>,
///                  \anchor ListItem_AddonNews
///                  _string_,
///     @return A brief changelog\, taken from the addons' `addon.xml` file.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link ListItem_AddonNews `ListItem.AddonNews`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.AddonSize`</b>,
///                  \anchor ListItem_AddonSize
///                  _string_,
///     @return The filesize of the addon.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link ListItem_AddonSize `ListItem.AddonSize`\endlink
///     <p>
///   }
///   \table_row3{   <b>`ListItem.AddonOrigin`</b>,
///                  \anchor ListItem_AddonOrigin
///                  _string_,
///     @return The name of the repository the add-on originates from.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.ExpirationDate`</b>,
///                  \anchor ListItem_ExpirationDate
///                  _string_,
///     @return The expiration date of the selected item in a container\, empty string if not supported.
///     <p>
///   }
///   \table_row3{   <b>`ListItem.ExpirationTime`</b>,
///                  \anchor ListItem_ExpirationTime
///                  _string_,
///     @return The expiration time of the selected item in a container\, empty string if not supported
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Art(type)`</b>,
///                  \anchor ListItem_Art_Type
///                  _string_,
///     @return A particular art type for an item.
///     @param type - the art type. It can be any value (set by scripts and scrappers). Common values:
///       - <b>clearart</b> - the clearart (if it exists) of the currently selected movie or tv show.
///       - <b>clearlogo</b> - the clearlogo (if it exists) of the currently selected movie or tv show.
///       - <b>landscape</b> - the 16:9 landscape (if it exists) of the currently selected item.
///       - <b>thumb</b> - the thumbnail of the currently selected item.
///       - <b>poster</b> - the poster of the currently selected movie or tv show.
///       - <b>banner</b> - the banner of the currently selected tv show.
///       - <b>fanart</b> - the fanart image of the currently selected item.
///       - <b>set.fanart</b> - the fanart image of the currently selected movieset.
///       - <b>tvshow.poster</b> - the tv show poster of the parent container.
///       - <b>tvshow.banner</b> - the tv show banner of the parent container.
///       - <b>tvshow.clearlogo</b> - the tv show clearlogo (if it exists) of the parent container.
///       - <b>tvshow.landscape</b> - the tv show landscape (if it exists) of the parent container.
///       - <b>tvshow.clearart</b> - the tv show clearart (if it exists) of the parent container.
///       - <b>season.poster</b> - the season poster of the currently selected season. (Only available in DialogVideoInfo.xml).
///       - <b>season.banner</b> - the season banner of the currently selected season. (Only available in DialogVideoInfo.xml).
///       - <b>season.fanart</b> - the fanart image of the currently selected season. (Only available in DialogVideoInfo.xml)
///       - <b>artist.thumb</b> - the artist thumb of an album or song item.
///       - <b>artist.fanart</b> - the artist fanart of an album or song item.
///       - <b>album.thumb</b> - the album thumb (cover) of a song item.
///       - <b>artist[n].*</b> - in case a song has multiple artists\, a digit is added to the art type for the 2nd artist onwards
/// e.g `Listitem.Art(artist1.thumb)` gives the thumb of the 2nd artist of a song.	
///       - <b>albumartist[n].*</b> - n case a song has multiple album artists\, a digit is added to the art type for the 2nd artist
/// onwards e.g `Listitem.Art(artist1.thumb)` gives the thumb of the 2nd artist of a song.
///     <p>
///     @todo Find a better way of finding the art types instead of manually defining them here.
///     <p><hr>
///     @skinning_v18 **[Infolabel Updated]** \link ListItem_Art_Type `ListItem.Art(type)`\endlink add <b>artist[n].*</b> and
///     <b>albumartist[n].*</b> as possible targets for <b>type</b>
///     <p>  
///   }
///   \table_row3{   <b>`ListItem.Platform`</b>,
///                  \anchor ListItem_Platform
///                  _string_,
///     @return The game platform (e.g. "Atari 2600") (RETROPLAYER).
///     <p><hr>
///     @skinning_v18 **[New Infolabel]** \link ListItem_Platform `ListItem.Platform`\endlink
///     <p>  
///   }
///   \table_row3{   <b>`ListItem.Genres`</b>,
///                  \anchor ListItem_Genres
///                  _string_,
///     @return The game genres (e.g. "["Action"\,"Strategy"]") (RETROPLAYER).
///     <p><hr>
///     @skinning_v18 **[New Infolabel]** \link ListItem_Genres `ListItem.Genres`\endlink
///     <p>  
///   }
///   \table_row3{   <b>`ListItem.Publisher`</b>,
///                  \anchor ListItem_Publisher
///                  _string_,
///     @return The game publisher (e.g. "Nintendo") (RETROPLAYER).
///     <p><hr>
///     @skinning_v18 **[New Infolabel]** \link ListItem_Publisher `ListItem.Publisher`\endlink
///     <p>  
///   }
///   \table_row3{   <b>`ListItem.Developer`</b>,
///                  \anchor ListItem_Developer
///                  _string_,
///     @return The game developer (e.g. "Square") (RETROPLAYER).
///     <p><hr>
///     @skinning_v18 **[New Infolabel]** \link ListItem_Developer `ListItem.Developer`\endlink
///     <p>  
///   }
///   \table_row3{   <b>`ListItem.Overview`</b>,
///                  \anchor ListItem_Overview
///                  _string_,
///     @return The game overview/summary (RETROPLAYER).
///     <p><hr>
///     @skinning_v18 **[New Infolabel]** \link ListItem_Overview `ListItem.Overview`\endlink
///     <p>  
///   }
///   \table_row3{   <b>`ListItem.GameClient`</b>,
///                  \anchor ListItem_GameClient
///                  _string_,
///     @return The add-on ID of the game client (a.k.a. emulator) to use for playing the game
///     (e.g. game.libretro.fceumm) (RETROPLAYER).
///     <p><hr>
///     @skinning_v18 **[New Infolabel]** \link ListItem_GameClient `ListItem.GameClient`\endlink
///     <p>  
///   }
///   \table_row3{   <b>`ListItem.Property(propname)`</b>,
///                  \anchor ListItem_Property_Propname
///                  _string_,
///     @return The requested property of a ListItem.
///     @param propname - the property requested
///     <p>
///   }
///   \table_row3{   <b>`ListItem.Property(Role.Composer)`</b>,
///                  \anchor ListItem_Property_Role_Composer
///                  _string_,
///     @return The name of the person who composed the selected song.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link ListItem_Property_Role_Composer `ListItem.Property(Role.Composer)`\endlink
///     <p>  
///   }
///   \table_row3{   <b>`ListItem.Property(Role.Conductor)`</b>,
///                  \anchor ListItem_Property_Role_Conductor
///                  _string_,
///     @return The name of the person who conducted the selected song.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link ListItem_Property_Role_Conductor `ListItem.Property(Role.Conductor)`\endlink
///     <p>  
///   }
///   \table_row3{   <b>`ListItem.Property(Role.Orchestra)`</b>,
///                  \anchor ListItem_Property_Role_Orchestra
///                  _string_,
///     @return The name of the orchestra performing the selected song.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link ListItem_Property_Role_Orchestra `ListItem.Property(Role.Orchestra)`\endlink
///     <p>  
///   }
///   \table_row3{   <b>`ListItem.Property(Role.Lyricist)`</b>,
///                  \anchor ListItem_Property_Role_Lyricist
///                  _string_,
///     @return The name of the person who wrote the lyrics of the selected song.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link ListItem_Property_Role_Lyricist `ListItem.Property(Role.Lyricist)`\endlink
///     <p>  
///   }
///   \table_row3{   <b>`ListItem.Property(Role.Remixer)`</b>,
///                  \anchor ListItem_Property_Role_Remixer
///                  _string_,
///     @return The name of the person who remixed the selected song.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link ListItem_Property_Role_Remixer `ListItem.Property(Role.Remixer)`\endlink
///     <p>  
///   }
///   \table_row3{   <b>`ListItem.Property(Role.Arranger)`</b>,
///                  \anchor ListItem_Property_Role_Arranger
///                  _string_,
///     @return The name of the person who arranged the selected song.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link ListItem_Property_Role_Arranger `ListItem.Property(Role.Arranger)`\endlink
///     <p>  
///   }
///   \table_row3{   <b>`ListItem.Property(Role.Engineer)`</b>,
///                  \anchor ListItem_Property_Role_Engineer
///                  _string_,
///     @return The name of the person who was the engineer of the selected song.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link ListItem_Property_Role_Engineer `ListItem.Property(Role.Engineer)`\endlink
///     <p>  
///   }
///   \table_row3{   <b>`ListItem.Property(Role.Producer)`</b>,
///                  \anchor ListItem_Property_Role_Producer
///                  _string_,
///     @return The name of the person who produced the selected song.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link ListItem_Property_Role_Producer `ListItem.Property(Role.Producer)`\endlink
///     <p>  
///   }
///   \table_row3{   <b>`ListItem.Property(Role.DJMixer)`</b>,
///                  \anchor ListItem_Property_Role_DJMixer
///                  _string_,
///     @return The name of the dj who remixed the selected song.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link ListItem_Property_Role_DJMixer `ListItem.Property(Role.DJMixer)`\endlink
///     <p>  
///   }
///   \table_row3{   <b>`ListItem.Property(Role.Mixer)`</b>,
///                  \anchor ListItem_Property_Role_Mixer
///                  _string_,
///     @return The name of the person who mixed the selected song.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link ListItem_Property_Role_DJMixer `ListItem.Property(Role.DJMixer)`\endlink
///     <p>  
///   }
///   \table_row3{   <b>`ListItem.Property(Game.VideoFilter)`</b>,
///                  \anchor ListItem_Property_Game_VideoFilter
///                  _string_,
///     @return The video filter of the list item representing a
///     gamewindow control (RETROPLAYER).
///     See \link RetroPlayer_VideoFilter RetroPlayer.VideoFilter \endlink
///     for the possible values.
///     <p><hr>
///     @skinning_v18 **[New Infolabel]** \link ListItem_Property_Game_VideoFilter `ListItem.Property(Game.VideoFilter)`\endlink
///     <p>  
///   }
///   \table_row3{   <b>`ListItem.Property(Game.StretchMode)`</b>,
///                  \anchor ListItem_Property_Game_StretchMode
///                  _string_,
///     @return The stretch mode of the list item representing a
///     gamewindow control (RETROPLAYER).
///     See \link RetroPlayer_StretchMode RetroPlayer.StretchMode \endlink
///     for the possible values.
///     <p><hr>
///     @skinning_v18 **[New Infolabel]** \link ListItem_Property_Game_StretchMode `ListItem.Property(Game.StretchMode)`\endlink
///     <p>  
///   }
///   \table_row3{   <b>`ListItem.Property(Game.VideoRotation)`</b>,
///                  \anchor ListItem_Property_Game_VideoRotation
///                  _integer_,
///     @return The video rotation of the list item representing a
///     gamewindow control (RETROPLAYER).
///     See \link RetroPlayer_VideoRotation RetroPlayer.VideoRotation \endlink
///     for the possible values.
///     <p><hr>
///     @skinning_v18 **[New Infolabel]** \link ListItem_Property_Game_VideoRotation `ListItem.Property(Game.VideoRotation)`\endlink
///     <p>  
///   }
///   \table_row3{   <b>`ListItem.ParentalRating`</b>,
///                  \anchor ListItem_ParentalRating
///                  _string_,
///     @return The parental rating of the list item (PVR).
///     <p>
///   }
/// \table_end
///
/// -----------------------------------------------------------------------------
const infomap listitem_labels[]= {{ "thumb",            LISTITEM_THUMB },
                                  { "icon",             LISTITEM_ICON },
                                  { "actualicon",       LISTITEM_ACTUAL_ICON },
                                  { "overlay",          LISTITEM_OVERLAY },
                                  { "label",            LISTITEM_LABEL },
                                  { "label2",           LISTITEM_LABEL2 },
                                  { "title",            LISTITEM_TITLE },
                                  { "tracknumber",      LISTITEM_TRACKNUMBER },
                                  { "artist",           LISTITEM_ARTIST },
                                  { "album",            LISTITEM_ALBUM },
                                  { "albumartist",      LISTITEM_ALBUM_ARTIST },
                                  { "year",             LISTITEM_YEAR },
                                  { "genre",            LISTITEM_GENRE },
                                  { "contributors",     LISTITEM_CONTRIBUTORS },
                                  { "contributorandrole", LISTITEM_CONTRIBUTOR_AND_ROLE },
                                  { "director",         LISTITEM_DIRECTOR },
                                  { "filename",         LISTITEM_FILENAME },
                                  { "filenameandpath",  LISTITEM_FILENAME_AND_PATH },
                                  { "fileextension",    LISTITEM_FILE_EXTENSION },
                                  { "date",             LISTITEM_DATE },
                                  { "datetime",         LISTITEM_DATETIME },
                                  { "size",             LISTITEM_SIZE },
                                  { "rating",           LISTITEM_RATING },
                                  { "ratingandvotes",   LISTITEM_RATING_AND_VOTES },
                                  { "userrating",       LISTITEM_USER_RATING },
                                  { "votes",            LISTITEM_VOTES },
                                  { "mood",             LISTITEM_MOOD },
                                  { "programcount",     LISTITEM_PROGRAM_COUNT },
                                  { "duration",         LISTITEM_DURATION },
                                  { "isselected",       LISTITEM_ISSELECTED },
                                  { "isplaying",        LISTITEM_ISPLAYING },
                                  { "plot",             LISTITEM_PLOT },
                                  { "plotoutline",      LISTITEM_PLOT_OUTLINE },
                                  { "episode",          LISTITEM_EPISODE },
                                  { "season",           LISTITEM_SEASON },
                                  { "tvshowtitle",      LISTITEM_TVSHOW },
                                  { "premiered",        LISTITEM_PREMIERED },
                                  { "comment",          LISTITEM_COMMENT },
                                  { "path",             LISTITEM_PATH },
                                  { "foldername",       LISTITEM_FOLDERNAME },
                                  { "folderpath",       LISTITEM_FOLDERPATH },
                                  { "picturepath",      LISTITEM_PICTURE_PATH },
                                  { "pictureresolution",LISTITEM_PICTURE_RESOLUTION },
                                  { "picturedatetime",  LISTITEM_PICTURE_DATETIME },
                                  { "picturedate",      LISTITEM_PICTURE_DATE },
                                  { "picturelongdatetime",LISTITEM_PICTURE_LONGDATETIME },
                                  { "picturelongdate",  LISTITEM_PICTURE_LONGDATE },
                                  { "picturecomment",   LISTITEM_PICTURE_COMMENT },
                                  { "picturecaption",   LISTITEM_PICTURE_CAPTION },
                                  { "picturedesc",      LISTITEM_PICTURE_DESC },
                                  { "picturekeywords",  LISTITEM_PICTURE_KEYWORDS },
                                  { "picturecammake",   LISTITEM_PICTURE_CAM_MAKE },
                                  { "picturecammodel",  LISTITEM_PICTURE_CAM_MODEL },
                                  { "pictureaperture",  LISTITEM_PICTURE_APERTURE },
                                  { "picturefocallen",  LISTITEM_PICTURE_FOCAL_LEN },
                                  { "picturefocusdist", LISTITEM_PICTURE_FOCUS_DIST },
                                  { "pictureexpmode",   LISTITEM_PICTURE_EXP_MODE },
                                  { "pictureexptime",   LISTITEM_PICTURE_EXP_TIME },
                                  { "pictureiso",       LISTITEM_PICTURE_ISO },
                                  { "pictureauthor",                 LISTITEM_PICTURE_AUTHOR },
                                  { "picturebyline",                 LISTITEM_PICTURE_BYLINE },
                                  { "picturebylinetitle",            LISTITEM_PICTURE_BYLINE_TITLE },
                                  { "picturecategory",               LISTITEM_PICTURE_CATEGORY },
                                  { "pictureccdwidth",               LISTITEM_PICTURE_CCD_WIDTH },
                                  { "picturecity",                   LISTITEM_PICTURE_CITY },
                                  { "pictureurgency",                LISTITEM_PICTURE_URGENCY },
                                  { "picturecopyrightnotice",        LISTITEM_PICTURE_COPYRIGHT_NOTICE },
                                  { "picturecountry",                LISTITEM_PICTURE_COUNTRY },
                                  { "picturecountrycode",            LISTITEM_PICTURE_COUNTRY_CODE },
                                  { "picturecredit",                 LISTITEM_PICTURE_CREDIT },
                                  { "pictureiptcdate",               LISTITEM_PICTURE_IPTCDATE },
                                  { "picturedigitalzoom",            LISTITEM_PICTURE_DIGITAL_ZOOM },
                                  { "pictureexposure",               LISTITEM_PICTURE_EXPOSURE },
                                  { "pictureexposurebias",           LISTITEM_PICTURE_EXPOSURE_BIAS },
                                  { "pictureflashused",              LISTITEM_PICTURE_FLASH_USED },
                                  { "pictureheadline",               LISTITEM_PICTURE_HEADLINE },
                                  { "picturecolour",                 LISTITEM_PICTURE_COLOUR },
                                  { "picturelightsource",            LISTITEM_PICTURE_LIGHT_SOURCE },
                                  { "picturemeteringmode",           LISTITEM_PICTURE_METERING_MODE },
                                  { "pictureobjectname",             LISTITEM_PICTURE_OBJECT_NAME },
                                  { "pictureorientation",            LISTITEM_PICTURE_ORIENTATION },
                                  { "pictureprocess",                LISTITEM_PICTURE_PROCESS },
                                  { "picturereferenceservice",       LISTITEM_PICTURE_REF_SERVICE },
                                  { "picturesource",                 LISTITEM_PICTURE_SOURCE },
                                  { "picturespecialinstructions",    LISTITEM_PICTURE_SPEC_INSTR },
                                  { "picturestate",                  LISTITEM_PICTURE_STATE },
                                  { "picturesupplementalcategories", LISTITEM_PICTURE_SUP_CATEGORIES },
                                  { "picturetransmissionreference",  LISTITEM_PICTURE_TX_REFERENCE },
                                  { "picturewhitebalance",           LISTITEM_PICTURE_WHITE_BALANCE },
                                  { "pictureimagetype",              LISTITEM_PICTURE_IMAGETYPE },
                                  { "picturesublocation",            LISTITEM_PICTURE_SUBLOCATION },
                                  { "pictureiptctime",               LISTITEM_PICTURE_TIMECREATED },
                                  { "picturegpslat",    LISTITEM_PICTURE_GPS_LAT },
                                  { "picturegpslon",    LISTITEM_PICTURE_GPS_LON },
                                  { "picturegpsalt",    LISTITEM_PICTURE_GPS_ALT },
                                  { "studio",           LISTITEM_STUDIO },
                                  { "country",          LISTITEM_COUNTRY },
                                  { "mpaa",             LISTITEM_MPAA },
                                  { "cast",             LISTITEM_CAST },
                                  { "castandrole",      LISTITEM_CAST_AND_ROLE },
                                  { "writer",           LISTITEM_WRITER },
                                  { "tagline",          LISTITEM_TAGLINE },
                                  { "status",           LISTITEM_STATUS },
                                  { "top250",           LISTITEM_TOP250 },
                                  { "trailer",          LISTITEM_TRAILER },
                                  { "sortletter",       LISTITEM_SORT_LETTER },
                                  { "tag",              LISTITEM_TAG },
                                  { "set",              LISTITEM_SET },
                                  { "setid",            LISTITEM_SETID },
                                  { "videocodec",       LISTITEM_VIDEO_CODEC },
                                  { "videoresolution",  LISTITEM_VIDEO_RESOLUTION },
                                  { "videoaspect",      LISTITEM_VIDEO_ASPECT },
                                  { "audiocodec",       LISTITEM_AUDIO_CODEC },
                                  { "audiochannels",    LISTITEM_AUDIO_CHANNELS },
                                  { "audiolanguage",    LISTITEM_AUDIO_LANGUAGE },
                                  { "subtitlelanguage", LISTITEM_SUBTITLE_LANGUAGE },
                                  { "isresumable",      LISTITEM_IS_RESUMABLE},
                                  { "percentplayed",    LISTITEM_PERCENT_PLAYED},
                                  { "isfolder",         LISTITEM_IS_FOLDER },
                                  { "isparentfolder",   LISTITEM_IS_PARENTFOLDER },
                                  { "iscollection",     LISTITEM_IS_COLLECTION },
                                  { "originaltitle",    LISTITEM_ORIGINALTITLE },
                                  { "lastplayed",       LISTITEM_LASTPLAYED },
                                  { "playcount",        LISTITEM_PLAYCOUNT },
                                  { "discnumber",       LISTITEM_DISC_NUMBER },
                                  { "starttime",        LISTITEM_STARTTIME },
                                  { "endtime",          LISTITEM_ENDTIME },
                                  { "endtimeresume",    LISTITEM_ENDTIME_RESUME },
                                  { "startdate",        LISTITEM_STARTDATE },
                                  { "enddate",          LISTITEM_ENDDATE },
                                  { "nexttitle",        LISTITEM_NEXT_TITLE },
                                  { "nextgenre",        LISTITEM_NEXT_GENRE },
                                  { "nextplot",         LISTITEM_NEXT_PLOT },
                                  { "nextplotoutline",  LISTITEM_NEXT_PLOT_OUTLINE },
                                  { "nextstarttime",    LISTITEM_NEXT_STARTTIME },
                                  { "nextendtime",      LISTITEM_NEXT_ENDTIME },
                                  { "nextstartdate",    LISTITEM_NEXT_STARTDATE },
                                  { "nextenddate",      LISTITEM_NEXT_ENDDATE },
                                  { "nextduration",     LISTITEM_NEXT_DURATION },
                                  { "channelname",      LISTITEM_CHANNEL_NAME },
                                  { "channelnumberlabel", LISTITEM_CHANNEL_NUMBER },
                                  { "channelgroup",     LISTITEM_CHANNEL_GROUP },
                                  { "hasepg",           LISTITEM_HAS_EPG },
                                  { "hastimer",         LISTITEM_HASTIMER },
                                  { "hastimerschedule", LISTITEM_HASTIMERSCHEDULE },
                                  { "hasreminder",      LISTITEM_HASREMINDER },
                                  { "hasreminderrule",  LISTITEM_HASREMINDERRULE },
                                  { "hasrecording",     LISTITEM_HASRECORDING },
                                  { "isrecording",      LISTITEM_ISRECORDING },
                                  { "isplayable",       LISTITEM_ISPLAYABLE },
                                  { "hasarchive",       LISTITEM_HASARCHIVE },
                                  { "inprogress",       LISTITEM_INPROGRESS },
                                  { "isencrypted",      LISTITEM_ISENCRYPTED },
                                  { "progress",         LISTITEM_PROGRESS },
                                  { "dateadded",        LISTITEM_DATE_ADDED },
                                  { "dbtype",           LISTITEM_DBTYPE },
                                  { "dbid",             LISTITEM_DBID },
                                  { "appearances",      LISTITEM_APPEARANCES },
                                  { "stereoscopicmode", LISTITEM_STEREOSCOPIC_MODE },
                                  { "isstereoscopic",   LISTITEM_IS_STEREOSCOPIC },
                                  { "imdbnumber",       LISTITEM_IMDBNUMBER },
                                  { "episodename",      LISTITEM_EPISODENAME },
                                  { "timertype",        LISTITEM_TIMERTYPE },
                                  { "epgeventtitle",    LISTITEM_EPG_EVENT_TITLE },
                                  { "epgeventicon",     LISTITEM_EPG_EVENT_ICON },
                                  { "timerisactive",    LISTITEM_TIMERISACTIVE },
                                  { "timerhaserror",    LISTITEM_TIMERHASERROR },
                                  { "timerhasconflict", LISTITEM_TIMERHASCONFLICT },
                                  { "addonname",        LISTITEM_ADDON_NAME },
                                  { "addonversion",     LISTITEM_ADDON_VERSION },
                                  { "addoncreator",     LISTITEM_ADDON_CREATOR },
                                  { "addonsummary",     LISTITEM_ADDON_SUMMARY },
                                  { "addondescription", LISTITEM_ADDON_DESCRIPTION },
                                  { "addondisclaimer",  LISTITEM_ADDON_DISCLAIMER },
                                  { "addonnews",        LISTITEM_ADDON_NEWS },
                                  { "addonbroken",      LISTITEM_ADDON_BROKEN },
                                  { "addontype",        LISTITEM_ADDON_TYPE },
                                  { "addoninstalldate", LISTITEM_ADDON_INSTALL_DATE },
                                  { "addonlastupdated", LISTITEM_ADDON_LAST_UPDATED },
                                  { "addonlastused",    LISTITEM_ADDON_LAST_USED },
                                  { "addonorigin",      LISTITEM_ADDON_ORIGIN },
                                  { "addonsize",        LISTITEM_ADDON_SIZE },
                                  { "expirationdate",   LISTITEM_EXPIRATION_DATE },
                                  { "expirationtime",   LISTITEM_EXPIRATION_TIME },
                                  { "art",              LISTITEM_ART },
                                  { "property",         LISTITEM_PROPERTY },
                                  { "parentalrating",   LISTITEM_PARENTAL_RATING }
};

/// \page modules__infolabels_boolean_conditions
/// \subsection modules__infolabels_boolean_conditions_Visualisation Visualisation
/// \table_start
///   \table_h3{ Labels, Type, Description }
///   \table_row3{   <b>`Visualisation.Enabled`</b>,
///                  \anchor Visualisation_Enabled
///                  _boolean_,
///     @return **True** if any visualisation has been set in settings (so not None).
///     <p>
///   }
///   \table_row3{   <b>`Visualisation.HasPresets`</b>,
///                  \anchor Visualisation_HasPresets
///                  _boolean_,
///     @return **True** if the visualisation has built in presets.
///     <p><hr>
///     @skinning_v16 **[New Boolean Condition]** \link Visualisation_HasPresets `Visualisation.HasPresets`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Visualisation.Locked`</b>,
///                  \anchor Visualisation_Locked
///                  _boolean_,
///     @return **True** if the current visualisation preset is locked (e.g. in Milkdrop).
///     <p>
///   }
///   \table_row3{   <b>`Visualisation.Preset`</b>,
///                  \anchor Visualisation_Preset
///                  _string_,
///     @return The current preset of the visualisation.
///     <p>
///   }
///   \table_row3{   <b>`Visualisation.Name`</b>,
///                  \anchor Visualisation_Name
///                  _string_,
///     @return the name of the visualisation.
///     <p>
///   }
/// \table_end
///
/// -----------------------------------------------------------------------------
const infomap visualisation[] =  {{ "locked",           VISUALISATION_LOCKED },
                                  { "preset",           VISUALISATION_PRESET },
                                  { "haspresets",       VISUALISATION_HAS_PRESETS },
                                  { "name",             VISUALISATION_NAME },
                                  { "enabled",          VISUALISATION_ENABLED }};

/// \page modules__infolabels_boolean_conditions
/// \subsection modules__infolabels_boolean_conditions_Fanart Fanart
/// \table_start
///   \table_h3{ Labels, Type, Description }
///   \table_row3{   <b>`Fanart.Color1`</b>,
///                  \anchor Fanart_Color1
///                  _string_,
///     @return The first of three colors included in the currently selected
///     Fanart theme for the parent TV Show.
///     @note Colors are arranged Lightest to Darkest.
///     <p>
///   }
///   \table_row3{   <b>`Fanart.Color2`</b>,
///                  \anchor Fanart_Color2
///                  _string_,
///     @return The second of three colors included in the currently selected
///     Fanart theme for the parent TV Show.
///     @note Colors are arranged Lightest to Darkest.
///     <p>
///   }
///   \table_row3{   <b>`Fanart.Color3`</b>,
///                  \anchor Fanart_Color3
///                  _string_,
///     @return The third of three colors included in the currently selected
///     Fanart theme for the parent TV Show.
///     @note Colors are arranged Lightest to Darkest.
///     <p>
///   }
///   \table_row3{   <b>`Fanart.Image`</b>,
///                  \anchor Fanart_Image
///                  _string_,
///     @return The fanart image\, if any
///     <p>
///   }
/// \table_end
///
/// -----------------------------------------------------------------------------
const infomap fanart_labels[] =  {{ "color1",           FANART_COLOR1 },
                                  { "color2",           FANART_COLOR2 },
                                  { "color3",           FANART_COLOR3 },
                                  { "image",            FANART_IMAGE }};

/// \page modules__infolabels_boolean_conditions
/// \subsection modules__infolabels_boolean_conditions_Skin Skin
/// \table_start
///   \table_h3{ Labels, Type, Description }
///   \table_row3{   <b>`Skin.CurrentTheme`</b>,
///                  \anchor Skin_CurrentTheme
///                  _string_,
///     @return The current selected skin theme.
///     <p>
///   }
///   \table_row3{   <b>`Skin.CurrentColourTheme`</b>,
///                  \anchor Skin_CurrentColourTheme
///                  _string_,
///     @return the current selected colour theme of the skin.
///     <p>
///   }
///   \table_row3{   <b>`Skin.AspectRatio`</b>,
///                  \anchor Skin_AspectRatio
///                  _string_,
///     @return The closest aspect ratio match using the resolution info from the skin's `addon.xml` file.
///     <p>
///   }
///   \table_row3{   <b>`Skin.Font`</b>,
///                  \anchor Skin_Font
///                  _string_,
///     @return the current fontset from `Font.xml`.
///     <p><hr>
///     @skinning_v18 **[New Infolabel]** \link Skin_Font `Skin.Font`\endlink
///     <p>
///   }
/// \table_end
///
/// -----------------------------------------------------------------------------
const infomap skin_labels[] =    {{ "currenttheme",      SKIN_THEME },
                                  { "currentcolourtheme",SKIN_COLOUR_THEME },
                                  { "aspectratio",       SKIN_ASPECT_RATIO},
                                  { "font",              SKIN_FONT}};

/// \page modules__infolabels_boolean_conditions
/// \subsection modules__infolabels_boolean_conditions_Window Window
/// \table_start
///   \table_h3{ Labels, Type, Description }
///   \table_row3{   <b>`Window.IsMedia`</b>,
///                  \anchor Window_IsMedia
///                  _boolean_,
///     @return **True** if this window is a media window (programs\, music\, video\,
///     scripts\, pictures)
///     <p>
///   }
///   \table_row3{   <b>`Window.Is(window)`</b>,
///                  \anchor Window_Is
///                  _boolean_,
///     @return **True** if the window with the given name is the window which is currently rendered.
///     @param window - the name of the window
///     @note Useful in xml files that are shared between multiple windows or dialogs.
///     <p><hr>
///     @skinning_v17 **[New Boolean Condition]** \ref Window_Is "Window.Is(window)"
///     <p>
///   }
///   \table_row3{   <b>`Window.IsActive(window)`</b>,
///                  \anchor Window_IsActive
///                  _boolean_,
///     @return **True** if the window with id or title _window_ is active
///     @param window - the id or name of the window
///     @note Excludes fade out time on dialogs
///     <p>
///   }
///   \table_row3{   <b>`Window.IsVisible(window)`</b>,
///                  \anchor Window_IsVisible
///                  _boolean_,
///     @return **True** if the window is visible
///     @note Includes fade out time on dialogs
///     <p>
///   }
///   \table_row3{   <b>`Window.IsTopmost(window)`</b>,
///                  \anchor Window_IsTopmost
///                  _boolean_,
///     @return **True** if the window with id or title _window_ is on top of the
///     window stack.
///     @param window - the id or name of the window
///     @note Excludes fade out time on dialogs
///     @deprecated use  \ref Window_IsDialogTopmost "Window.IsDialogTopmost(dialog)" instead
///     <p>
///   }
///   \table_row3{   <b>`Window.IsDialogTopmost(dialog)`</b>,
///                  \anchor Window_IsDialogTopmost
///                  _boolean_,
///     @return **True** if the dialog with id or title _dialog_ is on top of the
///     dialog stack.
///     @param window - the id or name of the window
///     @note Excludes fade out time on dialogs
///     <p>
///   }
///   \table_row3{   <b>`Window.IsModalDialogTopmost(dialog)`</b>,
///                  \anchor Window_IsModalDialogTopmost
///                  _boolean_,
///     @return **True** if the dialog with id or title _dialog_ is on top of the
///     modal dialog stack 
///     @note Excludes fade out time on dialogs
///     <p>
///   }
///   \table_row3{   <b>`Window.Previous(window)`</b>,
///                  \anchor Window_Previous
///                  _boolean_,
///     @return **True** if the window with id or title _window_ is being moved from.
///     @param window - the window id or title
///     @note Only valid while windows are changing.
///     <p>
///   }
///   \table_row3{   <b>`Window.Next(window)`</b>,
///                  \anchor Window_Next
///                  _boolean_,
///     @return **True** if the window with id or title _window_ is being moved to.
///     @param window - the window id or title
///     @note Only valid while windows are changing.
///     <p>
///   }
///   \table_row3{   <b>`Window.Property(Addon.ID)`</b>,
///                  \anchor Window_Property_AddonId
///                  _string_,
///     @return The id of the selected addon\, in `DialogAddonSettings.xml`.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link Window_Property_AddonId `Window.Property(Addon.ID)`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Window([window]).Property(key)`</b>,
///                  \anchor Window_Window_Property_key
///                  _string_,
///     @return A window property.
///     @param window - [opt] window id or name.
///     @param key - any value.
///     <p>
///   }
///   \table_row3{   <b>`Window(AddonBrowser).Property(Updated)`</b>,
///                  \anchor Window_Addonbrowser_Property_Updated
///                  _string_,
///     @return The date and time the addon repo was last checked for updates.
///     @todo move to a future window document.
///     <p><hr>
///     @skinning_v15 **[New Infolabel]** \link Window_Addonbrowser_Property_Updated `Window(AddonBrowser).Property(Updated)`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Window(Weather).Property(property)`</b>,
///                  \anchor Window_Weather_Property
///                  _string_,
///     @return The property for the weather window.
///     @param property - The requested property. The following are available:
///        - Current.ConditionIcon
///        - Day[0-6].OutlookIcon
///        - Current.FanartCode
///        - Day[0-6].FanartCode
///        - WeatherProviderLogo
///        - Daily.%i.OutlookIcon
///        - 36Hour.%i.OutlookIcon
///        - Weekend.%i.OutlookIcon
///        - Hourly.%i.OutlookIcon
///     @todo move to a future window document.
///     <p><hr>
///     @skinning_v16 **[Updated infolabel]** \link Window_Weather_Property `Window(Weather).Property(property)`\endlink
///     For skins that support extended weather info\, the following infolabels have been changed:
///       - Daily.%i.OutlookIcon
///       - 36Hour.%i.OutlookIcon
///       - Weekend.%i.OutlookIcon
///       - Hourly.%i.OutlookIcon
///     
///     previously the openweathermap addon would provide the full\, hardcoded path to the icon
///     ie. `resource://resource.images.weathericons.default/28.png`
///     to make it easier for skins to work with custom icon sets\, it now will return the filename only
///     i.e. 28.png
///     @skinning_v13 **[Infolabel Updated]** \link Window_Weather_Property `Window(Weather).Property(property)`\endlink
///     added `WeatherProviderLogo` property - weather provider logo (for weather addons that support it).
///     <p>
///   }
/// \table_end
///
/// -----------------------------------------------------------------------------
const infomap window_bools[] =   {{ "ismedia",          WINDOW_IS_MEDIA },
                                  { "is",               WINDOW_IS },
                                  { "isactive",         WINDOW_IS_ACTIVE },
                                  { "isvisible",        WINDOW_IS_VISIBLE },
                                  { "istopmost",        WINDOW_IS_DIALOG_TOPMOST }, //! @deprecated, remove in v19
                                  { "isdialogtopmost",  WINDOW_IS_DIALOG_TOPMOST },
                                  { "ismodaldialogtopmost", WINDOW_IS_MODAL_DIALOG_TOPMOST },
                                  { "previous",         WINDOW_PREVIOUS },
                                  { "next",             WINDOW_NEXT }};

/// \page modules__infolabels_boolean_conditions
/// \subsection modules__infolabels_boolean_conditions_Control Control
/// \table_start
///   \table_h3{ Labels, Type, Description }
///   \table_row3{   <b>`Control.HasFocus(id)`</b>,
///                  \anchor Control_HasFocus
///                  _boolean_,
///     @return **True** if the currently focused control has id "id".
///     @param id - The id of the control
///     <p>
///   }
///   \table_row3{   <b>`Control.IsVisible(id)`</b>,
///                  \anchor Control_IsVisible
///                  _boolean_,
///     @return **True** if the control with id "id" is visible.
///     @param id - The id of the control
///     <p>
///   }
///   \table_row3{   <b>`Control.IsEnabled(id)`</b>,
///                  \anchor Control_IsEnabled
///                  _boolean_,
///     @return **True** if the control with id "id" is enabled.
///     @param id - The id of the control
///     <p>
///   }
///   \table_row3{   <b>`Control.GetLabel(id)[.index()]`</b>,
///                  \anchor Control_GetLabel
///                  _string_,
///     @return The label value or texture name of the control with the given id.
///     @param id - The id of the control
///     @param index - [opt] Optionally you can specify index(1) to retrieve label2 from an Edit
///     control.
///     <p><hr>
///     @skinning_v15 **[Infolabel Updated]** \link Control_GetLabel `Control.GetLabel(id)`\endlink
///     added index parameter - allows skinner to retrieve label2 of a control. Only edit controls are supported.
///     ** Example** : `Control.GetLabel(999).index(1)` where:
///       - index(0) = label
///       - index(1) = label2
///     <p>
///   }
/// \table_end
///
/// -----------------------------------------------------------------------------
const infomap control_labels[] = {{ "hasfocus",         CONTROL_HAS_FOCUS },
                                  { "isvisible",        CONTROL_IS_VISIBLE },
                                  { "isenabled",        CONTROL_IS_ENABLED },
                                  { "getlabel",         CONTROL_GET_LABEL }};

/// \page modules__infolabels_boolean_conditions
/// \subsection modules__infolabels_boolean_conditions_Playlist Playlist
/// \table_start
///   \table_h3{ Labels, Type, Description }
///   \table_row3{   <b>`Playlist.Length(media)`</b>,
///                  \anchor Playlist_Length
///                  _integer_,
///     @return The total size of the current playlist.
///     @param media - [opt] mediatype with is either
///     video or music.
///     <p>
///   }
///   \table_row3{   <b>`Playlist.Position(media)`</b>,
///                  \anchor Playlist_Position
///                  _integer_,
///     @return The position of the current item in the current playlist.
///     @param media - [opt] mediatype with is either
///     video or music.
///     <p>
///   }
///   \table_row3{   <b>`Playlist.Random`</b>,
///                  \anchor Playlist_Random
///                  _integer_,
///     @return String ID for the random mode:
///       - **16041** (On)
///       - **591** (Off)
///     <p><hr>
///     @skinning_v18 **[Infolabel Updated]** \link Playlist_Random `Playlist.Random`\endlink will
///     now return **On/Off**
///     <p>
///   }
///   \table_row3{   <b>`Playlist.Repeat`</b>,
///                  \anchor Playlist_Repeat
///                  _integer_,
///     @return The String Id for the repeat mode. It can be one of the following
///     values:
///       - **592** (Repeat One)
///       - **593** (Repeat All)
///       - **594** (Repeat Off)
///     <p>
///   }
///   \table_row3{   <b>`Playlist.IsRandom`</b>,
///                  \anchor Playlist_IsRandom
///                  _boolean_,
///     @return **True** if the player is in random mode.
///     <p>
///   }
///   \table_row3{   <b>`Playlist.IsRepeat`</b>,
///                  \anchor Playlist_IsRepeat
///                  _boolean_,
///     @return **True** if the player is in repeat all mode.
///     <p>
///   }
///   \table_row3{   <b>`Playlist.IsRepeatOne`</b>,
///                  \anchor Playlist_IsRepeatOne
///                  _boolean_,
///     @return **True** if the player is in repeat one mode.
///     <p>
///   }
/// \table_end
///
/// -----------------------------------------------------------------------------
const infomap playlist[] =       {{ "length",           PLAYLIST_LENGTH },
                                  { "position",         PLAYLIST_POSITION },
                                  { "random",           PLAYLIST_RANDOM },
                                  { "repeat",           PLAYLIST_REPEAT },
                                  { "israndom",         PLAYLIST_ISRANDOM },
                                  { "isrepeat",         PLAYLIST_ISREPEAT },
                                  { "isrepeatone",      PLAYLIST_ISREPEATONE }};

/// \page modules__infolabels_boolean_conditions
/// \subsection modules__infolabels_boolean_conditions_Pvr Pvr
/// \table_start
///   \table_h3{ Labels, Type, Description }
///   \table_row3{   <b>`PVR.IsRecording`</b>,
///                  \anchor PVR_IsRecording
///                  _boolean_,
///     @return **True** when the system is recording a tv or radio programme.
///     <p>
///   }
///   \table_row3{   <b>`PVR.HasTimer`</b>,
///                  \anchor PVR_HasTimer
///                  _boolean_,
///     @return **True** when a recording timer is active.
///     <p>
///   }
///   \table_row3{   <b>`PVR.HasTVChannels`</b>,
///                  \anchor PVR_HasTVChannels
///                  _boolean_,
///     @return **True** if there are TV channels available.
///     <p>
///   }
///   \table_row3{   <b>`PVR.HasRadioChannels`</b>,
///                  \anchor PVR_HasRadioChannels
///                  _boolean_,
///     @return **True** if there are radio channels available.
///     <p>
///   }
///   \table_row3{   <b>`PVR.HasNonRecordingTimer`</b>,
///                  \anchor PVR_HasNonRecordingTimer
///                  _boolean_,
///     @return **True** if there are timers present who currently not do recording.
///     <p>
///   }
///   \table_row3{   <b>`PVR.BackendName`</b>,
///                  \anchor PVR_BackendName
///                  _string_,
///     @return The name of the backend being used.
///     <p>
///   }
///   \table_row3{   <b>`PVR.BackendVersion`</b>,
///                  \anchor PVR_BackendVersion
///                  _string_,
///     @return The version of the backend that's being used.
///     <p>
///   }
///   \table_row3{   <b>`PVR.BackendHost`</b>,
///                  \anchor PVR_BackendHost
///                  _string_,
///     @return The backend hostname.
///     <p>
///   }
///   \table_row3{   <b>`PVR.BackendDiskSpace`</b>,
///                  \anchor PVR_BackendDiskSpace
///                  _string_,
///     @return The available diskspace on the backend as string with size.
///     <p>
///   }
///   \table_row3{   <b>`PVR.BackendDiskSpaceProgr`</b>,
///                  \anchor PVR_BackendDiskSpaceProgr
///                  _integer_,
///     @return The available diskspace on the backend as percent value.
///     <p><hr>
///     @skinning_v14 **[New Infolabel]** \link PVR_BackendDiskSpaceProgr `PVR.BackendDiskSpaceProgr`\endlink
///     <p>
///   }
///   \table_row3{   <b>`PVR.BackendChannels`</b>,
///                  \anchor PVR_BackendChannels
///                  _string (integer)_,
///     @return The number of available channels the backend provides.
///     <p>
///   }
///   \table_row3{   <b>`PVR.BackendTimers`</b>,
///                  \anchor PVR_BackendTimers
///                  _string (integer)_,
///     @return The number of timers set for the backend.
///     <p>
///   }
///   \table_row3{   <b>`PVR.BackendRecordings`</b>,
///                  \anchor PVR_BackendRecordings
///                  _string (integer)_,
///     @return The number of recordings available on the backend.
///     <p>
///   }
///   \table_row3{   <b>`PVR.BackendDeletedRecordings`</b>,
///                  \anchor PVR_BackendDeletedRecordings
///                  _string (integer)_,
///     @return The number of deleted recordings present on the backend.
///     <p>
///   }
///   \table_row3{   <b>`PVR.BackendNumber`</b>,
///                  \anchor PVR_BackendNumber
///                  _string_,
///     @return The backend number.
///     <p>
///   }
///   \table_row3{   <b>`PVR.TotalDiscSpace`</b>,
///                  \anchor PVR_TotalDiscSpace
///                  _string_,
///     @return The total diskspace available for recordings.
///     <p>
///   }
///   \table_row3{   <b>`PVR.NextTimer`</b>,
///                  \anchor PVR_NextTimer
///                  _boolean_,
///     @return The next timer date.
///     <p>
///   }
///   \table_row3{   <b>`PVR.IsPlayingTV`</b>,
///                  \anchor PVR_IsPlayingTV
///                  _boolean_,
///     @return **True** when live tv is being watched.
///     <p>
///   }
///   \table_row3{   <b>`PVR.IsPlayingRadio`</b>,
///                  \anchor PVR_IsPlayingRadio
///                  _boolean_,
///     @return **True** when live radio is being listened to.
///     <p>
///   }
///   \table_row3{   <b>`PVR.IsPlayingRecording`</b>,
///                  \anchor PVR_IsPlayingRecording
///                  _boolean_,
///     @return **True** when a recording is being watched.
///     <p>
///   }
///   \table_row3{   <b>`PVR.IsPlayingEpgTag`</b>,
///                  \anchor PVR_IsPlayingEpgTag
///                  _boolean_,
///     @return **True** when an epg tag is being watched.
///     <p>
///   }
///   \table_row3{   <b>`PVR.EpgEventProgress`</b>,
///                  \anchor PVR_EpgEventProgress
///                  _integer_,
///     @return The percentage complete of the currently playing epg event.
///     <p><hr>
///     @skinning_v18 **[Infolabel Updated]** \link PVR_EpgEventProgress `PVR.EpgEventProgress`\endlink replaces
///     the old `PVR.Progress` infolabel.
///     <p>
///   }
///   \table_row3{   <b>`PVR.ActStreamClient`</b>,
///                  \anchor PVR_ActStreamClient
///                  _string_,
///     @return The stream client name.
///     <p>
///   }
///   \table_row3{   <b>`PVR.ActStreamDevice`</b>,
///                  \anchor PVR_ActStreamDevice
///                  _string_,
///     @return The stream device name.
///     <p>
///   }
///   \table_row3{   <b>`PVR.ActStreamStatus`</b>,
///                  \anchor PVR_ActStreamStatus
///                  _string_,
///     @return The status of the stream.
///     <p>
///   }
///   \table_row3{   <b>`PVR.ActStreamSignal`</b>,
///                  \anchor PVR_ActStreamSignal
///                  _string_,
///     @return The signal quality of the stream.
///     <p>
///   }
///   \table_row3{   <b>`PVR.ActStreamSnr`</b>,
///                  \anchor PVR_ActStreamSnr
///                  _string_,
///     @return The signal to noise ratio of the stream.
///     <p>
///   }
///   \table_row3{   <b>`PVR.ActStreamBer`</b>,
///                  \anchor PVR_ActStreamBer
///                  _string_,
///     @return The bit error rate of the stream.
///     <p>
///   }
///   \table_row3{   <b>`PVR.ActStreamUnc`</b>,
///                  \anchor PVR_ActStreamUnc
///                  _string_,
///     @return The UNC value of the stream.
///     <p>
///   }
///   \table_row3{   <b>`PVR.ActStreamProgrSignal`</b>,
///                  \anchor PVR_ActStreamProgrSignal
///                  _integer_,
///     @return The signal quality of the programme.
///     <p>
///   }
///   \table_row3{   <b>`PVR.ActStreamProgrSnr`</b>,
///                  \anchor PVR_ActStreamProgrSnr
///                  _integer_,
///     @return The signal to noise ratio of the programme.
///     <p>
///   }
///   \table_row3{   <b>`PVR.ActStreamIsEncrypted`</b>,
///                  \anchor PVR_ActStreamIsEncrypted
///                  _boolean_,
///     @return **True** when channel is encrypted on source.
///     <p>
///   }
///   \table_row3{   <b>`PVR.ActStreamEncryptionName`</b>,
///                  \anchor PVR_ActStreamEncryptionName
///                  _string_,
///     @return The encryption used on the stream.
///     <p>
///   }
///   \table_row3{   <b>`PVR.ActStreamServiceName`</b>,
///                  \anchor PVR_ActStreamServiceName
///                  _string_,
///     @return The service name of played channel if available.
///     <p>
///   }
///   \table_row3{   <b>`PVR.ActStreamMux`</b>,
///                  \anchor PVR_ActStreamMux
///                  _string_,
///     @return The multiplex type of played channel if available.
///     <p>
///   }
///   \table_row3{   <b>`PVR.ActStreamProviderName`</b>,
///                  \anchor PVR_ActStreamProviderName
///                  _string_,
///     @return The provider name of the played channel if available.
///     <p>
///   }
///   \table_row3{   <b>`PVR.IsTimeShift`</b>,
///                  \anchor PVR_IsTimeShift
///                  _boolean_,
///     @return **True** when for channel is timeshift available.
///     <p>
///   }
///   \table_row3{   <b>`PVR.TimeShiftProgress`</b>,
///                  \anchor PVR_TimeShiftProgress
///                  _integer_,
///     @return The position of currently timeshifted title on TV as integer.
///     <p>
///   }
///   \table_row3{   <b>`PVR.NowRecordingTitle`</b>,
///                  \anchor PVR_NowRecordingTitle
///                  _string_,
///     @return The title of the programme being recorded.
///     <p>
///   }
///   \table_row3{   <b>`PVR.NowRecordingDateTime`</b>,
///                  \anchor PVR_NowRecordingDateTime
///                  _Date/Time string_,
///     @return The start date and time of the current recording.
///     <p>
///   }
///   \table_row3{   <b>`PVR.NowRecordingChannel`</b>,
///                  \anchor PVR_NowRecordingChannel
///                  _string_,
///     @return The channel name of the current recording.
///     <p>
///   }
///   \table_row3{   <b>`PVR.NowRecordingChannelIcon`</b>,
///                  \anchor PVR_NowRecordingChannelIcon
///                  _string_,
///     @return The icon of the current recording channel.
///     <p>
///   }
///   \table_row3{   <b>`PVR.NextRecordingTitle`</b>,
///                  \anchor PVR_NextRecordingTitle
///                  _string_,
///     @return The title of the next programme that will be recorded.
///     <p>
///   }
///   \table_row3{   <b>`PVR.NextRecordingDateTime`</b>,
///                  \anchor PVR_NextRecordingDateTime
///                  _Date/Time string_,
///     @return The start date and time of the next recording.
///     <p>
///   }
///   \table_row3{   <b>`PVR.NextRecordingChannel`</b>,
///                  \anchor PVR_NextRecordingChannel
///                  _string_,
///     @return The channel name of the next recording.
///     <p>
///   }
///   \table_row3{   <b>`PVR.NextRecordingChannelIcon`</b>,
///                  \anchor PVR_NextRecordingChannelIcon
///                  _string_,
///     @return The icon of the next recording channel.
///     <p>
///   }
///   \table_row3{   <b>`PVR.TVNowRecordingTitle`</b>,
///                  \anchor PVR_TVNowRecordingTitle
///                  _string_,
///     @return The title of the tv programme being recorded.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link PVR_TVNowRecordingTitle `PVR.TVNowRecordingTitle`\endlink
///     <p>
///   }
///   \table_row3{   <b>`PVR.TVNowRecordingDateTime`</b>,
///                  \anchor PVR_TVNowRecordingDateTime
///                  _Date/Time string_,
///     @return The start date and time of the current tv recording.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link PVR_TVNowRecordingDateTime `PVR.TVNowRecordingDateTime`\endlink
///     <p>
///   }
///   \table_row3{   <b>`PVR.TVNowRecordingChannel`</b>,
///                  \anchor PVR_TVNowRecordingChannel
///                  _string_,
///     @return The channel name of the current tv recording.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link PVR_TVNowRecordingChannel `PVR.TVNowRecordingChannel`\endlink
///     <p>
///   }
///   \table_row3{   <b>`PVR.TVNowRecordingChannelIcon`</b>,
///                  \anchor PVR_TVNowRecordingChannelIcon
///                  _string_,
///     @return The icon of the current recording TV channel.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link PVR_TVNowRecordingChannelIcon `PVR.TVNowRecordingChannelIcon`\endlink
///     <p>
///   }
///   \table_row3{   <b>`PVR.TVNextRecordingTitle`</b>,
///                  \anchor PVR_TVNextRecordingTitle
///                  _string_,
///     @return The title of the next tv programme that will be recorded.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link PVR_TVNextRecordingTitle `PVR.TVNextRecordingTitle`\endlink
///     <p>
///   }
///   \table_row3{   <b>`PVR.TVNextRecordingDateTime`</b>,
///                  \anchor PVR_TVNextRecordingDateTime
///                  _Date/Time string_,
///     @return The start date and time of the next tv recording.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link PVR_TVNextRecordingDateTime `PVR.TVNextRecordingDateTime`\endlink
///     <p>
///   }
///   \table_row3{   <b>`PVR.TVNextRecordingChannel`</b>,
///                  \anchor PVR_TVNextRecordingChannel
///                  _string_,
///     @return The channel name of the next tv recording.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link PVR_TVNextRecordingChannel `PVR.TVNextRecordingChannel`\endlink
///     <p>
///   }
///   \table_row3{   <b>`PVR.TVNextRecordingChannelIcon`</b>,
///                  \anchor PVR_TVNextRecordingChannelIcon
///                  _string_,
///     @return The icon of the next recording tv channel.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link PVR_TVNextRecordingChannelIcon `PVR.TVNextRecordingChannelIcon`\endlink
///     <p>
///   }
///   \table_row3{   <b>`PVR.RadioNowRecordingTitle`</b>,
///                  \anchor PVR_RadioNowRecordingTitle
///                  _string_,
///     @return The title of the radio programme being recorded.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link PVR_RadioNowRecordingTitle `PVR.RadioNowRecordingTitle`\endlink
///     <p>
///   }
///   \table_row3{   <b>`PVR.RadioNowRecordingDateTime`</b>,
///                  \anchor PVR_RadioNowRecordingDateTime
///                  _Date/Time string_,
///     @return The start date and time of the current radio recording.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link PVR_RadioNowRecordingDateTime `PVR.RadioNowRecordingDateTime`\endlink
///     <p>
///   }
///   \table_row3{   <b>`PVR.RadioNowRecordingChannel`</b>,
///                  \anchor PVR_RadioNowRecordingChannel
///                  _string_,
///     @return The channel name of the current radio recording.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link PVR_RadioNowRecordingChannel `PVR.RadioNowRecordingChannel`\endlink
///     <p>
///   }
///   \table_row3{   <b>`PVR.RadioNowRecordingChannelIcon`</b>,
///                  \anchor PVR_RadioNowRecordingChannelIcon
///                  _string_,
///     @return The icon of the current recording radio channel.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link PVR_RadioNowRecordingChannelIcon `PVR.RadioNowRecordingChannelIcon`\endlink
///     <p>
///   }
///   \table_row3{   <b>`PVR.RadioNextRecordingTitle`</b>,
///                  \anchor PVR_RadioNextRecordingTitle
///                  _string_,
///     @return The title of the next radio programme that will be recorded.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link PVR_RadioNextRecordingTitle `PVR.RadioNextRecordingTitle`\endlink
///     <p>
///   }
///   \table_row3{   <b>`PVR.RadioNextRecordingDateTime`</b>,
///                  \anchor PVR_RadioNextRecordingDateTime
///                  _Date/Time string_,
///     @return The start date and time of the next radio recording.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link PVR_RadioNextRecordingDateTime `PVR.RadioNextRecordingDateTime`\endlink
///     <p>
///   }
///   \table_row3{   <b>`PVR.RadioNextRecordingChannel`</b>,
///                  \anchor PVR_RadioNextRecordingChannel
///                  _string_,
///     @return The channel name of the next radio recording.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link PVR_RadioNextRecordingChannel `PVR.RadioNextRecordingChannel`\endlink
///     <p>
///   }
///   \table_row3{   <b>`PVR.RadioNextRecordingChannelIcon`</b>,
///                  \anchor PVR_RadioNextRecordingChannelIcon
///                  _string_,
///     @return The icon of the next recording radio channel.
///     <p><hr>
///     @skinning_v17 **[New Infolabel]** \link PVR_RadioNextRecordingChannelIcon `PVR.RadioNextRecordingChannelIcon`\endlink
///     <p>
///   }
///   \table_row3{   <b>`PVR.IsRecordingTV`</b>,
///                  \anchor PVR_IsRecordingTV
///                  _boolean_,
///     @return **True** when the system is recording a tv programme.
///     <p><hr>
///     @skinning_v17 **[New Boolean Condition]** \link PVR_IsRecordingTV `PVR.IsRecordingTV`\endlink
///     <p>
///   }
///   \table_row3{   <b>`PVR.HasTVTimer`</b>,
///                  \anchor PVR_HasTVTimer
///                  _boolean_,
///     @return **True** if at least one tv timer is active.
///     <p><hr>
///     @skinning_v17 **[New Boolean Condition]** \link PVR_HasTVTimer `PVR.HasTVTimer`\endlink
///     <p>
///   }
///   \table_row3{   <b>`PVR.HasNonRecordingTVTimer`</b>,
///                  \anchor PVR_HasNonRecordingTVTimer
///                  _boolean_,
///     @return **True** if there are tv timers present who currently not do recording.
///     <p><hr>
///     @skinning_v17 **[New Boolean Condition]** \link PVR_HasNonRecordingTVTimer `PVR.HasNonRecordingTVTimer`\endlink
///     <p>
///   }
///   \table_row3{   <b>`PVR.IsRecordingRadio`</b>,
///                  \anchor PVR_IsRecordingRadio
///                  _boolean_,
///     @return **True** when the system is recording a radio programme.
///     <p><hr>
///     @skinning_v17 **[New Boolean Condition]** \link PVR_IsRecordingRadio `PVR.IsRecordingRadio`\endlink
///     <p>
///   }
///   \table_row3{   <b>`PVR.HasRadioTimer`</b>,
///                  \anchor PVR_HasRadioTimer
///                  _boolean_,
///     @return **True** if at least one radio timer is active.
///     <p><hr>
///     @skinning_v17 **[New Boolean Condition]** \link PVR_HasRadioTimer `PVR.HasRadioTimer`\endlink
///     <p>
///   }
///   \table_row3{   <b>`PVR.HasNonRecordingRadioTimer`</b>,
///                  \anchor PVR_HasNonRecordingRadioTimer
///                  _boolean_,
///     @return **True** if there are radio timers present who currently not do recording.
///     <p><hr>
///     @skinning_v17 **[New Boolean Condition]** \link PVR_HasNonRecordingRadioTimer `PVR.HasRadioTimer`\endlink
///     <p>
///   }
///   \table_row3{   <b>`PVR.ChannelNumberInput`</b>,
///                  \anchor PVR_ChannelNumberInput
///                  _string_,
///     @return The currently entered channel number while in numeric channel input mode\, an empty string otherwise.
///     <p><hr>
///     @skinning_v18 **[New Infolabel]** \link PVR_ChannelNumberInput `PVR.ChannelNumberInput`\endlink
///     <p>  
///   }
///   \table_row3{   <b>`PVR.CanRecordPlayingChannel`</b>,
///                  \anchor PVR_CanRecordPlayingChannel
///                  _boolean_,
///     @return **True** if PVR is currently playing a channel and if this channel can be recorded.
///     <p><hr>
///     @skinning_v18 **[Infolabel Updated]** \link PVR_CanRecordPlayingChannel `PVR.CanRecordPlayingChannel`\endlink replaces
///     the old `Player.CanRecord` infolabel.
///     <p>
///   }
///   \table_row3{   <b>`PVR.IsRecordingPlayingChannel`</b>,
///                  \anchor PVR_IsRecordingPlayingChannel
///                  _boolean_,
///     @return **True** if PVR is currently playing a channel and if this channel is currently recorded.
///     <p><hr>
///     @skinning_v18 **[Infolabel Updated]** \link PVR_IsRecordingPlayingChannel `PVR.IsRecordingPlayingChannel`\endlink replaces
///     the old `Player.Recording` infolabel.
///     <p>
///   }
///   \table_row3{   <b>`PVR.TimeshiftProgressPlayPos`</b>,
///                  \anchor PVR_TimeshiftProgressPlayPos
///                  _integer_,
///     @return The percentage of the current play position within the PVR timeshift progress.
///     <p><hr>
///     @skinning_v18 **[New Infolabel]** \link PVR_TimeshiftProgressPlayPos `PVR.TimeshiftProgressPlayPos`\endlink
///     <p>  
///   }
///   \table_row3{   <b>`PVR.TimeshiftProgressEpgStart`</b>,
///                  \anchor PVR_TimeshiftProgressEpgStart
///                  _integer_,
///     @return The percentage of the start of the currently playing epg event within the PVR timeshift progress.
///     <p><hr>
///     @skinning_v18 **[New Infolabel]** \link PVR_TimeshiftProgressEpgStart `PVR.TimeshiftProgressEpgStart`\endlink
///     <p>
///   }
///   \table_row3{   <b>`PVR.TimeshiftProgressEpgEnd`</b>,
///                  \anchor PVR_TimeshiftProgressEpgEnd
///                  _integer_,
///     @return The percentage of the end of the currently playing epg event within the PVR timeshift progress.
///     <p><hr>
///     @skinning_v18 **[New Infolabel]** \link PVR_TimeshiftProgressEpgEnd `PVR.TimeshiftProgressEpgEnd`\endlink
///     <p>
///   }
///   \table_row3{   <b>`PVR.TimeshiftProgressBufferStart`</b>,
///                  \anchor PVR_TimeshiftProgressBufferStart
///                  _integer_,
///     @return The percentage of the start of the timeshift buffer within the PVR timeshift progress.
///     <p><hr>
///     @skinning_v18 **[New Infolabel]** \link PVR_TimeshiftProgressBufferStart `PVR.TimeshiftProgressBufferStart`\endlink
///     <p>
///   }
///   \table_row3{   <b>`PVR.TimeshiftProgressBufferEnd`</b>,
///                  \anchor PVR_TimeshiftProgressBufferEnd
///                  _integer_,
///     @return The percentage of the end of the timeshift buffer within the PVR timeshift progress.
///     <p><hr>
///     @skinning_v18 **[New Infolabel]** \link PVR_TimeshiftProgressBufferEnd `PVR.TimeshiftProgressBufferEnd`\endlink
///     <p>
///   }
///   \table_row3{   <b>`PVR.EpgEventIcon`</b>,
///                  \anchor PVR_EpgEventIcon
///                  _string_,
///     @return The icon of the currently playing epg event\, if any.
///     <p><hr>
///     @skinning_v18 **[New Infolabel]** \link PVR_EpgEventIcon `PVR_EpgEventIcon`\endlink
///     <p>
///   }
///
const infomap pvr[] =            {{ "isrecording",              PVR_IS_RECORDING },
                                  { "hastimer",                 PVR_HAS_TIMER },
                                  { "hastvchannels",            PVR_HAS_TV_CHANNELS },
                                  { "hasradiochannels",         PVR_HAS_RADIO_CHANNELS },
                                  { "hasnonrecordingtimer",     PVR_HAS_NONRECORDING_TIMER },
                                  { "backendname",              PVR_BACKEND_NAME },
                                  { "backendversion",           PVR_BACKEND_VERSION },
                                  { "backendhost",              PVR_BACKEND_HOST },
                                  { "backenddiskspace",         PVR_BACKEND_DISKSPACE },
                                  { "backenddiskspaceprogr",    PVR_BACKEND_DISKSPACE_PROGR },
                                  { "backendchannels",          PVR_BACKEND_CHANNELS },
                                  { "backendtimers",            PVR_BACKEND_TIMERS },
                                  { "backendrecordings",        PVR_BACKEND_RECORDINGS },
                                  { "backenddeletedrecordings", PVR_BACKEND_DELETED_RECORDINGS },
                                  { "backendnumber",            PVR_BACKEND_NUMBER },
                                  { "totaldiscspace",           PVR_TOTAL_DISKSPACE },
                                  { "nexttimer",                PVR_NEXT_TIMER },
                                  { "isplayingtv",              PVR_IS_PLAYING_TV },
                                  { "isplayingradio",           PVR_IS_PLAYING_RADIO },
                                  { "isplayingrecording",       PVR_IS_PLAYING_RECORDING },
                                  { "isplayingepgtag",          PVR_IS_PLAYING_EPGTAG },
                                  { "epgeventprogress",         PVR_EPG_EVENT_PROGRESS },
                                  { "actstreamclient",          PVR_ACTUAL_STREAM_CLIENT },
                                  { "actstreamdevice",          PVR_ACTUAL_STREAM_DEVICE },
                                  { "actstreamstatus",          PVR_ACTUAL_STREAM_STATUS },
                                  { "actstreamsignal",          PVR_ACTUAL_STREAM_SIG },
                                  { "actstreamsnr",             PVR_ACTUAL_STREAM_SNR },
                                  { "actstreamber",             PVR_ACTUAL_STREAM_BER },
                                  { "actstreamunc",             PVR_ACTUAL_STREAM_UNC },
                                  { "actstreamprogrsignal",     PVR_ACTUAL_STREAM_SIG_PROGR },
                                  { "actstreamprogrsnr",        PVR_ACTUAL_STREAM_SNR_PROGR },
                                  { "actstreamisencrypted",     PVR_ACTUAL_STREAM_ENCRYPTED },
                                  { "actstreamencryptionname",  PVR_ACTUAL_STREAM_CRYPTION },
                                  { "actstreamservicename",     PVR_ACTUAL_STREAM_SERVICE },
                                  { "actstreammux",             PVR_ACTUAL_STREAM_MUX },
                                  { "actstreamprovidername",    PVR_ACTUAL_STREAM_PROVIDER },
                                  { "istimeshift",              PVR_IS_TIMESHIFTING },
                                  { "timeshiftprogress",        PVR_TIMESHIFT_PROGRESS },
                                  { "nowrecordingtitle",        PVR_NOW_RECORDING_TITLE },
                                  { "nowrecordingdatetime",     PVR_NOW_RECORDING_DATETIME },
                                  { "nowrecordingchannel",      PVR_NOW_RECORDING_CHANNEL },
                                  { "nowrecordingchannelicon",  PVR_NOW_RECORDING_CHAN_ICO },
                                  { "nextrecordingtitle",       PVR_NEXT_RECORDING_TITLE },
                                  { "nextrecordingdatetime",    PVR_NEXT_RECORDING_DATETIME },
                                  { "nextrecordingchannel",     PVR_NEXT_RECORDING_CHANNEL },
                                  { "nextrecordingchannelicon", PVR_NEXT_RECORDING_CHAN_ICO },
                                  { "tvnowrecordingtitle",            PVR_TV_NOW_RECORDING_TITLE },
                                  { "tvnowrecordingdatetime",         PVR_TV_NOW_RECORDING_DATETIME },
                                  { "tvnowrecordingchannel",          PVR_TV_NOW_RECORDING_CHANNEL },
                                  { "tvnowrecordingchannelicon",      PVR_TV_NOW_RECORDING_CHAN_ICO },
                                  { "tvnextrecordingtitle",           PVR_TV_NEXT_RECORDING_TITLE },
                                  { "tvnextrecordingdatetime",        PVR_TV_NEXT_RECORDING_DATETIME },
                                  { "tvnextrecordingchannel",         PVR_TV_NEXT_RECORDING_CHANNEL },
                                  { "tvnextrecordingchannelicon",     PVR_TV_NEXT_RECORDING_CHAN_ICO },
                                  { "radionowrecordingtitle",         PVR_RADIO_NOW_RECORDING_TITLE },
                                  { "radionowrecordingdatetime",      PVR_RADIO_NOW_RECORDING_DATETIME },
                                  { "radionowrecordingchannel",       PVR_RADIO_NOW_RECORDING_CHANNEL },
                                  { "radionowrecordingchannelicon",   PVR_RADIO_NOW_RECORDING_CHAN_ICO },
                                  { "radionextrecordingtitle",        PVR_RADIO_NEXT_RECORDING_TITLE },
                                  { "radionextrecordingdatetime",     PVR_RADIO_NEXT_RECORDING_DATETIME },
                                  { "radionextrecordingchannel",      PVR_RADIO_NEXT_RECORDING_CHANNEL },
                                  { "radionextrecordingchannelicon",  PVR_RADIO_NEXT_RECORDING_CHAN_ICO },
                                  { "isrecordingtv",              PVR_IS_RECORDING_TV },
                                  { "hastvtimer",                 PVR_HAS_TV_TIMER },
                                  { "hasnonrecordingtvtimer",     PVR_HAS_NONRECORDING_TV_TIMER },
                                  { "isrecordingradio",           PVR_IS_RECORDING_RADIO },
                                  { "hasradiotimer",              PVR_HAS_RADIO_TIMER },
                                  { "hasnonrecordingradiotimer",  PVR_HAS_NONRECORDING_RADIO_TIMER },
                                  { "channelnumberinput",         PVR_CHANNEL_NUMBER_INPUT },
                                  { "canrecordplayingchannel",    PVR_CAN_RECORD_PLAYING_CHANNEL },
                                  { "isrecordingplayingchannel",  PVR_IS_RECORDING_PLAYING_CHANNEL },
                                  { "timeshiftprogressplaypos",   PVR_TIMESHIFT_PROGRESS_PLAY_POS },
                                  { "timeshiftprogressepgstart",  PVR_TIMESHIFT_PROGRESS_EPG_START },
                                  { "timeshiftprogressepgend",    PVR_TIMESHIFT_PROGRESS_EPG_END },
                                  { "timeshiftprogressbufferstart", PVR_TIMESHIFT_PROGRESS_BUFFER_START },
                                  { "timeshiftprogressbufferend", PVR_TIMESHIFT_PROGRESS_BUFFER_END },
                                  { "epgeventicon",               PVR_EPG_EVENT_ICON }};

/// \page modules__infolabels_boolean_conditions
///   \table_row3{   <b>`PVR.EpgEventDuration`</b>,
///                  \anchor PVR_EpgEventDuration
///                  _string_,
///     @return The duration of the currently playing epg event in the
///     format <b>hh:mm:ss</b>.
///     @note <b>hh:</b> will be omitted if hours value is zero.
///     <p><hr>
///     @skinning_v18 **[Infolabel Updated]** \link PVR_EpgEventDuration `PVR.EpgEventDuration`\endlink replaces
///     the old `PVR.Duration` infolabel.
///     <p>
///   }
///   \table_row3{   <b>`PVR.EpgEventDuration(format)`</b>,
///                  \anchor PVR_EpgEventDuration_format
///                  _string_,
///     @return The duration of the currently playing EPG event in different formats.
///     @param format [opt] The format of the return time value.
///     See \ref TIME_FORMAT for the list of possible values.
///     <p>
///   }
///   \table_row3{   <b>`PVR.EpgEventElapsedTime`</b>,
///                  \anchor PVR_EpgEventElapsedTime
///                  _string_,
///     @return the time of the current position of the currently playing epg event in the
///     format <b>hh:mm:ss</b>.
///     @note <b>hh:</b> will be omitted if hours value is zero.
///     <p><hr>
///     @skinning_v18 **[Infolabel Updated]** \link PVR_EpgEventElapsedTime `PVR.EpgEventElapsedTime`\endlink replaces
///     the old `PVR.Time` infolabel.
///     <p>
///   }
///   \table_row3{   <b>`PVR.EpgEventElapsedTime(format)`</b>,
///                  \anchor PVR_EpgEventElapsedTime_format
///                  _string_,
///     @return The time of the current position of the currently playing epg event in different formats.
///     @param format [opt] The format of the return time value.
///     See \ref TIME_FORMAT for the list of possible values.
///     <p>
///   }
///   \table_row3{   <b>`PVR.EpgEventRemainingTime`</b>,
///                  \anchor PVR_EpgEventRemainingTime
///                  _string_,
///     @return The remaining time for currently playing epg event in the
///     format <b>hh:mm:ss</b>.
///     @note <b>hh:</b> will be omitted if hours value is zero.
///     <p><hr>
///     @skinning_v18 **[New Infolabel]** \link PVR_EpgEventRemainingTime `PVR.EpgEventRemainingTime`\endlink
///     <p>  
///   }
///   \table_row3{   <b>`PVR.EpgEventRemainingTime(format)`</b>,
///                  \anchor PVR_EpgEventRemainingTime_format
///                  _string_,
///     @return The remaining time for currently playing epg event in different formats.
///     @param format [opt] The format of the return time value.
///     See \ref TIME_FORMAT for the list of possible values.
///     <p>
///   }
///   \table_row3{   <b>`PVR.EpgEventSeekTime`</b>,
///                  \anchor PVR_EpgEventSeekTime
///                  _string_,
///     @return The time the user is seeking within the currently playing epg event in the
///     format <b>hh:mm:ss</b>.
///     @note <b>hh:</b> will be omitted if hours value is zero.
///     <p><hr>
///     @skinning_v18 **[New Infolabel]** \link PVR_EpgEventSeekTime `PVR.EpgEventSeekTime`\endlink
///     <p>  
///   }
///   \table_row3{   <b>`PVR.EpgEventSeekTime(format)`</b>,
///                  \anchor PVR_EpgEventSeekTime_format
///                  _string_,
///     @return The time the user is seeking within the currently playing epg event in different formats.
///     @param format [opt] The format of the return time value.
///     See \ref TIME_FORMAT for the list of possible values.
///     <p>
///   }
///   \table_row3{   <b>`PVR.EpgEventFinishTime`</b>,
///                  \anchor PVR_EpgEventFinishTime
///                  _string_,
///     @return The time the currently playing epg event will end in the
///     format <b>hh:mm:ss</b>.
///     @note <b>hh:</b> will be omitted if hours value is zero.
///     <p><hr>
///     @skinning_v18 **[New Infolabel]** \link PVR_EpgEventFinishTime `PVR.EpgEventFinishTime`\endlink
///     <p>  
///   }
///   \table_row3{   <b>`PVR.EpgEventFinishTime(format)`</b>,
///                  \anchor PVR_EpgEventFinishTime_format
///                  _string_,
///     Returns the time the currently playing epg event will end in different formats.
///     @param format [opt] The format of the return time value.
///     See \ref TIME_FORMAT for the list of possible values.
///     <p>
///   }
///   \table_row3{   <b>`PVR.TimeShiftStart`</b>,
///                  \anchor PVR_TimeShiftStart
///                  _string_,
///     @return The start time of the timeshift buffer in the
///     format <b>hh:mm:ss</b>.
///     @note <b>hh:</b> will be omitted if hours value is zero.
///     <p>
///   }
///   \table_row3{   <b>`PVR.TimeShiftStart(format)`</b>,
///                  \anchor PVR_TimeShiftStart_format
///                  _string_,
///     Returns the start time of the timeshift buffer in different formats.
///     @param format [opt] The format of the return time value.
///     See \ref TIME_FORMAT for the list of possible values.
///     <p>
///   }
///   \table_row3{   <b>`PVR.TimeShiftEnd`</b>,
///                  \anchor PVR_TimeShiftEnd
///                  _string_,
///     @return The end time of the timeshift buffer in the
///     format <b>hh:mm:ss</b>.
///     @note <b>hh:</b> will be omitted if hours value is zero.
///     <p>
///   }
///   \table_row3{   <b>`PVR.TimeShiftEnd(format)`</b>,
///                  \anchor PVR_TimeShiftEnd_format
///                  _string_,
///     @return The end time of the timeshift buffer in different formats.
///     @param format [opt] The format of the return time value.
///     See \ref TIME_FORMAT for the list of possible values.
///     <p>
///   }
///   \table_row3{   <b>`PVR.TimeShiftCur`</b>,
///                  \anchor PVR_TimeShiftCur
///                  _string_,
///     @return The current playback time within the timeshift buffer in the
///     format <b>hh:mm:ss</b>.
///     @note <b>hh:</b> will be omitted if hours value is zero.
///     <p>
///   }
///   \table_row3{   <b>`PVR.TimeShiftCur(format)`</b>,
///                  \anchor PVR_TimeShiftCur_format
///                  _string_,
///     Returns the current playback time within the timeshift buffer in different formats.
///     @param format [opt] The format of the return time value.
///     See \ref TIME_FORMAT for the list of possible values.
///     <p>
///   }
///   \table_row3{   <b>`PVR.TimeShiftOffset`</b>,
///                  \anchor PVR_TimeShiftOffset
///                  _string_,
///     @return The delta of timeshifted time to actual time in the
///     format <b>hh:mm:ss</b>.
///     @note <b>hh:</b> will be omitted if hours value is zero.
///     <p>
///   }
///   \table_row3{   <b>`PVR.TimeShiftOffset(format)`</b>,
///                  \anchor PVR_TimeShiftOffset_format
///                  _string_,
///     Returns the delta of timeshifted time to actual time in different formats.
///     @param format [opt] The format of the return time value.
///     See \ref TIME_FORMAT for the list of possible values.
///     <p>
///   }
///   \table_row3{   <b>`PVR.TimeshiftProgressDuration`</b>,
///                  \anchor PVR_TimeshiftProgressDuration
///                  _string_,
///     @return the duration of the PVR timeshift progress in the
///     format <b>hh:mm:ss</b>.
///     @note <b>hh:</b> will be omitted if hours value is zero.
///     <p><hr>
///     @skinning_v18 **[New Infolabel]** \link PVR_TimeshiftProgressDuration `PVR.TimeshiftProgressDuration`\endlink
///     <p>  
///   }
///   \table_row3{   <b>`PVR.TimeshiftProgressDuration(format)`</b>,
///                  \anchor PVR_TimeshiftProgressDuration_format
///                  _string_,
///     @return The duration of the PVR timeshift progress in different formats.
///     @param format [opt] The format of the return time value.
///     See \ref TIME_FORMAT for the list of possible values.
///     <p>
///   }
///   \table_row3{   <b>`PVR.TimeshiftProgressStartTime`</b>,
///                  \anchor PVR_TimeshiftProgressStartTime
///                  _string_,
///     @return The start time of the PVR timeshift progress in the
///     format <b>hh:mm:ss</b>.
///     @note <b>hh:</b> will be omitted if hours value is zero.
///     <p><hr>
///     @skinning_v18 **[New Infolabel]** \link PVR_TimeshiftProgressStartTime `PVR.TimeshiftProgressStartTime`\endlink
///     <p>  
///   }
///   \table_row3{   <b>`PVR.TimeshiftProgressStartTime(format)`</b>,
///                  \anchor PVR_TimeshiftProgressStartTime_format
///                  _string_,
///     @return The start time of the PVR timeshift progress in different formats.
///     @param format [opt] The format of the return time value.
///     See \ref TIME_FORMAT for the list of possible values.
///     <p>
///   }
///   \table_row3{   <b>`PVR.TimeshiftProgressEndTime`</b>,
///                  \anchor PVR_TimeshiftProgressEndTime
///                  _string_,
///     @return The end time of the PVR timeshift progress in the
///     format <b>hh:mm:ss</b>.
///     @note hh: will be omitted if hours value is zero.
///     <p><hr>
///     @skinning_v18 **[New Infolabel]** \link PVR_TimeshiftProgressEndTime `PVR.TimeshiftProgressEndTime`\endlink
///     <p>  
///   }
///   \table_row3{   <b>`PVR.TimeshiftProgressEndTime(format)`</b>,
///                  \anchor PVR_TimeshiftProgressEndTime_format
///                  _string_,
///     @return The end time of the PVR timeshift progress in different formats.
///     @param format [opt] The format of the return time value.
///     See \ref TIME_FORMAT for the list of possible values.
///     <p>
///   }
/// \table_end
///
/// -----------------------------------------------------------------------------
const infomap pvr_times[] =      {{ "epgeventduration",       PVR_EPG_EVENT_DURATION },
                                  { "epgeventelapsedtime",    PVR_EPG_EVENT_ELAPSED_TIME },
                                  { "epgeventremainingtime",  PVR_EPG_EVENT_REMAINING_TIME },
                                  { "epgeventfinishtime",     PVR_EPG_EVENT_FINISH_TIME },
                                  { "epgeventseektime",       PVR_EPG_EVENT_SEEK_TIME },
                                  { "timeshiftstart",         PVR_TIMESHIFT_START_TIME },
                                  { "timeshiftend",           PVR_TIMESHIFT_END_TIME },
                                  { "timeshiftcur",           PVR_TIMESHIFT_PLAY_TIME },
                                  { "timeshiftoffset",        PVR_TIMESHIFT_OFFSET },
                                  { "timeshiftprogressduration",  PVR_TIMESHIFT_PROGRESS_DURATION },
                                  { "timeshiftprogressstarttime", PVR_TIMESHIFT_PROGRESS_START_TIME },
                                  { "timeshiftprogressendtime",   PVR_TIMESHIFT_PROGRESS_END_TIME }};

/// \page modules__infolabels_boolean_conditions
/// \subsection modules__infolabels_boolean_conditions_RDS RDS
/// @note Only supported if both the PVR backend and the Kodi client support RDS.
/// 
/// \table_start
///   \table_h3{ Labels, Type, Description }
///   \table_row3{   <b>`RDS.HasRds`</b>,
///                  \anchor RDS_HasRds
///                  _boolean_,
///     @return **True** if RDS is present.
///     <p><hr>
///     @skinning_v16 **[New Boolean Condition]** \link RDS_HasRds `RDS.HasRds`\endlink
///     <p>
///   }
///   \table_row3{   <b>`RDS.HasRadioText`</b>,
///                  \anchor RDS_HasRadioText
///                  _boolean_,
///     @return **True** if RDS contains also Radiotext.
///     <p><hr>
///     @skinning_v16 **[New Boolean Condition]** \link RDS_HasRadioText `RDS.HasRadioText`\endlink
///     <p>
///   }
///   \table_row3{   <b>`RDS.HasRadioTextPlus`</b>,
///                  \anchor RDS_HasRadioTextPlus
///                  _boolean_,
///     @return **True** if RDS with Radiotext contains also the plus information.
///     <p><hr>
///     @skinning_v16 **[New Boolean Condition]** \link RDS_HasRadioTextPlus `RDS.HasRadioTextPlus`\endlink
///     <p>
///   }
///   \table_row3{   <b>`RDS.HasHotline`</b>,
///                  \anchor RDS_HasHotline
///                  _boolean_,
///     @return **True** if a hotline phone number is present.
///     @note Only available on RadiotextPlus
///     <p><hr>
///     @skinning_v16 **[New Boolean Condition]** \link RDS_HasHotline `RDS.HasHotline`\endlink
///     <p>
///   }
///   \table_row3{   <b>`RDS.HasStudio`</b>,
///                  \anchor RDS_HasStudio
///                  _boolean_,
///     @return **True** if a studio name is present.
///     @note Only available on RadiotextPlus
///     <p><hr>
///     @skinning_v16 **[New Boolean Condition]** \link RDS_HasStudio `RDS.HasStudio`\endlink
///     <p>
///   }
///   \table_row3{   <b>`RDS.AudioLanguage`</b>,
///                  \anchor RDS_AudioLanguage
///                  _string_,
///     @return The RDS reported audio language of the channel.
///     <p><hr>
///     @skinning_v16 **[New Infolabel]** \link RDS_AudioLanguage `RDS.AudioLanguage`\endlink
///     <p>
///   }
///   \table_row3{   <b>`RDS.ChannelCountry`</b>,
///                  \anchor RDS_ChannelCountry
///                  _string_,
///     @return The country where the radio channel is broadcasted.
///     <p><hr>
///     @skinning_v16 **[New Infolabel]** \link RDS_ChannelCountry `RDS.ChannelCountry`\endlink
///     <p>
///   }
///   \table_row3{   <b>`RDS.GetLine(number)`</b>,
///                  \anchor RDS_GetLine
///                  _string_,
///     @return The last sent RDS text messages on given number.
///     @param number - given number for RDS\, 0 is the
///     last and 4 rows are supported (0-3)
///     <p><hr>
///     @skinning_v16 **[New Infolabel]** \link RDS_GetLine `RDS.GetLine(number)`\endlink
///     <p>
///   }
///   \table_row3{   <b>`RDS.Title`</b>,
///                  \anchor RDS_Title
///                  _string_,
///     @return The title of item; e.g. track title of an album.
///     @note Only available on RadiotextPlus
///     <p><hr>
///     @skinning_v16 **[New Infolabel]** \link RDS_Title `RDS.Title`\endlink
///     <p>
///   }
///   \table_row3{   <b>`RDS.Artist`</b>,
///                  \anchor RDS_Artist
///                  _string_,
///     @return A person or band/collective generally considered responsible for the work.
///     @note Only available on RadiotextPlus
///     <p><hr>
///     @skinning_v16 **[New Infolabel]** \link RDS_Artist `RDS.Artist`\endlink
///     <p>
///   }
///   \table_row3{   <b>`RDS.Band`</b>,
///                  \anchor RDS_Band
///                  _string_,
///     @return The band/orchestra/musician.
///     @note Only available on RadiotextPlus
///     <p><hr>
///     @skinning_v16 **[New Infolabel]** \link RDS_Band `RDS.Band`\endlink
///     <p>
///   }
///   \table_row3{   <b>`RDS.Composer`</b>,
///                  \anchor RDS_Composer
///                  _string_,
///     @return The name of the original composer/author.
///     @note Only available on RadiotextPlus
///     <p><hr>
///     @skinning_v16 **[New Infolabel]** \link RDS_Composer `RDS.Composer`\endlink
///     <p>
///   }
///   \table_row3{   <b>`RDS.Conductor`</b>,
///                  \anchor RDS_Conductor
///                  _string_,
///     @return The artist(s) who performed the work. In classical music this would be
///     the conductor.
///     @note Only available on RadiotextPlus
///     <p><hr>
///     @skinning_v16 **[New Infolabel]** \link RDS_Conductor `RDS.Conductor`\endlink
///     <p>
///   }
///   \table_row3{   <b>`RDS.Album`</b>,
///                  \anchor RDS_Album
///                  _string_,
///     @return The album of the song.
///     @note Only available on RadiotextPlus
///     <p><hr>
///     @skinning_v16 **[New Infolabel]** \link RDS_Album `RDS.Album`\endlink
///     <p>
///   }
///   \table_row3{   <b>`RDS.TrackNumber`</b>,
///                  \anchor RDS_TrackNumber
///                  _string_,
///     @return The track number of the item on the album on which it was originally
///     released.
///     @note Only be available on RadiotextPlus
///     <p><hr>
///     @skinning_v16 **[New Infolabel]** \link RDS_TrackNumber `RDS.TrackNumber`\endlink
///     <p>
///   }
///   \table_row3{   <b>`RDS.RadioStyle`</b>,
///                  \anchor RDS_RadioStyle
///                  _string_,
///     @return The style of current played radio channel\, it is always
///     updated once the style changes\, e.g "popmusic" to "news" or "weather"...
///     | RDS                     | RBDS                    |
///     |:------------------------|:------------------------|
///     | none                    | none                    |
///     | news                    | news                    |
///     | currentaffairs          | information             |
///     | information             | sport                   |
///     | sport                   | talk                    |
///     | education               | rockmusic               |
///     | drama                   | classicrockmusic        |
///     | cultures                | adulthits               |
///     | science                 | softrock                |
///     | variedspeech            | top40                   |
///     | popmusic                | countrymusic            |
///     | rockmusic               | oldiesmusic             |
///     | easylistening           | softmusic               |
///     | lightclassics           | nostalgia               |
///     | seriousclassics         | jazzmusic               |
///     | othermusic              | classical               |
///     | weather                 | randb                   |
///     | finance                 | softrandb               |
///     | childrensprogs          | language                |
///     | socialaffairs           | religiousmusic          |
///     | religion                | religioustalk           |
///     | phonein                 | personality             |
///     | travelandtouring        | public                  |
///     | leisureandhobby         | college                 |
///     | jazzmusic               | spanishtalk             |
///     | countrymusic            | spanishmusic            |
///     | nationalmusic           | hiphop                  |
///     | oldiesmusic             |                         |
///     | folkmusic               |                         |
///     | documentary             | weather                 |
///     | alarmtest               | alarmtest               |
///     | alarm-alarm             | alarm-alarm             |
///     @note "alarm-alarm" is normally not used from radio stations\, is thought
///     to inform about horrible messages who are needed asap to all people.
///     <p><hr>
///     @skinning_v16 **[New Infolabel]** \link RDS_RadioStyle `RDS.RadioStyle`\endlink
///     <p>
///   }
///   \table_row3{   <b>`RDS.Comment`</b>,
///                  \anchor RDS_Comment
///                  _string_,
///     @return The radio station comment string if available.
///     @note Only available on RadiotextPlus)
///     <p><hr>
///     @skinning_v16 **[New Infolabel]** \link RDS_Comment `RDS.Comment`\endlink
///     <p>
///   }
///   \table_row3{   <b>`RDS.InfoNews`</b>,
///                  \anchor RDS_InfoNews
///                  _string_,
///     @return The message / headline (if available).
///     @note Only available on RadiotextPlus
///     <p><hr>
///     @skinning_v16 **[New Infolabel]** \link RDS_InfoNews `RDS.InfoNews`\endlink
///     <p>
///   }
///   \table_row3{   <b>`RDS.InfoNewsLocal`</b>,
///                  \anchor RDS_InfoNewsLocal
///                  _string_,
///     @return The local information news sended from radio channel (if available).
///     @note Only available on RadiotextPlus
///     <p><hr>
///     @skinning_v16 **[New Infolabel]** \link RDS_InfoNewsLocal `RDS.InfoNewsLocal`\endlink
///     <p>
///   }
///   \table_row3{   <b>`RDS.InfoStock`</b>,
///                  \anchor RDS_InfoStock
///                  _string_,
///     @return The stock information; either as one part or as several distinct parts:
///     "name 99latest value 99change 99high 99low 99volume" (if available).
///     @note Only available on RadiotextPlus
///     <p><hr>
///     @skinning_v16 **[New Infolabel]** \link RDS_InfoStock `RDS.InfoStock`\endlink
///     <p>
///   }
///   \table_row3{   <b>`RDS.InfoStockSize`</b>,
///                  \anchor RDS_InfoStockSize
///                  _string_,
///     @return The number of rows present in stock information.
///     @note Only available on RadiotextPlus
///     <p><hr>
///     @skinning_v16 **[New Infolabel]** \link RDS_InfoStockSize `RDS.InfoStockSize`\endlink
///     <p>
///   }
///   \table_row3{   <b>`RDS.InfoSport`</b>,
///                  \anchor RDS_InfoSport
///                  _string_,
///     @return The result of a match; either as one part or as several distinct parts:
///     "match 99result"\, e.g. "Bayern München : Borussia 995:5"  (if available).
///     @note Only available on RadiotextPlus
///     <p><hr>
///     @skinning_v16 **[New Infolabel]** \link RDS_InfoSport `RDS.InfoSport`\endlink
///     <p>
///   }
///   \table_row3{   <b>`RDS.InfoSportSize`</b>,
///                  \anchor RDS_InfoSportSize
///                  _string_,
///     @return The number of rows present in sport information.
///     @note Only available on RadiotextPlus
///     <p><hr>
///     @skinning_v16 **[New Infolabel]** \link RDS_InfoSportSize `RDS.InfoSportSize`\endlink
///     <p>
///   }
///   \table_row3{   <b>`RDS.InfoLottery`</b>,
///                  \anchor RDS_InfoLottery
///                  _string_,
///     @return The raffle / lottery: "key word 99values" (if available).
///     @note Only available on RadiotextPlus
///     <p><hr>
///     @skinning_v16 **[New Infolabel]** \link RDS_InfoLottery `RDS.InfoLottery`\endlink
///     <p>
///   }
///   \table_row3{   <b>`RDS.InfoLotterySize`</b>,
///                  \anchor RDS_InfoLotterySize
///                  _string_,
///     @return The number of rows present in lottery information.
///     @note Only available on RadiotextPlus
///     <p><hr>
///     @skinning_v16 **[New Infolabel]** \link RDS_InfoLotterySize `RDS.InfoLotterySize`\endlink
///     <p>
///   }
///   \table_row3{   <b>`RDS.InfoWeather`</b>,
///                  \anchor RDS_InfoWeather
///                  _string_,
///     @return The weather information (if available).
///     @note Only available on RadiotextPlus
///     <p><hr>
///     @skinning_v16 **[New Infolabel]** \link RDS_InfoWeather `RDS.InfoWeather`\endlink
///     <p>
///   }
///   \table_row3{   <b>`RDS.InfoWeatherSize`</b>,
///                  \anchor RDS_InfoWeatherSize
///                  _string_,
///     @return The number of rows present in weather information.
///     @note Only available on RadiotextPlus
///     <p><hr>
///     @skinning_v16 **[New Infolabel]** \link RDS_InfoWeatherSize `RDS.InfoWeatherSize`\endlink
///     <p>
///   }
///   \table_row3{   <b>`RDS.InfoCinema`</b>,
///                  \anchor RDS_InfoCinema
///                  _string_,
///     @return The information about movies in cinema (if available).
///     @note Only available on RadiotextPlus
///     <p><hr>
///     @skinning_v16 **[New Infolabel]** \link RDS_InfoCinema `RDS.InfoCinema`\endlink
///     <p>
///   }
///   \table_row3{   <b>`RDS.InfoCinemaSize`</b>,
///                  \anchor RDS_InfoCinemaSize
///                  _string_,
///     @return The number of rows present in cinema information.
///     @note Only available on RadiotextPlus
///     <p><hr>
///     @skinning_v16 **[New Infolabel]** \link RDS_InfoCinemaSize `RDS.InfoCinemaSize`\endlink
///     <p>
///   }
///   \table_row3{   <b>`RDS.InfoHoroscope`</b>,
///                  \anchor RDS_InfoHoroscope
///                  _string_,
///     @return The horoscope; either as one part or as two distinct parts:
///     "key word 99text"\, e.g. "sign of the zodiac 99blablabla" (if available).
///     @note Only available on RadiotextPlus
///     <p><hr>
///     @skinning_v16 **[New Infolabel]** \link RDS_InfoHoroscope `RDS.InfoHoroscope`\endlink
///     <p>
///   }
///   \table_row3{   <b>`RDS.InfoHoroscopeSize`</b>,
///                  \anchor RDS_InfoHoroscopeSize
///                  _string_,
///     @return The Number of rows present in horoscope information.
///     @note Only available on RadiotextPlus
///     <p><hr>
///     @skinning_v16 **[New Infolabel]** \link RDS_InfoHoroscopeSize `RDS.InfoHoroscopeSize`\endlink
///     <p>
///   }
///   \table_row3{   <b>`RDS.InfoOther`</b>,
///                  \anchor RDS_InfoOther
///                  _string_,
///     @return Other information\, not especially specified: "key word 99info" (if available).
///     @note Only available on RadiotextPlus
///     <p><hr>
///     @skinning_v16 **[New Infolabel]** \link RDS_InfoOther `RDS.InfoOther`\endlink
///     <p>
///   }
///   \table_row3{   <b>`RDS.InfoOtherSize`</b>,
///                  \anchor RDS_InfoOtherSize
///                  _string_,
///     @return The number of rows present with other information.
///     @note Only available on RadiotextPlus
///     <p><hr>
///     @skinning_v16 **[New Infolabel]** \link RDS_InfoOtherSize `RDS.InfoOtherSize`\endlink
///     <p>
///   }
///   \table_row3{   <b>`RDS.ProgStation`</b>,
///                  \anchor RDS_ProgStation
///                  _string_,
///     @return The name of the radio channel.
///     @note becomes also set from epg if it is not available from RDS
///     <p><hr>
///     @skinning_v16 **[New Infolabel]** \link RDS_ProgStation `RDS.ProgStation`\endlink
///     <p>
///   }
///   \table_row3{   <b>`RDS.ProgNow`</b>,
///                  \anchor RDS_ProgNow
///                  _string_,
///     @return The now playing program name.
///     @note becomes also be set from epg if from RDS not available
///     <p><hr>
///     @skinning_v16 **[New Infolabel]** \link RDS_ProgNow `RDS.ProgNow`\endlink
///     <p>
///   }
///   \table_row3{   <b>`RDS.ProgNext`</b>,
///                  \anchor RDS_ProgNext
///                  _string_,
///     @return The next played program name (if available).
///     @note becomes also be set from epg if from RDS not available
///     <p><hr>
///     @skinning_v16 **[New Infolabel]** \link RDS_ProgNext `RDS.ProgNext`\endlink
///     <p>
///   }
///   \table_row3{   <b>`RDS.ProgHost`</b>,
///                  \anchor RDS_ProgHost
///                  _string_,
///     @return The name of the host of the radio show.
///     <p><hr>
///     @skinning_v16 **[New Infolabel]** \link RDS_ProgHost `RDS.ProgHost`\endlink
///     <p>
///   }
///   \table_row3{   <b>`RDS.ProgEditStaff`</b>,
///                  \anchor RDS_ProgEditStaff
///                  _string_,
///     @return The name of the editorial staff; e.g. name of editorial journalist.
///     @note Only available on RadiotextPlus
///     <p><hr>
///     @skinning_v16 **[New Infolabel]** \link RDS_ProgEditStaff `RDS.ProgEditStaff`\endlink
///     <p>
///   }
///   \table_row3{   <b>`RDS.ProgHomepage`</b>,
///                  \anchor RDS_ProgHomepage
///                  _string_,
///     @return The Link to radio station homepage
///     @note Only available on RadiotextPlus
///     <p><hr>
///     @skinning_v16 **[New Infolabel]** \link RDS_ProgHomepage `RDS.ProgHomepage`\endlink
///     <p>
///   }
///   \table_row3{   <b>`RDS.ProgStyle`</b>,
///                  \anchor RDS_ProgStyle
///                  _string_,
///     @return A human readable string about radiostyle defined from RDS or RBDS.
///     <p><hr>
///     @skinning_v16 **[New Infolabel]** \link RDS_ProgStyle `RDS.ProgStyle`\endlink
///     <p>
///   }
///   \table_row3{   <b>`RDS.PhoneHotline`</b>,
///                  \anchor RDS_PhoneHotline
///                  _string_,
///     @return The telephone number of the radio station's hotline.
///     @note Only available on RadiotextPlus
///     <p><hr>
///     @skinning_v16 **[New Infolabel]** \link RDS_PhoneHotline `RDS.PhoneHotline`\endlink
///     <p>
///   }
///   \table_row3{   <b>`RDS.PhoneStudio`</b>,
///                  \anchor RDS_PhoneStudio
///                  _string_,
///     @return The telephone number of the radio station's studio.
///     @note Only available on RadiotextPlus
///     <p><hr>
///     @skinning_v16 **[New Infolabel]** \link RDS_PhoneStudio `RDS.PhoneStudio`\endlink
///     <p>
///   }
///   \table_row3{   <b>`RDS.SmsStudio`</b>,
///                  \anchor RDS_SmsStudio
///                  _string_,
///     @return The sms number of the radio stations studio (to send directly a sms to
///     the studio) (if available).
///     @note Only available on RadiotextPlus
///     <p><hr>
///     @skinning_v16 **[New Infolabel]** \link RDS_SmsStudio `RDS.SmsStudio`\endlink
///     <p>
///   }
///   \table_row3{   <b>`RDS.EmailHotline`</b>,
///                  \anchor RDS_EmailHotline
///                  _string_,
///     @return The email address of the radio stations hotline (if available).
///     @note Only available on RadiotextPlus
///     <p><hr>
///     @skinning_v16 **[New Infolabel]** \link RDS_EmailHotline `RDS.EmailHotline`\endlink
///     <p>
///   }
///   \table_row3{   <b>`RDS.EmailStudio`</b>,
///                  \anchor RDS_EmailStudio
///                  _string_,
///     @return The email address of the radio station's studio (if available).
///     @note Only available on RadiotextPlus
///     <p><hr>
///     @skinning_v16 **[New Infolabel]** \link RDS_EmailStudio `RDS.EmailStudio`\endlink
///     <p>
///   }
/// \table_end
///
/// -----------------------------------------------------------------------------
const infomap rds[] =            {{ "hasrds",                   RDS_HAS_RDS },
                                  { "hasradiotext",             RDS_HAS_RADIOTEXT },
                                  { "hasradiotextplus",         RDS_HAS_RADIOTEXT_PLUS },
                                  { "audiolanguage",            RDS_AUDIO_LANG },
                                  { "channelcountry",           RDS_CHANNEL_COUNTRY },
                                  { "title",                    RDS_TITLE },
                                  { "getline",                  RDS_GET_RADIOTEXT_LINE },
                                  { "artist",                   RDS_ARTIST },
                                  { "band",                     RDS_BAND },
                                  { "composer",                 RDS_COMPOSER },
                                  { "conductor",                RDS_CONDUCTOR },
                                  { "album",                    RDS_ALBUM },
                                  { "tracknumber",              RDS_ALBUM_TRACKNUMBER },
                                  { "radiostyle",               RDS_GET_RADIO_STYLE },
                                  { "comment",                  RDS_COMMENT },
                                  { "infonews",                 RDS_INFO_NEWS },
                                  { "infonewslocal",            RDS_INFO_NEWS_LOCAL },
                                  { "infostock",                RDS_INFO_STOCK },
                                  { "infostocksize",            RDS_INFO_STOCK_SIZE },
                                  { "infosport",                RDS_INFO_SPORT },
                                  { "infosportsize",            RDS_INFO_SPORT_SIZE },
                                  { "infolottery",              RDS_INFO_LOTTERY },
                                  { "infolotterysize",          RDS_INFO_LOTTERY_SIZE },
                                  { "infoweather",              RDS_INFO_WEATHER },
                                  { "infoweathersize",          RDS_INFO_WEATHER_SIZE },
                                  { "infocinema",               RDS_INFO_CINEMA },
                                  { "infocinemasize",           RDS_INFO_CINEMA_SIZE },
                                  { "infohoroscope",            RDS_INFO_HOROSCOPE },
                                  { "infohoroscopesize",        RDS_INFO_HOROSCOPE_SIZE },
                                  { "infoother",                RDS_INFO_OTHER },
                                  { "infoothersize",            RDS_INFO_OTHER_SIZE },
                                  { "progstation",              RDS_PROG_STATION },
                                  { "prognow",                  RDS_PROG_NOW },
                                  { "prognext",                 RDS_PROG_NEXT },
                                  { "proghost",                 RDS_PROG_HOST },
                                  { "progeditstaff",            RDS_PROG_EDIT_STAFF },
                                  { "proghomepage",             RDS_PROG_HOMEPAGE },
                                  { "progstyle",                RDS_PROG_STYLE },
                                  { "phonehotline",             RDS_PHONE_HOTLINE },
                                  { "phonestudio",              RDS_PHONE_STUDIO },
                                  { "smsstudio",                RDS_SMS_STUDIO },
                                  { "emailhotline",             RDS_EMAIL_HOTLINE },
                                  { "emailstudio",              RDS_EMAIL_STUDIO },
                                  { "hashotline",               RDS_HAS_HOTLINE_DATA },
                                  { "hasstudio",                RDS_HAS_STUDIO_DATA }};

/// \page modules__infolabels_boolean_conditions
/// \subsection modules__infolabels_boolean_conditions_slideshow Slideshow
/// \table_start
///   \table_h3{ Labels, Type, Description }
///   \table_row3{   <b>`Slideshow.IsActive`</b>,
///                  \anchor Slideshow_IsActive
///                  _boolean_,
///     @return **True** if the picture slideshow is running.
///     <p>
///   }
///   \table_row3{   <b>`Slideshow.IsPaused`</b>,
///                  \anchor Slideshow_IsPaused
///                  _boolean_,
///     @return **True** if the picture slideshow is paused.
///     <p>
///   }
///   \table_row3{   <b>`Slideshow.IsRandom`</b>,
///                  \anchor Slideshow_IsRandom
///                  _boolean_,
///     @return **True** if the picture slideshow is in random mode.
///     <p>
///   }
///   \table_row3{   <b>`Slideshow.IsVideo`</b>,
///                  \anchor Slideshow_IsVideo
///                  _boolean_,
///     @return **True** if the picture slideshow is playing a video.
///     <p><hr>
///     @skinning_v13 **[New Boolean Condition]** \link Slideshow_IsVideo `Slideshow.IsVideo`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Slideshow.Altitude`</b>,
///                  \anchor Slideshow_Altitude
///                  _string_,
///     @return The altitude in meters where the current picture was taken.
///     @note This is the value of the EXIF GPSInfo.GPSAltitude tag.
///     <p>
///   }
///   \table_row3{   <b>`Slideshow.Aperture`</b>,
///                  \anchor Slideshow_Aperture
///                  _string_,
///     @return The F-stop used to take the current picture.
///     @note This is the value of the EXIF FNumber tag (hex code 0x829D).
///     <p>
///   }
///   \table_row3{   <b>`Slideshow.Author`</b>,
///                  \anchor Slideshow_Author
///                  _string_,
///     @return The name of the person involved in writing about the current
///     picture. 
///     @note This is the value of the IPTC Writer tag (hex code 0x7A).
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link Slideshow_Author `Slideshow.Author`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Slideshow.Byline`</b>,
///                  \anchor Slideshow_Byline
///                  _string_,
///     @return The name of the person who created the current picture. 
///     @note This is the value of the IPTC Byline tag (hex code 0x50).
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link Slideshow_Byline `Slideshow.Byline`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Slideshow.BylineTitle`</b>,
///                  \anchor Slideshow_BylineTitle
///                  _string_,
///     @return The title of the person who created the current picture. 
///     @note This is the value of the IPTC BylineTitle tag (hex code 0x55).
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link Slideshow_BylineTitle `Slideshow.BylineTitle`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Slideshow.CameraMake`</b>,
///                  \anchor Slideshow_CameraMake
///                  _string_,
///     @return The manufacturer of the camera used to take the current picture.
///     @note This is the value of the EXIF Make tag (hex code 0x010F).
///     <p>
///   }
///   \table_row3{   <b>`Slideshow.CameraModel`</b>,
///                  \anchor Slideshow_CameraModel
///                  _string_,
///     @return The manufacturer's model name or number of the camera used to take
///     the current picture. 
///     @note This is the value of the EXIF Model tag (hex code 0x0110).
///     <p>
///   }
///   \table_row3{   <b>`Slideshow.Caption`</b>,
///                  \anchor Slideshow_Caption
///                  _string_,
///     @return A description of the current picture. 
///     @note This is the value of the IPTC Caption tag (hex code 0x78).
///     <p>
///   }
///   \table_row3{   <b>`Slideshow.Category`</b>,
///                  \anchor Slideshow_Category
///                  _string_,
///     @return The subject of the current picture as a category code.
///     @note This is the value of the IPTC Category tag (hex code 0x0F).
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link Slideshow_Category `Slideshow.Category`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Slideshow.CCDWidth`</b>,
///                  \anchor Slideshow_CCDWidth
///                  _string_,
///     @return The width of the CCD in the camera used to take the current
///     picture. 
///     @note This is calculated from three EXIF tags (0xA002 * 0xA210 / 0xA20e).
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link Slideshow_CCDWidth `Slideshow.CCDWidth`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Slideshow.City`</b>,
///                  \anchor Slideshow_City
///                  _string_,
///     @return The city where the current picture was taken.
///     @note This is the value of the IPTC City tag (hex code 0x5A).
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link Slideshow_City `Slideshow.City`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Slideshow.Colour`</b>,
///                  \anchor Slideshow_Colour
///                  _string_,
///     @return the colour of the picture. It can have one of the following values:
///       - <b>"Colour"</b>
///       - <b>"Black and White"</b>
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link Slideshow_Colour `Slideshow.Colour`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Slideshow.CopyrightNotice`</b>,
///                  \anchor Slideshow_CopyrightNotice
///                  _string_,
///     @return The copyright notice of the current picture. 
///     @note This is the value of the IPTC Copyright tag (hex code 0x74).
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link Slideshow_CopyrightNotice `Slideshow.CopyrightNotice`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Slideshow.Country`</b>,
///                  \anchor Slideshow_Country
///                  _string_,
///     @return The full name of the country where the current picture was taken.
///     @note This is the value of the IPTC CountryName tag (hex code 0x65).
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link Slideshow_Country `Slideshow.Country`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Slideshow.CountryCode`</b>,
///                  \anchor Slideshow_CountryCode
///                  _string_,
///     @return The country code of the country where the current picture was
///     taken. 
///     @note This is the value of the IPTC CountryCode tag (hex code 0x64).
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link Slideshow_CountryCode `Slideshow.CountryCode`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Slideshow.Credit`</b>,
///                  \anchor Slideshow_Credit
///                  _string_,
///     @return Who provided the current picture. 
///     @note This is the value of the IPTC Credit tag (hex code 0x6E).
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link Slideshow_Credit `Slideshow.Credit`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Slideshow.DigitalZoom`</b>,
///                  \anchor Slideshow_DigitalZoom
///                  _string_,
///     @return The digital zoom ratio when the current picture was taken.
///     @note This is the value of the EXIF .DigitalZoomRatio tag (hex code 0xA404).
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link Slideshow_DigitalZoom `Slideshow.DigitalZoom`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Slideshow.EXIFComment`</b>,
///                  \anchor Slideshow_EXIFComment
///                  _string_,
///     @return A description of the current picture. 
///     @note This is the value of the EXIF User Comment tag (hex code 0x9286). 
///     This is the same value as \ref Slideshow_SlideComment "Slideshow.SlideComment".
///     <p>
///   }
///   \table_row3{   <b>`Slideshow.EXIFDate`</b>,
///                  \anchor Slideshow_EXIFDate
///                  _string_,
///     @return The localized date of the current picture. The short form of the
///     date is used.
///     @note The value of the EXIF DateTimeOriginal tag (hex code
///     0x9003) is preferred. If the DateTimeOriginal tag is not found\, the
///     value of DateTimeDigitized (hex code 0x9004) or of DateTime (hex code
///     0x0132) might be used.
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link Slideshow_EXIFDate `Slideshow.EXIFDate`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Slideshow.EXIFDescription`</b>,
///                  \anchor Slideshow_EXIFDescription
///                  _string_,
///     @return A short description of the current picture. The SlideComment\,
///     EXIFComment or Caption values might contain a longer description. 
///     @note This is the value of the EXIF ImageDescription tag (hex code 0x010E).
///     <p>
///   }
///   \table_row3{   <b>`Slideshow.EXIFSoftware`</b>,
///                  \anchor Slideshow_EXIFSoftware
///                  _string_,
///     @return The name and version of the firmware used by the camera that took
///     the current picture. 
///     @note This is the value of the EXIF Software tag (hex code 0x0131).
///     <p>
///   }
///   \table_row3{   <b>`Slideshow.EXIFTime`</b>,
///                  \anchor Slideshow_EXIFTime
///                  _string_,
///     @return The date/timestamp of the current picture. The localized short
///     form of the date and time is used. 
///     @note The value of the EXIF DateTimeOriginal tag (hex code 0x9003) is 
///     preferred. If the DateTimeOriginal tag is not found\, the value of 
///     DateTimeDigitized (hex code 0x9004) or of DateTime (hex code 0x0132) 
///     might be used.
///     <p>
///   }
///   \table_row3{   <b>`Slideshow.Exposure`</b>,
///                  \anchor Slideshow_Exposure
///                  _string_,
///     @return The class of the program used by the camera to set exposure when
///     the current picture was taken. Values include:
///      - <b>"Manual"</b>
///      - <b>"Program (Auto)"</b>
///      - <b>"Aperture priority (Semi-Auto)"</b>
///      - <b>"Shutter priority (semi-auto)"</b>
///      - etc... 
///     @note This is the value of the EXIF ExposureProgram tag
///     (hex code 0x8822).
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link Slideshow_Exposure `Slideshow.Exposure`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Slideshow.ExposureBias`</b>,
///                  \anchor Slideshow_ExposureBias
///                  _string_,
///     @return The exposure bias of the current picture. Typically this is a
///     number between -99.99 and 99.99.
///     @note This is the value of the EXIF ExposureBiasValue tag (hex code 0x9204).
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link Slideshow_ExposureBias `Slideshow.ExposureBias`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Slideshow.ExposureMode`</b>,
///                  \anchor Slideshow_ExposureMode
///                  _string_,
///     @return The exposure mode of the current picture. The possible values are:
///      - <b>"Automatic"</b>
///      - <b>"Manual"</b>
///      - <b>"Auto bracketing"</b>
///     @note This is the value of the EXIF ExposureMode tag (hex code 0xA402).
///     <p>
///   }
///   \table_row3{   <b>`Slideshow.ExposureTime`</b>,
///                  \anchor Slideshow_ExposureTime
///                  _string_,
///     @return The exposure time of the current picture\, in seconds.
///     @note This is the value of the EXIF ExposureTime tag (hex code 0x829A).
///     If the ExposureTime tag is not found\, the ShutterSpeedValue tag (hex code
///     0x9201) might be used.
///     <p>
///   }
///   \table_row3{   <b>`Slideshow.Filedate`</b>,
///                  \anchor Slideshow_Filedate
///                  _string_,
///     @return The file date of the current picture.
///     <p>
///   }
///   \table_row3{   <b>`Slideshow.Filename`</b>,
///                  \anchor Slideshow_Filename
///                  _string_,
///     @return The file name of the current picture.
///     <p>
///   }
///   \table_row3{   <b>`Slideshow.Filesize`</b>,
///                  \anchor Slideshow_Filesize
///                  _string_,
///     @return The file size of the current picture.
///     <p>
///   }
///   \table_row3{   <b>`Slideshow.FlashUsed`</b>,
///                  \anchor Slideshow_FlashUsed
///                  _string_,
///     @return The status of flash when the current picture was taken. The value
///     will be either <b>"Yes"</b> or <b>"No"</b>\, and might include additional information.
///     @note This is the value of the EXIF Flash tag (hex code 0x9209).
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link Slideshow_FlashUsed `Slideshow.FlashUsed`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Slideshow.FocalLength`</b>,
///                  \anchor Slideshow_FocalLength
///                  _string_,
///     @return The focal length of the lens\, in mm. 
///     @note This is the value of the EXIF FocalLength tag (hex code 0x920A).
///     <p>
///   }
///   \table_row3{   <b>`Slideshow.FocusDistance`</b>,
///                  \anchor Slideshow_FocusDistance
///                  _string_,
///     @return The distance to the subject\, in meters.
///     @note This is the value of the EXIF SubjectDistance tag (hex code 0x9206).
///     <p>
///   }
///   \table_row3{   <b>`Slideshow.Headline`</b>,
///                  \anchor Slideshow_Headline
///                  _string_,
///     @return A synopsis of the contents of the current picture.
///     @note This is the value of the IPTC Headline tag (hex code 0x69).
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link Slideshow_Headline `Slideshow.Headline`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Slideshow.ImageType`</b>,
///                  \anchor Slideshow_ImageType
///                  _string_,
///     @return The color components of the current picture.
///     @note This is the value of the IPTC ImageType tag (hex code 0x82).
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link Slideshow_ImageType `Slideshow.ImageType`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Slideshow.IPTCDate`</b>,
///                  \anchor Slideshow_IPTCDate
///                  _string_,
///     @return The date when the intellectual content of the current picture was
///     created\, rather than when the picture was created.
///     @note This is the value of the IPTC DateCreated tag (hex code 0x37).
///     <p>
///   }
///   \table_row3{   <b>`Slideshow.ISOEquivalence`</b>,
///                  \anchor Slideshow_ISOEquivalence
///                  _string_,
///     @return The ISO speed of the camera when the current picture was taken.
///     @note This is the value of the EXIF ISOSpeedRatings tag (hex code 0x8827).
///     <p>
///   }
///   \table_row3{   <b>`Slideshow.Keywords`</b>,
///                  \anchor Slideshow_Keywords
///                  _string_,
///     @return The keywords assigned to the current picture.
///     @note This is the value of the IPTC Keywords tag (hex code 0x19).
///     <p>
///   }
///   \table_row3{   <b>`Slideshow.Latitude`</b>,
///                  \anchor Slideshow_Latitude
///                  _string_,
///     @return The latitude where the current picture was taken (degrees\,
///     minutes\, seconds North or South). 
///     @note This is the value of the EXIF GPSInfo.GPSLatitude and 
///     GPSInfo.GPSLatitudeRef tags.
///     <p>
///   }
///   \table_row3{   <b>`Slideshow.LightSource`</b>,
///                  \anchor Slideshow_LightSource
///                  _string_,
///     @return The kind of light source when the picture was taken. Possible
///     values include:
///      - <b>"Daylight"</b>
///      - <b>"Fluorescent"</b>
///      - <b>"Incandescent"</b>
///      - etc...
///     @note This is the value of the EXIF LightSource tag (hex code 0x9208).
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link Slideshow_LightSource `Slideshow.LightSource`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Slideshow.LongEXIFDate`</b>,
///                  \anchor Slideshow_LongEXIFDate
///                  _string_,
///     @return Only the localized date of the current picture. The long form of
///     the date is used. 
///     @note The value of the EXIF DateTimeOriginal tag (hex code
///     0x9003) is preferred. If the DateTimeOriginal tag is not found\, the
///     value of DateTimeDigitized (hex code 0x9004) or of DateTime (hex code
///     0x0132) might be used.
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link Slideshow_LongEXIFDate `Slideshow.LongEXIFDate`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Slideshow.LongEXIFTime`</b>,
///                  \anchor Slideshow_LongEXIFTime
///                  _string_,
///     @return The date/timestamp of the current picture. The localized long form
///     of the date and time is used.
///     @note The value of the EXIF DateTimeOriginal tag
///     (hex code 0x9003) is preferred. if the DateTimeOriginal tag is not found\,
///     the value of DateTimeDigitized (hex code 0x9004) or of DateTime (hex
///     code 0x0132) might be used.
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link Slideshow_LongEXIFTime `Slideshow.LongEXIFTime`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Slideshow.Longitude`</b>,
///                  \anchor Slideshow_Longitude
///                  _string_,
///     @return The longitude where the current picture was taken (degrees\,
///     minutes\, seconds East or West).
///     @note This is the value of the EXIF GPSInfo.GPSLongitude and 
///     GPSInfo.GPSLongitudeRef tags.
///     <p>
///   }
///   \table_row3{   <b>`Slideshow.MeteringMode`</b>,
///                  \anchor Slideshow_MeteringMode
///                  _string_,
///     @return The metering mode used when the current picture was taken. The
///     possible values are:
///      - <b>"Center weight"</b>
///      - <b>"Spot"</b>
///      - <b>"Matrix"</b>
///     @note This is the value of the EXIF MeteringMode tag (hex code 0x9207).
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link Slideshow_MeteringMode `Slideshow.MeteringMode`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Slideshow.ObjectName`</b>,
///                  \anchor Slideshow_ObjectName
///                  _string_,
///     @return a shorthand reference for the current picture.
///     @note This is the value of the IPTC ObjectName tag (hex code 0x05).
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link Slideshow_ObjectName `Slideshow.ObjectName`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Slideshow.Orientation`</b>,
///                  \anchor Slideshow_Orientation
///                  _string_,
///     @return The orientation of the current picture. Possible values are:
///      - <b>"Top Left"</b>
///      - <b>"Top Right"</b>
///      - <b>"Left Top"</b>
///      - <b>"Right Bottom"</b>
///      - etc...
///     @note This is the value of the EXIF Orientation tag (hex code 0x0112).
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link Slideshow_Orientation `Slideshow.Orientation`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Slideshow.Path`</b>,
///                  \anchor Slideshow_Path
///                  _string_,
///     @return The file path of the current picture.
///     <p>
///   }
///   \table_row3{   <b>`Slideshow.Process`</b>,
///                  \anchor Slideshow_Process
///                  _string_,
///     @return The process used to compress the current picture.
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link Slideshow_Process `Slideshow.Process`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Slideshow.ReferenceService`</b>,
///                  \anchor Slideshow_ReferenceService
///                  _string_,
///     @return The Service Identifier of a prior envelope to which the current
///     picture refers.
///     @note This is the value of the IPTC ReferenceService tag (hex code 0x2D).
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link Slideshow_ReferenceService `Slideshow.ReferenceService`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Slideshow.Resolution`</b>,
///                  \anchor Slideshow_Resolution
///                  _string_,
///     @return The dimensions of the current picture (Width x Height)
///     <p>
///   }
///   \table_row3{   <b>`Slideshow.SlideComment`</b>,
///                  \anchor Slideshow_SlideComment
///                  _string_,
///     @return A description of the current picture. 
///     @note This is the value of the EXIF User Comment tag (hex code 0x9286). 
///     This is the same value as \ref Slideshow_EXIFComment "Slideshow.EXIFComment".
///     <p>
///   }
///   \table_row3{   <b>`Slideshow.SlideIndex`</b>,
///                  \anchor Slideshow_SlideIndex
///                  _string_,
///     @return The slide index of the current picture.
///     <p>
///   }
///   \table_row3{   <b>`Slideshow.Source`</b>,
///                  \anchor Slideshow_Source
///                  _string_,
///     @return The original owner of the current picture.
///     @note This is the value of the IPTC Source tag (hex code 0x73).
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link Slideshow_Source `Slideshow.Source`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Slideshow.SpecialInstructions`</b>,
///                  \anchor Slideshow_SpecialInstructions
///                  _string_,
///     @return Other editorial instructions concerning the use of the current
///     picture.
///     @note This is the value of the IPTC SpecialInstructions tag (hex code 0x28).
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link Slideshow_SpecialInstructions `Slideshow.SpecialInstructions`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Slideshow.State`</b>,
///                  \anchor Slideshow_State
///                  _string_,
///     @return The State/Province where the current picture was taken.
///     @note This is the value of the IPTC ProvinceState tag (hex code 0x5F).
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link Slideshow_State `Slideshow.State`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Slideshow.Sublocation`</b>,
///                  \anchor Slideshow_Sublocation
///                  _string_,
///     @return The location within a city where the current picture was taken -
///     might indicate the nearest landmark. 
///     @note This is the value of the IPTC SubLocation tag (hex code 0x5C).
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link Slideshow_Sublocation `Slideshow.Sublocation`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Slideshow.SupplementalCategories`</b>,
///                  \anchor Slideshow_SupplementalCategories
///                  _string_,
///     @return The supplemental category codes to further refine the subject of the
///     current picture. 
///     @note This is the value of the IPTC SuppCategory tag (hex
///     code 0x14).
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link Slideshow_SupplementalCategories `Slideshow.SupplementalCategories`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Slideshow.TimeCreated`</b>,
///                  \anchor Slideshow_TimeCreated
///                  _string_,
///     @return The time when the intellectual content of the current picture was
///     created\, rather than when the picture was created. 
///     @note This is the value of the IPTC TimeCreated tag (hex code 0x3C).
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link Slideshow_TimeCreated `Slideshow.TimeCreated`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Slideshow.TransmissionReference`</b>,
///                  \anchor Slideshow_TransmissionReference
///                  _string_,
///     @return A code representing the location of original transmission of the
///     current picture. 
///     @note This is the value of the IPTC TransmissionReference tag
///     (hex code 0x67).
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link Slideshow_TransmissionReference `Slideshow.TransmissionReference`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Slideshow.Urgency`</b>,
///                  \anchor Slideshow_Urgency
///                  _string_,
///     @return The urgency of the current picture. Values are 1-9. The 1 is most
///     urgent. 
///     @note Some image management programs use urgency to indicate picture
///     rating\, where urgency 1 is 5 stars and urgency 5 is 1 star. Urgencies
///     6-9 are not used for rating. This is the value of the IPTC Urgency tag
///     (hex code 0x0A).
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link Slideshow_Urgency `Slideshow.Urgency`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Slideshow.WhiteBalance`</b>,
///                  \anchor Slideshow_WhiteBalance
///                  _string_,
///     @return The white balance mode set when the current picture was taken.
///     The possible values are:
///       - <b>"Manual"</b>
///       - <b>"Auto"</b>
///     <p>
///     @note This is the value of the EXIF WhiteBalance tag (hex code 0xA403).
///     <p><hr>
///     @skinning_v13 **[New Infolabel]** \link Slideshow_WhiteBalance `Slideshow.WhiteBalance`\endlink
///     <p>
///   }
/// \table_end
///
/// -----------------------------------------------------------------------------
const infomap slideshow[] =      {{ "ispaused",               SLIDESHOW_ISPAUSED },
                                  { "isactive",               SLIDESHOW_ISACTIVE },
                                  { "isvideo",                SLIDESHOW_ISVIDEO },
                                  { "israndom",               SLIDESHOW_ISRANDOM },
                                  { "filename",               SLIDESHOW_FILE_NAME },
                                  { "path",                   SLIDESHOW_FILE_PATH },
                                  { "filesize",               SLIDESHOW_FILE_SIZE },
                                  { "filedate",               SLIDESHOW_FILE_DATE },
                                  { "slideindex",             SLIDESHOW_INDEX },
                                  { "resolution",             SLIDESHOW_RESOLUTION },
                                  { "slidecomment",           SLIDESHOW_COMMENT },
                                  { "colour",                 SLIDESHOW_COLOUR },
                                  { "process",                SLIDESHOW_PROCESS },
                                  { "exiftime",               SLIDESHOW_EXIF_DATE_TIME },
                                  { "exifdate",               SLIDESHOW_EXIF_DATE },
                                  { "longexiftime",           SLIDESHOW_EXIF_LONG_DATE_TIME },
                                  { "longexifdate",           SLIDESHOW_EXIF_LONG_DATE },
                                  { "exifdescription",        SLIDESHOW_EXIF_DESCRIPTION },
                                  { "cameramake",             SLIDESHOW_EXIF_CAMERA_MAKE },
                                  { "cameramodel",            SLIDESHOW_EXIF_CAMERA_MODEL },
                                  { "exifcomment",            SLIDESHOW_EXIF_COMMENT },
                                  { "exifsoftware",           SLIDESHOW_EXIF_SOFTWARE },
                                  { "aperture",               SLIDESHOW_EXIF_APERTURE },
                                  { "focallength",            SLIDESHOW_EXIF_FOCAL_LENGTH },
                                  { "focusdistance",          SLIDESHOW_EXIF_FOCUS_DIST },
                                  { "exposure",               SLIDESHOW_EXIF_EXPOSURE },
                                  { "exposuretime",           SLIDESHOW_EXIF_EXPOSURE_TIME },
                                  { "exposurebias",           SLIDESHOW_EXIF_EXPOSURE_BIAS },
                                  { "exposuremode",           SLIDESHOW_EXIF_EXPOSURE_MODE },
                                  { "flashused",              SLIDESHOW_EXIF_FLASH_USED },
                                  { "whitebalance",           SLIDESHOW_EXIF_WHITE_BALANCE },
                                  { "lightsource",            SLIDESHOW_EXIF_LIGHT_SOURCE },
                                  { "meteringmode",           SLIDESHOW_EXIF_METERING_MODE },
                                  { "isoequivalence",         SLIDESHOW_EXIF_ISO_EQUIV },
                                  { "digitalzoom",            SLIDESHOW_EXIF_DIGITAL_ZOOM },
                                  { "ccdwidth",               SLIDESHOW_EXIF_CCD_WIDTH },
                                  { "orientation",            SLIDESHOW_EXIF_ORIENTATION },
                                  { "supplementalcategories", SLIDESHOW_IPTC_SUP_CATEGORIES },
                                  { "keywords",               SLIDESHOW_IPTC_KEYWORDS },
                                  { "caption",                SLIDESHOW_IPTC_CAPTION },
                                  { "author",                 SLIDESHOW_IPTC_AUTHOR },
                                  { "headline",               SLIDESHOW_IPTC_HEADLINE },
                                  { "specialinstructions",    SLIDESHOW_IPTC_SPEC_INSTR },
                                  { "category",               SLIDESHOW_IPTC_CATEGORY },
                                  { "byline",                 SLIDESHOW_IPTC_BYLINE },
                                  { "bylinetitle",            SLIDESHOW_IPTC_BYLINE_TITLE },
                                  { "credit",                 SLIDESHOW_IPTC_CREDIT },
                                  { "source",                 SLIDESHOW_IPTC_SOURCE },
                                  { "copyrightnotice",        SLIDESHOW_IPTC_COPYRIGHT_NOTICE },
                                  { "objectname",             SLIDESHOW_IPTC_OBJECT_NAME },
                                  { "city",                   SLIDESHOW_IPTC_CITY },
                                  { "state",                  SLIDESHOW_IPTC_STATE },
                                  { "country",                SLIDESHOW_IPTC_COUNTRY },
                                  { "transmissionreference",  SLIDESHOW_IPTC_TX_REFERENCE },
                                  { "iptcdate",               SLIDESHOW_IPTC_DATE },
                                  { "urgency",                SLIDESHOW_IPTC_URGENCY },
                                  { "countrycode",            SLIDESHOW_IPTC_COUNTRY_CODE },
                                  { "referenceservice",       SLIDESHOW_IPTC_REF_SERVICE },
                                  { "latitude",               SLIDESHOW_EXIF_GPS_LATITUDE },
                                  { "longitude",              SLIDESHOW_EXIF_GPS_LONGITUDE },
                                  { "altitude",               SLIDESHOW_EXIF_GPS_ALTITUDE },
                                  { "timecreated",            SLIDESHOW_IPTC_TIMECREATED },
                                  { "sublocation",            SLIDESHOW_IPTC_SUBLOCATION },
                                  { "imagetype",              SLIDESHOW_IPTC_IMAGETYPE },
};

/// \page modules__infolabels_boolean_conditions
/// \subsection modules__infolabels_boolean_conditions_Library Library
/// @todo Make this annotate an array of infobools/labels to make it easier to track
/// \table_start
///   \table_h3{ Labels, Type, Description }
///   \table_row3{   <b>`Library.IsScanning`</b>,
///                  \anchor Library_IsScanning
///                  _boolean_,
///     @return **True** if the library is being scanned.
///     <p>
///   }
///   \table_row3{   <b>`Library.IsScanningVideo`</b>,
///                  \anchor Library_IsScanningVideo
///                  _boolean_,
///     @return **True** if the video library is being scanned.
///     <p>
///   }
///   \table_row3{   <b>`Library.IsScanningMusic`</b>,
///                  \anchor Library_IsScanningMusic
///                  _boolean_,
///     @return **True** if the music library is being scanned.
///     <p>
///   }
///   \table_row3{   <b>`Library.HasContent(music)`</b>,
///                  \anchor Library_HasContent_Music
///                  _boolean_,
///     @return **True** if the library has music content.
///     <p>
///   }
///   \table_row3{   <b>`Library.HasContent(video)`</b>,
///                  \anchor Library_HasContent_Video
///                  _boolean_,
///     @return **True** if the library has video content.
///     <p>
///   }
///   \table_row3{   <b>`Library.HasContent(movies)`</b>,
///                  \anchor Library_HasContent_Movies
///                  _boolean_,
///     @return **True** if the library has movies.
///     <p>
///   }
///   \table_row3{   <b>`Library.HasContent(tvshows)`</b>,
///                  \anchor Library_HasContent_TVShows
///                  _boolean_,
///     @return **True** if the library has tvshows.
///     <p>
///   }
///   \table_row3{   <b>`Library.HasContent(musicvideos)`</b>,
///                  \anchor Library_HasContent_MusicVideos
///                  _boolean_,
///     @return **True** if the library has music videos.
///     <p>
///   }
///   \table_row3{   <b>`Library.HasContent(moviesets)`</b>,
///                  \anchor Library_HasContent_MovieSets
///                  _boolean_,
///     @return **True** if the library has movie sets.
///     <p>
///   }
///   \table_row3{   <b>`Library.HasContent(singles)`</b>,
///                  \anchor Library_HasContent_Singles
///                  _boolean_,
///     @return **True** if the library has singles.
///     <p>
///   }
///   \table_row3{   <b>`Library.HasContent(compilations)`</b>,
///                  \anchor Library_HasContent_Compilations
///                  _boolean_,
///     @return **True** if the library has compilations.
///     <p>
///   }
///   \table_row3{   <b>`Library.HasContent(Role.Composer)`</b>,
///                  \anchor Library_HasContent_Role_Composer
///                  _boolean_,
///     @return **True** if there are songs in the library which have composers.
///     <p><hr>
///     @skinning_v17 **[New Boolean Condition]** \link Library_HasContent_Role_Composer `Library.HasContent(Role.Composer)`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Library.HasContent(Role.Conductor)`</b>,
///                  \anchor Library_HasContent_Role_Conductor
///                  _boolean_,
///     @return **True** if there are songs in the library which have a conductor.
///     <p><hr>
///     @skinning_v17 **[New Boolean Condition]** \link Library_HasContent_Role_Conductor `Library.HasContent(Role.Conductor)`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Library.HasContent(Role.Orchestra)`</b>,
///                  \anchor Library_HasContent_Role_Orchestra
///                  _boolean_,
///     @return **True** if there are songs in the library which have an orchestra.
///     <p><hr>
///     @skinning_v17 **[New Boolean Condition]** \link Library_HasContent_Role_Orchestra `Library.HasContent(Role.Orchestra)`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Library.HasContent(Role.Lyricist)`</b>,
///                  \anchor Library_HasContent_Role_Lyricist
///                  _boolean_,
///     @return **True** if there are songs in the library which have a lyricist.
///     <p><hr>
///     @skinning_v17 **[New Boolean Condition]** \link Library_HasContent_Role_Lyricist `Library.HasContent(Role.Lyricist)`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Library.HasContent(Role.Remixer)`</b>,
///                  \anchor Library_HasContent_Role_Remixer
///                  _boolean_,
///     @return **True** if there are songs in the library which have a remixer.
///     <p><hr>
///     @skinning_v17 **[New Boolean Condition]** \link Library_HasContent_Role_Remixer `Library.HasContent(Role.Remixer)`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Library.HasContent(Role.Arranger)`</b>,
///                  \anchor Library_HasContent_Role_Remixer
///                  _boolean_,
///     @return **True** if there are songs in the library which have an arranger.
///     <p><hr>
///     @skinning_v17 **[New Boolean Condition]** \link Library_HasContent_Role_Remixer `Library.HasContent(Role.Arranger)`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Library.HasContent(Role.Engineer)`</b>,
///                  \anchor Library_HasContent_Role_Engineer
///                  _boolean_,
///     @return **True** if there are songs in the library which have an engineer.
///     <p><hr>
///     @skinning_v17 **[New Boolean Condition]** \link Library_HasContent_Role_Engineer `Library.HasContent(Role.Engineer)`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Library.HasContent(Role.Producer)`</b>,
///                  \anchor Library_HasContent_Role_Producer
///                  _boolean_,
///     @return **True** if there are songs in the library which have an producer.
///     <p><hr>
///     @skinning_v17 **[New Boolean Condition]** \link Library_HasContent_Role_Producer `Library.HasContent(Role.Producer)`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Library.HasContent(Role.DJMixer)`</b>,
///                  \anchor Library_HasContent_Role_DJMixer
///                  _boolean_,
///     @return **True** if there are songs in the library which have a DJMixer.
///     <p><hr>
///     @skinning_v17 **[New Boolean Condition]** \link Library_HasContent_Role_DJMixer `Library.HasContent(Role.DJMixer)`\endlink
///     <p>
///   }
///   \table_row3{   <b>`Library.HasContent(Role.Mixer)`</b>,
///                  \anchor Library_HasContent_Role_Mixer
///                  _boolean_,
///     @return **True** if there are songs in the library which have a mixer.
///     <p><hr>
///     @skinning_v17 **[New Boolean Condition]** \link Library_HasContent_Role_Mixer `Library.HasContent(Role.Mixer)`\endlink
///     <p>
///   }
/// \table_end
///
/// -----------------------------------------------------------------------------


/// \page modules__infolabels_boolean_conditions
/// \section modules_rm_infolabels_booleans Additional revision history for Infolabels and Boolean Conditions
/// <hr>
/// \subsection modules_rm_infolabels_booleans_v18 Kodi v18 (Leia)
///
/// @skinning_v18 **[Removed Infolabels]** The following infolabels have been removed:
///   - `Listitem.Property(artistthumbs)`, `Listitem.Property(artistthumb)` - use 
/// \link ListItem_Art_Type `ListItem.Art(type)`\endlink with <b>albumartist[n].*</b> or <b>artist[n].*</b> as <b>type</b>
///   - `ADSP.ActiveStreamType`
///   - `ADSP.DetectedStreamType`
///   - `ADSP.MasterName`
///   - `ADSP.MasterInfo`
///   - `ADSP.MasterOwnIcon`
///   - `ADSP.MasterOverrideIcon`
///   - `ListItem.ChannelNumber`, `ListItem.SubChannelNumber`, `MusicPlayer.ChannelNumber`, 
/// `MusicPlayer.SubChannelNumber`, `VideoPlayer.ChannelNumber`,
/// `VideoPlayer.SubChannelNumber`. Please use the following alternatives
/// \link ListItem_ChannelNumberLabel `ListItem.ChannelNumberLabel` \endlink, 
/// \link MusicPlayer_ChannelNumberLabel `MusicPlayer.ChannelNumberLabel` \endlink
/// \link VideoPlayer_ChannelNumberLabel `VideoPlayer.ChannelNumberLabel` \endlink from now on.
///
/// @skinning_v18 **[Removed Boolean Conditions]** The following infobools have been removed:
///   - `System.HasModalDialog`  - use \link System_HasActiveModalDialog `System.HasActiveModalDialog` \endlink and
///  \link System_HasVisibleModalDialog `System.HasVisibleModalDialog`\endlink instead
///   - `StringCompare()` - use \link String_IsEqual `String.IsEqual(info,string)`\endlink instead
///   - `SubString()` - use \link String_Contains `String.Contains(info,substring)`\endlink instead
///   - `IntegerGreaterThan()` - use \link Integer_IsGreater `Integer.IsGreater(info,number)`\endlink instead
///   - `IsEmpty()` - use \link String_IsEmpty `String.IsEmpty(info)`\endlink instead
///   - `System.HasADSP`
///   - `ADSP.IsActive`
///   - `ADSP.HasInputResample`
///   - `ADSP.HasPreProcess`
///   - `ADSP.HasMasterProcess`
///   - `ADSP.HasPostProcess`
///   - `ADSP.HasOutputResample`
///   - `ADSP.MasterActive`
/// <hr>
/// \subsection modules_rm_infolabels_booleans_v17 Kodi v17 (Krypton)
/// @skinning_v17 **[Removed Infolabels]** The following infolabels have been removed:
///   - `ListItem.StarRating` - use the other ratings instead.
///
/// @skinning_v17 **[Removed Boolean Conditions]** The following infobools have been removed:
///   - `on`  - use `true` instead
///   - `off`  - use `false` instead
///   - `Player.ShowCodec`
///   - `System.GetBool(pvrmanager.enabled)`
/// <hr>
/// \subsection modules_rm_infolabels_booleans_v16 Kodi v16 (Jarvis)
///  @skinning_v16 **[New Boolean Conditions]** The following infobools were added:
///    - `System.HasADSP`
///    - `ADSP.IsActive`
///    - `ADSP.HasInputResample`
///    - `ADSP.HasPreProcess`
///    - `ADSP.HasMasterProcess`
///    - `ADSP.HasPostProcess`
///    - `ADSP.HasOutputResample`
///    - `ADSP.MasterActive`
///    - `System.HasModalDialog`
/// 
///  @skinning_v16 **[New Infolabels]** The following infolabels were added:
///    - `ADSP.ActiveStreamType`
///    - `ADSP.DetectedStreamType`
///    - `ADSP.MasterName`
///    - `ADSP.MasterInfo`
///    - `ADSP.MasterOwnIcon`
///    - `ADSP.MasterOverrideIcon`
///   
///   @skinning_v16 **[Removed Boolean Conditions]** The following infobols were removed:
///    - `System.Platform.ATV2`

/// <hr>
/// \subsection modules_rm_infolabels_booleans_v15 Kodi v15 (Isengard)
/// <hr>
/// \subsection modules_rm_infolabels_booleans_v14 Kodi v14 (Helix)
///  @skinning_v14 **[New Infolabels]** The following infolabels were added:
///    - `ListItem.SubChannelNumber`
///    - `MusicPlayer.SubChannelNumber`
///    - `VideoPlayer.SubChannelNumber`
/// 
/// <hr>
/// \subsection modules_rm_infolabels_booleans_v13 XBMC v13 (Gotham)
///   @skinning_v13 **[Removed Infolabels]** The following infolabels were removed:
///    - `Network.SubnetAddress`
/// 
/// <hr>
// Crazy part, to use tableofcontents must it be on end
/// \page modules__infolabels_boolean_conditions
/// \tableofcontents

CGUIInfoManager::Property::Property(const std::string &property, const std::string &parameters)
: name(property)
{
  CUtil::SplitParams(parameters, params);
}

const std::string &CGUIInfoManager::Property::param(unsigned int n /* = 0 */) const
{
  if (n < params.size())
    return params[n];
  return StringUtils::Empty;
}

unsigned int CGUIInfoManager::Property::num_params() const
{
  return params.size();
}

void CGUIInfoManager::SplitInfoString(const std::string &infoString, std::vector<Property> &info)
{
  // our string is of the form:
  // category[(params)][.info(params).info2(params)] ...
  // so we need to split on . while taking into account of () pairs
  unsigned int parentheses = 0;
  std::string property;
  std::string param;
  for (size_t i = 0; i < infoString.size(); ++i)
  {
    if (infoString[i] == '(')
    {
      if (!parentheses++)
        continue;
    }
    else if (infoString[i] == ')')
    {
      if (!parentheses)
        CLog::Log(LOGERROR, "unmatched parentheses in %s", infoString.c_str());
      else if (!--parentheses)
        continue;
    }
    else if (infoString[i] == '.' && !parentheses)
    {
      if (!property.empty()) // add our property and parameters
      {
        StringUtils::ToLower(property);
        info.emplace_back(Property(property, param));
      }
      property.clear();
      param.clear();
      continue;
    }
    if (parentheses)
      param += infoString[i];
    else
      property += infoString[i];
  }

  if (parentheses)
    CLog::Log(LOGERROR, "unmatched parentheses in %s", infoString.c_str());

  if (!property.empty())
  {
    StringUtils::ToLower(property);
    info.emplace_back(Property(property, param));
  }
}

/// \brief Translates a string as given by the skin into an int that we use for more
/// efficient retrieval of data.
int CGUIInfoManager::TranslateSingleString(const std::string &strCondition)
{
  bool listItemDependent;
  return TranslateSingleString(strCondition, listItemDependent);
}

int CGUIInfoManager::TranslateSingleString(const std::string &strCondition, bool &listItemDependent)
{
  /* We need to disable caching in INFO::InfoBool::Get if either of the following are true:
   *  1. if condition is between LISTITEM_START and LISTITEM_END
   *  2. if condition is string or integer the corresponding label is between LISTITEM_START and LISTITEM_END
   *  This is achieved by setting the bool pointed at by listItemDependent, either here or in a recursive call
   */
  // trim whitespaces
  std::string strTest = strCondition;
  StringUtils::Trim(strTest);

  std::vector< Property> info;
  SplitInfoString(strTest, info);

  if (info.empty())
    return 0;

  const Property &cat = info[0];
  if (info.size() == 1)
  { // single category
    if (cat.name == "false" || cat.name == "no")
      return SYSTEM_ALWAYS_FALSE;
    else if (cat.name == "true" || cat.name == "yes")
      return SYSTEM_ALWAYS_TRUE;
  }
  else if (info.size() == 2)
  {
    const Property &prop = info[1];
    if (cat.name == "string")
    {
      if (prop.name == "isempty")
      {
        return AddMultiInfo(CGUIInfo(STRING_IS_EMPTY, TranslateSingleString(prop.param(), listItemDependent)));
      }
      else if (prop.num_params() == 2)
      {
        for (const infomap& string_bool : string_bools)
        {
          if (prop.name == string_bool.str)
          {
            int data1 = TranslateSingleString(prop.param(0), listItemDependent);
            // pipe our original string through the localize parsing then make it lowercase (picks up $LBRACKET etc.)
            std::string label = CGUIInfoLabel::GetLabel(prop.param(1));
            StringUtils::ToLower(label);
            // 'true', 'false', 'yes', 'no' are valid strings, do not resolve them to SYSTEM_ALWAYS_TRUE or SYSTEM_ALWAYS_FALSE
            if (label != "true" && label != "false" && label != "yes" && label != "no")
            {
              int data2 = TranslateSingleString(prop.param(1), listItemDependent);
              if (data2 > 0)
                return AddMultiInfo(CGUIInfo(string_bool.val, data1, -data2));
            }
            return AddMultiInfo(CGUIInfo(string_bool.val, data1, label));
          }
        }
      }
    }
    if (cat.name == "integer")
    {
      for (const infomap& integer_bool : integer_bools)
      {
        if (prop.name == integer_bool.str)
        {
          int data1 = TranslateSingleString(prop.param(0), listItemDependent);
          int data2 = atoi(prop.param(1).c_str());
          return AddMultiInfo(CGUIInfo(integer_bool.val, data1, data2));
        }
      }
    }
    else if (cat.name == "player")
    {
      for (const infomap& player_label : player_labels)
      {
        if (prop.name == player_label.str)
          return player_label.val;
      }
      for (const infomap& player_time : player_times)
      {
        if (prop.name == player_time.str)
          return AddMultiInfo(CGUIInfo(player_time.val, TranslateTimeFormat(prop.param())));
      }
      if (prop.name == "process" && prop.num_params())
      {
        for (const infomap& player_proces : player_process)
        {
          if (StringUtils::EqualsNoCase(prop.param(), player_proces.str))
            return player_proces.val;
        }
      }
      if (prop.num_params() == 1)
      {
        for (const infomap& i : player_param)
        {
          if (prop.name == i.str)
            return AddMultiInfo(CGUIInfo(i.val, prop.param()));
        }
      }
    }
    else if (cat.name == "weather")
    {
      for (const infomap& i : weather)
      {
        if (prop.name == i.str)
          return i.val;
      }
    }
    else if (cat.name == "network")
    {
      for (const infomap& network_label : network_labels)
      {
        if (prop.name == network_label.str)
          return network_label.val;
      }
    }
    else if (cat.name == "musicpartymode")
    {
      for (const infomap& i : musicpartymode)
      {
        if (prop.name == i.str)
          return i.val;
      }
    }
    else if (cat.name == "system")
    {
      for (const infomap& system_label : system_labels)
      {
        if (prop.name == system_label.str)
          return system_label.val;
      }
      if (prop.num_params() == 1)
      {
        const std::string &param = prop.param();
        if (prop.name == "getbool")
        {
          std::string paramCopy = param;
          StringUtils::ToLower(paramCopy);
          return AddMultiInfo(CGUIInfo(SYSTEM_GET_BOOL, paramCopy));
        }
        for (const infomap& i : system_param)
        {
          if (prop.name == i.str)
            return AddMultiInfo(CGUIInfo(i.val, param));
        }
        if (prop.name == "memory")
        {
          if (param == "free")
            return SYSTEM_FREE_MEMORY;
          else if (param == "free.percent")
            return SYSTEM_FREE_MEMORY_PERCENT;
          else if (param == "used")
            return SYSTEM_USED_MEMORY;
          else if (param == "used.percent")
            return SYSTEM_USED_MEMORY_PERCENT;
          else if (param == "total")
            return SYSTEM_TOTAL_MEMORY;
        }
        else if (prop.name == "addontitle")
        {
          // Example: System.AddonTitle(Skin.String(HomeVideosButton1)) => skin string HomeVideosButton1 holds an addon identifier string
          int infoLabel = TranslateSingleString(param, listItemDependent);
          if (infoLabel > 0)
            return AddMultiInfo(CGUIInfo(SYSTEM_ADDON_TITLE, infoLabel, 0));
          std::string label = CGUIInfoLabel::GetLabel(param);
          StringUtils::ToLower(label);
          return AddMultiInfo(CGUIInfo(SYSTEM_ADDON_TITLE, label, 1));
        }
        else if (prop.name == "addonicon")
        {
          int infoLabel = TranslateSingleString(param, listItemDependent);
          if (infoLabel > 0)
            return AddMultiInfo(CGUIInfo(SYSTEM_ADDON_ICON, infoLabel, 0));
          std::string label = CGUIInfoLabel::GetLabel(param);
          StringUtils::ToLower(label);
          return AddMultiInfo(CGUIInfo(SYSTEM_ADDON_ICON, label, 1));
        }
        else if (prop.name == "addonversion")
        {
          int infoLabel = TranslateSingleString(param, listItemDependent);
          if (infoLabel > 0)
            return AddMultiInfo(CGUIInfo(SYSTEM_ADDON_VERSION, infoLabel, 0));
          std::string label = CGUIInfoLabel::GetLabel(param);
          StringUtils::ToLower(label);
          return AddMultiInfo(CGUIInfo(SYSTEM_ADDON_VERSION, label, 1));
        }
        else if (prop.name == "idletime")
          return AddMultiInfo(CGUIInfo(SYSTEM_IDLE_TIME, atoi(param.c_str())));
      }
      if (prop.name == "alarmlessorequal" && prop.num_params() == 2)
        return AddMultiInfo(CGUIInfo(SYSTEM_ALARM_LESS_OR_EQUAL, prop.param(0), atoi(prop.param(1).c_str())));
      else if (prop.name == "date")
      {
        if (prop.num_params() == 2)
          return AddMultiInfo(CGUIInfo(SYSTEM_DATE, StringUtils::DateStringToYYYYMMDD(prop.param(0)) % 10000, StringUtils::DateStringToYYYYMMDD(prop.param(1)) % 10000));
        else if (prop.num_params() == 1)
        {
          int dateformat = StringUtils::DateStringToYYYYMMDD(prop.param(0));
          if (dateformat <= 0) // not concrete date
            return AddMultiInfo(CGUIInfo(SYSTEM_DATE, prop.param(0), -1));
          else
            return AddMultiInfo(CGUIInfo(SYSTEM_DATE, dateformat % 10000));
        }
        return SYSTEM_DATE;
      }
      else if (prop.name == "time")
      {
        if (prop.num_params() == 0)
          return AddMultiInfo(CGUIInfo(SYSTEM_TIME, TIME_FORMAT_GUESS));
        if (prop.num_params() == 1)
        {
          TIME_FORMAT timeFormat = TranslateTimeFormat(prop.param(0));
          if (timeFormat == TIME_FORMAT_GUESS)
            return AddMultiInfo(CGUIInfo(SYSTEM_TIME, StringUtils::TimeStringToSeconds(prop.param(0))));
          return AddMultiInfo(CGUIInfo(SYSTEM_TIME, timeFormat));
        }
        else
          return AddMultiInfo(CGUIInfo(SYSTEM_TIME, StringUtils::TimeStringToSeconds(prop.param(0)), StringUtils::TimeStringToSeconds(prop.param(1))));
      }
    }
    else if (cat.name == "library")
    {
      if (prop.name == "isscanning")
        return LIBRARY_IS_SCANNING;
      else if (prop.name == "isscanningvideo")
        return LIBRARY_IS_SCANNING_VIDEO; //! @todo change to IsScanning(Video)
      else if (prop.name == "isscanningmusic")
        return LIBRARY_IS_SCANNING_MUSIC;
      else if (prop.name == "hascontent" && prop.num_params())
      {
        std::string cat = prop.param(0);
        StringUtils::ToLower(cat);
        if (cat == "music")
          return LIBRARY_HAS_MUSIC;
        else if (cat == "video")
          return LIBRARY_HAS_VIDEO;
        else if (cat == "movies")
          return LIBRARY_HAS_MOVIES;
        else if (cat == "tvshows")
          return LIBRARY_HAS_TVSHOWS;
        else if (cat == "musicvideos")
          return LIBRARY_HAS_MUSICVIDEOS;
        else if (cat == "moviesets")
          return LIBRARY_HAS_MOVIE_SETS;
        else if (cat == "singles")
          return LIBRARY_HAS_SINGLES;
        else if (cat == "compilations")
          return LIBRARY_HAS_COMPILATIONS;
        else if (cat == "role" && prop.num_params() > 1)
          return AddMultiInfo(CGUIInfo(LIBRARY_HAS_ROLE, prop.param(1), 0));
      }
    }
    else if (cat.name == "musicplayer")
    {
      for (const infomap& player_time : player_times) //! @todo remove these, they're repeats
      {
        if (prop.name == player_time.str)
          return AddMultiInfo(CGUIInfo(player_time.val, TranslateTimeFormat(prop.param())));
      }
      if (prop.name == "content" && prop.num_params())
        return AddMultiInfo(CGUIInfo(MUSICPLAYER_CONTENT, prop.param(), 0));
      else if (prop.name == "property")
      {
        if (StringUtils::EqualsNoCase(prop.param(), "fanart_image"))
          return AddMultiInfo(CGUIInfo(PLAYER_ITEM_ART, "fanart"));

        return AddMultiInfo(CGUIInfo(MUSICPLAYER_PROPERTY, prop.param()));
      }
      return TranslateMusicPlayerString(prop.name);
    }
    else if (cat.name == "videoplayer")
    {
      if (prop.name != "starttime") // player.starttime is semantically different from videoplayer.starttime which has its own implementation!
      {
        for (const infomap& player_time : player_times) //! @todo remove these, they're repeats
        {
          if (prop.name == player_time.str)
            return AddMultiInfo(CGUIInfo(player_time.val, TranslateTimeFormat(prop.param())));
        }
      }
      if (prop.name == "content" && prop.num_params())
      {
        return AddMultiInfo(CGUIInfo(VIDEOPLAYER_CONTENT, prop.param(), 0));
      }
      for (const infomap& i : videoplayer)
      {
        if (prop.name == i.str)
          return i.val;
      }
    }
    else if (cat.name == "retroplayer")
    {
      for (const infomap& i : retroplayer)
      {
        if (prop.name == i.str)
          return i.val;
      }
    }
    else if (cat.name == "slideshow")
    {
      for (const infomap& i : slideshow)
      {
        if (prop.name == i.str)
          return i.val;
      }
    }
    else if (cat.name == "container")
    {
      for (const infomap& i : mediacontainer) // these ones don't have or need an id
      {
        if (prop.name == i.str)
          return i.val;
      }
      int id = atoi(cat.param().c_str());
      for (const infomap& container_bool : container_bools) // these ones can have an id (but don't need to?)
      {
        if (prop.name == container_bool.str)
          return id ? AddMultiInfo(CGUIInfo(container_bool.val, id)) : container_bool.val;
      }
      for (const infomap& container_int : container_ints) // these ones can have an int param on the property
      {
        if (prop.name == container_int.str)
          return AddMultiInfo(CGUIInfo(container_int.val, id, atoi(prop.param().c_str())));
      }
      for (const infomap& i : container_str) // these ones have a string param on the property
      {
        if (prop.name == i.str)
          return AddMultiInfo(CGUIInfo(i.val, id, prop.param()));
      }
      if (prop.name == "sortdirection")
      {
        SortOrder order = SortOrderNone;
        if (StringUtils::EqualsNoCase(prop.param(), "ascending"))
          order = SortOrderAscending;
        else if (StringUtils::EqualsNoCase(prop.param(), "descending"))
          order = SortOrderDescending;
        return AddMultiInfo(CGUIInfo(CONTAINER_SORT_DIRECTION, order));
      }
    }
    else if (cat.name == "listitem" ||
             cat.name == "listitemposition" ||
             cat.name == "listitemnowrap" ||
             cat.name == "listitemabsolute")
    {
      int ret = TranslateListItem(cat, prop, 0, false);
      if (ret)
        listItemDependent = true;
      return ret;
    }
    else if (cat.name == "visualisation")
    {
      for (const infomap& i : visualisation)
      {
        if (prop.name == i.str)
          return i.val;
      }
    }
    else if (cat.name == "fanart")
    {
      for (const infomap& fanart_label : fanart_labels)
      {
        if (prop.name == fanart_label.str)
          return fanart_label.val;
      }
    }
    else if (cat.name == "skin")
    {
      for (const infomap& skin_label : skin_labels)
      {
        if (prop.name == skin_label.str)
          return skin_label.val;
      }
      if (prop.num_params())
      {
        if (prop.name == "string")
        {
          if (prop.num_params() == 2)
            return AddMultiInfo(CGUIInfo(SKIN_STRING_IS_EQUAL, CSkinSettings::GetInstance().TranslateString(prop.param(0)), prop.param(1)));
          else
            return AddMultiInfo(CGUIInfo(SKIN_STRING, CSkinSettings::GetInstance().TranslateString(prop.param(0))));
        }
        if (prop.name == "hassetting")
          return AddMultiInfo(CGUIInfo(SKIN_BOOL, CSkinSettings::GetInstance().TranslateBool(prop.param(0))));
        else if (prop.name == "hastheme")
          return AddMultiInfo(CGUIInfo(SKIN_HAS_THEME, prop.param(0)));
      }
    }
    else if (cat.name == "window")
    {
      if (prop.name == "property" && prop.num_params() == 1)
      { //! @todo this doesn't support foo.xml
        int winID = cat.param().empty() ? 0 : CWindowTranslator::TranslateWindow(cat.param());
        if (winID != WINDOW_INVALID)
          return AddMultiInfo(CGUIInfo(WINDOW_PROPERTY, winID, prop.param()));
      }
      for (const infomap& window_bool : window_bools)
      {
        if (prop.name == window_bool.str)
        { //! @todo The parameter for these should really be on the first not the second property
          if (prop.param().find("xml") != std::string::npos)
            return AddMultiInfo(CGUIInfo(window_bool.val, 0, prop.param()));
          int winID = prop.param().empty() ? WINDOW_INVALID : CWindowTranslator::TranslateWindow(prop.param());
          return AddMultiInfo(CGUIInfo(window_bool.val, winID, 0));
        }
      }
    }
    else if (cat.name == "control")
    {
      for (const infomap& control_label : control_labels)
      {
        if (prop.name == control_label.str)
        { //! @todo The parameter for these should really be on the first not the second property
          int controlID = atoi(prop.param().c_str());
          if (controlID)
            return AddMultiInfo(CGUIInfo(control_label.val, controlID, 0));
          return 0;
        }
      }
    }
    else if (cat.name == "controlgroup" && prop.name == "hasfocus")
    {
      int groupID = atoi(cat.param().c_str());
      if (groupID)
        return AddMultiInfo(CGUIInfo(CONTROL_GROUP_HAS_FOCUS, groupID, atoi(prop.param(0).c_str())));
    }
    else if (cat.name == "playlist")
    {
      int ret = -1;
      for (const infomap& i : playlist)
      {
        if (prop.name == i.str)
        {
          ret = i.val;
          break;
        }
      }
      if (ret >= 0)
      {
        if (prop.num_params() <= 0)
          return ret;
        else
        {
          int playlistid = PLAYLIST_NONE;
          if (StringUtils::EqualsNoCase(prop.param(), "video"))
            playlistid = PLAYLIST_VIDEO;
          else if (StringUtils::EqualsNoCase(prop.param(), "music"))
            playlistid = PLAYLIST_MUSIC;

          if (playlistid > PLAYLIST_NONE)
            return AddMultiInfo(CGUIInfo(ret, playlistid, 1));
        }
      }
    }
    else if (cat.name == "pvr")
    {
      for (const infomap& i : pvr)
      {
        if (prop.name == i.str)
          return i.val;
      }
      for (const infomap& pvr_time : pvr_times)
      {
        if (prop.name == pvr_time.str)
          return AddMultiInfo(CGUIInfo(pvr_time.val, TranslateTimeFormat(prop.param())));
      }
    }
    else if (cat.name == "rds")
    {
      if (prop.name == "getline")
        return AddMultiInfo(CGUIInfo(RDS_GET_RADIOTEXT_LINE, atoi(prop.param(0).c_str())));

      for (const infomap& rd : rds)
      {
        if (prop.name == rd.str)
          return rd.val;
      }
    }
  }
  else if (info.size() == 3 || info.size() == 4)
  {
    if (info[0].name == "system" && info[1].name == "platform")
    { //! @todo replace with a single system.platform
      std::string platform = info[2].name;
      if (platform == "linux")
      {
        if (info.size() == 4)
        {
          std::string device = info[3].name;
          if (device == "raspberrypi")
            return SYSTEM_PLATFORM_LINUX_RASPBERRY_PI;
        }
        else return SYSTEM_PLATFORM_LINUX;
      }
      else if (platform == "windows")
        return SYSTEM_PLATFORM_WINDOWS;
      else if (platform == "uwp")
        return SYSTEM_PLATFORM_UWP;
      else if (platform == "darwin")
        return SYSTEM_PLATFORM_DARWIN;
      else if (platform == "osx")
        return SYSTEM_PLATFORM_DARWIN_OSX;
      else if (platform == "ios")
        return SYSTEM_PLATFORM_DARWIN_IOS;
      else if (platform == "tvos")
        return SYSTEM_PLATFORM_DARWIN_TVOS;
      else if (platform == "android")
        return SYSTEM_PLATFORM_ANDROID;
    }
    if (info[0].name == "musicplayer")
    { //! @todo these two don't allow duration(foo) and also don't allow more than this number of levels...
      if (info[1].name == "position")
      {
        int position = atoi(info[1].param().c_str());
        int value = TranslateMusicPlayerString(info[2].name); // musicplayer.position(foo).bar
        return AddMultiInfo(CGUIInfo(value, 0, position));
      }
      else if (info[1].name == "offset")
      {
        int position = atoi(info[1].param().c_str());
        int value = TranslateMusicPlayerString(info[2].name); // musicplayer.offset(foo).bar
        return AddMultiInfo(CGUIInfo(value, 1, position));
      }
    }
    else if (info[0].name == "container")
    {
      if (info[1].name == "listitem" ||
          info[1].name == "listitemposition" ||
          info[1].name == "listitemabsolute" ||
          info[1].name == "listitemnowrap")
      {
        int id = atoi(info[0].param().c_str());
        int ret = TranslateListItem(info[1], info[2], id, true);
        if (ret)
          listItemDependent = true;
        return ret;
      }
    }
    else if (info[0].name == "control")
    {
      const Property &prop = info[1];
      for (const infomap& control_label : control_labels)
      {
        if (prop.name == control_label.str)
        { //! @todo The parameter for these should really be on the first not the second property
          int controlID = atoi(prop.param().c_str());
          if (controlID)
            return AddMultiInfo(CGUIInfo(control_label.val, controlID, atoi(info[2].param(0).c_str())));
          return 0;
        }
      }
    }
  }

  return 0;
}

int CGUIInfoManager::TranslateListItem(const Property& cat, const Property& prop, int id, bool container)
{
  int ret = 0;
  std::string data3;
  int data4 = 0;
  if (prop.num_params() == 1)
  {
    // special case: map 'property(fanart_image)' to 'art(fanart)'
    if (prop.name == "property" && StringUtils::EqualsNoCase(prop.param(), "fanart_image"))
    {
      ret = LISTITEM_ART;
      data3 = "fanart";
    }
    else if (prop.name == "property" ||
             prop.name == "art" ||
             prop.name == "rating" ||
             prop.name == "votes" ||
             prop.name == "ratingandvotes")
    {
      data3 = prop.param();
    }
    else if (prop.name == "duration" || prop.name == "nextduration")
    {
      data4 = TranslateTimeFormat(prop.param());
    }
  }

  if (ret == 0)
  {
    for (const infomap& listitem_label : listitem_labels) // these ones don't have or need an id
    {
      if (prop.name == listitem_label.str)
      {
        ret = listitem_label.val;
        break;
      }
    }
  }

  if (ret)
  {
    int offset = std::atoi(cat.param().c_str());

    int flags = 0;
    if (cat.name == "listitem")
      flags = INFOFLAG_LISTITEM_WRAP;
    else if (cat.name == "listitemposition")
      flags = INFOFLAG_LISTITEM_POSITION;
    else if (cat.name == "listitemabsolute")
      flags = INFOFLAG_LISTITEM_ABSOLUTE;
    else if (cat.name == "listitemnowrap")
      flags = INFOFLAG_LISTITEM_NOWRAP;

    if (container)
      flags |= INFOFLAG_LISTITEM_CONTAINER;

    return AddMultiInfo(CGUIInfo(ret, id, offset, flags, data3, data4));
  }

  return 0;
}

int CGUIInfoManager::TranslateMusicPlayerString(const std::string &info) const
{
  for (const infomap& i : musicplayer)
  {
    if (info == i.str)
      return i.val;
  }
  return 0;
}

TIME_FORMAT CGUIInfoManager::TranslateTimeFormat(const std::string &format)
{
  if (format.empty())
    return TIME_FORMAT_GUESS;
  else if (StringUtils::EqualsNoCase(format, "hh"))
    return TIME_FORMAT_HH;
  else if (StringUtils::EqualsNoCase(format, "mm"))
    return TIME_FORMAT_MM;
  else if (StringUtils::EqualsNoCase(format, "ss"))
    return TIME_FORMAT_SS;
  else if (StringUtils::EqualsNoCase(format, "hh:mm"))
    return TIME_FORMAT_HH_MM;
  else if (StringUtils::EqualsNoCase(format, "mm:ss"))
    return TIME_FORMAT_MM_SS;
  else if (StringUtils::EqualsNoCase(format, "hh:mm:ss"))
    return TIME_FORMAT_HH_MM_SS;
  else if (StringUtils::EqualsNoCase(format, "hh:mm:ss xx"))
    return TIME_FORMAT_HH_MM_SS_XX;
  else if (StringUtils::EqualsNoCase(format, "h"))
    return TIME_FORMAT_H;
  else if (StringUtils::EqualsNoCase(format, "m"))
    return TIME_FORMAT_M;
  else if (StringUtils::EqualsNoCase(format, "h:mm:ss"))
    return TIME_FORMAT_H_MM_SS;
  else if (StringUtils::EqualsNoCase(format, "h:mm:ss xx"))
    return TIME_FORMAT_H_MM_SS_XX;
  else if (StringUtils::EqualsNoCase(format, "xx"))
    return TIME_FORMAT_XX;
  else if (StringUtils::EqualsNoCase(format, "secs"))
    return TIME_FORMAT_SECS;
  else if (StringUtils::EqualsNoCase(format, "mins"))
    return TIME_FORMAT_MINS;
  else if (StringUtils::EqualsNoCase(format, "hours"))
    return TIME_FORMAT_HOURS;
  return TIME_FORMAT_GUESS;
}

std::string CGUIInfoManager::GetLabel(int info, int contextWindow, std::string *fallback) const
{
  if (info >= CONDITIONAL_LABEL_START && info <= CONDITIONAL_LABEL_END)
  {
    return GetSkinVariableString(info, false);
  }
  else if (info >= MULTI_INFO_START && info <= MULTI_INFO_END)
  {
    return GetMultiInfoLabel(m_multiInfo[info - MULTI_INFO_START], contextWindow);
  }
  else if (info >= LISTITEM_START && info <= LISTITEM_END)
  {
    const CGUIListItemPtr item = GUIINFO::GetCurrentListItem(contextWindow);
    if (item && item->IsFileItem())
      return GetItemLabel(static_cast<CFileItem*>(item.get()), contextWindow, info, fallback);
  }

  std::string strLabel;
  m_infoProviders.GetLabel(strLabel, m_currentFile, contextWindow, CGUIInfo(info), fallback);
  return strLabel;
}

bool CGUIInfoManager::GetInt(int &value, int info, int contextWindow, const CGUIListItem *item /* = nullptr */) const
{
  if (info >= MULTI_INFO_START && info <= MULTI_INFO_END)
  {
    return GetMultiInfoInt(value, m_multiInfo[info - MULTI_INFO_START], contextWindow, item);
  }
  else if (info >= LISTITEM_START && info <= LISTITEM_END)
  {
    CGUIListItemPtr itemPtr;
    if (!item)
    {
      itemPtr = GUIINFO::GetCurrentListItem(contextWindow);
      item = itemPtr.get();
    }
    return GetItemInt(value, item, contextWindow, info);
  }

  value = 0;
  return m_infoProviders.GetInt(value, m_currentFile, contextWindow, CGUIInfo(info));
}

INFO::InfoPtr CGUIInfoManager::Register(const std::string &expression, int context)
{
  std::string condition(CGUIInfoLabel::ReplaceLocalize(expression));
  StringUtils::Trim(condition);

  if (condition.empty())
    return INFO::InfoPtr();

  CSingleLock lock(m_critInfo);
  std::pair<INFOBOOLTYPE::iterator, bool> res;

  if (condition.find_first_of("|+[]!") != condition.npos)
    res = m_bools.insert(std::make_shared<InfoExpression>(condition, context, m_refreshCounter));
  else
    res = m_bools.insert(std::make_shared<InfoSingle>(condition, context, m_refreshCounter));

  if (res.second)
    res.first->get()->Initialize();

  return *(res.first);
}

bool CGUIInfoManager::EvaluateBool(const std::string &expression, int contextWindow /* = 0 */, const CGUIListItemPtr &item /* = nullptr */)
{
  INFO::InfoPtr info = Register(expression, contextWindow);
  if (info)
    return info->Get(item.get());
  return false;
}

bool CGUIInfoManager::GetBool(int condition1, int contextWindow, const CGUIListItem *item)
{
  bool bReturn = false;
  int condition = std::abs(condition1);

  if (condition >= LISTITEM_START && condition < LISTITEM_END)
  {
    CGUIListItemPtr itemPtr;
    if (!item)
    {
      itemPtr = GUIINFO::GetCurrentListItem(contextWindow);
      item = itemPtr.get();
    }
    bReturn = GetItemBool(item, contextWindow, condition);
  }
  else if (condition >= MULTI_INFO_START && condition <= MULTI_INFO_END)
  {
    bReturn = GetMultiInfoBool(m_multiInfo[condition - MULTI_INFO_START], contextWindow, item);
  }
  else if (!m_infoProviders.GetBool(bReturn, m_currentFile, contextWindow, CGUIInfo(condition)))
  {
    // default: use integer value different from 0 as true
    int val;
    bReturn = GetInt(val, condition) && val != 0;
  }

  return (condition1 < 0) ? !bReturn : bReturn;
}

bool CGUIInfoManager::GetMultiInfoBool(const CGUIInfo &info, int contextWindow, const CGUIListItem *item)
{
  bool bReturn = false;
  int condition = std::abs(info.m_info);

  if (condition >= LISTITEM_START && condition <= LISTITEM_END)
  {
    CGUIListItemPtr itemPtr;
    if (!item)
    {
      itemPtr = GUIINFO::GetCurrentListItem(contextWindow, info.GetData1(), info.GetData2(), info.GetInfoFlag());
      item = itemPtr.get();
    }
    if (item)
    {
      if (condition == LISTITEM_PROPERTY)
      {
        if (item->HasProperty(info.GetData3()))
          bReturn = item->GetProperty(info.GetData3()).asBoolean();
      }
      else
        bReturn = GetItemBool(item, contextWindow, condition);
    }
    else
    {
      bReturn = false;
    }
  }
  else if (!m_infoProviders.GetBool(bReturn, m_currentFile, contextWindow, info))
  {
    switch (condition)
    {
      case STRING_IS_EMPTY:
        // note: Get*Image() falls back to Get*Label(), so this should cover all of them
        if (item && item->IsFileItem() && IsListItemInfo(info.GetData1()))
          bReturn = GetItemImage(item, contextWindow, info.GetData1()).empty();
        else
          bReturn = GetImage(info.GetData1(), contextWindow).empty();
        break;
      case STRING_IS_EQUAL:
        {
          std::string compare;
          if (info.GetData2() < 0) // info labels are stored with negative numbers
          {
            int info2 = -info.GetData2();
            CGUIListItemPtr item2;

            if (IsListItemInfo(info2))
            {
              int iResolvedInfo2 = ResolveMultiInfo(info2);
              if (iResolvedInfo2 != 0)
              {
                const GUIINFO::CGUIInfo& resolvedInfo2 = m_multiInfo[iResolvedInfo2 - MULTI_INFO_START];
                if (resolvedInfo2.GetInfoFlag() & INFOFLAG_LISTITEM_CONTAINER)
                  item2 = GUIINFO::GetCurrentListItem(contextWindow, resolvedInfo2.GetData1()); // data1 contains the container id
              }
            }

            if (item2 && item2->IsFileItem())
              compare = GetItemImage(item2.get(), contextWindow, info2);
            else if (item && item->IsFileItem())
              compare = GetItemImage(item, contextWindow, info2);
            else
              compare = GetImage(info2, contextWindow);
          }
          else if (!info.GetData3().empty())
          { // conditional string
            compare = info.GetData3();
          }
          if (item && item->IsFileItem() && IsListItemInfo(info.GetData1()))
            bReturn = StringUtils::EqualsNoCase(GetItemImage(item, contextWindow, info.GetData1()), compare);
          else
            bReturn = StringUtils::EqualsNoCase(GetImage(info.GetData1(), contextWindow), compare);
        }
        break;
      case INTEGER_IS_EQUAL:
      case INTEGER_GREATER_THAN:
      case INTEGER_GREATER_OR_EQUAL:
      case INTEGER_LESS_THAN:
      case INTEGER_LESS_OR_EQUAL:
        {
          int integer = 0;
          if (!GetInt(integer, info.GetData1(), contextWindow, item))
          {
            std::string value;
            if (item && item->IsFileItem() && IsListItemInfo(info.GetData1()))
              value = GetItemImage(item, contextWindow, info.GetData1());
            else
              value = GetImage(info.GetData1(), contextWindow);

            // Handle the case when a value contains time separator (:). This makes Integer.IsGreater
            // useful for Player.Time* members without adding a separate set of members returning time in seconds
            if (value.find_first_of( ':' ) != value.npos)
              integer = StringUtils::TimeStringToSeconds(value);
            else
              integer = atoi(value.c_str());
          }

          // compare
          if (condition == INTEGER_IS_EQUAL)
            bReturn = integer == info.GetData2();
          else if (condition == INTEGER_GREATER_THAN)
            bReturn = integer > info.GetData2();
          else if (condition == INTEGER_GREATER_OR_EQUAL)
            bReturn = integer >= info.GetData2();
          else if (condition == INTEGER_LESS_THAN)
            bReturn = integer < info.GetData2();
          else if (condition == INTEGER_LESS_OR_EQUAL)
            bReturn = integer <= info.GetData2();
        }
        break;
      case STRING_STARTS_WITH:
      case STRING_ENDS_WITH:
      case STRING_CONTAINS:
        {
          std::string compare = info.GetData3();
          // our compare string is already in lowercase, so lower case our label as well
          // as std::string::Find() is case sensitive
          std::string label;
          if (item && item->IsFileItem() && IsListItemInfo(info.GetData1()))
            label = GetItemImage(item, contextWindow, info.GetData1());
          else
            label = GetImage(info.GetData1(), contextWindow);
          StringUtils::ToLower(label);
          if (condition == STRING_STARTS_WITH)
            bReturn = StringUtils::StartsWith(label, compare);
          else if (condition == STRING_ENDS_WITH)
            bReturn = StringUtils::EndsWith(label, compare);
          else
            bReturn = label.find(compare) != std::string::npos;
        }
        break;
    }
  }
  return (info.m_info < 0) ? !bReturn : bReturn;
}

bool CGUIInfoManager::GetMultiInfoInt(int &value, const CGUIInfo &info, int contextWindow, const CGUIListItem *item) const
{
  if (info.m_info >= LISTITEM_START && info.m_info <= LISTITEM_END)
  {
    CGUIListItemPtr itemPtr;
    if (!item)
    {
      itemPtr = GUIINFO::GetCurrentListItem(contextWindow, info.GetData1(), info.GetData2(), info.GetInfoFlag());
      item = itemPtr.get();
    }
    if (item)
    {
      if (info.m_info == LISTITEM_PROPERTY)
      {
        if (item->HasProperty(info.GetData3()))
        {
          value = item->GetProperty(info.GetData3()).asInteger();
          return true;
        }
        return false;
      }
      else
        return GetItemInt(value, item, contextWindow, info.m_info);
    }
    else
    {
      return false;
    }
  }

  return m_infoProviders.GetInt(value, m_currentFile, contextWindow, info);
}

std::string CGUIInfoManager::GetMultiInfoLabel(const CGUIInfo &constinfo, int contextWindow, std::string *fallback) const
{
  CGUIInfo info(constinfo);

  if (info.m_info >= LISTITEM_START && info.m_info <= LISTITEM_END)
  {
    const CGUIListItemPtr item = GUIINFO::GetCurrentListItem(contextWindow, info.GetData1(), info.GetData2(), info.GetInfoFlag());
    if (item)
    {
      // Image prioritizes images over labels (in the case of music item ratings for instance)
      return GetMultiInfoItemImage(dynamic_cast<CFileItem*>(item.get()), contextWindow, info, fallback);
    }
    else
    {
      return std::string();
    }
  }
  else if (info.m_info == SYSTEM_ADDON_TITLE ||
           info.m_info == SYSTEM_ADDON_ICON ||
           info.m_info == SYSTEM_ADDON_VERSION)
  {
    if (info.GetData2() == 0)
    {
      // resolve the addon id
      const std::string addonId = GetLabel(info.GetData1(), contextWindow);
      info = CGUIInfo(info.m_info, addonId);
    }
  }

  std::string strValue;
  m_infoProviders.GetLabel(strValue, m_currentFile, contextWindow, info, fallback);
  return strValue;
}

/// \brief Obtains the filename of the image to show from whichever subsystem is needed
std::string CGUIInfoManager::GetImage(int info, int contextWindow, std::string *fallback)
{
  if (info >= CONDITIONAL_LABEL_START && info <= CONDITIONAL_LABEL_END)
  {
    return GetSkinVariableString(info, true);
  }
  else if (info >= MULTI_INFO_START && info <= MULTI_INFO_END)
  {
    return GetMultiInfoLabel(m_multiInfo[info - MULTI_INFO_START], contextWindow, fallback);
  }
  else if (info == LISTITEM_THUMB ||
           info == LISTITEM_ICON ||
           info == LISTITEM_ACTUAL_ICON ||
           info == LISTITEM_OVERLAY ||
           info == LISTITEM_ART)
  {
    const CGUIListItemPtr item = GUIINFO::GetCurrentListItem(contextWindow);
    if (item && item->IsFileItem())
      return GetItemImage(item.get(), contextWindow, info, fallback);
  }

  return GetLabel(info, contextWindow, fallback);
}

void CGUIInfoManager::ResetCurrentItem()
{
  m_currentFile->Reset();
  m_infoProviders.InitCurrentItem(nullptr);
}

void CGUIInfoManager::UpdateCurrentItem(const CFileItem &item)
{
  m_currentFile->UpdateInfo(item);
}

void CGUIInfoManager::SetCurrentItem(const CFileItem &item)
{
  *m_currentFile = item;
  m_currentFile->FillInDefaultIcon();

  m_infoProviders.InitCurrentItem(m_currentFile);

  SetChanged();
  NotifyObservers(ObservableMessageCurrentItem);
  // @todo this should be handled by one of the observers above and forwarded
  CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::Info, "xbmc", "OnChanged");
}

void CGUIInfoManager::SetCurrentAlbumThumb(const std::string &thumbFileName)
{
  if (XFILE::CFile::Exists(thumbFileName))
    m_currentFile->SetArt("thumb", thumbFileName);
  else
  {
    m_currentFile->SetArt("thumb", "");
    m_currentFile->FillInDefaultIcon();
  }
}

void CGUIInfoManager::Clear()
{
  CSingleLock lock(m_critInfo);
  m_skinVariableStrings.clear();

  /*
    Erase any info bools that are unused. We do this repeatedly as each run
    will remove those bools that are no longer dependencies of other bools
    in the vector.
   */
  INFOBOOLTYPE swapList(&InfoBoolComparator);
  do
  {
    swapList.clear();
    for (auto &item : m_bools)
      if (!item.unique())
        swapList.insert(item);
    m_bools.swap(swapList);
  } while (swapList.size() != m_bools.size());

  // log which ones are used - they should all be gone by now
  for (INFOBOOLTYPE::const_iterator i = m_bools.begin(); i != m_bools.end(); ++i)
    CLog::Log(LOGDEBUG, "Infobool '%s' still used by %u instances", (*i)->GetExpression().c_str(), (unsigned int) i->use_count());
}

void CGUIInfoManager::UpdateAVInfo()
{
  if (CServiceBroker::GetDataCacheCore().HasAVInfoChanges())
  {
    VideoStreamInfo video;
    AudioStreamInfo audio;
    SubtitleStreamInfo subtitle;

    g_application.GetAppPlayer().GetVideoStreamInfo(CURRENT_STREAM, video);
    g_application.GetAppPlayer().GetAudioStreamInfo(CURRENT_STREAM, audio);
    g_application.GetAppPlayer().GetSubtitleStreamInfo(CURRENT_STREAM, subtitle);

    m_infoProviders.UpdateAVInfo(audio, video, subtitle);
  }
}

int CGUIInfoManager::AddMultiInfo(const CGUIInfo &info)
{
  // check to see if we have this info already
  for (unsigned int i = 0; i < m_multiInfo.size(); ++i)
    if (m_multiInfo[i] == info)
      return static_cast<int>(i) + MULTI_INFO_START;
  // return the new offset
  m_multiInfo.emplace_back(info);
  int id = static_cast<int>(m_multiInfo.size()) + MULTI_INFO_START - 1;
  if (id > MULTI_INFO_END)
    CLog::Log(LOGERROR, "%s - too many multiinfo bool/labels in this skin", __FUNCTION__);
  return id;
}

int CGUIInfoManager::ResolveMultiInfo(int info) const
{
  int iLastInfo = 0;

  int iResolvedInfo = info;
  while (iResolvedInfo >= MULTI_INFO_START && iResolvedInfo <= MULTI_INFO_END)
  {
    iLastInfo = iResolvedInfo;
    iResolvedInfo = m_multiInfo[iResolvedInfo - MULTI_INFO_START].m_info;
  }

  return iLastInfo;
}

bool CGUIInfoManager::IsListItemInfo(int info) const
{
  int iResolvedInfo = info;
  while (iResolvedInfo >= MULTI_INFO_START && iResolvedInfo <= MULTI_INFO_END)
    iResolvedInfo = m_multiInfo[iResolvedInfo - MULTI_INFO_START].m_info;

  return (iResolvedInfo >= LISTITEM_START && iResolvedInfo <= LISTITEM_END);
}

bool CGUIInfoManager::GetItemInt(int &value, const CGUIListItem *item, int contextWindow, int info) const
{
  value = 0;

  if (!item)
    return false;

  return m_infoProviders.GetInt(value, item, contextWindow, CGUIInfo(info));
}

std::string CGUIInfoManager::GetItemLabel(const CFileItem *item, int contextWindow, int info, std::string *fallback /* = nullptr */) const
{
  return GetMultiInfoItemLabel(item, contextWindow, CGUIInfo(info), fallback);
}

std::string CGUIInfoManager::GetMultiInfoItemLabel(const CFileItem *item, int contextWindow, const CGUIInfo &info, std::string *fallback /* = nullptr */) const
{
  if (!item)
    return std::string();

  std::string value;

  if (info.m_info >= CONDITIONAL_LABEL_START && info.m_info <= CONDITIONAL_LABEL_END)
  {
    return GetSkinVariableString(info.m_info, false, item);
  }
  else if (info.m_info >= MULTI_INFO_START && info.m_info <= MULTI_INFO_END)
  {
    return GetMultiInfoItemLabel(item, contextWindow, m_multiInfo[info.m_info - MULTI_INFO_START], fallback);
  }
  else if (!m_infoProviders.GetLabel(value, item, contextWindow, info, fallback))
  {
    switch (info.m_info)
    {
      case LISTITEM_PROPERTY:
        return item->GetProperty(info.GetData3()).asString();
      case LISTITEM_LABEL:
        return item->GetLabel();
      case LISTITEM_LABEL2:
        return item->GetLabel2();
      case LISTITEM_FILENAME:
      case LISTITEM_FILE_EXTENSION:
      {
        std::string strFile = URIUtils::GetFileName(item->GetPath());
        if (info.m_info == LISTITEM_FILE_EXTENSION)
        {
          std::string strExtension = URIUtils::GetExtension(strFile);
          return StringUtils::TrimLeft(strExtension, ".");
        }
        return strFile;
      }
      case LISTITEM_DATE:
        if (item->m_dateTime.IsValid())
          return item->m_dateTime.GetAsLocalizedDate();
        break;
      case LISTITEM_DATETIME:
        if (item->m_dateTime.IsValid())
          return item->m_dateTime.GetAsLocalizedDateTime();
        break;
      case LISTITEM_SIZE:
        if (!item->m_bIsFolder || item->m_dwSize)
          return StringUtils::SizeToString(item->m_dwSize);
        break;
      case LISTITEM_PROGRAM_COUNT:
        return StringUtils::Format("%i", item->m_iprogramCount);
      case LISTITEM_ACTUAL_ICON:
        return item->GetIconImage();
      case LISTITEM_ICON:
      {
        std::string strThumb = item->GetArt("thumb");
        if (strThumb.empty())
          strThumb = item->GetIconImage();
        if (fallback)
          *fallback = item->GetIconImage();
        return strThumb;
      }
      case LISTITEM_ART:
        return item->GetArt(info.GetData3());
      case LISTITEM_OVERLAY:
        return item->GetOverlayImage();
      case LISTITEM_THUMB:
        return item->GetArt("thumb");
      case LISTITEM_FOLDERPATH:
        return CURL(item->GetPath()).GetWithoutUserDetails();
      case LISTITEM_FOLDERNAME:
      case LISTITEM_PATH:
      {
        std::string path;
        URIUtils::GetParentPath(item->GetPath(), path);
        path = CURL(path).GetWithoutUserDetails();
        if (info.m_info == LISTITEM_FOLDERNAME)
        {
          URIUtils::RemoveSlashAtEnd(path);
          path = URIUtils::GetFileName(path);
        }
        return path;
      }
      case LISTITEM_FILENAME_AND_PATH:
      {
        std::string path = item->GetPath();
        path = CURL(path).GetWithoutUserDetails();
        return path;
      }
      case LISTITEM_SORT_LETTER:
      {
        std::string letter;
        std::wstring character(1, item->GetSortLabel()[0]);
        StringUtils::ToUpper(character);
        g_charsetConverter.wToUTF8(character, letter);
        return letter;
      }
      case LISTITEM_STARTTIME:
      {
        if (item->m_dateTime.IsValid())
          return item->m_dateTime.GetAsLocalizedTime("", false);
        break;
      }
      case LISTITEM_STARTDATE:
      {
        if (item->m_dateTime.IsValid())
          return item->m_dateTime.GetAsLocalizedDate(true);
        break;
      }
    }
  }

  return value;
}

std::string CGUIInfoManager::GetItemImage(const CGUIListItem *item, int contextWindow, int info, std::string *fallback /*= nullptr*/) const
{
  if (!item || !item->IsFileItem())
    return std::string();

  return GetMultiInfoItemImage(static_cast<const CFileItem*>(item), contextWindow, CGUIInfo(info), fallback);
}

std::string CGUIInfoManager::GetMultiInfoItemImage(const CFileItem *item, int contextWindow, const CGUIInfo &info, std::string *fallback /*= nullptr*/) const
{
  if (info.m_info >= CONDITIONAL_LABEL_START && info.m_info <= CONDITIONAL_LABEL_END)
  {
    return GetSkinVariableString(info.m_info, true, item);
  }
  else if (info.m_info >= MULTI_INFO_START && info.m_info <= MULTI_INFO_END)
  {
    return GetMultiInfoItemImage(item, contextWindow, m_multiInfo[info.m_info - MULTI_INFO_START], fallback);
  }

  return GetMultiInfoItemLabel(item, contextWindow, info, fallback);
}

bool CGUIInfoManager::GetItemBool(const CGUIListItem *item, int contextWindow, int condition) const
{
  if (!item)
    return false;

  bool value = false;
  if (!m_infoProviders.GetBool(value, item, contextWindow, CGUIInfo(condition)))
  {
    switch (condition)
    {
      case LISTITEM_ISSELECTED:
        return item->IsSelected();
      case LISTITEM_IS_FOLDER:
        return item->m_bIsFolder;
      case LISTITEM_IS_PARENTFOLDER:
      {
        if (item->IsFileItem())
        {
          const CFileItem *pItem = static_cast<const CFileItem *>(item);
          return pItem->IsParentFolder();
        }
        break;
      }
    }
  }

  return value;
}

void CGUIInfoManager::ResetCache()
{
  // mark our infobools as dirty
  CSingleLock lock(m_critInfo);
  ++m_refreshCounter;
}

void CGUIInfoManager::SetCurrentVideoTag(const CVideoInfoTag &tag)
{
  m_currentFile->SetFromVideoInfoTag(tag);
  m_currentFile->m_lStartOffset = 0;
}

void CGUIInfoManager::SetCurrentSongTag(const MUSIC_INFO::CMusicInfoTag &tag)
{
  m_currentFile->SetFromMusicInfoTag(tag);
  m_currentFile->m_lStartOffset = 0;
}

const MUSIC_INFO::CMusicInfoTag* CGUIInfoManager::GetCurrentSongTag() const
{
  if (m_currentFile->HasMusicInfoTag())
    return m_currentFile->GetMusicInfoTag();

  return nullptr;
}

const CVideoInfoTag* CGUIInfoManager::GetCurrentMovieTag() const
{
  if (m_currentFile->HasVideoInfoTag())
    return m_currentFile->GetVideoInfoTag();

  return nullptr;
}

int CGUIInfoManager::RegisterSkinVariableString(const CSkinVariableString* info)
{
  if (!info)
    return 0;

  CSingleLock lock(m_critInfo);
  m_skinVariableStrings.emplace_back(*info);
  delete info;
  return CONDITIONAL_LABEL_START + m_skinVariableStrings.size() - 1;
}

int CGUIInfoManager::TranslateSkinVariableString(const std::string& name, int context)
{
  for (std::vector<CSkinVariableString>::const_iterator it = m_skinVariableStrings.begin();
       it != m_skinVariableStrings.end(); ++it)
  {
    if (StringUtils::EqualsNoCase(it->GetName(), name) && it->GetContext() == context)
      return it - m_skinVariableStrings.begin() + CONDITIONAL_LABEL_START;
  }
  return 0;
}

std::string CGUIInfoManager::GetSkinVariableString(int info,
                                                   bool preferImage /*= false*/,
                                                   const CGUIListItem *item /*= nullptr*/) const
{
  info -= CONDITIONAL_LABEL_START;
  if (info >= 0 && info < static_cast<int>(m_skinVariableStrings.size()))
    return m_skinVariableStrings[info].GetValue(preferImage, item);

  return "";
}

bool CGUIInfoManager::ConditionsChangedValues(const std::map<INFO::InfoPtr, bool>& map)
{
  for (std::map<INFO::InfoPtr, bool>::const_iterator it = map.begin() ; it != map.end() ; ++it)
  {
    if (it->first->Get() != it->second)
      return true;
  }
  return false;
}

int CGUIInfoManager::GetMessageMask()
{
  return TMSG_MASK_GUIINFOMANAGER;
}

void CGUIInfoManager::OnApplicationMessage(KODI::MESSAGING::ThreadMessage* pMsg)
{
  switch (pMsg->dwMessage)
  {
  case TMSG_GUI_INFOLABEL:
  {
    if (pMsg->lpVoid)
    {
      auto infoLabels = static_cast<std::vector<std::string>*>(pMsg->lpVoid);
      for (auto& param : pMsg->params)
        infoLabels->emplace_back(GetLabel(TranslateString(param)));
    }
  }
  break;

  case TMSG_GUI_INFOBOOL:
  {
    if (pMsg->lpVoid)
    {
      auto infoLabels = static_cast<std::vector<bool>*>(pMsg->lpVoid);
      for (auto& param : pMsg->params)
        infoLabels->push_back(EvaluateBool(param));
    }
  }
  break;

  case TMSG_UPDATE_CURRENT_ITEM:
  {
    CFileItem* item = static_cast<CFileItem*>(pMsg->lpVoid);
    if (!item)
      return;

    if (pMsg->param1 == 1 && item->HasMusicInfoTag()) // only grab music tag
      SetCurrentSongTag(*item->GetMusicInfoTag());
    else if (pMsg->param1 == 2 && item->HasVideoInfoTag()) // only grab video tag
      SetCurrentVideoTag(*item->GetVideoInfoTag());
    else
      SetCurrentItem(*item);

    delete item;
  }
  break;

  default:
    break;
  }
}

void CGUIInfoManager::RegisterInfoProvider(IGUIInfoProvider *provider)
{
  if (!CServiceBroker::GetWinSystem())
    return;

  CSingleLock lock(CServiceBroker::GetWinSystem()->GetGfxContext());

  m_infoProviders.RegisterProvider(provider, false);
}

void CGUIInfoManager::UnregisterInfoProvider(IGUIInfoProvider *provider)
{
  if (!CServiceBroker::GetWinSystem())
    return;

  CSingleLock lock(CServiceBroker::GetWinSystem()->GetGfxContext());

  m_infoProviders.UnregisterProvider(provider);
}
