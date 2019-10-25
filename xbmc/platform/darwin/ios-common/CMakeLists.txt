set(SOURCES AnnounceReceiver.mm
            DarwinEmbedNowPlayingInfoManager.mm
            IOSKeyboard.mm
            IOSKeyboardView.mm)

set(HEADERS AnnounceReceiver.h
            DarwinEmbedNowPlayingInfoManager.h
            IOSKeyboard.h
            IOSKeyboardView.h)

core_add_library(platform_ios-common)
