#pragma once
/*
 *      Copyright (C) 2005-2009 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifndef __CHANNELS_H
#define __CHANNELS_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "tools.h"

#define MAXAPIDS 32 // audio
#define MAXDPIDS 16 // dolby (AC3 + DTS)
#define MAXSPIDS 32 // subtitles
#define MAXCAIDS  8 // conditional access

#define MAXLANGCODE1 4 // a 3 letter language code, zero terminated
#define MAXLANGCODE2 8 // up to two 3 letter language codes, separated by '+' and zero terminated

#define CHANNELMOD_NONE     0x00
#define CHANNELMOD_ALL      0xFF
#define CHANNELMOD_NAME     0x01
#define CHANNELMOD_PIDS     0x02
#define CHANNELMOD_ID       0x04
#define CHANNELMOD_CA       0x10
#define CHANNELMOD_TRANSP   0x20
#define CHANNELMOD_LANGS    0x40
#define CHANNELMOD_RETUNE   (CHANNELMOD_PIDS | CHANNELMOD_CA | CHANNELMOD_TRANSP)

#define CA_FTA           0x0000
#define CA_DVB_MIN       0x0001
#define CA_DVB_MAX       0x000F
#define CA_USER_MIN      0x0010
#define CA_USER_MAX      0x00FF
#define CA_ENCRYPTED_MIN 0x0100
#define CA_ENCRYPTED_MAX 0xFFFF

typedef enum fe_type {
        FE_QPSK,
        FE_QAM,
        FE_OFDM,
        FE_ATSC
} fe_type_t;


typedef enum fe_caps {
        FE_IS_STUPID                    = 0,
        FE_CAN_INVERSION_AUTO           = 0x1,
        FE_CAN_FEC_1_2                  = 0x2,
        FE_CAN_FEC_2_3                  = 0x4,
        FE_CAN_FEC_3_4                  = 0x8,
        FE_CAN_FEC_4_5                  = 0x10,
        FE_CAN_FEC_5_6                  = 0x20,
        FE_CAN_FEC_6_7                  = 0x40,
        FE_CAN_FEC_7_8                  = 0x80,
        FE_CAN_FEC_8_9                  = 0x100,
        FE_CAN_FEC_AUTO                 = 0x200,
        FE_CAN_QPSK                     = 0x400,
        FE_CAN_QAM_16                   = 0x800,
        FE_CAN_QAM_32                   = 0x1000,
        FE_CAN_QAM_64                   = 0x2000,
        FE_CAN_QAM_128                  = 0x4000,
        FE_CAN_QAM_256                  = 0x8000,
        FE_CAN_QAM_AUTO                 = 0x10000,
        FE_CAN_TRANSMISSION_MODE_AUTO   = 0x20000,
        FE_CAN_BANDWIDTH_AUTO           = 0x40000,
        FE_CAN_GUARD_INTERVAL_AUTO      = 0x80000,
        FE_CAN_HIERARCHY_AUTO           = 0x100000,
        FE_CAN_8VSB                     = 0x200000,
        FE_CAN_16VSB                    = 0x400000,
        FE_HAS_EXTENDED_CAPS            = 0x800000,   /* We need more bitspace for newer APIs, indicate this. */
        FE_CAN_2G_MODULATION            = 0x10000000, /* frontend supports "2nd generation modulation" (DVB-S2) */
        FE_NEEDS_BENDING                = 0x20000000, /* not supported anymore, don't use (frontend requires frequency bending) */
        FE_CAN_RECOVER                  = 0x40000000, /* frontend can recover from a cable unplug automatically */
        FE_CAN_MUTE_TS                  = 0x80000000  /* frontend can stop spurious TS data output */
} fe_caps_t;

typedef enum fe_sec_voltage {
        SEC_VOLTAGE_13,
        SEC_VOLTAGE_18,
        SEC_VOLTAGE_OFF
} fe_sec_voltage_t;


typedef enum fe_sec_tone_mode {
        SEC_TONE_ON,
        SEC_TONE_OFF
} fe_sec_tone_mode_t;


typedef enum fe_sec_mini_cmd {
        SEC_MINI_A,
        SEC_MINI_B
} fe_sec_mini_cmd_t;


typedef enum fe_status {
        FE_HAS_SIGNAL   = 0x01,   /* found something above the noise level */
        FE_HAS_CARRIER  = 0x02,   /* found a DVB signal  */
        FE_HAS_VITERBI  = 0x04,   /* FEC is stable  */
        FE_HAS_SYNC     = 0x08,   /* found sync bytes  */
        FE_HAS_LOCK     = 0x10,   /* everything's working... */
        FE_TIMEDOUT     = 0x20,   /* no lock within the last ~2 seconds */
        FE_REINIT       = 0x40    /* frontend was reinitialized,  */
} fe_status_t;                    /* application is recommended to reset */
                                  /* DiSEqC, tone and parameters */

typedef enum fe_spectral_inversion {
        INVERSION_OFF,
        INVERSION_ON,
        INVERSION_AUTO
} fe_spectral_inversion_t;


typedef enum fe_code_rate {
        FEC_NONE = 0,
        FEC_1_2,
        FEC_2_3,
        FEC_3_4,
        FEC_4_5,
        FEC_5_6,
        FEC_6_7,
        FEC_7_8,
        FEC_8_9,
        FEC_AUTO,
        FEC_3_5,
        FEC_9_10,
} fe_code_rate_t;

typedef enum fe_modulation {
        QPSK,
        QAM_16,
        QAM_32,
        QAM_64,
        QAM_128,
        QAM_256,
        QAM_AUTO,
        VSB_8,
        VSB_16,
        PSK_8,
        APSK_16,
        APSK_32,
        DQPSK,
} fe_modulation_t;

typedef enum fe_transmit_mode {
        TRANSMISSION_MODE_2K,
        TRANSMISSION_MODE_8K,
        TRANSMISSION_MODE_AUTO
} fe_transmit_mode_t;

typedef enum fe_bandwidth {
        BANDWIDTH_8_MHZ,
        BANDWIDTH_7_MHZ,
        BANDWIDTH_6_MHZ,
        BANDWIDTH_AUTO
} fe_bandwidth_t;


typedef enum fe_guard_interval {
        GUARD_INTERVAL_1_32,
        GUARD_INTERVAL_1_16,
        GUARD_INTERVAL_1_8,
        GUARD_INTERVAL_1_4,
        GUARD_INTERVAL_AUTO
} fe_guard_interval_t;


typedef enum fe_hierarchy {
        HIERARCHY_NONE,
        HIERARCHY_1,
        HIERARCHY_2,
        HIERARCHY_4,
        HIERARCHY_AUTO
} fe_hierarchy_t;

typedef enum fe_pilot {
        PILOT_ON,
        PILOT_OFF,
        PILOT_AUTO,
} fe_pilot_t;

typedef enum fe_rolloff {
        ROLLOFF_35, /* Implied value in DVB-S, default for DVB-S2 */
        ROLLOFF_20,
        ROLLOFF_25,
        ROLLOFF_AUTO,
} fe_rolloff_t;

typedef enum fe_delivery_system {
        SYS_UNDEFINED,
        SYS_DVBC_ANNEX_AC,
        SYS_DVBC_ANNEX_B,
        SYS_DVBT,
        SYS_DSS,
        SYS_DVBS,
        SYS_DVBS2,
        SYS_DVBH,
        SYS_ISDBT,
        SYS_ISDBS,
        SYS_ISDBC,
        SYS_ATSC,
        SYS_ATSCMH,
        SYS_DMBTH,
        SYS_CMMB,
        SYS_DAB,
} fe_delivery_system_t;

struct tChannelParameterMap {
  int userValue;
  int driverValue;
  const char *userString;
};

class cChannel
{
private:
  char *name;
  char *shortName;
  char *provider;
  char *portalName;
  int __BeginData__;
  int frequency; // MHz
  int source;
  int srate;
  int vpid;
  int ppid;
  int vtype;
  int apids[MAXAPIDS + 1]; // list is zero-terminated
  char alangs[MAXAPIDS][MAXLANGCODE2];
  int dpids[MAXDPIDS + 1]; // list is zero-terminated
  char dlangs[MAXDPIDS][MAXLANGCODE2];
  int spids[MAXSPIDS + 1]; // list is zero-terminated
  char slangs[MAXSPIDS][MAXLANGCODE2];
  int tpid;
  int caids[MAXCAIDS + 1]; // list is zero-terminated
  int nid;
  int tid;
  int sid;
  int rid;
  int number;    // Sequence number assigned on load
  char polarization;
  int inversion;
  int bandwidth;
  int coderateH;
  int coderateL;
  int modulation;
  int system;
  int transmission;
  int guard;
  int hierarchy;
  int rollOff;
  int __EndData__;
  int modification;
  
  bool StringToParameters(const char *s);

public:
  cChannel(const PVR_CHANNEL *Channel);
  cChannel();
  virtual ~cChannel();
  
  bool ReadFromVTP(int channel);
  bool Parse(const char *s);
  const char *Name(void) const { return name; }
  const char *ShortName(bool OrName = false) const { return (OrName && isempty(shortName)) ? name : shortName; }
  const char *Provider(void) const { return provider; }
  const char *PortalName(void) const { return portalName; }
  int Frequency(void) const { return frequency; } ///< Returns the actual frequency, as given in 'channels.conf'
  int Source(void) const { return source; }
  int Srate(void) const { return srate; }
  int Vpid(void) const { return vpid; }
  int Ppid(void) const { return ppid; }
  int Vtype(void) const { return vtype; }
  const int *Apids(void) const { return apids; }
  const int *Dpids(void) const { return dpids; }
  const int *Spids(void) const { return spids; }
  int Apid(int i) const { return (0 <= i && i < MAXAPIDS) ? apids[i] : 0; }
  int Dpid(int i) const { return (0 <= i && i < MAXDPIDS) ? dpids[i] : 0; }
  int Spid(int i) const { return (0 <= i && i < MAXSPIDS) ? spids[i] : 0; }
  const char *Alang(int i) const { return (0 <= i && i < MAXAPIDS) ? alangs[i] : ""; }
  const char *Dlang(int i) const { return (0 <= i && i < MAXDPIDS) ? dlangs[i] : ""; }
  const char *Slang(int i) const { return (0 <= i && i < MAXSPIDS) ? slangs[i] : ""; }
  int Tpid(void) const { return tpid; }
  const int *Caids(void) const { return caids; }
  int Ca(int Index = 0) const { return Index < MAXCAIDS ? caids[Index] : 0; }
  int Nid(void) const { return nid; }
  int Tid(void) const { return tid; }
  int Sid(void) const { return sid; }
  int Rid(void) const { return rid; }
  int Number(void) const { return number; }
  void SetNumber(int Number) { number = Number; }
  char Polarization(void) const { return polarization; }
  int Inversion(void) const { return inversion; }
  int Bandwidth(void) const { return bandwidth; }
  int CoderateH(void) const { return coderateH; }
  int CoderateL(void) const { return coderateL; }
  int Modulation(void) const { return modulation; }
  int System(void) const { return system; }
  int Transmission(void) const { return transmission; }
  int Guard(void) const { return guard; }
  int Hierarchy(void) const { return hierarchy; }
  int RollOff(void) const { return rollOff; }

};

#endif //__TIMERS_H
