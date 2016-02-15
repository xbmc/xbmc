set(SOURCES AddonBuiltins.cpp
            ApplicationBuiltins.cpp
            Builtins.cpp
            CECBuiltins.cpp
            GUIBuiltins.cpp
            GUIControlBuiltins.cpp
            GUIContainerBuiltins.cpp
            LibraryBuiltins.cpp
            OpticalBuiltins.cpp
            PictureBuiltins.cpp
            PlayerBuiltins.cpp
            ProfileBuiltins.cpp
            PVRBuiltins.cpp
            SkinBuiltins.cpp
            SystemBuiltins.cpp
            WeatherBuiltins.cpp)

set(HEADERS AddonBuiltins.h
            AndroidBuiltins.h
            ApplicationBuiltins.h
            Builtins.h
            CECBuiltins.h
            GUIBuiltins.h
            GUIContainerBuiltins.h
            GUIControlBuiltins.h
            LibraryBuiltins.h
            OpticalBuiltins.h
            PictureBuiltins.h
            PlayerBuiltins.h
            ProfileBuiltins.h
            PVRBuiltins.h
            SkinBuiltins.h
            SystemBuiltins.h
            WeatherBuiltins.h)

core_add_library(interfaces_builtins)
add_dependencies(interfaces_builtins libcpluff)

if(ENABLE_INTERNAL_FFMPEG)
  add_dependencies(interfaces_builtins ffmpeg)
endif()
