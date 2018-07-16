/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/**
 * Bit options to pass to IAE::MakeStream
 */
enum AEStreamOptions
{
  AESTREAM_FORCE_RESAMPLE = 1 << 0,   /* force resample even if rates match */
  AESTREAM_PAUSED         = 1 << 1,   /* create the stream paused */
  AESTREAM_AUTOSTART      = 1 << 2,   /* autostart the stream when enough data is buffered */
};
