#include "All.h"

#ifdef BACKWARDS_COMPATIBILITY

#include "UnMAC.h"
#include "APEDecompressOld.h"
#include "../APEInfo.h"

CAPEDecompressOld::CAPEDecompressOld(int * pErrorCode, CAPEInfo * pAPEInfo, int nStartBlock, int nFinishBlock)
{
    *pErrorCode = ERROR_SUCCESS;

    // open / analyze the file
    m_spAPEInfo.Assign(pAPEInfo);

    // version check (this implementation only works with 3.92 and earlier files)
    if (GetInfo(APE_INFO_FILE_VERSION) > 3920)
    {
        *pErrorCode = ERROR_UNDEFINED;
        return;
    }

    // create the buffer
    m_nBlockAlign = GetInfo(APE_INFO_BLOCK_ALIGN);
    
    // initialize other stuff
    m_nBufferTail = 0;
    m_bDecompressorInitialized = FALSE;
    m_nCurrentFrame = 0;
    m_nCurrentBlock = 0;

    // set the "real" start and finish blocks
    m_nStartBlock = (nStartBlock < 0) ? 0 : min(nStartBlock, GetInfo(APE_INFO_TOTAL_BLOCKS));
    m_nFinishBlock = (nFinishBlock < 0) ? GetInfo(APE_INFO_TOTAL_BLOCKS) : min(nFinishBlock, GetInfo(APE_INFO_TOTAL_BLOCKS));
    m_bIsRanged = (m_nStartBlock != 0) || (m_nFinishBlock != GetInfo(APE_INFO_TOTAL_BLOCKS));
}

CAPEDecompressOld::~CAPEDecompressOld()
{

}

int CAPEDecompressOld::InitializeDecompressor()
{
    // check if we have anything to do
    if (m_bDecompressorInitialized)
        return ERROR_SUCCESS;

    // initialize the decoder
    RETURN_ON_ERROR(m_UnMAC.Initialize(this))

    int nMaximumDecompressedFrameBytes = m_nBlockAlign * GetInfo(APE_INFO_BLOCKS_PER_FRAME);
    int nTotalBufferBytes = max(65536, (nMaximumDecompressedFrameBytes + 16) * 2);
    m_spBuffer.Assign(new char [nTotalBufferBytes], TRUE);
    if (m_spBuffer == NULL)
        return ERROR_INSUFFICIENT_MEMORY;

    // update the initialized flag
    m_bDecompressorInitialized = TRUE;

    // seek to the beginning
    return Seek(0);
}

int CAPEDecompressOld::GetData(char * pBuffer, int nBlocks, int * pBlocksRetrieved)
{
    if (pBlocksRetrieved) *pBlocksRetrieved = 0;

    RETURN_ON_ERROR(InitializeDecompressor())
    
    // cap
    int nBlocksUntilFinish = m_nFinishBlock - m_nCurrentBlock;
    nBlocks = min(nBlocks, nBlocksUntilFinish);

    int nBlocksRetrieved = 0;

    // fulfill as much of the request as possible
    int nTotalBytesNeeded = nBlocks * m_nBlockAlign;
    int nBytesLeft = nTotalBytesNeeded;
    int nBlocksDecoded = 1;

    while (nBytesLeft > 0 && nBlocksDecoded > 0)
    {
        // empty the buffer
        int nBytesAvailable = m_nBufferTail;
        int nIntialBytes = min(nBytesLeft, nBytesAvailable);
        if (nIntialBytes > 0)
        {
            memcpy(&pBuffer[nTotalBytesNeeded - nBytesLeft], &m_spBuffer[0], nIntialBytes);
            
            if ((m_nBufferTail - nIntialBytes) > 0)
                memmove(&m_spBuffer[0], &m_spBuffer[nIntialBytes], m_nBufferTail - nIntialBytes);
                
            nBytesLeft -= nIntialBytes;
            m_nBufferTail -= nIntialBytes;

        }

        // decode more
        if (nBytesLeft > 0)
        {
            nBlocksDecoded = m_UnMAC.DecompressFrame((unsigned char *) &m_spBuffer[m_nBufferTail], m_nCurrentFrame++, 0);
            if (nBlocksDecoded == -1)
            {
                return -1;
            }
            m_nBufferTail += (nBlocksDecoded * m_nBlockAlign);
        }
    }
    
    nBlocksRetrieved = (nTotalBytesNeeded - nBytesLeft) / m_nBlockAlign;

    // update the position
    m_nCurrentBlock += nBlocksRetrieved;

    if (pBlocksRetrieved) *pBlocksRetrieved = nBlocksRetrieved;
    
    return ERROR_SUCCESS;
}

int CAPEDecompressOld::Seek(int nBlockOffset)
{
    RETURN_ON_ERROR(InitializeDecompressor())

    // use the offset
    nBlockOffset += m_nStartBlock;
    
    // cap (to prevent seeking too far)
    if (nBlockOffset >= m_nFinishBlock)
        nBlockOffset = m_nFinishBlock - 1;
    if (nBlockOffset < m_nStartBlock)
        nBlockOffset = m_nStartBlock;

    // flush the buffer
    m_nBufferTail = 0;
    
    // seek to the perfect location
    int nBaseFrame = nBlockOffset / GetInfo(APE_INFO_BLOCKS_PER_FRAME);
    int nBlocksToSkip = nBlockOffset % GetInfo(APE_INFO_BLOCKS_PER_FRAME);
    int nBytesToSkip = nBlocksToSkip * m_nBlockAlign;
        
    // skip necessary blocks
    int nMaximumDecompressedFrameBytes = m_nBlockAlign * GetInfo(APE_INFO_BLOCKS_PER_FRAME);
    char *pTempBuffer = new char [nMaximumDecompressedFrameBytes + 16];
    ZeroMemory(pTempBuffer, nMaximumDecompressedFrameBytes + 16);
    
    m_nCurrentFrame = nBaseFrame;

    int nBlocksDecoded = m_UnMAC.DecompressFrame((unsigned char *) pTempBuffer, m_nCurrentFrame++, 0);
    
    if (nBlocksDecoded == -1)
    {
        return -1;
    }
    
    int nBytesToKeep = (nBlocksDecoded * m_nBlockAlign) - nBytesToSkip;
    memcpy(&m_spBuffer[m_nBufferTail], &pTempBuffer[nBytesToSkip], nBytesToKeep);
    m_nBufferTail += nBytesToKeep;
    
    delete [] pTempBuffer;
    

    m_nCurrentBlock = nBlockOffset;
    
    return ERROR_SUCCESS;
}

int CAPEDecompressOld::GetInfo(APE_DECOMPRESS_FIELDS Field, int nParam1, int nParam2)
{
    int nRetVal = 0;
    BOOL bHandled = TRUE;

    switch (Field)
    {
    case APE_DECOMPRESS_CURRENT_BLOCK:
        nRetVal = m_nCurrentBlock - m_nStartBlock;
        break;
    case APE_DECOMPRESS_CURRENT_MS:
    {
        int nSampleRate = m_spAPEInfo->GetInfo(APE_INFO_SAMPLE_RATE, 0, 0);
        if (nSampleRate > 0)
            nRetVal = int((double(m_nCurrentBlock) * double(1000)) / double(nSampleRate));
        break;
    }
    case APE_DECOMPRESS_TOTAL_BLOCKS:
        nRetVal = m_nFinishBlock - m_nStartBlock;
        break;
    case APE_DECOMPRESS_LENGTH_MS:
    {
        int nSampleRate = m_spAPEInfo->GetInfo(APE_INFO_SAMPLE_RATE, 0, 0);
        if (nSampleRate > 0)
            nRetVal = int((double(m_nFinishBlock - m_nStartBlock) * double(1000)) / double(nSampleRate));
        break;
    }
    case APE_DECOMPRESS_CURRENT_BITRATE:
        nRetVal = GetInfo(APE_INFO_FRAME_BITRATE, m_nCurrentFrame);
        break;
    case APE_DECOMPRESS_AVERAGE_BITRATE:
    {
        if (m_bIsRanged)
        {
            // figure the frame range
            const int nBlocksPerFrame = GetInfo(APE_INFO_BLOCKS_PER_FRAME);
            int nStartFrame = m_nStartBlock / nBlocksPerFrame;
            int nFinishFrame = (m_nFinishBlock + nBlocksPerFrame - 1) / nBlocksPerFrame;

            // get the number of bytes in the first and last frame
            int nTotalBytes = (GetInfo(APE_INFO_FRAME_BYTES, nStartFrame) * (m_nStartBlock % nBlocksPerFrame)) / nBlocksPerFrame;
            if (nFinishFrame != nStartFrame)
                nTotalBytes += (GetInfo(APE_INFO_FRAME_BYTES, nFinishFrame) * (m_nFinishBlock % nBlocksPerFrame)) / nBlocksPerFrame;

            // get the number of bytes in between
            const int nTotalFrames = GetInfo(APE_INFO_TOTAL_FRAMES);
            for (int nFrame = nStartFrame + 1; (nFrame < nFinishFrame) && (nFrame < nTotalFrames); nFrame++)
                nTotalBytes += GetInfo(APE_INFO_FRAME_BYTES, nFrame);

            // figure the bitrate
            int nTotalMS = int((double(m_nFinishBlock - m_nStartBlock) * double(1000)) / double(GetInfo(APE_INFO_SAMPLE_RATE)));
            if (nTotalMS != 0)
                nRetVal = (nTotalBytes * 8) / nTotalMS;
        }
        else
        {
            nRetVal = GetInfo(APE_INFO_AVERAGE_BITRATE);
        }

        break;
    }
    default:
        bHandled = FALSE;
    }

    if (!bHandled && m_bIsRanged)
    {
        bHandled = TRUE;

        switch (Field)
        {
        case APE_INFO_WAV_HEADER_BYTES:
            nRetVal = sizeof(WAVE_HEADER);
            break;
        case APE_INFO_WAV_HEADER_DATA:
        {
            char * pBuffer = (char *) nParam1;
            int nMaxBytes = nParam2;
            
            if (sizeof(WAVE_HEADER) > nMaxBytes)
            {
                nRetVal = -1;
            }
            else
            {
                WAVEFORMATEX wfeFormat; GetInfo(APE_INFO_WAVEFORMATEX, (long) &wfeFormat, 0);
                WAVE_HEADER WAVHeader; FillWaveHeader(&WAVHeader, 
                    (m_nFinishBlock - m_nStartBlock) * GetInfo(APE_INFO_BLOCK_ALIGN), 
                    &wfeFormat,    0);
                memcpy(pBuffer, &WAVHeader, sizeof(WAVE_HEADER));
                nRetVal = 0;
            }
            break;
        }
        case APE_INFO_WAV_TERMINATING_BYTES:
            nRetVal = 0;
            break;
        case APE_INFO_WAV_TERMINATING_DATA:
            nRetVal = 0;
            break;
        default:
            bHandled = FALSE;
        }
    }

    if (bHandled == FALSE)
        nRetVal = m_spAPEInfo->GetInfo(Field, nParam1, nParam2);

    return nRetVal;
}

#endif // #ifdef BACKWARDS_COMPATIBILITY

