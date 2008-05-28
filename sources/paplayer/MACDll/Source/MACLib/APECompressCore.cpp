#include "All.h"
#include "APECompressCore.h"

#include "BitArray.h"
#include "Prepare.h"
#include "NewPredictor.h"

CAPECompressCore::CAPECompressCore(CIO * pIO, const WAVEFORMATEX * pwfeInput, int nMaxFrameBlocks, int nCompressionLevel)
{
    m_spBitArray.Assign(new CBitArray(pIO));
    m_spDataX.Assign(new int [nMaxFrameBlocks], TRUE);
    m_spDataY.Assign(new int [nMaxFrameBlocks], TRUE);
    m_spTempData.Assign(new int [nMaxFrameBlocks], TRUE);
    m_spPrepare.Assign(new CPrepare);
    m_spPredictorX.Assign(new CPredictorCompressNormal(nCompressionLevel));
    m_spPredictorY.Assign(new CPredictorCompressNormal(nCompressionLevel));

    memcpy(&m_wfeInput, pwfeInput, sizeof(WAVEFORMATEX));
    m_nPeakLevel = 0;
}

CAPECompressCore::~CAPECompressCore()
{
}

int CAPECompressCore::EncodeFrame(const void * pInputData, int nInputBytes)
{
    // variables
    const int nInputBlocks = nInputBytes / m_wfeInput.nBlockAlign;
    int nSpecialCodes = 0;

    // always start a new frame on a byte boundary
    m_spBitArray->AdvanceToByteBoundary();
    
    // do the preparation stage
    RETURN_ON_ERROR(Prepare(pInputData, nInputBytes, &nSpecialCodes))

    m_spPredictorX->Flush();
    m_spPredictorY->Flush();

    m_spBitArray->FlushState(m_BitArrayStateX);
    m_spBitArray->FlushState(m_BitArrayStateY);

    m_spBitArray->FlushBitArray();

    if (m_wfeInput.nChannels == 2) 
    {
        BOOL bEncodeX = TRUE;
        BOOL bEncodeY = TRUE;
        
        if ((nSpecialCodes & SPECIAL_FRAME_LEFT_SILENCE) && 
            (nSpecialCodes & SPECIAL_FRAME_RIGHT_SILENCE)) 
        {
            bEncodeX = FALSE;
            bEncodeY = FALSE;
        }
        
        if (nSpecialCodes & SPECIAL_FRAME_PSEUDO_STEREO) 
        {
            bEncodeY = FALSE;
        }
        
        if (bEncodeX && bEncodeY)
        {
            int nLastX = 0;
            for (int z = 0; z < nInputBlocks; z++)
            {
                m_spBitArray->EncodeValue(m_spPredictorY->CompressValue(m_spDataY[z], nLastX), m_BitArrayStateY);
                m_spBitArray->EncodeValue(m_spPredictorX->CompressValue(m_spDataX[z], m_spDataY[z]), m_BitArrayStateX);
                
                nLastX = m_spDataX[z];
            }
        }
        else if (bEncodeX) 
        {
            for (int z = 0; z < nInputBlocks; z++)
            {
                RETURN_ON_ERROR(m_spBitArray->EncodeValue(m_spPredictorX->CompressValue(m_spDataX[z]), m_BitArrayStateX))
            }
        }
        else if (bEncodeY) 
        {
            for (int z = 0; z < nInputBlocks; z++)
            {
                RETURN_ON_ERROR(m_spBitArray->EncodeValue(m_spPredictorY->CompressValue(m_spDataY[z]), m_BitArrayStateY))
            }
        }
    }
    else if (m_wfeInput.nChannels == 1) 
    {
        if (!(nSpecialCodes & SPECIAL_FRAME_MONO_SILENCE))
        {
            for (int z = 0; z < nInputBlocks; z++)
            {
                RETURN_ON_ERROR(m_spBitArray->EncodeValue(m_spPredictorX->CompressValue(m_spDataX[z]), m_BitArrayStateX))
            }
        }
    }    

    m_spBitArray->Finalize();

    // return success
    return 0;
}

int CAPECompressCore::Prepare(const void * pInputData, int nInputBytes, int * pSpecialCodes)
{
    // variable declares
    *pSpecialCodes = 0;
    unsigned int nCRC = 0;
    
    // do the preparation
    RETURN_ON_ERROR(m_spPrepare->Prepare((unsigned char *) pInputData, nInputBytes, &m_wfeInput, m_spDataX, m_spDataY,
        &nCRC, pSpecialCodes, &m_nPeakLevel))
    
    // store the CRC
    RETURN_ON_ERROR(m_spBitArray->EncodeUnsignedLong(nCRC))
    
    // store any special codes
    if (*pSpecialCodes != 0) 
    {
        RETURN_ON_ERROR(m_spBitArray->EncodeUnsignedLong(*pSpecialCodes))
    }
    
    return 0;
}

