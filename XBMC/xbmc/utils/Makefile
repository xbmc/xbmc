INCLUDES=-I. -I.. -I../linux -I../../guilib

SRCS=AlarmClock.cpp \
     Archive.cpp \
     CharsetConverter.cpp \
     CriticalSection.cpp \
     DelayController.cpp \
     Event.cpp \
     fstrcmp.cpp \
     GUIInfoManager.cpp \
     HTMLTable.cpp \
     HTMLUtil.cpp \
     HttpHeader.cpp \
     IMDB.cpp \
     InfoLoader.cpp \
     log.cpp \
     MusicAlbumInfo.cpp \
     MusicInfoScraper.cpp \
     RegExp.cpp \
     RssReader.cpp \
     ScraperParser.cpp \
     SingleLock.cpp \
     Splash.cpp \
     Stopwatch.cpp \
     SystemInfo.cpp \
     TuxBoxUtil.cpp \
     UdpClient.cpp \
     Weather.cpp \
     Thread.cpp \
     HTTP.cpp \
     SharedSection.cpp \
     Win32Exception.cpp \
     CPUInfo.cpp \
     PCMAmplifier.cpp \
     LabelFormatter.cpp \
     Network.cpp \
     BitstreamStats.cpp \
     PerformanceStats.cpp \
     PerformanceSample.cpp \
     LCDFactory.cpp \
     LCD.cpp \
     EventServer.cpp \
     EventPacket.cpp \
     EventClient.cpp \
     Socket.cpp \
     Fanart.cpp \
     ScraperUrl.cpp \
     MusicArtistInfo.cpp \
     RssFeed.cpp \
     Mutex.cpp \
     md5.cpp \
     ArabicShaping.cpp \
     AsyncFileCopy.cpp \
     DbusServer.cpp \
     DBusMessage.cpp \

LIB=utils.a

include ../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
