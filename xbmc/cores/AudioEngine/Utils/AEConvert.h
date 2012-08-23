#pragma once
/*
 *      Copyright (C) 2010-2012 Team XBMC
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

#include <stdint.h>
#include "../AEAudioFormat.h"

class CAEToFloatConv{
public:
  CAEToFloatConv(enum AEDataFormat dataFormat);
  ~CAEToFloatConv();
  unsigned int convert(const uint8_t *data, unsigned int samples, float   *dest);
  bool is_valid();
private:
  void* priv_data;
};

class CAEFrFloatConv{
public:
  CAEFrFloatConv(enum AEDataFormat dataFormat);
  ~CAEFrFloatConv();
  unsigned int convert(const float   *data, unsigned int samples, uint8_t *dest);
  bool is_valid();
private:
  void* priv_data;
};
