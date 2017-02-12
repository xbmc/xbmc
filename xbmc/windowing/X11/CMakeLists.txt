if (X_FOUND)

  set(SOURCES GLContextEGL.cpp
              GLContextGLX.cpp
              GLContext.cpp
              WinSystemX11GLContext.cpp
              WinSystemX11GLESContext.cpp
              XRandR.cpp
              WinSystemX11.cpp)

  set(HEADERS GLContext.h
              GLContextEGL.h
              GLContextGLX.h
              WinSystemX11GLContext.h
              WinSystemX11GLESContext.h
              XRandR.h
              WinSystemX11.h)

  core_add_library(windowing_X11)

endif()
