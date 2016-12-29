if(MICROHTTPD_FOUND)
  set(SOURCES HTTPPythonInvoker.cpp
              HTTPPythonWsgiInvoker.cpp)

  set(HEADERS HTTPPythonInvoker.h
              HTTPPythonRequest.h
              HTTPPythonWsgiInvoker.h)

  core_add_library(network_httprequesthandlers_python)
endif()
