/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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
