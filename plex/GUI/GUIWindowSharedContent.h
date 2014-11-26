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

#include "GUIPlexMediaWindow.h"
#include "PlexTypes.h"

class CGUIWindowSharedContent : public CGUIPlexMediaWindow
{
 public:

  CGUIWindowSharedContent()
    : CGUIPlexMediaWindow(WINDOW_SHARED_CONTENT, "MySharedContent.xml")
  {
  }
  
  bool OnMessage(CGUIMessage& message)
  {
    bool ret = CGUIPlexMediaWindow::OnMessage(message);
    
    if (message.GetMessage() == GUI_MSG_PLEX_SERVER_DATA_LOADED)
    {
      CStdString uuid = message.GetStringParam();
      CPlexServerPtr server = g_plexApplication.serverManager->FindByUUID(uuid);
      if (server && !server->IsShared())
      {
        CLog::Log(LOGDEBUG, "CGUIWindowSharedContent::OnMessage got loaded signal for non owned server. reloading.");
        Update(m_vecItems->GetPath(), false, false);
      }
    }
    
    return ret;
  }
};
