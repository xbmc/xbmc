/***************************************************************************
                          sidplayer.cpp  -  Wrapper to hide private
                                            header files (see below)
                             -------------------
    begin                : Fri Jun 9 2000
    copyright            : (C) 2000 by Simon White
    email                : s_a_white@email.com
 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/***************************************************************************
 *  $Log: sidplay2.cpp,v $
 *  Revision 1.19  2003/12/15 22:40:13  s_a_white
 *  Fixed compile error.
 *
 *  Revision 1.18  2003/12/15 22:28:20  s_a_white
 *  Added timebase function.
 *
 *  Revision 1.17  2003/10/16 07:41:05  s_a_white
 *  Allow redirection of debug information to file.
 *
 *  Revision 1.16  2003/02/20 19:12:54  s_a_white
 *  Fix included header files.
 *
 *  Revision 1.15  2002/03/04 19:05:49  s_a_white
 *  Fix C++ use of nothrow.
 *
 *  Revision 1.14  2001/12/13 08:28:08  s_a_white
 *  Added namespace support to fix problems with xsidplay.
 *
 *  Revision 1.13  2001/12/11 19:24:15  s_a_white
 *  More GCC3 Fixes.
 *
 *  Revision 1.12  2001/09/17 19:03:58  s_a_white
 *  2.1.0 interface stabalisation.
 *
 *  Revision 1.11  2001/09/01 11:16:12  s_a_white
 *  Renamed configure to config.
 *
 *  Revision 1.10  2001/07/25 17:01:13  s_a_white
 *  Support for new configuration interface.
 *
 *  Revision 1.9  2001/07/14 16:46:16  s_a_white
 *  Sync with sidbuilder class project.
 *
 *  Revision 1.8  2001/07/14 12:57:38  s_a_white
 *  Added credits and debug functions.  Removed external filter.
 *
 *  Revision 1.7  2001/03/21 22:31:22  s_a_white
 *  Filter redefinition support.
 *
 *  Revision 1.6  2001/03/01 23:46:37  s_a_white
 *  Support for sample mode to be selected at runtime.
 *
 *  Revision 1.5  2001/02/21 21:49:21  s_a_white
 *  Now uses new player::getErrorString function.
 *
 *  Revision 1.4  2001/02/13 21:32:35  s_a_white
 *  Windows DLL export fix.
 *
 *  Revision 1.3  2001/02/07 20:57:08  s_a_white
 *  New SID_EXPORT define.  Supports SidTune now.
 *
 *  Revision 1.2  2001/01/23 21:26:28  s_a_white
 *  Only way to load a tune now is by passing in a sidtune object.  This is
 *  required for songlength database support.
 *
 *  Revision 1.1  2000/12/12 19:14:44  s_a_white
 *  Library wrapper.
 *
 ***************************************************************************/

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
// Redirection to private version of sidplayer (This method is called Cheshire Cat)
// [ms: which is J. Carolan's name for a degenerate 'bridge']
// This interface can be directly replaced with a libsidplay1 or C interface wrapper.
//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------

#include <stdio.h>
#include "config.h"
#include "player.h"
#include "sidplay2.h"

#ifdef HAVE_EXCEPTIONS
#   include <new>
#endif

sidplay2::sidplay2 ()
#ifdef HAVE_EXCEPTIONS
: sidplayer (*(new(std::nothrow) SIDPLAY2_NAMESPACE::Player))
#else
: sidplayer (*(new SIDPLAY2_NAMESPACE::Player))
#endif
{
}

sidplay2::~sidplay2 ()
{   if (&sidplayer) delete &sidplayer; }

int sidplay2::config (const sid2_config_t &cfg)
{   return sidplayer.config (cfg); }

const sid2_config_t &sidplay2::config (void) const
{   return sidplayer.config (); }

void sidplay2::stop (void)
{   sidplayer.stop (); }

void sidplay2::pause (void)
{   sidplayer.pause (); }

uint_least32_t sidplay2::play (void *buffer, uint_least32_t length)
{   return sidplayer.play (buffer, length); }

int sidplay2::load (SidTune *tune)
{   return sidplayer.load (tune); }

const sid2_info_t &sidplay2::info () const
{   return sidplayer.info (); }

uint_least32_t sidplay2::time (void) const
{   return sidplayer.time (); }

uint_least32_t sidplay2::mileage (void) const
{   return sidplayer.mileage (); }

const char *sidplay2::error (void) const
{   return sidplayer.error (); }

int  sidplay2::fastForward  (uint percent)
{   return sidplayer.fastForward (percent); }

void sidplay2::debug (bool enable, FILE *out)
{   sidplayer.debug (enable, out); }

sid2_player_t sidplay2::state (void) const
{   return sidplayer.state (); }

uint_least32_t sidplay2::timebase (void) const
{   return SID2_TIME_BASE; }
