/***************************************************************************
                          c64vic.h  -  C64 VIC
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

#ifndef _c64vic_h_
#define _c64vic_h_

// The VIC emulation is very generic and here we need to effectively
// wire it into the computer (like adding a chip to a PCB).
#include "c64env.h"
#include "../mos656x/mos656x.h"

class c64vic: public MOS656X
{
private:
    c64env &m_env;

protected:
    void interrupt (bool state)
    {
        m_env.interruptIRQ (state);
    }

    void addrctrl (bool state)
    {
        m_env.signalAEC (state);
    }

public:
    c64vic (c64env *env)
    :MOS656X(&(env->context ())),
     m_env(*env) {}
    const char *error (void) {return "";}
};

#endif // _c64vic_h_
