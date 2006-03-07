
#pragma once

/*

Message types definitions
Only the numbers 0, 1, 2, 4, 8 may be used to construct new msg types

*/

typedef unsigned int DVDMsg;
typedef void*        DVDMsgData;


#define DVDMSG_IS(msg, type)        (msg & type)



// general
#define DVDMSG_GENERAL_RESYNC       0x00000001 // will be set in a packet to signal that we have an discontinuity
#define DVDMSG_GENERAL_FLUSH        0x00000002
#define DVDMSG_GENERAL_STREAMCHANGE 0x00000004

// inputstreams

// demuxers
#define DVDMSG_DEMUX_DATA_PACKET    0x00000100

// audio

// audio codec

// video
#define DVDMSG_VIDEO_NOSKIP         0x00100000

// video codec

//subtitle

class CDVDMessage
{
public:
  static void FreeMessageData(DVDMsg msg, DVDMsgData msg_data);
};