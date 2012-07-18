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

#ifndef THUMBSTORE_H_
#define THUMBSTORE_H_

#ifdef _WIN32
#include <unordered_map>
#else
#include <tr1/unordered_map>
#endif
#include <libspotify/api.h>
#include <string.h>
#include "URL.h"

using namespace std;

namespace addon_music_spotify {

  class SxThumb;
  class ThumbStore {
  public:

    static ThumbStore *getInstance();
    static void deInit();

    SxThumb *getThumb(const unsigned char* image);

    void removeThumb(const unsigned char* image);
    void removeThumb(SxThumb* thumb);

    CStdString *getFanart(const char *artist_name);

  private:
    ThumbStore();
    virtual ~ThumbStore();

    static ThumbStore *m_instance;

    typedef std::tr1::unordered_map<const unsigned char*, SxThumb*> thumbMap;
    thumbMap m_thumbs;

    typedef std::tr1::unordered_map<string, CStdString*> stringMap;
    stringMap m_fanarts;

    CStdString* m_stdFanart;
  };

} /* namespace addon_music_spotify */
#endif /* THUMBSTORE_H_ */
