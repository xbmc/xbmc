/*****************************************************************************************
UnMAC.h
Copyright (C) 2000-2001 by Matthew T. Ashland   All Rights Reserved.

Methods for decompressing or verifying APE files

Notes:
    -none
*****************************************************************************************/

#ifndef APE_UNMAC_H
#define APE_UNMAC_H

#include "../BitArray.h"
#include "../UnBitArrayBase.h"

class CAntiPredictor;
class CPrepare;
class CAPEDecompressCore;
class CPredictorBase;
class IPredictorDecompress;
class IAPEDecompress;

/*****************************************************************************************
CUnMAC class... a class that allows decoding on a frame-by-frame basis
*****************************************************************************************/
class CUnMAC 
{
public:
    
    // construction/destruction
    CUnMAC();
    ~CUnMAC();

    // functions
    int Initialize(IAPEDecompress *pAPEDecompress);
    int Uninitialize();
    int DecompressFrame(unsigned char *pOutputData, int32 FrameIndex, int CPULoadBalancingFactor = 0);

    int SeekToFrame(int FrameIndex);
    
private:

    // data members
    BOOL m_bInitialized;
    int m_LastDecodedFrameIndex;
    IAPEDecompress * m_pAPEDecompress;
    CPrepare * m_pPrepare;

    CAPEDecompressCore * m_pAPEDecompressCore;

    // functions
    void GenerateDecodedArrays(int nBlocks, int nSpecialCodes, int nFrameIndex, int nCPULoadBalancingFactor);
    void GenerateDecodedArray(int *Input_Array, uint32 Number_of_Elements, int Frame_Index, CAntiPredictor *pAntiPredictor, int CPULoadBalancingFactor = 0);

    int CreateAntiPredictors(int nCompressionLevel, int nVersion);

    int DecompressFrameOld(unsigned char *pOutputData, int32 FrameIndex, int CPULoadBalancingFactor);
    uint32 CalculateOldChecksum(int *pDataX, int *pDataY, int nChannels, int nBlocks);

public:
    
    int m_nBlocksProcessed;
    unsigned int m_nCRC;
    unsigned int m_nStoredCRC;
    WAVEFORMATEX m_wfeInput;
};

#endif // #ifndef APE_UNMAC_H
