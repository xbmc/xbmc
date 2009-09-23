/*****************************************************************************************
CAPEInfo:
    -a class to make working with APE files and getting information about them simple
*****************************************************************************************/
#include "All.h"
#include "APEInfo.h"
#include IO_HEADER_FILE
#include "APECompress.h"
#include "APEHeader.h"

#include <wchar.h>

/*****************************************************************************************
Construction
*****************************************************************************************/
CAPEInfo::CAPEInfo(int * pErrorCode, const wchar_t * pFilename, CAPETag * pTag) 
{
    *pErrorCode = ERROR_SUCCESS;
    CloseFile();
    
    // open the file
    m_spIO.Assign(new IO_CLASS_NAME);
    
    if (m_spIO->Open(pFilename) != 0)
    {
        CloseFile();
        *pErrorCode = ERROR_INVALID_INPUT_FILE;
        return;
    }
    
    // get the file information
    if (GetFileInformation(TRUE) != 0)
    {
        CloseFile();
        *pErrorCode = ERROR_INVALID_INPUT_FILE;
        return;
    }

    // get the tag (do this second so that we don't do it on failure)
    if (pTag == NULL)
    {
        // we don't want to analyze right away for non-local files
        // since a single I/O object is shared, we can't tag and read at the same time (i.e. in multiple threads)
        BOOL bAnalyzeNow = TRUE;
        if ((wcsnicmp(pFilename, L"http://", 7) == 0) || (wcsnicmp(pFilename, L"m01p://", 7) == 0))
            bAnalyzeNow = FALSE;

        m_spAPETag.Assign(new CAPETag(m_spIO, bAnalyzeNow));
    }
    else
    {
        m_spAPETag.Assign(pTag);
    }

}

CAPEInfo::CAPEInfo(int * pErrorCode, CIO * pIO, CAPETag * pTag)
{
    *pErrorCode = ERROR_SUCCESS;
    CloseFile();

    m_spIO.Assign(pIO, FALSE, FALSE);

    // get the file information
    if (GetFileInformation(TRUE) != 0)
    {
        CloseFile();
        *pErrorCode = ERROR_INVALID_INPUT_FILE;
        return;
    }

    // get the tag (do this second so that we don't do it on failure)
    if (pTag == NULL)
        m_spAPETag.Assign(new CAPETag(m_spIO, TRUE));
    else
        m_spAPETag.Assign(pTag);
}


/*****************************************************************************************
Destruction
*****************************************************************************************/
CAPEInfo::~CAPEInfo() 
{
    CloseFile();
}

/*****************************************************************************************
Close the file
*****************************************************************************************/
int CAPEInfo::CloseFile() 
{
    m_spIO.Delete();
    m_APEFileInfo.spWaveHeaderData.Delete();
    m_APEFileInfo.spSeekBitTable.Delete();
    m_APEFileInfo.spSeekByteTable.Delete();
    m_APEFileInfo.spAPEDescriptor.Delete();
    
    m_spAPETag.Delete();
    
    // re-initialize variables
    m_APEFileInfo.nSeekTableElements = 0;
    m_bHasFileInformationLoaded = FALSE;

    return ERROR_SUCCESS;
}

/*****************************************************************************************
Get the file information about the file
*****************************************************************************************/
int CAPEInfo::GetFileInformation(BOOL bGetTagInformation) 
{
    // quit if there is no simple file
    if (m_spIO == NULL) { return -1; }

    // quit if the file information has already been loaded
    if (m_bHasFileInformationLoaded) { return ERROR_SUCCESS; }

    // use a CAPEHeader class to help us analyze the file
    CAPEHeader APEHeader(m_spIO);
    int nRetVal = APEHeader.Analyze(&m_APEFileInfo);

    // update our internal state
    if (nRetVal == ERROR_SUCCESS)
        m_bHasFileInformationLoaded = TRUE;

    // return
    return nRetVal;
}

/*****************************************************************************************
Primary query function
*****************************************************************************************/
long CAPEInfo::GetInfo(APE_DECOMPRESS_FIELDS Field, int nParam1, int nParam2)
{
    long nRetVal = -1;

    switch (Field)
    {
    case APE_INFO_FILE_VERSION:
        nRetVal = m_APEFileInfo.nVersion;
        break;
    case APE_INFO_COMPRESSION_LEVEL:
        nRetVal = m_APEFileInfo.nCompressionLevel;
        break;
    case APE_INFO_FORMAT_FLAGS:
        nRetVal = m_APEFileInfo.nFormatFlags;
        break;
    case APE_INFO_SAMPLE_RATE:
        nRetVal = m_APEFileInfo.nSampleRate;
        break;
    case APE_INFO_BITS_PER_SAMPLE:
        nRetVal = m_APEFileInfo.nBitsPerSample;
        break;
    case APE_INFO_BYTES_PER_SAMPLE:
        nRetVal = m_APEFileInfo.nBytesPerSample;
        break;
    case APE_INFO_CHANNELS:
        nRetVal = m_APEFileInfo.nChannels;
        break;
    case APE_INFO_BLOCK_ALIGN:
        nRetVal = m_APEFileInfo.nBlockAlign;
        break;
    case APE_INFO_BLOCKS_PER_FRAME:
        nRetVal = m_APEFileInfo.nBlocksPerFrame;
        break;
    case APE_INFO_FINAL_FRAME_BLOCKS:
        nRetVal = m_APEFileInfo.nFinalFrameBlocks;
        break;
    case APE_INFO_TOTAL_FRAMES:
        nRetVal = m_APEFileInfo.nTotalFrames;
        break;
    case APE_INFO_WAV_HEADER_BYTES:
        nRetVal = m_APEFileInfo.nWAVHeaderBytes;
        break;
    case APE_INFO_WAV_TERMINATING_BYTES:
        nRetVal = m_APEFileInfo.nWAVTerminatingBytes;
        break;
    case APE_INFO_WAV_DATA_BYTES:
        nRetVal = m_APEFileInfo.nWAVDataBytes;
        break;
    case APE_INFO_WAV_TOTAL_BYTES:
        nRetVal = m_APEFileInfo.nWAVTotalBytes;
        break;
    case APE_INFO_APE_TOTAL_BYTES:
        nRetVal = m_APEFileInfo.nAPETotalBytes;
        break;
    case APE_INFO_TOTAL_BLOCKS:
        nRetVal = m_APEFileInfo.nTotalBlocks;
        break;
    case APE_INFO_LENGTH_MS:
        nRetVal = m_APEFileInfo.nLengthMS;
        break;
    case APE_INFO_AVERAGE_BITRATE:
        nRetVal = m_APEFileInfo.nAverageBitrate;
        break;
    case APE_INFO_FRAME_BITRATE:
    {
        int nFrame = nParam1;
        
        nRetVal = 0;

        int nFrameBytes = GetInfo(APE_INFO_FRAME_BYTES, nFrame);
        int nFrameBlocks = GetInfo(APE_INFO_FRAME_BLOCKS, nFrame);
        if ((nFrameBytes > 0) && (nFrameBlocks > 0) && m_APEFileInfo.nSampleRate > 0)
        {
            int nFrameMS = (nFrameBlocks * 1000) / m_APEFileInfo.nSampleRate;
            if (nFrameMS != 0)
            {
                nRetVal = (nFrameBytes * 8) / nFrameMS;
            }
        }
        break;
    }
    case APE_INFO_DECOMPRESSED_BITRATE:
        nRetVal = m_APEFileInfo.nDecompressedBitrate;
        break;
    case APE_INFO_PEAK_LEVEL:
        nRetVal = -1; // no longer supported
        break;
    case APE_INFO_SEEK_BIT:
    {
        int nFrame = nParam1;
        if (GET_FRAMES_START_ON_BYTES_BOUNDARIES(this)) 
        {
            nRetVal = 0;
        }
        else 
        {
            if (nFrame < 0 || nFrame >= m_APEFileInfo.nTotalFrames)
                nRetVal = 0;
            else
                nRetVal = m_APEFileInfo.spSeekBitTable[nFrame];
        }
        break;
    }
    case APE_INFO_SEEK_BYTE:
    {
        int nFrame = nParam1;
        if (nFrame < 0 || nFrame >= m_APEFileInfo.nTotalFrames)
            nRetVal = 0;
        else
            nRetVal = m_APEFileInfo.spSeekByteTable[nFrame] + m_APEFileInfo.nJunkHeaderBytes;
        break;
    }
    case APE_INFO_WAV_HEADER_DATA:
    {
        char * pBuffer = (char *) nParam1;
        int nMaxBytes = nParam2;
        
        if (m_APEFileInfo.nFormatFlags & MAC_FORMAT_FLAG_CREATE_WAV_HEADER)
        {
            if (sizeof(WAVE_HEADER) > nMaxBytes)
            {
                nRetVal = -1;
            }
            else
            {
                WAVEFORMATEX wfeFormat; GetInfo(APE_INFO_WAVEFORMATEX, (long) &wfeFormat, 0);
                WAVE_HEADER WAVHeader; FillWaveHeader(&WAVHeader, m_APEFileInfo.nWAVDataBytes, &wfeFormat,
                    m_APEFileInfo.nWAVTerminatingBytes);
                memcpy(pBuffer, &WAVHeader, sizeof(WAVE_HEADER));
                nRetVal = 0;
            }
        }
        else
        {
            if (m_APEFileInfo.nWAVHeaderBytes > nMaxBytes)
            {
                nRetVal = -1;
            }
            else
            {
                memcpy(pBuffer, m_APEFileInfo.spWaveHeaderData, m_APEFileInfo.nWAVHeaderBytes);
                nRetVal = 0;
            }
        }
        break;
    }
    case APE_INFO_WAV_TERMINATING_DATA:
    {
        char * pBuffer = (char *) nParam1;
        int nMaxBytes = nParam2;

        if (m_APEFileInfo.nWAVTerminatingBytes > nMaxBytes)
        {
            nRetVal = -1;
        }
        else
        {
            if (m_APEFileInfo.nWAVTerminatingBytes > 0) 
            {
                // variables
                int nOriginalFileLocation = m_spIO->GetPosition();
                unsigned int nBytesRead = 0;

                // check for a tag
                m_spIO->Seek(-(m_spAPETag->GetTagBytes() + m_APEFileInfo.nWAVTerminatingBytes), FILE_END);
                m_spIO->Read(pBuffer, m_APEFileInfo.nWAVTerminatingBytes, &nBytesRead);

                // restore the file pointer
                m_spIO->Seek(nOriginalFileLocation, FILE_BEGIN);
            }
            nRetVal = 0;
        }
        break;
    }
    case APE_INFO_WAVEFORMATEX:
    {
        WAVEFORMATEX * pWaveFormatEx = (WAVEFORMATEX *) nParam1;
        FillWaveFormatEx(pWaveFormatEx, m_APEFileInfo.nSampleRate, m_APEFileInfo.nBitsPerSample, m_APEFileInfo.nChannels);
        nRetVal = 0;
        break;
    }
    case APE_INFO_IO_SOURCE:
        nRetVal = (long) m_spIO.GetPtr();
        break;
    case APE_INFO_FRAME_BYTES:
    {
        int nFrame = nParam1;
        
        // bound-check the frame index
        if ((nFrame < 0) || (nFrame >= m_APEFileInfo.nTotalFrames)) 
        {
            nRetVal = -1;
        }
        else
        {
            if (nFrame != (m_APEFileInfo.nTotalFrames - 1)) 
                nRetVal = GetInfo(APE_INFO_SEEK_BYTE, nFrame + 1) - GetInfo(APE_INFO_SEEK_BYTE, nFrame);
            else 
                nRetVal = m_spIO->GetSize() - m_spAPETag->GetTagBytes() - m_APEFileInfo.nWAVTerminatingBytes - GetInfo(APE_INFO_SEEK_BYTE, nFrame);
        }
        break;
    }
    case APE_INFO_FRAME_BLOCKS:
    {
        int nFrame = nParam1;

        // bound-check the frame index
        if ((nFrame < 0) || (nFrame >= m_APEFileInfo.nTotalFrames)) 
        {
            nRetVal = -1;
        }
        else
        {
            if (nFrame != (m_APEFileInfo.nTotalFrames - 1)) 
                nRetVal = m_APEFileInfo.nBlocksPerFrame;
            else 
                nRetVal = m_APEFileInfo.nFinalFrameBlocks;
        }
        break;
    }
    case APE_INFO_TAG:
        nRetVal = (long) m_spAPETag.GetPtr();
        break;
    case APE_INTERNAL_INFO:
        nRetVal = (long) &m_APEFileInfo;
        break;
    }

    return nRetVal;
}

