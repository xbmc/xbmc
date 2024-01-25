set(SOURCES AgentController.cpp
            AgentInput.cpp
            AgentJoystick.cpp
            AgentKeyboard.cpp
            AgentMouse.cpp
)

set(HEADERS AgentController.h
            AgentInput.h
            AgentJoystick.h
            AgentKeyboard.h
            AgentMouse.h
)

core_add_library(games_agents_input)
