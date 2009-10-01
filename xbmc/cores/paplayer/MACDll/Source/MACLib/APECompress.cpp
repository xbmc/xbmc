#include "All.h"
#include "APECompress.h"
#include IO_HEADER_FILE
#include "APECompressCreate.h"
#include "WAVInputSource.h"

CAPECompress::CAPECompress()
{
    m_nBufferHead        = 0;
    m_nBufferTail        = 0;
    m_nBufferSize        = 0;
    m_bBufferLocked        = FALSE;
    m_bOwnsOutputIO        = FALSE;
    m_pioOutput            = NULL;

    m_spAPECompressCreate.Assign(new CAPECompressCreate());

    m_pBuffer = NULL;
}

CAPECompress::~CAPECompress()
{
    SAFE_ARRAY_DELETE(m_pBuffer)

    if (m_bOwnsOutputIO)
    {
        SAFE_DELETE(m_pioOutput)
    }
}

int CAPECompress::Start(const wchar_t * pOutputFilename, const WAVEFORMATEX * pwfeInput, int nMaxAudioBytes, int nCompressionLevel, const void * pHeaderData, int nHeaderBytes)
{
    m_pioOutput = new IO_CLASS_NAME;
    m_bOwnsOutputIO = TRUE;
    
    if (m_pioOutput->Create(pOutputFilename) != 0)
    {
        return ERROR_INVALID_OUTPUT_FILE;
    }
        
    m_spAPECompressCreate->Start(m_pioOutput, pwfeInput, nMaxAudioBytes, nCompressionLevel,
        pHeaderData, nHeaderBytes);
    
    SAFE_ARRAY_DELETE(m_pBuffer)
    m_nBufferSize = m_spAPECompressCreate->GetFullFrameBytes();
    m_pBuffer = new unsigned char [m_nBufferSize];
    memcpy(&m_wfeInput, pwfeInput, sizeof(WAVEFORMATEX));

    return ERROR_SUCCESS;
}

int CAPECompress::StartEx(CIO * pioOutput, const WAVEFORMATEX * pwfeInput, int nMaxAudioBytes, int nCompressionLevel, const void * pHeaderData, int nHeaderBytes)
{
    m_pioOutput = pioOutput;
    m_bOwnsOutputIO = FALSE;

    m_spAPECompressCreate->Start(m_pioOutput, pwfeInput, nMaxAudioBytes, nCompressionLevel,
        pHeaderData, nHeaderBytes);

    SAFE_ARRAY_DELETE(m_pBuffer)
    m_nBufferSize = m_spAPECompressCreate->GetFullFrameBytes();
    m_pBuffer = new unsigned char [m_nBufferSize];
    memcpy(&m_wfeInput, pwfeInput, sizeof(WAVEFORMATEX));

    return ERROR_SUCCESS;
}

int CAPECompress::GetBufferBytesAvailable()
{
    return m_nBufferSize - m_nBufferTail;
}

int CAPECompress::UnlockBuffer(int nBytesAdded, BOOL bProcess)
{
    if (m_bBufferLocked == FALSE)
        return ERROR_UNDEFINED;
    
    m_nBufferTail += nBytesAdded;
    m_bBufferLocked = FALSE;
    
    if (bProcess)
    {
        int nRetVal = ProcessBuffer();
        if (nRetVal != 0) { return nRetVal; }
    }
    
    return ERROR_SUCCESS;
}

unsigned char * CAPECompress::LockBuffer(int * pBytesAvailable)
{
    if (m_pBuffer == NULL) { return NULL; }
    
    if (m_bBufferLocked)
        return NULL;
    
    m_bBufferLocked = TRUE;
    
    if (pBytesAvailable)
        *pBytesAvailable = GetBufferBytesAvailable();
    
    return &m_pBuffer[m_nBufferTail];
}

int CAPECompress::AddData(unsigned char * pData, int nBytes)
{
    if (m_pBuffer == NULL) return ERROR_INSUFFICIENT_MEMORY;

    int nBytesDone = 0;
    
    while (nBytesDone < nBytes)
    {
        // lock the buffer
        int nBytesAvailable = 0;
        unsigned char * pBuffer = LockBuffer(&nBytesAvailable);
        if (pBuffer == NULL || nBytesAvailable <= 0)
            return ERROR_UNDEFINED;
        
        // calculate how many bytes to copy and add that much to the buffer
        int nBytesToProcess = min(nBytesAvailable, nBytes - nBytesDone);
        memcpy(pBuffer, &pData[nBytesDone], nBytesToProcess);
                        
        // unlock the buffer (fail if not successful)
        int nRetVal = UnlockBuffer(nBytesToProcess);
        if (nRetVal != ERROR_SUCCESS)
                return nRetVal;

        // update our progress
        nBytesDone += nBytesToProcess;
    }

    return ERROR_SUCCESS;
} 

int CAPECompress::Finish(unsigned char * pTerminatingData, int nTerminatingBytes, int nWAVTerminatingBytes)
{
    RETURN_ON_ERROR(ProcessBuffer(TRUE))
    return m_spAPECompressCreate->Finish(pTerminatingData, nTerminatingBytes, nWAVTerminatingBytes);
}

int CAPECompress::Kill()
{
    return ERROR_SUCCESS;
}

int CAPECompress::ProcessBuffer(BOOL bFinalize)
{
    if (m_pBuffer == NULL) { return ERROR_UNDEFINED; }
    
    try
    {
        // process as much as possible
        int nThreshold = (bFinalize) ? 0 : m_spAPECompressCreate->GetFullFrameBytes();
        
        while ((m_nBufferTail - m_nBufferHead) >= nThreshold)
        {
            int nFrameBytes = min(m_spAPECompressCreate->GetFullFrameBytes(), m_nBufferTail - m_nBufferHead);
            
            if (nFrameBytes == 0)
                break;

            int nRetVal = m_spAPECompressCreate->EncodeFrame(&m_pBuffer[m_nBufferHead], nFrameBytes);
            if (nRetVal != 0) { return nRetVal; }
            
            m_nBufferHead += nFrameBytes;
        }
        
        // shift the buffer
        if (m_nBufferHead != 0)
        {
            int nBytesLeft = m_nBufferTail - m_nBufferHead;
            
            if (nBytesLeft != 0)
                memmove(m_pBuffer, &m_pBuffer[m_nBufferHead], nBytesLeft);
            
            m_nBufferTail -= m_nBufferHead;
            m_nBufferHead = 0;
        }
    }
    catch(...)
    {
        return ERROR_UNDEFINED;
    }
    
    return ERROR_SUCCESS;
}

int CAPECompress::AddDataFromInputSource(CInputSource * pInputSource, int nMaxBytes, int * pBytesAdded)
{
    // error check the parameters
    if (pInputSource == NULL) return ERROR_BAD_PARAMETER;

    // initialize
    if (pBytesAdded) *pBytesAdded = 0;
        
    // lock the buffer
    int nBytesAvailable = 0;
    unsigned char * pBuffer = LockBuffer(&nBytesAvailable);
    if ((pBuffer == NULL) || (nBytesAvailable == 0))
        return ERROR_INSUFFICIENT_MEMORY;
    
    // calculate the 'ideal' number of bytes
    unsigned int nBytesRead = 0;

    int nIdealBytes = m_spAPECompressCreate->GetFullFrameBytes() - (m_nBufferTail - m_nBufferHead);
    if (nIdealBytes > 0)
    {
        // get the data
        int nBytesToAdd = nBytesAvailable;
        
        if (nMaxBytes > 0)
        {
            if (nBytesToAdd > nMaxBytes) nBytesToAdd = nMaxBytes;
        }

        if (nBytesToAdd > nIdealBytes) nBytesToAdd = nIdealBytes;

        // always make requests along block boundaries
        while ((nBytesToAdd % m_wfeInput.nBlockAlign) != 0)
            nBytesToAdd--;

        int nBlocksToAdd = nBytesToAdd / m_wfeInput.nBlockAlign;

        // get data
        int nBlocksAdded = 0;
        int nRetVal = pInputSource->GetData(pBuffer, nBlocksToAdd, &nBlocksAdded);
        if (nRetVal != 0)
            return ERROR_IO_READ;
        else
            nBytesRead = (nBlocksAdded * m_wfeInput.nBlockAlign);
        
        // store the bytes read
        if (pBytesAdded)
            *pBytesAdded = nBytesRead;
    }
        
    // unlock the data and process
    int nRetVal = UnlockBuffer(nBytesRead, TRUE);
    if (nRetVal != 0)
    {
        return nRetVal;
    }
    
    return ERROR_SUCCESS;
}

