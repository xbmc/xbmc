/*
 * AdPlug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2003 Simon Peter, <dn.tlp@gmx.net>, et al.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * players.h - Players enumeration, by Simon Peter <dn.tlp@gmx.net>
 */

#ifndef H_ADPLUG_PLAYERS
#define H_ADPLUG_PLAYERS

#include <string>
#include <list>

#include "opl.h"
#include "player.h"

class CPlayerDesc
{
public:
  typedef CPlayer *(*Factory)(Copl *);

  Factory	factory;
  std::string	filetype;

  CPlayerDesc();
  CPlayerDesc(const CPlayerDesc &pd);
  CPlayerDesc(Factory f, const std::string &type, const char *ext);

  ~CPlayerDesc();

  void add_extension(const char *ext);
  const char *get_extension(unsigned int n) const;

private:
  char		*extensions;
  unsigned long	extlength;
};

class CPlayers: public std::list<const CPlayerDesc *>
{
public:
  const CPlayerDesc *lookup_filetype(const std::string &ftype) const;
  const CPlayerDesc *lookup_extension(const std::string &extension) const;
};

#endif
