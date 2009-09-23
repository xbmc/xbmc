/***************************************************************************
             hardsid.cpp  -  Hardsid support interface.
                             -------------------
    begin                : Fri Dec 15 2000
    copyright            : (C) 2001-2001 by Jarno Paananen
    email                : jpaana@s2.org
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
 *  $Log: hardsid-unix.cpp,v $
 *  Revision 1.14  2004/05/27 21:18:28  jpaana
 *  The filter ioctl was reversed
 *
 *  Revision 1.13  2004/05/05 23:48:01  s_a_white
 *  Detect available sid devices on Unix system.
 *
 *  Revision 1.12  2004/04/29 23:20:01  s_a_white
 *  Optimisation to polling hardsid delay write to only access the hardsid
 *  if really necessary.
 *
 *  Revision 1.11  2003/10/28 00:15:16  s_a_white
 *  Get time with respect to correct clock phase.
 *
 *  Revision 1.10  2003/01/20 16:25:25  s_a_white
 *  Updated for new event scheduler interface.
 *
 *  Revision 1.9  2002/10/17 18:36:43  s_a_white
 *  Prevent multiple unlocks causing a NULL pointer access.
 *
 *  Revision 1.8  2002/08/14 16:03:54  jpaana
 *  Fixed to compile with new HardSID::lock method
 *
 *  Revision 1.7  2002/07/20 08:36:24  s_a_white
 *  Remove unnecessary and pointless conts.
 *
 *  Revision 1.6  2002/02/17 17:24:51  s_a_white
 *  Updated for new reset interface.
 *
 *  Revision 1.5  2002/01/30 01:47:47  jpaana
 *  Read ioctl used wrong parameter type and delay ioctl takes uint, not uint*
 *
 *  Revision 1.4  2002/01/30 00:43:50  s_a_white
 *  Added realtime delays even when there is no accesses to
 *  the sid.  Prevents excessive CPU usage.
 *
 *  Revision 1.3  2002/01/29 21:47:35  s_a_white
 *  Constant fixed interval delay added to prevent emulation going fast when
 *  there are no writes to the sid.
 *
 *  Revision 1.2  2002/01/29 00:32:56  jpaana
 *  Use the new read and delay IOCTLs
 *
 *  Revision 1.1  2002/01/28 22:35:20  s_a_white
 *  Initial Release.
 *
 *
 ***************************************************************************/

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include "config.h"
#include "hardsid-emu.h"

// Move these to common header file
#define HSID_IOCTL_RESET     _IOW('S', 0, int)
#define HSID_IOCTL_FIFOSIZE  _IOR('S', 1, int)
#define HSID_IOCTL_FIFOFREE  _IOR('S', 2, int)
#define HSID_IOCTL_SIDTYPE   _IOR('S', 3, int)
#define HSID_IOCTL_CARDTYPE  _IOR('S', 4, int)
#define HSID_IOCTL_MUTE      _IOW('S', 5, int)
#define HSID_IOCTL_NOFILTER  _IOW('S', 6, int)
#define HSID_IOCTL_FLUSH     _IO ('S', 7)
#define HSID_IOCTL_DELAY     _IOW('S', 8, int)
#define HSID_IOCTL_READ      _IOWR('S', 9, int*)

bool       HardSID::m_sidFree[16] = {0};
const uint HardSID::voices = HARDSID_VOICES;
uint       HardSID::sid = 0;
char       HardSID::credit[];

HardSID::HardSID (sidbuilder *builder)
:sidemu(builder),
 Event("HardSID Delay"),
 m_handle(0),
 m_eventContext(NULL),
 m_phase(EVENT_CLOCK_PHI1),
 m_instance(sid++),
 m_status(false),
 m_locked(false)
{
    uint num = 16;
    for ( uint i = 0; i < 16; i++ )
    {
        if(m_sidFree[i] == 0)
        {
            m_sidFree[i] = 1;
            num = i;
            break;
        }
    }

    // All sids in use?!?
    if ( num == 16 )
        return;

    m_instance = num;

    {
        char device[20];
        *m_errorBuffer = '\0';
        sprintf (device, "/dev/sid%u", m_instance);
        m_handle = open (device, O_RDWR);
        if (m_handle < 0)
        {
            if (m_instance == 0)
            {
                m_handle = open ("/dev/sid", O_RDWR);
                if (m_handle < 0)
                {
                    sprintf (m_errorBuffer, "HARDSID ERROR: Cannot access \"/dev/sid\" or \"%s\"", device);
                    return;
                }
            }
            else
            {
                sprintf (m_errorBuffer, "HARDSID ERROR: Cannot access \"%s\"", device);
                return;
            }
        }
    }

    m_status = true;
    reset ();
}

HardSID::~HardSID()
{
    sid--;
    m_sidFree[m_instance] = 0;
    if (m_handle)
        close (m_handle);
}

void HardSID::reset (uint8_t volume)
{
    for (uint i= 0; i < voices; i++)
        muted[i] = false;
    ioctl(m_handle, HSID_IOCTL_RESET, volume);
    m_accessClk = 0;
    if (m_eventContext != NULL)
        m_eventContext->schedule (this, HARDSID_DELAY_CYCLES, m_phase);
}

uint8_t HardSID::read (uint_least8_t addr)
{
    if (!m_handle)
        return 0;

    event_clock_t cycles = m_eventContext->getTime (m_accessClk, m_phase);
    m_accessClk += cycles;

    while ( cycles > 0xffff )
    {
        /* delay */
        ioctl(m_handle, HSID_IOCTL_DELAY, 0xffff);
        cycles -= 0xffff;
    }

    uint packet = (( cycles & 0xffff ) << 16 ) | (( addr & 0x1f ) << 8 );
    ioctl(m_handle, HSID_IOCTL_READ, &packet);

    cycles = 0;
    return (uint8_t) (packet & 0xff);
}

void HardSID::write (uint_least8_t addr, uint8_t data)
{
    if (!m_handle)
        return;

    event_clock_t cycles = m_eventContext->getTime (m_accessClk, m_phase);
    m_accessClk += cycles;

    while ( cycles > 0xffff )
    {
        /* delay */
        ioctl(m_handle, HSID_IOCTL_DELAY, 0xffff);
        cycles -= 0xffff;
    }

    uint packet = (( cycles & 0xffff ) << 16 ) | (( addr & 0x1f ) << 8 )
        | (data & 0xff);
    cycles = 0;
    ::write (m_handle, &packet, sizeof (packet));
}

void HardSID::voice (uint_least8_t num, uint_least8_t volume,
                     bool mute)
{
    // Only have 3 voices!
    if (num >= voices)
        return;
    muted[num] = mute;
    
    int cmute = 0;
    for ( uint i = 0; i < voices; i++ )
        cmute |= (muted[i] << i);
    ioctl (m_handle, HSID_IOCTL_MUTE, cmute);
}

void HardSID::event (void)
{
    event_clock_t cycles = m_eventContext->getTime (m_accessClk, m_phase);
    if (cycles < HARDSID_DELAY_CYCLES)
    {
        m_eventContext->schedule (this, HARDSID_DELAY_CYCLES - cycles,
                                EVENT_CLOCK_PHI1);
    }
    else
    {
        uint delay = (uint) cycles;
        m_accessClk += cycles;
        ioctl(m_handle, HSID_IOCTL_DELAY, delay);
        m_eventContext->schedule (this, HARDSID_DELAY_CYCLES, m_phase);
    }
}

void HardSID::filter(bool enable)
{
    ioctl (m_handle, HSID_IOCTL_NOFILTER, !enable);
}

void HardSID::flush(void)
{
    ioctl(m_handle, HSID_IOCTL_FLUSH);
}

bool HardSID::lock(c64env* env)
{
    if( env == NULL )
    {
	if (!m_locked)
	    return false;
        m_eventContext->cancel (this);
        m_locked = false;
        m_eventContext = NULL;
    }
    else
    {
	if (m_locked)
	    return false;
        m_locked = true;
        m_eventContext = &env->context();
        m_eventContext->schedule (this, HARDSID_DELAY_CYCLES, m_phase);
    }
    return true;
}
