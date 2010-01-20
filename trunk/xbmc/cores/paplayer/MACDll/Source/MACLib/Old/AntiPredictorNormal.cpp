#include "All.h"
#ifdef BACKWARDS_COMPATIBILITY

#include "Anti-Predictor.h"
#ifdef ENABLE_COMPRESSION_MODE_NORMAL

void CAntiPredictorNormal0000To3320::AntiPredict(int *pInputArray, int *pOutputArray, int NumberOfElements) 
{
    // variable declares
    int *ip, *op, *op1, *op2;
    int p, pw;
    int m;

    // short frame handling
    if (NumberOfElements < 32) 
    {
        memcpy(pOutputArray, pInputArray, NumberOfElements * 4);
        return;
    }

    ////////////////////////////////////////
    // order 3
    ////////////////////////////////////////
    memcpy(pOutputArray, pInputArray, 32);
    
    // initialize values
    m = 300;
    op = &pOutputArray[8];
    op1 = &pOutputArray[7];
    op2 = &pOutputArray[6];
    
    // make the first prediction
    p = (pOutputArray[7] * 3) - (pOutputArray[6] * 3) + pOutputArray[5];
    pw = (p * m) >> 12;
    
    // loop through the array
    for (ip = &pInputArray[8]; ip < &pInputArray[NumberOfElements]; ip++, op++, op1++, op2++) {

        // figure the output value
        *op = *ip + pw;
        
        // adjust m
        if (*ip > 0)
            m += (p > 0) ? 4 : -4;
        else if (*ip < 0)
            m += (p > 0) ? -4 : 4;

        // make the next prediction
        p = (*op * 3) - (*op1 * 3) + *op2;
        pw = (p * m) >> 12;
    }


    ///////////////////////////////////////
    // order 2
    ///////////////////////////////////////
    memcpy(pInputArray, pOutputArray, 32);
    m = 3000;

    op1 = &pInputArray[7];
    p = (*op1 * 2) - pInputArray[6];
    pw = (p * m) >> 12;
        
    for (op = &pInputArray[8], ip = &pOutputArray[8]; ip < &pOutputArray[NumberOfElements]; ip++, op++, op1++) 
    {
        *op = *ip + pw;
                
        // adjust m
        if (*ip > 0)
            m += (p > 0) ? 12 : -12;
        else if (*ip < 0)
            m += (p > 0) ? -12 : 12;

        p = (*op * 2) - *op1;
        pw = (p * m) >> 12;
        
    }

    ///////////////////////////////////////
    // order 1
    ///////////////////////////////////////
    pOutputArray[0] = pInputArray[0];
    pOutputArray[1] = pInputArray[1] + pOutputArray[0];
    pOutputArray[2] = pInputArray[2] + pOutputArray[1];
    pOutputArray[3] = pInputArray[3] + pOutputArray[2];
    pOutputArray[4] = pInputArray[4] + pOutputArray[3];
    pOutputArray[5] = pInputArray[5] + pOutputArray[4];
    pOutputArray[6] = pInputArray[6] + pOutputArray[5];
    pOutputArray[7] = pInputArray[7] + pOutputArray[6];

    m = 3900;
    
    p = pOutputArray[7];
    pw = (p * m) >> 12;
    
    for (op = &pOutputArray[8], ip = &pInputArray[8]; ip < &pInputArray[NumberOfElements]; ip++, op++) {
        *op = *ip + pw;
                
        // adjust m
        if (*ip > 0)
            m += (p > 0) ? 1 : -1;
        else if (*ip < 0)
            m += (p > 0) ? -1 : 1;

        p = *op;
        pw = (p * m) >> 12;
    }

}

void CAntiPredictorNormal3320To3800::AntiPredict(int *pInputArray, int *pOutputArray, int NumberOfElements) 
{
    // variable declares
    int q;

    // short frame handling
    if (NumberOfElements < 8) 
    {
        memcpy(pOutputArray, pInputArray, NumberOfElements * 4);
        return;
    }

    // make the first five samples identical in both arrays
    memcpy(pOutputArray, pInputArray, 20);
    
    // initialize values
    int m1 = 0;
    int m2 = 64;
    int m3 = 28;
    int OP0;
    int OP1 = pOutputArray[4];

    int p3 = (3 * (pOutputArray[4] - pOutputArray[3])) + pOutputArray[2];
    int p2 = pInputArray[4] + ((pInputArray[2] - pInputArray[3]) << 3) - pInputArray[1] + pInputArray[0];
    int p1 = pOutputArray[4];

    for (q = 5; q < NumberOfElements; q++) 
    {
        OP0 = pInputArray[q] + ((p1 * m1) >> 8);
        (pInputArray[q] ^ p1) > 0 ? m1++ : m1--;
        p1 = OP0;
                
        pInputArray[q] = OP0 + ((p2 * m2) >> 11);
        (OP0 ^ p2) > 0 ? m2++ : m2--;
        p2 = pInputArray[q] + ((pInputArray[q - 2] - pInputArray[q - 1]) << 3) - pInputArray[q - 3] + pInputArray[q - 4];
        
        pOutputArray[q] = pInputArray[q] + ((p3 * m3) >> 9);
        (pInputArray[q] ^ p3) > 0 ? m3++ : m3--;
        p3 = (3 * (pOutputArray[q] - pOutputArray[q - 1])) + pOutputArray[q - 2];
    }
    
    int m4 = 370;
    int m5 = 3900;

    pOutputArray[1] = pInputArray[1] + pOutputArray[0];
    pOutputArray[2] = pInputArray[2] + pOutputArray[1];
    pOutputArray[3] = pInputArray[3] + pOutputArray[2];
    pOutputArray[4] = pInputArray[4] + pOutputArray[3];

    int p4 = (2 * pInputArray[4]) - pInputArray[3];
    int p5 = pOutputArray[4];
    int IP0, IP1;

    IP1 = pInputArray[4];
    for (q = 5; q < NumberOfElements; q++) 
    {
        IP0 = pOutputArray[q] + ((p4 * m4) >> 9);
        (pOutputArray[q] ^ p4) > 0 ? m4++ : m4--;
        p4 = (2 * IP0) - IP1;

        pOutputArray[q] = IP0 + ((p5 * m5) >> 12);
        (IP0 ^ p5) > 0 ? m5++ : m5--;
        p5 = pOutputArray[q];

        IP1 = IP0;
    }
}

void CAntiPredictorNormal3800ToCurrent::AntiPredict(int *pInputArray, int *pOutputArray, int NumberOfElements) 
{
    // the frame to start prediction on
    #define FIRST_ELEMENT    4

    // short frame handling
    if (NumberOfElements < 8) 
    {
        memcpy(pOutputArray, pInputArray, NumberOfElements * 4);
        return;
    }

    // make the first five samples identical in both arrays
    memcpy(pOutputArray, pInputArray, FIRST_ELEMENT * 4);
    
    // variable declares and initializations
    int m2 = 64, m3 = 115, m4 = 64, m5 = 740, m6 = 0;
    int p4 = pInputArray[FIRST_ELEMENT - 1];
    int p3 = (pInputArray[FIRST_ELEMENT - 1] - pInputArray[FIRST_ELEMENT - 2]) << 1;
    int p2 = pInputArray[FIRST_ELEMENT - 1] + ((pInputArray[FIRST_ELEMENT - 3] - pInputArray[FIRST_ELEMENT - 2]) << 3);// - pInputArray[3] + pInputArray[2];
    int *op = &pOutputArray[FIRST_ELEMENT];
    int *ip = &pInputArray[FIRST_ELEMENT];
    int IPP2 = ip[-2];
    int p7 = 2 * ip[-1] - ip[-2];
    int opp = op[-1];
    
    // undo the initial prediction stuff
    for (int q = 1; q < FIRST_ELEMENT; q++) {
        pOutputArray[q] += pOutputArray[q - 1];
    }

    // pump the primary loop
    for (; op < &pOutputArray[NumberOfElements]; op++, ip++) {

        register int o = *op, i = *ip;
        
        /////////////////////////////////////////////
        o = i + (((p2 * m2) + (p3 * m3) + (p4 * m4)) >> 11);

        if (i > 0) 
        {
            m2 -= ((p2 >> 30) & 2) - 1;
            m3 -= ((p3 >> 28) & 8) - 4;
            m4 -= ((p4 >> 28) & 8) - 4;

        }
        else if (i < 0) 
        {
            m2 += ((p2 >> 30) & 2) - 1;
            m3 += ((p3 >> 28) & 8) - 4;
            m4 += ((p4 >> 28) & 8) - 4;
        }

        
        p2 = o + ((IPP2 - p4) << 3);
        p3 = (o - p4) << 1;
        IPP2 = p4;
        p4 = o;

        /////////////////////////////////////////////
        o += (((p7 * m5) - (opp * m6)) >> 10);

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

        p7 = 2 * o - opp;
        opp = o;

        /////////////////////////////////////////////
        *op = o + ((op[-1] * 31) >> 5);
    }
}

#endif // #ifdef ENABLE_COMPRESSION_MODE_NORMAL

#endif // #ifdef BACKWARDS_COMPATIBILITY
