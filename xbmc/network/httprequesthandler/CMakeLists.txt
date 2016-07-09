set(SOURCES HTTPFileHandler.cpp
            HTTPImageHandler.cpp
            HTTPImageTransformationHandler.cpp
            HTTPJsonRpcHandler.cpp
            HTTPPythonHandler.cpp
            HTTPRequestHandlerUtils.cpp
            HTTPVfsHandler.cpp
            HTTPWebinterfaceAddonsHandler.cpp
            HTTPWebinterfaceHandler.cpp
            IHTTPRequestHandler.cpp)

set(HEADERS HTTPFileHandler.h
            HTTPImageHandler.h
            HTTPImageTransformationHandler.h
            HTTPJsonRpcHandler.h
            HTTPPythonHandler.h
            HTTPRequestHandlerUtils.h
            HTTPVfsHandler.h
            HTTPWebinterfaceAddonsHandler.h
            HTTPWebinterfaceHandler.h
            IHTTPRequestHandler.h)

core_add_library(network_httprequesthandlers)
