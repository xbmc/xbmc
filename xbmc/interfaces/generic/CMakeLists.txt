set(SOURCES ILanguageInvoker.cpp
            LanguageInvokerThread.cpp
            RunningScriptObserver.cpp
            ScriptInvocationManager.cpp)

set(HEADERS ILanguageInvocationHandler.h
            ILanguageInvoker.h
            LanguageInvokerThread.h
            RunningScriptsHandler.h
            RunningScriptObserver.h
            ScriptInvocationManager.h)

core_add_library(generic_interface)
