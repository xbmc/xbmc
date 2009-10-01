#include "All.h"
#include "APEInfo.h"
#include "UnBitArray.h"
#include "BitArray.h"

const uint32 POWERS_OF_TWO_MINUS_ONE_REVERSED[33] = {4294967295,2147483647,1073741823,536870911,268435455,134217727,67108863,33554431,16777215,8388607,4194303,2097151,1048575,524287,262143,131071,65535,32767,16383,8191,4095,2047,1023,511,255,127,63,31,15,7,3,1,0};

const uint32 K_SUM_MIN_BOUNDARY[32] = {0,32,64,128,256,512,1024,2048,4096,8192,16384,32768,65536,131072,262144,524288,1048576,2097152,4194304,8388608,16777216,33554432,67108864,134217728,268435456,536870912,1073741824,2147483648,0,0,0,0};

const uint32 RANGE_TOTAL_1[65] = {0,14824,28224,39348,47855,53994,58171,60926,62682,63786,64463,64878,65126,65276,65365,65419,65450,65469,65480,65487,65491,65493,65494,65495,65496,65497,65498,65499,65500,65501,65502,65503,65504,65505,65506,65507,65508,65509,65510,65511,65512,65513,65514,65515,65516,65517,65518,65519,65520,65521,65522,65523,65524,65525,65526,65527,65528,65529,65530,65531,65532,65533,65534,65535,65536};
const uint32 RANGE_WIDTH_1[64] = {14824,13400,11124,8507,6139,4177,2755,1756,1104,677,415,248,150,89,54,31,19,11,7,4,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};

const uint32 RANGE_TOTAL_2[65] = {0,19578,36160,48417,56323,60899,63265,64435,64971,65232,65351,65416,65447,65466,65476,65482,65485,65488,65490,65491,65492,65493,65494,65495,65496,65497,65498,65499,65500,65501,65502,65503,65504,65505,65506,65507,65508,65509,65510,65511,65512,65513,65514,65515,65516,65517,65518,65519,65520,65521,65522,65523,65524,65525,65526,65527,65528,65529,65530,65531,65532,65533,65534,65535,65536};
const uint32 RANGE_WIDTH_2[64] = {19578,16582,12257,7906,4576,2366,1170,536,261,119,65,31,19,10,6,3,3,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,};

#define RANGE_OVERFLOW_TOTAL_WIDTH        65536
#define RANGE_OVERFLOW_SHIFT            16

#define CODE_BITS 32
#define TOP_VALUE ((unsigned int ) 1 << (CODE_BITS - 1))
#define SHIFT_BITS (CODE_BITS - 9)
#define EXTRA_BITS ((CODE_BITS - 2) % 8 + 1)
#define BOTTOM_VALUE (TOP_VALUE >> 8)

#define MODEL_ELEMENTS 64

/***********************************************************************************
Construction
***********************************************************************************/
CUnBitArray::CUnBitArray(CIO * pIO, int nVersion) 
{
    CreateHelper(pIO, 16384, nVersion);
    m_nFlushCounter = 0;
    m_nFinalizeCounter = 0;
}

CUnBitArray::~CUnBitArray()
{
    SAFE_ARRAY_DELETE(m_pBitArray)
}

unsigned int CUnBitArray::DecodeValue(DECODE_VALUE_METHOD DecodeMethod, int nParam1, int nParam2)
{
    switch (DecodeMethod)
    {
    case DECODE_VALUE_METHOD_UNSIGNED_INT:
        return DecodeValueXBits(32);
    }
    
    return 0;
}

void CUnBitArray::GenerateArray(int * pOutputArray, int nElements, int nBytesRequired) 
{
    GenerateArrayRange(pOutputArray, nElements);
}

__inline unsigned char CUnBitArray::GetC()
{
    unsigned char nValue = (unsigned char) (m_pBitArray[m_nCurrentBitIndex >> 5] >> (24 - (m_nCurrentBitIndex & 31)));
    m_nCurrentBitIndex += 8;
    return nValue;
}    

__inline int CUnBitArray::RangeDecodeFast(int nShift)
{
    while (m_RangeCoderInfo.range <= BOTTOM_VALUE)
    {   
        m_RangeCoderInfo.buffer = (m_RangeCoderInfo.buffer << 8) | ((m_pBitArray[m_nCurrentBitIndex >> 5] >> (24 - (m_nCurrentBitIndex & 31))) & 0xFF);
        m_nCurrentBitIndex += 8;
        m_RangeCoderInfo.low = (m_RangeCoderInfo.low << 8) | ((m_RangeCoderInfo.buffer >> 1) & 0xFF);
        m_RangeCoderInfo.range <<= 8;
    }

    // decode
       m_RangeCoderInfo.range = m_RangeCoderInfo.range >> nShift;
    return m_RangeCoderInfo.low / m_RangeCoderInfo.range;
}

__inline int CUnBitArray::RangeDecodeFastWithUpdate(int nShift)
{
    while (m_RangeCoderInfo.range <= BOTTOM_VALUE)
    {   
        m_RangeCoderInfo.buffer = (m_RangeCoderInfo.buffer << 8) | ((m_pBitArray[m_nCurrentBitIndex >> 5] >> (24 - (m_nCurrentBitIndex & 31))) & 0xFF);
        m_nCurrentBitIndex += 8;
        m_RangeCoderInfo.low = (m_RangeCoderInfo.low << 8) | ((m_RangeCoderInfo.buffer >> 1) & 0xFF);
        m_RangeCoderInfo.range <<= 8;
    }

    // decode
       m_RangeCoderInfo.range = m_RangeCoderInfo.range >> nShift;
    int nRetVal = m_RangeCoderInfo.low / m_RangeCoderInfo.range;
    m_RangeCoderInfo.low -= m_RangeCoderInfo.range * nRetVal;
    return nRetVal;
}

int CUnBitArray::DecodeValueRange(UNBIT_ARRAY_STATE & BitArrayState)
{
    // make sure there is room for the data
    // this is a little slower than ensuring a huge block to start with, but it's safer
    if (m_nCurrentBitIndex > m_nRefillBitThreshold)
    {
        FillBitArray();
    }

    int nValue = 0;

    if (m_nVersion >= 3990)
    {
        // figure the pivot value
        int nPivotValue = max(BitArrayState.nKSum / 32, 1);
        
        // get the overflow
        int nOverflow = 0;
        {
            // decode
            int nRangeTotal = RangeDecodeFast(RANGE_OVERFLOW_SHIFT);
            
            // lookup the symbol (must be a faster way than this)
            while (nRangeTotal >= RANGE_TOTAL_2[nOverflow + 1]) { nOverflow++; }
            
            // update
            m_RangeCoderInfo.low -= m_RangeCoderInfo.range * RANGE_TOTAL_2[nOverflow];
            m_RangeCoderInfo.range = m_RangeCoderInfo.range * RANGE_WIDTH_2[nOverflow];
            
            // get the working k
            if (nOverflow == (MODEL_ELEMENTS - 1))
            {
                nOverflow = RangeDecodeFastWithUpdate(16);
                nOverflow <<= 16;
                nOverflow |= RangeDecodeFastWithUpdate(16);
            }
        }

        // get the value
        int nBase = 0;
        {
            int nShift = 0;
            if (nPivotValue >= (1 << 16))
            {
                int nPivotValueBits = 0;
                while ((nPivotValue >> nPivotValueBits) > 0) { nPivotValueBits++; }
                int nSplitFactor = 1 << (nPivotValueBits - 16);

                int nPivotValueA = (nPivotValue / nSplitFactor) + 1;
                int nPivotValueB = nSplitFactor;

                while (m_RangeCoderInfo.range <= BOTTOM_VALUE)
                {   
                    m_RangeCoderInfo.buffer = (m_RangeCoderInfo.buffer << 8) | ((m_pBitArray[m_nCurrentBitIndex >> 5] >> (24 - (m_nCurrentBitIndex & 31))) & 0xFF);
                    m_nCurrentBitIndex += 8;
                    m_RangeCoderInfo.low = (m_RangeCoderInfo.low << 8) | ((m_RangeCoderInfo.buffer >> 1) & 0xFF);
                    m_RangeCoderInfo.range <<= 8;
                }
                  m_RangeCoderInfo.range = m_RangeCoderInfo.range / nPivotValueA;
                int nBaseA = m_RangeCoderInfo.low / m_RangeCoderInfo.range;
                m_RangeCoderInfo.low -= m_RangeCoderInfo.range * nBaseA;

                while (m_RangeCoderInfo.range <= BOTTOM_VALUE)
                {   
                    m_RangeCoderInfo.buffer = (m_RangeCoderInfo.buffer << 8) | ((m_pBitArray[m_nCurrentBitIndex >> 5] >> (24 - (m_nCurrentBitIndex & 31))) & 0xFF);
                    m_nCurrentBitIndex += 8;
                    m_RangeCoderInfo.low = (m_RangeCoderInfo.low << 8) | ((m_RangeCoderInfo.buffer >> 1) & 0xFF);
                    m_RangeCoderInfo.range <<= 8;
                }
                  m_RangeCoderInfo.range = m_RangeCoderInfo.range / nPivotValueB;
                int nBaseB = m_RangeCoderInfo.low / m_RangeCoderInfo.range;
                m_RangeCoderInfo.low -= m_RangeCoderInfo.range * nBaseB;

                nBase = nBaseA * nSplitFactor + nBaseB;
            }
            else
            {
                while (m_RangeCoderInfo.range <= BOTTOM_VALUE)
                {   
                    m_RangeCoderInfo.buffer = (m_RangeCoderInfo.buffer << 8) | ((m_pBitArray[m_nCurrentBitIndex >> 5] >> (24 - (m_nCurrentBitIndex & 31))) & 0xFF);
                    m_nCurrentBitIndex += 8;
                    m_RangeCoderInfo.low = (m_RangeCoderInfo.low << 8) | ((m_RangeCoderInfo.buffer >> 1) & 0xFF);
                    m_RangeCoderInfo.range <<= 8;
                }

                // decode
                   m_RangeCoderInfo.range = m_RangeCoderInfo.range / nPivotValue;
                int nBaseLower = m_RangeCoderInfo.low / m_RangeCoderInfo.range;
                m_RangeCoderInfo.low -= m_RangeCoderInfo.range * nBaseLower;

                nBase = nBaseLower;
            }
        }

        // build the value
        nValue = nBase + (nOverflow * nPivotValue);
    }
    else
    {
        // decode
        int nRangeTotal = RangeDecodeFast(RANGE_OVERFLOW_SHIFT);
        
        // lookup the symbol (must be a faster way than this)
        int nOverflow = 0;
        while (nRangeTotal >= RANGE_TOTAL_1[nOverflow + 1]) { nOverflow++; }
        
        // update
        m_RangeCoderInfo.low -= m_RangeCoderInfo.range * RANGE_TOTAL_1[nOverflow];
        m_RangeCoderInfo.range = m_RangeCoderInfo.range * RANGE_WIDTH_1[nOverflow];
        
        // get the working k
        int nTempK;
        if (nOverflow == (MODEL_ELEMENTS - 1))
        {
            nTempK = RangeDecodeFastWithUpdate(5);
            nOverflow = 0;
        }
        else
        {
            nTempK = (BitArrayState.k < 1) ? 0 : BitArrayState.k - 1;
        }
        
        // figure the extra bits on the left and the left value
        if (nTempK <= 16 || m_nVersion < 3910)
        {
            nValue = RangeDecodeFastWithUpdate(nTempK);
        }                    
        else
        {    
            int nX1 = RangeDecodeFastWithUpdate(16);
            int nX2 = RangeDecodeFastWithUpdate(nTempK - 16);
            nValue = nX1 | (nX2 << 16);
        }
            
        // build the value and output it
        nValue += (nOverflow << nTempK);
    }

    // update nKSum
    BitArrayState.nKSum += ((nValue + 1) / 2) - ((BitArrayState.nKSum + 16) >> 5);
    
    // update k
    if (BitArrayState.nKSum < K_SUM_MIN_BOUNDARY[BitArrayState.k]) 
        BitArrayState.k--;
    else if (BitArrayState.nKSum >= K_SUM_MIN_BOUNDARY[BitArrayState.k + 1]) 
        BitArrayState.k++;

    // output the value (converted to signed)
    return (nValue & 1) ? (nValue >> 1) + 1 : -(nValue >> 1);
}

void CUnBitArray::FlushState(UNBIT_ARRAY_STATE & BitArrayState)
{
    BitArrayState.k = 10;
    BitArrayState.nKSum = (1 << BitArrayState.k) * 16;
}

void CUnBitArray::FlushBitArray()
{
    AdvanceToByteBoundary();
    m_nCurrentBitIndex += 8; // ignore the first byte... (slows compression too much to not output this dummy byte)
    m_RangeCoderInfo.buffer = GetC();
    m_RangeCoderInfo.low = m_RangeCoderInfo.buffer >> (8 - EXTRA_BITS);
    m_RangeCoderInfo.range = (unsigned int) 1 << EXTRA_BITS;

    m_nRefillBitThreshold = (m_nBits - 512);
}

void CUnBitArray::Finalize()
{
    // normalize
    while (m_RangeCoderInfo.range <= BOTTOM_VALUE)
    {   
        m_nCurrentBitIndex += 8;
        m_RangeCoderInfo.range <<= 8;
    }
    
    // used to back-pedal the last two bytes out
    // this should never have been a problem because we've outputted and normalized beforehand
    // but stopped doing it as of 3.96 in case it accounted for rare decompression failures
    if (m_nVersion <= 3950)
        m_nCurrentBitIndex -= 16;
}

void CUnBitArray::GenerateArrayRange(int * pOutputArray, int nElements)
{
    UNBIT_ARRAY_STATE BitArrayState;
    FlushState(BitArrayState);
    FlushBitArray();
    
    for (int z = 0; z < nElements; z++)
    {
        pOutputArray[z] = DecodeValueRange(BitArrayState);
    }

    Finalize();    
}
