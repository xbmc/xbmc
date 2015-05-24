#pragma once
/*
 *      Copyright (C) 2013 Team XBMC
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

#include "JNIBase.h"

class CJNIDisplayMetrics
{
public:
  static int 	DENSITY_DEFAULT;
  static int 	DENSITY_HIGH;
  static int 	DENSITY_LOW;
  static int 	DENSITY_MEDIUM;
  static int 	DENSITY_TV;
  static int 	DENSITY_XHIGH;
  static int 	DENSITY_XXHIGH;
  static int 	DENSITY_XXXHIGH;

  static void PopulateStaticFields();

private:
  CJNIDisplayMetrics();
  ~CJNIDisplayMetrics() {};
  static const char *m_classname;
};
