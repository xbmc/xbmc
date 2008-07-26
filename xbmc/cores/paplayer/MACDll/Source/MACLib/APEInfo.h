/*****************************************************************************************
APEInfo.h
Copyright (C) 2000 by Matthew T. Ashland   All Rights Reserved.

Simple method for working with APE files... it encapsulates reading, writing and getting
file information.  Just create a CAPEInfo class, call OpenFile(), and use the class methods
to do whatever you need... the destructor will take care of any cleanup

Notes:
    -Most all functions return 0 upon success, and some error code (other than 0) on
    failure.  However, all of the file functions that are wrapped from the Win32 API
    return 0 on failure and some other number on success.  This applies to ReadFile, 
    WriteFile, SetFilePointer, etc...

WARNING:
    -This class driven system for using Monkey's Audio is still in development, so
    I can't make any guarantees that the classes and libraries won't change before
    everything gets finalized.  Use them at your own risk
*****************************************************************************************/

#ifndef APE_APEINFO_H
#define APE_APEINFO_H

#include "IO.h"
#include "APETag.h"
#include "MACLib.h"

/*****************************************************************************************
APE_FILE_INFO - structure which describes most aspects of an APE file 
(used internally for speed and ease)
*****************************************************************************************/
struct APE_FILE_INFO
{
    int nVersion;                                   // file version number * 1000 (3.93 = 3930)
    int nCompressionLevel;                          // the compression level
    int nFormatFlags;                               // format flags
    int nTotalFrames;                               // the total number frames (frames are used internally)
    int nBlocksPerFrame;                            // the samples in a frame (frames are used internally)
    int nFinalFrameBlocks;                          // the number of samples in the final frame
    int nChannels;                                  // audio channels
    int nSampleRate;                                // audio samples per second
    int nBitsPerSample;                             // audio bits per sample
    int nBytesPerSample;                            // audio bytes per sample
    int nBlockAlign;                                // audio block align (channels * bytes per sample)
    int nWAVHeaderBytes;                            // header bytes of the original WAV
    int nWAVDataBytes;                              // data bytes of the original WAV
    int nWAVTerminatingBytes;                       // terminating bytes of the original WAV
    int nWAVTotalBytes;                             // total bytes of the original WAV
    int nAPETotalBytes;                             // total bytes of the APE file
    int nTotalBlocks;                               // the total number audio blocks
    int nLengthMS;                                  // the length in milliseconds
    int nAverageBitrate;                            // the kbps (i.e. 637 kpbs)
    int nDecompressedBitrate;                       // the kbps of the decompressed audio (i.e. 1440 kpbs for CD audio)
    int nJunkHeaderBytes;                           // used for ID3v2, etc.
    int nSeekTableElements;                         // the number of elements in the seek table(s)

    CSmartPtr<uint32> spSeekByteTable;              // the seek table (byte)
    CSmartPtr<unsigned char> spSeekBitTable;        // the seek table (bits -- legacy)
    CSmartPtr<unsigned char> spWaveHeaderData;      // the pre-audio header data
    CSmartPtr<APE_DESCRIPTOR> spAPEDescriptor;      // the descriptor (only with newer files)
};

/*****************************************************************************************
Helper macros (sort of hacky)
*****************************************************************************************/
#define GET_USES_CRC(APE_INFO) (((APE_INFO)->GetInfo(APE_INFO_FORMAT_FLAGS) & MAC_FORMAT_FLAG_CRC) ? TRUE : FALSE)
#define GET_FRAMES_START_ON_BYTES_BOUNDARIES(APE_INFO) (((APE_INFO)->GetInfo(APE_INFO_FILE_VERSION) > 3800) ? TRUE : FALSE)
#define GET_USES_SPECIAL_FRAMES(APE_INFO) (((APE_INFO)->GetInfo(APE_INFO_FILE_VERSION) > 3820) ? TRUE : FALSE)
#define GET_IO(APE_INFO) ((CIO *) (APE_INFO)->GetInfo(APE_INFO_IO_SOURCE))
#define GET_TAG(APE_INFO) ((CAPETag *) (APE_INFO)->GetInfo(APE_INFO_TAG))

/*****************************************************************************************
CAPEInfo - use this for all work with APE files
*****************************************************************************************/
class CAPEInfo
{
public:
    
    // construction and destruction
    CAPEInfo(int * pErrorCode, const wchar_t * pFilename, CAPETag * pTag = NULL);
    CAPEInfo(int * pErrorCode, CIO * pIO, CAPETag * pTag = NULL);
    virtual ~CAPEInfo();

    // query for information
    long GetInfo(APE_DECOMPRESS_FIELDS Field, int nParam1 = 0, int nParam2 = 0);
    
private:

    // internal functions
    int GetFileInformation(BOOL bGetTagInformation = TRUE);
    int CloseFile();
    
    // internal variables
    BOOL m_bHasFileInformationLoaded;
    CSmartPtr<CIO> m_spIO;
    CSmartPtr<CAPETag> m_spAPETag;
    APE_FILE_INFO    m_APEFileInfo;
};

#endif // #ifndef APE_APEINFO_H
