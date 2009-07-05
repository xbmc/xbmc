/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2006 Simon Peter <dn.tlp@gmx.net>, et al.
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
 * adplug.cpp - CAdPlug utility class, by Simon Peter <dn.tlp@gmx.net>
 */

#include <string>
#include <binfile.h>

#include "adplug.h"
#include "debug.h"

/***** Replayer includes *****/

#include "hsc.h"
#include "amd.h"
#include "a2m.h"
#include "imf.h"
#include "sng.h"
#include "adtrack.h"
#include "bam.h"
#include "d00.h"
#include "dfm.h"
#include "hsp.h"
#include "ksm.h"
#include "mad.h"
#include "mid.h"
#include "mkj.h"
#include "cff.h"
#include "dmo.h"
#include "s3m.h"
#include "dtm.h"
#include "fmc.h"
#include "mtk.h"
#include "rad.h"
#include "raw.h"
#include "sa2.h"
#include "bmf.h"
#include "flash.h"
#include "hybrid.h"
#include "hyp.h"
#include "psi.h"
#include "rat.h"
#include "lds.h"
#include "u6m.h"
#include "rol.h"
#include "xsm.h"
#include "dro.h"
#include "msc.h"
#include "rix.h"
#include "adl.h"

/***** CAdPlug *****/

// List of all players that come with the standard AdPlug distribution
const CPlayerDesc CAdPlug::allplayers[] = {
  CPlayerDesc(ChscPlayer::factory, "HSC-Tracker", ".hsc\0"),
  CPlayerDesc(CsngPlayer::factory, "SNGPlay", ".sng\0"),
  CPlayerDesc(CimfPlayer::factory, "Apogee IMF", ".imf\0.wlf\0.adlib\0"),
  CPlayerDesc(Ca2mLoader::factory, "Adlib Tracker 2", ".a2m\0"),
  CPlayerDesc(CadtrackLoader::factory, "Adlib Tracker", ".sng\0"),
  CPlayerDesc(CamdLoader::factory, "AMUSIC", ".amd\0"),
  CPlayerDesc(CbamPlayer::factory, "Bob's Adlib Music", ".bam\0"),
  CPlayerDesc(Cd00Player::factory, "Packed EdLib", ".d00\0"),
  CPlayerDesc(CdfmLoader::factory, "Digital-FM", ".dfm\0"),
  CPlayerDesc(ChspLoader::factory, "HSC Packed", ".hsp\0"),
  CPlayerDesc(CksmPlayer::factory, "Ken Silverman Music", ".ksm\0"),
  CPlayerDesc(CmadLoader::factory, "Mlat Adlib Tracker", ".mad\0"),
  CPlayerDesc(CmidPlayer::factory, "MIDI", ".mid\0.cmf\0.sci\0.laa\0"),
  CPlayerDesc(CmkjPlayer::factory, "MKJamz", ".mkj\0"),
  CPlayerDesc(CcffLoader::factory, "Boomtracker", ".cff\0"),
  CPlayerDesc(CdmoLoader::factory, "TwinTeam", ".dmo\0"),
  CPlayerDesc(Cs3mPlayer::factory, "Scream Tracker 3", ".s3m\0"),
  CPlayerDesc(CdtmLoader::factory, "DeFy Adlib Tracker", ".dtm\0"),
  CPlayerDesc(CfmcLoader::factory, "Faust Music Creator", ".sng\0"),
  CPlayerDesc(CmtkLoader::factory, "MPU-401 Trakker", ".mtk\0"),
  CPlayerDesc(CradLoader::factory, "Reality Adlib Tracker", ".rad\0"),
  CPlayerDesc(CrawPlayer::factory, "RdosPlay RAW", ".raw\0"),
  CPlayerDesc(Csa2Loader::factory, "Surprise! Adlib Tracker", ".sat\0.sa2\0"),
  CPlayerDesc(CxadbmfPlayer::factory, "BMF Adlib Tracker", ".xad\0"),
  CPlayerDesc(CxadflashPlayer::factory, "Flash", ".xad\0"),
  CPlayerDesc(CxadhybridPlayer::factory, "Hybrid", ".xad\0"),
  CPlayerDesc(CxadhypPlayer::factory, "Hypnosis", ".xad\0"),
  CPlayerDesc(CxadpsiPlayer::factory, "PSI", ".xad\0"),
  CPlayerDesc(CxadratPlayer::factory, "rat", ".xad\0"),
  CPlayerDesc(CldsPlayer::factory, "LOUDNESS Sound System", ".lds\0"),
  CPlayerDesc(Cu6mPlayer::factory, "Ultima 6 Music", ".m\0"),
  CPlayerDesc(CrolPlayer::factory, "Adlib Visual Composer", ".rol\0"),
  CPlayerDesc(CxsmPlayer::factory, "eXtra Simple Music", ".xsm\0"),
  CPlayerDesc(CdroPlayer::factory, "DOSBox Raw OPL", ".dro\0"),
  CPlayerDesc(CmscPlayer::factory, "Adlib MSC Player", ".msc\0"),
  CPlayerDesc(CrixPlayer::factory, "Softstar RIX OPL Music", ".rix\0"),
  CPlayerDesc(CadlPlayer::factory, "Westwood ADL", ".adl\0"),
  CPlayerDesc()
};

const CPlayers &CAdPlug::init_players(const CPlayerDesc pd[])
{
  static CPlayers	initplayers;
  unsigned int		i;

  for(i = 0; pd[i].factory; i++)
    initplayers.push_back(&pd[i]);

  return initplayers;
}

const CPlayers CAdPlug::players = CAdPlug::init_players(CAdPlug::allplayers);
CAdPlugDatabase *CAdPlug::database = 0;

CPlayer *CAdPlug::factory(const std::string &fn, Copl *opl, const CPlayers &pl,
			  const CFileProvider &fp)
{
  CPlayer			*p;
  CPlayers::const_iterator	i;
  unsigned int			j;

  AdPlug_LogWrite("*** CAdPlug::factory(\"%s\",opl,fp) ***\n", fn.c_str());

  // Try a direct hit by file extension
  for(i = pl.begin(); i != pl.end(); i++)
    for(j = 0; (*i)->get_extension(j); j++)
      if(fp.extension(fn, (*i)->get_extension(j))) {
	AdPlug_LogWrite("Trying direct hit: %s\n", (*i)->filetype.c_str());
	if((p = (*i)->factory(opl)))
	  if(p->load(fn, fp)) {
	    AdPlug_LogWrite("got it!\n");
	    AdPlug_LogWrite("--- CAdPlug::factory ---\n");
	    return p;
	  } else
	    delete p;
      }

  // Try all players, one by one
  for(i = pl.begin(); i != pl.end(); i++) {
    AdPlug_LogWrite("Trying: %s\n", (*i)->filetype.c_str());
    if((p = (*i)->factory(opl)))
      if(p->load(fn, fp)) {
        AdPlug_LogWrite("got it!\n");
        AdPlug_LogWrite("--- CAdPlug::factory ---\n");
	return p;
      } else
	delete p;
  }

  // Unknown file
  AdPlug_LogWrite("End of list!\n");
  AdPlug_LogWrite("--- CAdPlug::factory ---\n");
  return 0;
}

void CAdPlug::set_database(CAdPlugDatabase *db)
{
  database = db;
}

std::string CAdPlug::get_version()
{
  return std::string(VERSION);
}

void CAdPlug::debug_output(const std::string &filename)
{
  AdPlug_LogFile(filename.c_str());
  AdPlug_LogWrite("CAdPlug::debug_output(\"%s\"): Redirected.\n",filename.c_str());
}
