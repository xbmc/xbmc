/***************************************************************************
                          c64cia.h  -  C64 CIAs
                             -------------------
    begin                : Fri Apr 4 2001
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

#ifndef _c64cia_h_
#define _c64cia_h_

// The CIA emulations are very generic and here we need to effectively
// wire them into the computer (like adding a chip to a PCB).

#include "c64env.h"
#include "../mos6526/mos6526.h"

/* CIA 1 specifics:
   Generates IRQs
*/
class c64cia1: public MOS6526
{
private:
    c64env &m_env;
    uint8_t lp;

protected:
    void interrupt (bool state)
    {
        m_env.interruptIRQ (state);
    }

    void portB ()
    {
        uint8_t lp = (prb | ~ddrb) & 0x10;
        if (lp != this->lp)
            m_env.lightpen();
        this->lp = lp;
    }

public:
    c64cia1 (c64env *env)
    :MOS6526(&(env->context ())),
     m_env(*env) {}
    const char *error (void) {return "";}

    void reset ()
    {
        lp = 0x10;
        MOS6526::reset ();
    }
};

/* CIA 2 specifics:
   Generates NMIs
*/
class c64cia2: public MOS6526
{
private:
    c64env &m_env;

protected:
    void interrupt (bool state)
    {
        if (state)
            m_env.interruptNMI ();
    }

public:
    c64cia2 (c64env *env)
    :MOS6526(&(env->context ())),
     m_env(*env) {}
    const char *error (void) {return "";}
};

#endif // _c64cia_h_
