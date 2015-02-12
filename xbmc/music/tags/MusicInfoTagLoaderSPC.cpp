/*
 *      Copyright (C) 2005-2013 Team XBMC
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


#include "MusicInfoTagLoaderSPC.h"
#include "snesapu/Types.h"
#include "MusicInfoTag.h"
#include "filesystem/File.h"
#include "utils/log.h"

using namespace XFILE;
using namespace MUSIC_INFO;

// copied from libspc, then modified. thanks :)
SPC_ID666 *SPC_get_id666FP (CFile& file)
{
  SPC_ID666 *id;
  unsigned char playtime_str[4] = { 0, 0, 0, 0 };

  id = (SPC_ID666 *)malloc(sizeof(*id));
  if (id == NULL)
    return NULL;

  file.Seek(0x23,SEEK_SET);
  char c;
  if (file.Read(&c,1) != 1 || c == 27) 
  {
      free(id);
      return NULL;
  }

  file.Seek(0x2E,SEEK_SET);
  if (file.Read(id->songname, 32) != 32)
  {
    free(id);
    return NULL;
  }
  id->songname[32] = '\0';

  if (file.Read(id->gametitle, 32) != 32)
  {
    free(id);
    return NULL;
  }
  id->gametitle[32] = '\0';

  if (file.Read(id->dumper, 16) != 16)
  {
    free(id);
    return NULL;
  }
  id->dumper[16] = '\0';

  if (file.Read(id->comments, 32) != 32)
  {
    free(id);
    return NULL;
  }
  id->comments[32] = '\0';

  file.Seek(0xA9,SEEK_SET);
  if (file.Read(playtime_str, 3) != 3)
  {
    free(id);
    return NULL;
  }
  playtime_str[3] = '\0';
  id->playtime = atoi((char*)playtime_str);

  file.Seek(0xD1,SEEK_SET);
  file.Read(&c,1);
  switch (c) {
  case 1:
      id->emulator = SPC_EMULATOR_ZSNES;
      break;
  case 2:
      id->emulator = SPC_EMULATOR_SNES9X;
      break;
  case 0:
  default:
      id->emulator = SPC_EMULATOR_UNKNOWN;
      break;
  }

  file.Seek(0xB0,SEEK_SET);
  if (file.Read(id->author, 32) != 32)
  {
    free(id);
    return NULL;
  }
  id->author[32] = '\0';

  return id;
}

CMusicInfoTagLoaderSPC::CMusicInfoTagLoaderSPC(void)
{
}

CMusicInfoTagLoaderSPC::~CMusicInfoTagLoaderSPC()
{
}

bool CMusicInfoTagLoaderSPC::Load(const std::string& strFileName, CMusicInfoTag& tag, EmbeddedArt *art)
{
  tag.SetLoaded(false);

  CFile file;
  if (!file.Open(strFileName))
  {
    CLog::Log(LOGERROR,"MusicInfoTagLoaderSPC: failed to open SPC %s",strFileName.c_str());
    return false;
  }

  tag.SetURL(strFileName);

  tag.SetLoaded(false);
  SPC_ID666* spc = SPC_get_id666FP(file);
  if (!spc)
    return false;
  if( strcmp(spc->songname,"") )
  {
    tag.SetTitle(spc->songname);
    tag.SetLoaded(true);
  }

  if( strcmp(spc->author,"") && tag.Loaded() )
    tag.SetArtist(spc->author);

  if (spc->playtime)
    tag.SetDuration(spc->playtime);
  else
    tag.SetDuration(4*60); // 4 mins

  free(spc);
  return tag.Loaded();
}

