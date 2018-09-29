/*
 *  Copyright (C) 2010-2017 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

// Several Android TV devices only support 384 kbit/s as maximum
#undef  AE_AC3_ENCODE_BITRATE
#define AE_AC3_ENCODE_BITRATE 384000
