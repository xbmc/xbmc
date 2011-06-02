/*
 *      Copyright (C) 2011 Fred Hoogduin
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

#include <vector>
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include "activerecording.h"


cActiveRecording::cActiveRecording(void)
{
}


cActiveRecording::~cActiveRecording(void)
{
}

// This is a minimalistic parser, parsing only the fields that
// are currently used by the implementation
bool cActiveRecording::Parse(const Json::Value& data)
{
  // From the Active Recording class pickup the Program class
  Json::Value programobject;
  programobject = data["Program"];

  // Then, from the Program class, pick up the upcoming program id
  upcomingprogramid = programobject["UpcomingProgramId"].asString();

  return true;
}
