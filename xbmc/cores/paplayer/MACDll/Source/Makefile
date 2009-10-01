#
# Simple Makefile
#
# Frank Klemm
#

TARGET   = mac
INCLUDES = -IShared -IMACLib -IConsole
CPPOPT   = -s -O3 -pedantic -D__GNUC_IA32__ -DBUILD_CROSS_PLATFORM
COMPILER = g++

SOURCEFILES = \
Console/Console.cpp		\
MACLib/APECompress.cpp		\
MACLib/APECompressCore.cpp	\
MACLib/APECompressCreate.cpp	\
MACLib/APEDecompress.cpp	\
MACLib/APEInfo.cpp		\
MACLib/APELink.cpp		\
MACLib/APESimple.cpp		\
MACLib/APETag.cpp		\
MACLib/BitArray.cpp		\
MACLib/MACLib.cpp		\
MACLib/MACProgressHelper.cpp	\
MACLib/NNFilter.cpp		\
MACLib/NewPredictor.cpp		\
MACLib/Prepare.cpp		\
MACLib/UnBitArray.cpp		\
MACLib/UnBitArrayBase.cpp	\
MACLib/WAVInputSource.cpp	\
MACLib/APEHeader.cpp \
Shared/GlobalFunctions.cpp	\
Shared/StdLibFileIO.cpp		\
Shared/WinFileIO.cpp		\
MACLib/MD5.cpp		\
Shared/CharacterHelper.cpp \
Shared/CircleBuffer.cpp \
MACLib/Old/APEDecompressOld.cpp \
MACLib/Old/APEDecompressCore.cpp \
MACLib/Old/Anti-Predictor.cpp \
MACLib/Old/UnMAC.cpp \
MACLib/Old/AntiPredictorExtraHigh.cpp \
MACLib/Old/AntiPredictorHigh.cpp \
MACLib/Old/AntiPredictorFast.cpp \
MACLib/Old/UnBitArrayOld.cpp \
MACLib/Old/AntiPredictorNormal.cpp

# %.cpp: %.cpp %.h
#     $(COMPILER) -static $(CPPOPT) $(INCLUDES) -o $(TARGET)-static 

$(TARGET): $(SOURCEFILES)
	$(COMPILER) -shared -fPIC $(CPPOPT) $(INCLUDES) -o $(TARGET).so MACDll/MACDll.cpp $(SOURCEFILES) MACLib/Assembly/NNFilterAsm.o 
#     $(COMPILER) -static $(CPPOPT) $(INCLUDES) -o $(TARGET)-static $(SOURCEFILES) MACLib/Assembly/NNFilterAsm.o
#     $(COMPILER)         $(CPPOPT) $(INCLUDES) -o $(TARGET)        $(SOURCEFILES) MACLib/Assembly/NNFilterAsm.o

MACLib/Assembly/Assembly.o:
	nasm -IMACLib/Assembly -f elf -o MACLib/Assembly/NNFilterAsm.o MACLib/Assembly/Assembly.nas -l MACLib/Assembly/NNFilterAsm.lst

APE_Source.tar.bz2:
	@sh ./MakeSourceBall

test:
	@echo e4dd45d9b5ec4cc91f3bd2210a543df6
	@./$(TARGET) Adagio.ape - -d | md5sum

speed:
	@sync
	@cat Adagio.ape > /dev/null
	@sync
	time ./mac Adagio.ape /dev/null -d


# Samual Barber: Adagio for Strings (10:10.84)
#
# C version		203.01 sec
# First ASM version      76.89 sec
#                        76.85 sec
#			 77.12 sec
#                        76.23 sec
#			 75.67 sec
#                        76.26 sec
#                        76.79 sec
#                        76.70 sec
