/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>
#include "URL.h"

class CDACP
{
  public:
    CDACP(const std::string &active_remote_header, const std::string &hostname, int port);

    void BeginFwd();
    void BeginRewnd();
    void ToggleMute();
    void NextItem();
    void PrevItem();
    void Pause();
    void PlayPause();
    void Play();
    void Stop();
    void PlayResume();
    void ShuffleSongs();
    void VolumeDown();
    void VolumeUp();

  private:
    void SendCmd(const std::string &cmd);

    CURL m_dacpUrl;
};
