/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string>
#include "libXBMC_gui.h"
#include "AddonHelpers_local.h"

using namespace std;

AddonCB *m_gui_cb = NULL;

void GUI_register_me(ADDON_HANDLE hdl)
{
  if (!hdl)
    fprintf(stderr, "libXBMC_gui-ERROR: GUI_register_me is called with NULL handle !!!\n");
  else
    m_gui_cb = (AddonCB*) hdl;
  return;
}
