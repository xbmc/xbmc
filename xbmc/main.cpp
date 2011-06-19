/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include "xbmc.h"
#include <stdio.h>

int main(int argc, const char* argv[])
{
  if (!XBMC_Init(grfxaXBMC, "xbmc", argc, argv))
    return -1;

  int status;
  try
  {
    status = g_application.Run();
  }
  catch(...)
  {
    fprintf(stderr, "ERROR: Exception caught on main loop. Exiting\n");
    status = -1;
  }
  return status;
}

