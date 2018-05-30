/*
 *      Copyright (C) 2017 Team Kodi
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

#include "cores/RetroPlayer/streams/RetroPlayerStreamTypes.h"
#include "cores/AudioEngine/Utils/AEChannelData.h"

namespace KODI
{
namespace RETRO
{
  class CAudioTranslator
  {
  public:
    /*!
     * \brief Translate audio PCM format (Game API to AudioEngine).
     * \param format The audio PCM format to translate.
     * \return Translated audio PCM format.
     */
    static AEDataFormat TranslatePCMFormat(PCMFormat format);

    /*!
     * \brief Translate audio channels (Game API to AudioEngine).
     * \param format The audio channels to translate.
     * \return Translated audio channels.
     */
    static AEChannel TranslateAudioChannel(AudioChannel channel);
  };
}
}
