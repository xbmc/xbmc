#pragma once
/*
 *      Copyright (C) 2010 Team XBMC
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
#include <stdint.h>
#include "DVDAudioTypes.h"

class CAudioStreamProbe
{
public:
  static enum DVDAudioEncodingType ProbeFormat(uint8_t *data, unsigned int size, unsigned int &bitRate);  
private:
  static bool ProbeAC3(uint8_t *data, unsigned int size, unsigned int &bitRate, bool &extended);
  static bool ProbeDTS(uint8_t *data, unsigned int size, unsigned int &bitRate);
  static bool ProbeAAC(uint8_t *data, unsigned int size, unsigned int &bitRate);
};

