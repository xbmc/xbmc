set(SOURCES AnnounceReceiver.mm
            DarwinEmbedKeyboard.mm
            DarwinEmbedKeyboardView.mm
            DarwinEmbedNowPlayingInfoManager.mm)

set(HEADERS AnnounceReceiver.h
            DarwinEmbedKeyboard.h
            DarwinEmbedKeyboardView.h
            DarwinEmbedNowPlayingInfoManager.h)

core_add_library(platform_ios-common)
