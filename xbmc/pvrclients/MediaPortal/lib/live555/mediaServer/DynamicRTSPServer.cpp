/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// Copyright (c) 1996-2010, Live Networks, Inc.  All rights reserved
// A subclass of "RTSPServer" that creates "ServerMediaSession"s on demand,
// based on whether or not the specified stream name exists as a file
// Implementation

#include "DynamicRTSPServer.hh"
#include <liveMedia.hh>
#include <string.h>

DynamicRTSPServer*
DynamicRTSPServer::createNew(UsageEnvironment& env, Port ourPort,
			     UserAuthenticationDatabase* authDatabase,
			     unsigned reclamationTestSeconds) {
  int ourSocket = -1;

  do {
    int ourSocket = setUpOurSocket(env, ourPort);
    if (ourSocket == -1) break;

    return new DynamicRTSPServer(env, ourSocket, ourPort, authDatabase, reclamationTestSeconds);
  } while (0);

  if (ourSocket != -1) ::closeSocket(ourSocket);
  return NULL;
}

DynamicRTSPServer::DynamicRTSPServer(UsageEnvironment& env, int ourSocket,
				     Port ourPort,
				     UserAuthenticationDatabase* authDatabase, unsigned reclamationTestSeconds)
  : RTSPServer(env, ourSocket, ourPort, authDatabase, reclamationTestSeconds) {
}

DynamicRTSPServer::~DynamicRTSPServer() {
}

static ServerMediaSession* createNewSMS(UsageEnvironment& env,
					char const* fileName, FILE* fid); // forward

ServerMediaSession*
DynamicRTSPServer::lookupServerMediaSession(char const* streamName) {
  // First, check whether the specified "streamName" exists as a local file:
  FILE* fid = fopen(streamName, "rb");
  Boolean fileExists = fid != NULL;

  // Next, check whether we already have a "ServerMediaSession" for this file:
  ServerMediaSession* sms = RTSPServer::lookupServerMediaSession(streamName);
  Boolean smsExists = sms != NULL;

  // Handle the four possibilities for "fileExists" and "smsExists":
  if (!fileExists) {
    if (smsExists) {
      // "sms" was created for a file that no longer exists. Remove it:
      removeServerMediaSession(sms);
    }
    return NULL;
  } else {
    if (!smsExists) {
      // Create a new "ServerMediaSession" object for streaming from the named file.
      sms = createNewSMS(envir(), streamName, fid);
      addServerMediaSession(sms);
    }
    fclose(fid);
    return sms;
  }
}

#define NEW_SMS(description) do {\
char const* descStr = description\
    ", streamed by the LIVE555 Media Server";\
sms = ServerMediaSession::createNew(env, fileName, fileName, descStr);\
} while(0)

static ServerMediaSession* createNewSMS(UsageEnvironment& env,
					char const* fileName, FILE* /*fid*/) {
  // Use the file name extension to determine the type of "ServerMediaSession":
  char const* extension = strrchr(fileName, '.');
  if (extension == NULL) return NULL;

  ServerMediaSession* sms = NULL;
  Boolean const reuseSource = False;
  if (strcmp(extension, ".aac") == 0) {
    // Assumed to be an AAC Audio (ADTS format) file:
    NEW_SMS("AAC Audio");
    sms->addSubsession(ADTSAudioFileServerMediaSubsession::createNew(env, fileName, reuseSource));
  } else if (strcmp(extension, ".amr") == 0) {
    // Assumed to be an AMR Audio file:
    NEW_SMS("AMR Audio");
    sms->addSubsession(AMRAudioFileServerMediaSubsession::createNew(env, fileName, reuseSource));
  } else if (strcmp(extension, ".m4e") == 0) {
    // Assumed to be a MPEG-4 Video Elementary Stream file:
    NEW_SMS("MPEG-4 Video");
    sms->addSubsession(MPEG4VideoFileServerMediaSubsession::createNew(env, fileName, reuseSource));
  } else if (strcmp(extension, ".mp3") == 0) {
    // Assumed to be a MPEG-1 or 2 Audio file:
    NEW_SMS("MPEG-1 or 2 Audio");
    // To stream using 'ADUs' rather than raw MP3 frames, uncomment the following:
//#define STREAM_USING_ADUS 1
    // To also reorder ADUs before streaming, uncomment the following:
//#define INTERLEAVE_ADUS 1
    // (For more information about ADUs and interleaving,
    //  see <http://www.live555.com/rtp-mp3/>)
    Boolean useADUs = False;
    Interleaving* interleaving = NULL;
#ifdef STREAM_USING_ADUS
    useADUs = True;
#ifdef INTERLEAVE_ADUS
    unsigned char interleaveCycle[] = {0,2,1,3}; // or choose your own...
    unsigned const interleaveCycleSize
      = (sizeof interleaveCycle)/(sizeof (unsigned char));
    interleaving = new Interleaving(interleaveCycleSize, interleaveCycle);
#endif
#endif
    sms->addSubsession(MP3AudioFileServerMediaSubsession::createNew(env, fileName, reuseSource, useADUs, interleaving));
  } else if (strcmp(extension, ".mpg") == 0) {
    // Assumed to be a MPEG-1 or 2 Program Stream (audio+video) file:
    NEW_SMS("MPEG-1 or 2 Program Stream");
    MPEG1or2FileServerDemux* demux
      = MPEG1or2FileServerDemux::createNew(env, fileName, reuseSource);
    sms->addSubsession(demux->newVideoServerMediaSubsession());
    sms->addSubsession(demux->newAudioServerMediaSubsession());
  } else if (strcmp(extension, ".ts") == 0) {
    // Assumed to be a MPEG Transport Stream file:
    // Use an index file name that's the same as the TS file name, except with ".tsx":
    unsigned indexFileNameLen = strlen(fileName) + 2; // allow for trailing "x\0"
    char* indexFileName = new char[indexFileNameLen];
    sprintf(indexFileName, "%sx", fileName);
    NEW_SMS("MPEG Transport Stream");
    sms->addSubsession(MPEG2TransportFileServerMediaSubsession::createNew(env, fileName, indexFileName, reuseSource));
    delete[] indexFileName;
  } else if (strcmp(extension, ".wav") == 0) {
    // Assumed to be a WAV Audio file:
    NEW_SMS("WAV Audio Stream");
    // To convert 16-bit PCM data to 8-bit u-law, prior to streaming,
    // change the following to True:
    Boolean convertToULaw = False;
    sms->addSubsession(WAVAudioFileServerMediaSubsession::createNew(env, fileName, reuseSource, convertToULaw));
  } else if (strcmp(extension, ".dv") == 0) {
    // Assumed to be a DV Video file
    // First, make sure that the RTPSinks' buffers will be large enough to handle the huge size of DV frames (as big as 288000).
    OutPacketBuffer::maxSize = 300000;

    NEW_SMS("DV Video");
    sms->addSubsession(DVVideoFileServerMediaSubsession::createNew(env, fileName, reuseSource));
  }

  return sms;
}
