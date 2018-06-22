/*
 *      Copyright (C) 2016-2017 Team Kodi
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
#include "cores/RetroPlayer/streams/RetroPlayerStreamTypes.h"
#include "games/controllers/ControllerTypes.h"
#include "input/keyboard/KeyboardTypes.h"

extern "C" {
#include "libavutil/pixfmt.h"
}

namespace KODI
{
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
     * \brief Translate stream type (Game API to RetroPlayer).
     * \param gameType The stream type to translate.
     * \param[out] retroType The translated stream type.
     * \return True if the Game API type was translated to a valid RetroPlayer type
     */
    static bool TranslateStreamType(GAME_STREAM_TYPE gameType, RETRO::StreamType &retroType);

    /*!
     * \brief Translate pixel format (Game API to RetroPlayer/FFMPEG).
     * \param format The pixel format to translate.
     * \return Translated pixel format.
     */
    static AVPixelFormat TranslatePixelFormat(GAME_PIXEL_FORMAT format);

    /*!
     * \brief Translate pixel format (RetroPlayer/FFMPEG to Game API).
     * \param format The pixel format to translate.
     * \return Translated pixel format.
     */
    static GAME_PIXEL_FORMAT TranslatePixelFormat(AVPixelFormat format);

    /*!
     * \brief Translate audio PCM format (Game API to RetroPlayer).
     * \param format The audio PCM format to translate.
     * \return Translated audio PCM format.
     */
    static RETRO::PCMFormat TranslatePCMFormat(GAME_PCM_FORMAT format);

    /*!
     * \brief Translate audio channels (Game API to RetroPlayer).
     * \param format The audio channels to translate.
     * \return Translated audio channels.
     */
    static RETRO::AudioChannel TranslateAudioChannel(GAME_AUDIO_CHANNEL channel);

    /*!
     * \brief Translate video rotation (Game API to RetroPlayer).
     * \param rotation The video rotation to translate.
     * \return Translated video rotation.
     */
    static RETRO::VideoRotation TranslateRotation(GAME_VIDEO_ROTATION rotation);

    /*!
     * \brief Translate key modifiers (Kodi to Game API).
     * \param modifiers The key modifiers to translate (e.g. Shift, Ctrl).
     * \return Translated key modifiers.
     */
    static GAME_KEY_MOD GetModifiers(KEYBOARD::Modifier modifier);

    /*!
     * \brief Translate region to string representation (e.g. for logging).
     * \param error The region to translate (e.g. PAL, NTSC).
     * \return Translated region.
     */
    static const char* TranslateRegion(GAME_REGION region);

    /*!
     * \brief Translate port type (Game API to Kodi)
     * \param portType  The port type to translate
     * \return Translated port type
     */
    static PORT_TYPE TranslatePortType(GAME_PORT_TYPE portType);
  };
}
}
