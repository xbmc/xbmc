#include "All.h"
#ifdef BACKWARDS_COMPATIBILITY

#include "../MACLib.h"
#include "Anti-Predictor.h"

CAntiPredictor * CreateAntiPredictor(int nCompressionLevel, int nVersion)
{
    CAntiPredictor *pAntiPredictor = NULL;

    switch (nCompressionLevel) 
    {
#ifdef ENABLE_COMPRESSION_MODE_FAST
        case COMPRESSION_LEVEL_FAST:
            if (nVersion < 3320)
            {
                pAntiPredictor = (CAntiPredictor *) new CAntiPredictorFast0000To3320;
            }
            else
            {
                pAntiPredictor = (CAntiPredictor *) new CAntiPredictorFast3320ToCurrent;
            }
            break;
#endif //ENABLE_COMPRESSION_MODE_FAST
            
#ifdef ENABLE_COMPRESSION_MODE_NORMAL
        
        case COMPRESSION_LEVEL_NORMAL:
            if (nVersion < 3320)
            {
                pAntiPredictor = (CAntiPredictor *) new CAntiPredictorNormal0000To3320;
            }                
            else if (nVersion < 3800)
            {
                pAntiPredictor = (CAntiPredictor *) new CAntiPredictorNormal3320To3800;
            }
            else
            {
                pAntiPredictor = (CAntiPredictor *) new CAntiPredictorNormal3800ToCurrent;
            }
            break;

#endif //ENABLE_COMPRESSION_MODE_NORMAL

#ifdef ENABLE_COMPRESSION_MODE_HIGH
        case COMPRESSION_LEVEL_HIGH:
            if (nVersion < 3320)
            {
                pAntiPredictor = (CAntiPredictor *) new CAntiPredictorHigh0000To3320;
            }
            else if (nVersion < 3600)
            {
                pAntiPredictor = (CAntiPredictor *) new CAntiPredictorHigh3320To3600;
            }
            else if (nVersion < 3700)
            {
                pAntiPredictor = (CAntiPredictor *) new CAntiPredictorHigh3600To3700;
            }
            else if (nVersion < 3800)
            {
                pAntiPredictor = (CAntiPredictor *) new CAntiPredictorHigh3700To3800;
            }
            else
            {
                pAntiPredictor = (CAntiPredictor *) new CAntiPredictorHigh3800ToCurrent;
            }
            break;
#endif //ENABLE_COMPRESSION_MODE_HIGH

#ifdef ENABLE_COMPRESSION_MODE_EXTRA_HIGH
        case COMPRESSION_LEVEL_EXTRA_HIGH:
            if (nVersion < 3320) 
            {
                pAntiPredictor = (CAntiPredictor *) new CAntiPredictorExtraHigh0000To3320;
            }
            else if (nVersion < 3600) 
            {
                pAntiPredictor = (CAntiPredictor *) new CAntiPredictorExtraHigh3320To3600;
            }
            else if (nVersion < 3700) 
            {
                pAntiPredictor = (CAntiPredictor *) new CAntiPredictorExtraHigh3600To3700;
            }
            else if (nVersion < 3800) 
            {
                pAntiPredictor = (CAntiPredictor *) new CAntiPredictorExtraHigh3700To3800;
            }    
            else
            {
                pAntiPredictor = (CAntiPredictor *) new CAntiPredictorExtraHigh3800ToCurrent;
            }
            break;
#endif //ENABLE_COMPRESSION_MODE_EXTRA_HIGH
    }

    return pAntiPredictor;
}



CAntiPredictor::CAntiPredictor() 
{
}

CAntiPredictor::~CAntiPredictor() 
{
}

void CAntiPredictor::AntiPredict(int *pInputArray, int *pOutputArray, int NumberOfElements) 
{
    return;
}

void CAntiPredictorOffset::AntiPredict(int *pInputArray, int *pOutputArray, int NumberOfElements, int Offset, int DeltaM) 
{

    memcpy(pOutputArray, pInputArray, Offset * 4);

    int *ip = &pInputArray[Offset];
    int *ipo = &pOutputArray[0];
    int *op = &pOutputArray[Offset];
    int m = 0;
    
    for (; op < &pOutputArray[NumberOfElements]; ip++, ipo++, op++) 
    {
        *op = *ip + ((*ipo * m) >> 12);

        (*ipo ^ *ip) > 0 ? m += DeltaM : m -= DeltaM;
    }
}

#ifdef ENABLE_COMPRESSION_MODE_EXTRA_HIGH

int CAntiPredictorExtraHighHelper::ConventionalDotProduct(short *bip, short *bbm, short *pIPAdaptFactor, int op, int nNumberOfIterations) 
{
    // dot product
    int nDotProduct = 0;
    short *pMaxBBM = &bbm[nNumberOfIterations];
    
    if (op == 0)
    {
        while(bbm < pMaxBBM)
        {
            EXPAND_32_TIMES(nDotProduct += *bip++ * *bbm++;)
        }
    }
    else if (op > 0) 
    {
        while(bbm < pMaxBBM)
        {
            EXPAND_32_TIMES(nDotProduct += *bip++ * *bbm; *bbm++ += *pIPAdaptFactor++;)
        }
    }
    else
    {
        while(bbm < pMaxBBM)
        {
            EXPAND_32_TIMES(nDotProduct += *bip++ * *bbm; *bbm++ -= *pIPAdaptFactor++;)
        }
    }
    
    // use the dot product
    return nDotProduct;
}

#ifdef ENABLE_ASSEMBLY

#define MMX_DOT_PRODUCT_3800_TO_CURRENT_PROCESS_CHUNK_HEAD        \
    __asm movq mm0, [esi]                                        \
    __asm add esi, 8                                            \
    __asm movq mm1, [esi]                                        \
    __asm add esi, 8                                            \
    __asm movq mm2, [esi]                                        \
    __asm add esi, 8                                            \
                                                                \
    __asm movq mm3, [edi]                                        \
    __asm add edi, 8                                            \
    __asm movq mm4, [edi]                                        \
    __asm add edi, 8                                            \
    __asm movq mm5, [edi]                                        \
    __asm sub edi, 16                                            \
                                                                \
    __asm pmaddwd mm0, mm3                                        \
    __asm pmaddwd mm1, mm4                                        \
    __asm pmaddwd mm2, mm5                                        \
                                                                \
    __asm paddd mm7, mm0                                        \
    __asm paddd mm7, mm1                                        \
    __asm paddd mm7, mm2                                        \


#define MMX_DOT_PRODUCT_3800_TO_CURRENT_PROCESS_CHUNK_ADD        \
    MMX_DOT_PRODUCT_3800_TO_CURRENT_PROCESS_CHUNK_HEAD            \
                                                                \
    __asm paddw mm3, DWORD PTR [eax]                            \
    __asm movq [edi], mm3                                        \
    __asm add eax, 8                                            \
    __asm add edi, 8                                            \
    __asm paddw mm4, DWORD PTR [eax]                            \
    __asm movq [edi], mm4                                        \
    __asm add eax, 8                                            \
    __asm add edi, 8                                            \
    __asm paddw mm5, DWORD PTR [eax]                            \
    __asm movq [edi], mm5                                        \
    __asm add eax, 8                                            \
    __asm add edi, 8

#define MMX_DOT_PRODUCT_3800_TO_CURRENT_PROCESS_CHUNK_SUBTRACT    \
    MMX_DOT_PRODUCT_3800_TO_CURRENT_PROCESS_CHUNK_HEAD            \
                                                                \
    __asm psubw mm3, DWORD PTR [eax]                            \
    __asm movq [edi], mm3                                        \
    __asm add eax, 8                                            \
    __asm add edi, 8                                            \
    __asm psubw mm4, DWORD PTR [eax]                            \
    __asm movq [edi], mm4                                        \
    __asm add eax, 8                                            \
    __asm add edi, 8                                            \
    __asm psubw mm5, DWORD PTR [eax]                            \
    __asm movq [edi], mm5                                        \
    __asm add eax, 8                                            \
    __asm add edi, 8

int CAntiPredictorExtraHighHelper::MMXDotProduct(short *bip, short *bbm, short *pIPAdaptFactor, int op, int nNumberOfIterations) 
{
    int nDotProduct;
    nNumberOfIterations = (nNumberOfIterations / 128);

    if (op > 0) 
    {
        __asm 
        {
            push eax

            mov eax, DWORD PTR [pIPAdaptFactor]

            push esi
            push edi

            mov esi, DWORD PTR bip[0]
            mov edi, DWORD PTR bbm[0]

            pxor mm7, mm7

LBL_ADD_AGAIN:

            /////////////////////////////////////////////////////////
            // process 8 mm registers full
            /////////////////////////////////////////////////////////

            MMX_DOT_PRODUCT_3800_TO_CURRENT_PROCESS_CHUNK_ADD
            MMX_DOT_PRODUCT_3800_TO_CURRENT_PROCESS_CHUNK_ADD
            MMX_DOT_PRODUCT_3800_TO_CURRENT_PROCESS_CHUNK_ADD    
            MMX_DOT_PRODUCT_3800_TO_CURRENT_PROCESS_CHUNK_ADD
            MMX_DOT_PRODUCT_3800_TO_CURRENT_PROCESS_CHUNK_ADD
            MMX_DOT_PRODUCT_3800_TO_CURRENT_PROCESS_CHUNK_ADD
            MMX_DOT_PRODUCT_3800_TO_CURRENT_PROCESS_CHUNK_ADD
            MMX_DOT_PRODUCT_3800_TO_CURRENT_PROCESS_CHUNK_ADD
            MMX_DOT_PRODUCT_3800_TO_CURRENT_PROCESS_CHUNK_ADD
            MMX_DOT_PRODUCT_3800_TO_CURRENT_PROCESS_CHUNK_ADD

            // fill the registers
            movq mm0, [esi]
            add esi, 8
            movq mm1, [esi]
            add esi, 8
            
            movq mm3, [edi]
            add edi, 8
            movq mm4, [edi]
            sub edi, 8

            pmaddwd mm0, mm3
            pmaddwd mm1, mm4

            paddd mm7, mm0
            paddd mm7, mm1

            paddw mm3, DWORD PTR [eax]
            movq [edi], mm3
            add eax, 8
            add edi, 8
            paddw mm4, DWORD PTR [eax]
            movq [edi], mm4
            add eax, 8
            add edi, 8

            sub nNumberOfIterations, 1
            cmp nNumberOfIterations, 0
            jg LBL_ADD_AGAIN

            ///////////////////////////////////////////////////////////////
            // clean-up
            ///////////////////////////////////////////////////////////////
            // mm7 has the final dot-product (split into two dwords)
            movq mm6, mm7
            psrlq mm7, 32
            paddd mm6, mm7
            movd nDotProduct, mm6
            
            pop edi
            pop esi
            pop eax
            emms

        }
    }
    else 
    {
        __asm 
        {
            push eax

            mov eax, DWORD PTR [pIPAdaptFactor]

            push esi
            push edi

            mov esi, DWORD PTR bip[0]
            mov edi, DWORD PTR bbm[0]

            pxor mm7, mm7

LBL_SUBTRACT_AGAIN:
            
            /////////////////////////////////////////////////////////
            // process 8 mm registers full
            /////////////////////////////////////////////////////////
            MMX_DOT_PRODUCT_3800_TO_CURRENT_PROCESS_CHUNK_SUBTRACT
            MMX_DOT_PRODUCT_3800_TO_CURRENT_PROCESS_CHUNK_SUBTRACT
            MMX_DOT_PRODUCT_3800_TO_CURRENT_PROCESS_CHUNK_SUBTRACT
            MMX_DOT_PRODUCT_3800_TO_CURRENT_PROCESS_CHUNK_SUBTRACT
            MMX_DOT_PRODUCT_3800_TO_CURRENT_PROCESS_CHUNK_SUBTRACT
            MMX_DOT_PRODUCT_3800_TO_CURRENT_PROCESS_CHUNK_SUBTRACT
            MMX_DOT_PRODUCT_3800_TO_CURRENT_PROCESS_CHUNK_SUBTRACT
            MMX_DOT_PRODUCT_3800_TO_CURRENT_PROCESS_CHUNK_SUBTRACT
            MMX_DOT_PRODUCT_3800_TO_CURRENT_PROCESS_CHUNK_SUBTRACT
            MMX_DOT_PRODUCT_3800_TO_CURRENT_PROCESS_CHUNK_SUBTRACT

            // fill the registers
            movq mm0, [esi]
            add esi, 8
            movq mm1, [esi]
            add esi, 8
            
            movq mm3, [edi]
            add edi, 8
            movq mm4, [edi]
            sub edi, 8

            pmaddwd mm0, mm3
            pmaddwd mm1, mm4

            paddd mm7, mm0
            paddd mm7, mm1

            psubw mm3, DWORD PTR [eax]
            movq [edi], mm3
            add eax, 8
            add edi, 8
            psubw mm4, DWORD PTR [eax]
            movq [edi], mm4
            add eax, 8
            add edi, 8

            sub nNumberOfIterations, 1
            cmp nNumberOfIterations, 0
            jg LBL_SUBTRACT_AGAIN

            ///////////////////////////////////////////////////////////////
            // clean-up
            ///////////////////////////////////////////////////////////////
            // mm7 has the final dot-product (split into two dwords)
            movq mm6, mm7
            psrlq mm7, 32
            paddd mm6, mm7
            movd nDotProduct, mm6
            
            pop edi
            pop esi
            pop eax
            emms

        }
    }
    
    return nDotProduct;
}

#endif // #ifdef ENABLE_ASSEMBLY

#endif // #ifdef ENABLE_COMPRESSION_MODE_EXTRA_HIGH

#endif // #ifdef BACKWARDS_COMPATIBILITY

