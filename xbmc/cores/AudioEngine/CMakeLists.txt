set(SOURCES AEResampleFactory.cpp
            AESinkFactory.cpp
            Encoders/AEEncoderFFmpeg.cpp
            Engines/ActiveAE/ActiveAE.cpp
            Engines/ActiveAE/ActiveAEBuffer.cpp
            Engines/ActiveAE/ActiveAEFilter.cpp
            Engines/ActiveAE/ActiveAESink.cpp
            Engines/ActiveAE/ActiveAEStream.cpp
            Engines/ActiveAE/ActiveAESound.cpp
            Engines/ActiveAE/ActiveAESettings.cpp
            Utils/AEBitstreamPacker.cpp
            Utils/AEChannelInfo.cpp
            Utils/AEDeviceInfo.cpp
            Utils/AELimiter.cpp
            Utils/AEPackIEC61937.cpp
            Utils/AEStreamInfo.cpp
            Utils/AEUtil.cpp)

set(HEADERS AEResampleFactory.h
            AESinkFactory.h
            Encoders/AEEncoderFFmpeg.h
            Engines/ActiveAE/ActiveAE.h
            Engines/ActiveAE/ActiveAEBuffer.h
            Engines/ActiveAE/ActiveAEFilter.h
            Engines/ActiveAE/ActiveAESink.h
            Engines/ActiveAE/ActiveAESound.h
            Engines/ActiveAE/ActiveAEStream.h
            Engines/ActiveAE/ActiveAESettings.h
            Interfaces/AE.h
            Interfaces/AEEncoder.h
            Interfaces/AEResample.h
            Interfaces/AESink.h
            Interfaces/AESound.h
            Interfaces/AEStream.h
            Interfaces/IAudioCallback.h
            Interfaces/ThreadedAE.h
            Utils/AEAudioFormat.h
            Utils/AEBitstreamPacker.h
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
                      Utils/AEELDParser.cpp)
  list(APPEND HEADERS Sinks/AESinkALSA.h
                      Utils/AEELDParser.h)
                      
  if(NOT CORE_PLATFORM_NAME_LC STREQUAL x11)
    list(APPEND SOURCES Sinks/alsa/ALSAHControlMonitor.cpp)
    list(APPEND HEADERS Sinks/alsa/ALSAHControlMonitor.h)
  endif()

  if(UDEV_FOUND)
    list(APPEND SOURCES Sinks/alsa/ALSADeviceMonitor.cpp)
    list(APPEND HEADERS Sinks/alsa/ALSADeviceMonitor.h)
  endif()
endif()

if(PULSEAUDIO_FOUND)
  list(APPEND SOURCES Sinks/AESinkPULSE.cpp)
  list(APPEND HEADERS Sinks/AESinkPULSE.h)
endif()

if(SNDIO_FOUND)
  list(APPEND SOURCES Sinks/AESinkSNDIO.cpp)
  list(APPEND HEADERS Sinks/AESinkSNDIO.h)
endif()

if(FFMPEG_FOUND)
  list(APPEND SOURCES Engines/ActiveAE/ActiveAEResampleFFMPEG.cpp)
  list(APPEND HEADERS Engines/ActiveAE/ActiveAEResampleFFMPEG.h)
endif()

if(CORE_SYSTEM_NAME MATCHES windows)
  list(APPEND SOURCES Sinks/AESinkWASAPI.cpp
                      Sinks/windows/AESinkFactoryWin.cpp)
  list(APPEND HEADERS Sinks/AESinkWASAPI.h
                      Sinks/windows/AESinkFactoryWin.h)
  if(CORE_SYSTEM_NAME STREQUAL windowsstore)
    list(APPEND SOURCES Sinks/AESinkXAudio.cpp
                        Sinks/windows/AESinkFactoryWin10.cpp)
    list(APPEND SOURCES Sinks/AESinkXAudio.h)
  elseif(CORE_SYSTEM_NAME STREQUAL windows)
    list(APPEND SOURCES Sinks/AESinkDirectSound.cpp
                        Sinks/windows/AESinkFactoryWin32.cpp)
    list(APPEND SOURCES Sinks/AESinkDirectSound.h)
  endif()
endif()

if(CORE_PLATFORM_NAME_LC STREQUAL rbpi)
  list(APPEND SOURCES Engines/ActiveAE/ActiveAEResamplePi.cpp
                      Sinks/AESinkPi.cpp)
  list(APPEND HEADERS Engines/ActiveAE/ActiveAEResamplePi.h
                      Sinks/AESinkPi.h)
endif()

if(CORE_SYSTEM_NAME STREQUAL osx)
  list(APPEND SOURCES Sinks/AESinkDARWINOSX.cpp
                      Sinks/darwin/CoreAudioHelpers.cpp
                      Sinks/osx/AEDeviceEnumerationOSX.cpp
                      Sinks/osx/CoreAudioChannelLayout.cpp
                      Sinks/osx/CoreAudioDevice.cpp
                      Sinks/osx/CoreAudioHardware.cpp
                      Sinks/osx/CoreAudioStream.cpp)
  list(APPEND HEADERS Sinks/AESinkDARWINOSX.h
                      Sinks/darwin/CoreAudioHelpers.h
                      Sinks/osx/AEDeviceEnumerationOSX.h
                      Sinks/osx/CoreAudioChannelLayout.h
                      Sinks/osx/CoreAudioDevice.h
                      Sinks/osx/CoreAudioHardware.h
                      Sinks/osx/CoreAudioStream.h)
endif()

if(CORE_SYSTEM_NAME STREQUAL darwin_embedded)
  list(APPEND SOURCES Sinks/AESinkDARWINIOS.mm
                      Sinks/darwin/CoreAudioHelpers.cpp)
  list(APPEND HEADERS Sinks/AESinkDARWINIOS.h
                      Sinks/darwin/CoreAudioHelpers.h)
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
if(NOT CORE_SYSTEM_NAME STREQUAL windows AND NOT CORE_SYSTEM_NAME STREQUAL windowsstore)
  if(HAVE_SSE)
    target_compile_options(${CORE_LIBRARY} PRIVATE -msse)
  endif()
  if(HAVE_SSE2)
    target_compile_options(${CORE_LIBRARY} PRIVATE -msse2)
  endif()
endif()
