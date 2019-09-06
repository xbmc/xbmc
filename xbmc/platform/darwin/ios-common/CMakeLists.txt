set(SOURCES AnnounceReceiver.mm
            DarwinEmbedKeyboard.mm
            DarwinEmbedKeyboardView.mm
            DarwinEmbedNowPlayingInfoManager.mm
            DarwinNSUserDefaults.mm)

set(HEADERS AnnounceReceiver.h
            DarwinEmbedKeyboard.h
            DarwinEmbedKeyboardView.h
            DarwinEmbedNowPlayingInfoManager.h
            DarwinNSUserDefaults.h)

core_add_library(platform_ios-common)
