/*!
\file GUIInfoManager.h
\brief
*/

#ifndef GUIINFOMANAGER_H_
#define GUIINFOMANAGER_H_

/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "Temperature.h"
#include "threads/CriticalSection.h"
#include "guilib/IMsgTargetCallback.h"
#include "inttypes.h"
#include "XBDateTime.h"
#include "utils/Observer.h"
#include "interfaces/info/SkinVariable.h"

#include <list>
#include <map>

namespace MUSIC_INFO
{
  class CMusicInfoTag;
}
class CVideoInfoTag;
class CFileItem;
class CGUIListItem;
class CDateTime;
namespace INFO
{
  class InfoBool;
  class InfoSingle;
}

// conditions for window retrieval
#define WINDOW_CONDITION_HAS_LIST_ITEMS  1
#define WINDOW_CONDITION_IS_MEDIA_WINDOW 2

#define PLAYER_HAS_MEDIA              1
#define PLAYER_HAS_AUDIO              2
#define PLAYER_HAS_VIDEO              3
#define PLAYER_PLAYING                4
#define PLAYER_PAUSED                 5
#define PLAYER_REWINDING              6
#define PLAYER_REWINDING_2x           7
#define PLAYER_REWINDING_4x           8
#define PLAYER_REWINDING_8x           9
#define PLAYER_REWINDING_16x         10
#define PLAYER_REWINDING_32x         11
#define PLAYER_FORWARDING            12
#define PLAYER_FORWARDING_2x         13
#define PLAYER_FORWARDING_4x         14
#define PLAYER_FORWARDING_8x         15
#define PLAYER_FORWARDING_16x        16
#define PLAYER_FORWARDING_32x        17
#define PLAYER_CAN_RECORD            18
#define PLAYER_RECORDING             19
#define PLAYER_CACHING               20
#define PLAYER_DISPLAY_AFTER_SEEK    21
#define PLAYER_PROGRESS              22
#define PLAYER_SEEKBAR               23
#define PLAYER_SEEKTIME              24
#define PLAYER_SEEKING               25
#define PLAYER_SHOWTIME              26
#define PLAYER_TIME                  27
#define PLAYER_TIME_REMAINING        28
#define PLAYER_DURATION              29
#define PLAYER_SHOWCODEC             30
#define PLAYER_SHOWINFO              31
#define PLAYER_VOLUME                32
#define PLAYER_MUTED                 33
#define PLAYER_HASDURATION           34
#define PLAYER_CHAPTER               35
#define PLAYER_CHAPTERCOUNT          36
#define PLAYER_TIME_SPEED            37
#define PLAYER_FINISH_TIME           38
#define PLAYER_CACHELEVEL            39
#define PLAYER_STAR_RATING           40
#define PLAYER_CHAPTERNAME           41
#define PLAYER_SUBTITLE_DELAY        42
#define PLAYER_AUDIO_DELAY           43
#define PLAYER_PASSTHROUGH           44
#define PLAYER_PATH                  45
#define PLAYER_FILEPATH              46
#define PLAYER_SEEKOFFSET            47
#define PLAYER_PROGRESS_CACHE        48

#define WEATHER_CONDITIONS          100
#define WEATHER_TEMPERATURE         101
#define WEATHER_LOCATION            102
#define WEATHER_IS_FETCHED          103
#define WEATHER_FANART_CODE         104
#define WEATHER_PLUGIN              105

#define SYSTEM_TEMPERATURE_UNITS    106
#define SYSTEM_PROGRESS_BAR         107
#define SYSTEM_LANGUAGE             108
#define SYSTEM_TIME                 110
#define SYSTEM_DATE                 111
#define SYSTEM_CPU_TEMPERATURE      112
#define SYSTEM_GPU_TEMPERATURE      113
#define SYSTEM_FAN_SPEED            114
#define SYSTEM_FREE_SPACE_C         115
// #define SYSTEM_FREE_SPACE_D         116 //116 is reserved for space on D
#define SYSTEM_FREE_SPACE_E         117
#define SYSTEM_FREE_SPACE_F         118
#define SYSTEM_FREE_SPACE_G         119
#define SYSTEM_BUILD_VERSION        120
#define SYSTEM_BUILD_DATE           121
#define SYSTEM_ETHERNET_LINK_ACTIVE 122
#define SYSTEM_FPS                  123
#define SYSTEM_ALWAYS_TRUE          125   // useful for <visible fade="10" start="hidden">true</visible>, to fade in a control
#define SYSTEM_ALWAYS_FALSE         126   // used for <visible fade="10">false</visible>, to fade out a control (ie not particularly useful!)
#define SYSTEM_MEDIA_DVD            127
#define SYSTEM_DVDREADY             128
#define SYSTEM_HAS_ALARM            129
#define SYSTEM_SCREEN_MODE          132
#define SYSTEM_SCREEN_WIDTH         133
#define SYSTEM_SCREEN_HEIGHT        134
#define SYSTEM_CURRENT_WINDOW       135
#define SYSTEM_CURRENT_CONTROL      136
#define SYSTEM_DVD_LABEL            138
#define SYSTEM_HAS_DRIVE_F          139
#define SYSTEM_HASLOCKS             140
#define SYSTEM_ISMASTER             141
#define SYSTEM_TRAYOPEN             142
#define SYSTEM_SHOW_EXIT_BUTTON     143
#define SYSTEM_ALARM_POS            144
#define SYSTEM_LOGGEDON             145
#define SYSTEM_PROFILENAME          146
#define SYSTEM_PROFILETHUMB         147
#define SYSTEM_HAS_LOGINSCREEN      148
#define SYSTEM_HAS_DRIVE_G          149
#define SYSTEM_HDD_SMART            150
#define SYSTEM_HDD_TEMPERATURE      151
#define SYSTEM_HDD_MODEL            152
#define SYSTEM_HDD_SERIAL           153
#define SYSTEM_HDD_FIRMWARE         154
#define SYSTEM_HDD_PASSWORD         156
#define SYSTEM_HDD_LOCKSTATE        157
#define SYSTEM_HDD_LOCKKEY          158
#define SYSTEM_INTERNET_STATE       159
#define LCD_PLAY_ICON               160
#define LCD_PROGRESS_BAR            161
#define LCD_CPU_TEMPERATURE         162
#define LCD_GPU_TEMPERATURE         163
#define LCD_HDD_TEMPERATURE         164
#define LCD_FAN_SPEED               165
#define LCD_DATE                    166
#define LCD_TIME_21                 172 // Small bigfont
#define LCD_TIME_22                 173
#define LCD_TIME_W21                174 // Medum bigfont
#define LCD_TIME_W22                175
#define LCD_TIME_41                 176 // Big bigfont
#define LCD_TIME_42                 177
#define LCD_TIME_43                 178
#define LCD_TIME_44                 179
#define SYSTEM_ALARM_LESS_OR_EQUAL  180
#define SYSTEM_PROFILECOUNT         181
#define SYSTEM_ISFULLSCREEN         182
#define SYSTEM_ISSTANDALONE         183
#define SYSTEM_ISINHIBIT            184
#define SYSTEM_HAS_SHUTDOWN         185

#define NETWORK_IP_ADDRESS          190
#define NETWORK_MAC_ADDRESS         191
#define NETWORK_IS_DHCP             192
#define NETWORK_LINK_STATE          193
#define NETWORK_SUBNET_MASK         194
#define NETWORK_GATEWAY_ADDRESS     195
#define NETWORK_DNS1_ADDRESS        196
#define NETWORK_DNS2_ADDRESS        197
#define NETWORK_DHCP_ADDRESS        198

#define MUSICPLAYER_TITLE           200
#define MUSICPLAYER_ALBUM           201
#define MUSICPLAYER_ARTIST          202
#define MUSICPLAYER_GENRE           203
#define MUSICPLAYER_YEAR            204
#define MUSICPLAYER_DURATION        205
#define MUSICPLAYER_TRACK_NUMBER    208
#define MUSICPLAYER_COVER           210
#define MUSICPLAYER_BITRATE         211
#define MUSICPLAYER_PLAYLISTLEN     212
#define MUSICPLAYER_PLAYLISTPOS     213
#define MUSICPLAYER_CHANNELS        214
#define MUSICPLAYER_BITSPERSAMPLE   215
#define MUSICPLAYER_SAMPLERATE      216
#define MUSICPLAYER_CODEC           217
#define MUSICPLAYER_DISC_NUMBER     218
#define MUSICPLAYER_RATING          219
#define MUSICPLAYER_COMMENT         220
#define MUSICPLAYER_LYRICS          221
#define MUSICPLAYER_HASPREVIOUS     222
#define MUSICPLAYER_HASNEXT         223
#define MUSICPLAYER_EXISTS          224
#define MUSICPLAYER_PLAYLISTPLAYING 225
#define MUSICPLAYER_ALBUM_ARTIST    226
#define MUSICPLAYER_PLAYCOUNT       227
#define MUSICPLAYER_LASTPLAYED      228
#define MUSICPLAYER_CHANNEL_NAME    229
#define MUSICPLAYER_CHANNEL_NUMBER  230
#define MUSICPLAYER_CHANNEL_GROUP   231

#define VIDEOPLAYER_TITLE             250
#define VIDEOPLAYER_GENRE             251
#define VIDEOPLAYER_DIRECTOR          252
#define VIDEOPLAYER_YEAR              253
#define VIDEOPLAYER_HASSUBTITLES      254
#define VIDEOPLAYER_SUBTITLESENABLED  255
#define VIDEOPLAYER_COVER             258
#define VIDEOPLAYER_USING_OVERLAYS    259
#define VIDEOPLAYER_ISFULLSCREEN      260
#define VIDEOPLAYER_HASMENU           261
#define VIDEOPLAYER_PLAYLISTLEN       262
#define VIDEOPLAYER_PLAYLISTPOS       263
#define VIDEOPLAYER_EVENT             264
#define VIDEOPLAYER_ORIGINALTITLE     265
#define VIDEOPLAYER_PLOT              266
#define VIDEOPLAYER_PLOT_OUTLINE      267
#define VIDEOPLAYER_EPISODE           268
#define VIDEOPLAYER_SEASON            269
#define VIDEOPLAYER_RATING            270
#define VIDEOPLAYER_TVSHOW            271
#define VIDEOPLAYER_PREMIERED         272
#define VIDEOPLAYER_CONTENT           273
#define VIDEOPLAYER_STUDIO            274
#define VIDEOPLAYER_MPAA              275
#define VIDEOPLAYER_CAST              276
#define VIDEOPLAYER_CAST_AND_ROLE     277
#define VIDEOPLAYER_ARTIST            278
#define VIDEOPLAYER_ALBUM             279
#define VIDEOPLAYER_WRITER            280
#define VIDEOPLAYER_TAGLINE           281
#define VIDEOPLAYER_HAS_INFO          282
#define VIDEOPLAYER_TOP250            283
#define VIDEOPLAYER_RATING_AND_VOTES  284
#define VIDEOPLAYER_TRAILER           285
#define VIDEOPLAYER_VIDEO_CODEC       286
#define VIDEOPLAYER_VIDEO_RESOLUTION  287
#define VIDEOPLAYER_AUDIO_CODEC       288
#define VIDEOPLAYER_AUDIO_CHANNELS    289
#define VIDEOPLAYER_VIDEO_ASPECT      290
#define VIDEOPLAYER_HASTELETEXT       291
#define VIDEOPLAYER_COUNTRY           292
#define VIDEOPLAYER_PLAYCOUNT         293
#define VIDEOPLAYER_LASTPLAYED        294

#define VIDEOPLAYER_STARTTIME         295
#define VIDEOPLAYER_ENDTIME           296
#define VIDEOPLAYER_NEXT_TITLE        297
#define VIDEOPLAYER_NEXT_GENRE        298
#define VIDEOPLAYER_NEXT_PLOT         299
#define VIDEOPLAYER_NEXT_PLOT_OUTLINE 300
#define VIDEOPLAYER_NEXT_STARTTIME    301
#define VIDEOPLAYER_NEXT_ENDTIME      302
#define VIDEOPLAYER_NEXT_DURATION     303
#define VIDEOPLAYER_CHANNEL_NAME      304
#define VIDEOPLAYER_CHANNEL_NUMBER    305
#define VIDEOPLAYER_CHANNEL_GROUP     306
#define VIDEOPLAYER_PARENTAL_RATING   307
#define VIDEOPLAYER_HAS_EPG           308

#define AUDIOSCROBBLER_ENABLED      325
#define AUDIOSCROBBLER_CONN_STATE   326
#define AUDIOSCROBBLER_SUBMIT_INT   327
#define AUDIOSCROBBLER_FILES_CACHED 328
#define AUDIOSCROBBLER_SUBMIT_STATE 329
#define LASTFM_RADIOPLAYING         330
#define LASTFM_CANLOVE              331
#define LASTFM_CANBAN               332

#define CONTAINER_SCROLL_PREVIOUS   345 // NOTE: These 5 must be kept in this consecutive order
#define CONTAINER_MOVE_PREVIOUS     346
#define CONTAINER_STATIC            347
#define CONTAINER_MOVE_NEXT         348
#define CONTAINER_SCROLL_NEXT       349

#define CONTAINER_HASFILES          351
#define CONTAINER_HASFOLDERS        352
#define CONTAINER_STACKED           353
#define CONTAINER_FOLDERNAME        354
#define CONTAINER_SCROLLING         355
#define CONTAINER_PLUGINNAME        356
#define CONTAINER_PROPERTY          357
#define CONTAINER_SORT_DIRECTION    358
#define CONTAINER_NUM_ITEMS         359
#define CONTAINER_FOLDERTHUMB       360
#define CONTAINER_FOLDERPATH        361
#define CONTAINER_CONTENT           362
#define CONTAINER_HAS_THUMB         363
#define CONTAINER_SORT_METHOD       364

#define CONTAINER_HAS_FOCUS         367
#define CONTAINER_ROW               368
#define CONTAINER_COLUMN            369
#define CONTAINER_POSITION          370
#define CONTAINER_VIEWMODE          371
#define CONTAINER_HAS_NEXT          372
#define CONTAINER_HAS_PREVIOUS      373
#define CONTAINER_SUBITEM           374
#define CONTAINER_TVSHOWTHUMB       375
#define CONTAINER_NUM_PAGES         376
#define CONTAINER_CURRENT_PAGE      377
#define CONTAINER_SEASONTHUMB       378
#define CONTAINER_SHOWPLOT          379
#define CONTAINER_TOTALTIME         380

#define MUSICPM_ENABLED             381
#define MUSICPM_SONGSPLAYED         382
#define MUSICPM_MATCHINGSONGS       383
#define MUSICPM_MATCHINGSONGSPICKED 384
#define MUSICPM_MATCHINGSONGSLEFT   385
#define MUSICPM_RELAXEDSONGSPICKED  386
#define MUSICPM_RANDOMSONGSPICKED   387

#define PLAYLIST_LENGTH             390
#define PLAYLIST_POSITION           391
#define PLAYLIST_RANDOM             392
#define PLAYLIST_REPEAT             393
#define PLAYLIST_ISRANDOM           394
#define PLAYLIST_ISREPEAT           395
#define PLAYLIST_ISREPEATONE        396

#define VISUALISATION_LOCKED        400
#define VISUALISATION_PRESET        401
#define VISUALISATION_NAME          402
#define VISUALISATION_ENABLED       403

#define STRING_IS_EMPTY             410
#define STRING_COMPARE              411
#define STRING_STR                  412
#define INTEGER_GREATER_THAN        413
#define STRING_STR_LEFT             414
#define STRING_STR_RIGHT            415

#define SKIN_BOOL                   600
#define SKIN_STRING                 601
#define SKIN_HAS_MUSIC_OVERLAY      602
#define SKIN_HAS_VIDEO_OVERLAY      603
#define SKIN_THEME                  604
#define SKIN_COLOUR_THEME           605
#define SKIN_HAS_THEME              606

#define SYSTEM_TOTAL_MEMORY         644
#define SYSTEM_CPU_USAGE            645
#define SYSTEM_USED_MEMORY_PERCENT  646
#define SYSTEM_USED_MEMORY          647
#define SYSTEM_FREE_MEMORY          648
#define SYSTEM_FREE_MEMORY_PERCENT  649
#define SYSTEM_UPTIME               654
#define SYSTEM_TOTALUPTIME          655
#define SYSTEM_CPUFREQUENCY         656
#define SYSTEM_SCREEN_RESOLUTION    659
#define SYSTEM_VIDEO_ENCODER_INFO   660
#define SYSTEM_KERNEL_VERSION       667
#define SYSTEM_FREE_SPACE           679
#define SYSTEM_USED_SPACE           680
#define SYSTEM_TOTAL_SPACE          681
#define SYSTEM_USED_SPACE_PERCENT   682
#define SYSTEM_FREE_SPACE_PERCENT   683
#define SYSTEM_DVD_TRAY_STATE       700
#define SYSTEM_GET_BOOL             704
#define SYSTEM_GET_CORE_USAGE       705
#define SYSTEM_HAS_CORE_ID          706
#define SYSTEM_RENDER_VENDOR        707
#define SYSTEM_RENDER_RENDERER      708
#define SYSTEM_RENDER_VERSION       709
#define SYSTEM_SETTING              710
#define SYSTEM_HAS_ADDON            711
#define SYSTEM_ADDON_TITLE          712
#define SYSTEM_ADDON_ICON           713
#define SYSTEM_BATTERY_LEVEL        714
#define SYSTEM_IDLE_TIME            715
#define SYSTEM_FRIENDLY_NAME        716
#define SYSTEM_SCREENSAVER_ACTIVE   717

#define LIBRARY_HAS_MUSIC           720
#define LIBRARY_HAS_VIDEO           721
#define LIBRARY_HAS_MOVIES          722
#define LIBRARY_HAS_TVSHOWS         723
#define LIBRARY_HAS_MUSICVIDEOS     724
#define LIBRARY_IS_SCANNING         725
#define LIBRARY_IS_SCANNING_VIDEO   726
#define LIBRARY_IS_SCANNING_MUSIC   727

#define SYSTEM_PLATFORM_XBOX        740
#define SYSTEM_PLATFORM_LINUX       741
#define SYSTEM_PLATFORM_WINDOWS     742
#define SYSTEM_PLATFORM_OSX         743
#define SYSTEM_PLATFORM_DARWIN_OSX  744
#define SYSTEM_PLATFORM_DARWIN_IOS  745
#define SYSTEM_PLATFORM_DARWIN_ATV2 746

#define SYSTEM_CAN_POWERDOWN        750
#define SYSTEM_CAN_SUSPEND          751
#define SYSTEM_CAN_HIBERNATE        752
#define SYSTEM_CAN_REBOOT           753

#define SLIDESHOW_ISPAUSED          800
#define SLIDESHOW_ISRANDOM          801
#define SLIDESHOW_ISACTIVE          802

#define SLIDE_INFO_START            900
#define SLIDE_INFO_END              980

#define FANART_COLOR1               1000
#define FANART_COLOR2               1001
#define FANART_COLOR3               1002
#define FANART_IMAGE                1003

#define PVR_CONDITIONS_START        1100
#define PVR_IS_RECORDING            (PVR_CONDITIONS_START)
#define PVR_HAS_TIMER               (PVR_CONDITIONS_START + 1)
#define PVR_HAS_NONRECORDING_TIMER  (PVR_CONDITIONS_START + 2)
#define PVR_HAS_EPG                 (PVR_CONDITIONS_START + 3)
#define PVR_HAS_TXT                 (PVR_CONDITIONS_START + 4)
#define PVR_HAS_DIRECTOR            (PVR_CONDITIONS_START + 5)
#define PVR_IS_PLAYING_TV           (PVR_CONDITIONS_START + 6)
#define PVR_IS_PLAYING_RADIO        (PVR_CONDITIONS_START + 7)
#define PVR_IS_PLAYING_RECORDING    (PVR_CONDITIONS_START + 8)
#define PVR_ACTUAL_STREAM_ENCRYPTED (PVR_CONDITIONS_START + 9)
#define PVR_CONDITIONS_END          PVR_ACTUAL_STREAM_ENCRYPTED

#define PVR_STRINGS_START           1200
#define PVR_NEXT_RECORDING_CHANNEL  (PVR_STRINGS_START)
#define PVR_NEXT_RECORDING_CHAN_ICO (PVR_STRINGS_START + 1)
#define PVR_NEXT_RECORDING_DATETIME (PVR_STRINGS_START + 2)
#define PVR_NEXT_RECORDING_TITLE    (PVR_STRINGS_START + 3)
#define PVR_NOW_RECORDING_CHANNEL   (PVR_STRINGS_START + 4)
#define PVR_NOW_RECORDING_CHAN_ICO  (PVR_STRINGS_START + 5)
#define PVR_NOW_RECORDING_DATETIME  (PVR_STRINGS_START + 6)
#define PVR_NOW_RECORDING_TITLE     (PVR_STRINGS_START + 7)
#define PVR_BACKEND_NAME            (PVR_STRINGS_START + 8)
#define PVR_BACKEND_VERSION         (PVR_STRINGS_START + 9)
#define PVR_BACKEND_HOST            (PVR_STRINGS_START + 10)
#define PVR_BACKEND_DISKSPACE       (PVR_STRINGS_START + 11)
#define PVR_BACKEND_CHANNELS        (PVR_STRINGS_START + 12)
#define PVR_BACKEND_TIMERS          (PVR_STRINGS_START + 13)
#define PVR_BACKEND_RECORDINGS      (PVR_STRINGS_START + 14)
#define PVR_BACKEND_NUMBER          (PVR_STRINGS_START + 15)
#define PVR_TOTAL_DISKSPACE         (PVR_STRINGS_START + 16)
#define PVR_NEXT_TIMER              (PVR_STRINGS_START + 17)
#define PVR_PLAYING_DURATION        (PVR_STRINGS_START + 18)
#define PVR_PLAYING_TIME            (PVR_STRINGS_START + 19)
#define PVR_PLAYING_PROGRESS        (PVR_STRINGS_START + 20)
#define PVR_ACTUAL_STREAM_CLIENT    (PVR_STRINGS_START + 21)
#define PVR_ACTUAL_STREAM_DEVICE    (PVR_STRINGS_START + 22)
#define PVR_ACTUAL_STREAM_STATUS    (PVR_STRINGS_START + 23)
#define PVR_ACTUAL_STREAM_SIG       (PVR_STRINGS_START + 24)
#define PVR_ACTUAL_STREAM_SNR       (PVR_STRINGS_START + 25)
#define PVR_ACTUAL_STREAM_SIG_PROGR (PVR_STRINGS_START + 26)
#define PVR_ACTUAL_STREAM_SNR_PROGR (PVR_STRINGS_START + 27)
#define PVR_ACTUAL_STREAM_BER       (PVR_STRINGS_START + 28)
#define PVR_ACTUAL_STREAM_UNC       (PVR_STRINGS_START + 29)
#define PVR_ACTUAL_STREAM_VIDEO_BR  (PVR_STRINGS_START + 30)
#define PVR_ACTUAL_STREAM_AUDIO_BR  (PVR_STRINGS_START + 31)
#define PVR_ACTUAL_STREAM_DOLBY_BR  (PVR_STRINGS_START + 32)
#define PVR_ACTUAL_STREAM_CRYPTION  (PVR_STRINGS_START + 33)
#define PVR_STRINGS_END             PVR_ACTUAL_STREAM_CRYPTION

#define WINDOW_PROPERTY             9993
#define WINDOW_IS_TOPMOST           9994
#define WINDOW_IS_VISIBLE           9995
#define WINDOW_NEXT                 9996
#define WINDOW_PREVIOUS             9997
#define WINDOW_IS_MEDIA             9998
#define WINDOW_IS_ACTIVE            9999

#define CONTROL_GET_LABEL           29996
#define CONTROL_IS_ENABLED          29997
#define CONTROL_IS_VISIBLE          29998
#define CONTROL_GROUP_HAS_FOCUS     29999
#define CONTROL_HAS_FOCUS           30000

#define VERSION_MAJOR 11
#define VERSION_MINOR 0
#define VERSION_TAG "-RC1"

#define LISTITEM_START              35000
#define LISTITEM_THUMB              (LISTITEM_START)
#define LISTITEM_LABEL              (LISTITEM_START + 1)
#define LISTITEM_TITLE              (LISTITEM_START + 2)
#define LISTITEM_TRACKNUMBER        (LISTITEM_START + 3)
#define LISTITEM_ARTIST             (LISTITEM_START + 4)
#define LISTITEM_ALBUM              (LISTITEM_START + 5)
#define LISTITEM_YEAR               (LISTITEM_START + 6)
#define LISTITEM_GENRE              (LISTITEM_START + 7)
#define LISTITEM_ICON               (LISTITEM_START + 8)
#define LISTITEM_DIRECTOR           (LISTITEM_START + 9)
#define LISTITEM_OVERLAY            (LISTITEM_START + 10)
#define LISTITEM_LABEL2             (LISTITEM_START + 11)
#define LISTITEM_FILENAME           (LISTITEM_START + 12)
#define LISTITEM_DATE               (LISTITEM_START + 13)
#define LISTITEM_SIZE               (LISTITEM_START + 14)
#define LISTITEM_RATING             (LISTITEM_START + 15)
#define LISTITEM_PROGRAM_COUNT      (LISTITEM_START + 16)
#define LISTITEM_DURATION           (LISTITEM_START + 17)
#define LISTITEM_ISPLAYING          (LISTITEM_START + 18)
#define LISTITEM_ISSELECTED         (LISTITEM_START + 19)
#define LISTITEM_PLOT               (LISTITEM_START + 20)
#define LISTITEM_PLOT_OUTLINE       (LISTITEM_START + 21)
#define LISTITEM_EPISODE            (LISTITEM_START + 22)
#define LISTITEM_SEASON             (LISTITEM_START + 23)
#define LISTITEM_TVSHOW             (LISTITEM_START + 24)
#define LISTITEM_PREMIERED          (LISTITEM_START + 25)
#define LISTITEM_COMMENT            (LISTITEM_START + 26)
#define LISTITEM_ACTUAL_ICON        (LISTITEM_START + 27)
#define LISTITEM_PATH               (LISTITEM_START + 28)
#define LISTITEM_PICTURE_PATH       (LISTITEM_START + 29)
#define LISTITEM_PICTURE_DATETIME   (LISTITEM_START + 30)
#define LISTITEM_PICTURE_RESOLUTION (LISTITEM_START + 31)
#define LISTITEM_STUDIO             (LISTITEM_START + 32)
#define LISTITEM_MPAA               (LISTITEM_START + 33)
#define LISTITEM_CAST               (LISTITEM_START + 34)
#define LISTITEM_CAST_AND_ROLE      (LISTITEM_START + 35)
#define LISTITEM_WRITER             (LISTITEM_START + 36)
#define LISTITEM_TAGLINE            (LISTITEM_START + 37)
#define LISTITEM_TOP250             (LISTITEM_START + 38)
#define LISTITEM_RATING_AND_VOTES   (LISTITEM_START + 39)
#define LISTITEM_TRAILER            (LISTITEM_START + 40)
#define LISTITEM_STAR_RATING        (LISTITEM_START + 41)
#define LISTITEM_FILENAME_AND_PATH  (LISTITEM_START + 42)
#define LISTITEM_SORT_LETTER        (LISTITEM_START + 43)
#define LISTITEM_ALBUM_ARTIST       (LISTITEM_START + 44)
#define LISTITEM_FOLDERNAME         (LISTITEM_START + 45)
#define LISTITEM_VIDEO_CODEC        (LISTITEM_START + 46)
#define LISTITEM_VIDEO_RESOLUTION   (LISTITEM_START + 47)
#define LISTITEM_VIDEO_ASPECT       (LISTITEM_START + 48)
#define LISTITEM_AUDIO_CODEC        (LISTITEM_START + 49)
#define LISTITEM_AUDIO_CHANNELS     (LISTITEM_START + 50)
#define LISTITEM_AUDIO_LANGUAGE     (LISTITEM_START + 51)
#define LISTITEM_SUBTITLE_LANGUAGE  (LISTITEM_START + 52)
#define LISTITEM_IS_FOLDER          (LISTITEM_START + 53)
#define LISTITEM_ORIGINALTITLE      (LISTITEM_START + 54)
#define LISTITEM_COUNTRY            (LISTITEM_START + 55)
#define LISTITEM_PLAYCOUNT          (LISTITEM_START + 56)
#define LISTITEM_LASTPLAYED         (LISTITEM_START + 57)
#define LISTITEM_FOLDERPATH         (LISTITEM_START + 58)
#define LISTITEM_DISC_NUMBER        (LISTITEM_START + 59)
#define LISTITEM_STARTTIME          (LISTITEM_START + 60)
#define LISTITEM_ENDTIME            (LISTITEM_START + 61)
#define LISTITEM_STARTDATE          (LISTITEM_START + 62)
#define LISTITEM_ENDDATE            (LISTITEM_START + 63)
#define LISTITEM_NEXT_TITLE         (LISTITEM_START + 64)
#define LISTITEM_NEXT_GENRE         (LISTITEM_START + 65)
#define LISTITEM_NEXT_PLOT          (LISTITEM_START + 66)
#define LISTITEM_NEXT_PLOT_OUTLINE  (LISTITEM_START + 67)
#define LISTITEM_NEXT_STARTTIME     (LISTITEM_START + 68)
#define LISTITEM_NEXT_ENDTIME       (LISTITEM_START + 69)
#define LISTITEM_NEXT_STARTDATE     (LISTITEM_START + 70)
#define LISTITEM_NEXT_ENDDATE       (LISTITEM_START + 71)
#define LISTITEM_NEXT_DURATION      (LISTITEM_START + 72)
#define LISTITEM_CHANNEL_NAME       (LISTITEM_START + 73)
#define LISTITEM_CHANNEL_NUMBER     (LISTITEM_START + 74)
#define LISTITEM_CHANNEL_GROUP      (LISTITEM_START + 75)
#define LISTITEM_HASTIMER           (LISTITEM_START + 76)
#define LISTITEM_ISRECORDING        (LISTITEM_START + 77)
#define LISTITEM_ISENCRYPTED        (LISTITEM_START + 78)
#define LISTITEM_PARENTALRATING     (LISTITEM_START + 79)
#define LISTITEM_PROGRESS           (LISTITEM_START + 80)
#define LISTITEM_FILE_EXTENSION     (LISTITEM_START + 81)
#define LISTITEM_IS_RESUMABLE       (LISTITEM_START + 82)
#define LISTITEM_PERCENT_PLAYED     (LISTITEM_START + 83)
#define LISTITEM_HAS_EPG            (LISTITEM_START + 84)

#define LISTITEM_PROPERTY_START     (LISTITEM_START + 200)
#define LISTITEM_PROPERTY_END       (LISTITEM_PROPERTY_START + 1000)
#define LISTITEM_END                (LISTITEM_PROPERTY_END)

#define MUSICPLAYER_PROPERTY_OFFSET 900 // last 100 id's reserved for musicplayer props.

#define CONDITIONAL_LABEL_START       LISTITEM_END + 1 // 36001
#define CONDITIONAL_LABEL_END         37000

// the multiple information vector
#define MULTI_INFO_START              40000
#define MULTI_INFO_END                99999
#define COMBINED_VALUES_START        100000

// forward
class CInfoLabel;
class CGUIWindow;

// Info Flags
// Stored in the top 8 bits of GUIInfo::m_data1
// therefore we only have room for 8 flags
#define INFOFLAG_LISTITEM_WRAP        ((uint32_t) (1 << 25))  // Wrap ListItem lookups
#define INFOFLAG_LISTITEM_POSITION    ((uint32_t) (1 << 26))  // Absolute ListItem lookups

// structure to hold multiple integer data
// for storage referenced from a single integer
class GUIInfo
{
public:
  GUIInfo(int info, uint32_t data1 = 0, int data2 = 0, uint32_t flag = 0)
  {
    m_info = info;
    m_data1 = data1;
    m_data2 = data2;
    if (flag)
      SetInfoFlag(flag);
  }
  bool operator ==(const GUIInfo &right) const
  {
    return (m_info == right.m_info && m_data1 == right.m_data1 && m_data2 == right.m_data2);
  };
  uint32_t GetInfoFlag() const;
  uint32_t GetData1() const;
  int GetData2() const;
  int m_info;
private:
  void SetInfoFlag(uint32_t flag);
  uint32_t m_data1;
  int m_data2;
};

/*!
 \ingroup strings
 \brief
 */
class CGUIInfoManager : public IMsgTargetCallback, public Observable
{
public:
  CGUIInfoManager(void);
  virtual ~CGUIInfoManager(void);

  void Clear();
  virtual bool OnMessage(CGUIMessage &message);

  /*! \brief Register a boolean condition/expression
   This routine allows controls or other clients of the info manager to register
   to receive updates of particular expressions, in a particular context (currently windows).

   In the future, it will allow clients to receive pushed callbacks when the expression changes.

   \param expression the boolean condition or expression
   \param context the context window
   \return an identifier used to reference this expression

   \sa GetBoolValue
   */
  unsigned int Register(const CStdString &expression, int context = 0);

  /*! \brief Get a previously registered boolean expression's value
   Checks the cache and evaluates the boolean expression if required.
   \sa Register
   */
  bool GetBoolValue(unsigned int expression, const CGUIListItem *item = NULL);

  /*! \brief Evaluate a boolean expression
   \param expression the expression to evaluate
   \param context the context in which to evaluate the expression (currently windows)
   \return the value of the evaluated expression.
   \sa Register, GetBoolValue
   */
  bool EvaluateBool(const CStdString &expression, int context = 0);

  int TranslateString(const CStdString &strCondition);

  /*! \brief Get integer value of info.
   \param value int reference to pass value of given info
   \param info id of info
   \param context the context in which to evaluate the expression (currently windows)
   \param item optional listitem if want to get listitem related int
   \return true if given info was handled
   \sa GetItemInt, GetMultiInfoInt
   */
  bool GetInt(int &value, int info, int contextWindow = 0, const CGUIListItem *item = NULL) const;
  CStdString GetLabel(int info, int contextWindow = 0);

  CStdString GetImage(int info, int contextWindow);

  CStdString GetTime(TIME_FORMAT format = TIME_FORMAT_GUESS) const;
  CStdString GetLcdTime( int _eInfo ) const;
  CStdString GetDate(bool bNumbersOnly = false);
  CStdString GetDuration(TIME_FORMAT format = TIME_FORMAT_GUESS) const;

  void SetCurrentItem(CFileItem &item);
  void ResetCurrentItem();
  // Current song stuff
  /// \brief Retrieves tag info (if necessary) and fills in our current song path.
  void SetCurrentSong(CFileItem &item);
  void SetCurrentAlbumThumb(const CStdString thumbFileName);
  void SetCurrentMovie(CFileItem &item);
  void SetCurrentSlide(CFileItem &item);
  const CFileItem &GetCurrentSlide() const;
  void ResetCurrentSlide();
  void SetCurrentSongTag(const MUSIC_INFO::CMusicInfoTag &tag);
  void SetCurrentVideoTag(const CVideoInfoTag &tag);

  const MUSIC_INFO::CMusicInfoTag *GetCurrentSongTag() const;
  const CVideoInfoTag* GetCurrentMovieTag() const;

  CStdString GetMusicLabel(int item);
  CStdString GetMusicTagLabel(int info, const CFileItem *item);
  CStdString GetVideoLabel(int item);
  CStdString GetPlaylistLabel(int item) const;
  CStdString GetMusicPartyModeLabel(int item);
  const CStdString GetMusicPlaylistInfo(const GUIInfo& info);
  CStdString GetPictureLabel(int item);

  __int64 GetPlayTime() const;  // in ms
  CStdString GetCurrentPlayTime(TIME_FORMAT format = TIME_FORMAT_GUESS) const;
  int GetPlayTimeRemaining() const;
  int GetTotalPlayTime() const;
  CStdString GetCurrentPlayTimeRemaining(TIME_FORMAT format) const;
  CStdString GetVersion();
  CStdString GetBuild();

  bool GetDisplayAfterSeek();
  void SetDisplayAfterSeek(unsigned int timeOut = 2500, int seekOffset = 0);
  void SetSeeking(bool seeking) { m_playerSeeking = seeking; };
  void SetShowTime(bool showtime) { m_playerShowTime = showtime; };
  void SetShowCodec(bool showcodec) { m_playerShowCodec = showcodec; };
  void SetShowInfo(bool showinfo) { m_playerShowInfo = showinfo; };
  void ToggleShowCodec() { m_playerShowCodec = !m_playerShowCodec; };
  bool ToggleShowInfo() { m_playerShowInfo = !m_playerShowInfo; return m_playerShowInfo; };
  bool m_performingSeek;

  std::string GetSystemHeatInfo(int info);
  CTemperature GetGPUTemperature();

  void UpdateFPS();
  inline float GetFPS() const { return m_fps; };

  void SetNextWindow(int windowID) { m_nextWindowID = windowID; };
  void SetPreviousWindow(int windowID) { m_prevWindowID = windowID; };

  void ResetCache();
  bool GetItemInt(int &value, const CGUIListItem *item, int info) const;
  CStdString GetItemLabel(const CFileItem *item, int info);
  CStdString GetItemImage(const CFileItem *item, int info);

  // Called from tuxbox service thread to update current status
  void UpdateFromTuxBox();

  /*! \brief containers call here to specify that the focus is changing
   \param id control id
   \param next true if we're moving to the next item, false if previous
   \param scrolling true if the container is scrolling, false if the movement requires no scroll
   */
  void SetContainerMoving(int id, bool next, bool scrolling)
  {
    // magnitude 2 indicates a scroll, sign indicates direction
    m_containerMoves[id] = (next ? 1 : -1) * (scrolling ? 2 : 1);
  }

  void SetLibraryBool(int condition, bool value);
  bool GetLibraryBool(int condition);
  void ResetLibraryBools();
  CStdString LocalizeTime(const CDateTime &time, TIME_FORMAT format) const;

  int TranslateSingleString(const CStdString &strCondition);

  int RegisterSkinVariableString(const INFO::CSkinVariableString* info);
  int TranslateSkinVariableString(const CStdString& name, int context);
  CStdString GetSkinVariableString(int info, bool preferImage = false, const CGUIListItem *item=NULL);
protected:
  friend class INFO::InfoSingle;
  bool GetBool(int condition, int contextWindow = 0, const CGUIListItem *item=NULL);

  // routines for window retrieval
  bool CheckWindowCondition(CGUIWindow *window, int condition) const;
  CGUIWindow *GetWindowWithCondition(int contextWindow, int condition) const;

  /*! \brief class for holding information on properties
   */
  class Property
  {
  public:
    Property(const CStdString &property, const CStdString &parameters);

    const CStdString &param(unsigned int n = 0) const;
    unsigned int num_params() const;

    CStdString name;
  private:
    std::vector<CStdString> params;
  };

  bool GetMultiInfoBool(const GUIInfo &info, int contextWindow = 0, const CGUIListItem *item = NULL);
  bool GetMultiInfoInt(int &value, const GUIInfo &info, int contextWindow = 0) const;
  CStdString GetMultiInfoLabel(const GUIInfo &info, int contextWindow = 0);
  int TranslateListItem(const Property &info);
  int TranslateMusicPlayerString(const CStdString &info) const;
  TIME_FORMAT TranslateTimeFormat(const CStdString &format);
  bool GetItemBool(const CGUIListItem *item, int condition) const;

  /*! \brief Split an info string into it's constituent parts and parameters
   Format is:
     
     info1(params1).info2(params2).info3(params3) ...
   
   where the parameters are an optional comma separated parameter list.
   
   \param infoString the original string
   \param info the resulting pairs of info and parameters.
   */
  void SplitInfoString(const CStdString &infoString, std::vector<Property> &info);

  // Conditional string parameters for testing are stored in a vector for later retrieval.
  // The offset into the string parameters array is returned.
  int ConditionalStringParameter(const CStdString &strParameter, bool caseSensitive = false);
  int AddMultiInfo(const GUIInfo &info);
  int AddListItemProp(const CStdString &str, int offset=0);

  CStdString GetAudioScrobblerLabel(int item);

  // Conditional string parameters are stored here
  CStdStringArray m_stringParameters;

  // Array of multiple information mapped to a single integer lookup
  std::vector<GUIInfo> m_multiInfo;
  std::vector<std::string> m_listitemProperties;

  CStdString m_currentMovieDuration;

  // Current playing stuff
  CFileItem* m_currentFile;
  CStdString m_currentMovieThumb;
  unsigned int m_lastMusicBitrateTime;
  unsigned int m_MusicBitrate;
  CFileItem* m_currentSlide;

  // fan stuff
  unsigned int m_lastSysHeatInfoTime;
  int m_fanSpeed;
  CTemperature m_gpuTemp;
  CTemperature m_cpuTemp;

  //Fullscreen OSD Stuff
  unsigned int m_AfterSeekTimeout;
  int m_seekOffset;
  bool m_playerSeeking;
  bool m_playerShowTime;
  bool m_playerShowCodec;
  bool m_playerShowInfo;

  // FPS counters
  float m_fps;
  unsigned int m_frameCounter;
  unsigned int m_lastFPSTime;

  std::map<int, int> m_containerMoves;  // direction of list moving
  int m_nextWindowID;
  int m_prevWindowID;

  std::vector<INFO::InfoBool*> m_bools;
  std::vector<INFO::CSkinVariableString> m_skinVariableStrings;
  unsigned int m_updateTime;

  int m_libraryHasMusic;
  int m_libraryHasMovies;
  int m_libraryHasTVShows;
  int m_libraryHasMusicVideos;

  CCriticalSection m_critInfo;
};

/*!
 \ingroup strings
 \brief
 */
extern CGUIInfoManager g_infoManager;
#endif




