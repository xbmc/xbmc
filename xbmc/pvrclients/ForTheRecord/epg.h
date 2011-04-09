#pragma once
/*
 *      Copyright (C) 2010 Marcel Groothuis
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

#ifndef __EPG_H
#define __EPG_H

#include <string>
#include "libXBMC_addon.h"
#include "libXBMC_pvr.h"
#include <json/json.h>

class cEpg
{
private:
  std::string m_guideprogramid;
  std::string m_title;
  std::string m_subtitle;
  std::string m_description;
  time_t m_starttime;
  time_t m_endtime;
  time_t m_utcdiff;

public:
  cEpg();
  virtual ~cEpg();
  void Reset();

  bool Parse(const Json::Value& data);
  const std::string& UniqueId(void) const { return m_guideprogramid; }
  time_t StartTime(void) const { return m_starttime; }
  time_t EndTime(void) const { return m_endtime; }
  const char *Title(void) const { return m_title.c_str(); }
  const char *Subtitle(void) const { return m_subtitle.c_str(); }
  const char *Description(void) const { return m_description.c_str(); }
};

#endif //__EPG_H
