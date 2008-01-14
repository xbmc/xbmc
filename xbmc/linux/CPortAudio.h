#ifndef _PORT_AUDIO_H_
#define _PORT_AUDIO_H_

#include <portaudio.h>
#include <vector>
#include "StdString.h"

#define SAFELY(op)     \
{                      \
    int err;           \
    if (0) printf("[PortAudio] DEBUG: %s\n", #op); \
    if ((err=op) != 0) \
       printf("[PortAudio] ERROR: %s.\n", Pa_GetErrorText(err)); \
}

class CPortAudio
{
 public:

  //
  // Get a list of output devices.
  //
  static std::vector<PaDeviceInfo*> GetDeviceList(bool includeOutput=true, bool includeInput=false);
  
  //
  // Create an output stream.
  //
  static PaStream* CreateOutputStream(const CStdString& strName, int channels, int sampleRate, int bitsPerSample, bool isDigital, int packetSize);
};

#endif
