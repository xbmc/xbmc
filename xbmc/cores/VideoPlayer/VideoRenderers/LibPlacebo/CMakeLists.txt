set(SOURCES libav_wrapper.c
            PlHelper.cpp
            PLD3D11Texture.cpp)

set(HEADERS 
            PlHelper.h
            PLD3D11Texture.h)
core_add_library(videorenderers-windows)
