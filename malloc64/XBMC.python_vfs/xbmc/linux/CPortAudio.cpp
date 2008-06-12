/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include <pthread.h>
#include "CPortAudio.h"
#include "PlatformDefs.h"
#include "log.h"

#ifdef __APPLE__
#include "pa_mac_core.h"
#endif

pthread_once_t once_control = PTHREAD_ONCE_INIT;
void InitializePA()
{
    Pa_Initialize();
}

//////////////////////////////////////////////////////////////////////////////
//
// History:
//   12.15.07   ESF  Created.
//
//////////////////////////////////////////////////////////////////////////////
std::vector<PaDeviceInfo* > CPortAudio::GetDeviceList(bool includeOutput, bool includeInput)
{
    // Make sure it's initialized.
    pthread_once(&once_control, InitializePA);

    std::vector<PaDeviceInfo* > ret;

    int numDevices = Pa_GetDeviceCount();
    for(int i=0; i<numDevices; i++)
    {
        PaDeviceInfo* deviceInfo = (PaDeviceInfo* )Pa_GetDeviceInfo(i);
        if (includeOutput && deviceInfo->maxOutputChannels > 0 ||
            includeInput  && deviceInfo->maxInputChannels > 0)
        {
          ret.push_back(deviceInfo);
        }
    }

    return ret;
}

//////////////////////////////////////////////////////////////////////////////
//
// History:
//   12.15.07   ESF  Created.
//
//////////////////////////////////////////////////////////////////////////////
PaStream* CPortAudio::CreateOutputStream(const CStdString& strName, int channels, int sampleRate, int bitsPerSample, bool isDigital, int packetSize)
{
    PaStream* ret = 0;

    CLog::Log(LOGNOTICE, "Asked to create device:   [%s]", strName.c_str());
    CLog::Log(LOGNOTICE, "Device should be digital: [%d]\n", isDigital);
    CLog::Log(LOGNOTICE, "Channels:                 [%d]\n", channels);
    CLog::Log(LOGNOTICE, "Sample Rate:              [%d]\n", sampleRate);
    CLog::Log(LOGNOTICE, "BitsPerSample:            [%d]\n", bitsPerSample);
    CLog::Log(LOGNOTICE, "PacketSize:               [%d]\n", packetSize);

    std::vector<PaDeviceInfo* > deviceList = CPortAudio::GetDeviceList();
    std::vector<PaDeviceInfo* >::const_iterator iter = deviceList.begin();

    int numDevices = Pa_GetDeviceCount();
    int pickedDevice = -1;
    int backupDevice = -1;

    // Find the device.
    for(int i=0; i<numDevices && pickedDevice == -1; i++)
    {
        PaDeviceInfo* deviceInfo = (PaDeviceInfo* )Pa_GetDeviceInfo(i);

        if (deviceInfo->maxOutputChannels > 0)
        {
          // Keep around first output device.
          if (backupDevice == -1)
            backupDevice = i;

          CLog::Log(LOGDEBUG, "Considering:              [%s]\n", deviceInfo->name);

          if (strName.Equals(deviceInfo->name) ||
              (backupDevice != -1 && strName.Equals("Default")))
          {
            CLog::Log(LOGNOTICE, "Picked device:            [%s]\n", deviceInfo->name);
            pickedDevice = i;
          }
        }
    }

    int err = 0;
    int framesPerBuffer = packetSize/(channels*bitsPerSample/8);

    // If we didn't pick a device, use the backup device.
    if (pickedDevice == -1)
      pickedDevice = backupDevice;

    if (pickedDevice != -1)
    {
      // Open the specifc device.
      PaStreamParameters outputParameters;
      outputParameters.channelCount = channels;
      outputParameters.device = pickedDevice;
      outputParameters.hostApiSpecificStreamInfo = NULL;
      outputParameters.sampleFormat = paInt16;
      outputParameters.suggestedLatency = Pa_GetDeviceInfo(pickedDevice)->defaultLowOutputLatency;

#ifdef __APPLE__
      // We want to change sample rates to fit our needs, but only if we're doing digital out.
      if (isDigital == true)
      {
        PaMacCoreStreamInfo macStream;
        PaMacCore_SetupStreamInfo(&macStream, paMacCoreChangeDeviceParameters | paMacCoreFailIfConversionRequired);
        outputParameters.hostApiSpecificStreamInfo = &macStream;
      }
#endif

      err = Pa_OpenStream(&ret, 0, &outputParameters, (double)sampleRate, framesPerBuffer, paNoFlag, 0, 0);
    }

    if (err != 0)
        CLog::Log(LOGERROR, "[PortAudio] Error opening stream: %d.\n", err);

    return ret;
}
