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

#ifndef DIALOGS_GUIDIALOGMUTEBUG_H_INCLUDED
#define DIALOGS_GUIDIALOGMUTEBUG_H_INCLUDED
#include "GUIDialogMuteBug.h"
#endif

#ifndef DIALOGS_GUIUSERMESSAGES_H_INCLUDED
#define DIALOGS_GUIUSERMESSAGES_H_INCLUDED
#include "GUIUserMessages.h"
#endif

#ifndef DIALOGS_APPLICATION_H_INCLUDED
#define DIALOGS_APPLICATION_H_INCLUDED
#include "Application.h"
#endif


// the MuteBug is a true modeless dialog

CGUIDialogMuteBug::CGUIDialogMuteBug(void)
    : CGUIDialog(WINDOW_DIALOG_MUTE_BUG, "DialogMuteBug.xml")
{
  m_loadType = LOAD_ON_GUI_INIT;
}

CGUIDialogMuteBug::~CGUIDialogMuteBug(void)
{}

void CGUIDialogMuteBug::UpdateVisibility()
{
  if (g_application.IsMuted() || g_application.GetVolume(false) <= VOLUME_MINIMUM)
    Show();
  else
    Close();
}
