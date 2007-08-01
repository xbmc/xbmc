/***************************************************************************
                          sid6526.cpp  -  description
                             -------------------
    begin                : Wed Jun 7 2000
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
 *  $Log: sid6526.cpp,v $
 *  Revision 1.12  2004/05/28 15:45:12  s_a_white
 *  Correct credit email address
 *
 *  Revision 1.11  2003/10/28 00:22:53  s_a_white
 *  getTime now returns a time with respect to the clocks desired phase.
 *
 *  Revision 1.10  2003/02/24 19:45:00  s_a_white
 *  Make sure events are canceled on reset.
 *
 *  Revision 1.9  2003/02/20 18:55:14  s_a_white
 *  sid2crc support.
 *
 *  Revision 1.8  2003/01/17 08:37:12  s_a_white
 *  Event scheduler phase support.
 *
 *  Revision 1.7  2002/10/02 19:48:15  s_a_white
 *  Make CIA control register reflect that the timer cannot be disabled.
 *
 *  Revision 1.6  2002/03/11 18:00:29  s_a_white
 *  Better mirror sidplay1s handling of random numbers.
 *
 *  Revision 1.5  2002/03/03 22:03:49  s_a_white
 *  Tidy.
 *
 *  Revision 1.4  2001/10/02 18:01:36  s_a_white
 *  Support for cleaned c64env.
 *
 *  Revision 1.3  2001/09/18 02:22:37  jpaana
 *  Fixed include filename to lowercase
 *
 *  Revision 1.2  2001/09/03 22:24:09  s_a_white
 *  New counts for timer A are correctly formed.
 *
 *  Revision 1.1  2001/09/01 11:11:19  s_a_white
 *  This is the old fake6526 code required for sidplay1 environment modes.
 *
 ***************************************************************************/

#include <time.h>
#include "config.h"
#include "sidendian.h"
#include "sid6526.h"

const char * const SID6526::credit =
{   // Optional information
    "*SID6526 (SIDPlay1 Fake CIA) Emulation:\0"
    "\tCopyright (C) 2001 Simon White <" S_A_WHITE_EMAIL ">\0"
};

SID6526::SID6526 (c64env *env)
:m_env(*env),
 m_eventContext(m_env.context ()),
 m_phase(EVENT_CLOCK_PHI1),
 rnd(0),
 m_taEvent(*this)
{
    clock (0xffff);
    reset (false);
}

void SID6526::reset (bool seed)
{
    locked = false;
    ta   = ta_latch = m_count;
    cra  = 0;
    // Initialise random number generator
    if (seed)
        rnd = 0;
    else
        rnd += time(NULL) & 0xff;
    m_accessClk = 0;
    // Remove outstanding events
    m_eventContext.cancel (&m_taEvent);
}

uint8_t SID6526::read (uint_least8_t addr)
{
    if (addr > 0x0f)
        return 0;

    switch (addr)
    {
    case 0x04:
    case 0x05:
    case 0x11:
    case 0x12:
        rnd = rnd * 13 + 1;
        return (uint8_t) (rnd >> 3);
    break;
    default:
        return regs[addr];
    }
}

void SID6526::write (uint_least8_t addr, uint8_t data)
{
    if (addr > 0x0f)
        return;

    regs[addr] = data;

    if (locked)
        return; // Stop program changing time interval

    {   // Sync up timer
        event_clock_t cycles;
        cycles       = m_eventContext.getTime (m_accessClk, m_phase);
        m_accessClk += cycles;
        ta          -= cycles;
        if (!ta)
            event ();
    }

    switch (addr)
    {
    case 0x4: endian_16lo8 (ta_latch, data); break;
    case 0x5:
        endian_16hi8 (ta_latch, data);
        if (!(cra & 0x01)) // Reload timer if stopped
            ta = ta_latch;
    break;
    case 0x0e:
        cra = data | 0x01;
        if (data & 0x10)
        {
            cra &= (~0x10);
            ta   = ta_latch;
        }
        m_eventContext.schedule (&m_taEvent, (event_clock_t) ta + 1,
                                 m_phase);
    break;
    default:
    break;
    }
}

void SID6526::event (void)
{   // Timer Modes
    m_accessClk = m_eventContext.getTime (m_phase);
    ta = ta_latch;
    m_eventContext.schedule (&m_taEvent, (event_clock_t) ta + 1,
                             m_phase);
    m_env.interruptIRQ (true);
}
