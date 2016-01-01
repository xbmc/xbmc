#pragma once
/*
 *      Copyright (C) 2010-2015 Team XBMC
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

/**
 * Bit options to pass to IAE::MakeStream
 */
enum AEStreamOptions
{
  AESTREAM_FORCE_RESAMPLE = 1 << 0,   /* force resample even if rates match */
  AESTREAM_PAUSED         = 1 << 1,   /* create the stream paused */
  AESTREAM_AUTOSTART      = 1 << 2,   /* autostart the stream when enough data is buffered */
  AESTREAM_BYPASS_ADSP    = 1 << 3    /* if this option is set the ADSP-System is bypassed and the raw stream will be passed through IAESink */
};
