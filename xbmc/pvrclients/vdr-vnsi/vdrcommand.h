/*
 *      Copyright (C) 2010 Alwin Esch (Team XBMC)
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

#ifndef VDRCOMMAND_H
#define VDRCOMMAND_H

/** Current VNSI Protocol Version number */
const static uint32_t VNSIProtocolVersion      = 2;


/** Packet types */
const static uint32_t CHANNEL_REQUEST_RESPONSE = 1;
const static uint32_t CHANNEL_STREAM           = 2;
const static uint32_t CHANNEL_KEEPALIVE        = 3;
const static uint32_t CHANNEL_NETLOG           = 4;
const static uint32_t CHANNEL_STATUS           = 5;
const static uint32_t CHANNEL_SCAN             = 6;


/** Response packets operation codes */

/* OPCODE 1 - 19: VNSI network functions for general purpose */
const static uint32_t VDR_LOGIN                 = 1;
const static uint32_t VDR_GETTIME               = 2;
const static uint32_t VDR_ENABLESTATUSINTERFACE = 3;
const static uint32_t VDR_ENABLEOSDINTERFACE    = 4;

/* OPCODE 20 - 39: VNSI network functions for live streaming */
const static uint32_t VDR_CHANNELSTREAM_OPEN    = 20;
const static uint32_t VDR_CHANNELSTREAM_CLOSE   = 21;

/* OPCODE 40 - 59: VNSI network functions for recording streaming */
const static uint32_t VDR_RECSTREAM_OPEN        = 40;
const static uint32_t VDR_RECSTREAM_CLOSE       = 41;
const static uint32_t VDR_RECSTREAM_GETBLOCK    = 42;
const static uint32_t VDR_RECSTREAM_POSTOFRAME  = 43;
const static uint32_t VDR_RECSTREAM_FRAMETOPOS  = 44;
const static uint32_t VDR_RECSTREAM_GETIFRAME   = 45;

/* OPCODE 60 - 79: VNSI network functions for channel access */
const static uint32_t VDR_CHANNELS_GETCOUNT     = 61;
const static uint32_t VDR_CHANNELS_GETCHANNELS  = 63;

/* OPCODE 80 - 99: VNSI network functions for timer access */
const static uint32_t VDR_TIMER_GETCOUNT        = 80;
const static uint32_t VDR_TIMER_GET             = 81;
const static uint32_t VDR_TIMER_GETLIST         = 82;
const static uint32_t VDR_TIMER_ADD             = 83;
const static uint32_t VDR_TIMER_DELETE          = 84;
const static uint32_t VDR_TIMER_UPDATE          = 85;

/* OPCODE 100 - 119: VNSI network functions for recording access */
const static uint32_t VDR_RECORDINGS_DISKSIZE   = 100;
const static uint32_t VDR_RECORDINGS_GETCOUNT   = 101;
const static uint32_t VDR_RECORDINGS_GETLIST    = 102;
const static uint32_t VDR_RECORDINGS_RENAME     = 103;
const static uint32_t VDR_RECORDINGS_DELETE     = 104;

/* OPCODE 120 - 139: VNSI network functions for epg access and manipulating */
const static uint32_t VDR_EPG_GETFORCHANNEL     = 120;

/* OPCODE 140 - 159: VNSI network functions for channel scanning */
const static uint32_t VDR_SCAN_SUPPORTED        = 140;
const static uint32_t VDR_SCAN_GETCOUNTRIES     = 141;
const static uint32_t VDR_SCAN_GETSATELLITES    = 142;
const static uint32_t VDR_SCAN_START            = 143;
const static uint32_t VDR_SCAN_STOP             = 144;


/** Stream packet types (server -> client) */
const static uint32_t VDR_STREAM_CHANGE       = 1;
const static uint32_t VDR_STREAM_STATUS       = 2;
const static uint32_t VDR_STREAM_QUEUESTATUS  = 3;
const static uint32_t VDR_STREAM_MUXPKT       = 4;
const static uint32_t VDR_STREAM_SIGNALINFO   = 5;
const static uint32_t VDR_STREAM_CONTENTINFO  = 6;

/** Scan packet types (server -> client) */
const static uint32_t VDR_SCANNER_PERCENTAGE  = 1;
const static uint32_t VDR_SCANNER_SIGNAL      = 2;
const static uint32_t VDR_SCANNER_DEVICE      = 3;
const static uint32_t VDR_SCANNER_TRANSPONDER = 4;
const static uint32_t VDR_SCANNER_NEWCHANNEL  = 5;
const static uint32_t VDR_SCANNER_FINISHED    = 6;
const static uint32_t VDR_SCANNER_STATUS      = 7;

/** Status packet types (server -> client) */
const static uint32_t VDR_STATUS_TIMERCHANGE   = 1;
const static uint32_t VDR_STATUS_RECORDING     = 2;
const static uint32_t VDR_STATUS_MESSAGE       = 3;
const static uint32_t VDR_STATUS_CHANNELCHANGE = 4;

/** Packet return codes */
const static uint32_t VDR_RET_OK              = 0;
const static uint32_t VDR_RET_RECRUNNING      = 1;
const static uint32_t VDR_RET_NOTSUPPORTED    = 995;
const static uint32_t VDR_RET_DATAUNKNOWN     = 996;
const static uint32_t VDR_RET_DATALOCKED      = 997;
const static uint32_t VDR_RET_DATAINVALID     = 998;
const static uint32_t VDR_RET_ERROR           = 999;

#endif
