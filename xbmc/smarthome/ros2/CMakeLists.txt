set(HEADERS IRos2.h)

if (ENABLE_ROS2)
  list(APPEND SOURCES Ros2.cpp
                      Ros2InputPublisher.cpp
                      Ros2Node.cpp
                      Ros2NowPlayingPublisher.cpp
                      Ros2PowerMeterManager.cpp
                      Ros2PowerMeterSubscriber.cpp
                      Ros2PropertyManager.cpp
                      Ros2PropertySubscriber.cpp
                      Ros2SystemHealthManager.cpp
                      Ros2SystemHealthSubscriber.cpp
                      Ros2Translator.cpp
                      Ros2VehicleManager.cpp
                      Ros2VehicleSubscriber.cpp
                      Ros2VideoSubscription.cpp
  )
  list(APPEND HEADERS Ros2.h
                      Ros2InputPublisher.h
                      Ros2Node.h
                      Ros2NowPlayingPublisher.h
                      Ros2PowerMeterManager.h
                      Ros2PowerMeterSubscriber.h
                      Ros2PropertyManager.h
                      Ros2PropertySubscriber.h
                      Ros2SystemHealthManager.h
                      Ros2SystemHealthSubscriber.h
                      Ros2Translator.h
                      Ros2VehicleManager.h
                      Ros2VehicleSubscriber.h
                      Ros2VideoSubscription.h
  )

  core_add_library(smarthome_ros2)

  # TODO: Why is this needed? Link error started occuring after an image_transport PR
  target_link_libraries(smarthome_ros2 PUBLIC
    image_transport::image_transport
  )
  # TODO: Why is this needed? Missing oasis_msgs headers after updating to Ubuntu 23.10
  find_package(oasis_msgs REQUIRED)
  target_link_libraries(smarthome_ros2 PUBLIC
    ${oasis_msgs_LIBRARIES}
  )
  target_include_directories(smarthome_ros2 PUBLIC
    ${oasis_msgs_INCLUDE_DIRS}
  )
else()
  if (NOT ENABLE_STATIC_LIBS)
    core_add_library(smarthome_ros2)
  endif()
endif()
