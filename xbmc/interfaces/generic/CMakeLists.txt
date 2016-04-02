set(SOURCES ILanguageInvoker.cpp
            LanguageInvokerThread.cpp
            ScriptInvocationManager.cpp)

set(HEADERS ILanguageInvocationHandler.h
            ILanguageInvoker.h
            LanguageInvokerThread.h
            ScriptInvocationManager.h)

core_add_library(generic_interface)
