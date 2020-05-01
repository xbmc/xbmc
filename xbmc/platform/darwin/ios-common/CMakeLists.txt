set(SOURCES AnnounceReceiver.mm
            CPUInfoDarwinEmbed.cpp
            DarwinEmbedKeyboard.mm
            DarwinEmbedKeyboardView.mm
            DarwinEmbedNowPlayingInfoManager.mm
            DarwinNSUserDefaults.mm
            NSData+GZIP.m)

set(HEADERS AnnounceReceiver.h
            CPUInfoDarwinEmbed.h
            DarwinEmbedKeyboard.h
            DarwinEmbedKeyboardView.h
            DarwinEmbedNowPlayingInfoManager.h
            DarwinNSUserDefaults.h
            NSData+GZIP.h)

set(UNITY_EXCLUDES ${SOURCES})

core_add_library(platform_ios-common)
