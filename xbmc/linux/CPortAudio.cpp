#include <pthread.h>
#include "CPortAudio.h"

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
    
    printf("Asked to create device:   [%s]\n", strName.c_str());
    printf("Device should be digital: [%d]\n", isDigital);
    printf("Channels:                 [%d]\n", channels);
    printf("Sample Rate:              [%d]\n", sampleRate);
    printf("bitsPerSample:            [%d]\n", bitsPerSample);
    
    std::vector<PaDeviceInfo* > deviceList = CPortAudio::GetDeviceList();
    std::vector<PaDeviceInfo* >::const_iterator iter = deviceList.begin();

    int numDevices = Pa_GetDeviceCount();
    int pickedDevice = -1;
    
    // Find the device.
    for(int i=0; i<numDevices && pickedDevice == -1; i++)
    {
        PaDeviceInfo* deviceInfo = (PaDeviceInfo* )Pa_GetDeviceInfo(i);
       
        if (strName.Equals(deviceInfo->name) || (isDigital == true && strstr(deviceInfo->name, "Digital") != 0))
        {
            printf("Picked device:            [%s]\n", deviceInfo->name);
            pickedDevice = i;
        }
    }
    
    // Open the device.
    PaStreamParameters outputParameters;
    outputParameters.channelCount = channels;
    outputParameters.device = pickedDevice;
    outputParameters.hostApiSpecificStreamInfo = NULL;
    outputParameters.sampleFormat = paInt16;
    outputParameters.suggestedLatency = Pa_GetDeviceInfo(pickedDevice)->defaultLowOutputLatency;

    int err = Pa_OpenStream(
                    &ret,
                    0,
                    &outputParameters,
                    (double)sampleRate,
                    packetSize/(channels*bitsPerSample/8),
                    paNoFlag,
                    0,
                    0);
    
    if (err != 0)
        printf("[PortAudio] Error opening stream: %d.\n", err);
    
    return ret;
}
