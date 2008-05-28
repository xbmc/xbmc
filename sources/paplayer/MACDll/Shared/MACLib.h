/*****************************************************************************************
Monkey's Audio MACLib.h (include for using MACLib.lib in your projects)
Copyright (C) 2000-2003 by Matthew T. Ashland   All Rights Reserved.

Overview:

There are two main interfaces... create one (using CreateIAPExxx) and go to town:

    IAPECompress - for creating APE files
    IAPEDecompress - for decompressing and analyzing APE files

Note(s):

Unless otherwise specified, functions return ERROR_SUCCESS (0) on success and an 
error code on failure.

The terminology "Sample" refers to a single sample value, and "Block" refers 
to a collection    of "Channel" samples.  For simplicity, MAC typically uses blocks
everywhere so that channel mis-alignment cannot happen. (i.e. on a CD, a sample is
2 bytes and a block is 4 bytes ([2 bytes per sample] * [2 channels] = 4 bytes))

Questions / Suggestions:

Please direct questions or comments to the Monkey's Audio developers board:
http://www.monkeysaudio.com/cgi-bin/YaBB/YaBB.cgi -> Developers
or, if necessary, matt @ monkeysaudio.com
*****************************************************************************************/

#ifndef APE_MACLIB_H
#define APE_MACLIB_H

/*************************************************************************************************
APE File Format Overview: (pieces in order -- only valid for the latest version APE files)

    JUNK - any amount of "junk" before the APE_DESCRIPTOR (so people that put ID3v2 tags on the files aren't hosed)
    APE_DESCRIPTOR - defines the sizes (and offsets) of all the pieces, as well as the MD5 checksum
    APE_HEADER - describes all of the necessary information about the APE file
    SEEK TABLE - the table that represents seek offsets [optional]
    HEADER DATA - the pre-audio data from the original file [optional]
    APE FRAMES - the actual compressed audio (broken into frames for seekability)
    TERMINATING DATA - the post-audio data from the original file [optional]
    TAG - describes all the properties of the file [optional]

Notes:

    Junk:

    This block may not be supported in the future, so don't write any software that adds meta data
    before the APE_DESCRIPTOR.  Please use the APE Tag for any meta data.

    Seek Table:

    A 32-bit unsigned integer array of offsets from the header to the frame data.  May become "delta" 
    values someday to better suit huge files.

    MD5 Hash:

    Since the header is the last part written to an APE file, you must calculate the MD5 checksum out of order.
    So, you first calculate from the tail of the seek table to the end of the terminating data.
    Then, go back and do from the end of the descriptor to the tail of the seek table.
    You may wish to just cache the header data when starting and run it last, so you don't 
    need to seek back in the I/O.
*************************************************************************************************/

/*****************************************************************************************
Defines
*****************************************************************************************/
#define COMPRESSION_LEVEL_FAST          1000
#define COMPRESSION_LEVEL_NORMAL        2000
#define COMPRESSION_LEVEL_HIGH          3000
#define COMPRESSION_LEVEL_EXTRA_HIGH    4000
#define COMPRESSION_LEVEL_INSANE        5000

#define MAC_FORMAT_FLAG_8_BIT                 1    // is 8-bit [OBSOLETE]
#define MAC_FORMAT_FLAG_CRC                   2    // uses the new CRC32 error detection [OBSOLETE]
#define MAC_FORMAT_FLAG_HAS_PEAK_LEVEL        4    // uint32 nPeakLevel after the header [OBSOLETE]
#define MAC_FORMAT_FLAG_24_BIT                8    // is 24-bit [OBSOLETE]
#define MAC_FORMAT_FLAG_HAS_SEEK_ELEMENTS    16    // has the number of seek elements after the peak level
#define MAC_FORMAT_FLAG_CREATE_WAV_HEADER    32    // create the wave header on decompression (not stored)

#define CREATE_WAV_HEADER_ON_DECOMPRESSION    -1
#define MAX_AUDIO_BYTES_UNKNOWN -1

typedef void (__stdcall * APE_PROGRESS_CALLBACK) (int);

/*****************************************************************************************
WAV header structure
*****************************************************************************************/
struct WAVE_HEADER
{
    // RIFF header
    char cRIFFHeader[4];
    unsigned int nRIFFBytes;

    // data type
    char cDataTypeID[4];

    // wave format
    char cFormatHeader[4];
    unsigned int nFormatBytes;

    unsigned short nFormatTag;
    unsigned short nChannels;
    unsigned int nSamplesPerSec;
    unsigned int nAvgBytesPerSec;
    unsigned short nBlockAlign;
    unsigned short nBitsPerSample;
    
    // data chunk header
    char cDataHeader[4];
    unsigned int nDataBytes;
};

/*****************************************************************************************
APE_DESCRIPTOR structure (file header that describes lengths, offsets, etc.)
*****************************************************************************************/
struct APE_DESCRIPTOR
{
    char    cID[4];                             // should equal 'MAC '
    uint16  nVersion;                           // version number * 1000 (3.81 = 3810)

    uint32  nDescriptorBytes;                   // the number of descriptor bytes (allows later expansion of this header)
    uint32  nHeaderBytes;                       // the number of header APE_HEADER bytes
    uint32  nSeekTableBytes;                    // the number of bytes of the seek table
    uint32  nHeaderDataBytes;                   // the number of header data bytes (from original file)
    uint32  nAPEFrameDataBytes;                 // the number of bytes of APE frame data
    uint32  nAPEFrameDataBytesHigh;             // the high order number of APE frame data bytes
    uint32  nTerminatingDataBytes;              // the terminating data of the file (not including tag data)

    uint8   cFileMD5[16];                       // the MD5 hash of the file (see notes for usage... it's a littly tricky)
};

/*****************************************************************************************
APE_HEADER structure (describes the format, duration, etc. of the APE file)
*****************************************************************************************/
struct APE_HEADER
{
    uint16    nCompressionLevel;                 // the compression level (see defines I.E. COMPRESSION_LEVEL_FAST)
    uint16    nFormatFlags;                      // any format flags (for future use)

    uint32    nBlocksPerFrame;                   // the number of audio blocks in one frame
    uint32    nFinalFrameBlocks;                 // the number of audio blocks in the final frame
    uint32    nTotalFrames;                      // the total number of frames

    uint16    nBitsPerSample;                    // the bits per sample (typically 16)
    uint16    nChannels;                         // the number of channels (1 or 2)
    uint32    nSampleRate;                       // the sample rate (typically 44100)
};

/*************************************************************************************************
Classes (fully defined elsewhere)
*************************************************************************************************/
class CIO;
class CInputSource;
class CAPEInfo;

/*************************************************************************************************
IAPEDecompress fields - used when querying for information

Note(s):
-the distinction between APE_INFO_XXXX and APE_DECOMPRESS_XXXX is that the first is querying the APE
information engine, and the other is querying the decompressor, and since the decompressor can be
a range of an APE file (for APL), differences will arise.  Typically, use the APE_DECOMPRESS_XXXX
fields when querying for info about the length, etc. so APL will work properly. 
(i.e. (APE_INFO_TOTAL_BLOCKS != APE_DECOMPRESS_TOTAL_BLOCKS) for APL files)
*************************************************************************************************/
enum APE_DECOMPRESS_FIELDS
{
    APE_INFO_FILE_VERSION = 1000,               // version of the APE file * 1000 (3.93 = 3930) [ignored, ignored]
    APE_INFO_COMPRESSION_LEVEL = 1001,          // compression level of the APE file [ignored, ignored]
    APE_INFO_FORMAT_FLAGS = 1002,               // format flags of the APE file [ignored, ignored]
    APE_INFO_SAMPLE_RATE = 1003,                // sample rate (Hz) [ignored, ignored]
    APE_INFO_BITS_PER_SAMPLE = 1004,            // bits per sample [ignored, ignored]
    APE_INFO_BYTES_PER_SAMPLE = 1005,           // number of bytes per sample [ignored, ignored]
    APE_INFO_CHANNELS = 1006,                   // channels [ignored, ignored]
    APE_INFO_BLOCK_ALIGN = 1007,                // block alignment [ignored, ignored]
    APE_INFO_BLOCKS_PER_FRAME = 1008,           // number of blocks in a frame (frames are used internally)  [ignored, ignored]
    APE_INFO_FINAL_FRAME_BLOCKS = 1009,         // blocks in the final frame (frames are used internally) [ignored, ignored]
    APE_INFO_TOTAL_FRAMES = 1010,               // total number frames (frames are used internally) [ignored, ignored]
    APE_INFO_WAV_HEADER_BYTES = 1011,           // header bytes of the decompressed WAV [ignored, ignored]
    APE_INFO_WAV_TERMINATING_BYTES = 1012,      // terminating bytes of the decompressed WAV [ignored, ignored]
    APE_INFO_WAV_DATA_BYTES = 1013,             // data bytes of the decompressed WAV [ignored, ignored]
    APE_INFO_WAV_TOTAL_BYTES = 1014,            // total bytes of the decompressed WAV [ignored, ignored]
    APE_INFO_APE_TOTAL_BYTES = 1015,            // total bytes of the APE file [ignored, ignored]
    APE_INFO_TOTAL_BLOCKS = 1016,               // total blocks of audio data [ignored, ignored]
    APE_INFO_LENGTH_MS = 1017,                  // length in ms (1 sec = 1000 ms) [ignored, ignored]
    APE_INFO_AVERAGE_BITRATE = 1018,            // average bitrate of the APE [ignored, ignored]
    APE_INFO_FRAME_BITRATE = 1019,              // bitrate of specified APE frame [frame index, ignored]
    APE_INFO_DECOMPRESSED_BITRATE = 1020,       // bitrate of the decompressed WAV [ignored, ignored]
    APE_INFO_PEAK_LEVEL = 1021,                 // peak audio level (obsolete) (-1 is unknown) [ignored, ignored]
    APE_INFO_SEEK_BIT = 1022,                   // bit offset [frame index, ignored]
    APE_INFO_SEEK_BYTE = 1023,                  // byte offset [frame index, ignored]
    APE_INFO_WAV_HEADER_DATA = 1024,            // error code [buffer *, max bytes]
    APE_INFO_WAV_TERMINATING_DATA = 1025,       // error code [buffer *, max bytes]
    APE_INFO_WAVEFORMATEX = 1026,               // error code [waveformatex *, ignored]
    APE_INFO_IO_SOURCE = 1027,                  // I/O source (CIO *) [ignored, ignored]
    APE_INFO_FRAME_BYTES = 1028,                // bytes (compressed) of the frame [frame index, ignored]
    APE_INFO_FRAME_BLOCKS = 1029,               // blocks in a given frame [frame index, ignored]
    APE_INFO_TAG = 1030,                        // point to tag (CAPETag *) [ignored, ignored]
    
    APE_DECOMPRESS_CURRENT_BLOCK = 2000,        // current block location [ignored, ignored]
    APE_DECOMPRESS_CURRENT_MS = 2001,           // current millisecond location [ignored, ignored]
    APE_DECOMPRESS_TOTAL_BLOCKS = 2002,         // total blocks in the decompressors range [ignored, ignored]
    APE_DECOMPRESS_LENGTH_MS = 2003,            // total blocks in the decompressors range [ignored, ignored]
    APE_DECOMPRESS_CURRENT_BITRATE = 2004,      // current bitrate [ignored, ignored]
    APE_DECOMPRESS_AVERAGE_BITRATE = 2005,      // average bitrate (works with ranges) [ignored, ignored]

    APE_INTERNAL_INFO = 3000,                   // for internal use -- don't use (returns APE_FILE_INFO *) [ignored, ignored]
};

/*************************************************************************************************
IAPEDecompress - interface for working with existing APE files (decoding, seeking, analyzing, etc.)
*************************************************************************************************/
class IAPEDecompress
{
public:

    // destructor (needed so implementation's destructor will be called)
    virtual ~IAPEDecompress() {}
    
    /*********************************************************************************************
    * Decompress / Seek
    *********************************************************************************************/
    
    //////////////////////////////////////////////////////////////////////////////////////////////
    // GetData(...) - gets raw decompressed audio
    // 
    // Parameters:
    //    char * pBuffer
    //        a pointer to a buffer to put the data into
    //    int nBlocks
    //        the number of audio blocks desired (see note at intro about blocks vs. samples)
    //    int * pBlocksRetrieved
    //        the number of blocks actually retrieved (could be less at end of file or on critical failure)
    //////////////////////////////////////////////////////////////////////////////////////////////
    virtual int GetData(char * pBuffer, int nBlocks, int * pBlocksRetrieved) = 0;

    //////////////////////////////////////////////////////////////////////////////////////////////
    // Seek(...) - seeks
    // 
    // Parameters:
    //    int nBlockOffset
    //        the block to seek to (see note at intro about blocks vs. samples)
    //////////////////////////////////////////////////////////////////////////////////////////////
    virtual int Seek(int nBlockOffset) = 0;

    /*********************************************************************************************
    * Get Information
    *********************************************************************************************/

    //////////////////////////////////////////////////////////////////////////////////////////////
    // GetInfo(...) - get information about the APE file or the state of the decompressor
    // 
    // Parameters:
    //    APE_DECOMPRESS_FIELDS Field
    //        the field we're querying (see APE_DECOMPRESS_FIELDS above for more info)
    //    int nParam1
    //        generic parameter... usage is listed in APE_DECOMPRESS_FIELDS
    //    int nParam2
    //        generic parameter... usage is listed in APE_DECOMPRESS_FIELDS
    //////////////////////////////////////////////////////////////////////////////////////////////
    virtual int GetInfo(APE_DECOMPRESS_FIELDS Field, int nParam1 = 0, int nParam2 = 0) = 0;
};

/*************************************************************************************************
IAPECompress - interface for creating APE files

Usage:

    To create an APE file, you Start(...), then add data (in a variety of ways), then Finish(...)
*************************************************************************************************/
class IAPECompress
{
public:

    // destructor (needed so implementation's destructor will be called)
    virtual ~IAPECompress() {}
    
    /*********************************************************************************************
    * Start
    *********************************************************************************************/
    
    //////////////////////////////////////////////////////////////////////////////////////////////
    // Start(...) / StartEx(...) - starts encoding
    // 
    // Parameters:
    //    CIO * pioOutput / const str_utf16 * pFilename
    //        the output... either a filename or an I/O source
    //    WAVEFORMATEX * pwfeInput
    //        format of the audio to encode (use FillWaveFormatEx() if necessary)
    //    int nMaxAudioBytes
    //        the absolute maximum audio bytes that will be encoded... encoding fails with a
    //        ERROR_APE_COMPRESS_TOO_MUCH_DATA if you attempt to encode more than specified here
    //        (if unknown, use MAX_AUDIO_BYTES_UNKNOWN to allocate as much storage in the seek table as
    //        possible... limit is then 2 GB of data (~4 hours of CD music)... this wastes around
    //        30kb, so only do it if completely necessary)
    //    int nCompressionLevel
    //        the compression level for the APE file (fast - extra high)
    //        (note: extra-high is much slower for little gain)
    //    const void * pHeaderData
    //        a pointer to a buffer containing the WAV header (data before the data block in the WAV)
    //        (note: use NULL for on-the-fly encoding... see next parameter)
    //    int nHeaderBytes
    //        number of bytes in the header data buffer (use CREATE_WAV_HEADER_ON_DECOMPRESSION and
    //        NULL for the pHeaderData and MAC will automatically create the appropriate WAV header
    //        on decompression)
    //////////////////////////////////////////////////////////////////////////////////////////////

    virtual int Start(const str_utf16 * pOutputFilename, const WAVEFORMATEX * pwfeInput, 
        int nMaxAudioBytes = MAX_AUDIO_BYTES_UNKNOWN, int nCompressionLevel = COMPRESSION_LEVEL_NORMAL, 
        const void * pHeaderData = NULL, int nHeaderBytes = CREATE_WAV_HEADER_ON_DECOMPRESSION) = 0;

    virtual int StartEx(CIO * pioOutput, const WAVEFORMATEX * pwfeInput, 
        int nMaxAudioBytes = MAX_AUDIO_BYTES_UNKNOWN, int nCompressionLevel = COMPRESSION_LEVEL_NORMAL, 
        const void * pHeaderData = NULL, int nHeaderBytes = CREATE_WAV_HEADER_ON_DECOMPRESSION) = 0;
    
    /*********************************************************************************************
    * Add / Compress Data
    *    - there are 3 ways to add data:
    *        1) simple call AddData(...)
    *        2) lock MAC's buffer, copy into it, and unlock (LockBuffer(...) / UnlockBuffer(...))
    *        3) from an I/O source (AddDataFromInputSource(...))
    *********************************************************************************************/

    //////////////////////////////////////////////////////////////////////////////////////////////
    // AddData(...) - adds data to the encoder
    // 
    // Parameters:
    //    unsigned char * pData
    //        a pointer to a buffer containing the raw audio data
    //    int nBytes
    //        the number of bytes in the buffer
    //////////////////////////////////////////////////////////////////////////////////////////////
    virtual int AddData(unsigned char * pData, int nBytes) = 0;
    
    //////////////////////////////////////////////////////////////////////////////////////////////
    // GetBufferBytesAvailable(...) - returns the number of bytes available in the buffer
    //    (helpful when locking)
    //////////////////////////////////////////////////////////////////////////////////////////////
    virtual int GetBufferBytesAvailable() = 0;

    //////////////////////////////////////////////////////////////////////////////////////////////
    // LockBuffer(...) - locks MAC's buffer so we can copy into it
    // 
    // Parameters:
    //    int * pBytesAvailable
    //        returns the number of bytes available in the buffer (DO NOT COPY MORE THAN THIS IN)
    //
    // Return:
    //    pointer to the buffer (add at that location)
    //////////////////////////////////////////////////////////////////////////////////////////////
    virtual unsigned char * LockBuffer(int * pBytesAvailable) = 0;

    //////////////////////////////////////////////////////////////////////////////////////////////
    // UnlockBuffer(...) - releases the buffer
    // 
    // Parameters:
    //    int nBytesAdded
    //        the number of bytes copied into the buffer
    //    BOOL bProcess
    //        whether MAC should process as much as possible of the buffer
    //////////////////////////////////////////////////////////////////////////////////////////////
    virtual int UnlockBuffer(int nBytesAdded, BOOL bProcess = TRUE) = 0;
    

    //////////////////////////////////////////////////////////////////////////////////////////////
    // AddDataFromInputSource(...) - use a CInputSource (input source) to add data
    //
    // Parameters:
    //    CInputSource * pInputSource
    //        a pointer to the input source
    //    int nMaxBytes
    //        the maximum number of bytes to let MAC add (-1 if MAC can add any amount)
    //    int * pBytesAdded
    //        returns the number of bytes added from the I/O source
    //////////////////////////////////////////////////////////////////////////////////////////////
    virtual int AddDataFromInputSource(CInputSource * pInputSource, int nMaxBytes = -1, int * pBytesAdded = NULL) = 0;
    
    /*********************************************************************************************
    * Finish / Kill
    *********************************************************************************************/

    //////////////////////////////////////////////////////////////////////////////////////////////
    // Finish(...) - ends encoding and finalizes the file
    // 
    // Parameters:
    //    unsigned char * pTerminatingData
    //        a pointer to a buffer containing the information to place at the end of the APE file
    //        (comprised of the WAV terminating data (data after the data block in the WAV) followed
    //        by any tag information)
    //    int nTerminatingBytes
    //        number of bytes in the terminating data buffer
    //    int nWAVTerminatingBytes
    //        the number of bytes of the terminating data buffer that should be appended to a decoded
    //        WAV file (it's basically nTerminatingBytes - the bytes that make up the tag)
    //////////////////////////////////////////////////////////////////////////////////////////////
    virtual int Finish(unsigned char * pTerminatingData, int nTerminatingBytes, int nWAVTerminatingBytes) = 0;

    //////////////////////////////////////////////////////////////////////////////////////////////
    // Kill(...) - stops encoding and deletes the output file
    // --- NOT CURRENTLY IMPLEMENTED ---
    //////////////////////////////////////////////////////////////////////////////////////////////
    virtual int Kill() = 0;
};

/*************************************************************************************************
Functions to create the interfaces

Usage:
    Interface creation returns a NULL pointer on failure (and fills error code if it was passed in)

Usage example:
    int nErrorCode;
    IAPEDecompress * pAPEDecompress = CreateIAPEDecompress("c:\\1.ape", &nErrorCode);
    if (pAPEDecompress == NULL)
    {
        // failure... nErrorCode will have specific code
    }

*************************************************************************************************/
extern "C"
{
    IAPEDecompress * __stdcall CreateIAPEDecompress(const str_utf16 * pFilename, int * pErrorCode = NULL);
    IAPEDecompress * __stdcall CreateIAPEDecompressEx(CIO * pIO, int * pErrorCode = NULL);
    IAPEDecompress * __stdcall CreateIAPEDecompressEx2(CAPEInfo * pAPEInfo, int nStartBlock = -1, int nFinishBlock = -1, int * pErrorCode = NULL);
    IAPECompress * __stdcall CreateIAPECompress(int * pErrorCode = NULL);
}

/*************************************************************************************************
Simple functions - see the SDK sample projects for usage examples
*************************************************************************************************/
extern "C"
{
    // process whole files
    DLLEXPORT int __stdcall CompressFile(const str_ansi * pInputFilename, const str_ansi * pOutputFilename, int nCompressionLevel = COMPRESSION_LEVEL_NORMAL, int * pPercentageDone = NULL, APE_PROGRESS_CALLBACK ProgressCallback = 0, int * pKillFlag = NULL);
    DLLEXPORT int __stdcall DecompressFile(const str_ansi * pInputFilename, const str_ansi * pOutputFilename, int * pPercentageDone, APE_PROGRESS_CALLBACK ProgressCallback, int * pKillFlag);
    DLLEXPORT int __stdcall ConvertFile(const str_ansi * pInputFilename, const str_ansi * pOutputFilename, int nCompressionLevel, int * pPercentageDone, APE_PROGRESS_CALLBACK ProgressCallback, int * pKillFlag);
    DLLEXPORT int __stdcall VerifyFile(const str_ansi * pInputFilename, int * pPercentageDone, APE_PROGRESS_CALLBACK ProgressCallback, int * pKillFlag); 

    DLLEXPORT int __stdcall CompressFileW(const str_utf16 * pInputFilename, const str_utf16 * pOutputFilename, int nCompressionLevel = COMPRESSION_LEVEL_NORMAL, int * pPercentageDone = NULL, APE_PROGRESS_CALLBACK ProgressCallback = 0, int * pKillFlag = NULL);
    DLLEXPORT int __stdcall DecompressFileW(const str_utf16 * pInputFilename, const str_utf16 * pOutputFilename, int * pPercentageDone, APE_PROGRESS_CALLBACK ProgressCallback, int * pKillFlag);
    DLLEXPORT int __stdcall ConvertFileW(const str_utf16 * pInputFilename, const str_utf16 * pOutputFilename, int nCompressionLevel, int * pPercentageDone, APE_PROGRESS_CALLBACK ProgressCallback, int * pKillFlag);
    DLLEXPORT int __stdcall VerifyFileW(const str_utf16 * pInputFilename, int * pPercentageDone, APE_PROGRESS_CALLBACK ProgressCallback, int * pKillFlag, BOOL bQuickVerifyIfPossible = FALSE); 

    // helper functions
    DLLEXPORT int __stdcall FillWaveFormatEx(WAVEFORMATEX * pWaveFormatEx, int nSampleRate = 44100, int nBitsPerSample = 16, int nChannels = 2);
    DLLEXPORT int __stdcall FillWaveHeader(WAVE_HEADER * pWAVHeader, int nAudioBytes, WAVEFORMATEX * pWaveFormatEx, int nTerminatingBytes = 0);
}

#endif // #ifndef APE_MACLIB_H
