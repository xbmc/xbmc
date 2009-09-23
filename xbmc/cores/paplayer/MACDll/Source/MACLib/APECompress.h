#ifndef APE_APECOMPRESS_H
#define APE_APECOMPRESS_H

#include "MACLib.h"
class CAPECompressCreate;

/*************************************************************************************************
CAPECompress - uses the CAPECompressHub to provide a simpler compression interface (with buffering, etc)
*************************************************************************************************/
class CAPECompress : public IAPECompress
{
public:

    CAPECompress();
    ~CAPECompress();

    // start encoding
    int Start(const wchar_t * pOutputFilename, const WAVEFORMATEX * pwfeInput, int nMaxAudioBytes, int nCompressionLevel = COMPRESSION_LEVEL_NORMAL, const void * pHeaderData = NULL, int nHeaderBytes = CREATE_WAV_HEADER_ON_DECOMPRESSION);
    int StartEx(CIO * pioOutput, const WAVEFORMATEX * pwfeInput, int nMaxAudioBytes, int nCompressionLevel = COMPRESSION_LEVEL_NORMAL, const void * pHeaderData = NULL, int nHeaderBytes = CREATE_WAV_HEADER_ON_DECOMPRESSION);
    
    // add data / compress data

    // allows linear, immediate access to the buffer (fast)
    int GetBufferBytesAvailable();
    int UnlockBuffer(int nBytesAdded, BOOL bProcess = TRUE);
    unsigned char * LockBuffer(int * pBytesAvailable);
    
    // slower, but easier than locking and unlocking (copies data)
    int AddData(unsigned char * pData, int nBytes);
    
    // use a CIO (input source) to add data
    int AddDataFromInputSource(CInputSource * pInputSource, int nMaxBytes = -1, int * pBytesAdded = NULL);
    
    // finish / kill
    int Finish(unsigned char * pTerminatingData, int nTerminatingBytes, int nWAVTerminatingBytes);
    int Kill();
    
private:
    
    int    ProcessBuffer(BOOL bFinalize = FALSE);
    
    CSmartPtr<CAPECompressCreate> m_spAPECompressCreate;

    int                m_nBufferHead;
    int                m_nBufferTail;
    int                m_nBufferSize;
    unsigned char * m_pBuffer;
    BOOL            m_bBufferLocked;

    CIO    *            m_pioOutput;
    BOOL            m_bOwnsOutputIO;
    WAVEFORMATEX    m_wfeInput;

};

#endif // #ifndef APE_APECOMPRESS_H
