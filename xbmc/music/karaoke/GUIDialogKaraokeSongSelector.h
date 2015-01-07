#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "guilib/GUIDialog.h"
#include "music/MusicDatabase.h"
#include "music/Song.h"


class CGUIDialogKaraokeSongSelector: public CGUIDialog
{
public:
  CGUIDialogKaraokeSongSelector( int id, const char *xmlFile );
  virtual ~CGUIDialogKaraokeSongSelector(void);

  //! Key/button parser; returns 0-9 for a numeric button action, and -1 for anything else
  static int  GetKeyNumber( int actionid );

protected:
  // Those functions control the selection process
  virtual void OnInitWindow();
  virtual void OnDeinitWindow(int nextWindowID);
  virtual bool OnAction(const CAction &action);
  virtual void FrameMove();

  void OnButtonNumeric( unsigned int code, bool reset_autotimer = true ); // 0x00 - 0x09
  void OnButtonSelect(); // Song is selected
  void OnBackspace(); // Backspace pressed
  void UpdateData();

  // Configuration
  //! Auto-close timeout
  unsigned int  m_autoCloseTimeout;

  //! Start playing song as soon as it's selected?
  bool      m_startPlaying;

protected:
  //! Currently selected number
  unsigned int  m_selectedNumber;

  //! True if the number above did select some song and the info is in m_karaokeData
  bool      m_songSelected;

  //! True if we need to update fields before rendering
  bool      m_updateData;

  //! Database stuff
  CMusicDatabase m_musicdatabase;
  CSong          m_karaokeSong;
};


// A 'small' version of dialog using DialogKaraokeSongSelector.xml
class CGUIDialogKaraokeSongSelectorSmall : public CGUIDialogKaraokeSongSelector
{
  public:
    CGUIDialogKaraokeSongSelectorSmall();
    void DoModal(unsigned int startcode, int iWindowID = WINDOW_INVALID, const std::string &param = "");
};


// A 'large' version of dialog using DialogKaraokeSongSelectorLarge.xml
class CGUIDialogKaraokeSongSelectorLarge : public CGUIDialogKaraokeSongSelector
{
  public:
    CGUIDialogKaraokeSongSelectorLarge();
    void DoModal(int iWindowID = WINDOW_INVALID, const std::string &param = "");
};
