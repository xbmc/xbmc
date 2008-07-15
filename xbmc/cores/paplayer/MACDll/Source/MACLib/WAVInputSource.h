#ifndef APE_WAVINPUTSOURCE_H
#define APE_WAVINPUTSOURCE_H

#include "IO.h"

/*************************************************************************************
CInputSource - base input format class (allows multiple format support)
*************************************************************************************/
class CInputSource
{
public:

    // construction / destruction
    CInputSource(CIO * pIO, WAVEFORMATEX * pwfeSource, int * pTotalBlocks, int * pHeaderBytes, int * pTerminatingBytes, int * pErrorCode = NULL) { }
    CInputSource(const wchar_t * pSourceName, WAVEFORMATEX * pwfeSource, int * pTotalBlocks, int * pHeaderBytes, int * pTerminatingBytes, int * pErrorCode = NULL) { }
    virtual ~CInputSource() { }
    
    // get data
    virtual int GetData(unsigned char * pBuffer, int nBlocks, int * pBlocksRetrieved) = 0;
    
    // get header / terminating data
    virtual int GetHeaderData(unsigned char * pBuffer) = 0;
    virtual int GetTerminatingData(unsigned char * pBuffer) = 0;
};

/*************************************************************************************
CWAVInputSource - wraps working with WAV files (could be extended to any format)
*************************************************************************************/
class CWAVInputSource : public CInputSource
{
public:

    // construction / destruction
    CWAVInputSource(CIO * pIO, WAVEFORMATEX * pwfeSource, int * pTotalBlocks, int * pHeaderBytes, int * pTerminatingBytes, int * pErrorCode = NULL);
    CWAVInputSource(const wchar_t * pSourceName, WAVEFORMATEX * pwfeSource, int * pTotalBlocks, int * pHeaderBytes, int * pTerminatingBytes, int * pErrorCode = NULL);
    ~CWAVInputSource();
    
    // get data
    int GetData(unsigned char * pBuffer, int nBlocks, int * pBlocksRetrieved);
    
    // get header / terminating data
    int GetHeaderData(unsigned char * pBuffer);
    int GetTerminatingData(unsigned char * pBuffer);

private:

    int AnalyzeSource();

    CSmartPtr<CIO> m_spIO;
    
    WAVEFORMATEX m_wfeSource;
    int m_nHeaderBytes;
    int m_nDataBytes;
    int m_nTerminatingBytes;
    int m_nFileBytes;
    BOOL m_bIsValid;
};

/*************************************************************************************
Input souce creation
*************************************************************************************/
CInputSource * CreateInputSource(const wchar_t * pSourceName, WAVEFORMATEX * pwfeSource, int * pTotalBlocks, int * pHeaderBytes, int * pTerminatingBytes, int * pErrorCode = NULL);

#endif // #ifndef APE_WAVINPUTSOURCE_H
