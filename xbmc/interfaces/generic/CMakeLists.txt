set(SOURCES HeldNotifications.cpp
            ILanguageInvoker.cpp
            LanguageInvokerThread.cpp
            RunningScriptObserver.cpp
            ScriptInvocationManager.cpp
            ScriptRunner.cpp)

set(HEADERS HeldNotifications.h
            ILanguageInvocationHandler.h
            ILanguageInvoker.h
            LanguageInvokerThread.h
            RunningScriptsHandler.h
            RunningScriptObserver.h
            ScriptInvocationManager.h
            ScriptRunner.h)

core_add_library(generic_interface)
