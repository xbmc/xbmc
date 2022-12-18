/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

constexpr int LOG_LEVEL_NONE = -1; // nothing at all is logged
constexpr int LOG_LEVEL_NORMAL = 0; // shows notice, error, severe and fatal
constexpr int LOG_LEVEL_DEBUG = 1; // shows all
constexpr int LOG_LEVEL_DEBUG_FREEMEM = 2; // shows all + shows freemem on screen
constexpr int LOG_LEVEL_MAX = LOG_LEVEL_DEBUG_FREEMEM;

// ones we use in the code
constexpr int LOGDEBUG = 0;
constexpr int LOGINFO = 1;
constexpr int LOGWARNING = 2;
constexpr int LOGERROR = 3;
constexpr int LOGFATAL = 4;
constexpr int LOGNONE = 5;

// extra masks - from bit 5
constexpr int LOGMASKBIT = 5;
constexpr int LOGMASK = ((1 << LOGMASKBIT) - 1);

constexpr int LOGSAMBA = (1 << (LOGMASKBIT + 0));
constexpr int LOGCURL = (1 << (LOGMASKBIT + 1));
constexpr int LOGFFMPEG = (1 << (LOGMASKBIT + 2));
constexpr int LOGDBUS = (1 << (LOGMASKBIT + 4));
constexpr int LOGJSONRPC = (1 << (LOGMASKBIT + 5));
constexpr int LOGAUDIO = (1 << (LOGMASKBIT + 6));
constexpr int LOGAIRTUNES = (1 << (LOGMASKBIT + 7));
constexpr int LOGUPNP = (1 << (LOGMASKBIT + 8));
constexpr int LOGCEC = (1 << (LOGMASKBIT + 9));
constexpr int LOGVIDEO = (1 << (LOGMASKBIT + 10));
constexpr int LOGWEBSERVER = (1 << (LOGMASKBIT + 11));
constexpr int LOGDATABASE = (1 << (LOGMASKBIT + 12));
constexpr int LOGAVTIMING = (1 << (LOGMASKBIT + 13));
constexpr int LOGWINDOWING = (1 << (LOGMASKBIT + 14));
constexpr int LOGPVR = (1 << (LOGMASKBIT + 15));
constexpr int LOGEPG = (1 << (LOGMASKBIT + 16));
constexpr int LOGANNOUNCE = (1 << (LOGMASKBIT + 17));
constexpr int LOGWSDISCOVERY = (1 << (LOGMASKBIT + 18));
constexpr int LOGADDONS = (1 << (LOGMASKBIT + 19));
