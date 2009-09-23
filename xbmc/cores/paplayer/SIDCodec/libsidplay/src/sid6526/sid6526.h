/***************************************************************************
                          sid6526.h  -  fake CIA timer for sidplay1
                                        environment modes
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
 *  $Log: sid6526.h,v $
 *  Revision 1.5  2003/10/28 00:22:53  s_a_white
 *  getTime now returns a time with respect to the clocks desired phase.
 *
 *  Revision 1.4  2003/02/20 18:55:14  s_a_white
 *  sid2crc support.
 *
 *  Revision 1.3  2002/07/20 08:34:52  s_a_white
 *  Remove unnecessary and pointless conts.
 *
 *  Revision 1.2  2002/03/11 18:00:29  s_a_white
 *  Better mirror sidplay1s handling of random numbers.
 *
 *  Revision 1.1  2001/09/01 11:11:19  s_a_white
 *  This is the old fake6526 code required for sidplay1 environment modes.
 *
 ***************************************************************************/

#ifndef _sid6526_h_
#define _sid6526_h_

#include "component.h"
#include "event.h"
#include "c64env.h"

class SID6526: public component
{
private:

    static const char * const credit;

    c64env       &m_env;
    EventContext &m_eventContext;
    event_clock_t m_accessClk;
    event_phase_t m_phase;

    uint8_t regs[0x10];
    uint8_t cra;             // Timer A Control Register
    uint_least16_t ta_latch;
    uint_least16_t ta;       // Current count (reduces to zero)
    uint_least32_t rnd;
    uint_least16_t m_count;
    bool locked; // Prevent code changing CIA.

    class TaEvent: public Event
    {
    private:
        SID6526 &m_cia;
        void event (void) {m_cia.event ();}

    public:
        TaEvent (SID6526 &cia)
            :Event("CIA Timer A"),
             m_cia(cia) {}
    } m_taEvent;

public:
    SID6526 (c64env *env);

    //Common:
    void    reset (void) { reset (false); }
    void    reset (bool seed);
    uint8_t read  (uint_least8_t addr);
    void    write (uint_least8_t addr, uint8_t data);
    const   char *credits (void) {return credit;}
    const   char *error   (void) {return "";}

    // Specific:
    void event (void);
    void clock (uint_least16_t count) { m_count = count; }
    void lock  () { locked = true; }
};

#endif // _sid6526_h_
