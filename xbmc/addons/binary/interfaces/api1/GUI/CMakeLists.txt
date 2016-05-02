set(SOURCES AddonCallbacksGUI.cpp
            AddonGUIRenderingControl.cpp
            AddonGUIWindow.cpp)

set(HEADERS AddonCallbacksGUI.h
            AddonGUIRenderingControl.h
            AddonGUIWindow.h)

core_add_library(api1AddonCallbacks_GUI)
add_dependencies(api1AddonCallbacks_GUI libcpluff)

if(ENABLE_INTERNAL_FFMPEG)
  add_dependencies(api1AddonCallbacks_GUI ffmpeg)
endif()
