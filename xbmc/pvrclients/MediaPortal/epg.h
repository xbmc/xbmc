#pragma once
/*
 *      Copyright (C) 2005-2009 Team XBMC
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

#ifndef __EPG_H
#define __EPG_H

#include <stdlib.h>
#include "libXBMC_addon.h"
#include "libXBMC_pvr.h"

class cEpg
{
private:
  unsigned int m_uid;
  string m_title;
  string m_shortText;
  string m_description;
  //string m_aux;
  time_t m_StartTime;
  time_t m_EndTime;
  int m_Duration;
  string m_genre;
  int m_genre_type;
  int m_genre_subtype;
  //time_t m_vps;              // Video Programming Service timestamp (VPS, aka "Programme Identification Label", PIL)
  time_t m_UTCdiff;

public:
  cEpg();
  virtual ~cEpg();
  void Reset();

  bool ParseLine(string& data);
  //bool ParseEntryLine(const char *s);
  //const char *Aux(void) const { return m_aux; }
  int UniqueId(void) const { return m_uid; }
  time_t StartTime(void) const { return m_StartTime; }
  time_t EndTime(void) const { return m_EndTime; }
  time_t Duration(void) const { return m_Duration; }
  //time_t Vps(void) const { return m_vps; }
  //void SetVps(time_t Vps);
  const char *Title(void) const { return m_title.c_str(); }
  const char *ShortText(void) const { return m_shortText.c_str(); }
  const char *Description(void) const { return m_description.c_str(); }
  const char *Genre(void) const { return m_genre.c_str(); }
  int GenreType(void) const { return m_genre_type; }
  int GenreSubType(void) const { return m_genre_subtype; }
  //void SetTitle(const char *Title);
  //void SetShortText(const char *ShortText);
  //void SetDescription(const char *Description);
  void SetGenre(string& Genre, int genreType, int genreSubType);


};

#endif //__EPG_H
