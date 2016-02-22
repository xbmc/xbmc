/*
 *      vdr-plugin-vnsi - XBMC server plugin for VDR
 *
 *      Copyright (C) 2010 Alwin Esch (Team XBMC)
 *      Copyright (C) 2010, 2011 Alexander Pipelka
 *
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

#ifndef VNSI_COMMAND_H
#define VNSI_COMMAND_H

/** Current VNSI Protocol Version number */
#define VNSI_PROTOCOLVERSION 2

/** Packet types */
#define VNSI_CHANNEL_REQUEST_RESPONSE 1
#define VNSI_CHANNEL_STREAM           2
#define VNSI_CHANNEL_KEEPALIVE        3
#define VNSI_CHANNEL_NETLOG           4
#define VNSI_CHANNEL_STATUS           5
#define VNSI_CHANNEL_SCAN             6


/** Response packets operation codes */

/* OPCODE 1 - 19: VNSI network functions for general purpose */
#define VNSI_LOGIN                 1
#define VNSI_GETTIME               2
#define VNSI_ENABLESTATUSINTERFACE 3
#define VNSI_PING                  7

/* OPCODE 20 - 39: VNSI network functions for live streaming */
#define VNSI_CHANNELSTREAM_OPEN    20
#define VNSI_CHANNELSTREAM_CLOSE   21

/* OPCODE 40 - 59: VNSI network functions for recording streaming */
#define VNSI_RECSTREAM_OPEN        40
#define VNSI_RECSTREAM_CLOSE       41
#define VNSI_RECSTREAM_GETBLOCK    42
#define VNSI_RECSTREAM_POSTOFRAME  43
#define VNSI_RECSTREAM_FRAMETOPOS  44
#define VNSI_RECSTREAM_GETIFRAME   45

/* OPCODE 60 - 79: VNSI network functions for channel access */
#define VNSI_CHANNELS_GETCOUNT     61
#define VNSI_CHANNELS_GETCHANNELS  63
#define VNSI_CHANNELGROUP_GETCOUNT 65
#define VNSI_CHANNELGROUP_LIST     66
#define VNSI_CHANNELGROUP_MEMBERS  67

/* OPCODE 80 - 99: VNSI network functions for timer access */
#define VNSI_TIMER_GETCOUNT        80
#define VNSI_TIMER_GET             81
#define VNSI_TIMER_GETLIST         82
#define VNSI_TIMER_ADD             83
#define VNSI_TIMER_DELETE          84
#define VNSI_TIMER_UPDATE          85

/* OPCODE 100 - 119: VNSI network functions for recording access */
#define VNSI_RECORDINGS_DISKSIZE   100
#define VNSI_RECORDINGS_GETCOUNT   101
#define VNSI_RECORDINGS_GETLIST    102
#define VNSI_RECORDINGS_RENAME     103
#define VNSI_RECORDINGS_DELETE     104

/* OPCODE 120 - 139: VNSI network functions for epg access and manipulating */
#define VNSI_EPG_GETFORCHANNEL     120

/* OPCODE 140 - 159: VNSI network functions for channel scanning */
#define VNSI_SCAN_SUPPORTED        140
#define VNSI_SCAN_GETCOUNTRIES     141
#define VNSI_SCAN_GETSATELLITES    142
#define VNSI_SCAN_START            143
#define VNSI_SCAN_STOP             144


/** Stream packet types (server -> client) */
#define VNSI_STREAM_CHANGE       1
#define VNSI_STREAM_STATUS       2
#define VNSI_STREAM_QUEUESTATUS  3
#define VNSI_STREAM_MUXPKT       4
#define VNSI_STREAM_SIGNALINFO   5
#define VNSI_STREAM_CONTENTINFO  6

/** Scan packet types (server -> client) */
#define VNSI_SCANNER_PERCENTAGE  1
#define VNSI_SCANNER_SIGNAL      2
#define VNSI_SCANNER_DEVICE      3
#define VNSI_SCANNER_TRANSPONDER 4
#define VNSI_SCANNER_NEWCHANNEL  5
#define VNSI_SCANNER_FINISHED    6
#define VNSI_SCANNER_STATUS      7

/** Status packet types (server -> client) */
#define VNSI_STATUS_TIMERCHANGE      1
#define VNSI_STATUS_RECORDING        2
#define VNSI_STATUS_MESSAGE          3
#define VNSI_STATUS_CHANNELCHANGE    4
#define VNSI_STATUS_RECORDINGSCHANGE 5

/** Packet return codes */
#define VNSI_RET_OK              0
#define VNSI_RET_RECRUNNING      1
#define VNSI_RET_NOTSUPPORTED    995
#define VNSI_RET_DATAUNKNOWN     996
#define VNSI_RET_DATALOCKED      997
#define VNSI_RET_DATAINVALID     998
#define VNSI_RET_ERROR           999

#endif // VNSI_COMMAND_H
