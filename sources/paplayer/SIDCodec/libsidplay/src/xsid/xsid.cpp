/***************************************************************************
                          xsid.cpp  -  Support for Playsids Extended
                                       Registers
                             -------------------
    begin                : Tue Jun 20 2000
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
 *  $Log: xsid.cpp,v $
 *  Revision 1.24  2004/05/28 15:45:13  s_a_white
 *  Correct credit email address
 *
 *  Revision 1.23  2003/10/28 00:22:53  s_a_white
 *  getTime now returns a time with respect to the clocks desired phase.
 *
 *  Revision 1.22  2003/02/24 19:45:32  s_a_white
 *  Make sure events are canceled on reset.
 *
 *  Revision 1.21  2003/01/17 08:36:37  s_a_white
 *  Event scheduler phase support.
 *
 *  Revision 1.20  2002/07/17 21:19:54  s_a_white
 *  Minor non sid sample mode fixes.
 *
 *  Revision 1.19  2002/02/21 20:26:13  s_a_white
 *  Nolonger default to Galway Mode when Noise samples init incorrectly. Fixes
 *  VARIOUS/S-Z/Zyron/Bouncy_Balls.sid (HVSC).
 *
 *  Revision 1.18  2002/02/17 16:34:39  s_a_white
 *  New reset interface
 *
 *  Revision 1.17  2002/01/28 19:31:42  s_a_white
 *  PSID sample improvements.
 *
 *  Revision 1.16  2001/10/02 18:03:03  s_a_white
 *  Support updated sidbuilder class interface.
 *
 *  Revision 1.15  2001/09/17 18:36:41  s_a_white
 *  Changed object construction to prevent multiple resets.
 *
 *  Revision 1.14  2001/07/14 12:59:53  s_a_white
 *  XSID effeciency increased.  Now uses new component classes and event
 *  generation.
 *
 *  Revision 1.13  2001/03/25 19:51:23  s_a_white
 *  Performance update.
 *
 *  Revision 1.12  2001/03/19 23:40:19  s_a_white
 *  Removed repeat definition of state for debug mode.
 *
 *  Revision 1.11  2001/03/09 22:27:13  s_a_white
 *  Speed optimisation update.
 *
 *  Revision 1.10  2001/03/01 23:45:58  s_a_white
 *  Combined both through sid and non-through sid modes.  Can be selected
 *  at runtime now.
 *
 *  Revision 1.9  2001/02/21 21:46:34  s_a_white
 *  0x1d = 0 now fixed.  Limit checking on sid volume.  This helps us determine
 *  even better what the sample offset should be (fixes Skate and Die).
 *
 *  Revision 1.8  2001/02/07 21:02:30  s_a_white
 *  Supported for delaying samples for frame simulation.  New alogarithm to
 *  better guess original tunes volume when playing samples.
 *
 *  Revision 1.7  2000/12/12 22:51:01  s_a_white
 *  Bug Fix #122033.
 *
 ***************************************************************************/

#include <string.h>
#include <stdio.h>
#include "config.h"
#include "sidendian.h"
#include "xsid.h"


// Convert from 4 bit resolution to 8 bits
/* Rev 2.0.5 (saw) - Removed for a more non-linear equivalent
   which better models the SIDS master volume register
const int8_t XSID::sampleConvertTable[16] =
{
    '\x80', '\x91', '\xa2', '\xb3', '\xc4', '\xd5', '\xe6', '\xf7',
    '\x08', '\x19', '\x2a', '\x3b', '\x4c', '\x5d', '\x6e', '\x7f'
};
*/
const int8_t XSID::sampleConvertTable[16] =
{
    '\x80', '\x94', '\xa9', '\xbc', '\xce', '\xe1', '\xf2', '\x03',
    '\x1b', '\x2a', '\x3b', '\x49', '\x58', '\x66', '\x73', '\x7f'
};

const char *XSID::credit =
{
    "xSID (Extended SID) Engine:\0"
    "\tCopyright (C) 2000 Simon White <" S_A_WHITE_EMAIL ">\0"
};


channel::channel (const char * const name, EventContext *context, XSID *xsid)
:m_name(name),
 m_context(*context),
 m_phase(EVENT_CLOCK_PHI1),
 m_xsid(*xsid),
 sampleEvent(this),
 galwayEvent(this)
{
    memset (reg, 0, sizeof (reg));
    active = true;
    reset  ();
}

void channel::reset ()
{
    galVolume  = 0; // This is left to free run until reset
    mode       = FM_NONE;
    free ();
    // Remove outstanding events
    m_context.cancel (&m_xsid);
    m_context.cancel (&sampleEvent);
    m_context.cancel (&galwayEvent);
}

void channel::free ()
{
    active      = false;
    cycleCount  = 0;
    sampleLimit = 0;
    // Set XSID to stopped state
    reg[convertAddr (0x1d)] = 0;
    silence ();
}

inline int8_t channel::output ()
{
    outputs++;
    return sample;
}

void channel::checkForInit ()
{   // Check to see mode of operation
    // See xsid documentation
    switch (reg[convertAddr (0x1d)])
    {
    case 0xFF:
    case 0xFE:
    case 0xFC:
        sampleInit ();
        break;
    case 0xFD:
        if (!active)
            return;
        free (); // Stop
        // Calculate the sample offset
        m_xsid.sampleOffsetCalc ();
        break;
    case 0x00:
        break;
    default:
        galwayInit ();
    }
}

void channel::sampleInit ()
{
    uint8_t *r;
    if (active && (mode == FM_GALWAY))
        return;

#ifdef XSID_DEBUG
    printf ("XSID [%s]: Sample Init\n", m_name);
    if (active && (mode == FM_HUELS))
        printf ("XSID [%s]: Stopping Playing Sample\n", m_name);
#endif

    // Check all important parameters are legal
    r             = &reg[convertAddr (0x1d)];
    volShift      = (uint_least8_t) (0 - (int8_t) r[0]) >> 1;
    r[0]          = 0;
    // Use endian_16 as can't g
    r             = &reg[convertAddr (0x1e)];
    address       = endian_16 (r[1], r[0]);
    r             = &reg[convertAddr (0x3d)];
    samEndAddr    = endian_16 (r[1], r[0]);
    if (samEndAddr <= address) return;
    samScale      = reg[convertAddr  (0x5f)];
    r             = &reg[convertAddr (0x5d)];
    samPeriod     = endian_16 (r[1], r[0]) >> samScale;
    if (!samPeriod)
    {   // Stop this channel
        reg[convertAddr (0x1d)] = 0xfd;
        checkForInit ();
        return;
    }

    // Load the other parameters
    samNibble     = 0;
    samRepeat     = reg[convertAddr  (0x3f)];
    samOrder      = reg[convertAddr  (0x7d)];
    r             = &reg[convertAddr (0x7e)];
    samRepeatAddr = endian_16 (r[1], r[0]);
    cycleCount    = samPeriod;

    // Support Galway Samples, but that
    // mode it setup only when as Galway
    // Noise sequence begins
    if (mode == FM_NONE)
        mode  = FM_HUELS;

    active  = true;
    cycles  = 0;
    outputs = 0;

    sampleLimit = 8 >> volShift;
    sample      = sampleCalculate ();

    // Calculate the sample offset
    m_xsid.sampleOffsetCalc ();

#ifdef XSID_DEBUG
#   if XSID_DEBUG > 1
    printf ("XSID [%s]: Sample Start Address:  0x%04x\n", m_name, address);
    printf ("XSID [%s]: Sample End Address:    0x%04x\n", m_name, samEndAddr);
    printf ("XSID [%s]: Sample Repeat Address: 0x%04x\n", m_name, samRepeatAddr);
    printf ("XSID [%s]: Sample Period: %u\n", m_name, samPeriod);
    printf ("XSID [%s]: Sample Repeat: %u\n", m_name, samRepeat);
    printf ("XSID [%s]: Sample Order:  %u\n", m_name, samOrder);
#   endif
    printf ("XSID [%s]: Sample Start\n", m_name);
#endif // XSID_DEBUG

    // Schedule a sample update
    m_context.schedule (&m_xsid, 0, m_phase);
    m_context.schedule (&sampleEvent, cycleCount, m_phase);
}

void channel::sampleClock ()
{
    cycleCount   = samPeriod;
    if (address >= samEndAddr)
    {
        if (samRepeat != 0xFF)
        {
            if (samRepeat)
                samRepeat--;
            else
                samRepeatAddr = address;
        }

        address      = samRepeatAddr;
        if (address >= samEndAddr)
        {   // The sequence has completed
            uint8_t &status = reg[convertAddr (0x1d)];
            if (!status)
                status = 0xfd;
            if (status != 0xfd)
                active = false;
#ifdef XSID_DEBUG
            printf ("XSID [%s]: Sample Stop (%lu Cycles, %lu Outputs)\n",
                    m_name, cycles, outputs);
            if (status != 0xfd)
                printf ("XSID [%s]: Starting Delayed Sequence\n", m_name);
#endif
            checkForInit ();
            return;
        }
    }

    // We have reached the required sample
    // So now we need to extract the right nibble
    sample  = sampleCalculate ();
    cycles += cycleCount;
    // Schedule a sample update
    m_context.schedule (&sampleEvent, cycleCount, m_phase);
    m_context.schedule (&m_xsid, 0, m_phase);
}

int8_t channel::sampleCalculate ()
{
    uint_least8_t tempSample = m_xsid.readMemByte (address);
    if (samOrder == SO_LOWHIGH)
    {
        if (samScale == 0)
        {
            if (samNibble != 0)
                tempSample >>= 4;
        }
        // AND 15 further below.
    }
    else // if (samOrder == SO_HIGHLOW)
    {
        if (samScale == 0)
        {
            if (samNibble == 0)
                tempSample >>= 4;
        }
        else // if (samScale != 0)
            tempSample >>= 4;
        // AND 15 further below.
    }

    // Move to next address
    address   += samNibble;
    samNibble ^= 1;
    return (int8_t) ((tempSample & 0x0f) - 0x08) >> volShift;
}

void channel::galwayInit()
{
    uint8_t *r;
    if (active)
        return;

#ifdef XSID_DEBUG
    printf ("XSID [%s]: Galway Init\n", m_name);
#endif

    // Check all important parameters are legal
    r             = &reg[convertAddr (0x1d)];
    galTones      = r[0];
    r[0]          = 0;
    galInitLength = reg[convertAddr (0x3d)];
    if (!galInitLength) return;
    galLoopWait   = reg[convertAddr (0x3f)];
    if (!galLoopWait)   return;
    galNullWait   = reg[convertAddr (0x5d)];
    if (!galNullWait)   return;

    // Load the other parameters
    r        = &reg[convertAddr(0x1e)];
    address  = endian_16 (r[1], r[0]);
    volShift = reg[convertAddr (0x3e)] & 0x0f;
    mode     = FM_GALWAY;
    active   = true;
    cycles   = 0;
    outputs  = 0;

    sampleLimit = 8;
    sample      = (int8_t) galVolume - 8;
    galwayTonePeriod ();

    // Calculate the sample offset
    m_xsid.sampleOffsetCalc ();

#ifdef XSID_DEBUG
    printf ("XSID [%s]: Galway Start\n", m_name);
#endif

    // Schedule a sample update
    m_context.schedule (&m_xsid, 0, m_phase);
    m_context.schedule (&galwayEvent, cycleCount, m_phase);
}

void channel::galwayClock ()
{
    if (--galLength)
        cycleCount = samPeriod;
    else if (galTones == 0xff)
    {   // The sequence has completed
        uint8_t &status = reg[convertAddr (0x1d)];
        if (!status)
            status = 0xfd;
        if (status != 0xfd)
            active = false;
#ifdef XSID_DEBUG
        printf ("XSID [%s]: Galway Stop (%lu Cycles, %lu Outputs)\n",
                m_name, cycles, outputs);
        if (status != 0xfd)
            printf ("XSID [%s]: Starting Delayed Sequence\n", m_name);
#endif
        checkForInit ();
        return;
    }
    else
        galwayTonePeriod ();

    // See Galway Example...
    galVolume += volShift;
    galVolume &= 0x0f;
    sample     = (int8_t) galVolume - 8;
    cycles    += cycleCount;
    m_context.schedule (&galwayEvent, cycleCount, m_phase);
    m_context.schedule (&m_xsid, 0, m_phase);
}

void channel::galwayTonePeriod ()
{   // Calculate the number of cycles over which sample should last
    galLength  = galInitLength;
    samPeriod  = m_xsid.readMemByte (address + galTones);
    samPeriod *= galLoopWait;
    samPeriod += galNullWait;
    cycleCount = samPeriod;
#if XSID_DEBUG > 2
    printf ("XSID [%s]: Galway Settings\n", m_name);
    printf ("XSID [%s]: Length %u, LoopWait %u, NullWait %u\n",
        m_name, galLength, galLoopWait, galNullWait);
    printf ("XSID [%s]: Tones %u, Data %u\n",
        m_name, galTones, m_xsid.readMemByte (address + galTones));
#endif
    galTones--;
}

void channel::silence ()
{
    sample = 0;
    m_context.cancel   (&sampleEvent);
    m_context.cancel   (&galwayEvent);
    m_context.schedule (&m_xsid, 0, m_phase);
}


XSID::XSID (EventContext *context)
:sidemu(NULL),
 Event("xSID"),
 ch4("CH4", context, this),
 ch5("CH5", context, this),
 muted(false),
 suppressed(false),
 wasRunning(false)
{
    sidSamples (true);
}

void XSID::reset (uint8_t)
{
    ch4.reset ();
    ch5.reset ();
    suppressed = false;
    wasRunning = false;
}

void XSID::event (void)
{
    if (ch4 || ch5)
    {
        setSidData0x18 ();
        wasRunning = true;
    }
    else if (wasRunning)
    {
        recallSidData0x18 ();
        wasRunning = false;
    }
}

// Use Suppress to delay the samples and start them later
// Effectivly allows running samples in a frame based mode.
void XSID::suppress (bool enable)
{
    // @FIXME@: Mute Temporary Hack
    suppressed = enable;
    if (!suppressed)
    {   // Get the channels running
#if XSID_DEBUG
        printf ("XSID: Un-suppressing\n");
#endif
        ch4.checkForInit ();
        ch5.checkForInit ();
    }
#if XSID_DEBUG
    else
        printf ("XSID: Suppressing\n");
#endif
}

// By muting samples they will start and play the at the
// appropriate time but no sound is produced.  Un-muting
// will cause sound output from the current play position.
void XSID::mute (bool enable)
{
    if (!muted && enable && wasRunning)
        recallSidData0x18 ();
    muted = enable;
}

void XSID::write (uint_least16_t addr, uint8_t data)
{
    channel *ch;
    uint8_t tempAddr;

    // Make sure address is legal
    if ((addr & 0xfe8c) ^ 0x000c)
        return;

    ch = &ch4;
    if (addr & 0x0100)
        ch = &ch5;

    tempAddr = (uint8_t) addr;
    ch->write (tempAddr, data);
#if XSID_DEBUG > 1
    printf ("XSID: Addr 0x%02x, Data 0x%02x\n", tempAddr, data);
#endif

    if (tempAddr == 0x1d)
    {
        if (suppressed)
        {
#if XSID_DEBUG
            printf ("XSID: Initialise Suppressed\n");
#endif
            return;
        }
        ch->checkForInit ();
    }
}

int8_t XSID::sampleOutput (void)
{
    int8_t sample;
    sample  = ch4.output ();
    sample += ch5.output ();
    // Automatically compensated for by C64 code
    //return (sample >> 1);
    return sample;
}

void XSID::setSidData0x18 (void)
{
    if (!_sidSamples || muted)
        return;

    uint8_t data = (sidData0x18 & 0xf0);
    data |= ((sampleOffset + sampleOutput ()) & 0x0f);

#ifdef XSID_DEBUG
    if ((sampleOffset + sampleOutput ()) > 0x0f)
    {
        printf ("XSID: Sample Wrapped [offset %u, sample %d]\n",
                sampleOffset, sampleOutput ());
    }
#   if XSID_DEBUG > 1
    printf ("XSID: Writing Sample to SID Volume [0x%02x]\n", data);
#   endif
#endif // XSID_DEBUG

    writeMemByte (data);
}

void XSID::recallSidData0x18 (void)
{   // Rev 2.0.5 (saw) - Changed to recall volume differently depending on mode
    // Normally after samples volume should be restored to half volume,
    // however, Galway Tunes sound horrible and seem to require setting back to
    // the original volume.  Setting back to the original volume for normal
    // samples can have nasty pulsing effects
    if (ch4.isGalway ())
    {
        if (_sidSamples && !muted)
            writeMemByte (sidData0x18);
    }
    else
        setSidData0x18 ();
}

void XSID::sampleOffsetCalc (void)
{
    // Try to determine a sensible offset between voice
    // and sample volumes.
    uint_least8_t lower = ch4.limit () + ch5.limit ();
    uint_least8_t upper;

    // Both channels seem to be off.  Keep current offset!
    if (!lower)
        return;

    sampleOffset = sidData0x18 & 0x0f;

    // Is possible to compensate for both channels
    // set to 4 bits here, but should never happen.
    if (lower > 8)
        lower >>= 1;
    upper = 0x0f - lower + 1;

    // Check against limits
    if (sampleOffset < lower)
        sampleOffset = lower;
    else if (sampleOffset > upper)
        sampleOffset = upper;

#ifdef XSID_DEBUG
    printf ("XSID: Sample Offset %d based on channel(s) ", sampleOffset);
    if (ch4)
        printf ("4 ");
    if (ch5)
        printf ("5");
    printf ("\n");
#endif // XSID_DEBUG
}

bool XSID::storeSidData0x18 (uint8_t data)
{
    sidData0x18 = data;
    if (ch4 || ch5)
    {   // Force volume to be changed at next clock
        sampleOffsetCalc ();
        if (_sidSamples)
        {
#if XSID_DEBUG
            printf ("XSID: SID Volume Changed Externally (Corrected).\n");
#endif
            return true;
        }
    }
    writeMemByte (sidData0x18);
    return false;
}
