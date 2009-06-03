/*
 * AdPlug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2003 Simon Peter <dn.tlp@gmx.net>, et al.
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

#include <stdlib.h>
#include <string.h>

#include "players.h"

/***** CPlayerDesc *****/

CPlayerDesc::CPlayerDesc()
  : factory(0), extensions(0), extlength(0)
{
}

CPlayerDesc::CPlayerDesc(const CPlayerDesc &pd)
  : factory(pd.factory), filetype(pd.filetype), extlength(pd.extlength)
{
  if(pd.extensions) {
    extensions = (char *)malloc(extlength);
    memcpy(extensions, pd.extensions, extlength);
  } else
    extensions = 0;
}

CPlayerDesc::CPlayerDesc(Factory f, const std::string &type, const char *ext)
  : factory(f), filetype(type), extensions(0)
{
  const char *i = ext;

  // Determine length of passed extensions list
  while(*i) i += strlen(i) + 1;
  extlength = i - ext + 1;	// length = difference between last and first char + 1

  extensions = (char *)malloc(extlength);
  memcpy(extensions, ext, extlength);
}

CPlayerDesc::~CPlayerDesc()
{
  if(extensions) free(extensions);
}

void CPlayerDesc::add_extension(const char *ext)
{
  unsigned long newlength = extlength + strlen(ext) + 1;

  extensions = (char *)realloc(extensions, newlength);
  strcpy(extensions + extlength - 1, ext);
  extensions[newlength - 1] = '\0';
  extlength = newlength;
}

const char *CPlayerDesc::get_extension(unsigned int n) const
{
  const char	*i = extensions;
  unsigned int	j;

  for(j = 0; j < n && (*i); j++, i += strlen(i) + 1) ;
  return (*i != '\0' ? i : 0);
}

/***** CPlayers *****/

const CPlayerDesc *CPlayers::lookup_filetype(const std::string &ftype) const
{
  const_iterator	i;

  for(i = begin(); i != end(); i++)
    if((*i)->filetype == ftype)
      return *i;

  return 0;
}

const CPlayerDesc *CPlayers::lookup_extension(const std::string &extension) const
{
  const_iterator	i;
  unsigned int		j;

  for(i = begin(); i != end(); i++)
    for(j = 0; (*i)->get_extension(j); j++)
      if(!stricmp(extension.c_str(), (*i)->get_extension(j)))
	return *i;

  return 0;
}
