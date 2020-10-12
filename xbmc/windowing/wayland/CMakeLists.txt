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

if(EGL_FOUND)
  list(APPEND SOURCES WinSystemWaylandEGLContext.cpp)
  list(APPEND HEADERS WinSystemWaylandEGLContext.h)
endif()

if(OPENGL_FOUND)
  list(APPEND SOURCES WinSystemWaylandEGLContextGL.cpp)
  list(APPEND HEADERS WinSystemWaylandEGLContextGL.h)
endif()
if(OPENGLES_FOUND)
  list(APPEND SOURCES WinSystemWaylandEGLContextGLES.cpp)
  list(APPEND HEADERS WinSystemWaylandEGLContextGLES.h)
endif()


core_add_library(windowing_WAYLAND)
