set(SOURCES GenericTouchActionHandler.cpp
            GenericTouchInputHandler.cpp
            GenericTouchPinchDetector.cpp
            GenericTouchRotateDetector.cpp
            GenericTouchSwipeDetector.cpp)

set(HEADERS GenericTouchActionHandler.h
            GenericTouchInputHandler.h
            GenericTouchPinchDetector.h
            GenericTouchRotateDetector.h
            GenericTouchSwipeDetector.h
            IGenericTouchGestureDetector.h)

core_add_library(input_touch_generic)
