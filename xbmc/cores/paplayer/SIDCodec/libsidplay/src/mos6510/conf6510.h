/***************************************************************************
                          config.h  -  description
                             -------------------
    begin                : Thu May 11 2000
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
 *  $Log: conf6510.h,v $
 *  Revision 1.3  2001/03/19 23:41:51  s_a_white
 *  Better support for global debug.
 *
 *  Revision 1.2  2000/12/11 19:03:16  s_a_white
 *  AC99 Update.
 *
 ***************************************************************************/

#ifndef _conf6510_h_
#define _conf6510_h_

#include "sidconfig.h"

#define MOS6510_CYCLE_BASED
#define MOS6510_ACCURATE_CYCLES
#define MOS6510_SIDPLAY
//#define MOS6510_STATE_6510
//#define MOS6510_DEBUG 1

// Support global debug option
#ifdef DEBUG
#   ifndef MOS6510_DEBUG
#   define MOS6510_DEBUG DEBUG
#   endif
#endif

#endif // _conf6510_h_
