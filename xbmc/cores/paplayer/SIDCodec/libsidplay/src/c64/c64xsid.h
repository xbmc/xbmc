/***************************************************************************
                          c64sid.h  -  ReSid Wrapper
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

/* This file could be a specialisation of a sid implementation.
 * However since the sid emulation is not part of this project
 * we are actually creating a wrapper instead.
 */

#include "c64env.h"
#include "sidbuilder.h"
#include "../xsid/xsid.h"

class c64xsid: public XSID
{
private:
    c64env        &m_env;
    sidemu        *m_sid;
    int_least32_t  m_gain;

private:
    uint8_t readMemByte  (uint_least16_t addr)
    {
        uint8_t data = m_env.readMemRamByte (addr);
        m_env.sid2crc (data);
        return data;
    }
    void    writeMemByte (uint8_t data)
    {   m_sid->write (0x18, data);}

public:
    c64xsid (c64env *env, sidemu *sid)
    :XSID(&env->context ()),
     m_env(*env), m_sid(sid), m_gain(100)
    {;}
    
    // Standard component interface
    const char *error (void) {return "";}
    void reset () { sidemu::reset (); }
    void reset (uint8_t volume)
    {
        XSID::reset  (volume);
        m_sid->reset (volume);
    }

    uint8_t read (uint_least8_t addr)
    {   return m_sid->read (addr); }

    void write (uint_least8_t addr, uint8_t data)
    {
        if (addr == 0x18)
            XSID::storeSidData0x18 (data);
        else
            m_sid->write (addr, data);
    }

    void write16 (uint_least16_t addr, uint8_t data)
    {
        XSID::write (addr, data);
    }

    // Standard SID interface
    int_least32_t output (uint_least8_t bits)
    {   return m_sid->output (bits) + (XSID::output (bits) * m_gain / 100); }

    void voice (uint_least8_t num, uint_least8_t vol,
                bool mute)
    {
        if (num == 3)
            XSID::mute (mute);
        else
            m_sid->voice (num, vol, mute);
    }

    void gain (int_least8_t percent)
    {
        // 0 to 99 is loss, 101 - 200 is gain
        m_gain  = percent;
        m_gain += 100;
        if (m_gain > 200)
            m_gain = 200;
    }

    // Xsid specific
    void emulation (sidemu *sid) {m_sid = sid;}
    sidemu *emulation (void) { return m_sid; }
};
