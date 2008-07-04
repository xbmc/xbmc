/***************************************************************************
                          sid6510c.h  -  Special MOS6510 to be fully
                                         compatible with sidplay
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
 *  $Log: sid6510c.h,v $
 *  Revision 1.19  2003/01/23 19:52:42  s_a_white
 *  Redundent code removal.
 *
 *  Revision 1.18  2003/01/20 18:37:08  s_a_white
 *  Stealing update.  Apparently the cpu does a memory read from any non
 *  write cycle (whether it needs to or not) resulting in those cycles
 *  being stolen.
 *
 *  Revision 1.17  2003/01/17 08:44:22  s_a_white
 *  Better handling the operation of IRQs during stolen cycles.
 *
 *  Revision 1.16  2002/11/25 21:07:32  s_a_white
 *  Allow setting of program counter on reset.
 *
 *  Revision 1.15  2002/11/21 19:52:48  s_a_white
 *  CPU upgraded to be like other components.  Theres nolonger a clock call,
 *  instead events are registered to occur at a specific time.
 *
 *  Revision 1.14  2002/11/19 22:56:25  s_a_white
 *  Sidplay1 modes modified to make them nolonger require the psid driver.
 *
 *  Revision 1.13  2002/11/01 17:35:27  s_a_white
 *  Frame based support for old sidplay1 modes.
 *
 *  Revision 1.12  2002/03/12 18:47:13  s_a_white
 *  Made IRQ in sidplay1 compatibility modes behaves like JSR.  This fixes tunes
 *  that have kernel switched out.
 *
 *  Revision 1.11  2002/02/07 18:02:10  s_a_white
 *  Real C64 compatibility fixes. Debug of BRK works again. Fixed illegal
 *  instructions to work like sidplay1.
 *
 *  Revision 1.10  2002/02/04 23:53:23  s_a_white
 *  Improved compatibilty of older sidplay1 modes. Fixed BRK to work like sidplay1
 *  only when stack is 0xff in real mode for better compatibility with C64.
 *
 *  Revision 1.9  2001/09/01 11:08:06  s_a_white
 *  Fixes for sidplay1 environment modes.
 *
 *  Revision 1.8  2001/07/14 13:18:15  s_a_white
 *  Stack & PC invalid tests now only performed on a BRK.
 *
 *  Revision 1.7  2001/03/24 18:09:17  s_a_white
 *  On entry to interrupt routine the first instruction in the handler is now always
 *  executed before pending interrupts are re-checked.
 *
 *  Revision 1.6  2001/03/22 22:40:07  s_a_white
 *  Replaced tabs characters.
 *
 *  Revision 1.5  2001/03/21 22:26:13  s_a_white
 *  Fake interrupts now been moved into here from player.cpp.  At anytime it's
 *  now possible to ditch this compatibility class and use the real thing.
 *
 *  Revision 1.4  2001/03/09 22:28:03  s_a_white
 *  Speed optimisation update.
 *
 *  Revision 1.3  2001/02/13 21:02:25  s_a_white
 *  Small tidy up and possibly a small performace increase.
 *
 *  Revision 1.2  2000/12/11 19:04:32  s_a_white
 *  AC99 Update.
 *
 ***************************************************************************/

#ifndef _sid6510c_h_
#define _sid6510c_h_

#include "mos6510c.h"
#include "sid2types.h"

class SID6510: public MOS6510
{
private:
    // Sidplay Specials
    bool          m_sleeping;
    sid2_env_t    m_mode;
    event_clock_t m_delayClk;
    bool          m_framelock;

public:
    SID6510 (EventContext *context);

    // Standard Functions
    void reset (void);
    void reset (uint_least16_t pc, uint8_t a, uint8_t x, uint8_t y);

    void environment (sid2_env_t mode) { m_mode = mode; }
    void triggerRST (void);
    void triggerNMI (void);
    void triggerIRQ (void);
    void sleep      (void);

protected:
    void FetchOpcode (void);

private:
    struct ProcessorCycle delayCycle;

    inline void sid_illegal (void);
    inline void sid_delay   (void);
    inline void sid_brk  (void);
    inline void sid_jmp  (void);
    inline void sid_rts  (void);
    inline void sid_cli  (void);
    inline void sid_rti  (void);
    inline void sid_irq  (void);
};

#endif // _sid6510c_h_
