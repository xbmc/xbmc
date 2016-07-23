set(SOURCES AddonsOperations.cpp
            ApplicationOperations.cpp
            AudioLibrary.cpp
            FavouritesOperations.cpp
            FileItemHandler.cpp
            FileOperations.cpp
            GUIOperations.cpp
            InputOperations.cpp
            JSONRPC.cpp
            JSONServiceDescription.cpp
            PlayerOperations.cpp
            PlaylistOperations.cpp
            ProfilesOperations.cpp
            PVROperations.cpp
            SettingsOperations.cpp
            SystemOperations.cpp
            TextureOperations.cpp
            VideoLibrary.cpp
            XBMCOperations.cpp)

set(HEADERS AddonsOperations.h
            ApplicationOperations.h
            AudioLibrary.h
            FavouritesOperations.h
            FileItemHandler.h
            FileOperations.h
            GUIOperations.h
            IClient.h
            IJSONRPCAnnouncer.h
            InputOperations.h
            ITransportLayer.h
            JSONRPC.h
            JSONRPCUtils.h
            JSONServiceDescription.h
            JSONUtils.h
            PlayerOperations.h
            PlaylistOperations.h
            ProfilesOperations.h
            PVROperations.h
            SettingsOperations.h
            SystemOperations.h
            TextureOperations.h
            VideoLibrary.h
            XBMCOperations.h)

core_add_library(jsonrpc_interface)
add_dependencies(${CORE_LIBRARY} generate_json_header)
