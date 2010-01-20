/***************************************************************************
             hardsid-emu.h - Hardsid support interface.
                             -------------------
    begin                : Fri Dec 15 2000
    copyright            : (C) 2000-2002 by Simon White
                         : (C) 2001-2002 by Jarno Paananen
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
 *  $Log: hardsid-emu.h,v $
 *  Revision 1.8  2004/03/18 20:50:21  s_a_white
 *  Indicate the 2.07 extensions.
 *
 *  Revision 1.7  2003/10/28 00:15:16  s_a_white
 *  Get time with respect to correct clock phase.
 *
 *  Revision 1.6  2003/06/27 07:08:17  s_a_white
 *  Use new hardsid.dll muting interface.
 *
 *  Revision 1.5  2002/08/09 18:11:35  s_a_white
 *  Added backwards compatibility support for older hardsid.dll.
 *
 *  Revision 1.4  2002/07/20 08:36:24  s_a_white
 *  Remove unnecessary and pointless conts.
 *
 *  Revision 1.3  2002/02/17 17:24:50  s_a_white
 *  Updated for new reset interface.
 *
 *  Revision 1.2  2002/01/29 21:47:35  s_a_white
 *  Constant fixed interval delay added to prevent emulation going fast when
 *  there are no writes to the sid.
 *
 *  Revision 1.1  2002/01/28 22:35:20  s_a_white
 *  Initial Release.
 *
 ***************************************************************************/

#ifndef _hardsid_emu_h_
#define _hardsid_emu_h_

#include <sidplay/sidbuilder.h>
#include <sidplay/event.h>
#include "config.h"

#ifdef HAVE_MSWINDOWS

#include <windows.h>

#define HSID_VERSION_MIN (WORD) 0x0200
#define HSID_VERSION_204 (WORD) 0x0204
#define HSID_VERSION_207 (WORD) 0x0207

//**************************************************************************
// Version 2 Interface
typedef void (CALLBACK* HsidDLL2_Delay_t)   (BYTE deviceID, WORD cycles);
typedef BYTE (CALLBACK* HsidDLL2_Devices_t) (void);
typedef void (CALLBACK* HsidDLL2_Filter_t)  (BYTE deviceID, BOOL filter);
typedef void (CALLBACK* HsidDLL2_Flush_t)   (BYTE deviceID);
typedef void (CALLBACK* HsidDLL2_Mute_t)    (BYTE deviceID, BYTE channel, BOOL mute);
typedef void (CALLBACK* HsidDLL2_MuteAll_t) (BYTE deviceID, BOOL mute);
typedef void (CALLBACK* HsidDLL2_Reset_t)   (BYTE deviceID);
typedef BYTE (CALLBACK* HsidDLL2_Read_t)    (BYTE deviceID, WORD cycles, BYTE SID_reg);
typedef void (CALLBACK* HsidDLL2_Sync_t)    (BYTE deviceID);
typedef void (CALLBACK* HsidDLL2_Write_t)   (BYTE deviceID, WORD cycles, BYTE SID_reg, BYTE data);
typedef WORD (CALLBACK* HsidDLL2_Version_t) (void);

// Version 2.04 Extensions
typedef BOOL (CALLBACK* HsidDLL2_Lock_t)    (BYTE deviceID);
typedef void (CALLBACK* HsidDLL2_Unlock_t)  (BYTE deviceID);
typedef void (CALLBACK* HsidDLL2_Reset2_t)  (BYTE deviceID, BYTE volume);

// Version 2.07 Extensions
typedef void (CALLBACK* HsidDLL2_Mute2_t)   (BYTE deviceID, BYTE channel, BOOL mute, BOOL manual);

struct HsidDLL2
{
    HINSTANCE          Instance;
    HsidDLL2_Delay_t   Delay;
    HsidDLL2_Devices_t Devices;
    HsidDLL2_Filter_t  Filter;
    HsidDLL2_Flush_t   Flush;
    HsidDLL2_Lock_t    Lock;
    HsidDLL2_Unlock_t  Unlock;
    HsidDLL2_Mute_t    Mute;
    HsidDLL2_Mute2_t   Mute2;
    HsidDLL2_MuteAll_t MuteAll;
    HsidDLL2_Reset_t   Reset;
    HsidDLL2_Reset2_t  Reset2;
    HsidDLL2_Read_t    Read;
    HsidDLL2_Sync_t    Sync;
    HsidDLL2_Write_t   Write;
    WORD               Version;
};

#endif // HAVE_MSWINDOWS

#define HARDSID_VOICES 3
// Approx 60ms
#define HARDSID_DELAY_CYCLES 60000

/***************************************************************************
 * HardSID SID Specialisation
 ***************************************************************************/
class HardSID: public sidemu, private Event
{
private:
    friend class HardSIDBuilder;

    // HardSID specific data
#ifdef HAVE_UNIX
    static         bool m_sidFree[16];
    int            m_handle;
#endif

    static const   uint voices;
    static         uint sid;
    static char    credit[100];


    // Generic variables
    EventContext  *m_eventContext;
    event_phase_t  m_phase;
    event_clock_t  m_accessClk;
    char           m_errorBuffer[100];

    // Must stay in this order
    bool           muted[HARDSID_VOICES];
    uint           m_instance;
    bool           m_status;
    bool           m_locked;

public:
    HardSID  (sidbuilder *builder);
    ~HardSID ();

    // Standard component functions
    const char   *credits (void) {return credit;}
    void          reset   () { sidemu::reset (); }
    void          reset   (uint8_t volume);
    uint8_t       read    (uint_least8_t addr);
    void          write   (uint_least8_t addr, uint8_t data);
    const char   *error   (void) {return m_errorBuffer;}
    operator bool () const { return m_status; }

    // Standard SID functions
    int_least32_t output  (uint_least8_t bits);
    void          filter  (bool enable);
    void          model   (sid2_model_t model) {;}
    void          voice   (uint_least8_t num, uint_least8_t volume,
                           bool mute);
    void          gain    (int_least8_t) {;}

    // HardSID specific
    void          flush   (void);

    // Must lock the SID before using the standard functions.
    bool          lock    (c64env *env);

private:
    // Fixed interval timer delay to prevent sidplay2
    // shoot to 100% CPU usage when song nolonger
    // writes to SID.
    void event (void);
};

inline int_least32_t HardSID::output (uint_least8_t bits)
{   // Not supported, should return samples off card...???
    return 0;
}

#endif // _hardsid_emu_h_
