#pragma once

/*
 *      Copyright (C) 2011 Plex
 *      http://www.plexapp.com
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
#include <map>

#include "FileItem.h"
#include "guilib/GUIWindow.h"
#include "pictures/PictureThumbLoader.h"
#include "PlexContentPlayerMixin.h"
#include "PlexContentWorker.h"
#include "Stopwatch.h"
#include "ThumbLoader.h"

class PlexContentWorkerManager;

class CGUIWindowPlexSearch : public CGUIWindow, 
                             public PlexContentPlayerMixin
{
 public:
  
  CGUIWindowPlexSearch();
  virtual ~CGUIWindowPlexSearch();
  
  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage& message);
  virtual void OnInitWindow();
  virtual void Render();
  
  bool InProgress();
  
 protected:
  
  void Bind();
  void Reset();
  void StartSearch(const std::string& search);
  void Character(WCHAR ch);
  void Backspace();
  void UpdateLabel();
  void OnClickButton(int iButtonControl);
  void MoveCursor(int iAmount);
  int  GetCursorPos() const;
  char GetCharacter(int iButton);
  
 private:
  
  std::string BuildSearchUrl(const std::string& theUrl, const std::string& theQuery);
  virtual void SaveStateBeforePlay(CGUIBaseContainer* container);
  
  CVideoThumbLoader  m_videoThumbLoader;
  CMusicThumbLoader  m_musicThumbLoader;
  CStdStringW        m_strEdit;
  DWORD              m_lastSearchUpdate;
  DWORD              m_lastArrowKey;
  bool               m_resetOnNextResults;
  
  int                m_selectedContainerID;
  int                m_selectedItem;
  
  std::map<int, Group> m_categoryResults;
  
  PlexContentWorkerManager* m_workerManager;
};
