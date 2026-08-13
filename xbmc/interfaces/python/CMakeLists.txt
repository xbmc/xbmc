set(SOURCES AddonPythonInvoker.cpp
            CallbackHandler.cpp
            ContextItemAddonInvoker.cpp
            LanguageHook.cpp
            PythonInvoker.cpp
            PythonToCppException.cpp
            XBPython.cpp
            PyContext.cpp)

set(HEADERS AddonPythonInvoker.h
            CallbackHandler.h
            ContextItemAddonInvoker.h
            LanguageHook.h
            PyContext.h
            PythonInvoker.h
            PythonToCppException.h
            pythreadstate.h
            XBPython.h)

core_add_library(python_interface)
