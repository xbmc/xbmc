/*
 *      Copyright (C) 2010 Alwin Esch (Team XBMC)
 *      Copyright (C) 2011 Alexander Pipelka
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

#ifndef XVDR_COMMAND_H
#define XVDR_COMMAND_H

/** Current XVDR Protocol Version number */
#define XVDRPROTOCOLVERSION      3


/** Packet types */
#define XVDR_CHANNEL_REQUEST_RESPONSE 1
#define XVDR_CHANNEL_STREAM           2
#define XVDR_CHANNEL_STATUS           5
#define XVDR_CHANNEL_SCAN             6


/** Response packets operation codes */

/* OPCODE 1 - 19: XVDR network functions for general purpose */
#define XVDR_LOGIN                 1
#define XVDR_GETTIME               2
#define XVDR_ENABLESTATUSINTERFACE 3
#define XVDR_PING                  7
#define XVDR_UPDATECHANNELS        8

/* OPCODE 20 - 39: XVDR network functions for live streaming */
#define XVDR_CHANNELSTREAM_OPEN    20
#define XVDR_CHANNELSTREAM_CLOSE   21

/* OPCODE 40 - 59: XVDR network functions for recording streaming */
#define XVDR_RECSTREAM_OPEN        40
#define XVDR_RECSTREAM_CLOSE       41
#define XVDR_RECSTREAM_GETBLOCK    42
#define XVDR_RECSTREAM_POSTOFRAME  43
#define XVDR_RECSTREAM_FRAMETOPOS  44
#define XVDR_RECSTREAM_GETIFRAME   45
#define XVDR_RECSTREAM_UPDATE      46

/* OPCODE 60 - 79: XVDR network functions for channel access */
#define XVDR_CHANNELS_GETCOUNT     61
#define XVDR_CHANNELS_GETCHANNELS  63
#define XVDR_CHANNELGROUP_GETCOUNT 65
#define XVDR_CHANNELGROUP_LIST     66
#define XVDR_CHANNELGROUP_MEMBERS  67

/* OPCODE 80 - 99: XVDR network functions for timer access */
#define XVDR_TIMER_GETCOUNT        80
#define XVDR_TIMER_GET             81
#define XVDR_TIMER_GETLIST         82
#define XVDR_TIMER_ADD             83
#define XVDR_TIMER_DELETE          84
#define XVDR_TIMER_UPDATE          85

/* OPCODE 100 - 119: XVDR network functions for recording access */
#define XVDR_RECORDINGS_DISKSIZE   100
#define XVDR_RECORDINGS_GETCOUNT   101
#define XVDR_RECORDINGS_GETLIST    102
#define XVDR_RECORDINGS_RENAME     103
#define XVDR_RECORDINGS_DELETE     104

/* OPCODE 120 - 139: XVDR network functions for epg access and manipulating */
#define XVDR_EPG_GETFORCHANNEL     120

/* OPCODE 140 - 159: XVDR network functions for channel scanning */
#define XVDR_SCAN_SUPPORTED        140
#define XVDR_SCAN_GETCOUNTRIES     141
#define XVDR_SCAN_GETSATELLITES    142
#define XVDR_SCAN_START            143
#define XVDR_SCAN_STOP             144


/** Stream packet types (server -> client) */
#define XVDR_STREAM_CHANGE       1
#define XVDR_STREAM_STATUS       2
#define XVDR_STREAM_QUEUESTATUS  3
#define XVDR_STREAM_MUXPKT       4
#define XVDR_STREAM_SIGNALINFO   5
#define XVDR_STREAM_CONTENTINFO  6

/** Stream status codes */
#define XVDR_STREAM_STATUS_SIGNALLOST     111
#define XVDR_STREAM_STATUS_SIGNALRESTORED 112

/** Scan packet types (server -> client) */
#define XVDR_SCANNER_PERCENTAGE  1
#define XVDR_SCANNER_SIGNAL      2
#define XVDR_SCANNER_DEVICE      3
#define XVDR_SCANNER_TRANSPONDER 4
#define XVDR_SCANNER_NEWCHANNEL  5
#define XVDR_SCANNER_FINISHED    6
#define XVDR_SCANNER_STATUS      7

/** Status packet types (server -> client) */
#define XVDR_STATUS_TIMERCHANGE      1
#define XVDR_STATUS_RECORDING        2
#define XVDR_STATUS_MESSAGE          3
#define XVDR_STATUS_CHANNELCHANGE    4
#define XVDR_STATUS_RECORDINGSCHANGE 5

/** Packet return codes */
#define XVDR_RET_OK              0
#define XVDR_RET_RECRUNNING      1
#define XVDR_RET_NOTSUPPORTED    995
#define XVDR_RET_DATAUNKNOWN     996
#define XVDR_RET_DATALOCKED      997
#define XVDR_RET_DATAINVALID     998
#define XVDR_RET_ERROR           999

#endif // XVDR_COMMAND_H
