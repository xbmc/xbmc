/***************************************************************************
                          player.cpp  -  Main Library Code
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
 *  $Log: player.cpp,v $
 *  Revision 1.80  2004/05/26 19:42:44  s_a_white
 *  Fixed exceed c64 data memory check.
 *
 *  Revision 1.79  2004/05/24 23:11:15  s_a_white
 *  Fixed email addresses displayed to end user.
 *
 *  Revision 1.78  2004/05/20 22:37:38  s_a_white
 *  Protect ourselves from sid images that would exceed the top of memory.
 *
 *  Revision 1.77  2004/05/03 22:47:15  s_a_white
 *  Change how port handling is dealt with to provide better C64 compatiiblity.
 *  Add character rom support.
 *
 *  Revision 1.76  2004/03/20 23:07:45  s_a_white
 *  Don't initialise the lower memory areas as applying the poweron settings does
 *  that and they are optimised to assume all that memory is 0.
 *
 *  Revision 1.75  2004/03/20 22:56:34  s_a_white
 *  Use correct memory initialisation pattern in real c64 mode.
 *
 *  Revision 1.74  2004/03/20 16:13:49  s_a_white
 *  Remove sid volume reset to reduce DC clicks, potentially incompatibility
 *  added though.
 *
 *  Revision 1.73  2004/03/19 00:14:33  s_a_white
 *  Do waveform sync after reset.  This is because the internal clocks are wong
 *  once the scedular becomes reset.
 *
 *  Revision 1.72  2004/03/18 20:22:03  s_a_white
 *  Added sidmapper (so support more the 2 sids).
 *
 *  Revision 1.71  2004/02/11 20:59:17  s_a_white
 *  PACKAGE_NAME should be defined from PACKAGE not NAME.
 *
 *  Revision 1.70  2004/01/31 17:12:15  s_a_white
 *  Support new & old automake package defines.
 *
 *  Revision 1.69  2004/01/31 17:01:44  s_a_white
 *  Add ability to specify the maximum number of sid writes forming the sid2crc.
 *
 *  Revision 1.68  2004/01/30 19:37:14  s_a_white
 *  Support a compressed poweron data image (for use with psid64).
 *
 *  Revision 1.67  2003/12/15 23:49:38  s_a_white
 *  Fixup functions using inline and correct mileage calculations.
 *
 *  Revision 1.66  2003/10/01 22:30:59  s_a_white
 *  Fixed memory accesses to second sid.  Can be accessed by mirroring
 *  addresses that we must convert to the non mirrored address.
 *
 *  Revision 1.65  2003/07/16 20:56:54  s_a_white
 *  Some initialisation code from the psiddrv is required to start basic tunes.
 *
 *  Revision 1.64  2003/07/16 07:00:52  s_a_white
 *  Added support for c64 basic tunes.
 *
 *  Revision 1.63  2003/06/27 21:15:26  s_a_white
 *  Tidy up mono to stereo sid conversion, only allowing it theres sufficient
 *  support from the emulation.  Allow user to override whether they want this
 *  to happen.
 *
 *  Revision 1.62  2003/05/28 20:14:54  s_a_white
 *  Support the psiddrv overlapping unused parts of the tunes load image.
 *  Fix memory leak whereby m_ram/m_rom weren't being deleted on
 *  player object destruction.
 *
 *  Revision 1.60  2003/02/20 19:11:48  s_a_white
 *  sid2crc support.
 *
 *  Revision 1.59  2003/01/23 17:32:37  s_a_white
 *  Redundent code removal.
 *
 *  Revision 1.58  2003/01/17 08:35:26  s_a_white
 *  Simulate memory state after kernels power on reset code has completed.
 *
 *  Revision 1.57  2002/12/14 10:10:30  s_a_white
 *  Kernal Mod: Bypass screen clear as we don't have a screen.  Fix setting
 *  of PAL/NTSC flag for real c64 mode.
 *
 *  Revision 1.56  2002/12/13 22:01:54  s_a_white
 *  Kernel mods:  Memory now bypassed by upping the start address so the
 *  end or ram is found instantly.  Basic cold/warm start address points to a
 *  busy loop to prevent undersiable side effects (this may change later).
 *
 *  Revision 1.55  2002/11/27 00:16:51  s_a_white
 *  Make sure driver info gets reset and exported properly.
 *
 *  Revision 1.54  2002/11/25 21:09:41  s_a_white
 *  Reset address for old sidplay1 modes now directly passed to the CPU.  This
 *  prevents tune corruption and banking issues for the different modes.
 *
 *  Revision 1.53  2002/11/21 19:53:58  s_a_white
 *  CPU nolonger a special case.  It now uses the event scheduler like all the
 *  other components.
 *
 *  Revision 1.52  2002/11/20 21:44:03  s_a_white
 *  Fix fake IRQ to properly obtain next address.
 *
 *  Revision 1.51  2002/11/19 22:55:18  s_a_white
 *  Sidplay1 modes modified to make them nolonger require the psid driver.
 *  Full c64 kernal supported in real c64 mode.
 *
 *  Revision 1.50  2002/11/01 17:36:01  s_a_white
 *  Frame based support for old sidplay1 modes.
 *
 *  Revision 1.49  2002/10/20 08:58:36  s_a_white
 *  Modify IO map so psiddrv can detect special cases.
 *
 *  Revision 1.48  2002/10/15 18:20:54  s_a_white
 *  Make all addresses from ea31 to ea7d valid IRQ exit points.  This
 *  approximates the functionality of a real C64.
 *
 *  Revision 1.47  2002/10/02 19:43:47  s_a_white
 *  RSID support.
 *
 *  Revision 1.46  2002/09/17 17:02:41  s_a_white
 *  Fixed location of kernel IRQ exit code.
 *
 *  Revision 1.45  2002/09/12 21:01:30  s_a_white
 *  Added support for simulating the random delay before the user loads a
 *  program on a real C64.
 *
 *  Revision 1.44  2002/09/09 18:01:30  s_a_white
 *  Prevent m_info driver details getting modified when C64 crashes.
 *
 *  Revision 1.43  2002/08/20 23:21:41  s_a_white
 *  Setup default sample format.
 *
 *  Revision 1.42  2002/04/14 21:46:50  s_a_white
 *  PlaySID reads fixed to come from RAM only.
 *
 *  Revision 1.41  2002/03/12 18:43:59  s_a_white
 *  Tidy up handling of envReset on illegal CPU instructions.
 *
 *  Revision 1.40  2002/03/11 18:01:30  s_a_white
 *  Prevent lockup if config call fails with existing and old configurations.
 *
 *  Revision 1.39  2002/03/03 22:01:58  s_a_white
 *  New clock speed & sid model interface.
 *
 *  Revision 1.38  2002/02/17 16:33:02  s_a_white
 *  New reset interface for sidbuilders.
 *
 *  Revision 1.37  2002/02/17 12:42:45  s_a_white
 *  envReset now sets playerStopped indicators.  This means the player
 *  nolonger locks up when a HLT instruction is encountered.
 *
 *  Revision 1.36  2002/02/06 20:12:03  s_a_white
 *  Added sidplay1 random extension for vic reads.
 *
 *  Revision 1.35  2002/01/29 21:50:33  s_a_white
 *  Auto switching to a better emulation mode.  m_tuneInfo reloaded after a
 *  config.  Initial code added to support more than two sids.
 *
 *  Revision 1.34  2002/01/14 23:16:27  s_a_white
 *  Prevent multiple initialisations if already stopped.
 *
 *  Revision 1.33  2001/12/13 08:28:08  s_a_white
 *  Added namespace support to fix problems with xsidplay.
 *
 *  Revision 1.32  2001/11/16 19:25:33  s_a_white
 *  Removed m_context as where getting mixed with parent class.
 *
 *  Revision 1.31  2001/10/18 22:33:40  s_a_white
 *  Initialisation order fixes.
 *
 *  Revision 1.30  2001/10/02 18:26:36  s_a_white
 *  Removed ReSID support and updated for new scheduler.
 *
 *  Revision 1.29  2001/09/17 19:02:38  s_a_white
 *  Now uses fixed point maths for sample output and rtc.
 *
 *  Revision 1.28  2001/09/04 18:50:57  s_a_white
 *  Fake CIA address now masked.
 *
 *  Revision 1.27  2001/09/01 11:15:46  s_a_white
 *  Fixes sidplay1 environment modes.
 *
 *  Revision 1.26  2001/08/10 20:04:46  s_a_white
 *  Initialise requires rtc reset for correct use with stop operation.
 *
 *  Revision 1.25  2001/07/27 12:51:55  s_a_white
 *  Removed warning.
 *
 *  Revision 1.24  2001/07/25 17:01:13  s_a_white
 *  Support for new configuration interface.
 *
 *  Revision 1.23  2001/07/14 16:46:16  s_a_white
 *  Sync with sidbuilder class project.
 *
 *  Revision 1.22  2001/07/14 12:56:15  s_a_white
 *  SID caching no longer needed. IC  components now run using event
 *  generation (based on VICE).  Handling of IRQs now more effecient.  All
 *  sidplay1 hacks either removed or moved to sid6510.  Fixed PAL/NTSC
 *  speeding fixing.  Now uses new component and sidbuilder classes.
 *
 *  Revision 1.21  2001/04/23 17:09:56  s_a_white
 *  Fixed video speed selection using unforced/forced and NTSC clockSpeeds.
 *
 *  Revision 1.20  2001/03/26 21:46:43  s_a_white
 *  Removed unused #include.
 *
 *  Revision 1.19  2001/03/25 19:48:13  s_a_white
 *  xsid.reset added.
 *
 *  Revision 1.18  2001/03/22 22:45:20  s_a_white
 *  Re-ordered initialisations to match defintions.
 *
 *  Revision 1.17  2001/03/21 22:32:34  s_a_white
 *  Filter redefinition support.  VIC & NMI support added.  Moved fake interrupts
 *  to sid6510 class.
 *
 *  Revision 1.16  2001/03/09 22:26:36  s_a_white
 *  Support for updated C64 player.
 *
 *  Revision 1.15  2001/03/08 22:46:42  s_a_white
 *  playAddr = 0xffff now better supported.
 *
 *  Revision 1.14  2001/03/01 23:46:37  s_a_white
 *  Support for sample mode to be selected at runtime.
 *
 *  Revision 1.13  2001/02/28 18:55:27  s_a_white
 *  Removed initBank* related stuff.  IRQ terminating ROM jumps at 0xea31,
 *  0xea7e and 0xea81 now handled.
 *
 *  Revision 1.12  2001/02/21 21:43:10  s_a_white
 *  Now use VSID code and this handles interrupts much better!  The whole
 *  initialise sequence has been modified to support this.
 *
 *  Revision 1.11  2001/02/13 21:01:14  s_a_white
 *  Support for real interrupts.  C64 Initialisation routine now run from Player::play
 *  instead of Player::initialise.  Prevents lockups if init routine does not return.
 *
 *  Revision 1.10  2001/02/08 17:21:14  s_a_white
 *  Initial SID volumes not being stored in cache.  Fixes Dulcedo Cogitationis.
 *
 *  Revision 1.9  2001/02/07 20:56:46  s_a_white
 *  Samples now delayed until end of simulated frame.
 *
 *  Revision 1.8  2001/01/23 21:26:28  s_a_white
 *  Only way to load a tune now is by passing in a sidtune object.  This is
 *  required for songlength database support.
 *
 *  Revision 1.7  2001/01/07 15:13:39  s_a_white
 *  Hardsid update to mute sids when program exits.
 *
 *  Revision 1.6  2000/12/21 22:48:27  s_a_white
 *  Re-order voices for mono to stereo conversion to match sidplay1.
 *
 *  Revision 1.5  2000/12/14 23:53:36  s_a_white
 *  Small optimisation update, and comment revision.
 *
 *  Revision 1.4  2000/12/13 17:56:24  s_a_white
 *  Interrupt vector address changed from 0x315 to 0x314.
 *
 *  Revision 1.3  2000/12/13 12:00:25  mschwendt
 *  Corrected order of members in member initializer-list.
 *
 *  Revision 1.2  2000/12/12 22:50:15  s_a_white
 *  Bug Fix #122033.
 *
 ***************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "config.h"
#include "sidendian.h"
#include "player.h"

#ifdef HAVE_EXCEPTIONS
#   include <new>
#endif

#ifndef PACKAGE_NAME
#   define PACKAGE_NAME PACKAGE
#endif

#ifndef PACKAGE_VERSION
#   define PACKAGE_VERSION VERSION
#endif


SIDPLAY2_NAMESPACE_START

static const uint8_t kernal[] = {
#include "kernal.bin"
};

static const uint8_t character[] = {
#include "char.bin"
};

static const uint8_t basic[] = {
#include "basic.bin"
};

static const uint8_t poweron[] = {
#include "poweron.bin"
};

const double Player::CLOCK_FREQ_NTSC = 1022727.14;
const double Player::CLOCK_FREQ_PAL  = 985248.4;
const double Player::VIC_FREQ_PAL    = 50.0;
const double Player::VIC_FREQ_NTSC   = 60.0;

// These texts are used to override the sidtune settings.
const char  *Player::TXT_PAL_VBI        = "50 Hz VBI (PAL)";
const char  *Player::TXT_PAL_VBI_FIXED  = "60 Hz VBI (PAL FIXED)";
const char  *Player::TXT_PAL_CIA        = "CIA (PAL)";
const char  *Player::TXT_PAL_UNKNOWN    = "UNKNOWN (PAL)";
const char  *Player::TXT_NTSC_VBI       = "60 Hz VBI (NTSC)";
const char  *Player::TXT_NTSC_VBI_FIXED = "50 Hz VBI (NTSC FIXED)";
const char  *Player::TXT_NTSC_CIA       = "CIA (NTSC)";
const char  *Player::TXT_NTSC_UNKNOWN   = "UNKNOWN (NTSC)";
const char  *Player::TXT_NA             = "NA";

// Error Strings
const char  *Player::ERR_CONF_WHILST_ACTIVE    = "SIDPLAYER ERROR: Trying to configure player whilst active.";
const char  *Player::ERR_UNSUPPORTED_FREQ      = "SIDPLAYER ERROR: Unsupported sampling frequency.";
const char  *Player::ERR_UNSUPPORTED_PRECISION = "SIDPLAYER ERROR: Unsupported sample precision.";
const char  *Player::ERR_MEM_ALLOC             = "SIDPLAYER ERROR: Memory Allocation Failure.";
const char  *Player::ERR_UNSUPPORTED_MODE      = "SIDPLAYER ERROR: Unsupported Environment Mode (Coming Soon).";

const char  *Player::credit[];


// Set the ICs environment variable to point to
// this player
Player::Player (void)
// Set default settings for system
:c64env  (&m_scheduler),
 m_scheduler ("SIDPlay 2"),
 sid6510 (&m_scheduler),
 mos6510 (&m_scheduler),
 cpu     (&sid6510),
 xsid    (this, &nullsid),
 cia     (this),
 cia2    (this),
 sid6526 (this),
 vic     (this),
 mixerEvent (this),
 rtc        (&m_scheduler),
 m_tune (NULL),
 m_ram  (NULL),
 m_rom  (NULL),
 m_errorString       (TXT_NA),
 m_fastForwardFactor (1.0),
 m_mileage           (0),
 m_playerState       (sid2_stopped),
 m_running           (false),
 m_sid2crc           (0xffffffff),
 m_sid2crcCount      (0),
 m_emulateStereo     (true),
 m_sampleCount       (0)
{
    srand ((uint) ::time(NULL));
    m_rand = (uint_least32_t) rand ();
    
    // Set the ICs to use this environment
    sid6510.setEnvironment (this);
    mos6510.setEnvironment (this);

    // SID Initialise
    {for (int i = 0; i < SID2_MAX_SIDS; i++)
        sid[i] = &nullsid;
	}
    xsid.emulation(sid[0]);
    sid[0] = &xsid;
    // Setup sid mapping table
    {for (int i = 0; i < SID2_MAPPER_SIZE; i++)
        m_sidmapper[i] = 0;
	}

    // Setup exported info
    m_info.credits         = credit;
    m_info.channels        = 1;
    m_info.driverAddr      = 0;
    m_info.driverLength    = 0;
    m_info.name            = PACKAGE_NAME;
    m_info.tuneInfo        = NULL;
    m_info.version         = PACKAGE_VERSION;
    m_info.eventContext    = &context();
    // Number of SIDs support by this library
    m_info.maxsids         = SID2_MAX_SIDS;
    m_info.environment     = sid2_envR;
    m_info.sid2crc         = 0;
    m_info.sid2crcCount    = 0;

    // Configure default settings
    m_cfg.clockDefault    = SID2_CLOCK_CORRECT;
    m_cfg.clockForced     = false;
    m_cfg.clockSpeed      = SID2_CLOCK_CORRECT;
    m_cfg.environment     = m_info.environment;
    m_cfg.forceDualSids   = false;
    m_cfg.emulateStereo   = m_emulateStereo;
    m_cfg.frequency       = 48000;
    m_cfg.optimisation    = SID2_DEFAULT_OPTIMISATION;
    m_cfg.playback        = sid2_stereo;
    m_cfg.precision       = SID2_DEFAULT_PRECISION;
    m_cfg.sidDefault      = SID2_MODEL_CORRECT;
    m_cfg.sidEmulation    = NULL;
    m_cfg.sidModel        = SID2_MODEL_CORRECT;
    m_cfg.sidSamples      = true;
    m_cfg.leftVolume      = 255;
    m_cfg.rightVolume     = 255;
    m_cfg.sampleFormat    = SID2_LITTLE_SIGNED;
    m_cfg.powerOnDelay    = SID2_DEFAULT_POWER_ON_DELAY;
    m_cfg.sid2crcCount    = 0;

    // Configured by default for Sound Blaster (compatibles)
    if (SID2_DEFAULT_PRECISION == 8)
        m_cfg.sampleFormat = SID2_LITTLE_UNSIGNED;
    config (m_cfg);

    // Get component credits
    credit[0] = PACKAGE_NAME " V" PACKAGE_VERSION " Engine:\0\tCopyright (C) 2000 Simon White <" S_A_WHITE_EMAIL ">\0"
                "\thttp://sidplay2.sourceforge.net\0";
    credit[1] = xsid.credits ();
    credit[2] = "*MOS6510 (CPU) Emulation:\0\tCopyright (C) 2000 Simon White <" S_A_WHITE_EMAIL ">\0";
    credit[3] = cia.credits ();
    credit[4] = vic.credits ();
    credit[5] = NULL;
}

Player::~Player ()
{
   if (m_ram == m_rom)
      delete [] m_ram;
   else
   {
      delete [] m_rom;
      delete [] m_ram;
   }
}

// Makes the next sequence of notes available.  For sidplay compatibility
// this function should be called from interrupt event
void Player::fakeIRQ (void)
{   // Check to see if the play address has been provided or whether
    // we should pick it up from an IRQ vector
    uint_least16_t playAddr = m_tuneInfo.playAddr;

    // We have to reload the new play address
    if (playAddr)
        evalBankSelect (m_playBank);
    else
    {
        if (isKernal)
        {   // Setup the entry point from hardware IRQ
            playAddr = endian_little16 (&m_ram[0x0314]);
        }
        else
        {   // Setup the entry point from software IRQ
            playAddr = endian_little16 (&m_ram[0xFFFE]);
        }
    }

    // Setup the entry point and restart the cpu
    cpu->triggerIRQ ();
    sid6510.reset   (playAddr, 0, 0, 0);
}

int Player::fastForward (uint percent)
{
    if (percent > 3200)
    {
        m_errorString = "SIDPLAYER ERROR: Percentage value out of range";
        return -1;
    }
    {
        float64_t fastForwardFactor;
        fastForwardFactor   = (float64_t) percent / 100.0;
        // Conversion to fixed point 8.24
        m_samplePeriod      = (event_clock_t) ((float64_t) m_samplePeriod /
                              m_fastForwardFactor * fastForwardFactor);
        m_fastForwardFactor = fastForwardFactor;
    }
    return 0;
}

int Player::initialise ()
{   // Fix the mileage counter if just finished another song.
    mileageCorrect ();
    m_mileage += time ();

    reset ();

    {
        uint_least32_t page = ((uint_least32_t) m_tuneInfo.loadAddr
                            + m_tuneInfo.c64dataLen - 1) >> 8;
        if (page > 0xff)
        {
            m_errorString = "SIDPLAYER ERROR: Size of music data exceeds C64 memory.";
            return -1;
        }
    }

    if (psidDrvReloc (m_tuneInfo, m_info) < 0)
        return -1;

    // The Basic ROM sets these values on loading a file.
    {   // Program end address
        uint_least16_t start = m_tuneInfo.loadAddr;
        uint_least16_t end   = start + m_tuneInfo.c64dataLen;
        endian_little16 (&m_ram[0x2d], end); // Variables start
        endian_little16 (&m_ram[0x2f], end); // Arrays start
        endian_little16 (&m_ram[0x31], end); // Strings start
        endian_little16 (&m_ram[0xac], start);
        endian_little16 (&m_ram[0xae], end);
    }

    if (!m_tune->placeSidTuneInC64mem (m_ram))
    {   // Rev 1.6 (saw) - Allow loop through errors
        m_errorString = m_tuneInfo.statusString;
        return -1;
    }

    psidDrvInstall (m_info);
    rtc.reset ();
    envReset  (false);
    return 0;
}

int Player::load (SidTune *tune)
{
    if (!tune)
    {   // Unload tune
        m_info.tuneInfo = NULL;
        return 0;
    }
    m_tune = tune;
    m_info.tuneInfo = &m_tuneInfo;

    // Un-mute all voices
    xsid.mute (false);

    for (int i = 0; i < SID2_MAX_SIDS; i++)
    {
        uint_least8_t v = 3;
        while (v--)
            sid[i]->voice (v, 0, false);
    }

    {   // Must re-configure on fly for stereo support!
        int ret = config (m_cfg);
        // Failed configuration with new tune, reject it
        if (ret < 0)
        {
            m_tune = NULL;
            return -1;
        }
    }
    return 0;
}

void Player::mileageCorrect (void)
{   // Calculate 1 bit below the timebase so we can round the
    // mileage count
    if (((m_sampleCount * 2 * SID2_TIME_BASE) / m_cfg.frequency) & 1)
        m_mileage++;
    m_sampleCount = 0;
}

void Player::pause (void)
{
    if (m_running)
    {
        m_playerState = sid2_paused;
        m_running     = false;
    }
}

uint_least32_t Player::play (void *buffer, uint_least32_t length)
{
    // Make sure a _tune is loaded
    if (!m_tune)
    {
      printf("no tune!");  
      return 0;
    }

    // Setup Sample Information
    m_sampleIndex  = 0;
    m_sampleCount  = length;
    m_sampleBuffer = (char *) buffer;

    // Start the player loop
    m_playerState = sid2_playing;
    m_running     = true;

    while (m_running)
        m_scheduler.clock ();

    if (m_playerState == sid2_stopped)
        initialise ();

    return m_sampleIndex;
}

void Player::stop (void)
{   // Re-start song
    if (m_tune && (m_playerState != sid2_stopped))
    {
        if (!m_running)
            initialise ();
        else
        {
            m_playerState = sid2_stopped;
            m_running     = false;
        }
    }
}


//-------------------------------------------------------------------------
// Temporary hack till real bank switching code added

//  Input: A 16-bit effective address
// Output: A default bank-select value for $01.
uint8_t Player::iomap (uint_least16_t addr)
{
    if (m_info.environment != sid2_envPS)
    {   // Force Real C64 Compatibility
        switch (m_tuneInfo.compatibility)
        {
        case SIDTUNE_COMPATIBILITY_R64:
        case SIDTUNE_COMPATIBILITY_BASIC:
            return 0;     // Special case, converted to 0x37 later
        }

        if (addr == 0)
            return 0;     // Special case, converted to 0x37 later
        if (addr < 0xa000)
            return 0x37;  // Basic-ROM, Kernal-ROM, I/O
        if (addr  < 0xd000)
            return 0x36;  // Kernal-ROM, I/O
        if (addr >= 0xe000)
            return 0x35;  // I/O only
    }
    return 0x34;  // RAM only (special I/O in PlaySID mode)
}

void Player::evalBankSelect (uint8_t data)
{   // Determine new memory configuration.
    m_port.pr_out = data;
    m_port.pr_in  = (data & m_port.ddr) | (~m_port.ddr & (m_port.pr_in | 0x17) & 0xdf);
    data     |= ~m_port.ddr;
    data     &= 7;
    isBasic   = ((data & 3) == 3);
    isIO      = (data >  4);
    isKernal  = ((data & 2) != 0);
    isChar    = ((data ^ 4) > 4);
}

uint8_t Player::readMemByte_plain (uint_least16_t addr)
{   // Bank Select Register Value DOES NOT get to ram
    if (addr > 1)
        return m_ram[addr];
    else if (addr)
        return m_port.pr_in;
    return m_port.ddr;
}

uint8_t Player::readMemByte_io (uint_least16_t addr)
{
    uint_least16_t tempAddr = (addr & 0xfc1f);

    // Not SID ?
    if (( tempAddr & 0xff00 ) != 0xd400 )
    {
        if (m_info.environment == sid2_envR)
        {
            switch (endian_16hi8 (addr))
            {
            case 0:
            case 1:
                return readMemByte_plain (addr);
            case 0xdc:
                return cia.read (addr&0x0f);
            case 0xdd:
                return cia2.read (addr&0x0f);
            case 0xd0:
            case 0xd1:
            case 0xd2:
            case 0xd3:
                return vic.read (addr&0x3f);
            default:
                return m_rom[addr];
            }
        }
        else
        {
            switch (endian_16hi8 (addr))
            {
            case 0:
            case 1:
                return readMemByte_plain (addr);
            // Sidplay1 Random Extension CIA
            case 0xdc:
                return sid6526.read (addr&0x0f);
            // Sidplay1 Random Extension VIC 
            case 0xd0:
                switch (addr & 0x3f)
                {
                case 0x11:
                case 0x12:
                    return sid6526.read ((addr-13)&0x0f);
                }
                // Deliberate run on
            default:
                return m_rom[addr];
            }
        }
    }

    {   // Read real sid for these
        int i = m_sidmapper[(addr >> 5) & (SID2_MAPPER_SIZE - 1)];
        return sid[i]->read (tempAddr & 0xff);
    }
}

uint8_t Player::readMemByte_sidplaytp(uint_least16_t addr)
{
    if (addr < 0xD000)
        return readMemByte_plain (addr);
    else
    {
        // Get high-nibble of address.
        switch (addr >> 12)
        {
        case 0xd:
            if (isIO)
                return readMemByte_io (addr);
            else
                return m_ram[addr];
        break;
        case 0xe:
        case 0xf:
        default:  // <-- just to please the compiler
              return m_ram[addr];
        }
    }
}
        
uint8_t Player::readMemByte_sidplaybs (uint_least16_t addr)
{
    if (addr < 0xA000)
        return readMemByte_plain (addr);
    else
    {
        // Get high-nibble of address.
        switch (addr >> 12)
        {
        case 0xa:
        case 0xb:
            if (isBasic)
                return m_rom[addr];
            else
                return m_ram[addr];
        break;
        case 0xc:
            return m_ram[addr];
        break;
        case 0xd:
            if (isIO)
                return readMemByte_io (addr);
            else if (isChar)
                return m_rom[addr];
            else
                return m_ram[addr];
        break;
        case 0xe:
        case 0xf:
        default:  // <-- just to please the compiler
            if (isKernal)
                return m_rom[addr];
            else
                return m_ram[addr];
        }
    }
}

void Player::writeMemByte_plain (uint_least16_t addr, uint8_t data)
{
    if (addr > 1)
        m_ram[addr] = data;
    else if (addr)
    {   // Determine new memory configuration.
        evalBankSelect (data);
    }
    else
    {
        m_port.ddr = data;
        evalBankSelect (m_port.pr_out);
    }
}

void Player::writeMemByte_playsid (uint_least16_t addr, uint8_t data)
{
    uint_least16_t tempAddr = (addr & 0xfc1f);

    // Not SID ?
    if (( tempAddr & 0xff00 ) != 0xd400 )
    {
        if (m_info.environment == sid2_envR)
        {
            switch (endian_16hi8 (addr))
            {
            case 0:
            case 1:
                writeMemByte_plain (addr, data);
            return;
            case 0xdc:
                cia.write (addr&0x0f, data);
            return;
            case 0xdd:
                cia2.write (addr&0x0f, data);
            return;
            case 0xd0:
            case 0xd1:
            case 0xd2:
            case 0xd3:
                vic.write (addr&0x3f, data);
            return;
            default:
                m_rom[addr] = data;
            return;
            }
        }
        else
        {
            switch (endian_16hi8 (addr))
            {
            case 0:
            case 1:
                writeMemByte_plain (addr, data);
            return;
            case 0xdc: // Sidplay1 CIA
                sid6526.write (addr&0x0f, data);
            return;
            default:
                m_rom[addr] = data;
            return;
            }
        }
    }

    // $D41D/1E/1F, $D43D/3E/3F, ...
    // Map to real address to support PlaySID
    // Extended SID Chip Registers.
    sid2crc (data);
    if (( tempAddr & 0x00ff ) >= 0x001d )
        xsid.write16 (addr & 0x01ff, data);
    else // Mirrored SID.
    {
        int i = m_sidmapper[(addr >> 5) & (SID2_MAPPER_SIZE - 1)];
        // Convert address to that acceptable by resid
        sid[i]->write (tempAddr & 0xff, data);
        // Support dual sid
        if (m_emulateStereo)
            sid[1]->write (tempAddr & 0xff, data);
    }
}

void Player::writeMemByte_sidplay (uint_least16_t addr, uint8_t data)
{
    if (addr < 0xA000)
        writeMemByte_plain (addr, data);
    else
    {
        // Get high-nibble of address.
        switch (addr >> 12)
        {
        case 0xa:
        case 0xb:
        case 0xc:
            m_ram[addr] = data;
        break;
        case 0xd:
            if (isIO)
                writeMemByte_playsid (addr, data);
            else
                m_ram[addr] = data;
        break;
        case 0xe:
        case 0xf:
        default:  // <-- just to please the compiler
            m_ram[addr] = data;
        }
    }
}

// --------------------------------------------------
// These must be available for use:
void Player::reset (void)
{
    int i;

    m_playerState  = sid2_stopped;
    m_running      = false;
    m_sid2crc      = 0xffffffff;
    m_info.sid2crc = m_sid2crc ^ 0xffffffff;    
    m_sid2crcCount = m_info.sid2crcCount = 0;

    // Select Sidplay1 compatible CPU or real thing
    cpu = &sid6510;
    sid6510.environment (m_info.environment);

    m_scheduler.reset ();
    for (i = 0; i < SID2_MAX_SIDS; i++)
    {
        sidemu &s = *sid[i];
        s.reset (0x0f);
        // Synchronise the waveform generators
        // (must occur after reset)
        s.write (0x04, 0x08);
        s.write (0x0b, 0x08);
        s.write (0x12, 0x08);
        s.write (0x04, 0x00);
        s.write (0x0b, 0x00);
        s.write (0x12, 0x00);
    }

    if (m_info.environment == sid2_envR)
    {
        cia.reset  ();
        cia2.reset ();
        vic.reset  ();
    }
    else
    {
        sid6526.reset (m_cfg.powerOnDelay <= SID2_MAX_POWER_ON_DELAY);
        sid6526.write (0x0e, 1); // Start timer
        if (m_tuneInfo.songSpeed == SIDTUNE_SPEED_VBI)
            sid6526.lock ();
    }

    // Initalise Memory
    m_port.pr_in = 0;
    memset (m_ram, 0, 0x10000);
    switch (m_info.environment)
    {
    case sid2_envPS:
        break;
    case sid2_envR:
    {   // Initialize RAM with powerup pattern
        for (int i=0x07c0; i<0x10000; i+=128)
            memset (m_ram+i, 0xff, 64);
        memset (m_rom, 0, 0x10000);
        break;
    }
    default:
        memset (m_rom, 0, 0x10000);
        memset (m_rom + 0xA000, RTSn, 0x2000);
    }

    if (m_info.environment == sid2_envR)
    {
        memcpy (&m_rom[0xe000], kernal, sizeof (kernal));
        memcpy (&m_rom[0xd000], character, sizeof (character));
        m_rom[0xfd69] = 0x9f; // Bypass memory check
        m_rom[0xe55f] = 0x00; // Bypass screen clear
        m_rom[0xfdc4] = 0xea; // Ingore sid volume reset to avoid DC
        m_rom[0xfdc5] = 0xea; // click (potentially incompatibility)!!
        m_rom[0xfdc6] = 0xea;
        if (m_tuneInfo.compatibility == SIDTUNE_COMPATIBILITY_BASIC)
            memcpy (&m_rom[0xa000], basic, sizeof (basic));

        // Copy in power on settings.  These were created by running
        // the kernel reset routine and storing the usefull values
        // from $0000-$03ff.  Format is:
        // -offset byte (bit 7 indicates presence rle byte)
        // -rle count byte (bit 7 indicates compression used)
        // data (single byte) or quantity represented by uncompressed count
        // -all counts and offsets are 1 less than they should be
        //if (m_tuneInfo.compatibility >= SIDTUNE_COMPATIBILITY_R64)
        {
            uint_least16_t addr = 0;
            for (int i = 0; i < sizeof (poweron);)
            {
                uint8_t off   = poweron[i++];
                uint8_t count = 0;
                bool compressed = false;

                // Determine data count/compression
                if (off & 0x80)
                {   // fixup offset
                    off  &= 0x7f;
                    count = poweron[i++];
                    if (count & 0x80)
                    {   // fixup count
                        count &= 0x7f;
                        compressed = true;
                    }
                }

                // Fix count off by ones (see format details)
                count++;
                addr += off;

                // Extract compressed data
                if (compressed)
                {
                    uint8_t data = poweron[i++];
                    while (count-- > 0)
                        m_ram[addr++] = data;
                }
                // Extract uncompressed data
                else
                {
                    while (count-- > 0)
                        m_ram[addr++] = poweron[i++];
                }
            }
        }
    }
    else // !sid2_envR
    {
        memset (m_rom + 0xE000, RTSn, 0x2000);    
        // fake VBI-interrupts that do $D019, BMI ...
        m_rom[0x0d019] = 0xff;
        if (m_info.environment == sid2_envPS)
        {
            m_ram[0xff48] = JMPi;
            endian_little16 (&m_ram[0xff49], 0x0314);
        }

        // Software vectors
        endian_little16 (&m_ram[0x0314], 0xEA31); // IRQ
        endian_little16 (&m_ram[0x0316], 0xFE66); // BRK
        endian_little16 (&m_ram[0x0318], 0xFE47); // NMI
        // Hardware vectors
        if (m_info.environment == sid2_envPS)
            endian_little16 (&m_rom[0xfffa], 0xFFFA); // NMI
        else
            endian_little16 (&m_rom[0xfffa], 0xFE43); // NMI
        endian_little16 (&m_rom[0xfffc], 0xFCE2); // RESET
        endian_little16 (&m_rom[0xfffe], 0xFF48); // IRQ
        memcpy (&m_ram[0xfffa], &m_rom[0xfffa], 6);
    }

    // Will get done later if can't now
    if (m_tuneInfo.clockSpeed == SIDTUNE_CLOCK_PAL)
        m_ram[0x02a6] = 1;
    else // SIDTUNE_CLOCK_NTSC
        m_ram[0x02a6] = 0;
}

// This resets the cpu once the program is loaded to begin
// running. Also called when the emulation crashes
void Player::envReset (bool safe)
{
    if (safe)
    {   // Emulation crashed so run in safe mode
        if (m_info.environment == sid2_envR)
        {
            uint8_t prg[] = {LDAb, 0x7f, STAa, 0x0d, 0xdc, RTSn};
            sid2_info_t info;
            SidTuneInfo tuneInfo;
            // Install driver
            tuneInfo.relocStartPage = 0x09;
            tuneInfo.relocPages     = 0x20;
            tuneInfo.initAddr       = 0x0800;
            tuneInfo.songSpeed      = SIDTUNE_SPEED_CIA_1A;
            info.environment        = m_info.environment;
            psidDrvReloc (tuneInfo, info);
            // Install prg & driver
            memcpy (&m_ram[0x0800], prg, sizeof (prg));
            psidDrvInstall (info);
        }
        else
        {   // If theres no irqs, song wont continue
            sid6526.reset ();
        }

        // Make sids silent
        for (int i = 0; i < SID2_MAX_SIDS; i++)
            sid[i]->reset (0);
    }

    m_port.ddr = 0x2F;

    // defaults: Basic-ROM on, Kernal-ROM on, I/O on
    if (m_info.environment != sid2_envR)
    {
        uint8_t song = m_tuneInfo.currentSong - 1;
        uint8_t bank = iomap (m_tuneInfo.initAddr);
        evalBankSelect (bank);
        m_playBank = iomap (m_tuneInfo.playAddr);
        if (m_info.environment != sid2_envPS)
            sid6510.reset (m_tuneInfo.initAddr, song, 0, 0);
        else
            sid6510.reset (m_tuneInfo.initAddr, song, song, song);
    }
    else
    {
        evalBankSelect (0x37);
        cpu->reset ();
    }

    mixerReset ();
    xsid.suppress (true);
}

bool Player::envCheckBankJump (uint_least16_t addr)
{
    switch (m_info.environment)
    {
    case sid2_envBS:
        if (addr >= 0xA000)
        {
            // Get high-nibble of address.
            switch (addr >> 12)
            {
            case 0xa:
            case 0xb:
                if (isBasic)
                    return false;
            break;

            case 0xc:
            break;

            case 0xd:
                if (isIO)
                    return false;
            break;

            case 0xe:
            case 0xf:
            default:  // <-- just to please the compiler
               if (isKernal)
                    return false;
            break;
            }
        }
    break;

    case sid2_envTP:
        if ((addr >= 0xd000) && isKernal)
            return false;
    break;

    default:
    break;
    }

    return true;
}

// Used for sid2crc (tracking sid register writes)
static const uint_least32_t crc32Table[0x100] =
{
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA,
    0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
    0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
    0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
    0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE,
    0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC,
    0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
    0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
    0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
    0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940,
    0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116,
    0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
    0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
    0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
    0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A,
    0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818,
    0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
    0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
    0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
    0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C,
    0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2,
    0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
    0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
    0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
    0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086,
    0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4,
    0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
    0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
    0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
    0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8,
    0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE,
    0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
    0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
    0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
    0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252,
    0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60,
    0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
    0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
    0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
    0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04,
    0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A,
    0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
    0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
    0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
    0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E,
    0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
    0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C,
    0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
    0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
    0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
    0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0,
    0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6,
    0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
    0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
    0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
};

void Player::sid2crc (uint8_t data)
{
    if (m_sid2crcCount < m_cfg.sid2crcCount)
    {
        m_info.sid2crcCount = ++m_sid2crcCount;
        m_sid2crc = (m_sid2crc >> 8) ^ crc32Table[(m_sid2crc & 0xFF) ^ data];
        m_info.sid2crc = m_sid2crc ^ 0xffffffff;
    }
}

SIDPLAY2_NAMESPACE_STOP
