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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
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
