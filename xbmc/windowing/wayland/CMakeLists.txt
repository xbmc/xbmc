# from ArchSetup.cmake
set_source_files_properties(${WAYLAND_EXTRA_PROTOCOL_GENERATED_DIR}/wayland-extra-protocols.cpp
                            ${WAYLAND_EXTRA_PROTOCOL_GENERATED_DIR}/wayland-extra-protocols.hpp
                            PROPERTIES GENERATED TRUE)

set(SOURCES Connection.cpp
            OptionalsReg.cpp
            Output.cpp
            InputProcessorKeyboard.h
            InputProcessorPointer.h
            InputProcessorTouch.h
            OSScreenSaverIdleInhibitUnstableV1.cpp
            Registry.cpp
            Seat.cpp
            SeatInputProcessing.cpp
            SeatSelection.cpp
            ShellSurface.cpp
            ShellSurfaceWlShell.cpp
            ShellSurfaceXdgShell.cpp
            ShellSurfaceXdgShellUnstableV6.cpp
            Util.cpp
            VideoSyncWpPresentation.cpp
            ${WAYLAND_EXTRA_PROTOCOL_GENERATED_DIR}/wayland-extra-protocols.cpp
            WindowDecorator.cpp
            WinEventsWayland.cpp
            WinSystemWayland.cpp
            XkbcommonKeymap.cpp)

set(HEADERS Connection.h
            OptionalsReg.h
            Output.h
            InputProcessorKeyboard.cpp
            InputProcessorPointer.cpp
            InputProcessorTouch.cpp
            OSScreenSaverIdleInhibitUnstableV1.h
            Registry.h
            Seat.h
            SeatInputProcessing.h
            SeatSelection.h
            ShellSurface.h
            ShellSurfaceWlShell.h
            ShellSurfaceXdgShell.h
            ShellSurfaceXdgShellUnstableV6.h
            Signals.h
            VideoSyncWpPresentation.h
            ${WAYLAND_EXTRA_PROTOCOL_GENERATED_DIR}/wayland-extra-protocols.hpp
            WindowDecorator.h
            WinEventsWayland.h
            WinSystemWayland.h
            XkbcommonKeymap.h)

if(TARGET EGL::EGL)
  list(APPEND SOURCES WinSystemWaylandEGLContext.cpp)
  list(APPEND HEADERS WinSystemWaylandEGLContext.h)
endif()

if(TARGET OpenGL::GL)
  list(APPEND SOURCES WinSystemWaylandEGLContextGL.cpp)
  list(APPEND HEADERS WinSystemWaylandEGLContextGL.h)
endif()
if(TARGET OpenGL::GLES)
  list(APPEND SOURCES WinSystemWaylandEGLContextGLES.cpp)
  list(APPEND HEADERS WinSystemWaylandEGLContextGLES.h)
endif()
if(TARGET_WEBOS)
  set_source_files_properties(${WAYLAND_EXTRA_PROTOCOL_GENERATED_DIR}/wayland-webos-protocols.cpp
                              ${WAYLAND_EXTRA_PROTOCOL_GENERATED_DIR}/wayland-webos-protocols.hpp
                              PROPERTIES GENERATED TRUE)

  list(APPEND SOURCES OSScreenSaverWebOS.cpp
                      SeatWebOS.cpp
                      ShellSurfaceWebOSShell.cpp
                      WinSystemWaylandWebOS.cpp
                      ${WAYLAND_EXTRA_PROTOCOL_GENERATED_DIR}/wayland-webos-protocols.cpp)
  list(APPEND HEADERS OSScreenSaverWebOS.h
                      SeatWebOS.h
                      ShellSurfaceWebOSShell.h
                      WinSystemWaylandWebOS.h
                      ${WAYLAND_EXTRA_PROTOCOL_GENERATED_DIR}/wayland-webos-protocols.hpp)
endif()

core_add_library(windowing_WAYLAND)
