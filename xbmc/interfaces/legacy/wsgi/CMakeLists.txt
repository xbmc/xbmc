if(TARGET ${APP_NAME_LC}::MicroHttpd)
  set(SOURCES WsgiErrorStream.cpp
              WsgiInputStream.cpp
              WsgiResponseBody.cpp
              WsgiResponse.cpp)

  set(HEADERS WsgiErrorStream.h
              WsgiInputStream.h
              WsgiResponse.h
              WsgiResponseBody.h)

  core_add_library(legacy_interface_wsgi)
endif()
