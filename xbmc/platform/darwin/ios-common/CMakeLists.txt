set(SOURCES AnnounceReceiver.mm
            CPUInfoDarwinEmbed.cpp
            DarwinEmbedKeyboard.mm
            DarwinEmbedKeyboardView.mm
            DarwinEmbedNowPlayingInfoManager.mm
            DarwinEmbedUtils.mm
            DarwinNSUserDefaults.mm
            NSData+GZIP.m)

set(HEADERS AnnounceReceiver.h
            CPUInfoDarwinEmbed.h
            DarwinEmbedKeyboard.h
            DarwinEmbedKeyboardView.h
            DarwinEmbedNowPlayingInfoManager.h
            DarwinEmbedUtils.h
            DarwinNSUserDefaults.h
            NSData+GZIP.h)

core_add_library(platform_ios-common)
