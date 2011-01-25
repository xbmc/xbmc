/***************************************************************************
                          mos6526.cpp  -  CIA Timer
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
 *  $Log: mos6526.cpp,v $
 *  Revision 1.23  2004/05/04 22:40:20  s_a_white
 *  Fix pulse mode output on the ports to work.
 *
 *  Revision 1.22  2004/04/23 00:58:22  s_a_white
 *  Reload counter on starting timer, not stopping.
 *
 *  Revision 1.21  2004/04/13 07:39:31  s_a_white
 *  Add lightpen support.
 *
 *  Revision 1.20  2004/03/20 16:17:29  s_a_white
 *  Clear all registers at reset.  Fix port B read bug.
 *
 *  Revision 1.19  2004/03/14 23:07:50  s_a_white
 *  Remove warning in Visual C about precendence order.
 *
 *  Revision 1.18  2004/03/09 20:52:30  s_a_white
 *  Added missing header.
 *
 *  Revision 1.17  2004/03/09 20:44:34  s_a_white
 *  Full serial and I/O port implementation.  No keyboard/joystick support as we
 *  are not that kind of emulator.
 *
 *  Revision 1.16  2004/02/29 14:29:44  s_a_white
 *  Serial port emulation.
 *
 *  Revision 1.15  2004/01/08 08:59:20  s_a_white
 *  Support the TOD frequency divider.
 *
 *  Revision 1.14  2004/01/06 21:28:27  s_a_white
 *  Initial TOD support (code taken from vice)
 *
 *  Revision 1.13  2003/10/28 00:22:53  s_a_white
 *  getTime now returns a time with respect to the clocks desired phase.
 *
 *  Revision 1.12  2003/02/24 19:44:30  s_a_white
 *  Make sure events are canceled on reset.
 *
 *  Revision 1.11  2003/01/17 08:39:04  s_a_white
 *  Event scheduler phase support.  Better default handling of keyboard lines.
 *
 *  Revision 1.10  2002/12/16 22:12:24  s_a_white
 *  Simulate serial input from data port A to prevent kernel lockups.
 *
 *  Revision 1.9  2002/11/20 22:50:27  s_a_white
 *  Reload count when timers are stopped
 *
 *  Revision 1.8  2002/10/02 19:49:21  s_a_white
 *  Revert previous change as was incorrect.
 *
 *  Revision 1.7  2002/09/11 22:30:47  s_a_white
 *  Counter interval writes now go to a new register call prescaler.  This is
 *  copied to the timer latch/counter as appropriate.
 *
 *  Revision 1.6  2002/09/09 22:49:06  s_a_white
 *  Proper idr clear if interrupt was only internally pending.
 *
 *  Revision 1.5  2002/07/20 08:34:52  s_a_white
 *  Remove unnecessary and pointless conts.
 *
 *  Revision 1.4  2002/03/03 22:04:08  s_a_white
 *  Tidy.
 *
 *  Revision 1.3  2001/07/14 13:03:33  s_a_white
 *  Now uses new component classes and event generation.
 *
 *  Revision 1.2  2001/03/23 23:21:38  s_a_white
 *  Removed redundant reset funtion.  Timer b now gets initialised properly.
 *  Switch case now allows write/read from timer b.
 *
 *  Revision 1.1  2001/03/21 22:41:45  s_a_white
 *  Non faked CIA emulation with NMI support.  Removal of Hacked VIC support
 *  off CIA timer.
 *
 *  Revision 1.8  2001/03/09 23:44:30  s_a_white
 *  Integrated more 6526 features.  All timer modes and interrupts correctly
 *  supported.
 *
 *  Revision 1.7  2001/02/21 22:07:10  s_a_white
 *  Prevent re-triggering of interrupt if it's already active.
 *
 *  Revision 1.6  2001/02/13 21:00:01  s_a_white
 *  Support for real interrupts.
 *
 *  Revision 1.4  2000/12/11 18:52:12  s_a_white
 *  Conversion to AC99
 *
 ***************************************************************************/

#include <string.h>
#include "config.h"
#include "sidendian.h"
#include "mos6526.h"

enum
{
    INTERRUPT_TA      = 1 << 0,
    INTERRUPT_TB      = 1 << 1,
    INTERRUPT_ALARM   = 1 << 2,
    INTERRUPT_SP      = 1 << 3,
    INTERRUPT_FLAG    = 1 << 4,
    INTERRUPT_REQUEST = 1 << 7
};

enum
{
    PRA     = 0,
    PRB     = 1,
    DDRA    = 2,
    DDRB    = 3,
    TAL     = 4,
    TAH     = 5,
    TBL     = 6,
    TBH     = 7,
    TOD_TEN = 8,
    TOD_SEC = 9,
    TOD_MIN = 10,
    TOD_HR  = 11,
    SDR     = 12,
    ICR     = 13,
    IDR     = 13,
    CRA     = 14,
    CRB     = 15
};

const char *MOS6526::credit =
{   // Optional information
    "*MOS6526 (CIA) Emulation:\0"
    "\tCopyright (C) 2001-2004 Simon White <" S_A_WHITE_EMAIL ">\0"
};


MOS6526::MOS6526 (EventContext *context)
:pra(regs[PRA]),
 prb(regs[PRB]),
 ddra(regs[DDRA]),
 ddrb(regs[DDRB]),
 idr(0),
 event_context(*context),
 m_phase(EVENT_CLOCK_PHI1),
 m_todPeriod(~0), // Dummy
 event_ta(this),
 event_tb(this),
 event_tod(this)
{
    reset ();
}

void MOS6526::clock (float64_t clock)
{    // Fixed point 25.7
    m_todPeriod = (event_clock_t) (clock * (float64_t) (1 << 7));
}

void MOS6526::reset (void)
{
    ta  = ta_latch = 0xffff;
    tb  = tb_latch = 0xffff;
    ta_underflow = tb_underflow = false;
    cra = crb = sdr_out = 0;
    sdr_count = 0;
    sdr_buffered = false;
    // Clear off any IRQs
    trigger (0);
    cnt_high  = true;
    icr = idr = 0;
    m_accessClk = 0;
    dpa = 0xf0;
    memset (regs, 0, sizeof (regs));

    // Reset tod
    memset(m_todclock, 0, sizeof(m_todclock));
    memset(m_todalarm, 0, sizeof(m_todalarm));
    memset(m_todlatch, 0, sizeof(m_todlatch));
    m_todlatched = false;
    m_todstopped = true;
    m_todclock[TOD_HR-TOD_TEN] = 1; // the most common value
    m_todCycles = 0;

    // Remove outstanding events
    event_context.cancel   (&event_ta);
    event_context.cancel   (&event_tb);
    event_context.schedule (&event_tod, 0, m_phase);
}

uint8_t MOS6526::read (uint_least8_t addr)
{
    event_clock_t cycles;
    if (addr > 0x0f) return 0;
    bool ta_pulse = false, tb_pulse = false;

    cycles       = event_context.getTime (m_accessClk, event_context.phase ());
    m_accessClk += cycles;

    // Sync up timers
    if ((cra & 0x21) == 0x01)
    {
        ta -= cycles;
        if (!ta)
        {
            ta_event ();
            ta_pulse = true;
        }
    }
    if ((crb & 0x61) == 0x01)
    {
        tb -= cycles;
        if (!tb)
        {
            tb_event ();
            tb_pulse = true;
        }
    }

    switch (addr)
    {
    case PRA: // Simulate a serial port
        return (pra | ~ddra);
    case PRB:
    {
        uint8_t data = prb | ~ddrb;
        // Timers can appear on the port
        if (cra & 0x02)
        {
            data &= 0xbf;
            if (cra & 0x04 ? ta_underflow : ta_pulse)
                data |= 0x40;
        }
        if (crb & 0x02)
        {
            data &= 0x7f;
            if (crb & 0x04 ? tb_underflow : tb_pulse)
                data |= 0x80;
        }
        return data;
    }
    case TAL: return endian_16lo8 (ta);
    case TAH: return endian_16hi8 (ta);
    case TBL: return endian_16lo8 (tb);
    case TBH: return endian_16hi8 (tb);

    // TOD implementation taken from Vice
    // TOD clock is latched by reading Hours, and released
    // upon reading Tenths of Seconds. The counter itself
    // keeps ticking all the time.
    // Also note that this latching is different from the input one.
    case TOD_TEN: // Time Of Day clock 1/10 s
    case TOD_SEC: // Time Of Day clock sec
    case TOD_MIN: // Time Of Day clock min
    case TOD_HR:  // Time Of Day clock hour
        if (!m_todlatched)
            memcpy(m_todlatch, m_todclock, sizeof(m_todlatch));
        if (addr == TOD_TEN)
            m_todlatched = false;
        if (addr == TOD_HR)
            m_todlatched = true;
        return m_todlatch[addr - TOD_TEN];

    case IDR:
    {   // Clear IRQs, and return interrupt
        // data register
        uint8_t ret = idr;
        trigger (0);
        return ret;
    }

    case CRA: return cra;
    case CRB: return crb;
    default:  return regs[addr];
    }
}

void MOS6526::write (uint_least8_t addr, uint8_t data)
{
    event_clock_t cycles;
    if (addr > 0x0f) return;

    regs[addr] = data;
    cycles     = event_context.getTime (m_accessClk, event_context.phase ());

    if (cycles)
    {
        m_accessClk += cycles;
        // Sync up timers
        if ((cra & 0x21) == 0x01)
        {
            ta -= cycles;
            if (!ta)
                ta_event ();
        }
        if ((crb & 0x61) == 0x01)
        {
            tb -= cycles;
            if (!tb)
                tb_event ();
        }
    }

    switch (addr)
    {
    case PRA: case DDRA:
        portA ();
        break;
    case PRB: case DDRB:
        portB ();
        break;
    case TAL: endian_16lo8 (ta_latch, data); break;
    case TAH:
        endian_16hi8 (ta_latch, data);
        if (!(cra & 0x01)) // Reload timer if stopped
            ta = ta_latch;
    break;

    case TBL: endian_16lo8 (tb_latch, data); break;
    case TBH:
        endian_16hi8 (tb_latch, data);
        if (!(crb & 0x01)) // Reload timer if stopped
            tb = tb_latch;
    break;

    // TOD implementation taken from Vice
    case TOD_HR:  // Time Of Day clock hour
        // Flip AM/PM on hour 12
        //   (Andreas Boose <viceteam@t-online.de> 1997/10/11).
        // Flip AM/PM only when writing time, not when writing alarm
        // (Alexander Bluhm <mam96ehy@studserv.uni-leipzig.de> 2000/09/17).
        data &= 0x9f;
        if ((data & 0x1f) == 0x12 && !(crb & 0x80))
            data ^= 0x80;
        // deliberate run on
    case TOD_TEN: // Time Of Day clock 1/10 s
    case TOD_SEC: // Time Of Day clock sec
    case TOD_MIN: // Time Of Day clock min
        if (crb & 0x80)
            m_todalarm[addr - TOD_TEN] = data;
        else
        {
            if (addr == TOD_TEN)
                m_todstopped = false;
            if (addr == TOD_HR)
                m_todstopped = true;
            m_todclock[addr - TOD_TEN] = data;
        }
        // check alarm
        if (!m_todstopped && !memcmp(m_todalarm, m_todclock, sizeof(m_todalarm)))
            trigger (INTERRUPT_ALARM);
        break;

    case SDR:
        if (cra & 0x40)
            sdr_buffered = true;
        break;

    case ICR:
        if (data & 0x80)
            icr |= data & 0x1f;
        else
            icr &= ~data;
        trigger (idr);
    break;

    case CRA:
        // Reset the underflow flipflop for the data port
        if ((data & 1) && !(cra & 1))
        {
            ta = ta_latch;
            ta_underflow = true;
        }
        cra = data;

        // Check for forced load
        if (data & 0x10)
        {
            cra &= (~0x10);
            ta   = ta_latch;
        }

        if ((data & 0x21) == 0x01)
        {   // Active
            event_context.schedule (&event_ta, (event_clock_t) ta + 1,
                                    m_phase);
        } else
        {   // Inactive
            event_context.cancel (&event_ta);
        }
    break;

    case CRB:
        // Reset the underflow flipflop for the data port
        if ((data & 1) && !(crb & 1))
        {
            tb = tb_latch;
            tb_underflow = true;
        }
        // Check for forced load
        crb = data;
        if (data & 0x10)
        {
            crb &= (~0x10);
            tb   = tb_latch;
        }

        if ((data & 0x61) == 0x01)
        {   // Active
            event_context.schedule (&event_tb, (event_clock_t) tb + 1,
                                    m_phase);
        } else
        {   // Inactive
            event_context.cancel (&event_tb);
        }
    break;

    default:
    break;
    }
}

void MOS6526::trigger (int irq)
{
    if (!irq)
    {   // Clear any requested IRQs
        if (idr & INTERRUPT_REQUEST)
            interrupt (false);
        idr = 0;
        return;
    }

    idr |= irq;
    if (icr & idr)
    {
        if (!(idr & INTERRUPT_REQUEST))
        {
            idr |= INTERRUPT_REQUEST;
            interrupt (true);
        }
    }
}

void MOS6526::ta_event (void)
{   // Timer Modes
    event_clock_t cycles;
    uint8_t mode = cra & 0x21;

    if (mode == 0x21)
    {
        if (ta--)
            return;
    }

    cycles       = event_context.getTime (m_accessClk, m_phase);
    m_accessClk += cycles;

    ta = ta_latch;
    ta_underflow ^= true; // toggle flipflop
    if (cra & 0x08)
    {   // one shot, stop timer A
        cra &= (~0x01);
    } else if (mode == 0x01)
    {   // Reset event
        event_context.schedule (&event_ta, (event_clock_t) ta + 1,
                                m_phase);
    }
    trigger (INTERRUPT_TA);
    
    // Handle serial port
    if (cra & 0x40)
    {
        if (sdr_count)
        {
            if (!--sdr_count)
                trigger (INTERRUPT_SP);
        }
        if (!sdr_count && sdr_buffered)
        {
            sdr_out = regs[SDR];
            sdr_buffered = false;
            sdr_count = 16; // Output rate 8 bits at ta / 2
        }
    }

    switch (crb & 0x61)
    {
    case 0x01: tb -= cycles; break;
    case 0x41:
    case 0x61:
        tb_event ();
    break;
    }
}
    
void MOS6526::tb_event (void)
{   // Timer Modes
    uint8_t mode = crb & 0x61;
    switch (mode)
    {
    case 0x01:
        break;

    case 0x21:
    case 0x41:
        if (tb--)
            return;
    break;

    case 0x61:
        if (cnt_high)
        {
            if (tb--)
                return;
        }
    break;
    
    default:
        return;
    }

    m_accessClk = event_context.getTime (m_phase);
    tb = tb_latch;
    tb_underflow ^= true; // toggle flipflop
    if (crb & 0x08)
    {   // one shot, stop timer A
        crb &= (~0x01);
    } else if (mode == 0x01)
    {   // Reset event
        event_context.schedule (&event_tb, (event_clock_t) tb + 1,
                                m_phase);
    }
    trigger (INTERRUPT_TB);
}

// TOD implementation taken from Vice
#define byte2bcd(byte) (((((byte) / 10) << 4) + ((byte) % 10)) & 0xff)
#define bcd2byte(bcd)  (((10*(((bcd) & 0xf0) >> 4)) + ((bcd) & 0xf)) & 0xff)

void MOS6526::tod_event(void)
{   // Reload divider according to 50/60 Hz flag
    // Only performed on expiry according to Frodo
    if (cra & 0x80)
        m_todCycles += (m_todPeriod * 5);
    else
        m_todCycles += (m_todPeriod * 6);    
    
    // Fixed precision 25.7
    event_context.schedule (&event_tod, m_todCycles >> 7, m_phase);
    m_todCycles &= 0x7F; // Just keep the decimal part

    if (!m_todstopped)
    {
        // inc timer
        uint8_t *tod = m_todclock;
        uint8_t t = bcd2byte(*tod) + 1;
        *tod++ = byte2bcd(t % 10);
        if (t >= 10)
        {
            t = bcd2byte(*tod) + 1;
            *tod++ = byte2bcd(t % 60);
            if (t >= 60)
            {
                t = bcd2byte(*tod) + 1;
                *tod++ = byte2bcd(t % 60);
                if (t >= 60)
                {
                    uint8_t pm = *tod & 0x80;
                    t = *tod & 0x1f;
                    if (t == 0x11)
                        pm ^= 0x80; // toggle am/pm on 0:59->1:00 hr
                    if (t == 0x12)
                        t = 1;
                    else if (++t == 10)
                        t = 0x10;   // increment, adjust bcd
                    t &= 0x1f;
                    *tod = t | pm;
                }
            }
        }
        // check alarm
        if (!memcmp(m_todalarm, m_todclock, sizeof(m_todalarm)))
            trigger (INTERRUPT_ALARM);
    }
}
