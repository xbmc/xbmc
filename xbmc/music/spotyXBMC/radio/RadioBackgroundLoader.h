/*
 spotyxbmc2 - A project to integrate Spotify into XBMC
 Copyright (C) 2011  David Erenger

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.

 For contact with the author:
 david.erenger@gmail.com
 */

#ifndef RADIOBACKGROUNDLOADER_H_
#define RADIOBACKGROUNDLOADER_H_

#include "threads/Thread.h"
#include "SxRadio.h"

namespace addon_music_spotify {

  class RadioBackgroundLoader: public CThread {
  public:
    RadioBackgroundLoader(SxRadio* radio);
    virtual ~RadioBackgroundLoader();

    void SleepMs(int ms) {
      Sleep(ms);
    }
  private:
    void OnStartup();
    void OnExit();
    void OnException(); // signal termination handler
    void Process();
    SxRadio* m_radio;

  };

} /* namespace addon_music_spotify */
#endif /* RADIOBACKGROUNDLOADER_H_ */
