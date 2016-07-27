set(SOURCES Platinum/Source/Core/PltAction.cpp
            Platinum/Source/Core/PltArgument.cpp
            Platinum/Source/Core/PltConstants.cpp
            Platinum/Source/Core/PltCtrlPoint.cpp
            Platinum/Source/Core/PltCtrlPointTask.cpp
            Platinum/Source/Core/PltDatagramStream.cpp
            Platinum/Source/Core/PltDeviceData.cpp
            Platinum/Source/Core/PltDeviceHost.cpp
            Platinum/Source/Core/PltEvent.cpp
            Platinum/Source/Core/PltHttp.cpp
            Platinum/Source/Core/PltHttpClientTask.cpp
            Platinum/Source/Core/PltHttpServer.cpp
            Platinum/Source/Core/PltHttpServerTask.cpp
            Platinum/Source/Core/PltIconsData.cpp
            Platinum/Source/Core/PltMimeType.cpp
            Platinum/Source/Core/PltProtocolInfo.cpp
            Platinum/Source/Core/PltService.cpp
            Platinum/Source/Core/PltSsdp.cpp
            Platinum/Source/Core/PltStateVariable.cpp
            Platinum/Source/Core/PltTaskManager.cpp
            Platinum/Source/Core/PltThreadTask.cpp
            Platinum/Source/Core/PltUPnP.cpp
            Platinum/Source/Devices/MediaServer/PltDidl.cpp
            Platinum/Source/Devices/MediaServer/PltFileMediaServer.cpp
            Platinum/Source/Devices/MediaServer/PltMediaBrowser.cpp
            Platinum/Source/Devices/MediaServer/PltMediaCache.cpp
            Platinum/Source/Devices/MediaServer/PltMediaItem.cpp
            Platinum/Source/Devices/MediaServer/PltMediaServer.cpp
            Platinum/Source/Devices/MediaServer/ConnectionManagerSCPD.cpp
            Platinum/Source/Devices/MediaServer/ContentDirectorySCPD.cpp
            Platinum/Source/Devices/MediaServer/ContentDirectorywSearchSCPD.cpp
            Platinum/Source/Devices/MediaServer/PltSyncMediaBrowser.cpp
            Neptune/Source/Core/Neptune.cpp
            Neptune/Source/Core/NptAutomaticCleaner.cpp
            Neptune/Source/Core/NptBase64.cpp
            Neptune/Source/Core/NptBufferedStreams.cpp
            Neptune/Source/Core/NptCommon.cpp
            Neptune/Source/Core/NptDataBuffer.cpp
            Neptune/Source/Core/NptDebug.cpp
            Neptune/Source/Core/NptFile.cpp
            Neptune/Source/Core/NptHash.cpp
            Neptune/Source/Core/NptHttp.cpp
            Neptune/Source/Core/NptList.cpp
            Neptune/Source/Core/NptLogging.cpp
            Neptune/Source/Core/NptMessaging.cpp
            Neptune/Source/Core/NptNetwork.cpp
            Neptune/Source/Core/NptQueue.cpp
            Neptune/Source/Core/NptResults.cpp
            Neptune/Source/Core/NptRingBuffer.cpp
            Neptune/Source/Core/NptSimpleMessageQueue.cpp
            Neptune/Source/Core/NptSockets.cpp
            Neptune/Source/Core/NptStreams.cpp
            Neptune/Source/Core/NptStrings.cpp
            Neptune/Source/Core/NptSystem.cpp
            Neptune/Source/Core/NptThreads.cpp
            Neptune/Source/Core/NptTime.cpp
            Neptune/Source/Core/NptTls.cpp
            Neptune/Source/Core/NptUri.cpp
            Neptune/Source/Core/NptUtils.cpp
            Neptune/Source/Core/NptXml.cpp
            Neptune/Source/System/Bsd/NptBsdSockets.cpp
            Neptune/Source/System/StdC/NptStdcDebug.cpp
            Neptune/Source/System/StdC/NptStdcEnvironment.cpp
            Platinum/Source/Devices/MediaRenderer/PltMediaRenderer.cpp
            Platinum/Source/Devices/MediaRenderer/PltMediaController.cpp
            Platinum/Source/Devices/MediaRenderer/AVTransportSCPD.cpp
            Platinum/Source/Devices/MediaRenderer/RdrConnectionManagerSCPD.cpp
            Platinum/Source/Devices/MediaRenderer/RenderingControlSCPD.cpp
            Platinum/Source/Devices/MediaConnect/X_MS_MediaReceiverRegistrarSCPD.cpp
            Platinum/Source/Devices/MediaConnect/PltMediaConnect.cpp
            Neptune/Source/System/Posix/NptPosixEnvironment.cpp)

if(NOT CORE_SYSTEM_NAME STREQUAL windows)
  list(APPEND SOURCES Neptune/Source/System/Posix/NptPosixSystem.cpp
                      Neptune/Source/System/Posix/NptSelectableMessageQueue.cpp
                      Neptune/Source/System/Posix/NptPosixQueue.cpp
                      Neptune/Source/System/Posix/NptPosixThreads.cpp
                      Neptune/Source/System/Posix/NptPosixTime.cpp
                      Neptune/Source/System/Bsd/NptBsdNetwork.cpp)
else()
  list(APPEND SOURCES Neptune/Source/System/Win32/NptWin32Console.cpp
                      Neptune/Source/System/Win32/NptWin32Debug.cpp
                      Neptune/Source/System/Win32/NptWin32DynamicLibraries.cpp
                      Neptune/Source/System/Win32/NptWin32File.cpp
                      Neptune/Source/System/Win32/NptWin32MessageQueue.cpp
                      Neptune/Source/System/Win32/NptWin32Network.cpp
                      Neptune/Source/System/Win32/NptWin32Queue.cpp
                      Neptune/Source/System/Win32/NptWin32SerialPort.cpp
                      Neptune/Source/System/Win32/NptWin32System.cpp
                      Neptune/Source/System/Win32/NptWin32Threads.cpp
                      Neptune/Source/System/Win32/NptWin32Time.cpp)
endif()

if(APPLE)
  list(APPEND SOURCES Neptune/Source/System/Apple/NptAppleAutoreleasePool.mm
                      Neptune/Source/System/Apple/NptAppleLogConfig.mm)
endif()

add_library(upnp STATIC ${SOURCES} ${HEADERS})
target_compile_definitions(upnp PUBLIC -DNPT_CONFIG_ENABLE_LOGGING)
target_include_directories(upnp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
                                       Platinum/Source/Core
                                       Platinum/Source/Platinum
                                       Platinum/Source/Devices/MediaConnect
                                       Platinum/Source/Devices/MediaRenderer
                                       Platinum/Source/Devices/MediaServer
                                       Platinum/Source/Extras
                                       Neptune/Source/Core
                                       Neptune/Source/System/Posix)
if(CORE_SYSTEM_NAME STREQUAL windows)
  target_include_directories(upnp PRIVATE Neptune/Source/System/Win32)
endif()
set_target_properties(upnp PROPERTIES FOLDER lib)
source_group_by_folder(upnp)
set(core_DEPENDS upnp ${core_DEPENDS} CACHE STRING "" FORCE)
