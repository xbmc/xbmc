/***************************************************************************
                          sid2types.h  -  sidplay2 specific types
                             -------------------
    begin                : Fri Aug 10 2001
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

#ifndef _sid2types_h_
#define _sid2types_h_

#include "sidtypes.h"
#include "event.h"

class   sidbuilder;
struct  SidTuneInfo;

#ifndef SIDPLAY2_DEFAULTS
#define SIDPLAY2_DEFAULTS
    // Maximum values
    const uint_least8_t  SID2_MAX_PRECISION      = 16;
    const uint_least8_t  SID2_MAX_OPTIMISATION   = 2;
    // Delays <= MAX produce constant results.
    // Delays >  MAX produce random results
    const uint_least16_t SID2_MAX_POWER_ON_DELAY = 0x1FFF;
    // Default settings
    const uint_least32_t SID2_DEFAULT_SAMPLING_FREQ  = 44100;
    const uint_least8_t  SID2_DEFAULT_PRECISION      = 16;
    const uint_least8_t  SID2_DEFAULT_OPTIMISATION   = 1;
    const bool           SID2_DEFAULT_SID_SAMPLES    = true; // Samples through sid
    const uint_least16_t SID2_DEFAULT_POWER_ON_DELAY = SID2_MAX_POWER_ON_DELAY + 1;
#endif // SIDPLAY2_DEFAULTS

typedef enum {sid2_playing = 0, sid2_paused, sid2_stopped}         sid2_player_t;
typedef enum {sid2_left  = 0, sid2_mono,  sid2_stereo, sid2_right} sid2_playback_t;
typedef enum {sid2_envPS = 0, sid2_envTP, sid2_envBS,  sid2_envR,
              sid2_envTR} sid2_env_t;
typedef enum {SID2_MODEL_CORRECT, SID2_MOS6581, SID2_MOS8580}      sid2_model_t;
typedef enum {SID2_CLOCK_CORRECT, SID2_CLOCK_PAL, SID2_CLOCK_NTSC} sid2_clock_t;

typedef enum
{   // Soundcard sample format
    SID2_LITTLE_SIGNED,
    SID2_LITTLE_UNSIGNED,
    SID2_BIG_SIGNED,
    SID2_BIG_UNSIGNED
} sid2_sample_t;

/* Environment Modes
sid2_envPS = Playsid
sid2_envTP = Sidplay  - Transparent Rom
sid2_envBS = Sidplay  - Bankswitching
sid2_envR  = Sidplay2 - Real C64 Environment
*/

struct sid2_config_t
{
    sid2_clock_t        clockDefault;  // Intended tune speed when unknown
    bool                clockForced;
    sid2_clock_t        clockSpeed;    // User requested emulation speed
    sid2_env_t          environment;
    bool                forceDualSids;
    bool                emulateStereo;
    uint_least32_t      frequency;
    uint_least8_t       optimisation;
    sid2_playback_t     playback;
    int                 precision;
    sid2_model_t        sidDefault;    // Intended sid model when unknown
    sidbuilder         *sidEmulation;
    sid2_model_t        sidModel;      // User requested sid model
    bool                sidSamples;
    uint_least32_t      leftVolume;
    uint_least32_t      rightVolume;
    sid2_sample_t       sampleFormat;
    uint_least16_t      powerOnDelay;
    uint_least32_t      sid2crcCount;  // Max sid writes to form crc
};

struct sid2_info_t
{
    const char       **credits;
    uint               channels;
    uint_least16_t     driverAddr;
    uint_least16_t     driverLength;
    const char        *name;
    const SidTuneInfo *tuneInfo; // May not need this
    const char        *version;
    // load, config and stop calls will reset this
    // and remove all pending events! 10th sec resolution.
    EventContext      *eventContext;
    uint               maxsids;
    sid2_env_t         environment;
    uint_least16_t     powerOnDelay;
    uint_least32_t     sid2crc;
    uint_least32_t     sid2crcCount; // Number of sid writes forming crc
};

#endif // _sid2types_h_
