/*
  Adplug - Replayer for many OPL2/OPL3 audio file formats.
  Copyright (C) 1999 - 2003 Simon Peter, <dn.tlp@gmx.net>, et al.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

  xad.cpp - XAD shell player by Riven the Mage <riven@ok.ru>
*/

#include "xad.h"
#include "debug.h"

/* -------- Public Methods -------------------------------- */

CxadPlayer::CxadPlayer(Copl * newopl) : CPlayer(newopl)
{
  tune = 0;
}

CxadPlayer::~CxadPlayer()
{
  if (tune)
    delete [] tune;
}

bool CxadPlayer::load(const std::string &filename, const CFileProvider &fp)
{
  binistream *f = fp.open(filename); if(!f) return false;
  bool ret = false;

  // load header
  xad.id = f->readInt(4);
  f->readString(xad.title, 36);
  f->readString(xad.author, 36);
  xad.fmt = f->readInt(2);
  xad.speed = f->readInt(1);
  xad.reserved_a = f->readInt(1);

  // 'XAD!' - signed ?
  if(xad.id != 0x21444158) { fp.close(f); return false; }

  // get file size
  tune_size = fp.filesize(f) - 80;

  // load()
  tune = new unsigned char [tune_size];
  f->readString((char *)tune, tune_size);
  fp.close(f);

  ret = xadplayer_load();

  if (ret)
    rewind(0);

  return ret;
}

void CxadPlayer::rewind(int subsong)
{
  opl->init();

  plr.speed = xad.speed;
  plr.speed_counter = 1;
  plr.playing = 1;
  plr.looping = 0;

  // rewind()
  xadplayer_rewind(subsong);

#ifdef DEBUG
  AdPlug_LogWrite("-----------\n");
#endif
}

bool CxadPlayer::update()
{
  if (--plr.speed_counter)
    goto update_end;

  plr.speed_counter = plr.speed;

  // update()
  xadplayer_update();

update_end:
  return (plr.playing && (!plr.looping));
}

float CxadPlayer::getrefresh()
{
  return xadplayer_getrefresh();
}

std::string CxadPlayer::gettype()
{
  return xadplayer_gettype();
}

std::string CxadPlayer::gettitle()
{
  return xadplayer_gettitle();
}

std::string CxadPlayer::getauthor()
{
  return xadplayer_getauthor();
}

std::string CxadPlayer::getinstrument(unsigned int i)
{
  return xadplayer_getinstrument(i);
}

unsigned int CxadPlayer::getinstruments()
{
  return xadplayer_getinstruments();
}

/* -------- Protected Methods ------------------------------- */

void CxadPlayer::opl_write(int reg, int val)
{
  adlib[reg] = val;
#ifdef DEBUG
  AdPlug_LogWrite("[ %02X ] = %02X\n",reg,val);
#endif
  opl->write(reg,val);
}
