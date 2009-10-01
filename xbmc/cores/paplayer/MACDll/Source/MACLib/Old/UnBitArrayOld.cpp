#include "All.h"
#ifdef BACKWARDS_COMPATIBILITY

#include "../APEInfo.h"
#include "UnBitArrayOld.h"
#include "../BitArray.h"

const uint32 K_SUM_MIN_BOUNDARY_OLD[32] = {0,128,256,512,1024,2048,4096,8192,16384,32768,65536,131072,262144,524288,1048576,2097152,4194304,8388608,16777216,33554432,67108864,134217728,268435456,536870912,1073741824,2147483648,0,0,0,0,0,0};
const uint32 K_SUM_MAX_BOUNDARY_OLD[32] = {128,256,512,1024,2048,4096,8192,16384,32768,65536,131072,262144,524288,1048576,2097152,4194304,8388608,16777216,33554432,67108864,134217728,268435456,536870912,1073741824,2147483648,0,0,0,0,0,0,0};
const uint32 Powers_of_Two[32] = {1,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,16384,32768,65536,131072,262144,524288,1048576,2097152,4194304,8388608,16777216,33554432,67108864,134217728,268435456,536870912,1073741824,2147483648};
const uint32 Powers_of_Two_Reversed[32] = {2147483648,1073741824,536870912,268435456,134217728,67108864,33554432,16777216,8388608,4194304,2097152,1048576,524288,262144,131072,65536,32768,16384,8192,4096,2048,1024,512,256,128,64,32,16,8,4,2,1};
const uint32 Powers_of_Two_Minus_One[33] = {0,1,3,7,15,31,63,127,255,511,1023,2047,4095,8191,16383,32767,65535,131071,262143,524287,1048575,2097151,4194303,8388607,16777215,33554431,67108863,134217727,268435455,536870911,1073741823,2147483647,4294967295};
const uint32 Powers_of_Two_Minus_One_Reversed[33] = {4294967295,2147483647,1073741823,536870911,268435455,134217727,67108863,33554431,16777215,8388607,4194303,2097151,1048575,524287,262143,131071,65535,32767,16383,8191,4095,2047,1023,511,255,127,63,31,15,7,3,1,0};

const uint32 K_SUM_MIN_BOUNDARY[32] = {0,32,64,128,256,512,1024,2048,4096,8192,16384,32768,65536,131072,262144,524288,1048576,2097152,4194304,8388608,16777216,33554432,67108864,134217728,268435456,536870912,1073741824,2147483648,0,0,0,0};
const uint32 K_SUM_MAX_BOUNDARY[32] = {32,64,128,256,512,1024,2048,4096,8192,16384,32768,65536,131072,262144,524288,1048576,2097152,4194304,8388608,16777216,33554432,67108864,134217728,268435456,536870912,1073741824,2147483648,0,0,0,0,0};

/***********************************************************************************
Construction
***********************************************************************************/
CUnBitArrayOld::CUnBitArrayOld(IAPEDecompress * pAPEDecompress, int nVersion) 
{
    int nBitArrayBytes = 262144;

    // calculate the bytes
    if (nVersion <= 3880)
    {
        int nMaxFrameBytes = (pAPEDecompress->GetInfo(APE_INFO_BLOCKS_PER_FRAME) * 50) / 8;
        nBitArrayBytes = 65536;
        while (nBitArrayBytes < nMaxFrameBytes)
        {
            nBitArrayBytes <<= 1;
        }
        
        nBitArrayBytes = max(nBitArrayBytes, 262144);
    }
    else if (nVersion <= 3890)
    {
        nBitArrayBytes = 65536;
    }
    else
    {
        // error
    }
    
    CreateHelper(GET_IO(pAPEDecompress), nBitArrayBytes, nVersion);

    // set the refill threshold
    if (m_nVersion <= 3880)
        m_nRefillBitThreshold = (m_nBits - (16384 * 8));
    else
        m_nRefillBitThreshold = (m_nBits - 512);
}

CUnBitArrayOld::~CUnBitArrayOld()
{
    SAFE_ARRAY_DELETE(m_pBitArray)
}

////////////////////////////////////////////////////////////////////////////////////
// Gets the number of m_nBits of data left in the m_nCurrentBitIndex array
////////////////////////////////////////////////////////////////////////////////////
uint32 CUnBitArrayOld::GetBitsRemaining()
{
    return (m_nElements * 32 - m_nCurrentBitIndex);
}

////////////////////////////////////////////////////////////////////////////////////
// Gets a rice value from the array
////////////////////////////////////////////////////////////////////////////////////
uint32 CUnBitArrayOld::DecodeValueRiceUnsigned(uint32 k) 
{
    // variable declares
    uint32 v;
    
    // plug through the string of 0's (the overflow)
    uint32 BitInitial = m_nCurrentBitIndex;
    while (!(m_pBitArray[m_nCurrentBitIndex >> 5] & Powers_of_Two_Reversed[m_nCurrentBitIndex++ & 31])) {}
    
    // if k = 0, your done
    if (k == 0)
        return (m_nCurrentBitIndex - BitInitial - 1);
    
    // put the overflow value into v
    v = (m_nCurrentBitIndex - BitInitial - 1) << k;
    
    return v | DecodeValueXBits(k);
}

////////////////////////////////////////////////////////////////////////////////////
// Get the optimal k for a given value
////////////////////////////////////////////////////////////////////////////////////
__inline uint32 CUnBitArrayOld::Get_K(uint32 x) 
{
    if (x == 0)    return 0;

    uint32 k = 0;
    while (x >= Powers_of_Two[++k]) {}
    return k;    
}

unsigned int CUnBitArrayOld::DecodeValue(DECODE_VALUE_METHOD DecodeMethod, int nParam1, int nParam2)
{
    switch (DecodeMethod)
    {
    case DECODE_VALUE_METHOD_UNSIGNED_INT:
        return DecodeValueXBits(32);
    case DECODE_VALUE_METHOD_UNSIGNED_RICE:
        return DecodeValueRiceUnsigned(nParam1);
    case DECODE_VALUE_METHOD_X_BITS:
        return DecodeValueXBits(nParam1);
    }

    return 0;
}

////////////////////////////////////////////////////////////////////////////////////
// Generates an array from the m_nCurrentBitIndexarray
////////////////////////////////////////////////////////////////////////////////////
void CUnBitArrayOld::GenerateArrayOld(int* Output_Array, uint32 Number_of_Elements, int Minimum_nCurrentBitIndex_Array_Bytes) {

    //variable declarations
    uint32 K_Sum;
    uint32 q;
    uint32 kmin, kmax;
    uint32 k;
    uint32 Max;
    int *p1, *p2;

    // fill bit array if necessary
    // could use seek information to determine what the max was...
    uint32 Max_Bits_Needed = Number_of_Elements * 50;

    if (Minimum_nCurrentBitIndex_Array_Bytes > 0) 
    {
        // this is actually probably double what is really needed
        // we can only calculate the space needed for both arrays in multichannel
        Max_Bits_Needed = ((Minimum_nCurrentBitIndex_Array_Bytes + 4) * 8);
    }
    
    if (Max_Bits_Needed > GetBitsRemaining())
        FillBitArray();

    // decode the first 5 elements (all k = 10)
    Max = (Number_of_Elements < 5) ? Number_of_Elements : 5;
    for (q = 0; q < Max; q++) 
    {
        Output_Array[q] = DecodeValueRiceUnsigned(10);
    }
    
    // quit if that was all
    if (Number_of_Elements <= 5) 
    { 
        for (p2 = &Output_Array[0]; p2 < &Output_Array[Number_of_Elements]; p2++)
            *p2 = (*p2 & 1) ? (*p2 >> 1) + 1 : -(*p2 >> 1);
        return; 
    }

    // update k and K_Sum
    K_Sum = Output_Array[0] + Output_Array[1] + Output_Array[2] + Output_Array[3] + Output_Array[4];
    k = Get_K(K_Sum / 10);

    // work through the rest of the elements before the primary loop
    Max = (Number_of_Elements < 64) ? Number_of_Elements : 64;
    for (q = 5; q < Max; q++) 
    {
        Output_Array[q] = DecodeValueRiceUnsigned(k);
        K_Sum += Output_Array[q];
        k = Get_K(K_Sum / (q  + 1) / 2);
    }

    // quit if that was all
    if (Number_of_Elements <= 64) 
    { 
        for (p2 = &Output_Array[0]; p2 < &Output_Array[Number_of_Elements]; p2++)
            *p2 = (*p2 & 1) ? (*p2 >> 1) + 1 : -(*p2 >> 1);
        return; 
    }
        
    // set all of the variables up for the primary loop
    uint32 v, Bit_Array_Index;
    k = Get_K(K_Sum >> 7);
    kmin = K_SUM_MIN_BOUNDARY_OLD[k];
    kmax = K_SUM_MAX_BOUNDARY_OLD[k];
    p1 = &Output_Array[64]; p2 = &Output_Array[0];

    // the primary loop
    for (p1 = &Output_Array[64], p2 = &Output_Array[0]; p1 < &Output_Array[Number_of_Elements]; p1++, p2++) 
    {
        // plug through the string of 0's (the overflow)
        uint32 Bit_Initial = m_nCurrentBitIndex;
        while (!(m_pBitArray[m_nCurrentBitIndex >> 5] & Powers_of_Two_Reversed[m_nCurrentBitIndex++ & 31])) {}
    
        // if k = 0, your done
        if (k == 0) 
        {
            v = (m_nCurrentBitIndex - Bit_Initial - 1);
        }
        else 
        {
            // put the overflow value into v
            v = (m_nCurrentBitIndex - Bit_Initial - 1) << k;
    
            // store the bit information and incement the bit pointer by 'k'
            Bit_Array_Index = m_nCurrentBitIndex >> 5;
            unsigned int Bit_Index = m_nCurrentBitIndex & 31;
            m_nCurrentBitIndex += k;

            // figure the extra bits on the left and the left value
            int Left_Extra_Bits = (32 - k) - Bit_Index;
            unsigned int Left_Value = m_pBitArray[Bit_Array_Index] & Powers_of_Two_Minus_One_Reversed[Bit_Index];
            
            if (Left_Extra_Bits >= 0) 
                v |= (Left_Value >> Left_Extra_Bits);
            else 
                v |= (Left_Value << -Left_Extra_Bits) | (m_pBitArray[Bit_Array_Index + 1] >> (32 + Left_Extra_Bits));
        }    

        *p1 = v;
        K_Sum += *p1 - *p2;

        // convert *p2 to unsigned
        *p2 = (*p2 % 2) ? (*p2 >> 1) + 1 : -(*p2 >> 1);

        // adjust k if necessary
        if ((K_Sum < kmin) || (K_Sum >= kmax)) 
        {
            if (K_Sum < kmin) 
                while (K_Sum < K_SUM_MIN_BOUNDARY_OLD[--k]) {}
            else
                while (K_Sum >= K_SUM_MAX_BOUNDARY_OLD[++k]) {}

            kmax = K_SUM_MAX_BOUNDARY_OLD[k];
            kmin = K_SUM_MIN_BOUNDARY_OLD[k];
        }
    }

    for (; p2 < &Output_Array[Number_of_Elements]; p2++)
        *p2 = (*p2 & 1) ? (*p2 >> 1) + 1 : -(*p2 >> 1);
}

void CUnBitArrayOld::GenerateArray(int *pOutputArray, int nElements, int nBytesRequired) 
{
    if (m_nVersion < 3860)
    {
        GenerateArrayOld(pOutputArray, nElements, nBytesRequired);
    }
    else if (m_nVersion <= 3890)
    {
        GenerateArrayRice(pOutputArray, nElements, nBytesRequired);
    }
    else
    {    
        // error
    }
}

void CUnBitArrayOld::GenerateArrayRice(int* Output_Array, uint32 Number_of_Elements, int Minimum_nCurrentBitIndex_Array_Bytes) 
{
    /////////////////////////////////////////////////////////////////////////////
    // decode the bit array
    /////////////////////////////////////////////////////////////////////////////
    
    k = 10;
    K_Sum = 1024 * 16;

    if (m_nVersion <= 3880)
    {
        // the primary loop
        for (int *p1 = &Output_Array[0]; p1 < &Output_Array[Number_of_Elements]; p1++) 
        {
            *p1 = DecodeValueNew(FALSE);
        }
    }
    else
    {
        // the primary loop
        for (int *p1 = &Output_Array[0]; p1 < &Output_Array[Number_of_Elements]; p1++) 
        {
            *p1 = DecodeValueNew(TRUE);
        }
    }
}

__inline int CUnBitArrayOld::DecodeValueNew(BOOL bCapOverflow)
{
    // make sure there is room for the data
    // this is a little slower than ensuring a huge block to start with, but it's safer
    if (m_nCurrentBitIndex > m_nRefillBitThreshold)
    {
        FillBitArray();
    }
    
    unsigned int v;
    
    // plug through the string of 0's (the overflow)
    uint32 Bit_Initial = m_nCurrentBitIndex;
    while (!(m_pBitArray[m_nCurrentBitIndex >> 5] & Powers_of_Two_Reversed[m_nCurrentBitIndex++ & 31])) {}
    
    int nOverflow = (m_nCurrentBitIndex - Bit_Initial - 1);
    
    if (bCapOverflow)
    {
        while (nOverflow >= 16)
        {
            k += 4;
            nOverflow -= 16;
        }
    }
    
    // if k = 0, your done
    if (k != 0)
    {
        // put the overflow value into v
        v = nOverflow << k;
        
        // store the bit information and incement the bit pointer by 'k'
        unsigned int Bit_Array_Index = m_nCurrentBitIndex >> 5;
        unsigned int Bit_Index = m_nCurrentBitIndex & 31;
        m_nCurrentBitIndex += k;
        
        // figure the extra bits on the left and the left value
        int Left_Extra_Bits = (32 - k) - Bit_Index;
        unsigned int Left_Value = m_pBitArray[Bit_Array_Index] & Powers_of_Two_Minus_One_Reversed[Bit_Index];
        
        if (Left_Extra_Bits >= 0) 
        {
            v |= (Left_Value >> Left_Extra_Bits);
        }
        else 
        {
            v |= (Left_Value << -Left_Extra_Bits) | (m_pBitArray[Bit_Array_Index + 1] >> (32 + Left_Extra_Bits));
        }
    }    
    else
    {
        v = nOverflow;
    }
    
    // update K_Sum
    K_Sum += v - ((K_Sum + 8) >> 4);
    
    // update k
    if (K_Sum < K_SUM_MIN_BOUNDARY[k]) 
        k--;
    else if (K_Sum >= K_SUM_MAX_BOUNDARY[k]) 
        k++;
    
    // convert to unsigned and save
    return (v & 1) ? (v >> 1) + 1 : -(int(v >> 1));
}

#endif // #ifdef BACKWARDS_COMPATIBILITY
