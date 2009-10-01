/*****************************************************************************************
UnMAC.cpp
Copyright (C) 2000-2001 by Matthew T. Ashland   All Rights Reserved.

CUnMAC - the main hub for decompression (manages all of the other components)

Notes:
    -none
*****************************************************************************************/

/*****************************************************************************************
Includes
*****************************************************************************************/
#include "All.h"
#ifdef BACKWARDS_COMPATIBILITY

#include "../APEInfo.h"
#include "UnMAC.h"
#include "GlobalFunctions.h"
#include "../UnBitArrayBase.h"
#include "Anti-Predictor.h"
#include "../Prepare.h"
#include "../UnBitArray.h"
#include "../NewPredictor.h"
#include "APEDecompressCore.h"

/*****************************************************************************************
CUnMAC class construction
*****************************************************************************************/
CUnMAC::CUnMAC() 
{
    // initialize member variables
    m_bInitialized                = FALSE;
    m_LastDecodedFrameIndex        = -1;
    m_pAPEDecompress            = NULL;

    m_pAPEDecompressCore        = NULL;
    m_pPrepare                    = NULL;

    m_nBlocksProcessed            = 0;
    m_nCRC                        = 0;

}

/*****************************************************************************************
CUnMAC class destruction
*****************************************************************************************/
CUnMAC::~CUnMAC() 
{
    // uninitialize the decoder in case it isn't already
    Uninitialize();
}

/*****************************************************************************************
Initialize
*****************************************************************************************/
int CUnMAC::Initialize(IAPEDecompress *pAPEDecompress) 
{
    // uninitialize if it is currently initialized
    if (m_bInitialized)
        Uninitialize();

    if (pAPEDecompress == NULL)
    {
        Uninitialize();
        return ERROR_INITIALIZING_UNMAC;
    }

    // set the member pointer to the IAPEDecompress class
    m_pAPEDecompress = pAPEDecompress;

    // set the last decode frame to -1 so it forces a seek on start
    m_LastDecodedFrameIndex = -1;

    m_pAPEDecompressCore = new CAPEDecompressCore(GET_IO(pAPEDecompress), pAPEDecompress);
    m_pPrepare = new CPrepare;

    // set the initialized flag to TRUE
    m_bInitialized = TRUE;
    
    m_pAPEDecompress->GetInfo(APE_INFO_WAVEFORMATEX, (long) &m_wfeInput);

    // return a successful value
    return ERROR_SUCCESS;
}

/*****************************************************************************************
Uninitialize
*****************************************************************************************/
int CUnMAC::Uninitialize() 
{
    if (m_bInitialized) 
    {
        SAFE_DELETE(m_pAPEDecompressCore)
        SAFE_DELETE(m_pPrepare)
        
        // clear the APE info pointer
        m_pAPEDecompress = NULL;

        // set the last decoded frame again
        m_LastDecodedFrameIndex = -1;

        // set the initialized flag to FALSE
        m_bInitialized = FALSE;
    }

    return ERROR_SUCCESS;
}

/*****************************************************************************************
Decompress frame
*****************************************************************************************/
int CUnMAC::DecompressFrame(unsigned char *pOutputData, int32 FrameIndex, int CPULoadBalancingFactor) 
{
    return DecompressFrameOld(pOutputData, FrameIndex, CPULoadBalancingFactor);
}

/*****************************************************************************************
Seek to the proper frame (if necessary) and do any alignment of the bit array
*****************************************************************************************/
int CUnMAC::SeekToFrame(int FrameIndex)
{
    if (GET_FRAMES_START_ON_BYTES_BOUNDARIES(m_pAPEDecompress)) 
    {
        if ((m_LastDecodedFrameIndex == -1) || ((FrameIndex - 1) != m_LastDecodedFrameIndex)) 
        {
            int SeekRemainder = (m_pAPEDecompress->GetInfo(APE_INFO_SEEK_BYTE, FrameIndex) - m_pAPEDecompress->GetInfo(APE_INFO_SEEK_BYTE, 0)) % 4;
            m_pAPEDecompressCore->GetUnBitArrray()->FillAndResetBitArray(m_pAPEDecompress->GetInfo(APE_INFO_SEEK_BYTE, FrameIndex) - SeekRemainder, SeekRemainder * 8);
        }
        else 
        {
            m_pAPEDecompressCore->GetUnBitArrray()->AdvanceToByteBoundary();
        }
    }
    else 
    {
        if ((m_LastDecodedFrameIndex == -1) || ((FrameIndex - 1) != m_LastDecodedFrameIndex)) 
        {
            m_pAPEDecompressCore->GetUnBitArrray()->FillAndResetBitArray(m_pAPEDecompress->GetInfo(APE_INFO_SEEK_BYTE, FrameIndex), m_pAPEDecompress->GetInfo(APE_INFO_SEEK_BIT, FrameIndex));
        }
    }

    return ERROR_SUCCESS;
}

/*****************************************************************************************
Old code for frame decompression
*****************************************************************************************/
int CUnMAC::DecompressFrameOld(unsigned char *pOutputData, int32 FrameIndex, int CPULoadBalancingFactor) 
{
    // error check the parameters (too high of a frame index, etc.)
    if (FrameIndex >= m_pAPEDecompress->GetInfo(APE_INFO_TOTAL_FRAMES)) { return ERROR_SUCCESS; }

    // get the number of samples in the frame
    int nBlocks = 0;
    nBlocks = ((FrameIndex + 1) >= m_pAPEDecompress->GetInfo(APE_INFO_TOTAL_FRAMES)) ? m_pAPEDecompress->GetInfo(APE_INFO_FINAL_FRAME_BLOCKS) : m_pAPEDecompress->GetInfo(APE_INFO_BLOCKS_PER_FRAME);
    if (nBlocks == 0)
        return -1; // nothing to do (file must be zero length) (have to return error)

    // take care of seeking and frame alignment
    if (SeekToFrame(FrameIndex) != 0) { return -1; }

    // get the checksum
    unsigned int nSpecialCodes = 0;
    uint32 nStoredCRC = 0;
    
    if (GET_USES_CRC(m_pAPEDecompress) == FALSE)
    {
        nStoredCRC = m_pAPEDecompressCore->GetUnBitArrray()->DecodeValue(DECODE_VALUE_METHOD_UNSIGNED_RICE, 30);
        if (nStoredCRC == 0)
        {
            nSpecialCodes = SPECIAL_FRAME_LEFT_SILENCE | SPECIAL_FRAME_RIGHT_SILENCE;
        }            
    }
    else
    {
        nStoredCRC = m_pAPEDecompressCore->GetUnBitArrray()->DecodeValue(DECODE_VALUE_METHOD_UNSIGNED_INT);
        
        // get any 'special' codes if the file uses them (for silence, FALSE stereo, etc.)
        nSpecialCodes = 0;
        if (GET_USES_SPECIAL_FRAMES(m_pAPEDecompress))
        {
            if (nStoredCRC & 0x80000000) 
            {
                nSpecialCodes = m_pAPEDecompressCore->GetUnBitArrray()->DecodeValue(DECODE_VALUE_METHOD_UNSIGNED_INT);
            }
            nStoredCRC &= 0x7FFFFFFF;
        }
    }

    // the CRC that will be figured during decompression
    uint32 CRC = 0xFFFFFFFF;

    // decompress and convert from (x,y) -> (l,r)
    // sort of int and ugly.... sorry
    if (m_pAPEDecompress->GetInfo(APE_INFO_CHANNELS) == 2) 
    {
        m_pAPEDecompressCore->GenerateDecodedArrays(nBlocks, nSpecialCodes, FrameIndex, CPULoadBalancingFactor);

        WAVEFORMATEX WaveFormatEx; m_pAPEDecompress->GetInfo(APE_INFO_WAVEFORMATEX, (long) &WaveFormatEx);
        m_pPrepare->UnprepareOld(m_pAPEDecompressCore->GetDataX(), m_pAPEDecompressCore->GetDataY(), nBlocks, &WaveFormatEx, 
            pOutputData, (unsigned int *) &CRC, (int *) &nSpecialCodes, m_pAPEDecompress->GetInfo(APE_INFO_FILE_VERSION));
    }
    else if (m_pAPEDecompress->GetInfo(APE_INFO_CHANNELS) == 1) 
    {
        m_pAPEDecompressCore->GenerateDecodedArrays(nBlocks, nSpecialCodes, FrameIndex, CPULoadBalancingFactor);
        
        WAVEFORMATEX WaveFormatEx; m_pAPEDecompress->GetInfo(APE_INFO_WAVEFORMATEX, (long) &WaveFormatEx);
        m_pPrepare->UnprepareOld(m_pAPEDecompressCore->GetDataX(), NULL, nBlocks, &WaveFormatEx, 
            pOutputData, (unsigned int *) &CRC, (int *) &nSpecialCodes, m_pAPEDecompress->GetInfo(APE_INFO_FILE_VERSION));
    }

    if (GET_USES_SPECIAL_FRAMES(m_pAPEDecompress))
    {
        CRC >>= 1;
    }

    // check the CRC
    if (GET_USES_CRC(m_pAPEDecompress) == FALSE)
    {
        uint32 nChecksum = CalculateOldChecksum(m_pAPEDecompressCore->GetDataX(), m_pAPEDecompressCore->GetDataY(), m_pAPEDecompress->GetInfo(APE_INFO_CHANNELS), nBlocks);
        if (nChecksum != nStoredCRC)
            return -1;
    }
    else
    {
        if (CRC != nStoredCRC)
            return -1;
    }

    m_LastDecodedFrameIndex = FrameIndex;
    return nBlocks;    
}


/*****************************************************************************************
Figures the old checksum using the X,Y data
*****************************************************************************************/
uint32 CUnMAC::CalculateOldChecksum(int *pDataX, int *pDataY, int nChannels, int nBlocks)
{
    uint32 nChecksum = 0;

    if (nChannels == 2)
    {
        for (int z = 0; z < nBlocks; z++)
        {
            int R = pDataX[z] - (pDataY[z] / 2);
            int L = R + pDataY[z];
            nChecksum += (labs(R) + labs(L));
        }
    }
    else if (nChannels == 1)
    {
        for (int z = 0; z < nBlocks; z++)
            nChecksum += labs(pDataX[z]);
    }
    
    return nChecksum;
}

#endif // #ifdef BACKWARDS_COMPATIBILITY

