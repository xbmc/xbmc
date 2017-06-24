if(VAAPI_FOUND)
list(APPEND SOURCES ../linux/VAAPI.cpp)
list(APPEND HEADERS ../linux/VAAPI.h)
endif()

if(VDPAU_FOUND)
list(APPEND SOURCES ../linux/VDPAU.cpp)
list(APPEND HEADERS ../linux/VDPAU.h)
endif()

core_add_library(linuxdecoders_X11)