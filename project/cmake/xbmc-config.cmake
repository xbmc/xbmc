SET(XBMC_INCLUDE_DIR /usr/local/include)
LIST(APPEND CMAKE_MODULE_PATH /usr/local/lib/xbmc)
ADD_DEFINITIONS(-DTARGET_POSIX -DTARGET_LINUX -D_LINUX)

include(xbmc-addon-helpers)
