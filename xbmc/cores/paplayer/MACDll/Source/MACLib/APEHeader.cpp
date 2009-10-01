#include "All.h"
#include "APEHeader.h"
#include "MACLib.h"
#include "APEInfo.h"

#ifndef __APPLE__
#include <linux/smbno.h>
#endif

// TODO: should push and pop the file position

CAPEHeader::CAPEHeader(CIO * pIO)
{
    m_pIO = pIO;
}

CAPEHeader::~CAPEHeader()
{

}

int CAPEHeader::FindDescriptor(BOOL bSeek)
{
    // store the original location and seek to the beginning
    int nOriginalFileLocation = m_pIO->GetPosition();
    m_pIO->Seek(0, FILE_BEGIN);

    // set the default junk bytes to 0
    int nJunkBytes = 0;

    // skip an ID3v2 tag (which we really don't support anyway...)
    unsigned int nBytesRead = 0; 
    unsigned char cID3v2Header[10];
    m_pIO->Read((unsigned char *) cID3v2Header, 10, &nBytesRead);
    if (cID3v2Header[0] == 'I' && cID3v2Header[1] == 'D' && cID3v2Header[2] == '3') 
    {
        // why is it so hard to figure the lenght of an ID3v2 tag ?!?
        unsigned int nLength = *((unsigned int *) &cID3v2Header[6]);

        unsigned int nSyncSafeLength = 0;
        nSyncSafeLength = (cID3v2Header[6] & 127) << 21;
        nSyncSafeLength += (cID3v2Header[7] & 127) << 14;
        nSyncSafeLength += (cID3v2Header[8] & 127) << 7;
        nSyncSafeLength += (cID3v2Header[9] & 127);

        BOOL bHasTagFooter = FALSE;

        if (cID3v2Header[5] & 16) 
        {
            bHasTagFooter = TRUE;
            nJunkBytes = nSyncSafeLength + 20;
        }
        else 
        {
            nJunkBytes = nSyncSafeLength + 10;
        }

        // error check
        if (cID3v2Header[5] & 64)
        {
            // this ID3v2 length calculator algorithm can't cope with extended headers
            // we should be ok though, because the scan for the MAC header below should
            // really do the trick
        }

        m_pIO->Seek(nJunkBytes, FILE_BEGIN);

        // scan for padding (slow and stupid, but who cares here...)
        if (!bHasTagFooter)
        {
            char cTemp = 0;
            m_pIO->Read((unsigned char *) &cTemp, 1, &nBytesRead);
            while (cTemp == 0 && nBytesRead == 1)
            {
                nJunkBytes++;
                m_pIO->Read((unsigned char *) &cTemp, 1, &nBytesRead);
            }
        }
    }
    m_pIO->Seek(nJunkBytes, FILE_BEGIN);

    // scan until we hit the APE_DESCRIPTOR, the end of the file, or 1 MB later
    unsigned int nGoalID = (' ' << 24) | ('C' << 16) | ('A' << 8) | ('M');
    unsigned int nReadID = 0;
    int nRetVal = m_pIO->Read(&nReadID, 4, &nBytesRead);
    if (nRetVal != 0 || nBytesRead != 4) return ERROR_UNDEFINED;

    nBytesRead = 1;
    int nScanBytes = 0;
    while ((nGoalID != nReadID) && (nBytesRead == 1) && (nScanBytes < (1024 * 1024)))
    {
        unsigned char cTemp;
        m_pIO->Read(&cTemp, 1, &nBytesRead);
        nReadID = (((unsigned int) cTemp) << 24) | (nReadID >> 8);
        nJunkBytes++;
        nScanBytes++;
    }

    if (nGoalID != nReadID)
        nJunkBytes = -1;

    // seek to the proper place (depending on result and settings)
    if (bSeek && (nJunkBytes != -1))
    {
        // successfully found the start of the file (seek to it and return)
        m_pIO->Seek(nJunkBytes, FILE_BEGIN);
    }
    else
    {
        // restore the original file pointer
        m_pIO->Seek(nOriginalFileLocation, FILE_BEGIN);
    }

    return nJunkBytes;
}

int CAPEHeader::Analyze(APE_FILE_INFO * pInfo)
{
    // error check
    if ((m_pIO == NULL) || (pInfo == NULL))
        return ERROR_INVALID_FUNCTION_PARAMETER;

    // variables
    unsigned int nBytesRead = 0;

    // find the descriptor
    pInfo->nJunkHeaderBytes = FindDescriptor(TRUE);
    if (pInfo->nJunkHeaderBytes < 0)
        return ERROR_UNDEFINED;

    // read the first 8 bytes of the descriptor (ID and version)
    APE_COMMON_HEADER CommonHeader; memset(&CommonHeader, 0, sizeof(APE_COMMON_HEADER));
    m_pIO->Read(&CommonHeader, sizeof(APE_COMMON_HEADER), &nBytesRead);

    // make sure we're at the ID
    if (CommonHeader.cID[0] != 'M' || CommonHeader.cID[1] != 'A' || CommonHeader.cID[2] != 'C' || CommonHeader.cID[3] != ' ')
        return ERROR_UNDEFINED;

    int nRetVal = ERROR_UNDEFINED;

    if (CommonHeader.nVersion >= 3980)
    {
        // current header format
        nRetVal = AnalyzeCurrent(pInfo);
    }
    else
    {
        // legacy support
        nRetVal = AnalyzeOld(pInfo);
    }

    return nRetVal;
}

int CAPEHeader::AnalyzeCurrent(APE_FILE_INFO * pInfo)
{
    // variable declares
    unsigned int nBytesRead = 0;
    pInfo->spAPEDescriptor.Assign(new APE_DESCRIPTOR); memset(pInfo->spAPEDescriptor, 0, sizeof(APE_DESCRIPTOR));
    APE_HEADER APEHeader; memset(&APEHeader, 0, sizeof(APEHeader));

    // read the descriptor
    m_pIO->Seek(pInfo->nJunkHeaderBytes, FILE_BEGIN);
    m_pIO->Read(pInfo->spAPEDescriptor, sizeof(APE_DESCRIPTOR), &nBytesRead);

    if ((pInfo->spAPEDescriptor->nDescriptorBytes - nBytesRead) > 0)
        m_pIO->Seek(pInfo->spAPEDescriptor->nDescriptorBytes - nBytesRead, FILE_CURRENT);

    // read the header
    m_pIO->Read(&APEHeader, sizeof(APEHeader), &nBytesRead);

    if ((pInfo->spAPEDescriptor->nHeaderBytes - nBytesRead) > 0)
        m_pIO->Seek(pInfo->spAPEDescriptor->nHeaderBytes - nBytesRead, FILE_CURRENT);

    // fill the APE info structure
    pInfo->nVersion                = int(pInfo->spAPEDescriptor->nVersion);
    pInfo->nCompressionLevel    = int(APEHeader.nCompressionLevel);
    pInfo->nFormatFlags            = int(APEHeader.nFormatFlags);
    pInfo->nTotalFrames            = int(APEHeader.nTotalFrames);
    pInfo->nFinalFrameBlocks    = int(APEHeader.nFinalFrameBlocks);
    pInfo->nBlocksPerFrame        = int(APEHeader.nBlocksPerFrame);
    pInfo->nChannels            = int(APEHeader.nChannels);
    pInfo->nSampleRate            = int(APEHeader.nSampleRate);
    pInfo->nBitsPerSample        = int(APEHeader.nBitsPerSample);
    pInfo->nBytesPerSample        = pInfo->nBitsPerSample / 8;
    pInfo->nBlockAlign            = pInfo->nBytesPerSample * pInfo->nChannels;
    pInfo->nTotalBlocks            = (APEHeader.nTotalFrames == 0) ? 0 : ((APEHeader.nTotalFrames -  1) * pInfo->nBlocksPerFrame) + APEHeader.nFinalFrameBlocks;
    pInfo->nWAVHeaderBytes        = (APEHeader.nFormatFlags & MAC_FORMAT_FLAG_CREATE_WAV_HEADER) ? sizeof(WAVE_HEADER) : pInfo->spAPEDescriptor->nHeaderDataBytes;
    pInfo->nWAVTerminatingBytes    = pInfo->spAPEDescriptor->nTerminatingDataBytes;
    pInfo->nWAVDataBytes        = pInfo->nTotalBlocks * pInfo->nBlockAlign;
    pInfo->nWAVTotalBytes        = pInfo->nWAVDataBytes + pInfo->nWAVHeaderBytes + pInfo->nWAVTerminatingBytes;
    pInfo->nAPETotalBytes        = m_pIO->GetSize();
    pInfo->nLengthMS            = int((double(pInfo->nTotalBlocks) * double(1000)) / double(pInfo->nSampleRate));
    pInfo->nAverageBitrate        = (pInfo->nLengthMS <= 0) ? 0 : int((double(pInfo->nAPETotalBytes) * double(8)) / double(pInfo->nLengthMS));
    pInfo->nDecompressedBitrate = (pInfo->nBlockAlign * pInfo->nSampleRate * 8) / 1000;
    pInfo->nSeekTableElements    = pInfo->spAPEDescriptor->nSeekTableBytes / 4;

    // get the seek tables (really no reason to get the whole thing if there's extra)
    pInfo->spSeekByteTable.Assign(new uint32 [pInfo->nSeekTableElements], TRUE);
    if (pInfo->spSeekByteTable == NULL) { return ERROR_UNDEFINED; }

    m_pIO->Read((unsigned char *) pInfo->spSeekByteTable.GetPtr(), 4 * pInfo->nSeekTableElements, &nBytesRead);

    // get the wave header
    if (!(APEHeader.nFormatFlags & MAC_FORMAT_FLAG_CREATE_WAV_HEADER))
    {
        pInfo->spWaveHeaderData.Assign(new unsigned char [pInfo->nWAVHeaderBytes], TRUE);
        if (pInfo->spWaveHeaderData == NULL) { return ERROR_UNDEFINED; }
        m_pIO->Read((unsigned char *) pInfo->spWaveHeaderData, pInfo->nWAVHeaderBytes, &nBytesRead);
    }

    return ERROR_SUCCESS;
}

int CAPEHeader::AnalyzeOld(APE_FILE_INFO * pInfo)
{
    // variable declares
    unsigned int nBytesRead = 0;

    // read the MAC header from the file
    APE_HEADER_OLD APEHeader;
    m_pIO->Seek(pInfo->nJunkHeaderBytes, FILE_BEGIN);
    m_pIO->Read((unsigned char *) &APEHeader, sizeof(APEHeader), &nBytesRead);

    // fail on 0 length APE files (catches non-finalized APE files)
    if (APEHeader.nTotalFrames == 0)
        return ERROR_UNDEFINED;

    int nPeakLevel = -1;
    if (APEHeader.nFormatFlags & MAC_FORMAT_FLAG_HAS_PEAK_LEVEL)
        m_pIO->Read((unsigned char *) &nPeakLevel, 4, &nBytesRead);

    if (APEHeader.nFormatFlags & MAC_FORMAT_FLAG_HAS_SEEK_ELEMENTS)
        m_pIO->Read((unsigned char *) &pInfo->nSeekTableElements, 4, &nBytesRead);
    else
        pInfo->nSeekTableElements = APEHeader.nTotalFrames;
    
    // fill the APE info structure
    pInfo->nVersion                = int(APEHeader.nVersion);
    pInfo->nCompressionLevel    = int(APEHeader.nCompressionLevel);
    pInfo->nFormatFlags            = int(APEHeader.nFormatFlags);
    pInfo->nTotalFrames            = int(APEHeader.nTotalFrames);
    pInfo->nFinalFrameBlocks    = int(APEHeader.nFinalFrameBlocks);
    pInfo->nBlocksPerFrame        = ((APEHeader.nVersion >= 3900) || ((APEHeader.nVersion >= 3800) && (APEHeader.nCompressionLevel == COMPRESSION_LEVEL_EXTRA_HIGH))) ? 73728 : 9216;
    if ((APEHeader.nVersion >= 3950)) pInfo->nBlocksPerFrame = 73728 * 4;
    pInfo->nChannels            = int(APEHeader.nChannels);
    pInfo->nSampleRate            = int(APEHeader.nSampleRate);
    pInfo->nBitsPerSample        = (pInfo->nFormatFlags & MAC_FORMAT_FLAG_8_BIT) ? 8 : ((pInfo->nFormatFlags & MAC_FORMAT_FLAG_24_BIT) ? 24 : 16);
    pInfo->nBytesPerSample        = pInfo->nBitsPerSample / 8;
    pInfo->nBlockAlign            = pInfo->nBytesPerSample * pInfo->nChannels;
    pInfo->nTotalBlocks            = (APEHeader.nTotalFrames == 0) ? 0 : ((APEHeader.nTotalFrames -  1) * pInfo->nBlocksPerFrame) + APEHeader.nFinalFrameBlocks;
    pInfo->nWAVHeaderBytes        = (APEHeader.nFormatFlags & MAC_FORMAT_FLAG_CREATE_WAV_HEADER) ? sizeof(WAVE_HEADER) : APEHeader.nHeaderBytes;
    pInfo->nWAVTerminatingBytes    = int(APEHeader.nTerminatingBytes);
    pInfo->nWAVDataBytes        = pInfo->nTotalBlocks * pInfo->nBlockAlign;
    pInfo->nWAVTotalBytes        = pInfo->nWAVDataBytes + pInfo->nWAVHeaderBytes + pInfo->nWAVTerminatingBytes;
    pInfo->nAPETotalBytes        = m_pIO->GetSize();
    pInfo->nLengthMS            = int((double(pInfo->nTotalBlocks) * double(1000)) / double(pInfo->nSampleRate));
    pInfo->nAverageBitrate        = (pInfo->nLengthMS <= 0) ? 0 : int((double(pInfo->nAPETotalBytes) * double(8)) / double(pInfo->nLengthMS));
    pInfo->nDecompressedBitrate = (pInfo->nBlockAlign * pInfo->nSampleRate * 8) / 1000;

    // get the wave header
    if (!(APEHeader.nFormatFlags & MAC_FORMAT_FLAG_CREATE_WAV_HEADER))
    {
        pInfo->spWaveHeaderData.Assign(new unsigned char [APEHeader.nHeaderBytes], TRUE);
        if (pInfo->spWaveHeaderData == NULL) { return ERROR_UNDEFINED; }
        m_pIO->Read((unsigned char *) pInfo->spWaveHeaderData, APEHeader.nHeaderBytes, &nBytesRead);
    }

    // get the seek tables (really no reason to get the whole thing if there's extra)
    pInfo->spSeekByteTable.Assign(new uint32 [pInfo->nSeekTableElements], TRUE);
    if (pInfo->spSeekByteTable == NULL) { return ERROR_UNDEFINED; }

    m_pIO->Read((unsigned char *) pInfo->spSeekByteTable.GetPtr(), 4 * pInfo->nSeekTableElements, &nBytesRead);

    if (APEHeader.nVersion <= 3800) 
    {
        pInfo->spSeekBitTable.Assign(new unsigned char [pInfo->nSeekTableElements], TRUE);
        if (pInfo->spSeekBitTable == NULL) { return ERROR_UNDEFINED; }

        m_pIO->Read((unsigned char *) pInfo->spSeekBitTable, pInfo->nSeekTableElements, &nBytesRead);
    }

    return ERROR_SUCCESS;
}

