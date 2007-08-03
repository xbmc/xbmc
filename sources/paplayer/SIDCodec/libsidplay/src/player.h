    /***************************************************************************
                          player.h  -  description
                             -------------------
    begin                : Fri Jun 9 2000
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
 *  $Log: player.h,v $
 *  Revision 1.51  2004/05/03 22:42:56  s_a_white
 *  Change how port handling is dealt with to provide better C64 compatiiblity.
 *  Add character rom support.
 *
 *  Revision 1.50  2004/04/13 07:40:47  s_a_white
 *  Add lightpen support.
 *
 *  Revision 1.49  2004/03/18 20:15:15  s_a_white
 *  Added sidmapper (so support more the 2 sids).
 *
 *  Revision 1.48  2004/01/31 17:01:44  s_a_white
 *  Add ability to specify the maximum number of sid writes forming the sid2crc.
 *
 *  Revision 1.47  2003/12/15 23:50:37  s_a_white
 *  Fixup use of inline functions and correct mileage calculations.
 *
 *  Revision 1.46  2003/10/16 07:42:49  s_a_white
 *  Allow redirection of debug information to file.
 *
 *  Revision 1.45  2003/06/27 21:15:25  s_a_white
 *  Tidy up mono to stereo sid conversion, only allowing it theres sufficient
 *  support from the emulation.  Allow user to override whether they want this
 *  to happen.
 *
 *  Revision 1.44  2003/05/28 20:11:19  s_a_white
 *  Support the psiddrv overlapping unused parts of the tunes load image.
 *
 *  Revision 1.43  2003/02/20 19:11:48  s_a_white
 *  sid2crc support.
 *
 *  Revision 1.42  2003/01/23 17:32:39  s_a_white
 *  Redundent code removal.
 *
 *  Revision 1.41  2003/01/20 18:37:08  s_a_white
 *  Stealing update.  Apparently the cpu does a memory read from any non
 *  write cycle (whether it needs to or not) resulting in those cycles
 *  being stolen.
 *
 *  Revision 1.40  2003/01/17 08:35:46  s_a_white
 *  Event scheduler phase support.
 *
 *  Revision 1.39  2002/12/03 23:25:53  s_a_white
 *  Prevent PSID digis from playing in real C64 mode.
 *
 *  Revision 1.38  2002/11/27 00:16:51  s_a_white
 *  Make sure driver info gets reset and exported properly.
 *
 *  Revision 1.37  2002/11/20 21:44:34  s_a_white
 *  Initial support for external DMA to steal cycles away from the CPU.
 *
 *  Revision 1.36  2002/11/19 22:55:50  s_a_white
 *  PSIDv2NG/RSID changes to deal with spec updates for recommended
 *  implementation.
 *
 *  Revision 1.35  2002/11/01 17:36:01  s_a_white
 *  Frame based support for old sidplay1 modes.
 *
 *  Revision 1.34  2002/10/02 19:45:23  s_a_white
 *  RSID support.
 *
 *  Revision 1.33  2002/09/12 21:01:31  s_a_white
 *  Added support for simulating the random delay before the user loads a
 *  program on a real C64.
 *
 *  Revision 1.32  2002/09/09 18:01:30  s_a_white
 *  Prevent m_info driver details getting modified when C64 crashes.
 *
 *  Revision 1.31  2002/07/21 19:39:41  s_a_white
 *  Proper error handling of reloc info overlapping load image.
 *
 *  Revision 1.30  2002/07/20 08:34:52  s_a_white
 *  Remove unnecessary and pointless conts.
 *
 *  Revision 1.29  2002/07/17 21:48:10  s_a_white
 *  PSIDv2NG reloc exclude region extension.
 *
 *  Revision 1.28  2002/04/14 21:46:50  s_a_white
 *  PlaySID reads fixed to come from RAM only.
 *
 *  Revision 1.27  2002/03/12 18:43:59  s_a_white
 *  Tidy up handling of envReset on illegal CPU instructions.
 *
 *  Revision 1.26  2002/03/03 22:01:58  s_a_white
 *  New clock speed & sid model interface.
 *
 *  Revision 1.25  2002/01/29 21:50:33  s_a_white
 *  Auto switching to a better emulation mode.  m_tuneInfo reloaded after a
 *  config.  Initial code added to support more than two sids.
 *
 *  Revision 1.24  2002/01/28 19:32:01  s_a_white
 *  PSID sample improvements.
 *
 *  Revision 1.23  2001/12/13 08:28:08  s_a_white
 *  Added namespace support to fix problems with xsidplay.
 *
 *  Revision 1.22  2001/10/18 22:34:04  s_a_white
 *  GCC3 fixes.
 *
 *  Revision 1.21  2001/10/02 18:26:36  s_a_white
 *  Removed ReSID support and updated for new scheduler.
 *
 *  Revision 1.20  2001/09/20 19:32:39  s_a_white
 *  Support for a null sid emulation to use when a builder create call fails.
 *
 *  Revision 1.19  2001/09/17 19:02:38  s_a_white
 *  Now uses fixed point maths for sample output and rtc.
 *
 *  Revision 1.18  2001/09/01 11:15:46  s_a_white
 *  Fixes sidplay1 environment modes.
 *
 *  Revision 1.17  2001/08/10 21:01:06  s_a_white
 *  Fixed RTC initialisation order warning.
 *
 *  Revision 1.16  2001/08/10 20:03:19  s_a_white
 *  Added RTC reset.
 *
 *  Revision 1.15  2001/07/25 17:01:13  s_a_white
 *  Support for new configuration interface.
 *
 *  Revision 1.14  2001/07/14 16:46:16  s_a_white
 *  Sync with sidbuilder class project.
 *
 *  Revision 1.13  2001/07/14 12:50:58  s_a_white
 *  Support for credits and debuging.  External filter selection removed.  RTC
 *  and samples obtained in a more efficient way.  Support for component
 *  and sidbuilder classes.
 *
 *  Revision 1.12  2001/04/23 17:09:56  s_a_white
 *  Fixed video speed selection using unforced/forced and NTSC clockSpeeds.
 *
 *  Revision 1.11  2001/03/22 22:45:20  s_a_white
 *  Re-ordered initialisations to match defintions.
 *
 *  Revision 1.10  2001/03/21 23:28:12  s_a_white
 *  Support new component names.
 *
 *  Revision 1.9  2001/03/21 22:32:55  s_a_white
 *  Filter redefinition support.  VIC & NMI support added.
 *
 *  Revision 1.8  2001/03/08 22:48:33  s_a_white
 *  Sid reset on player destruction removed.  Now handled locally by the sids.
 *
 *  Revision 1.7  2001/03/01 23:46:37  s_a_white
 *  Support for sample mode to be selected at runtime.
 *
 *  Revision 1.6  2001/02/28 18:52:55  s_a_white
 *  Removed initBank* related stuff.
 *
 *  Revision 1.5  2001/02/21 21:41:51  s_a_white
 *  Added seperate ram bank to hold C64 player.
 *
 *  Revision 1.4  2001/02/07 20:56:46  s_a_white
 *  Samples now delayed until end of simulated frame.
 *
 *  Revision 1.3  2001/01/23 21:26:28  s_a_white
 *  Only way to load a tune now is by passing in a sidtune object.  This is
 *  required for songlength database support.
 *
 *  Revision 1.2  2001/01/07 15:58:37  s_a_white
 *  SID2_LIB_API now becomes a core define (SID_API).
 *
 *  Revision 1.1  2000/12/12 19:15:40  s_a_white
 *  Renamed from sidplayer
 *
 ***************************************************************************/

#ifndef _player_h_
#define _player_h_

#include <stdio.h>
#include <string.h>

#include "sid2types.h"
#include "SidTune.h"
#include "sidbuilder.h"

#include "config.h"
#include "sidenv.h"
#include "c64env.h"
#include "c64/c64xsid.h"
#include "c64/c64cia.h"
#include "c64/c64vic.h"

#include "mos6510/mos6510.h"
#include "sid6526/sid6526.h"
#include "nullsid.h"
#define  SID2_MAX_SIDS 2
#define  SID2_TIME_BASE 10
#define  SID2_MAPPER_SIZE 32

SIDPLAY2_NAMESPACE_START

class Player: private C64Environment, c64env
{
private:
    static const double CLOCK_FREQ_NTSC;
    static const double CLOCK_FREQ_PAL;
    static const double VIC_FREQ_PAL;
    static const double VIC_FREQ_NTSC;

    static const char  *TXT_PAL_VBI;
    static const char  *TXT_PAL_VBI_FIXED;
    static const char  *TXT_PAL_CIA;
    static const char  *TXT_PAL_UNKNOWN;
    static const char  *TXT_NTSC_VBI;
    static const char  *TXT_NTSC_VBI_FIXED;
    static const char  *TXT_NTSC_CIA;
    static const char  *TXT_NTSC_UNKNOWN;
    static const char  *TXT_NA;

    static const char  *ERR_CONF_WHILST_ACTIVE;
    static const char  *ERR_UNSUPPORTED_FREQ;
    static const char  *ERR_UNSUPPORTED_PRECISION;
    static const char  *ERR_MEM_ALLOC;
    static const char  *ERR_UNSUPPORTED_MODE;
    static const char  *credit[10]; // 10 credits max

    static const char  *ERR_PSIDDRV_NO_SPACE; 
    static const char  *ERR_PSIDDRV_RELOC;

    EventScheduler m_scheduler;

    //SID6510  cpu(6510, "Main CPU");
    SID6510 sid6510;
    MOS6510 mos6510;
    MOS6510 *cpu;
    // Sid objects to use.
    NullSID nullsid;
    c64xsid xsid;
    c64cia1 cia;
    c64cia2 cia2;
    SID6526 sid6526;
    c64vic  vic;
    sidemu *sid[SID2_MAX_SIDS];
    int     m_sidmapper[32]; // Mapping table in d4xx-d7xx

    class EventMixer: public Event
    {
    private:
        Player &m_player;
        void event (void) { m_player.mixer (); }

    public:
        EventMixer (Player *player)
        :Event("Mixer"),
         m_player(*player) {}
    } mixerEvent;
    friend class EventMixer;

    class EventRTC: public Event
    {
    private:
        EventContext &m_eventContext;
        event_clock_t m_seconds;
        event_clock_t m_period;
        event_clock_t m_clk;

        void event (void)
        {   // Fixed point 25.7 (approx 2 dp)
            event_clock_t cycles;
            m_clk += m_period;
            cycles = m_clk >> 7;
            m_clk &= 0x7F;
            m_seconds++;
            m_eventContext.schedule (this, cycles, EVENT_CLOCK_PHI1);
        }

    public:
        EventRTC (EventContext *context)
        :Event("RTC"),
         m_eventContext(*context),
         m_seconds(0)
        {;}

        event_clock_t getTime () const {return m_seconds;}

        void reset (void)
        {   // Fixed point 25.7
            m_seconds = 0;
            m_clk     = m_period & 0x7F;
            m_eventContext.schedule (this, m_period >> 7, EVENT_CLOCK_PHI1);
        }

        void clock (float64_t period)
        {   // Fixed point 25.7
            m_period = (event_clock_t) (period / 10.0 * (float64_t) (1 << 7));
            reset ();
        }   
    } rtc;

    // User Configuration Settings
    SidTuneInfo   m_tuneInfo;
    uint8_t      *m_ram, *m_rom;
    sid2_info_t   m_info;
    sid2_config_t m_cfg;

    const char     *m_errorString;
    float64_t       m_fastForwardFactor;
    uint_least32_t  m_mileage;
    int_least32_t   m_leftVolume;
    int_least32_t   m_rightVolume;
    volatile sid2_player_t m_playerState;
    volatile bool   m_running;
    int             m_rand;
    uint_least32_t  m_sid2crc;
    uint_least32_t  m_sid2crcCount;
    bool            m_emulateStereo;

    // Mixer settings
    event_clock_t  m_sampleClock;
    event_clock_t  m_samplePeriod;
    uint_least32_t m_sampleCount;
    uint_least32_t m_sampleIndex;
    char          *m_sampleBuffer;

    // C64 environment settings
    struct
    {
        uint8_t pr_out;
        uint8_t ddr;
        uint8_t pr_in;
    } m_port;

    uint8_t m_playBank;

    // temp stuff -------------
    bool   isKernal;
    bool   isBasic;
    bool   isIO;
    bool   isChar;
    void   evalBankSelect (uint8_t data);
    void   c64_initialise (void);
    // ------------------------

private:
    float64_t clockSpeed     (sid2_clock_t clock, sid2_clock_t defaultClock,
                              bool forced);
    int       environment    (sid2_env_t env);
    void      fakeIRQ        (void);
    int       initialise     (void);
    void      nextSequence   (void);
    void      mixer          (void);
    void      mixerReset     (void);
    void      mileageCorrect (void);
    int       sidCreate      (sidbuilder *builder, sid2_model_t model,
                              sid2_model_t defaultModel);
    void      sidSamples     (bool enable);
    void      reset          ();
    uint8_t   iomap          (uint_least16_t addr);

    uint8_t readMemByte_plain     (uint_least16_t addr);
    uint8_t readMemByte_io        (uint_least16_t addr);
    uint8_t readMemByte_sidplaytp (uint_least16_t addr);
    uint8_t readMemByte_sidplaybs (uint_least16_t addr);
    void    writeMemByte_plain    (uint_least16_t addr, uint8_t data);
    void    writeMemByte_playsid  (uint_least16_t addr, uint8_t data);
    void    writeMemByte_sidplay  (uint_least16_t addr, uint8_t data);

    // Use pointers to please requirements of all the provided
    // environments.
    uint8_t (Player::*m_readMemByte)    (uint_least16_t);
    void    (Player::*m_writeMemByte)   (uint_least16_t, uint8_t);
    uint8_t (Player::*m_readMemDataByte)(uint_least16_t);

    uint8_t  readMemRamByte (uint_least16_t addr)
    {   return m_ram[addr]; }
    void sid2crc (uint8_t data);

    // Environment Function entry Points
    void           envReset           (bool safe);
    void           envReset           (void) { envReset (true); }
    inline uint8_t envReadMemByte     (uint_least16_t addr);
    inline void    envWriteMemByte    (uint_least16_t addr, uint8_t data);
    bool           envCheckBankJump   (uint_least16_t addr);
    inline uint8_t envReadMemDataByte (uint_least16_t addr);
    inline void    envSleep           (void);

    void   envLoadFile (char *file)
    {
        char name[0x100] = "E:/testsuite/";
        strcat (name, file);
        strcat (name, ".prg");
        m_tune->load (name);
        stop ();
    }

    // Rev 2.0.3 Added - New Mixer Routines
    uint_least32_t (Player::*output) (char *buffer);

    // Rev 2.0.4 (saw) - Added to reduce code size
    int_least32_t monoOutGenericLeftIn   (uint_least8_t  bits);
    int_least32_t monoOutGenericStereoIn (uint_least8_t  bits);
    int_least32_t monoOutGenericRightIn  (uint_least8_t bits);

    // 8 bit output
    uint_least32_t monoOut8MonoIn       (char *buffer);
    uint_least32_t monoOut8StereoIn     (char *buffer);
    uint_least32_t monoOut8StereoRIn    (char *buffer);
    uint_least32_t stereoOut8MonoIn     (char *buffer);
    uint_least32_t stereoOut8StereoIn   (char *buffer);

    // Rev 2.0.4 (jp) - Added 16 bit support
    uint_least32_t monoOut16MonoIn      (char *buffer);
    uint_least32_t monoOut16StereoIn    (char *buffer);
    uint_least32_t monoOut16StereoRIn   (char *buffer);
    uint_least32_t stereoOut16MonoIn    (char *buffer);
    uint_least32_t stereoOut16StereoIn  (char *buffer);

    inline void interruptIRQ (bool state);
    inline void interruptNMI (void);
    inline void interruptRST (void);
    void signalAEC (bool state) { cpu->aecSignal (state); }
    void lightpen  () { vic.lightpen (); }

    // PSID driver
    int  psidDrvReloc   (SidTuneInfo &tuneInfo, sid2_info_t &info);
    void psidDrvInstall (sid2_info_t &info);
    void psidRelocAddr  (SidTuneInfo &tuneInfo, int startp, int endp);

public:
    Player ();
    ~Player ();

    const sid2_config_t &config (void) const { return m_cfg; }
    const sid2_info_t   &info   (void) const { return m_info; }

    int            config       (const sid2_config_t &cfg);
    int            fastForward  (uint percent);
    int            load         (SidTune *tune);
    uint_least32_t mileage      (void) const { return m_mileage + time(); }
    void           pause        (void);
    uint_least32_t play         (void *buffer, uint_least32_t length);
    sid2_player_t  state        (void) const { return m_playerState; }
    void           stop         (void);
    uint_least32_t time         (void) const {return rtc.getTime (); }
    void           debug        (bool enable, FILE *out)
                                { cpu->debug (enable, out); }
    const char    *error        (void) const { return m_errorString; }

    SidTune      *m_tune;
};


uint8_t Player::envReadMemByte (uint_least16_t addr)
{   // Read from plain only to prevent execution of rom code
    return (this->*(m_readMemByte)) (addr);
}

void Player::envWriteMemByte (uint_least16_t addr, uint8_t data)
{   // Writes must be passed to env version.
    (this->*(m_writeMemByte)) (addr, data);
}

uint8_t Player::envReadMemDataByte (uint_least16_t addr)
{   // Read from plain only to prevent execution of rom code
    return (this->*(m_readMemDataByte)) (addr);
}

void Player::envSleep (void)
{
    if (m_info.environment != sid2_envR)
    {   // Start the sample sequence
        xsid.suppress (false);
        xsid.suppress (true);
    }
}

void Player::interruptIRQ (bool state)
{
    if (state)
    {
        if (m_info.environment == sid2_envR)
            cpu->triggerIRQ ();
        else
            fakeIRQ ();
    }
    else
        cpu->clearIRQ ();
}

void Player::interruptNMI ()
{
    cpu->triggerNMI ();
}

void Player::interruptRST ()
{
    stop ();
}

SIDPLAY2_NAMESPACE_STOP

#endif // _player_h_
