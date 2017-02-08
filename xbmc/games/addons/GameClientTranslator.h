/*
 *      Copyright (C) 2016 Team Kodi
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "addons/kodi-addon-dev-kit/include/kodi/kodi_game_types.h"
#include "cores/AudioEngine/Utils/AEChannelData.h"
#include "input/Key.h"

#include "libavcodec/avcodec.h"
#include "libavutil/pixfmt.h"

namespace GAME
{
  /*!
   * \ingroup games
   * \brief Translates data types from Game API to the corresponding format in Kodi.
   *
   * This class is stateless.
   */
  class CGameClientTranslator
  {
    CGameClientTranslator() = delete;

  public:
    /*!
     * \brief Translates game errors to string representation (e.g. for logging).
     * \param error The error to translate.
     * \return Translated error.
     */
    static const char* ToString(GAME_ERROR error);

    /*!
     * \brief Translates game memory types to string representation (e.g. for logging).
     * \param memory The memory type to translate.
     * \return Translated memory type.
     */
    static const char* ToString(GAME_MEMORY error);

    /*!
     * \brief Translate pixel format (Game API to FFMPEG).
     * \param format The pixel format to translate.
     * \return Translated pixel format.
     */
    static AVPixelFormat TranslatePixelFormat(GAME_PIXEL_FORMAT format);

    /*!
     * \brief Translate video codec (Game API to FFMPEG).
     * \param format The video codec to translate.
     * \return Translated video codec format.
     */
    static AVCodecID TranslateVideoCodec(GAME_VIDEO_CODEC codec);

    /*!
     * \brief Translate audio PCM format (Game API to AudioEngine).
     * \param format The audio PCM format to translate.
     * \return Translated audio PCM format.
     */
    static AEDataFormat TranslatePCMFormat(GAME_PCM_FORMAT format);

    /*!
     * \brief Translate audio channels (Game API to AudioEngine).
     * \param format The audio channels to translate.
     * \return Translated audio channels.
     */
    static AEChannel TranslateAudioChannel(GAME_AUDIO_CHANNEL channel);

    /*!
     * \brief Translate audio codec (Game API to FFMPEG).
     * \param format The audio codec to translate.
     * \return Translated audio codec format.
     */
    static AVCodecID TranslateAudioCodec(GAME_AUDIO_CODEC codec);

    /*!
     * \brief Translate key modifiers (Kodi to Game API).
     * \param modifiers The key modifiers to translate (e.g. Shift, Ctrl).
     * \return Translated key modifiers.
     */
    static GAME_KEY_MOD  GetModifiers(CKey::Modifier modifier);

    /*!
     * \brief Translate region to string representation (e.g. for logging).
     * \param error The region to translate (e.g. PAL, NTSC).
     * \return Translated region.
     */
    static const char* TranslateRegion(GAME_REGION region);
  };
}
