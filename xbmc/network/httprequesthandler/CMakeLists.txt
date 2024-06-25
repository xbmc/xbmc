if(TARGET ${APP_NAME_LC}::MicroHttpd)
  set(SOURCES HTTPFileHandler.cpp
              HTTPImageHandler.cpp
              HTTPImageTransformationHandler.cpp
              HTTPJsonRpcHandler.cpp
              HTTPRequestHandlerUtils.cpp
              HTTPVfsHandler.cpp
              HTTPWebinterfaceAddonsHandler.cpp
              HTTPWebinterfaceHandler.cpp
              IHTTPRequestHandler.cpp)

  if(TARGET ${APP_NAME_LC}::Python)
    list(APPEND SOURCES HTTPPythonHandler.cpp)
  endif()

  set(HEADERS HTTPFileHandler.h
              HTTPImageHandler.h
              HTTPImageTransformationHandler.h
              HTTPJsonRpcHandler.h
              HTTPRequestHandlerUtils.h
              HTTPVfsHandler.h
              HTTPWebinterfaceAddonsHandler.h
              HTTPWebinterfaceHandler.h
              IHTTPRequestHandler.h)
  if(TARGET ${APP_NAME_LC}::Python)
    list(APPEND HEADERS HTTPPythonHandler.h)
  endif()

  core_add_library(network_httprequesthandlers)
endif()
