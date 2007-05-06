INCLUDES=-I. -I.. -I../linux -I../../guilib

SRCS=AlarmClock.cpp Archive.cpp CharsetConverter.cpp CriticalSection.cpp DelayController.cpp DownloadQueue.cpp DownloadQueueManager.cpp Event.cpp fstrcmp.cpp GUIInfoManager.cpp HTMLTable.cpp HTMLUtil.cpp HttpHeader.cpp Idle.cpp IMDB.cpp InfoLoader.cpp KaiClient.cpp KaiRequestList.cpp log.cpp MusicAlbumInfo.cpp MusicInfoScraper.cpp RegExp.cpp RssReader.cpp ScraperParser.cpp SingleLock.cpp Splash.cpp Stopwatch.cpp SystemInfo.cpp Trainer.cpp TuxBoxUtil.cpp UdpClient.cpp Weather.cpp Thread.cpp HTTP.cpp SharedSection.cpp

LIB=utils.a

include ../../Makefile.include
-include Makefile.depend

utils.a : md5.o 

OBJS+=md5.o
