/*
 *      Copyright (C) 2005-2011 Team XBMC
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

#include "NetworkUtils.h"
#include <stdio.h>

std::string CNetworkUtils::IPTotring(unsigned int ip)
{
  char buffer[16];
  sprintf(buffer, "%i:%i:%i:%i", ip & 0xff, (ip & (0xff << 8)) >> 8, (ip & (0xff << 16)) >> 16, (ip & (0xff << 24)) >> 24);
  std::string returnString = buffer;
  return returnString;
}
