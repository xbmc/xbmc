#ifndef APE_HEADER_H
#define APE_HEADER_H

/*****************************************************************************************
APE header that all APE files have in common (old and new)
*****************************************************************************************/
struct APE_COMMON_HEADER
{
    char cID[4];                            // should equal 'MAC '
    uint16 nVersion;                        // version number * 1000 (3.81 = 3810)
};

/*****************************************************************************************
APE header structure for old APE files (3.97 and earlier)
*****************************************************************************************/
struct APE_HEADER_OLD 
{
    char cID[4];                            // should equal 'MAC '
    uint16 nVersion;                        // version number * 1000 (3.81 = 3810)
    uint16 nCompressionLevel;               // the compression level
    uint16 nFormatFlags;                    // any format flags (for future use)
    uint16 nChannels;                       // the number of channels (1 or 2)
    uint32 nSampleRate;                     // the sample rate (typically 44100)
    uint32 nHeaderBytes;                    // the bytes after the MAC header that compose the WAV header
    uint32 nTerminatingBytes;               // the bytes after that raw data (for extended info)
    uint32 nTotalFrames;                    // the number of frames in the file
    uint32 nFinalFrameBlocks;               // the number of samples in the final frame
};

struct APE_FILE_INFO;
class CIO;

/*****************************************************************************************
CAPEHeader - makes managing APE headers a little smoother (and the format change as of 3.98)
*****************************************************************************************/
class CAPEHeader
{

public:
    
    CAPEHeader(CIO * pIO);
    ~CAPEHeader();

    int Analyze(APE_FILE_INFO * pInfo);

protected:

    int AnalyzeCurrent(APE_FILE_INFO * pInfo);
    int AnalyzeOld(APE_FILE_INFO * pInfo);

    int FindDescriptor(BOOL bSeek);

    CIO * m_pIO;
};

#endif // #ifndef APE_HEADER_H

