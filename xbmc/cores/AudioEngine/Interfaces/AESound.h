/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>
#include "cores/AudioEngine/Utils/AEAudioFormat.h"

class IAESound
{
protected:
  friend class IAE;
  explicit IAESound(const std::string &filename) {}
  virtual ~IAESound() = default;

public:
  /* play the sound this object represents */
  virtual void Play() = 0;

  /* stop playing the sound this object represents */
  virtual void Stop() = 0;

  /* return true if the sound is currently playing */
  virtual bool IsPlaying() = 0;

  /* set the playback channel of this sound, AE_CH_NULL for all */
  virtual void SetChannel(AEChannel channel) = 0;

  /* get the current playback channel of this sound, AE_CH_NULL for all */
  virtual AEChannel GetChannel() = 0;

  /* set the playback volume of this sound */
  virtual void SetVolume(float volume) = 0;

  /* get the current playback volume of this sound */
  virtual float GetVolume() = 0;
};

