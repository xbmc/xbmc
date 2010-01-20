#ifndef APE_UNBITARRAY_OLD_H
#define APE_UNBITARRAY_OLD_H

#include "All.h"
#ifdef BACKWARDS_COMPATIBILITY

#include "../UnBitArrayBase.h"

class IAPEDecompress;

// decodes 0000 up to and including 3890
class CUnBitArrayOld : public CUnBitArrayBase
{
public:

    // construction/destruction
    CUnBitArrayOld(IAPEDecompress *pAPEDecompress, int nVersion);
    ~CUnBitArrayOld();
    
    // functions
    void GenerateArray(int *pOutputArray, int nElements, int nBytesRequired = -1);
    unsigned int DecodeValue(DECODE_VALUE_METHOD DecodeMethod, int nParam1 = 0, int nParam2 = 0);
    
private:
    
    void GenerateArrayOld(int* pOutputArray, uint32 NumberOfElements, int MinimumBitArrayBytes);
    void GenerateArrayRice(int* pOutputArray, uint32 NumberOfElements, int MinimumBitArrayBytes);
    
    uint32 DecodeValueRiceUnsigned(uint32 k);
    
    // data 
    uint32 k;
    uint32 K_Sum;
    uint32 m_nRefillBitThreshold;
    
    // functions
    __inline int DecodeValueNew(BOOL bCapOverflow);
    uint32 GetBitsRemaining();
    __inline uint32 Get_K(uint32 x);
};

#endif

#endif    // #ifndef APE_UNBITARRAY_OLD_H


