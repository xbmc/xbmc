#include "All.h"
#ifdef BACKWARDS_COMPATIBILITY

#include "Anti-Predictor.h"

#ifdef ENABLE_COMPRESSION_MODE_HIGH

void CAntiPredictorHigh0000To3320::AntiPredict(int *pInputArray, int *pOutputArray, int NumberOfElements) 
{
    // variable declares
    int p, pw;
    int q;
    int m;

    // short frame handling
    if (NumberOfElements < 32) 
    {
        memcpy(pOutputArray, pInputArray, NumberOfElements * 4);
        return;
    }
    
    ////////////////////////////////////////
    // order 5
    ////////////////////////////////////////
    memcpy(pOutputArray, pInputArray, 32);
    
    // initialize values
    m = 0;
  
    for (q = 8; q < NumberOfElements; q++) 
    {
        p = (5 * pOutputArray[q - 1]) - (10 * pOutputArray[q - 2]) + (12 * pOutputArray[q - 3]) - (7 * pOutputArray[q - 4]) + pOutputArray[q - 5];
        
        pw = (p * m) >> 12;
        
        pOutputArray[q] = pInputArray[q] + pw;
        
        // adjust m
        if (pInputArray[q] > 0)
            (p > 0) ? m += 1 : m -= 1;
        else if (pInputArray[q] < 0)
            (p > 0) ? m -= 1 : m += 1;
        
    }

    ///////////////////////////////////////
    // order 4
    ///////////////////////////////////////
    memcpy(pInputArray, pOutputArray, 32);
    m = 0;

    for (q = 8; q < NumberOfElements; q++) 
    {
        p = (4 * pInputArray[q - 1]) - (6 * pInputArray[q - 2]) + (4 * pInputArray[q - 3]) - pInputArray[q - 4];
        pw = (p * m) >> 12;
        
        pInputArray[q] = pOutputArray[q] + pw;
        
        // adjust m
        if (pOutputArray[q] > 0)
            (p > 0) ? m += 2 : m -= 2;
        else if (pOutputArray[q] < 0)
            (p > 0) ? m -= 2 : m += 2;
    }
   
    CAntiPredictorNormal0000To3320 AntiPredictor;
    AntiPredictor.AntiPredict(pInputArray, pOutputArray, NumberOfElements);
}

void CAntiPredictorHigh3320To3600::AntiPredict(int *pInputArray, int *pOutputArray, int NumberOfElements) 
{
    // short frame handling
    if (NumberOfElements < 8) 
    {
        memcpy(pOutputArray, pInputArray, NumberOfElements * 4);
        return;
    }

    // do the offset anti-prediction
    CAntiPredictorOffset AntiPredictorOffset;
    AntiPredictorOffset.AntiPredict(pInputArray, pOutputArray, NumberOfElements, 2, 12);
    AntiPredictorOffset.AntiPredict(pOutputArray, pInputArray, NumberOfElements, 3, 12);
    
    AntiPredictorOffset.AntiPredict(pInputArray, pOutputArray, NumberOfElements, 4, 12);
    AntiPredictorOffset.AntiPredict(pOutputArray, pInputArray, NumberOfElements, 5, 12);
    
    AntiPredictorOffset.AntiPredict(pInputArray, pOutputArray, NumberOfElements, 6, 12);
    AntiPredictorOffset.AntiPredict(pOutputArray, pInputArray, NumberOfElements, 7, 12);
    
    // use the normal mode
    CAntiPredictorNormal3320To3800 AntiPredictor;
    AntiPredictor.AntiPredict(pInputArray, pOutputArray, NumberOfElements);
}

void CAntiPredictorHigh3600To3700::AntiPredict(int *pInputArray, int *pOutputArray, int NumberOfElements) 
{
    // variable declares
    int q;

    // short frame handling
    if (NumberOfElements < 16) 
    {
        memcpy(pOutputArray, pInputArray, NumberOfElements * 4);
        return;
    }

    // make the first five samples identical in both arrays
    memcpy(pOutputArray, pInputArray, 13 * 4);
    
    // initialize values
    int bm1 = 0;
    int bm2 = 0;
    int bm3 = 0;
    int bm4 = 0;
    int bm5 = 0;
    int bm6 = 0;
    int bm7 = 0;
    int bm8 = 0;
    int bm9 = 0;
    int bm10 = 0;
    int bm11 = 0;
    int bm12 = 0;
    int bm13 = 0;

    int m2 = 64;

    int m3 = 28;
    int m4 = 16;
    int OP0;
    int OP1 = pOutputArray[12];
    int p4 = pInputArray[12];
    int p3 = (pInputArray[12] - pInputArray[11]) << 1;
    int p2 = pInputArray[12] + ((pInputArray[10] - pInputArray[11]) << 3);// - pInputArray[3] + pInputArray[2];
    int bp1 = pOutputArray[12];
    int bp2 = pOutputArray[11];
    int bp3 = pOutputArray[10];
    int bp4 = pOutputArray[9];
    int bp5 = pOutputArray[8];
    int bp6 = pOutputArray[7];
    int bp7 = pOutputArray[6];
    int bp8 = pOutputArray[5];
    int bp9 = pOutputArray[4];
    int bp10 = pOutputArray[3];
    int bp11 = pOutputArray[2];
    int bp12 = pOutputArray[1];
    int bp13 = pOutputArray[0];

    for (q = 13; q < NumberOfElements; q++) 
    {
        pInputArray[q] = pInputArray[q] - 1;
        OP0 = (pInputArray[q] - ((bp1 * bm1) >> 8) + ((bp2 * bm2) >> 8) - ((bp3 * bm3) >> 8) - ((bp4 * bm4) >> 8) - ((bp5 * bm5) >> 8) - ((bp6 * bm6) >> 8) - ((bp7 * bm7) >> 8) - ((bp8 * bm8) >> 8) - ((bp9 * bm9) >> 8) + ((bp10 * bm10) >> 8) + ((bp11 * bm11) >> 8) + ((bp12 * bm12) >> 8) + ((bp13 * bm13) >> 8));
        
        if (pInputArray[q] > 0) 
        {
            bm1 -= bp1 > 0 ? 1 : -1;
            bm2 += bp2 >= 0 ? 1 : -1;
            bm3 -= bp3 > 0 ? 1 : -1;
            bm4 -= bp4 >= 0 ? 1 : -1;
            bm5 -= bp5 > 0 ? 1 : -1;
            bm6 -= bp6 >= 0 ? 1 : -1;
            bm7 -= bp7 > 0 ? 1 : -1;
            bm8 -= bp8 >= 0 ? 1 : -1;
            bm9 -= bp9 > 0 ? 1 : -1;
            bm10 += bp10 >= 0 ? 1 : -1;
            bm11 += bp11 > 0 ? 1 : -1;
            bm12 += bp12 >= 0 ? 1 : -1;
            bm13 += bp13 > 0 ? 1 : -1;
        }
        else if (pInputArray[q] < 0) 
        {
            bm1 -= bp1 <= 0 ? 1 : -1;
            bm2 += bp2 < 0 ? 1 : -1;
            bm3 -= bp3 <= 0 ? 1 : -1;
            bm4 -= bp4 < 0 ? 1 : -1;
            bm5 -= bp5 <= 0 ? 1 : -1;
            bm6 -= bp6 < 0 ? 1 : -1;
            bm7 -= bp7 <= 0 ? 1 : -1;
            bm8 -= bp8 < 0 ? 1 : -1;
            bm9 -= bp9 <= 0 ? 1 : -1;
            bm10 += bp10 < 0 ? 1 : -1;
            bm11 += bp11 <= 0 ? 1 : -1;
            bm12 += bp12 < 0 ? 1 : -1;
            bm13 += bp13 <= 0 ? 1 : -1;
        }

        bp13 = bp12;
        bp12 = bp11;
        bp11 = bp10;
        bp10 = bp9;
        bp9 = bp8;
        bp8 = bp7;
        bp7 = bp6;
        bp6 = bp5;
        bp5 = bp4;
        bp4 = bp3;
        bp3 = bp2;
        bp2 = bp1;
        bp1 = OP0;

        pInputArray[q] = OP0 + ((p2 * m2) >> 11) + ((p3 * m3) >> 9) + ((p4 * m4) >> 9);

        if (OP0 > 0) 
        {
            m2 -= p2 > 0 ? -1 : 1;
            m3 -= p3 > 0 ? -1 : 1;
            m4 -= p4 > 0 ? -1 : 1;
        }
        else if (OP0 < 0) 
        {
            m2 -= p2 > 0 ? 1 : -1;
            m3 -= p3 > 0 ? 1 : -1;
            m4 -= p4 > 0 ? 1 : -1;
        }

        p2 = pInputArray[q] + ((pInputArray[q - 2] - pInputArray[q - 1]) << 3);
        p3 = (pInputArray[q] - pInputArray[q - 1]) << 1;
        p4 = pInputArray[q];
        pOutputArray[q] = pInputArray[q];// + ((p3 * m3) >> 9);
    }
    
    m4 = 370;
    int m5 = 3900;

    pOutputArray[1] = pInputArray[1] + pOutputArray[0];
    pOutputArray[2] = pInputArray[2] + pOutputArray[1];
    pOutputArray[3] = pInputArray[3] + pOutputArray[2];
    pOutputArray[4] = pInputArray[4] + pOutputArray[3];
    pOutputArray[5] = pInputArray[5] + pOutputArray[4];
    pOutputArray[6] = pInputArray[6] + pOutputArray[5];
    pOutputArray[7] = pInputArray[7] + pOutputArray[6];
    pOutputArray[8] = pInputArray[8] + pOutputArray[7];
    pOutputArray[9] = pInputArray[9] + pOutputArray[8];
    pOutputArray[10] = pInputArray[10] + pOutputArray[9];
    pOutputArray[11] = pInputArray[11] + pOutputArray[10];
    pOutputArray[12] = pInputArray[12] + pOutputArray[11];

    p4 = (2 * pInputArray[12]) - pInputArray[11];
    int p6 = 0;
    int p5 = pOutputArray[12];
    int IP0, IP1;
    int m6 = 0;

    IP1 = pInputArray[12];
    for (q = 13; q < NumberOfElements; q++) 
    {
        IP0 = pOutputArray[q] + ((p4 * m4) >> 9) - ((p6 * m6) >> 10);
        (pOutputArray[q] ^ p4) >= 0 ? m4++ : m4--;
        (pOutputArray[q] ^ p6) >= 0 ? m6-- : m6++;
        p4 = (2 * IP0) - IP1;
        p6 = IP0;

        pOutputArray[q] = IP0 + ((p5 * 31) >> 5);
        p5 = pOutputArray[q];

        IP1 = IP0;
    }
}

void CAntiPredictorHigh3700To3800::AntiPredict(int *pInputArray, int *pOutputArray, int NumberOfElements) 
{
    // the frame to start prediction on
    #define FIRST_ELEMENT    16

    int x = 100;
    int y = -25;

    // short frame handling
    if (NumberOfElements < 20) 
    {
        memcpy(pOutputArray, pInputArray, NumberOfElements * 4);
        return;
    }

    // make the first five samples identical in both arrays
    memcpy(pOutputArray, pInputArray, FIRST_ELEMENT * 4);
    
    // variable declares and initializations
    int bm[FIRST_ELEMENT]; memset(bm, 0, FIRST_ELEMENT * 4);
    int m2 = 64, m3 = 115, m4 = 64, m5 = 740, m6 = 0;
    int p4 = pInputArray[FIRST_ELEMENT - 1];
    int p3 = (pInputArray[FIRST_ELEMENT - 1] - pInputArray[FIRST_ELEMENT - 2]) << 1;
    int p2 = pInputArray[FIRST_ELEMENT - 1] + ((pInputArray[FIRST_ELEMENT - 3] - pInputArray[FIRST_ELEMENT - 2]) << 3);// - pInputArray[3] + pInputArray[2];
    int *op = &pOutputArray[FIRST_ELEMENT];
    int *ip = &pInputArray[FIRST_ELEMENT];
    int IPP2 = ip[-2];
    int IPP1 = ip[-1];
    int p7 = 2 * ip[-1] - ip[-2];
    int opp = op[-1];
    int Original;
    
    // undo the initial prediction stuff
    for (int q = 1; q < FIRST_ELEMENT; q++) {
        pOutputArray[q] += pOutputArray[q - 1];
    }

    // pump the primary loop
    for (;op < &pOutputArray[NumberOfElements]; op++, ip++) {

        Original = *ip - 1;
        *ip = Original - (((ip[-1] * bm[0]) + (ip[-2] * bm[1]) + (ip[-3] * bm[2]) + (ip[-4] * bm[3]) + (ip[-5] * bm[4]) + (ip[-6] * bm[5]) + (ip[-7] * bm[6]) + (ip[-8] * bm[7]) + (ip[-9] * bm[8]) + (ip[-10] * bm[9]) + (ip[-11] * bm[10]) + (ip[-12] * bm[11]) + (ip[-13] * bm[12]) + (ip[-14] * bm[13]) + (ip[-15] * bm[14]) + (ip[-16] * bm[15])) >> 8);

        if (Original > 0) 
        {
            bm[0] -= ip[-1] > 0 ? 1 : -1;
            bm[1] += (((unsigned int)(ip[-2]) >> 30) & 2) - 1;
            bm[2] -= ip[-3] > 0 ? 1 : -1;
            bm[3] += (((unsigned int)(ip[-4]) >> 30) & 2) - 1;
            bm[4] -= ip[-5] > 0 ? 1 : -1;
            bm[5] += (((unsigned int)(ip[-6]) >> 30) & 2) - 1;
            bm[6] -= ip[-7] > 0 ? 1 : -1;
            bm[7] += (((unsigned int)(ip[-8]) >> 30) & 2) - 1;
            bm[8] -= ip[-9] > 0 ? 1 : -1;
            bm[9] += (((unsigned int)(ip[-10]) >> 30) & 2) - 1;
            bm[10] -= ip[-11] > 0 ? 1 : -1;
            bm[11] += (((unsigned int)(ip[-12]) >> 30) & 2) - 1;
            bm[12] -= ip[-13] > 0 ? 1 : -1;
            bm[13] += (((unsigned int)(ip[-14]) >> 30) & 2) - 1;
            bm[14] -= ip[-15] > 0 ? 1 : -1;
            bm[15] += (((unsigned int)(ip[-16]) >> 30) & 2) - 1;
        }
        else if (Original < 0) 
        {
            bm[0] -= ip[-1] <= 0 ? 1 : -1;
            bm[1] -= (((unsigned int)(ip[-2]) >> 30) & 2) - 1;
            bm[2] -= ip[-3] <= 0 ? 1 : -1;
            bm[3] -= (((unsigned int)(ip[-4]) >> 30) & 2) - 1;
            bm[4] -= ip[-5] <= 0 ? 1 : -1;
            bm[5] -= (((unsigned int)(ip[-6]) >> 30) & 2) - 1;
            bm[6] -= ip[-7] <= 0 ? 1 : -1;
            bm[7] -= (((unsigned int)(ip[-8]) >> 30) & 2) - 1;
            bm[8] -= ip[-9] <= 0 ? 1 : -1;
            bm[9] -= (((unsigned int)(ip[-10]) >> 30) & 2) - 1;
            bm[10] -= ip[-11] <= 0 ? 1 : -1;
            bm[11] -= (((unsigned int)(ip[-12]) >> 30) & 2) - 1;
            bm[12] -= ip[-13] <= 0 ? 1 : -1;
            bm[13] -= (((unsigned int)(ip[-14]) >> 30) & 2) - 1;
            bm[14] -= ip[-15] <= 0 ? 1 : -1;
            bm[15] -= (((unsigned int)(ip[-16]) >> 30) & 2) - 1;
        }

        /////////////////////////////////////////////
        *op = *ip + (((p2 * m2) + (p3 * m3) + (p4 * m4)) >> 11);

        if (*ip > 0) 
        {
            m2 -= p2 > 0 ? -1 : 1;
            m3 -= p3 > 0 ? -4 : 4;
            m4 -= p4 > 0 ? -4 : 4;
        }
        else if (*ip < 0)
        {
            m2 -= p2 > 0 ? 1 : -1;
            m3 -= p3 > 0 ? 4 : -4;
            m4 -= p4 > 0 ? 4 : -4;
        }

        p4 = *op;
        p2 = p4 + ((IPP2 - IPP1) << 3);
        p3 = (p4 - IPP1) << 1;

        IPP2 = IPP1;
        IPP1 = p4;

        /////////////////////////////////////////////
        *op += (((p7 * m5) - (opp * m6)) >> 10);
        
        (IPP1 ^ p7) >= 0 ? m5+=2 : m5-=2;
        (IPP1 ^ opp) >= 0 ? m6-- : m6++;
  
        p7 = 2 * *op - opp;
        opp = *op;

        /////////////////////////////////////////////
        *op += ((op[-1] * 31) >> 5);
    }
}

void CAntiPredictorHigh3800ToCurrent::AntiPredict(int *pInputArray, int *pOutputArray, int NumberOfElements) 
{
    // the frame to start prediction on
    #define FIRST_ELEMENT    16

    // short frame handling
    if (NumberOfElements < 20) 
    {
        memcpy(pOutputArray, pInputArray, NumberOfElements * 4);
        return;
    }

    // make the first five samples identical in both arrays
    memcpy(pOutputArray, pInputArray, FIRST_ELEMENT * 4);
    
    // variable declares and initializations
    int bm[FIRST_ELEMENT]; memset(bm, 0, FIRST_ELEMENT * 4);
    int m2 = 64, m3 = 115, m4 = 64, m5 = 740, m6 = 0;
    int p4 = pInputArray[FIRST_ELEMENT - 1];
    int p3 = (pInputArray[FIRST_ELEMENT - 1] - pInputArray[FIRST_ELEMENT - 2]) << 1;
    int p2 = pInputArray[FIRST_ELEMENT - 1] + ((pInputArray[FIRST_ELEMENT - 3] - pInputArray[FIRST_ELEMENT - 2]) << 3);// - pInputArray[3] + pInputArray[2];
    int *op = &pOutputArray[FIRST_ELEMENT];
    int *ip = &pInputArray[FIRST_ELEMENT];
    int IPP2 = ip[-2];
    int IPP1 = ip[-1];
    int p7 = 2 * ip[-1] - ip[-2];
    int opp = op[-1];
    
    // undo the initial prediction stuff
    for (int q = 1; q < FIRST_ELEMENT; q++) 
    {
        pOutputArray[q] += pOutputArray[q - 1];
    }

    // pump the primary loop
    for (;op < &pOutputArray[NumberOfElements]; op++, ip++) 
    {

        unsigned int *pip = (unsigned int *) &ip[-FIRST_ELEMENT];
        int *pbm = &bm[0];
        int nDotProduct = 0;
        
        if (*ip > 0) 
        {
            EXPAND_16_TIMES(nDotProduct += *pip * *pbm; *pbm++ += ((*pip++ >> 30) & 2) - 1;)
        }
        else if (*ip < 0) 
        {
            EXPAND_16_TIMES(nDotProduct += *pip * *pbm; *pbm++ -= ((*pip++ >> 30) & 2) - 1;)
        }
        else
        {
            EXPAND_16_TIMES(nDotProduct += *pip++ * *pbm++;)
        }

        *ip -= (nDotProduct >> 9);

        /////////////////////////////////////////////
        *op = *ip + (((p2 * m2) + (p3 * m3) + (p4 * m4)) >> 11);

        if (*ip > 0) 
        {
            m2 -= ((p2 >> 30) & 2) - 1;
            m3 -= ((p3 >> 28) & 8) - 4;
            m4 -= ((p4 >> 28) & 8) - 4;

        }
        else if (*ip < 0) 
        {
            m2 += ((p2 >> 30) & 2) - 1;
            m3 += ((p3 >> 28) & 8) - 4;
            m4 += ((p4 >> 28) & 8) - 4;
        }

        
        p2 = *op + ((IPP2 - p4) << 3);
        p3 = (*op - p4) << 1;
        IPP2 = p4;
        p4 = *op;
        
        /////////////////////////////////////////////
        *op += (((p7 * m5) - (opp * m6)) >> 10);

        if (p4 > 0) 
        {
            m5 -= ((p7 >> 29) & 4) - 2;
            m6 += ((opp >> 30) & 2) - 1;
        }
        else if (p4 < 0) 
        {
            m5 += ((p7 >> 29) & 4) - 2;
            m6 -= ((opp >> 30) & 2) - 1;
        }
  
        p7 = 2 * *op - opp;
        opp = *op;

        /////////////////////////////////////////////
        *op += ((op[-1] * 31) >> 5);
    }

    #undef FIRST_ELEMENT
}

#endif // #ifdef ENABLE_COMPRESSION_MODE_HIGH

#endif // #ifdef BACKWARDS_COMPATIBILITY
