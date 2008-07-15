#ifndef APE_APECOMPRESSCORE_H
#define APE_APECOMPRESSCORE_H

#include "APECompress.h"
#include "BitArray.h"

class CPrepare;
class IPredictorCompress;

/*************************************************************************************************
CAPECompressCore - manages the core of compression and bitstream output
*************************************************************************************************/
class  CAPECompressCore
{
public:
    CAPECompressCore(CIO * pIO, const WAVEFORMATEX * pwfeInput, int nMaxFrameBlocks, int nCompressionLevel);
    ~CAPECompressCore();

    int EncodeFrame(const void * pInputData, int nInputBytes);

    CBitArray * GetBitArray() { return m_spBitArray.GetPtr(); }
    int GetPeakLevel() { return m_nPeakLevel; }

private:

    int Prepare(const void * pInputData, int nInputBytes, int * pSpecialCodes);

    CSmartPtr<CBitArray> m_spBitArray;
    CSmartPtr<IPredictorCompress> m_spPredictorX;
    CSmartPtr<IPredictorCompress> m_spPredictorY;
    BIT_ARRAY_STATE    m_BitArrayStateX;
    BIT_ARRAY_STATE    m_BitArrayStateY;
    CSmartPtr<int> m_spDataX;
    CSmartPtr<int> m_spDataY;
    CSmartPtr<int> m_spTempData;
    CSmartPtr<CPrepare> m_spPrepare;
    WAVEFORMATEX m_wfeInput;
    int    m_nPeakLevel;
};

#endif // #ifndef APE_APECOMPRESSCORE_H
