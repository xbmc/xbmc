#include "All.h"
#ifdef BACKWARDS_COMPATIBILITY

#include "UnMAC.h"
#include "APEDecompressCore.h"
#include "../APEInfo.h"
#include "GlobalFunctions.h"
#include "../UnBitArrayBase.h"
#include "Anti-Predictor.h"
#include "UnMAC.h"
#include "../Prepare.h"
#include "../UnBitArray.h"
#include "../Assembly/Assembly.h"

CAPEDecompressCore::CAPEDecompressCore(CIO * pIO, IAPEDecompress * pAPEDecompress)
{
    m_pAPEDecompress = pAPEDecompress;

    // initialize the bit array
    m_pUnBitArray = CreateUnBitArray(pAPEDecompress, pAPEDecompress->GetInfo(APE_INFO_FILE_VERSION));
    
    if (m_pAPEDecompress->GetInfo(APE_INFO_FILE_VERSION) >= 3930)
        throw(0);

    m_pAntiPredictorX = CreateAntiPredictor(pAPEDecompress->GetInfo(APE_INFO_COMPRESSION_LEVEL), pAPEDecompress->GetInfo(APE_INFO_FILE_VERSION));
    m_pAntiPredictorY = CreateAntiPredictor(pAPEDecompress->GetInfo(APE_INFO_COMPRESSION_LEVEL), pAPEDecompress->GetInfo(APE_INFO_FILE_VERSION));
    
    m_pDataX = new int [pAPEDecompress->GetInfo(APE_INFO_BLOCKS_PER_FRAME) + 16];
    m_pDataY = new int [pAPEDecompress->GetInfo(APE_INFO_BLOCKS_PER_FRAME) + 16];
    m_pTempData = new int [pAPEDecompress->GetInfo(APE_INFO_BLOCKS_PER_FRAME) + 16];

    m_nBlocksProcessed = 0;
    
    // check to see if MMX is available
    m_bMMXAvailable = false;//GetMMXAvailable();
    
}

CAPEDecompressCore::~CAPEDecompressCore()
{
    SAFE_DELETE(m_pUnBitArray)
        
    SAFE_DELETE(m_pAntiPredictorX)
    SAFE_DELETE(m_pAntiPredictorY)
    
    SAFE_ARRAY_DELETE(m_pDataX)
    SAFE_ARRAY_DELETE(m_pDataY)
    SAFE_ARRAY_DELETE(m_pTempData)
}

void CAPEDecompressCore::GenerateDecodedArrays(int nBlocks, int nSpecialCodes, int nFrameIndex, int nCPULoadBalancingFactor)
{
    CUnBitArray * pBitArray = (CUnBitArray *) m_pUnBitArray;
    
    if (m_pAPEDecompress->GetInfo(APE_INFO_CHANNELS) == 2)
    {
        if ((nSpecialCodes & SPECIAL_FRAME_LEFT_SILENCE) && (nSpecialCodes & SPECIAL_FRAME_RIGHT_SILENCE)) 
        {
            memset(m_pDataX, 0, nBlocks * 4);
            memset(m_pDataY, 0, nBlocks * 4);
        }
        else if (nSpecialCodes & SPECIAL_FRAME_PSEUDO_STEREO) 
        {
            GenerateDecodedArray(m_pDataX, nBlocks, nFrameIndex, m_pAntiPredictorX, nCPULoadBalancingFactor);
            memset(m_pDataY, 0, nBlocks * 4);
        }
        else 
        {
            GenerateDecodedArray(m_pDataX, nBlocks, nFrameIndex, m_pAntiPredictorX, nCPULoadBalancingFactor);
            GenerateDecodedArray(m_pDataY, nBlocks, nFrameIndex, m_pAntiPredictorY, nCPULoadBalancingFactor);
        }
    }
    else
    {
        if (nSpecialCodes & SPECIAL_FRAME_LEFT_SILENCE)
        {
            memset(m_pDataX, 0, nBlocks * 4);
        }
        else
        {
            GenerateDecodedArray(m_pDataX, nBlocks, nFrameIndex, m_pAntiPredictorX, nCPULoadBalancingFactor);
        }
    }
}


void CAPEDecompressCore::GenerateDecodedArray(int * Input_Array, uint32 Number_of_Elements, int Frame_Index, CAntiPredictor *pAntiPredictor, int CPULoadBalancingFactor)
{
    const int nFrameBytes = m_pAPEDecompress->GetInfo(APE_INFO_FRAME_BYTES, Frame_Index);

    // run the prediction sequence
    switch (m_pAPEDecompress->GetInfo(APE_INFO_COMPRESSION_LEVEL)) 
    {

#ifdef ENABLE_COMPRESSION_MODE_FAST
        case COMPRESSION_LEVEL_FAST:
            if (m_pAPEDecompress->GetInfo(APE_INFO_FILE_VERSION) < 3320)
            {
                m_pUnBitArray->GenerateArray(m_pTempData, Number_of_Elements, nFrameBytes);
                pAntiPredictor->AntiPredict(m_pTempData, Input_Array, Number_of_Elements);
            }
            else
            {
                m_pUnBitArray->GenerateArray(Input_Array, Number_of_Elements, nFrameBytes);
                pAntiPredictor->AntiPredict(Input_Array, NULL, Number_of_Elements);
            }

            break;
#endif // #ifdef ENABLE_COMPRESSION_MODE_FAST
            
#ifdef ENABLE_COMPRESSION_MODE_NORMAL
        
        case COMPRESSION_LEVEL_NORMAL:
        {
            // get the array from the bitstream
            m_pUnBitArray->GenerateArray(m_pTempData, Number_of_Elements, nFrameBytes);
            pAntiPredictor->AntiPredict(m_pTempData, Input_Array, Number_of_Elements);
            break;
        }

#endif // #ifdef ENABLE_COMPRESSION_MODE_NORMAL

#ifdef ENABLE_COMPRESSION_MODE_HIGH
        case COMPRESSION_LEVEL_HIGH:
            // get the array from the bitstream
            m_pUnBitArray->GenerateArray(m_pTempData, Number_of_Elements, nFrameBytes);
            pAntiPredictor->AntiPredict(m_pTempData, Input_Array, Number_of_Elements);
            break;
#endif // #ifdef ENABLE_COMPRESSION_MODE_HIGH

#ifdef ENABLE_COMPRESSION_MODE_EXTRA_HIGH
        case COMPRESSION_LEVEL_EXTRA_HIGH:
            
            unsigned int aryCoefficientsA[64], aryCoefficientsB[64], nNumberOfCoefficients;
            
            #define GET_COEFFICIENTS(NumberOfCoefficientsBits, ValueBits)                                            \
                nNumberOfCoefficients = m_pUnBitArray->DecodeValue(DECODE_VALUE_METHOD_X_BITS, NumberOfCoefficientsBits);        \
                for (unsigned int z = 0; z <= nNumberOfCoefficients; z++)                                            \
                {                                                                                                    \
                    aryCoefficientsA[z] = m_pUnBitArray->DecodeValue(DECODE_VALUE_METHOD_X_BITS, ValueBits);                    \
                    aryCoefficientsB[z] = m_pUnBitArray->DecodeValue(DECODE_VALUE_METHOD_X_BITS, ValueBits);                    \
                }                                                                                                    \

            if (m_pAPEDecompress->GetInfo(APE_INFO_FILE_VERSION) < 3320) 
            {
                GET_COEFFICIENTS(4, 6)
                m_pUnBitArray->GenerateArray(m_pTempData, Number_of_Elements, nFrameBytes);
                ((CAntiPredictorExtraHigh0000To3320 *) pAntiPredictor)->AntiPredict(m_pTempData, Input_Array, Number_of_Elements, nNumberOfCoefficients, &aryCoefficientsA[0], &aryCoefficientsB[0]);
            }
            else if (m_pAPEDecompress->GetInfo(APE_INFO_FILE_VERSION) < 3600) 
            {
                GET_COEFFICIENTS(3, 5)
                m_pUnBitArray->GenerateArray(m_pTempData, Number_of_Elements, nFrameBytes);
                ((CAntiPredictorExtraHigh3320To3600 *) pAntiPredictor)->AntiPredict(m_pTempData, Input_Array, Number_of_Elements, nNumberOfCoefficients, &aryCoefficientsA[0], &aryCoefficientsB[0]);
            }
            else if (m_pAPEDecompress->GetInfo(APE_INFO_FILE_VERSION) < 3700) 
            {
                GET_COEFFICIENTS(3, 6)
                m_pUnBitArray->GenerateArray(m_pTempData, Number_of_Elements, nFrameBytes);
                ((CAntiPredictorExtraHigh3600To3700 *) pAntiPredictor)->AntiPredict(m_pTempData, Input_Array, Number_of_Elements, nNumberOfCoefficients, &aryCoefficientsA[0], &aryCoefficientsB[0]);
            }
            else if (m_pAPEDecompress->GetInfo(APE_INFO_FILE_VERSION) < 3800) 
            {
                GET_COEFFICIENTS(3, 6)
                m_pUnBitArray->GenerateArray(m_pTempData, Number_of_Elements, nFrameBytes);
                ((CAntiPredictorExtraHigh3700To3800 *) pAntiPredictor)->AntiPredict(m_pTempData, Input_Array, Number_of_Elements, nNumberOfCoefficients, &aryCoefficientsA[0], &aryCoefficientsB[0]);
            }    
            else
            {
                m_pUnBitArray->GenerateArray(m_pTempData, Number_of_Elements, nFrameBytes);
                ((CAntiPredictorExtraHigh3800ToCurrent *) pAntiPredictor)->AntiPredict(m_pTempData, Input_Array, Number_of_Elements, m_bMMXAvailable, CPULoadBalancingFactor, m_pAPEDecompress->GetInfo(APE_INFO_FILE_VERSION));
            }
            
            break;
#endif // #ifdef ENABLE_COMPRESSION_MODE_EXTRA_HIGH
    }
}

#endif // #ifdef BACKWARDS_COMPATIBILITY

