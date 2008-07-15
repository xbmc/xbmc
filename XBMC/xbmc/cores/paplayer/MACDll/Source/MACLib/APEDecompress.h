#ifndef APE_APEDECOMPRESS_H
#define APE_APEDECOMPRESS_H

#include "APEDecompress.h"

class CUnBitArray;
class CPrepare;
class CAPEInfo;
class IPredictorDecompress;
#include "UnBitArrayBase.h"
#include "MACLib.h"
#include "Prepare.h"
#include "CircleBuffer.h"

class CAPEDecompress : public IAPEDecompress
{
public:

    CAPEDecompress(int * pErrorCode, CAPEInfo * pAPEInfo, int nStartBlock = -1, int nFinishBlock = -1);
    ~CAPEDecompress();

    int GetData(char * pBuffer, int nBlocks, int * pBlocksRetrieved);
    int Seek(int nBlockOffset);

    int GetInfo(APE_DECOMPRESS_FIELDS Field, int nParam1 = 0, int nParam2 = 0);

protected:

    // file info
    int m_nBlockAlign;
    int m_nCurrentFrame;
    
    // start / finish information
    int m_nStartBlock;
    int m_nFinishBlock;
    int m_nCurrentBlock;
    BOOL m_bIsRanged;
    BOOL m_bDecompressorInitialized;

    // decoding tools    
    CPrepare m_Prepare;
    WAVEFORMATEX m_wfeInput;
    unsigned int m_nCRC;
    unsigned int m_nStoredCRC;
    int m_nSpecialCodes;
    
    int SeekToFrame(int nFrameIndex);
    void DecodeBlocksToFrameBuffer(int nBlocks);
    int FillFrameBuffer();
    void StartFrame();
    void EndFrame();
    int InitializeDecompressor();

    // more decoding components
    CSmartPtr<CAPEInfo> m_spAPEInfo;
    CSmartPtr<CUnBitArrayBase> m_spUnBitArray;
    UNBIT_ARRAY_STATE m_BitArrayStateX;
    UNBIT_ARRAY_STATE m_BitArrayStateY;

    CSmartPtr<IPredictorDecompress> m_spNewPredictorX;
    CSmartPtr<IPredictorDecompress> m_spNewPredictorY;

    int m_nLastX;
    
    // decoding buffer
    BOOL m_bErrorDecodingCurrentFrame;
    int m_nCurrentFrameBufferBlock;
    int m_nFrameBufferFinishedBlocks;
    CCircleBuffer m_cbFrameBuffer;
};

#endif // #ifndef APE_APEDECOMPRESS_H
