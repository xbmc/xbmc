set(SOURCES AddonPythonInvoker.cpp
            CallbackHandler.cpp
            ContextItemAddonInvoker.cpp
            LanguageHook.cpp
            PythonInvoker.cpp
            XBPython.cpp
            swig.cpp
            PyContext.cpp)

set(HEADERS AddonPythonInvoker.h
            CallbackHandler.h
            ContextItemAddonInvoker.h
            LanguageHook.h
            preamble.h
            PyContext.h
            PythonInvoker.h
            pythreadstate.h
            swig.h
            XBPython.h)

core_add_library(python_interface)
