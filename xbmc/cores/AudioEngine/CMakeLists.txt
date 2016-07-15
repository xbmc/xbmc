set(SOURCES AEFactory.cpp
            AEResampleFactory.cpp
            AESinkFactory.cpp
            Engines/ActiveAE/AudioDSPAddons/ActiveAEDSP.cpp
            Engines/ActiveAE/AudioDSPAddons/ActiveAEDSPAddon.cpp
            Engines/ActiveAE/AudioDSPAddons/ActiveAEDSPDatabase.cpp
            Engines/ActiveAE/AudioDSPAddons/ActiveAEDSPMode.cpp
            Engines/ActiveAE/AudioDSPAddons/ActiveAEDSPProcess.cpp
            Encoders/AEEncoderFFmpeg.cpp
            Engines/ActiveAE/ActiveAE.cpp
            Engines/ActiveAE/ActiveAEBuffer.cpp
            Engines/ActiveAE/ActiveAEFilter.cpp
            Engines/ActiveAE/ActiveAESink.cpp
            Engines/ActiveAE/ActiveAEStream.cpp
            Engines/ActiveAE/ActiveAESound.cpp
            Utils/AEBitstreamPacker.cpp
            Utils/AEChannelInfo.cpp
            Utils/AEBuffer.cpp
            Utils/AEDeviceInfo.cpp
            Utils/AELimiter.cpp
            Utils/AEPackIEC61937.cpp
            Utils/AEStreamInfo.cpp
            Utils/AEUtil.cpp
            Sinks/AESinkNULL.cpp)

set(HEADERS AEFactory.h
            AEResampleFactory.h
            AESinkFactory.h
            Engines/ActiveAE/AudioDSPAddons/ActiveAEDSP.h
            Engines/ActiveAE/AudioDSPAddons/ActiveAEDSPAddon.h
            Engines/ActiveAE/AudioDSPAddons/ActiveAEDSPDatabase.h
            Engines/ActiveAE/AudioDSPAddons/ActiveAEDSPMode.h
            Engines/ActiveAE/AudioDSPAddons/ActiveAEDSPProcess.h
            Encoders/AEEncoderFFmpeg.h
            Engines/ActiveAE/ActiveAE.h
            Engines/ActiveAE/ActiveAEBuffer.h
            Engines/ActiveAE/ActiveAEFilter.h
            Engines/ActiveAE/ActiveAESink.h
            Engines/ActiveAE/ActiveAESound.h
            Engines/ActiveAE/ActiveAEStream.h
            Interfaces/AE.h
            Interfaces/AEEncoder.h
            Interfaces/AEResample.h
            Interfaces/AESink.h
            Interfaces/AESound.h
            Interfaces/AEStream.h
            Interfaces/IAudioCallback.h
            Interfaces/ThreadedAE.h
            Sinks/AESinkNULL.h
            Utils/AEAudioFormat.h
            Utils/AEBitstreamPacker.h
            Utils/AEBuffer.h
            Utils/AEChannelData.h
            Utils/AEChannelInfo.h
            Utils/AEDeviceInfo.h
            Utils/AELimiter.h
            Utils/AEPackIEC61937.h
            Utils/AERingBuffer.h
            Utils/AEStreamData.h
            Utils/AEStreamInfo.h
            Utils/AEUtil.h)

if(ALSA_FOUND)
  list(APPEND SOURCES Sinks/AESinkALSA.cpp
                      Sinks/alsa/ALSADeviceMonitor.cpp
                      Sinks/alsa/ALSAHControlMonitor.cpp
                      Sinks/AESinkOSS.cpp
                      Utils/AEELDParser.cpp)
  list(APPEND HEADERS Sinks/AESinkALSA.h
                      Sinks/alsa/ALSADeviceMonitor.h
                      Sinks/alsa/ALSAHControlMonitor.h
                      Sinks/AESinkOSS.h
                      Utils/AEELDParser.h)
endif()

if(PULSEAUDIO_FOUND)
  list(APPEND SOURCES Sinks/AESinkPULSE.cpp)
  list(APPEND HEADERS Sinks/AESinkPULSE.h)
endif()

if(FFMPEG_FOUND)
  list(APPEND SOURCES Engines/ActiveAE/ActiveAEResampleFFMPEG.cpp)
  list(APPEND HEADERS Engines/ActiveAE/ActiveAEResampleFFMPEG.h)
endif()

if(CORE_SYSTEM_NAME STREQUAL windows)
  list(APPEND SOURCES Sinks/AESinkDirectSound.cpp
                      Sinks/AESinkWASAPI.cpp)
  list(APPEND HEADERS Sinks/AESinkDirectSound.h
                      Sinks/AESinkWASAPI.h)
endif()

if(CORE_SYSTEM_NAME STREQUAL rbpi)
  list(APPEND SOURCES Engines/ActiveAE/ActiveAEResamplePi.cpp
                      Sinks/AESinkPi.cpp)
  list(APPEND HEADERS Engines/ActiveAE/ActiveAEResamplePi.h
                      Sinks/AESinkPi.h)
endif()

if(CORE_SYSTEM_NAME STREQUAL osx)
  list(APPEND SOURCES Sinks/AESinkDARWINOSX.cpp
                      Sinks/osx/AEDeviceEnumerationOSX.cpp
                      Sinks/osx/CoreAudioChannelLayout.cpp
                      Sinks/osx/CoreAudioDevice.cpp
                      Sinks/osx/CoreAudioHardware.cpp
                      Sinks/osx/CoreAudioHelpers.cpp
                      Sinks/osx/CoreAudioStream.cpp)
  list(APPEND HEADERS Sinks/AESinkDARWINOSX.h
                      Sinks/osx/AEDeviceEnumerationOSX.h
                      Sinks/osx/CoreAudioChannelLayout.h
                      Sinks/osx/CoreAudioDevice.h
                      Sinks/osx/CoreAudioHardware.h
                      Sinks/osx/CoreAudioHelpers.h
                      Sinks/osx/CoreAudioStream.h)
endif()

if(CORE_SYSTEM_NAME STREQUAL ios)
  list(APPEND SOURCES Sinks/AESinkDARWINIOS.mm
                      Sinks/osx/CoreAudioHelpers.cpp)
  list(APPEND HEADERS Sinks/AESinkDARWINIOS.h
                      Sinks/osx/CoreAudioHelpers.h)
endif()

if(CORE_SYSTEM_NAME STREQUAL android)
  list(APPEND SOURCES Sinks/AESinkAUDIOTRACK.cpp)
  list(APPEND HEADERS Sinks/AESinkAUDIOTRACK.h)
endif()

if(CORE_SYSTEM_NAME STREQUAL freebsd)
  list(APPEND SOURCES Sinks/AESinkOSS.cpp)
  list(APPEND HEADERS Sinks/AESinkOSS.h)
endif()

core_add_library(audioengine)
target_include_directories(${CORE_LIBRARY} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
