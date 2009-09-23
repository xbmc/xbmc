#ifndef APE_NEWPREDICTOR_H
#define APE_NEWPREDICTOR_H

#include "Predictor.h"

#include "RollBuffer.h"
#include "NNFilter.h"
#include "ScaledFirstOrderFilter.h"

/*************************************************************************************************
Functions to create the interfaces
*************************************************************************************************/
IPredictorCompress * __stdcall CreateIPredictorCompress();
IPredictorDecompress * __stdcall CreateIPredictorDecompress();

#define WINDOW_BLOCKS           512

#define BUFFER_COUNT            1
#define HISTORY_ELEMENTS        8
#define M_COUNT                 8

class CPredictorCompressNormal : public IPredictorCompress
{
public:
    CPredictorCompressNormal(int nCompressionLevel);
    virtual ~CPredictorCompressNormal();

    int CompressValue(int nA, int nB = 0);
    int Flush();

protected:

    // buffer information
    CRollBufferFast<int, WINDOW_BLOCKS, 10> m_rbPrediction;
    CRollBufferFast<int, WINDOW_BLOCKS, 9> m_rbAdapt;

    CScaledFirstOrderFilter<31, 5> m_Stage1FilterA;
    CScaledFirstOrderFilter<31, 5> m_Stage1FilterB;

    // adaption
    int m_aryM[9];
    
    // other
    int m_nCurrentIndex;
    CNNFilter * m_pNNFilter;
    CNNFilter * m_pNNFilter1;
    CNNFilter * m_pNNFilter2;
};

class CPredictorDecompressNormal3930to3950 : public IPredictorDecompress
{
public:
    CPredictorDecompressNormal3930to3950(int nCompressionLevel, int nVersion);
    virtual ~CPredictorDecompressNormal3930to3950();

    int DecompressValue(int nInput, int);
    int Flush();

protected:

    // buffer information
    int * m_pBuffer[BUFFER_COUNT];

    // adaption
    int m_aryM[M_COUNT];
    
    // buffer pointers
    int * m_pInputBuffer;

    // other
    int m_nCurrentIndex;
    int m_nLastValue;
    CNNFilter * m_pNNFilter;
    CNNFilter * m_pNNFilter1;
};

class CPredictorDecompress3950toCurrent : public IPredictorDecompress
{
public:
    CPredictorDecompress3950toCurrent(int nCompressionLevel, int nVersion);
    virtual ~CPredictorDecompress3950toCurrent();

    int DecompressValue(int nA, int nB = 0);
    int Flush();

protected:

    // adaption
    int m_aryMA[M_COUNT];
    int m_aryMB[M_COUNT];
    
    // buffer pointers
    CRollBufferFast<int, WINDOW_BLOCKS, 8> m_rbPredictionA;
    CRollBufferFast<int, WINDOW_BLOCKS, 8> m_rbPredictionB;

    CRollBufferFast<int, WINDOW_BLOCKS, 8> m_rbAdaptA;
    CRollBufferFast<int, WINDOW_BLOCKS, 8> m_rbAdaptB;

    CScaledFirstOrderFilter<31, 5> m_Stage1FilterA;
    CScaledFirstOrderFilter<31, 5> m_Stage1FilterB;

    // other
    int m_nCurrentIndex;
    int m_nLastValueA;
    int m_nVersion;
    CNNFilter * m_pNNFilter;
    CNNFilter * m_pNNFilter1;
    CNNFilter * m_pNNFilter2;
};

#endif // #ifndef APE_NEWPREDICTOR_H
