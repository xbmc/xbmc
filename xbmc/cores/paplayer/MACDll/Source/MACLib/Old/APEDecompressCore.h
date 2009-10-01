#ifndef APE_DECOMPRESS_CORE_H
#define APE_DECOMPRESS_CORE_H

class CAPEDecompressCore
{
public:

    CAPEDecompressCore(CIO * pIO, IAPEDecompress * pAPEDecompress);
    ~CAPEDecompressCore();
    
    void GenerateDecodedArrays(int nBlocks, int nSpecialCodes, int nFrameIndex, int nCPULoadBalancingFactor);
    void GenerateDecodedArray(int *Input_Array, uint32 Number_of_Elements, int Frame_Index, CAntiPredictor *pAntiPredictor, int CPULoadBalancingFactor = 0);
    
    int * GetDataX() { return m_pDataX; }
    int * GetDataY() { return m_pDataY; }
    
    CUnBitArrayBase * GetUnBitArrray() { return m_pUnBitArray; }
    
    int * m_pTempData;
    int * m_pDataX;
    int * m_pDataY;
    
    CAntiPredictor * m_pAntiPredictorX;
    CAntiPredictor * m_pAntiPredictorY;
    
    CUnBitArrayBase * m_pUnBitArray;
    BIT_ARRAY_STATE m_BitArrayStateX;
    BIT_ARRAY_STATE m_BitArrayStateY;
    
    IAPEDecompress * m_pAPEDecompress;
    
    BOOL m_bMMXAvailable;
    int m_nBlocksProcessed;
};


#endif // #ifndef APE_DECOMPRESS_CORE_H

