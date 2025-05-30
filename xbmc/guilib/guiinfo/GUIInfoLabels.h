/*
 *  Copyright (C; 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

// clang-format off
static constexpr uint32_t PLAYER_HAS_MEDIA                  = 1;
static constexpr uint32_t PLAYER_HAS_AUDIO                  = 2;
static constexpr uint32_t PLAYER_HAS_VIDEO                  = 3;
static constexpr uint32_t PLAYER_PLAYING                    = 4;
static constexpr uint32_t PLAYER_PAUSED                     = 5;
static constexpr uint32_t PLAYER_REWINDING                  = 6;
static constexpr uint32_t PLAYER_REWINDING_2x               = 7;
static constexpr uint32_t PLAYER_REWINDING_4x               = 8;
static constexpr uint32_t PLAYER_REWINDING_8x               = 9;
static constexpr uint32_t PLAYER_REWINDING_16x              = 10;
static constexpr uint32_t PLAYER_REWINDING_32x              = 11;
static constexpr uint32_t PLAYER_FORWARDING                 = 12;
static constexpr uint32_t PLAYER_FORWARDING_2x              = 13;
static constexpr uint32_t PLAYER_FORWARDING_4x              = 14;
static constexpr uint32_t PLAYER_FORWARDING_8x              = 15;
static constexpr uint32_t PLAYER_FORWARDING_16x             = 16;
static constexpr uint32_t PLAYER_FORWARDING_32x             = 17;
// unused id 18
// unused id 19
static constexpr uint32_t PLAYER_CACHING                    = 20;
// unused id 21
static constexpr uint32_t PLAYER_PROGRESS                   = 22;
static constexpr uint32_t PLAYER_SEEKBAR                    = 23;
static constexpr uint32_t PLAYER_SEEKTIME                   = 24;
static constexpr uint32_t PLAYER_SEEKING                    = 25;
static constexpr uint32_t PLAYER_SHOWTIME                   = 26;
static constexpr uint32_t PLAYER_TIME                       = 27;
static constexpr uint32_t PLAYER_TIME_REMAINING             = 28;
static constexpr uint32_t PLAYER_DURATION                   = 29;
static constexpr uint32_t PLAYER_HASPERFORMEDSEEK           = 30;
static constexpr uint32_t PLAYER_SHOWINFO                   = 31;
static constexpr uint32_t PLAYER_VOLUME                     = 32;
static constexpr uint32_t PLAYER_MUTED                      = 33;
static constexpr uint32_t PLAYER_HASDURATION                = 34;
static constexpr uint32_t PLAYER_CHAPTER                    = 35;
static constexpr uint32_t PLAYER_CHAPTERCOUNT               = 36;
static constexpr uint32_t PLAYER_TIME_SPEED                 = 37;
static constexpr uint32_t PLAYER_FINISH_TIME                = 38;
static constexpr uint32_t PLAYER_CACHELEVEL                 = 39;
// unused id 40
static constexpr uint32_t PLAYER_CHAPTERNAME                = 41;
static constexpr uint32_t PLAYER_SUBTITLE_DELAY             = 42;
static constexpr uint32_t PLAYER_AUDIO_DELAY                = 43;
static constexpr uint32_t PLAYER_PASSTHROUGH                = 44;
// unused id 45
// unused id 46
static constexpr uint32_t PLAYER_SEEKOFFSET                 = 47;
static constexpr uint32_t PLAYER_PROGRESS_CACHE             = 48;
static constexpr uint32_t PLAYER_ITEM_ART                   = 49;
static constexpr uint32_t PLAYER_CAN_PAUSE                  = 50;
static constexpr uint32_t PLAYER_CAN_SEEK                   = 51;
static constexpr uint32_t PLAYER_START_TIME                 = 52;
// unused id 53
static constexpr uint32_t PLAYER_ISINTERNETSTREAM           = 54;
// unused id 55
static constexpr uint32_t PLAYER_SEEKSTEPSIZE               = 56;
static constexpr uint32_t PLAYER_IS_CHANNEL_PREVIEW_ACTIVE  = 57;
static constexpr uint32_t PLAYER_SUPPORTS_TEMPO             = 58;
static constexpr uint32_t PLAYER_IS_TEMPO                   = 59;
static constexpr uint32_t PLAYER_PLAYSPEED                  = 60;
static constexpr uint32_t PLAYER_SEEKNUMERIC                = 61;
static constexpr uint32_t PLAYER_HAS_GAME                   = 62;
static constexpr uint32_t PLAYER_HAS_PROGRAMS               = 63;
static constexpr uint32_t PLAYER_HAS_RESOLUTIONS            = 64;
static constexpr uint32_t PLAYER_FRAMEADVANCE               = 65;
static constexpr uint32_t PLAYER_ICON                       = 66;
// unused id 67
static constexpr uint32_t PLAYER_CHAPTERS                   = 68;
static constexpr uint32_t PLAYER_EDITLIST                   = 69;
static constexpr uint32_t PLAYER_CUTS                       = 70;
static constexpr uint32_t PLAYER_SCENE_MARKERS              = 71;
static constexpr uint32_t PLAYER_HAS_SCENE_MARKERS          = 72;
// unused id 73 to 80

// Keep player infolabels that work with offset and position together
static constexpr uint32_t PLAYER_PATH                       = 81;
static constexpr uint32_t PLAYER_FILEPATH                   = 82;
static constexpr uint32_t PLAYER_TITLE                      = 83;
static constexpr uint32_t PLAYER_FILENAME                   = 84;

// Range of player infolabels that work with offset and position
static constexpr int      PLAYER_OFFSET_POSITION_FIRST      = PLAYER_PATH;
static constexpr int      PLAYER_OFFSET_POSITION_LAST       = PLAYER_FILENAME;

static constexpr uint32_t PLAYER_IS_REMOTE                  = 85;
static constexpr uint32_t PLAYER_IS_EXTERNAL                = 86;

static constexpr uint32_t WEATHER_CONDITIONS_TEXT           = 100;
static constexpr uint32_t WEATHER_TEMPERATURE               = 101;
static constexpr uint32_t WEATHER_LOCATION                  = 102;
static constexpr uint32_t WEATHER_IS_FETCHED                = 103;
static constexpr uint32_t WEATHER_FANART_CODE               = 104;
static constexpr uint32_t WEATHER_PLUGIN                    = 105;
static constexpr uint32_t WEATHER_CONDITIONS_ICON           = 106;

static constexpr uint32_t SYSTEM_TEMPERATURE_UNITS          = 107;
static constexpr uint32_t SYSTEM_PROGRESS_BAR               = 108;
static constexpr uint32_t SYSTEM_LANGUAGE                   = 109;
static constexpr uint32_t SYSTEM_TIME                       = 110;
static constexpr uint32_t SYSTEM_DATE                       = 111;
static constexpr uint32_t SYSTEM_CPU_TEMPERATURE            = 112;
static constexpr uint32_t SYSTEM_GPU_TEMPERATURE            = 113;
static constexpr uint32_t SYSTEM_FAN_SPEED                  = 114;
static constexpr uint32_t SYSTEM_FREE_SPACE_C               = 115;
//static constexpr uint32_t SYSTEM_FREE_SPACE_D               = 116; // reserved for space on D
static constexpr uint32_t SYSTEM_FREE_SPACE_E               = 117;
static constexpr uint32_t SYSTEM_FREE_SPACE_F               = 118;
static constexpr uint32_t SYSTEM_FREE_SPACE_G               = 119;
static constexpr uint32_t SYSTEM_BUILD_VERSION              = 120;
static constexpr uint32_t SYSTEM_BUILD_DATE                 = 121;
static constexpr uint32_t SYSTEM_ETHERNET_LINK_ACTIVE       = 122;
static constexpr uint32_t SYSTEM_FPS                        = 123;
// unused id 124
static constexpr uint32_t SYSTEM_ALWAYS_TRUE                = 125;   // useful for <visible fade="10" start="hidden">true</visible>, to fade in a control
static constexpr uint32_t SYSTEM_ALWAYS_FALSE               = 126;   // used for <visible fade="10">false</visible>, to fade out a control (ie not particularly useful!;
static constexpr uint32_t SYSTEM_MEDIA_DVD                  = 127;
static constexpr uint32_t SYSTEM_DVDREADY                   = 128;
static constexpr uint32_t SYSTEM_HAS_ALARM                  = 129;
static constexpr uint32_t SYSTEM_SUPPORTS_CPU_USAGE         = 130;
// unused id 131
static constexpr uint32_t SYSTEM_SCREEN_MODE                = 132;
static constexpr uint32_t SYSTEM_SCREEN_WIDTH               = 133;
static constexpr uint32_t SYSTEM_SCREEN_HEIGHT              = 134;
static constexpr uint32_t SYSTEM_CURRENT_WINDOW             = 135;
static constexpr uint32_t SYSTEM_CURRENT_CONTROL            = 136;
static constexpr uint32_t SYSTEM_CURRENT_CONTROL_ID         = 137;
static constexpr uint32_t SYSTEM_DVD_LABEL                  = 138;
// unused id 139
static constexpr uint32_t SYSTEM_HASLOCKS                   = 140;
static constexpr uint32_t SYSTEM_ISMASTER                   = 141;
static constexpr uint32_t SYSTEM_TRAYOPEN                   = 142;
static constexpr uint32_t SYSTEM_SHOW_EXIT_BUTTON           = 143;
static constexpr uint32_t SYSTEM_ALARM_POS                  = 144;
static constexpr uint32_t SYSTEM_LOGGEDON                   = 145;
static constexpr uint32_t SYSTEM_PROFILENAME                = 146;
static constexpr uint32_t SYSTEM_PROFILETHUMB               = 147;
static constexpr uint32_t SYSTEM_HAS_LOGINSCREEN            = 148;
static constexpr uint32_t SYSTEM_HAS_ACTIVE_MODAL_DIALOG    = 149;
static constexpr uint32_t SYSTEM_HDD_SMART                  = 150;
static constexpr uint32_t SYSTEM_HDD_TEMPERATURE            = 151;
static constexpr uint32_t SYSTEM_HDD_MODEL                  = 152;
static constexpr uint32_t SYSTEM_HDD_SERIAL                 = 153;
static constexpr uint32_t SYSTEM_HDD_FIRMWARE               = 154;
static constexpr uint32_t SYSTEM_HAS_VISIBLE_MODAL_DIALOG   = 155;
static constexpr uint32_t SYSTEM_HDD_PASSWORD               = 156;
static constexpr uint32_t SYSTEM_HDD_LOCKSTATE              = 157;
static constexpr uint32_t SYSTEM_HDD_LOCKKEY                = 158;
static constexpr uint32_t SYSTEM_INTERNET_STATE             = 159;
static constexpr uint32_t SYSTEM_HAS_INPUT_HIDDEN           = 160;
static constexpr uint32_t SYSTEM_HAS_PVR_ADDON              = 161;

static constexpr uint32_t SYSTEM_ALARM_LESS_OR_EQUAL        = 180;
static constexpr uint32_t SYSTEM_PROFILECOUNT               = 181;
static constexpr uint32_t SYSTEM_ISFULLSCREEN               = 182;
static constexpr uint32_t SYSTEM_ISSTANDALONE               = 183;
static constexpr uint32_t SYSTEM_IDLE_SHUTDOWN_INHIBITED    = 184;
static constexpr uint32_t SYSTEM_HAS_SHUTDOWN               = 185;
static constexpr uint32_t SYSTEM_HAS_PVR                    = 186;
static constexpr uint32_t SYSTEM_STARTUP_WINDOW             = 187;
static constexpr uint32_t SYSTEM_STEREOSCOPIC_MODE          = 188;
static constexpr uint32_t SYSTEM_BUILD_VERSION_SHORT        = 189;

static constexpr uint32_t NETWORK_IP_ADDRESS                = 190;
static constexpr uint32_t NETWORK_MAC_ADDRESS               = 191;
static constexpr uint32_t NETWORK_IS_DHCP                   = 192;
static constexpr uint32_t NETWORK_LINK_STATE                = 193;
static constexpr uint32_t NETWORK_SUBNET_MASK               = 194;
static constexpr uint32_t NETWORK_GATEWAY_ADDRESS           = 195;
static constexpr uint32_t NETWORK_DNS1_ADDRESS              = 196;
static constexpr uint32_t NETWORK_DNS2_ADDRESS              = 197;

// Keep musicplayer infolabels that work with offset and position together
static constexpr uint32_t MUSICPLAYER_TITLE                 = 200;
static constexpr uint32_t MUSICPLAYER_ALBUM                 = 201;
static constexpr uint32_t MUSICPLAYER_ARTIST                = 202;
static constexpr uint32_t MUSICPLAYER_GENRE                 = 203;
static constexpr uint32_t MUSICPLAYER_YEAR                  = 204;
static constexpr uint32_t MUSICPLAYER_DURATION              = 205;
static constexpr uint32_t MUSICPLAYER_TRACK_NUMBER          = 208;
// unused id 209
static constexpr uint32_t MUSICPLAYER_COVER                 = 210;
static constexpr uint32_t MUSICPLAYER_BITRATE               = 211;
static constexpr uint32_t MUSICPLAYER_PLAYLISTLEN           = 212;
static constexpr uint32_t MUSICPLAYER_PLAYLISTPOS           = 213;
static constexpr uint32_t MUSICPLAYER_CHANNELS              = 214;
static constexpr uint32_t MUSICPLAYER_BITSPERSAMPLE         = 215;
static constexpr uint32_t MUSICPLAYER_SAMPLERATE            = 216;
static constexpr uint32_t MUSICPLAYER_CODEC                 = 217;
static constexpr uint32_t MUSICPLAYER_DISC_NUMBER           = 218;
static constexpr uint32_t MUSICPLAYER_RATING                = 219;
static constexpr uint32_t MUSICPLAYER_COMMENT               = 220;
static constexpr uint32_t MUSICPLAYER_LYRICS                = 221;
static constexpr uint32_t MUSICPLAYER_ALBUM_ARTIST          = 222;
static constexpr uint32_t MUSICPLAYER_PLAYCOUNT             = 223;
static constexpr uint32_t MUSICPLAYER_LASTPLAYED            = 224;
static constexpr uint32_t MUSICPLAYER_USER_RATING           = 225;
static constexpr uint32_t MUSICPLAYER_RATING_AND_VOTES      = 226;
static constexpr uint32_t MUSICPLAYER_VOTES                 = 227;
static constexpr uint32_t MUSICPLAYER_MOOD                  = 228;
static constexpr uint32_t MUSICPLAYER_CONTRIBUTORS          = 229;
static constexpr uint32_t MUSICPLAYER_CONTRIBUTOR_AND_ROLE  = 230;
static constexpr uint32_t MUSICPLAYER_DBID                  = 231;
static constexpr uint32_t MUSICPLAYER_DISC_TITLE            = 232;
static constexpr uint32_t MUSICPLAYER_RELEASEDATE           = 233;
static constexpr uint32_t MUSICPLAYER_ORIGINALDATE          = 234;
static constexpr uint32_t MUSICPLAYER_BPM                   = 235;

// Range of musicplayer infolabels that work with offset and position
static constexpr int      MUSICPLAYER_OFFSET_POSITION_FIRST = MUSICPLAYER_TITLE;
static constexpr int      MUSICPLAYER_OFFSET_POSITION_LAST  = MUSICPLAYER_BPM;

static constexpr uint32_t MUSICPLAYER_PROPERTY              = 236;
static constexpr uint32_t MUSICPLAYER_CHANNEL_NAME          = 237;
static constexpr uint32_t MUSICPLAYER_CHANNEL_GROUP         = 238;
static constexpr uint32_t MUSICPLAYER_CHANNEL_NUMBER        = 239;
static constexpr uint32_t MUSICPLAYER_TOTALDISCS            = 240;
static constexpr uint32_t MUSICPLAYER_STATIONNAME           = 241;

// Musicplayer infobools
static constexpr uint32_t MUSICPLAYER_HASPREVIOUS           = 242;
static constexpr uint32_t MUSICPLAYER_HASNEXT               = 243;
static constexpr uint32_t MUSICPLAYER_EXISTS                = 244;
static constexpr uint32_t MUSICPLAYER_PLAYLISTPLAYING       = 245;
static constexpr uint32_t MUSICPLAYER_CONTENT               = 246;
static constexpr uint32_t MUSICPLAYER_ISMULTIDISC           = 247;

// More Musicplayer infolabels
static constexpr uint32_t MUSICPLAYER_CHANNEL_LOGO          = 248;

// Videoplayer infolabels
static constexpr uint32_t VIDEOPLAYER_HDR_TYPE              = 249;
// Keep videoplayer infolabels that work with offset and position together
static constexpr uint32_t VIDEOPLAYER_TITLE                 = 250;
static constexpr uint32_t VIDEOPLAYER_GENRE                 = 251;
static constexpr uint32_t VIDEOPLAYER_DIRECTOR              = 252;
static constexpr uint32_t VIDEOPLAYER_YEAR                  = 253;
static constexpr uint32_t VIDEOPLAYER_COVER                 = 254;
static constexpr uint32_t VIDEOPLAYER_ORIGINALTITLE         = 255;
static constexpr uint32_t VIDEOPLAYER_PLOT                  = 256;
static constexpr uint32_t VIDEOPLAYER_PLOT_OUTLINE          = 257;
static constexpr uint32_t VIDEOPLAYER_EPISODE               = 258;
static constexpr uint32_t VIDEOPLAYER_SEASON                = 259;
static constexpr uint32_t VIDEOPLAYER_RATING                = 260;
static constexpr uint32_t VIDEOPLAYER_TVSHOW                = 261;
static constexpr uint32_t VIDEOPLAYER_PREMIERED             = 262;
static constexpr uint32_t VIDEOPLAYER_STUDIO                = 263;
static constexpr uint32_t VIDEOPLAYER_MPAA                  = 264;
static constexpr uint32_t VIDEOPLAYER_ARTIST                = 265;
static constexpr uint32_t VIDEOPLAYER_ALBUM                 = 266;
static constexpr uint32_t VIDEOPLAYER_WRITER                = 267;
static constexpr uint32_t VIDEOPLAYER_TAGLINE               = 268;
static constexpr uint32_t VIDEOPLAYER_TOP250                = 269;
static constexpr uint32_t VIDEOPLAYER_RATING_AND_VOTES      = 270;
static constexpr uint32_t VIDEOPLAYER_TRAILER               = 271;
static constexpr uint32_t VIDEOPLAYER_COUNTRY               = 272;
static constexpr uint32_t VIDEOPLAYER_PLAYCOUNT             = 273;
static constexpr uint32_t VIDEOPLAYER_LASTPLAYED            = 274;
static constexpr uint32_t VIDEOPLAYER_VOTES                 = 275;
static constexpr uint32_t VIDEOPLAYER_IMDBNUMBER            = 276;
static constexpr uint32_t VIDEOPLAYER_USER_RATING           = 277;
static constexpr uint32_t VIDEOPLAYER_DBID                  = 278;
static constexpr uint32_t VIDEOPLAYER_TVSHOWDBID            = 279;
static constexpr uint32_t VIDEOPLAYER_ART                   = 280;

// Range of videoplayer infolabels that work with offset and position
static constexpr int      VIDEOPLAYER_OFFSET_POSITION_FIRST = VIDEOPLAYER_TITLE;
static constexpr int      VIDEOPLAYER_OFFSET_POSITION_LAST  = VIDEOPLAYER_ART;

static constexpr uint32_t VIDEOPLAYER_AUDIO_BITRATE         = 281;
static constexpr uint32_t VIDEOPLAYER_VIDEO_BITRATE         = 282;
static constexpr uint32_t VIDEOPLAYER_VIDEO_CODEC           = 283;
static constexpr uint32_t VIDEOPLAYER_VIDEO_RESOLUTION      = 284;
static constexpr uint32_t VIDEOPLAYER_AUDIO_CODEC           = 285;
static constexpr uint32_t VIDEOPLAYER_AUDIO_CHANNELS        = 286;
static constexpr uint32_t VIDEOPLAYER_VIDEO_ASPECT          = 287;
static constexpr uint32_t VIDEOPLAYER_SUBTITLES_LANG        = 288;
// unused id 289
static constexpr uint32_t VIDEOPLAYER_AUDIO_LANG            = 290;
static constexpr uint32_t VIDEOPLAYER_STEREOSCOPIC_MODE     = 291;
static constexpr uint32_t VIDEOPLAYER_CAST                  = 292;
static constexpr uint32_t VIDEOPLAYER_CAST_AND_ROLE         = 293;
static constexpr uint32_t VIDEOPLAYER_UNIQUEID              = 294;
static constexpr uint32_t VIDEOPLAYER_AUDIOSTREAMCOUNT      = 295;
static constexpr uint32_t VIDEOPLAYER_VIDEOVERSION_NAME     = 296;
static constexpr uint32_t VIDEOPLAYER_VIDEOSTREAMCOUNT      = 297;

// Videoplayer infobools
static constexpr uint32_t VIDEOPLAYER_HASSUBTITLES          = 300;
static constexpr uint32_t VIDEOPLAYER_SUBTITLESENABLED      = 301;
static constexpr uint32_t VIDEOPLAYER_USING_OVERLAYS        = 302;
static constexpr uint32_t VIDEOPLAYER_ISFULLSCREEN          = 303;
static constexpr uint32_t VIDEOPLAYER_HASMENU               = 304;
static constexpr uint32_t VIDEOPLAYER_PLAYLISTLEN           = 305;
static constexpr uint32_t VIDEOPLAYER_PLAYLISTPOS           = 306;
static constexpr uint32_t VIDEOPLAYER_CONTENT               = 307;
static constexpr uint32_t VIDEOPLAYER_HAS_INFO              = 308;
static constexpr uint32_t VIDEOPLAYER_HASTELETEXT           = 309;
static constexpr uint32_t VIDEOPLAYER_IS_STEREOSCOPIC       = 310;
static constexpr uint32_t VIDEOPLAYER_HAS_VIDEOVERSIONS     = 311;

// PVR infolabels
static constexpr uint32_t VIDEOPLAYER_TITLE_EXTRAINFO       = 312;
static constexpr uint32_t VIDEOPLAYER_EVENT                 = 313;
static constexpr uint32_t VIDEOPLAYER_EPISODENAME           = 314;
static constexpr uint32_t VIDEOPLAYER_STARTTIME             = 315;
static constexpr uint32_t VIDEOPLAYER_ENDTIME               = 316;
static constexpr uint32_t VIDEOPLAYER_NEXT_TITLE            = 317;
static constexpr uint32_t VIDEOPLAYER_NEXT_GENRE            = 318;
static constexpr uint32_t VIDEOPLAYER_NEXT_PLOT             = 319;
static constexpr uint32_t VIDEOPLAYER_NEXT_PLOT_OUTLINE     = 320;
static constexpr uint32_t VIDEOPLAYER_NEXT_STARTTIME        = 321;
static constexpr uint32_t VIDEOPLAYER_NEXT_ENDTIME          = 322;
static constexpr uint32_t VIDEOPLAYER_NEXT_DURATION         = 323;
static constexpr uint32_t VIDEOPLAYER_CHANNEL_NAME          = 324;
static constexpr uint32_t VIDEOPLAYER_CHANNEL_GROUP         = 325;
static constexpr uint32_t VIDEOPLAYER_PARENTAL_RATING       = 326;
static constexpr uint32_t VIDEOPLAYER_CHANNEL_NUMBER        = 327;

// PVR infobools
static constexpr uint32_t VIDEOPLAYER_HAS_EPG               = 328;
static constexpr uint32_t VIDEOPLAYER_CAN_RESUME_LIVE_TV    = 329;

static constexpr uint32_t RETROPLAYER_VIDEO_FILTER          = 330;
static constexpr uint32_t RETROPLAYER_STRETCH_MODE          = 331;
static constexpr uint32_t RETROPLAYER_VIDEO_ROTATION        = 332;

// More VideoPlayer infolabels
static constexpr uint32_t VIDEOPLAYER_CHANNEL_LOGO          = 333;
static constexpr uint32_t VIDEOPLAYER_EPISODEPART           = 334;
static constexpr uint32_t VIDEOPLAYER_PARENTAL_RATING_CODE  = 335;
static constexpr uint32_t VIDEOPLAYER_PARENTAL_RATING_ICON  = 336;
static constexpr uint32_t VIDEOPLAYER_PARENTAL_RATING_SOURCE  = 337;
static constexpr uint32_t VIDEOPLAYER_MEDIAPROVIDERS        = 338;

// More MusicPlayer infolabels
static constexpr uint32_t MUSICPLAYER_MEDIAPROVIDERS        = 339;

static constexpr uint32_t CONTAINER_HAS_PARENT_ITEM         = 341;
static constexpr uint32_t CONTAINER_CAN_FILTER              = 342;
static constexpr uint32_t CONTAINER_CAN_FILTERADVANCED      = 343;
static constexpr uint32_t CONTAINER_FILTERED                = 344;

static constexpr uint32_t CONTAINER_SCROLL_PREVIOUS         = 345;
static constexpr uint32_t CONTAINER_MOVE_PREVIOUS           = 346;
// unused id 347
static constexpr uint32_t CONTAINER_MOVE_NEXT               = 348;
static constexpr uint32_t CONTAINER_SCROLL_NEXT             = 349;
static constexpr uint32_t CONTAINER_ISUPDATING              = 350;
static constexpr uint32_t CONTAINER_HASFILES                = 351;
static constexpr uint32_t CONTAINER_HASFOLDERS              = 352;
static constexpr uint32_t CONTAINER_STACKED                 = 353;
static constexpr uint32_t CONTAINER_FOLDERNAME              = 354;
static constexpr uint32_t CONTAINER_SCROLLING               = 355;
static constexpr uint32_t CONTAINER_PLUGINNAME              = 356;
static constexpr uint32_t CONTAINER_PROPERTY                = 357;
static constexpr uint32_t CONTAINER_SORT_DIRECTION          = 358;
static constexpr uint32_t CONTAINER_NUM_ITEMS               = 359;
static constexpr uint32_t CONTAINER_FOLDERPATH              = 360;
static constexpr uint32_t CONTAINER_CONTENT                 = 361;
static constexpr uint32_t CONTAINER_HAS_THUMB               = 362;
static constexpr uint32_t CONTAINER_SORT_METHOD             = 363;
static constexpr uint32_t CONTAINER_CURRENT_ITEM            = 364;
static constexpr uint32_t CONTAINER_ART                     = 365;
static constexpr uint32_t CONTAINER_HAS_FOCUS               = 366;
static constexpr uint32_t CONTAINER_ROW                     = 367;
static constexpr uint32_t CONTAINER_COLUMN                  = 368;
static constexpr uint32_t CONTAINER_POSITION                = 369;
static constexpr uint32_t CONTAINER_VIEWMODE                = 370;
static constexpr uint32_t CONTAINER_HAS_NEXT                = 371;
static constexpr uint32_t CONTAINER_HAS_PREVIOUS            = 372;
static constexpr uint32_t CONTAINER_SUBITEM                 = 373;
static constexpr uint32_t CONTAINER_NUM_PAGES               = 374;
static constexpr uint32_t CONTAINER_CURRENT_PAGE            = 375;
static constexpr uint32_t CONTAINER_SHOWPLOT                = 376;
static constexpr uint32_t CONTAINER_TOTALTIME               = 377;
static constexpr uint32_t CONTAINER_SORT_ORDER              = 378;
static constexpr uint32_t CONTAINER_TOTALWATCHED            = 379;
static constexpr uint32_t CONTAINER_TOTALUNWATCHED          = 380;
static constexpr uint32_t CONTAINER_VIEWCOUNT               = 381;
static constexpr uint32_t CONTAINER_SHOWTITLE               = 382;
static constexpr uint32_t CONTAINER_PLUGINCATEGORY          = 383;
static constexpr uint32_t CONTAINER_NUM_ALL_ITEMS           = 384;
static constexpr uint32_t CONTAINER_NUM_NONFOLDER_ITEMS     = 385;

static constexpr uint32_t MUSICPM_ENABLED                   = 390;
static constexpr uint32_t MUSICPM_SONGSPLAYED               = 391;
static constexpr uint32_t MUSICPM_MATCHINGSONGS             = 392;
static constexpr uint32_t MUSICPM_MATCHINGSONGSPICKED       = 393;
static constexpr uint32_t MUSICPM_MATCHINGSONGSLEFT         = 394;
static constexpr uint32_t MUSICPM_RELAXEDSONGSPICKED        = 395;
static constexpr uint32_t MUSICPM_RANDOMSONGSPICKED         = 396;

static constexpr uint32_t PLAYLIST_LENGTH                   = 400;
static constexpr uint32_t PLAYLIST_POSITION                 = 401;
static constexpr uint32_t PLAYLIST_RANDOM                   = 402;
static constexpr uint32_t PLAYLIST_REPEAT                   = 403;
static constexpr uint32_t PLAYLIST_ISRANDOM                 = 404;
static constexpr uint32_t PLAYLIST_ISREPEAT                 = 405;
static constexpr uint32_t PLAYLIST_ISREPEATONE              = 406;

static constexpr uint32_t VISUALISATION_LOCKED              = 410;
static constexpr uint32_t VISUALISATION_PRESET              = 411;
static constexpr uint32_t VISUALISATION_NAME                = 412;
static constexpr uint32_t VISUALISATION_ENABLED             = 413;
static constexpr uint32_t VISUALISATION_HAS_PRESETS         = 414;

static constexpr uint32_t STRING_IS_EMPTY                   = 420;
static constexpr uint32_t STRING_IS_EQUAL                   = 421;
static constexpr uint32_t STRING_STARTS_WITH                = 422;
static constexpr uint32_t STRING_ENDS_WITH                  = 423;
static constexpr uint32_t STRING_CONTAINS                   = 424;

static constexpr uint32_t INTEGER_IS_EQUAL                  = 450;
static constexpr uint32_t INTEGER_GREATER_THAN              = 451;
static constexpr uint32_t INTEGER_GREATER_OR_EQUAL          = 452;
static constexpr uint32_t INTEGER_LESS_THAN                 = 453;
static constexpr uint32_t INTEGER_LESS_OR_EQUAL             = 454;
static constexpr uint32_t INTEGER_EVEN                      = 455;
static constexpr uint32_t INTEGER_ODD                       = 456;
static constexpr uint32_t INTEGER_VALUEOF                   = 457;

static constexpr uint32_t SKIN_BOOL                         = 600;
static constexpr uint32_t SKIN_STRING                       = 601;
static constexpr uint32_t SKIN_STRING_IS_EQUAL              = 602;
// unused id 603
static constexpr uint32_t SKIN_THEME                        = 604;
static constexpr uint32_t SKIN_COLOUR_THEME                 = 605;
static constexpr uint32_t SKIN_HAS_THEME                    = 606;
static constexpr uint32_t SKIN_ASPECT_RATIO                 = 607;
static constexpr uint32_t SKIN_FONT                         = 608;
static constexpr uint32_t SKIN_INTEGER                      = 609;
static constexpr uint32_t SKIN_TIMER_IS_RUNNING             = 610;
static constexpr uint32_t SKIN_TIMER_ELAPSEDSECS            = 611;

static constexpr uint32_t SYSTEM_IS_SCREENSAVER_INHIBITED   = 641;
static constexpr uint32_t SYSTEM_ADDON_UPDATE_COUNT         = 642;
static constexpr uint32_t SYSTEM_PRIVACY_POLICY             = 643;
static constexpr uint32_t SYSTEM_TOTAL_MEMORY               = 644;
static constexpr uint32_t SYSTEM_CPU_USAGE                  = 645;
static constexpr uint32_t SYSTEM_USED_MEMORY_PERCENT        = 646;
static constexpr uint32_t SYSTEM_USED_MEMORY                = 647;
static constexpr uint32_t SYSTEM_FREE_MEMORY                = 648;
static constexpr uint32_t SYSTEM_FREE_MEMORY_PERCENT        = 649;
// unused id 650 to 653
static constexpr uint32_t SYSTEM_UPTIME                     = 654;
static constexpr uint32_t SYSTEM_TOTALUPTIME                = 655;
static constexpr uint32_t SYSTEM_CPUFREQUENCY               = 656;
// unused id 657
// unused id 658
static constexpr uint32_t SYSTEM_SCREEN_RESOLUTION          = 659;
static constexpr uint32_t SYSTEM_VIDEO_ENCODER_INFO         = 660;
// unused id 661 to 666
static constexpr uint32_t SYSTEM_OS_VERSION_INFO            = 667;
// unused id 668 to 678
static constexpr uint32_t SYSTEM_FREE_SPACE                 = 679;
static constexpr uint32_t SYSTEM_USED_SPACE                 = 680;
static constexpr uint32_t SYSTEM_TOTAL_SPACE                = 681;
static constexpr uint32_t SYSTEM_USED_SPACE_PERCENT         = 682;
static constexpr uint32_t SYSTEM_FREE_SPACE_PERCENT         = 683;
// unused id 684 to 702
static constexpr uint32_t SYSTEM_ADDON_IS_ENABLED           = 703;
static constexpr uint32_t SYSTEM_GET_BOOL                   = 704;
static constexpr uint32_t SYSTEM_GET_CORE_USAGE             = 705;
static constexpr uint32_t SYSTEM_HAS_CORE_ID                = 706;
static constexpr uint32_t SYSTEM_RENDER_VENDOR              = 707;
static constexpr uint32_t SYSTEM_RENDER_RENDERER            = 708;
static constexpr uint32_t SYSTEM_RENDER_VERSION             = 709;
static constexpr uint32_t SYSTEM_SETTING                    = 710;
static constexpr uint32_t SYSTEM_HAS_ADDON                  = 711;
static constexpr uint32_t SYSTEM_ADDON_TITLE                = 712;
static constexpr uint32_t SYSTEM_ADDON_ICON                 = 713;
static constexpr uint32_t SYSTEM_BATTERY_LEVEL              = 714;
static constexpr uint32_t SYSTEM_IDLE_TIME                  = 715;
static constexpr uint32_t SYSTEM_FRIENDLY_NAME              = 716;
static constexpr uint32_t SYSTEM_SCREENSAVER_ACTIVE         = 717;
static constexpr uint32_t SYSTEM_ADDON_VERSION              = 718;
static constexpr uint32_t SYSTEM_DPMS_ACTIVE                = 719;

static constexpr uint32_t LIBRARY_HAS_MUSIC                 = 720;
static constexpr uint32_t LIBRARY_HAS_VIDEO                 = 721;
static constexpr uint32_t LIBRARY_HAS_MOVIES                = 722;
static constexpr uint32_t LIBRARY_HAS_MOVIE_SETS            = 723;
static constexpr uint32_t LIBRARY_HAS_TVSHOWS               = 724;
static constexpr uint32_t LIBRARY_HAS_MUSICVIDEOS           = 725;
static constexpr uint32_t LIBRARY_HAS_SINGLES               = 726;
static constexpr uint32_t LIBRARY_HAS_COMPILATIONS          = 727;
static constexpr uint32_t LIBRARY_IS_SCANNING               = 728;
static constexpr uint32_t LIBRARY_IS_SCANNING_VIDEO         = 729;
static constexpr uint32_t LIBRARY_IS_SCANNING_MUSIC         = 730;
// unused id 731 to 734
static constexpr uint32_t LIBRARY_HAS_ROLE                  = 735;
static constexpr uint32_t LIBRARY_HAS_BOXSETS               = 736;
static constexpr uint32_t LIBRARY_HAS_NODE                  = 737;

static constexpr uint32_t SYSTEM_PLATFORM_LINUX             = 741;
static constexpr uint32_t SYSTEM_PLATFORM_WINDOWS           = 742;
static constexpr uint32_t SYSTEM_PLATFORM_DARWIN            = 743;
static constexpr uint32_t SYSTEM_PLATFORM_DARWIN_OSX        = 744;
static constexpr uint32_t SYSTEM_PLATFORM_DARWIN_IOS        = 745;
static constexpr uint32_t SYSTEM_PLATFORM_UWP               = 746;
static constexpr uint32_t SYSTEM_PLATFORM_ANDROID           = 747;
static constexpr uint32_t SYSTEM_PLATFORM_WINDOWING         = 748;
static constexpr uint32_t SYSTEM_PLATFORM_WIN10             = 749;
static constexpr uint32_t SYSTEM_CAN_POWERDOWN              = 750;
static constexpr uint32_t SYSTEM_CAN_SUSPEND                = 751;
static constexpr uint32_t SYSTEM_CAN_HIBERNATE              = 752;
static constexpr uint32_t SYSTEM_CAN_REBOOT                 = 753;
static constexpr uint32_t SYSTEM_MEDIA_AUDIO_CD             = 754;
static constexpr uint32_t SYSTEM_PLATFORM_DARWIN_TVOS       = 755;
static constexpr uint32_t SYSTEM_SUPPORTED_HDR_TYPES        = 756;
static constexpr uint32_t SYSTEM_PLATFORM_WEBOS             = 757;
static constexpr uint32_t SYSTEM_MEDIA_BLURAY_PLAYLIST      = 758;

static constexpr uint32_t SLIDESHOW_ISPAUSED                = 800;
static constexpr uint32_t SLIDESHOW_ISRANDOM                = 801;
static constexpr uint32_t SLIDESHOW_ISACTIVE                = 802;
static constexpr uint32_t SLIDESHOW_ISVIDEO                 = 803;

static constexpr int      SLIDESHOW_LABELS_START            = 900;
static constexpr uint32_t SLIDESHOW_FILE_NAME               = SLIDESHOW_LABELS_START;
static constexpr uint32_t SLIDESHOW_FILE_PATH               = SLIDESHOW_LABELS_START + 1;
static constexpr uint32_t SLIDESHOW_FILE_SIZE               = SLIDESHOW_LABELS_START + 2;
static constexpr uint32_t SLIDESHOW_FILE_DATE               = SLIDESHOW_LABELS_START + 3;
static constexpr uint32_t SLIDESHOW_INDEX                   = SLIDESHOW_LABELS_START + 4;
static constexpr uint32_t SLIDESHOW_RESOLUTION              = SLIDESHOW_LABELS_START + 5;
static constexpr uint32_t SLIDESHOW_COMMENT                 = SLIDESHOW_LABELS_START + 6;
static constexpr uint32_t SLIDESHOW_COLOUR                  = SLIDESHOW_LABELS_START + 7;
static constexpr uint32_t SLIDESHOW_PROCESS                 = SLIDESHOW_LABELS_START + 8;

static constexpr uint32_t SLIDESHOW_EXIF_LONG_DATE          = SLIDESHOW_LABELS_START + 17;
static constexpr uint32_t SLIDESHOW_EXIF_LONG_DATE_TIME     = SLIDESHOW_LABELS_START + 18;
static constexpr uint32_t SLIDESHOW_EXIF_DATE               = SLIDESHOW_LABELS_START + 19; // Implementation only to just get localized date
static constexpr uint32_t SLIDESHOW_EXIF_DATE_TIME          = SLIDESHOW_LABELS_START + 20;
static constexpr uint32_t SLIDESHOW_EXIF_DESCRIPTION        = SLIDESHOW_LABELS_START + 21;
static constexpr uint32_t SLIDESHOW_EXIF_CAMERA_MAKE        = SLIDESHOW_LABELS_START + 22;
static constexpr uint32_t SLIDESHOW_EXIF_CAMERA_MODEL       = SLIDESHOW_LABELS_START + 23;
static constexpr uint32_t SLIDESHOW_EXIF_COMMENT            = SLIDESHOW_LABELS_START + 24;
//empty label   = SLIDESHOW_LABELS_START + 25;
static constexpr uint32_t SLIDESHOW_EXIF_APERTURE           = SLIDESHOW_LABELS_START + 26;
static constexpr uint32_t SLIDESHOW_EXIF_FOCAL_LENGTH       = SLIDESHOW_LABELS_START + 27;
static constexpr uint32_t SLIDESHOW_EXIF_FOCUS_DIST         = SLIDESHOW_LABELS_START + 28;
static constexpr uint32_t SLIDESHOW_EXIF_EXPOSURE           = SLIDESHOW_LABELS_START + 29;
static constexpr uint32_t SLIDESHOW_EXIF_EXPOSURE_TIME      = SLIDESHOW_LABELS_START + 30;
static constexpr uint32_t SLIDESHOW_EXIF_EXPOSURE_BIAS      = SLIDESHOW_LABELS_START + 31;
static constexpr uint32_t SLIDESHOW_EXIF_EXPOSURE_MODE      = SLIDESHOW_LABELS_START + 32;
static constexpr uint32_t SLIDESHOW_EXIF_FLASH_USED         = SLIDESHOW_LABELS_START + 33;
static constexpr uint32_t SLIDESHOW_EXIF_WHITE_BALANCE      = SLIDESHOW_LABELS_START + 34;
static constexpr uint32_t SLIDESHOW_EXIF_LIGHT_SOURCE       = SLIDESHOW_LABELS_START + 35;
static constexpr uint32_t SLIDESHOW_EXIF_METERING_MODE      = SLIDESHOW_LABELS_START + 36;
static constexpr uint32_t SLIDESHOW_EXIF_ISO_EQUIV          = SLIDESHOW_LABELS_START + 37;
static constexpr uint32_t SLIDESHOW_EXIF_DIGITAL_ZOOM       = SLIDESHOW_LABELS_START + 38;
static constexpr uint32_t SLIDESHOW_EXIF_CCD_WIDTH          = SLIDESHOW_LABELS_START + 39;
static constexpr uint32_t SLIDESHOW_EXIF_GPS_LATITUDE       = SLIDESHOW_LABELS_START + 40;
static constexpr uint32_t SLIDESHOW_EXIF_GPS_LONGITUDE      = SLIDESHOW_LABELS_START + 41;
static constexpr uint32_t SLIDESHOW_EXIF_GPS_ALTITUDE       = SLIDESHOW_LABELS_START + 42;
static constexpr uint32_t SLIDESHOW_EXIF_ORIENTATION        = SLIDESHOW_LABELS_START + 43;
static constexpr uint32_t SLIDESHOW_EXIF_XPCOMMENT          = SLIDESHOW_LABELS_START + 44;

static constexpr uint32_t SLIDESHOW_IPTC_SUBLOCATION        = SLIDESHOW_LABELS_START + 57;
static constexpr uint32_t SLIDESHOW_IPTC_IMAGETYPE          = SLIDESHOW_LABELS_START + 58;
static constexpr uint32_t SLIDESHOW_IPTC_TIMECREATED        = SLIDESHOW_LABELS_START + 59;
static constexpr uint32_t SLIDESHOW_IPTC_SUP_CATEGORIES     = SLIDESHOW_LABELS_START + 60;
static constexpr uint32_t SLIDESHOW_IPTC_KEYWORDS           = SLIDESHOW_LABELS_START + 61;
static constexpr uint32_t SLIDESHOW_IPTC_CAPTION            = SLIDESHOW_LABELS_START + 62;
static constexpr uint32_t SLIDESHOW_IPTC_AUTHOR             = SLIDESHOW_LABELS_START + 63;
static constexpr uint32_t SLIDESHOW_IPTC_HEADLINE           = SLIDESHOW_LABELS_START + 64;
static constexpr uint32_t SLIDESHOW_IPTC_SPEC_INSTR         = SLIDESHOW_LABELS_START + 65;
static constexpr uint32_t SLIDESHOW_IPTC_CATEGORY           = SLIDESHOW_LABELS_START + 66;
static constexpr uint32_t SLIDESHOW_IPTC_BYLINE             = SLIDESHOW_LABELS_START + 67;
static constexpr uint32_t SLIDESHOW_IPTC_BYLINE_TITLE       = SLIDESHOW_LABELS_START + 68;
static constexpr uint32_t SLIDESHOW_IPTC_CREDIT             = SLIDESHOW_LABELS_START + 69;
static constexpr uint32_t SLIDESHOW_IPTC_SOURCE             = SLIDESHOW_LABELS_START + 70;
static constexpr uint32_t SLIDESHOW_IPTC_COPYRIGHT_NOTICE   = SLIDESHOW_LABELS_START + 71;
static constexpr uint32_t SLIDESHOW_IPTC_OBJECT_NAME        = SLIDESHOW_LABELS_START + 72;
static constexpr uint32_t SLIDESHOW_IPTC_CITY               = SLIDESHOW_LABELS_START + 73;
static constexpr uint32_t SLIDESHOW_IPTC_STATE              = SLIDESHOW_LABELS_START + 74;
static constexpr uint32_t SLIDESHOW_IPTC_COUNTRY            = SLIDESHOW_LABELS_START + 75;
static constexpr uint32_t SLIDESHOW_IPTC_TX_REFERENCE       = SLIDESHOW_LABELS_START + 76;
static constexpr uint32_t SLIDESHOW_IPTC_DATE               = SLIDESHOW_LABELS_START + 77;
static constexpr uint32_t SLIDESHOW_IPTC_URGENCY            = SLIDESHOW_LABELS_START + 78;
static constexpr uint32_t SLIDESHOW_IPTC_COUNTRY_CODE       = SLIDESHOW_LABELS_START + 79;
static constexpr uint32_t SLIDESHOW_IPTC_REF_SERVICE        = SLIDESHOW_LABELS_START + 80;
static constexpr int      SLIDESHOW_LABELS_END              = SLIDESHOW_IPTC_REF_SERVICE;

static constexpr uint32_t FANART_COLOR1                     = 1000;
static constexpr uint32_t FANART_COLOR2                     = 1001;
static constexpr uint32_t FANART_COLOR3                     = 1002;
static constexpr uint32_t FANART_IMAGE                      = 1003;

static constexpr uint32_t SYSTEM_PROFILEAUTOLOGIN           = 1004;
// unused id 1005
static constexpr uint32_t SYSTEM_HAS_CMS                    = 1006;
static constexpr uint32_t SYSTEM_BUILD_VERSION_CODE         = 1007;
static constexpr uint32_t SYSTEM_BUILD_VERSION_GIT          = 1008;
// unused id 1009
// unused id 1010
static constexpr uint32_t SYSTEM_LOCALE_REGION              = 1011;
static constexpr uint32_t SYSTEM_LOCALE                     = 1012;

static constexpr uint32_t PVR_CONDITIONS_START              = 1100;
static constexpr uint32_t PVR_IS_RECORDING                  = PVR_CONDITIONS_START;
static constexpr uint32_t PVR_HAS_TIMER                     = PVR_CONDITIONS_START + 1;
static constexpr uint32_t PVR_HAS_NONRECORDING_TIMER        = PVR_CONDITIONS_START + 2;
static constexpr uint32_t PVR_IS_PLAYING_TV                 = PVR_CONDITIONS_START + 3;
static constexpr uint32_t PVR_IS_PLAYING_RADIO              = PVR_CONDITIONS_START + 4;
static constexpr uint32_t PVR_IS_PLAYING_RECORDING          = PVR_CONDITIONS_START + 5;
static constexpr uint32_t PVR_ACTUAL_STREAM_ENCRYPTED       = PVR_CONDITIONS_START + 6;
static constexpr uint32_t PVR_HAS_TV_CHANNELS               = PVR_CONDITIONS_START + 7;
static constexpr uint32_t PVR_HAS_RADIO_CHANNELS            = PVR_CONDITIONS_START + 8;
static constexpr uint32_t PVR_IS_TIMESHIFTING               = PVR_CONDITIONS_START + 9;
static constexpr uint32_t PVR_IS_RECORDING_TV               = PVR_CONDITIONS_START + 10;
static constexpr uint32_t PVR_HAS_TV_TIMER                  = PVR_CONDITIONS_START + 11;
static constexpr uint32_t PVR_HAS_NONRECORDING_TV_TIMER     = PVR_CONDITIONS_START + 12;
static constexpr uint32_t PVR_IS_RECORDING_RADIO            = PVR_CONDITIONS_START + 13;
static constexpr uint32_t PVR_HAS_RADIO_TIMER               = PVR_CONDITIONS_START + 14;
static constexpr uint32_t PVR_HAS_NONRECORDING_RADIO_TIMER  = PVR_CONDITIONS_START + 15;
static constexpr uint32_t PVR_IS_PLAYING_EPGTAG             = PVR_CONDITIONS_START + 16;
static constexpr uint32_t PVR_CAN_RECORD_PLAYING_CHANNEL    = PVR_CONDITIONS_START + 17;
static constexpr uint32_t PVR_IS_RECORDING_PLAYING_CHANNEL  = PVR_CONDITIONS_START + 18;
static constexpr uint32_t PVR_IS_PLAYING_ACTIVE_RECORDING   = PVR_CONDITIONS_START + 19;

static constexpr uint32_t PVR_STRINGS_START                 = 1200;
static constexpr uint32_t PVR_NEXT_RECORDING_CHANNEL        = PVR_STRINGS_START;
static constexpr uint32_t PVR_NEXT_RECORDING_CHAN_ICO       = PVR_STRINGS_START + 1;
static constexpr uint32_t PVR_NEXT_RECORDING_DATETIME       = PVR_STRINGS_START + 2;
static constexpr uint32_t PVR_NEXT_RECORDING_TITLE          = PVR_STRINGS_START + 3;
static constexpr uint32_t PVR_NOW_RECORDING_CHANNEL         = PVR_STRINGS_START + 4;
static constexpr uint32_t PVR_NOW_RECORDING_CHAN_ICO        = PVR_STRINGS_START + 5;
static constexpr uint32_t PVR_NOW_RECORDING_DATETIME        = PVR_STRINGS_START + 6;
static constexpr uint32_t PVR_NOW_RECORDING_TITLE           = PVR_STRINGS_START + 7;
static constexpr uint32_t PVR_BACKEND_NAME                  = PVR_STRINGS_START + 8;
static constexpr uint32_t PVR_BACKEND_VERSION               = PVR_STRINGS_START + 9;
static constexpr uint32_t PVR_BACKEND_HOST                  = PVR_STRINGS_START + 10;
static constexpr uint32_t PVR_BACKEND_DISKSPACE             = PVR_STRINGS_START + 11;
static constexpr uint32_t PVR_BACKEND_CHANNELS              = PVR_STRINGS_START + 12;
static constexpr uint32_t PVR_BACKEND_TIMERS                = PVR_STRINGS_START + 13;
static constexpr uint32_t PVR_BACKEND_RECORDINGS            = PVR_STRINGS_START + 14;
static constexpr uint32_t PVR_BACKEND_DELETED_RECORDINGS    = PVR_STRINGS_START + 15;
static constexpr uint32_t PVR_BACKEND_NUMBER                = PVR_STRINGS_START + 16;
static constexpr uint32_t PVR_TOTAL_DISKSPACE               = PVR_STRINGS_START + 17;
static constexpr uint32_t PVR_NEXT_TIMER                    = PVR_STRINGS_START + 18;
static constexpr uint32_t PVR_EPG_EVENT_DURATION            = PVR_STRINGS_START + 19;
static constexpr uint32_t PVR_EPG_EVENT_ELAPSED_TIME        = PVR_STRINGS_START + 20;
static constexpr uint32_t PVR_EPG_EVENT_PROGRESS            = PVR_STRINGS_START + 21;
static constexpr uint32_t PVR_ACTUAL_STREAM_CLIENT          = PVR_STRINGS_START + 22;
static constexpr uint32_t PVR_ACTUAL_STREAM_DEVICE          = PVR_STRINGS_START + 23;
static constexpr uint32_t PVR_ACTUAL_STREAM_STATUS          = PVR_STRINGS_START + 24;
static constexpr uint32_t PVR_ACTUAL_STREAM_SIG             = PVR_STRINGS_START + 25;
static constexpr uint32_t PVR_ACTUAL_STREAM_SNR             = PVR_STRINGS_START + 26;
static constexpr uint32_t PVR_ACTUAL_STREAM_SIG_PROGR       = PVR_STRINGS_START + 27;
static constexpr uint32_t PVR_ACTUAL_STREAM_SNR_PROGR       = PVR_STRINGS_START + 28;
static constexpr uint32_t PVR_ACTUAL_STREAM_BER             = PVR_STRINGS_START + 29;
static constexpr uint32_t PVR_ACTUAL_STREAM_UNC             = PVR_STRINGS_START + 30;
static constexpr uint32_t PVR_ACTUAL_STREAM_CRYPTION        = PVR_STRINGS_START + 34;
static constexpr uint32_t PVR_ACTUAL_STREAM_SERVICE         = PVR_STRINGS_START + 35;
static constexpr uint32_t PVR_ACTUAL_STREAM_MUX             = PVR_STRINGS_START + 36;
static constexpr uint32_t PVR_ACTUAL_STREAM_PROVIDER        = PVR_STRINGS_START + 37;
static constexpr uint32_t PVR_BACKEND_DISKSPACE_PROGR       = PVR_STRINGS_START + 38;
static constexpr uint32_t PVR_TIMESHIFT_START_TIME          = PVR_STRINGS_START + 39;
static constexpr uint32_t PVR_TIMESHIFT_END_TIME            = PVR_STRINGS_START + 40;
static constexpr uint32_t PVR_TIMESHIFT_PLAY_TIME           = PVR_STRINGS_START + 41;
static constexpr uint32_t PVR_TIMESHIFT_PROGRESS            = PVR_STRINGS_START + 42;
static constexpr uint32_t PVR_TV_NOW_RECORDING_TITLE        = PVR_STRINGS_START + 43;
static constexpr uint32_t PVR_TV_NOW_RECORDING_CHANNEL      = PVR_STRINGS_START + 44;
static constexpr uint32_t PVR_TV_NOW_RECORDING_CHAN_ICO     = PVR_STRINGS_START + 45;
static constexpr uint32_t PVR_TV_NOW_RECORDING_DATETIME     = PVR_STRINGS_START + 46;
static constexpr uint32_t PVR_TV_NEXT_RECORDING_TITLE       = PVR_STRINGS_START + 47;
static constexpr uint32_t PVR_TV_NEXT_RECORDING_CHANNEL     = PVR_STRINGS_START + 48;
static constexpr uint32_t PVR_TV_NEXT_RECORDING_CHAN_ICO    = PVR_STRINGS_START + 49;
static constexpr uint32_t PVR_TV_NEXT_RECORDING_DATETIME    = PVR_STRINGS_START + 50;
static constexpr uint32_t PVR_RADIO_NOW_RECORDING_TITLE     = PVR_STRINGS_START + 51;
static constexpr uint32_t PVR_RADIO_NOW_RECORDING_CHANNEL   = PVR_STRINGS_START + 52;
static constexpr uint32_t PVR_RADIO_NOW_RECORDING_CHAN_ICO  = PVR_STRINGS_START + 53;
static constexpr uint32_t PVR_RADIO_NOW_RECORDING_DATETIME  = PVR_STRINGS_START + 54;
static constexpr uint32_t PVR_RADIO_NEXT_RECORDING_TITLE    = PVR_STRINGS_START + 55;
static constexpr uint32_t PVR_RADIO_NEXT_RECORDING_CHANNEL  = PVR_STRINGS_START + 56;
static constexpr uint32_t PVR_RADIO_NEXT_RECORDING_CHAN_ICO = PVR_STRINGS_START + 57;
static constexpr uint32_t PVR_RADIO_NEXT_RECORDING_DATETIME = PVR_STRINGS_START + 58;
static constexpr uint32_t PVR_CHANNEL_NUMBER_INPUT          = PVR_STRINGS_START + 59;
static constexpr uint32_t PVR_EPG_EVENT_REMAINING_TIME      = PVR_STRINGS_START + 60;
static constexpr uint32_t PVR_EPG_EVENT_FINISH_TIME         = PVR_STRINGS_START + 61;
static constexpr uint32_t PVR_TIMESHIFT_OFFSET              = PVR_STRINGS_START + 62;
static constexpr uint32_t PVR_EPG_EVENT_SEEK_TIME           = PVR_STRINGS_START + 63;
static constexpr uint32_t PVR_TIMESHIFT_PROGRESS_PLAY_POS   = PVR_STRINGS_START + 64;
static constexpr uint32_t PVR_TIMESHIFT_PROGRESS_DURATION   = PVR_STRINGS_START + 65;
static constexpr uint32_t PVR_TIMESHIFT_PROGRESS_EPG_START  = PVR_STRINGS_START + 66;
static constexpr uint32_t PVR_TIMESHIFT_PROGRESS_EPG_END    = PVR_STRINGS_START + 67;
static constexpr uint32_t PVR_TIMESHIFT_PROGRESS_BUFFER_START = PVR_STRINGS_START + 68;
static constexpr uint32_t PVR_TIMESHIFT_PROGRESS_BUFFER_END = PVR_STRINGS_START + 69;
static constexpr uint32_t PVR_TIMESHIFT_PROGRESS_START_TIME = PVR_STRINGS_START + 70;
static constexpr uint32_t PVR_TIMESHIFT_PROGRESS_END_TIME   = PVR_STRINGS_START + 71;
static constexpr uint32_t PVR_EPG_EVENT_ICON                = PVR_STRINGS_START + 72;
static constexpr uint32_t PVR_TIMESHIFT_SEEKBAR             = PVR_STRINGS_START + 73;
static constexpr uint32_t PVR_BACKEND_PROVIDERS             = PVR_STRINGS_START + 74;
static constexpr uint32_t PVR_BACKEND_CHANNEL_GROUPS        = PVR_STRINGS_START + 75;
static constexpr uint32_t PVR_CLIENT_NAME                   = PVR_STRINGS_START + 76;
static constexpr uint32_t PVR_INSTANCE_NAME                 = PVR_STRINGS_START + 77;

static constexpr uint32_t PVR_INTS_START                    = 1300;
static constexpr uint32_t PVR_CLIENT_COUNT                  = PVR_INTS_START;

static constexpr uint32_t RDS_DATA_START                    = 1400;
static constexpr uint32_t RDS_HAS_RDS                       = RDS_DATA_START;
static constexpr uint32_t RDS_HAS_RADIOTEXT                 = RDS_DATA_START + 1;
static constexpr uint32_t RDS_HAS_RADIOTEXT_PLUS            = RDS_DATA_START + 2;
static constexpr uint32_t RDS_GET_RADIOTEXT_LINE            = RDS_DATA_START + 3;
static constexpr uint32_t RDS_TITLE                         = RDS_DATA_START + 4;
static constexpr uint32_t RDS_BAND                          = RDS_DATA_START + 5;
static constexpr uint32_t RDS_ARTIST                        = RDS_DATA_START + 6;
static constexpr uint32_t RDS_COMPOSER                      = RDS_DATA_START + 7;
static constexpr uint32_t RDS_CONDUCTOR                     = RDS_DATA_START + 8;
static constexpr uint32_t RDS_ALBUM                         = RDS_DATA_START + 9;
static constexpr uint32_t RDS_ALBUM_TRACKNUMBER             = RDS_DATA_START + 10;
static constexpr uint32_t RDS_GET_RADIO_STYLE               = RDS_DATA_START + 11;
static constexpr uint32_t RDS_COMMENT                       = RDS_DATA_START + 12;
static constexpr uint32_t RDS_INFO_NEWS                     = RDS_DATA_START + 13;
static constexpr uint32_t RDS_INFO_NEWS_LOCAL               = RDS_DATA_START + 14;
static constexpr uint32_t RDS_INFO_STOCK                    = RDS_DATA_START + 15;
static constexpr uint32_t RDS_INFO_STOCK_SIZE               = RDS_DATA_START + 16;
static constexpr uint32_t RDS_INFO_SPORT                    = RDS_DATA_START + 17;
static constexpr uint32_t RDS_INFO_SPORT_SIZE               = RDS_DATA_START + 18;
static constexpr uint32_t RDS_INFO_LOTTERY                  = RDS_DATA_START + 19;
static constexpr uint32_t RDS_INFO_LOTTERY_SIZE             = RDS_DATA_START + 20;
static constexpr uint32_t RDS_INFO_WEATHER                  = RDS_DATA_START + 21;
static constexpr uint32_t RDS_INFO_WEATHER_SIZE             = RDS_DATA_START + 22;
static constexpr uint32_t RDS_INFO_CINEMA                   = RDS_DATA_START + 23;
static constexpr uint32_t RDS_INFO_CINEMA_SIZE              = RDS_DATA_START + 24;
static constexpr uint32_t RDS_INFO_HOROSCOPE                = RDS_DATA_START + 25;
static constexpr uint32_t RDS_INFO_HOROSCOPE_SIZE           = RDS_DATA_START + 26;
static constexpr uint32_t RDS_INFO_OTHER                    = RDS_DATA_START + 27;
static constexpr uint32_t RDS_INFO_OTHER_SIZE               = RDS_DATA_START + 28;
static constexpr uint32_t RDS_PROG_STATION                  = RDS_DATA_START + 29;
static constexpr uint32_t RDS_PROG_NOW                      = RDS_DATA_START + 30;
static constexpr uint32_t RDS_PROG_NEXT                     = RDS_DATA_START + 31;
static constexpr uint32_t RDS_PROG_HOST                     = RDS_DATA_START + 32;
static constexpr uint32_t RDS_PROG_EDIT_STAFF               = RDS_DATA_START + 33;
static constexpr uint32_t RDS_PROG_HOMEPAGE                 = RDS_DATA_START + 34;
static constexpr uint32_t RDS_PROG_STYLE                    = RDS_DATA_START + 35;
static constexpr uint32_t RDS_PHONE_HOTLINE                 = RDS_DATA_START + 36;
static constexpr uint32_t RDS_PHONE_STUDIO                  = RDS_DATA_START + 37;
static constexpr uint32_t RDS_SMS_STUDIO                    = RDS_DATA_START + 38;
static constexpr uint32_t RDS_EMAIL_HOTLINE                 = RDS_DATA_START + 39;
static constexpr uint32_t RDS_EMAIL_STUDIO                  = RDS_DATA_START + 40;
static constexpr uint32_t RDS_HAS_HOTLINE_DATA              = RDS_DATA_START + 41;
static constexpr uint32_t RDS_HAS_STUDIO_DATA               = RDS_DATA_START + 42;
static constexpr uint32_t RDS_AUDIO_LANG                    = RDS_DATA_START + 43;
static constexpr uint32_t RDS_CHANNEL_COUNTRY               = RDS_DATA_START + 44;
static constexpr uint32_t RDS_DATA_END                      = RDS_CHANNEL_COUNTRY;

static constexpr uint32_t PLAYER_PROCESS_START              = 1500;
static constexpr uint32_t PLAYER_PROCESS_VIDEODECODER       = PLAYER_PROCESS_START;
static constexpr uint32_t PLAYER_PROCESS_DEINTMETHOD        = PLAYER_PROCESS_START + 1;
static constexpr uint32_t PLAYER_PROCESS_PIXELFORMAT        = PLAYER_PROCESS_START + 2;
static constexpr uint32_t PLAYER_PROCESS_VIDEOWIDTH         = PLAYER_PROCESS_START + 3;
static constexpr uint32_t PLAYER_PROCESS_VIDEOHEIGHT        = PLAYER_PROCESS_START + 4;
static constexpr uint32_t PLAYER_PROCESS_VIDEOFPS           = PLAYER_PROCESS_START + 5;
static constexpr uint32_t PLAYER_PROCESS_VIDEODAR           = PLAYER_PROCESS_START + 6;
static constexpr uint32_t PLAYER_PROCESS_VIDEOHWDECODER     = PLAYER_PROCESS_START + 7;
static constexpr uint32_t PLAYER_PROCESS_AUDIODECODER       = PLAYER_PROCESS_START + 8;
static constexpr uint32_t PLAYER_PROCESS_AUDIOCHANNELS      = PLAYER_PROCESS_START + 9;
static constexpr uint32_t PLAYER_PROCESS_AUDIOSAMPLERATE    = PLAYER_PROCESS_START + 10;
static constexpr uint32_t PLAYER_PROCESS_AUDIOBITSPERSAMPLE = PLAYER_PROCESS_START + 11;
static constexpr uint32_t PLAYER_PROCESS_VIDEOSCANTYPE      = PLAYER_PROCESS_START + 12;

static constexpr uint32_t ADDON_INFOS_START                 = 1600;
static constexpr uint32_t ADDON_SETTING_STRING              = ADDON_INFOS_START;
static constexpr uint32_t ADDON_SETTING_BOOL                = ADDON_INFOS_START + 1;
static constexpr uint32_t ADDON_SETTING_INT                 = ADDON_INFOS_START + 2;

static constexpr uint32_t WINDOW_PROPERTY                   = 9993;
// unused id 9994
static constexpr uint32_t WINDOW_IS_VISIBLE                 = 9995;
static constexpr uint32_t WINDOW_NEXT                       = 9996;
static constexpr uint32_t WINDOW_PREVIOUS                   = 9997;
static constexpr uint32_t WINDOW_IS_MEDIA                   = 9998;
static constexpr uint32_t WINDOW_IS_ACTIVE                  = 9999;
static constexpr uint32_t WINDOW_IS                         = 10000;
static constexpr uint32_t WINDOW_IS_DIALOG_TOPMOST          = 10001;
static constexpr uint32_t WINDOW_IS_MODAL_DIALOG_TOPMOST    = 10002;

static constexpr uint32_t CONTROL_GET_LABEL                 = 29996;
static constexpr uint32_t CONTROL_IS_ENABLED                = 29997;
static constexpr uint32_t CONTROL_IS_VISIBLE                = 29998;
static constexpr uint32_t CONTROL_GROUP_HAS_FOCUS           = 29999;
static constexpr uint32_t CONTROL_HAS_FOCUS                 = 30000;

static constexpr int      LISTITEM_START                    = 35000;
static constexpr uint32_t LISTITEM_THUMB                    = LISTITEM_START;
static constexpr uint32_t LISTITEM_LABEL                    = LISTITEM_START + 1;
static constexpr uint32_t LISTITEM_TITLE                    = LISTITEM_START + 2;
static constexpr uint32_t LISTITEM_TRACKNUMBER              = LISTITEM_START + 3;
static constexpr uint32_t LISTITEM_ARTIST                   = LISTITEM_START + 4;
static constexpr uint32_t LISTITEM_ALBUM                    = LISTITEM_START + 5;
static constexpr uint32_t LISTITEM_YEAR                     = LISTITEM_START + 6;
static constexpr uint32_t LISTITEM_GENRE                    = LISTITEM_START + 7;
static constexpr uint32_t LISTITEM_ICON                     = LISTITEM_START + 8;
static constexpr uint32_t LISTITEM_DIRECTOR                 = LISTITEM_START + 9;
static constexpr uint32_t LISTITEM_OVERLAY                  = LISTITEM_START + 10;
static constexpr uint32_t LISTITEM_LABEL2                   = LISTITEM_START + 11;
static constexpr uint32_t LISTITEM_FILENAME                 = LISTITEM_START + 12;
static constexpr uint32_t LISTITEM_DATE                     = LISTITEM_START + 13;
static constexpr uint32_t LISTITEM_SIZE                     = LISTITEM_START + 14;
static constexpr uint32_t LISTITEM_RATING                   = LISTITEM_START + 15;
static constexpr uint32_t LISTITEM_PROGRAM_COUNT            = LISTITEM_START + 16;
static constexpr uint32_t LISTITEM_DURATION                 = LISTITEM_START + 17;
static constexpr uint32_t LISTITEM_ISPLAYING                = LISTITEM_START + 18;
static constexpr uint32_t LISTITEM_ISSELECTED               = LISTITEM_START + 19;
static constexpr uint32_t LISTITEM_PLOT                     = LISTITEM_START + 20;
static constexpr uint32_t LISTITEM_PLOT_OUTLINE             = LISTITEM_START + 21;
static constexpr uint32_t LISTITEM_EPISODE                  = LISTITEM_START + 22;
static constexpr uint32_t LISTITEM_SEASON                   = LISTITEM_START + 23;
static constexpr uint32_t LISTITEM_TVSHOW                   = LISTITEM_START + 24;
static constexpr uint32_t LISTITEM_PREMIERED                = LISTITEM_START + 25;
static constexpr uint32_t LISTITEM_COMMENT                  = LISTITEM_START + 26;
static constexpr uint32_t LISTITEM_ACTUAL_ICON              = LISTITEM_START + 27;
static constexpr uint32_t LISTITEM_PATH                     = LISTITEM_START + 28;
static constexpr uint32_t LISTITEM_PICTURE_PATH             = LISTITEM_START + 29;

static constexpr int      LISTITEM_PICTURE_START            = LISTITEM_START + 30;
static constexpr uint32_t LISTITEM_PICTURE_RESOLUTION       = LISTITEM_PICTURE_START; // => SLIDESHOW_RESOLUTION
static constexpr uint32_t LISTITEM_PICTURE_LONGDATE         = LISTITEM_START + 31;    // => SLIDESHOW_EXIF_LONG_DATE
static constexpr uint32_t LISTITEM_PICTURE_LONGDATETIME     = LISTITEM_START + 32;    // => SLIDESHOW_EXIF_LONG_DATE_TIME
static constexpr uint32_t LISTITEM_PICTURE_DATE             = LISTITEM_START + 33;    // => SLIDESHOW_EXIF_DATE
static constexpr uint32_t LISTITEM_PICTURE_DATETIME         = LISTITEM_START + 34;    // => SLIDESHOW_EXIF_DATE_TIME
static constexpr uint32_t LISTITEM_PICTURE_COMMENT          = LISTITEM_START + 35;    // => SLIDESHOW_COMMENT
static constexpr uint32_t LISTITEM_PICTURE_CAPTION          = LISTITEM_START + 36;    // => SLIDESHOW_IPTC_CAPTION
static constexpr uint32_t LISTITEM_PICTURE_DESC             = LISTITEM_START + 37;    // => SLIDESHOW_EXIF_DESCRIPTION
static constexpr uint32_t LISTITEM_PICTURE_KEYWORDS         = LISTITEM_START + 38;    // => SLIDESHOW_IPTC_KEYWORDS
static constexpr uint32_t LISTITEM_PICTURE_CAM_MAKE         = LISTITEM_START + 39;    // => SLIDESHOW_EXIF_CAMERA_MAKE
static constexpr uint32_t LISTITEM_PICTURE_CAM_MODEL        = LISTITEM_START + 40;    // => SLIDESHOW_EXIF_CAMERA_MODEL
static constexpr uint32_t LISTITEM_PICTURE_APERTURE         = LISTITEM_START + 41;    // => SLIDESHOW_EXIF_APERTURE
static constexpr uint32_t LISTITEM_PICTURE_FOCAL_LEN        = LISTITEM_START + 42;    // => SLIDESHOW_EXIF_FOCAL_LENGTH
static constexpr uint32_t LISTITEM_PICTURE_FOCUS_DIST       = LISTITEM_START + 43;    // => SLIDESHOW_EXIF_FOCUS_DIST
static constexpr uint32_t LISTITEM_PICTURE_EXP_MODE         = LISTITEM_START + 44;    // => SLIDESHOW_EXIF_EXPOSURE_MODE
static constexpr uint32_t LISTITEM_PICTURE_EXP_TIME         = LISTITEM_START + 45;    // => SLIDESHOW_EXIF_EXPOSURE_TIME
static constexpr uint32_t LISTITEM_PICTURE_ISO              = LISTITEM_START + 46;    // => SLIDESHOW_EXIF_ISO_EQUIV
static constexpr uint32_t LISTITEM_PICTURE_AUTHOR           = LISTITEM_START + 47; // => SLIDESHOW_IPTC_AUTHOR
static constexpr uint32_t LISTITEM_PICTURE_BYLINE           = LISTITEM_START + 48; // => SLIDESHOW_IPTC_BYLINE
static constexpr uint32_t LISTITEM_PICTURE_BYLINE_TITLE     = LISTITEM_START + 49; // => SLIDESHOW_IPTC_BYLINE_TITLE
static constexpr uint32_t LISTITEM_PICTURE_CATEGORY         = LISTITEM_START + 50; // => SLIDESHOW_IPTC_CATEGORY
static constexpr uint32_t LISTITEM_PICTURE_CCD_WIDTH        = LISTITEM_START + 51; // => SLIDESHOW_EXIF_CCD_WIDTH
static constexpr uint32_t LISTITEM_PICTURE_CITY             = LISTITEM_START + 52; // => SLIDESHOW_IPTC_CITY
static constexpr uint32_t LISTITEM_PICTURE_URGENCY          = LISTITEM_START + 53; // => SLIDESHOW_IPTC_URGENCY
static constexpr uint32_t LISTITEM_PICTURE_COPYRIGHT_NOTICE = LISTITEM_START + 54; // => SLIDESHOW_IPTC_COPYRIGHT_NOTICE
static constexpr uint32_t LISTITEM_PICTURE_COUNTRY          = LISTITEM_START + 55; // => SLIDESHOW_IPTC_COUNTRY
static constexpr uint32_t LISTITEM_PICTURE_COUNTRY_CODE     = LISTITEM_START + 56; // => SLIDESHOW_IPTC_COUNTRY_CODE
static constexpr uint32_t LISTITEM_PICTURE_CREDIT           = LISTITEM_START + 57; // => SLIDESHOW_IPTC_CREDIT
static constexpr uint32_t LISTITEM_PICTURE_IPTCDATE         = LISTITEM_START + 58; // => SLIDESHOW_IPTC_DATE
static constexpr uint32_t LISTITEM_PICTURE_DIGITAL_ZOOM     = LISTITEM_START + 59; // => SLIDESHOW_EXIF_DIGITAL_ZOOM
static constexpr uint32_t LISTITEM_PICTURE_EXPOSURE         = LISTITEM_START + 60; // => SLIDESHOW_EXIF_EXPOSURE
static constexpr uint32_t LISTITEM_PICTURE_EXPOSURE_BIAS    = LISTITEM_START + 61; // => SLIDESHOW_EXIF_EXPOSURE_BIAS
static constexpr uint32_t LISTITEM_PICTURE_FLASH_USED       = LISTITEM_START + 62; // => SLIDESHOW_EXIF_FLASH_USED
static constexpr uint32_t LISTITEM_PICTURE_HEADLINE         = LISTITEM_START + 63; // => SLIDESHOW_IPTC_HEADLINE
static constexpr uint32_t LISTITEM_PICTURE_COLOUR           = LISTITEM_START + 64; // => SLIDESHOW_COLOUR
static constexpr uint32_t LISTITEM_PICTURE_LIGHT_SOURCE     = LISTITEM_START + 65; // => SLIDESHOW_EXIF_LIGHT_SOURCE
static constexpr uint32_t LISTITEM_PICTURE_METERING_MODE    = LISTITEM_START + 66; // => SLIDESHOW_EXIF_METERING_MODE
static constexpr uint32_t LISTITEM_PICTURE_OBJECT_NAME      = LISTITEM_START + 67; // => SLIDESHOW_IPTC_OBJECT_NAME
static constexpr uint32_t LISTITEM_PICTURE_ORIENTATION      = LISTITEM_START + 68; // => SLIDESHOW_EXIF_ORIENTATION
static constexpr uint32_t LISTITEM_PICTURE_PROCESS          = LISTITEM_START + 69; // => SLIDESHOW_PROCESS
static constexpr uint32_t LISTITEM_PICTURE_REF_SERVICE      = LISTITEM_START + 70; // => SLIDESHOW_IPTC_REF_SERVICE
static constexpr uint32_t LISTITEM_PICTURE_SOURCE           = LISTITEM_START + 71; // => SLIDESHOW_IPTC_SOURCE
static constexpr uint32_t LISTITEM_PICTURE_SPEC_INSTR       = LISTITEM_START + 72; // => SLIDESHOW_IPTC_SPEC_INSTR
static constexpr uint32_t LISTITEM_PICTURE_STATE            = LISTITEM_START + 73; // => SLIDESHOW_IPTC_STATE
static constexpr uint32_t LISTITEM_PICTURE_SUP_CATEGORIES   = LISTITEM_START + 74; // => SLIDESHOW_IPTC_SUP_CATEGORIES
static constexpr uint32_t LISTITEM_PICTURE_TX_REFERENCE     = LISTITEM_START + 75; // => SLIDESHOW_IPTC_TX_REFERENCE
static constexpr uint32_t LISTITEM_PICTURE_WHITE_BALANCE    = LISTITEM_START + 76; // => SLIDESHOW_EXIF_WHITE_BALANCE
static constexpr uint32_t LISTITEM_PICTURE_IMAGETYPE        = LISTITEM_START + 77; // => SLIDESHOW_IPTC_IMAGETYPE
static constexpr uint32_t LISTITEM_PICTURE_SUBLOCATION      = LISTITEM_START + 78; // => SLIDESHOW_IPTC_SUBLOCATION
static constexpr uint32_t LISTITEM_PICTURE_TIMECREATED      = LISTITEM_START + 79; // => SLIDESHOW_IPTC_TIMECREATED
static constexpr uint32_t LISTITEM_PICTURE_GPS_LAT          = LISTITEM_START + 80;    // => SLIDESHOW_EXIF_GPS_LATITUDE
static constexpr uint32_t LISTITEM_PICTURE_GPS_LON          = LISTITEM_START + 81;    // => SLIDESHOW_EXIF_GPS_LONGITUDE
static constexpr uint32_t LISTITEM_PICTURE_GPS_ALT          = LISTITEM_START + 82;    // => SLIDESHOW_EXIF_GPS_ALTITUDE
static constexpr int      LISTITEM_PICTURE_END              = LISTITEM_PICTURE_GPS_ALT;

static constexpr uint32_t LISTITEM_STUDIO                   = LISTITEM_START + 83;
static constexpr uint32_t LISTITEM_MPAA                     = LISTITEM_START + 84;
static constexpr uint32_t LISTITEM_CAST                     = LISTITEM_START + 85;
static constexpr uint32_t LISTITEM_CAST_AND_ROLE            = LISTITEM_START + 86;
static constexpr uint32_t LISTITEM_WRITER                   = LISTITEM_START + 87;
static constexpr uint32_t LISTITEM_TAGLINE                  = LISTITEM_START + 88;
static constexpr uint32_t LISTITEM_TOP250                   = LISTITEM_START + 89;
static constexpr uint32_t LISTITEM_RATING_AND_VOTES         = LISTITEM_START + 90;
static constexpr uint32_t LISTITEM_TRAILER                  = LISTITEM_START + 91;
static constexpr uint32_t LISTITEM_APPEARANCES              = LISTITEM_START + 92;
static constexpr uint32_t LISTITEM_FILENAME_AND_PATH        = LISTITEM_START + 93;
static constexpr uint32_t LISTITEM_SORT_LETTER              = LISTITEM_START + 94;
static constexpr uint32_t LISTITEM_ALBUM_ARTIST             = LISTITEM_START + 95;
static constexpr uint32_t LISTITEM_FOLDERNAME               = LISTITEM_START + 96;
static constexpr uint32_t LISTITEM_VIDEO_CODEC              = LISTITEM_START + 97;
static constexpr uint32_t LISTITEM_VIDEO_RESOLUTION         = LISTITEM_START + 98;
static constexpr uint32_t LISTITEM_VIDEO_ASPECT             = LISTITEM_START + 99;
static constexpr uint32_t LISTITEM_AUDIO_CODEC              = LISTITEM_START + 100;
static constexpr uint32_t LISTITEM_AUDIO_CHANNELS           = LISTITEM_START + 101;
static constexpr uint32_t LISTITEM_AUDIO_LANGUAGE           = LISTITEM_START + 102;
static constexpr uint32_t LISTITEM_SUBTITLE_LANGUAGE        = LISTITEM_START + 103;
static constexpr uint32_t LISTITEM_IS_FOLDER                = LISTITEM_START + 104;
static constexpr uint32_t LISTITEM_ORIGINALTITLE            = LISTITEM_START + 105;
static constexpr uint32_t LISTITEM_COUNTRY                  = LISTITEM_START + 106;
static constexpr uint32_t LISTITEM_PLAYCOUNT                = LISTITEM_START + 107;
static constexpr uint32_t LISTITEM_LASTPLAYED               = LISTITEM_START + 108;
static constexpr uint32_t LISTITEM_FOLDERPATH               = LISTITEM_START + 109;
static constexpr uint32_t LISTITEM_DISC_NUMBER              = LISTITEM_START + 110;
static constexpr uint32_t LISTITEM_FILE_EXTENSION           = LISTITEM_START + 111;
static constexpr uint32_t LISTITEM_IS_RESUMABLE             = LISTITEM_START + 112;
static constexpr uint32_t LISTITEM_PERCENT_PLAYED           = LISTITEM_START + 113;
static constexpr uint32_t LISTITEM_DATE_ADDED               = LISTITEM_START + 114;
static constexpr uint32_t LISTITEM_DBTYPE                   = LISTITEM_START + 115;
static constexpr uint32_t LISTITEM_DBID                     = LISTITEM_START + 116;
static constexpr uint32_t LISTITEM_ART                      = LISTITEM_START + 117;
static constexpr uint32_t LISTITEM_STARTTIME                = LISTITEM_START + 118;
static constexpr uint32_t LISTITEM_ENDTIME                  = LISTITEM_START + 119;
static constexpr uint32_t LISTITEM_STARTDATE                = LISTITEM_START + 120;
static constexpr uint32_t LISTITEM_ENDDATE                  = LISTITEM_START + 121;
static constexpr uint32_t LISTITEM_NEXT_TITLE               = LISTITEM_START + 122;
static constexpr uint32_t LISTITEM_NEXT_GENRE               = LISTITEM_START + 123;
static constexpr uint32_t LISTITEM_NEXT_PLOT                = LISTITEM_START + 124;
static constexpr uint32_t LISTITEM_NEXT_PLOT_OUTLINE        = LISTITEM_START + 125;
static constexpr uint32_t LISTITEM_NEXT_STARTTIME           = LISTITEM_START + 126;
static constexpr uint32_t LISTITEM_NEXT_ENDTIME             = LISTITEM_START + 127;
static constexpr uint32_t LISTITEM_NEXT_STARTDATE           = LISTITEM_START + 128;
static constexpr uint32_t LISTITEM_NEXT_ENDDATE             = LISTITEM_START + 129;
static constexpr uint32_t LISTITEM_NEXT_DURATION            = LISTITEM_START + 130;
static constexpr uint32_t LISTITEM_CHANNEL_NAME             = LISTITEM_START + 131;
static constexpr uint32_t LISTITEM_CHANNEL_GROUP            = LISTITEM_START + 132;
static constexpr uint32_t LISTITEM_HASTIMER                 = LISTITEM_START + 133;
static constexpr uint32_t LISTITEM_ISRECORDING              = LISTITEM_START + 134;
static constexpr uint32_t LISTITEM_ISENCRYPTED              = LISTITEM_START + 135;
static constexpr uint32_t LISTITEM_PARENTAL_RATING          = LISTITEM_START + 136;
static constexpr uint32_t LISTITEM_PROGRESS                 = LISTITEM_START + 137;
static constexpr uint32_t LISTITEM_HAS_EPG                  = LISTITEM_START + 138;
static constexpr uint32_t LISTITEM_VOTES                    = LISTITEM_START + 139;
static constexpr uint32_t LISTITEM_STEREOSCOPIC_MODE        = LISTITEM_START + 140;
static constexpr uint32_t LISTITEM_IS_STEREOSCOPIC          = LISTITEM_START + 141;
static constexpr uint32_t LISTITEM_INPROGRESS               = LISTITEM_START + 142;
static constexpr uint32_t LISTITEM_HASRECORDING             = LISTITEM_START + 143;
static constexpr uint32_t LISTITEM_HASREMINDER              = LISTITEM_START + 144;
static constexpr uint32_t LISTITEM_CHANNEL_NUMBER           = LISTITEM_START + 145;
static constexpr uint32_t LISTITEM_IMDBNUMBER               = LISTITEM_START + 146;
static constexpr uint32_t LISTITEM_EPISODENAME              = LISTITEM_START + 147;
static constexpr uint32_t LISTITEM_IS_COLLECTION            = LISTITEM_START + 148;
static constexpr uint32_t LISTITEM_HASTIMERSCHEDULE         = LISTITEM_START + 149;
static constexpr uint32_t LISTITEM_TIMERTYPE                = LISTITEM_START + 150;
static constexpr uint32_t LISTITEM_EPG_EVENT_TITLE          = LISTITEM_START + 151;
static constexpr uint32_t LISTITEM_DATETIME                 = LISTITEM_START + 152;
static constexpr uint32_t LISTITEM_USER_RATING              = LISTITEM_START + 153;
static constexpr uint32_t LISTITEM_TAG                      = LISTITEM_START + 154;
static constexpr uint32_t LISTITEM_SET                      = LISTITEM_START + 155;
static constexpr uint32_t LISTITEM_SETID                    = LISTITEM_START + 156;
static constexpr uint32_t LISTITEM_IS_PARENTFOLDER          = LISTITEM_START + 157;
static constexpr uint32_t LISTITEM_MOOD                     = LISTITEM_START + 158;
static constexpr uint32_t LISTITEM_CONTRIBUTORS             = LISTITEM_START + 159;
static constexpr uint32_t LISTITEM_CONTRIBUTOR_AND_ROLE     = LISTITEM_START + 160;
static constexpr uint32_t LISTITEM_TIMERISACTIVE            = LISTITEM_START + 161;
static constexpr uint32_t LISTITEM_TIMERHASCONFLICT         = LISTITEM_START + 162;
static constexpr uint32_t LISTITEM_TIMERHASERROR            = LISTITEM_START + 163;

static constexpr uint32_t LISTITEM_ADDON_NAME               = LISTITEM_START + 164;
static constexpr uint32_t LISTITEM_ADDON_VERSION            = LISTITEM_START + 165;
static constexpr uint32_t LISTITEM_ADDON_CREATOR            = LISTITEM_START + 166;
static constexpr uint32_t LISTITEM_ADDON_SUMMARY            = LISTITEM_START + 167;
static constexpr uint32_t LISTITEM_ADDON_DESCRIPTION        = LISTITEM_START + 168;
static constexpr uint32_t LISTITEM_ADDON_DISCLAIMER         = LISTITEM_START + 169;
static constexpr uint32_t LISTITEM_ADDON_BROKEN             = LISTITEM_START + 170;
static constexpr uint32_t LISTITEM_ADDON_LIFECYCLE_TYPE     = LISTITEM_START + 171;
static constexpr uint32_t LISTITEM_ADDON_LIFECYCLE_DESC     = LISTITEM_START + 172;
static constexpr uint32_t LISTITEM_ADDON_TYPE               = LISTITEM_START + 173;
static constexpr uint32_t LISTITEM_ADDON_INSTALL_DATE       = LISTITEM_START + 174;
static constexpr uint32_t LISTITEM_ADDON_LAST_UPDATED       = LISTITEM_START + 175;
static constexpr uint32_t LISTITEM_ADDON_LAST_USED          = LISTITEM_START + 176;
static constexpr uint32_t LISTITEM_STATUS                   = LISTITEM_START + 177;
static constexpr uint32_t LISTITEM_ENDTIME_RESUME           = LISTITEM_START + 178;
static constexpr uint32_t LISTITEM_ADDON_ORIGIN             = LISTITEM_START + 179;
static constexpr uint32_t LISTITEM_ADDON_NEWS               = LISTITEM_START + 180;
static constexpr uint32_t LISTITEM_ADDON_SIZE               = LISTITEM_START + 181;
static constexpr uint32_t LISTITEM_EXPIRATION_DATE          = LISTITEM_START + 182;
static constexpr uint32_t LISTITEM_EXPIRATION_TIME          = LISTITEM_START + 183;
static constexpr uint32_t LISTITEM_PROPERTY                 = LISTITEM_START + 184;
static constexpr uint32_t LISTITEM_EPG_EVENT_ICON           = LISTITEM_START + 185;
static constexpr uint32_t LISTITEM_HASREMINDERRULE          = LISTITEM_START + 186;
static constexpr uint32_t LISTITEM_HASARCHIVE               = LISTITEM_START + 187;
static constexpr uint32_t LISTITEM_ISPLAYABLE               = LISTITEM_START + 188;
static constexpr uint32_t LISTITEM_FILENAME_NO_EXTENSION    = LISTITEM_START + 189;
static constexpr uint32_t LISTITEM_CURRENTITEM              = LISTITEM_START + 190;
static constexpr uint32_t LISTITEM_IS_NEW                   = LISTITEM_START + 191;
static constexpr uint32_t LISTITEM_DISC_TITLE               = LISTITEM_START + 192;
static constexpr uint32_t LISTITEM_IS_BOXSET                = LISTITEM_START + 193;
static constexpr uint32_t LISTITEM_TOTALDISCS               = LISTITEM_START + 194;
static constexpr uint32_t LISTITEM_RELEASEDATE              = LISTITEM_START + 195;
static constexpr uint32_t LISTITEM_ORIGINALDATE             = LISTITEM_START + 196;
static constexpr uint32_t LISTITEM_BPM                      = LISTITEM_START + 197;
static constexpr uint32_t LISTITEM_UNIQUEID                 = LISTITEM_START + 198;
static constexpr uint32_t LISTITEM_BITRATE                  = LISTITEM_START + 199;
static constexpr uint32_t LISTITEM_SAMPLERATE               = LISTITEM_START + 200;
static constexpr uint32_t LISTITEM_MUSICCHANNELS            = LISTITEM_START + 201;
static constexpr uint32_t LISTITEM_IS_PREMIERE              = LISTITEM_START + 202;
static constexpr uint32_t LISTITEM_IS_FINALE                = LISTITEM_START + 203;
static constexpr uint32_t LISTITEM_IS_LIVE                  = LISTITEM_START + 204;
static constexpr uint32_t LISTITEM_TVSHOWDBID               = LISTITEM_START + 205;
static constexpr uint32_t LISTITEM_ALBUMSTATUS              = LISTITEM_START + 206;
static constexpr uint32_t LISTITEM_ISAUTOUPDATEABLE         = LISTITEM_START + 207;
static constexpr uint32_t LISTITEM_VIDEO_HDR_TYPE           = LISTITEM_START + 208;
static constexpr uint32_t LISTITEM_SONG_VIDEO_URL           = LISTITEM_START + 209;
static constexpr uint32_t LISTITEM_PARENTAL_RATING_CODE     = LISTITEM_START + 210;
static constexpr uint32_t LISTITEM_VIDEO_WIDTH              = LISTITEM_START + 211;
static constexpr uint32_t LISTITEM_VIDEO_HEIGHT             = LISTITEM_START + 212;
static constexpr uint32_t LISTITEM_HASVIDEOVERSIONS         = LISTITEM_START + 213;
static constexpr uint32_t LISTITEM_ISVIDEOEXTRA             = LISTITEM_START + 214;
static constexpr uint32_t LISTITEM_VIDEOVERSION_NAME        = LISTITEM_START + 215;
static constexpr uint32_t LISTITEM_HASVIDEOEXTRAS           = LISTITEM_START + 216;
static constexpr uint32_t LISTITEM_PVR_CLIENT_NAME          = LISTITEM_START + 217;
static constexpr uint32_t LISTITEM_PVR_INSTANCE_NAME        = LISTITEM_START + 218;
static constexpr uint32_t LISTITEM_CHANNEL_LOGO             = LISTITEM_START + 219;
static constexpr uint32_t LISTITEM_PVR_GROUP_ORIGIN         = LISTITEM_START + 220;
static constexpr uint32_t LISTITEM_PARENTAL_RATING_ICON     = LISTITEM_START + 221;
static constexpr uint32_t LISTITEM_PARENTAL_RATING_SOURCE   = LISTITEM_START + 222;
static constexpr uint32_t LISTITEM_EPISODEPART              = LISTITEM_START + 223;
static constexpr uint32_t LISTITEM_MEDIAPROVIDERS           = LISTITEM_START + 224;
static constexpr uint32_t LISTITEM_TITLE_EXTRAINFO          = LISTITEM_START + 225;
static constexpr uint32_t LISTITEM_DECODED_FILENAME_AND_PATH  = LISTITEM_START + 226;

static constexpr int      LISTITEM_END                      = LISTITEM_START + 2500;

static constexpr int      CONDITIONAL_LABEL_START           = LISTITEM_END + 1; // 37501
static constexpr int      CONDITIONAL_LABEL_END             = 39999;

// the multiple information vector
static constexpr int      MULTI_INFO_START                  = 40000;
static constexpr int      MULTI_INFO_END                    = 99999;
static constexpr uint32_t COMBINED_VALUES_START             = 100000;

// listitem info Flags
// Stored in the top 8 bits of GUIInfo::m_data1
// therefore we only have room for 8 flags
static constexpr uint32_t INFOFLAG_LISTITEM_WRAP            = static_cast<uint32_t>(1 << 25);  // Wrap ListItem lookups
static constexpr uint32_t INFOFLAG_LISTITEM_POSITION        = static_cast<uint32_t>(1 << 26);  // ListItem lookups based on cursor position
static constexpr uint32_t INFOFLAG_LISTITEM_ABSOLUTE        = static_cast<uint32_t>(1 << 27);  // Absolute ListItem lookups
static constexpr uint32_t INFOFLAG_LISTITEM_NOWRAP          = static_cast<uint32_t>(1 << 28);  // Do not wrap ListItem lookups
static constexpr uint32_t INFOFLAG_LISTITEM_CONTAINER       = static_cast<uint32_t>(1 << 29);  // Lookup the item in given container
// clang-format on
