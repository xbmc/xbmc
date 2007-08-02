/***************************************************************************
                          xsid.h  -  Support for Playsids Extended
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
 *  $Log: xsid.h,v $
 *  Revision 1.22  2003/10/28 00:22:53  s_a_white
 *  getTime now returns a time with respect to the clocks desired phase.
 *
 *  Revision 1.21  2002/09/23 19:42:52  s_a_white
 *  Fixed intel compiler warnings.
 *
 *  Revision 1.20  2002/07/17 19:18:17  s_a_white
 *  Changed bad #if to #ifdef
 *
 *  Revision 1.19  2002/02/21 20:26:13  s_a_white
 *  Nolonger default to Galway Mode when Noise samples init incorrectly. Fixes
 *  VARIOUS/S-Z/Zyron/Bouncy_Balls.sid (HVSC).
 *
 *  Revision 1.18  2002/02/17 16:34:39  s_a_white
 *  New reset interface
 *
 *  Revision 1.17  2001/11/16 19:22:04  s_a_white
 *  Removed compiler warning for unused parameter.
 *
 *  Revision 1.16  2001/10/18 22:36:16  s_a_white
 *  GCC3 fixes.
 *
 *  Revision 1.15  2001/07/14 16:48:35  s_a_white
 *  Sync with sidbuilder class project.
 *
 *  Revision 1.14  2001/07/14 12:59:39  s_a_white
 *  XSID effeciency increased.  Now uses new component classes and event
 *  generation.
 *
 *  Revision 1.13  2001/04/20 22:21:06  s_a_white
 *  inlined updateSidData0x18.
 *
 *  Revision 1.12  2001/03/25 19:51:23  s_a_white
 *  Performance update.
 *
 *  Revision 1.11  2001/03/19 23:40:46  s_a_white
 *  Better support for global debug.
 *
 *  Revision 1.10  2001/03/09 22:27:13  s_a_white
 *  Speed optimisation update.
 *
 *  Revision 1.9  2001/03/01 23:45:58  s_a_white
 *  Combined both through sid and non-through sid modes.  Can be selected
 *  at runtime now.
 *
 *  Revision 1.8  2001/02/21 21:46:34  s_a_white
 *  0x1d = 0 now fixed.  Limit checking on sid volume.  This helps us determine
 *  even better what the sample offset should be (fixes Skate and Die).
 *
 *  Revision 1.7  2001/02/07 21:02:30  s_a_white
 *  Supported for delaying samples for frame simulation.  New alogarithm to
 *  better guess original tunes volume when playing samples.
 *
 *  Revision 1.6  2000/12/12 22:51:01  s_a_white
 *  Bug Fix #122033.
 *
 ***************************************************************************/

/*
Effectively there is only 1 channel, which can either perform Galway Noise
or Sampling.  However, to achieve all the effects on a C64, 2 sampling
channels are required.  No divide by 2 is required and is compensated for
automatically in the C64 machine code.

Confirmed by Warren Pilkington using the tune Turbo Outrun:
A new sample must interrupt an existing sample running on the same channel.

Confirmed by Michael Schwendt and Antonia Vera using the tune Game Over:
A Galway Sample or Noise sequence cannot interrupt any other.  However
the last of these new requested sequences will be played after the current
sequence ends.

Lastly playing samples through the SIDs volume is not as clean as playing
them on their own channel.  Playing through the SID will effect the volume
of the other channels and this will be most noticable at low frequencies.
These effects are however present in the original SID music.

Some SIDs put values directly into the volume register.  Others play samples
with respect to the current volume.  We can't for definate know which the author
has chosen originally.  We must just make a guess based on what the volume
is initially at the start of a sample sequence and from the details xSID has been
programmed with.
*/

#ifndef _xsid_h_
#define _xsid_h_

#include "config.h"
#include "sidbuilder.h"
#include "event.h"

// XSID configuration settings
//#define XSID_DEBUG 1

// Support global debug option
#ifdef DEBUG
#   ifndef XSID_DEBUG
#   define XSID_DEBUG DEBUG
#   endif
#endif

#ifdef XSID_DEBUG
#   include <stdio.h>
#endif

class XSID;
class channel
{
private:
    // General
    const char * const m_name;
    EventContext &m_context;
    event_phase_t m_phase;
    XSID         &m_xsid;
    friend class XSID;

    class SampleEvent: public Event
    {
    private:
        channel &m_ch;
        void event (void) { m_ch.sampleClock (); }

    public:
        SampleEvent (channel *ch)
        :Event("xSID Sample"),
         m_ch(*ch) {}
    } sampleEvent;
    friend class SampleEvent;

    class GalwayEvent: public Event
    {
    private:
        channel &m_ch;
        void event (void) { m_ch.galwayClock (); }

    public:
        GalwayEvent (channel *ch)
        :Event("xSID Galway"),
         m_ch(*ch) {}
    } galwayEvent;
    friend class GalwayEvent;

    uint8_t  reg[0x10];
    enum    {FM_NONE = 0, FM_HUELS, FM_GALWAY} mode;
    bool     active;
    uint_least16_t address;
    uint_least16_t cycleCount; // Counts to zero and triggers!
    uint_least8_t  volShift;
    uint_least8_t  sampleLimit;
    int8_t         sample;

    // Sample Section
    uint_least8_t  samRepeat;
    uint_least8_t  samScale;
    enum {SO_LOWHIGH = 0, SO_HIGHLOW = 1};
    uint_least8_t  samOrder;
    uint_least8_t  samNibble;
    uint_least16_t samEndAddr;
    uint_least16_t samRepeatAddr;
    uint_least16_t samPeriod;

    // Galway Section
    uint_least8_t  galTones;
    uint_least8_t  galInitLength;
    uint_least8_t  galLength;
    uint_least8_t  galVolume;
    uint_least8_t  galLoopWait;
    uint_least8_t  galNullWait;

    // For Debugging
    event_clock_t cycles;
    event_clock_t outputs;

private:
    channel (const char * const name, EventContext *context, XSID *xsid);
    void free        (void);
    void silence     (void);
    void sampleInit  (void);
    void sampleClock (void);
    void galwayInit  (void);
    void galwayClock (void);

    // Compress address to not leave so many spaces
    uint_least8_t convertAddr(uint_least8_t addr)
    { return (((addr) & 0x3) | ((addr) >> 3) & 0x0c); }

    void    reset    (void);
    uint8_t read     (uint_least8_t  addr)
    { return reg[convertAddr (addr)]; }
    void    write    (uint_least8_t addr, uint8_t data)
    { reg[convertAddr (addr)] = data; }
    int8_t  output   (void);
    bool    isGalway (void)
    { return mode == FM_GALWAY; }

    uint_least8_t limit  (void)
    { return sampleLimit; }

    inline void   checkForInit     (void);
    inline int8_t sampleCalculate  (void);
    inline void   galwayTonePeriod (void);

    // Used to indicate if channel is running
    operator bool()  const { return (active); }
};


class XSID: public sidemu, private Event
{
    friend class channel;

private:
    channel ch4;
    channel ch5;
    bool    muted;
    bool    suppressed;
    static  const char *credit;

    uint8_t             sidData0x18;
    bool                _sidSamples;
    int8_t              sampleOffset;
    static const int8_t sampleConvertTable[16];
    bool                wasRunning;

private:
    void    event                (void);
    void    checkForInit         (channel *ch);
    inline  void setSidData0x18  (void);
    inline  void recallSidData0x18 (void);
    int8_t  sampleOutput         (void);
    void    sampleOffsetCalc     (void);
    virtual uint8_t readMemByte  (uint_least16_t addr) = 0;
    virtual void    writeMemByte (uint8_t data) = 0;

public:
    XSID (EventContext *context);

    // Standard calls
    void    reset () { sidemu::reset (); }
    void    reset (uint8_t);
    uint8_t read  (uint_least8_t) { return 0; }
    void    write (uint_least8_t, uint8_t) { ; }
    const   char *credits (void) {return credit;}

    // Specialist calls
    uint8_t read     (uint_least16_t) { return 0; }
    void    write    (uint_least16_t addr, uint8_t data);
    int_least32_t output (uint_least8_t bits = 16);
    void    mute     (bool enable);
    bool    isMuted  (void) { return muted; }
    void    suppress (bool enable);

    void    sidSamples (bool enable)
    {   _sidSamples = enable; }
    // Return whether we care it was changed.
    bool storeSidData0x18 (uint8_t data);
};


/***************************************************************************
 * Inline functions
 **************************************************************************/

inline int_least32_t XSID::output (uint_least8_t bits)
{
    int_least32_t sample;
    if (_sidSamples || muted)
        return 0;
    sample = sampleConvertTable[sampleOutput () + 8];
    return sample << (bits - 8);
}

#endif // _xsid_h_
