#ifndef APE_APECOMPRESSCREATE_H
#define APE_APECOMPRESSCREATE_H

#include "APECompress.h"

class CAPECompressCore;

class CAPECompressCreate
{
public:
    CAPECompressCreate();
    ~CAPECompressCreate();
    
    int InitializeFile(CIO * pIO, const WAVEFORMATEX * pwfeInput, int nMaxFrames, int nCompressionLevel, const void * pHeaderData, int nHeaderBytes);
    int FinalizeFile(CIO * pIO, int nNumberOfFrames, int nFinalFrameBlocks, const void * pTerminatingData, int nTerminatingBytes, int nWAVTerminatingBytes, int nPeakLevel);
    
    int SetSeekByte(int nFrame, int nByteOffset);

    int Start(CIO * pioOutput, const WAVEFORMATEX * pwfeInput, int nMaxAudioBytes, int nCompressionLevel = COMPRESSION_LEVEL_NORMAL, const void * pHeaderData = NULL, int nHeaderBytes = CREATE_WAV_HEADER_ON_DECOMPRESSION);
        
    int GetFullFrameBytes();
    int EncodeFrame(const void * pInputData, int nInputBytes);

    int Finish(const void * pTerminatingData, int nTerminatingBytes, int nWAVTerminatingBytes);
    

private:
    
    CSmartPtr<uint32> m_spSeekTable;
    int m_nMaxFrames;

    CSmartPtr<CIO> m_spIO;
    CSmartPtr<CAPECompressCore> m_spAPECompressCore;
    
    WAVEFORMATEX    m_wfeInput;
    int                m_nCompressionLevel;
    int                m_nSamplesPerFrame;
    int                m_nFrameIndex;
    int                m_nLastFrameBlocks;

};

#endif // #ifndef APE_APECOMPRESSCREATE_H
