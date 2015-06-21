set(SOURCES ApplicationMessenger.cpp)

set(HEADERS ApplicationMessenger.h
            IMessageTarget.h
            ThreadMessage.h)

core_add_library(messaging)
