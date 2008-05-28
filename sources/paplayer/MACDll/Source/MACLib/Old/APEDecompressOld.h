#ifndef _apedecompressold_h_
#define _apedecompressold_h_

#include "../APEDecompress.h"
#include "UnMAC.h"

class CAPEDecompressOld : public IAPEDecompress
{
public:
    CAPEDecompressOld(int * pErrorCode, CAPEInfo * pAPEInfo, int nStartBlock = -1, int nFinishBlock = -1);
    ~CAPEDecompressOld();

    int GetData(char * pBuffer, int nBlocks, int * pBlocksRetrieved);
    int Seek(int nBlockOffset);

    int GetInfo(APE_DECOMPRESS_FIELDS Field, int nParam1 = 0, int nParam2 = 0);
    
protected:

    // buffer
    CSmartPtr<char> m_spBuffer;
    int m_nBufferTail;
    
    // file info
    int m_nBlockAlign;
    int m_nCurrentFrame;

    // start / finish information
    int m_nStartBlock;
    int m_nFinishBlock;
    int m_nCurrentBlock;
    BOOL m_bIsRanged;

    // decoding tools    
    CUnMAC m_UnMAC;
    CSmartPtr<CAPEInfo> m_spAPEInfo;
    
    BOOL m_bDecompressorInitialized;
    int InitializeDecompressor();
};

#endif //_apedecompressold_h_

