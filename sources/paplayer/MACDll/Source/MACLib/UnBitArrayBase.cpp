#include "All.h"
#include "UnBitArrayBase.h"
#include "APEInfo.h"
#include "UnBitArray.h"
#ifdef BACKWARDS_COMPATIBILITY
    #include "Old/APEDecompressOld.h"
    #include "Old/UnBitArrayOld.h"
#endif

const uint32 POWERS_OF_TWO_MINUS_ONE[33] = {0,1,3,7,15,31,63,127,255,511,1023,2047,4095,8191,16383,32767,65535,131071,262143,524287,1048575,2097151,4194303,8388607,16777215,33554431,67108863,134217727,268435455,536870911,1073741823,2147483647,4294967295};

CUnBitArrayBase * CreateUnBitArray(IAPEDecompress * pAPEDecompress, int nVersion)
{
#ifdef BACKWARDS_COMPATIBILITY
    if (nVersion >= 3900)
        return (CUnBitArrayBase * ) new CUnBitArray(GET_IO(pAPEDecompress), nVersion);
    else
        return (CUnBitArrayBase * ) new CUnBitArrayOld(pAPEDecompress, nVersion);
#else
    return (CUnBitArrayBase * ) new CUnBitArray(GET_IO(pAPEDecompress), nVersion);
#endif
}

void CUnBitArrayBase::AdvanceToByteBoundary() 
{
    int nMod = m_nCurrentBitIndex % 8;
    if (nMod != 0) { m_nCurrentBitIndex += 8 - nMod; }
}

uint32 CUnBitArrayBase::DecodeValueXBits(uint32 nBits) 
{
    // get more data if necessary
    if ((m_nCurrentBitIndex + nBits) >= m_nBits)
        FillBitArray();

    // variable declares
    uint32 nLeftBits = 32 - (m_nCurrentBitIndex & 31);
    uint32 nBitArrayIndex = m_nCurrentBitIndex >> 5;
    m_nCurrentBitIndex += nBits;
    
    // if their isn't an overflow to the right value, get the value and exit
    if (nLeftBits >= nBits)
        return (m_pBitArray[nBitArrayIndex] & (POWERS_OF_TWO_MINUS_ONE[nLeftBits])) >> (nLeftBits - nBits);
    
    // must get the "split" value from left and right
    int nRightBits = nBits - nLeftBits;
    
    uint32 nLeftValue = ((m_pBitArray[nBitArrayIndex] & POWERS_OF_TWO_MINUS_ONE[nLeftBits]) << nRightBits);
    uint32 nRightValue = (m_pBitArray[nBitArrayIndex + 1] >> (32 - nRightBits));
    return (nLeftValue | nRightValue);
}

int CUnBitArrayBase::FillAndResetBitArray(int nFileLocation, int nNewBitIndex) 
{
    // reset the bit index
    m_nCurrentBitIndex = nNewBitIndex;
    
    // seek if necessary
    if (nFileLocation != -1)
    {
        if (m_pIO->Seek(nFileLocation, FILE_BEGIN) != 0)
            return ERROR_IO_READ;
    }
        
    // read the new data into the bit array
    unsigned int nBytesRead = 0;
    if (m_pIO->Read(((unsigned char *) m_pBitArray), m_nBytes, &nBytesRead) != 0)
        return ERROR_IO_READ;

    return 0;
}

int CUnBitArrayBase::FillBitArray() 
{
    // get the bit array index
    uint32 nBitArrayIndex = m_nCurrentBitIndex >> 5;
    
    // move the remaining data to the front
    memmove((void *) (m_pBitArray), (const void *) (m_pBitArray + nBitArrayIndex), m_nBytes - (nBitArrayIndex * 4));
    
    // read the new data
    int nBytesToRead = nBitArrayIndex * 4;
    unsigned int nBytesRead = 0;
    int nRetVal = m_pIO->Read((unsigned char *) (m_pBitArray + m_nElements - nBitArrayIndex), nBytesToRead, &nBytesRead);
    
    // adjust the m_Bit pointer
    m_nCurrentBitIndex = m_nCurrentBitIndex & 31;
    
    // return
    return (nRetVal == 0) ? 0 : ERROR_IO_READ;
}

int CUnBitArrayBase::CreateHelper(CIO * pIO, int nBytes, int nVersion)
{
    // check the parameters
    if ((pIO == NULL) || (nBytes <= 0)) { return ERROR_BAD_PARAMETER; }

    // save the size
    m_nElements = nBytes / 4;
    m_nBytes = m_nElements * 4;
    m_nBits = m_nBytes * 8;
    
    // set the variables
    m_pIO = pIO;
    m_nVersion = nVersion;
    m_nCurrentBitIndex = 0;
    
    // create the bitarray
    m_pBitArray = new uint32 [m_nElements];
    
    return (m_pBitArray != NULL) ? 0 : ERROR_INSUFFICIENT_MEMORY;
}
