
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
#define DVDMSG_GENERAL_SYNCRONIZE   0x00000008
#define DVDMSG_GENERAL_SETCLOCK     0x00000010

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

class CDVDMsgSyncronize
{
public:
  CDVDMsgSyncronize(long objects, DWORD timeout);
  ~CDVDMsgSyncronize();

  long Release();

  // waits untill all threads is either waiting, released the object.
  // if abort is set somehow
  void Wait(volatile bool *abort); 
private:

  long m_objects;
  long m_references;
  unsigned int m_timeout;
};

typedef struct SDVDMsgSetClock
{
  __int64 pts;
  __int64 dts;
} SDVDMsgSetClock;