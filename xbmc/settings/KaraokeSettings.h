/*
 *      Copyright (C) 2005-2011 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "utils/XMLUtils.h"

class CKaraokeSettings
{
  public:
    CKaraokeSettings();
    CKaraokeSettings(TiXmlElement *pRootElement);
    bool AlwaysEmptyOnCDGs();                 // always have empty background on CDG files
    bool KeepDelay();                         // store user-changed song delay in the database
    bool ChangeGenreForSongs();
    bool UseSongSpecificBackground();         // use song-specific video or image if available instead of default
    float SyncDelayCDG();                     // seems like different delay is needed for CDG and MP3s
    float SyncDelayLRC();
    int StartIndex();                         // auto-assign numbering start from this value
    CStdString DefaultBackgroundType();       // empty string or "vis", "image" or "video"
    CStdString DefaultBackgroundFilePath();   // only for "image" or "video" types above
  private:
    void Initialise();
    bool m_alwaysEmptyOnCdgs;
    bool m_keepDelay;
    bool m_changeGenreForSongs;
    bool m_useSongSpecificBackground;
    float m_syncDelayCDG;
    float m_syncDelayLRC;
    int m_startIndex;
    CStdString m_defaultBackgroundType;
    CStdString m_defaultBackgroundFilePath;
};
