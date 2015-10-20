#pragma once
/*
 *      Copyright (C) 2015 Team Kodi
 *      http://kodi.tv
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

namespace jni
{

class CJNIAudioFormat
{
  public:
    static void PopulateStaticFields();

    static int ENCODING_PCM_16BIT;

    static int CHANNEL_OUT_STEREO;
    static int CHANNEL_OUT_5POINT1;

    static int CHANNEL_OUT_FRONT_LEFT;
    static int CHANNEL_OUT_FRONT_LEFT_OF_CENTER;
    static int CHANNEL_OUT_FRONT_CENTER;
    static int CHANNEL_OUT_FRONT_RIGHT_OF_CENTER;
    static int CHANNEL_OUT_FRONT_RIGHT;
    static int CHANNEL_OUT_LOW_FREQUENCY;
    static int CHANNEL_OUT_SIDE_LEFT;
    static int CHANNEL_OUT_SIDE_RIGHT;
    static int CHANNEL_OUT_BACK_LEFT;
    static int CHANNEL_OUT_BACK_CENTER;
    static int CHANNEL_OUT_BACK_RIGHT;

    static int CHANNEL_INVALID;
};

};

