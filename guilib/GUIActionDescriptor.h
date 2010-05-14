/*
 *      Copyright (C) 2009-2010 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifndef GUI_ACTION_DESCRIPTOR
#define GUI_ACTION_DESCRIPTOR

#include "StdString.h"
#include "system.h"

class CGUIActionDescriptor
{
public:
  typedef enum { LANG_XBMC = 0, LANG_PYTHON = 1 /*, LANG_JAVASCRIPT = 2 */ } ActionLang;

  CGUIActionDescriptor()
  {
    m_lang = LANG_XBMC;
    m_action = "";
    m_sourceWindowId = -1;
  }

  CGUIActionDescriptor(CStdString& action)
  {
    m_lang = LANG_XBMC;
    m_action = action;
    m_sourceWindowId = -1;
  }

  CGUIActionDescriptor(ActionLang lang, CStdString& action)
  {
    m_lang = lang;
    m_action = action;
    m_sourceWindowId = -1;
  }

  CStdString m_action;
  ActionLang m_lang;
  int m_sourceWindowId; // the id of the window that was a source of an action
};

#endif
