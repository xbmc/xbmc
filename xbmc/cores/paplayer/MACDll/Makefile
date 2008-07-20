OBJS= \
Source/Console/Console.o		\
Source/MACLib/APECompress.o		\
Source/MACLib/APECompressCore.o	\
Source/MACLib/APECompressCreate.o	\
Source/MACLib/APEDecompress.o	\
Source/MACLib/APEInfo.o		\
Source/MACLib/APELink.o		\
Source/MACLib/APESimple.o		\
Source/MACLib/APETag.o		\
Source/MACLib/BitArray.o		\
Source/MACLib/MACLib.o		\
Source/MACLib/MACProgressHelper.o	\
Source/MACLib/NNFilter.o		\
Source/MACLib/NewPredictor.o		\
Source/MACLib/Prepare.o		\
Source/MACLib/UnBitArray.o		\
Source/MACLib/UnBitArrayBase.o	\
Source/MACLib/WAVInputSource.o	\
Source/MACLib/APEHeader.o \
Source/Shared/GlobalFunctions.o	\
Source/Shared/StdLibFileIO.o		\
Source/Shared/WinFileIO.o		\
Source/MACLib/MD5.o		\
Source/Shared/CharacterHelper.o \
Source/Shared/CircleBuffer.o \
Source/MACLib/Old/APEDecompressOld.o \
Source/MACLib/Old/APEDecompressCore.o \
Source/MACLib/Old/Anti-Predictor.o \
Source/MACLib/Old/UnMAC.o \
Source/MACLib/Old/AntiPredictorExtraHigh.o \
Source/MACLib/Old/AntiPredictorHigh.o \
Source/MACLib/Old/AntiPredictorFast.o \
Source/MACLib/Old/UnBitArrayOld.o \
Source/MACLib/Old/AntiPredictorNormal.o \
Source/MACDll/MACDll.o

CFLAGS +=-ISource/Shared -ISource/MACLib -ISource/Console -s -O3 -pedantic -D__GNUC_IA32__ -DBUILD_CROSS_PLATFORM -D_LINUX

LIB=../../../../system/players/paplayer/MACDll-i486-linux.so

include ../../../../Makefile.include

lib: $(LIB)

$(LIB): $(OBJS)
	gcc -fPIC -shared -o $@ $(OBJS) `cat ../../DllLoader/exports/wrapper.def` ../../DllLoader/exports/wrapper.o
