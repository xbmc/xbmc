/***************************************************************************
                          mos6510.h  -  description
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
 *  $Log: mos6510.h,v $
 *  Revision 1.6  2004/05/24 23:11:10  s_a_white
 *  Fixed email addresses displayed to end user.
 *
 *  Revision 1.5  2001/07/14 16:47:21  s_a_white
 *  Sync with sidbuilder class project.
 *
 *  Revision 1.4  2001/07/14 13:04:34  s_a_white
 *  Accumulator is now unsigned, which improves code readability.
 *
 *  Revision 1.3  2000/12/11 19:03:16  s_a_white
 *  AC99 Update.
 *
 ***************************************************************************/

#ifndef _mos6510_h_
#define _mos6510_h_

#include "config.h"
#include "component.h"
#include "sidenv.h"
#include "event.h"

#undef  MOS6510_VERSION
#undef  MOS6510_DATE
#undef  MOS6510_AUTHOR
#undef  MOS6510_EMAIL
#define MOS6510_VERSION "1.08"
#define MOS6510_DATE    "23th May 2000"
#define MOS6510_AUTHOR  "Simon White"
#define MOS6510_EMAIL   S_A_WHITE_EMAIL
#define MOS6510_INTERRUPT_DELAY 2

#include "opcodes.h"
#include "conf6510.h"

// Status Register flag definistions
#define SR_NEGATIVE  7
#define SR_OVERFLOW  6
#define SR_NOTUSED   5
#define SR_BREAK     4
#define SR_DECIMAL   3
#define SR_INTERRUPT 2
#define SR_ZERO      1
#define SR_CARRY     0

#define SP_PAGE      0x01

// Check to see what type of emulation is required
#ifdef MOS6510_CYCLE_BASED
#   ifdef MOS6510_SIDPLAY
#       include "cycle_based/sid6510c.h"
#   else
#       include "cycle_based/mos6510c.h"
#   endif // MOS6510_SIDPLAY
#else
    // Line based emulation code has not been provided
#endif // MOS6510_CYCLE_BASED

#endif // _mos6510_h_

