/***************************************************************************
                          nullsid.h  -  Null SID Emulation
                             -------------------
    begin                : Thurs Sep 20 2001
    copyright            : (C) 2001 by Simon White
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

#ifndef _nullsid_h_
#define _nullsid_h_

#include "sidbuilder.h"

class NullSID: public sidemu
{
public:
    NullSID () : sidemu (NULL) {;}

    // Standard component functions
    void    reset () { sidemu::reset (); }
    void    reset (uint8_t) { ; }
    uint8_t read  (uint_least8_t) { return 0; }
    void    write (uint_least8_t, uint8_t) { ; }
    const   char *credits (void) { return ""; }
    const   char *error   (void) { return ""; }

    // Standard SID functions
    int_least32_t output (uint_least8_t) { return 0; }
    void          voice  (uint_least8_t, uint_least8_t,
                          bool) { ; }
    void          gain   (int_least8_t) { ; }
};

#endif // _nullsid_h_
