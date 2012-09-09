#pragma once

/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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

#include "guilib/GUIWindow.h"

class CKaraokeLyrics;
class CKaraokeWindowBackground;


class CGUIWindowKaraokeLyrics : public CGUIWindow
{
public:
  CGUIWindowKaraokeLyrics(void);
  virtual ~CGUIWindowKaraokeLyrics(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  virtual void Render();

  void    newSong( CKaraokeLyrics * lyrics );
  void    pauseSong( bool now_paused );
  void    stopSong();

protected:

  //! Critical section protects this class from requests from different threads
  CCriticalSection   m_CritSection;

  //! Pointer to karaoke lyrics renderer
  CKaraokeLyrics  *  m_Lyrics;

  //! Pointer to background object
  CKaraokeWindowBackground * m_Background;
};
