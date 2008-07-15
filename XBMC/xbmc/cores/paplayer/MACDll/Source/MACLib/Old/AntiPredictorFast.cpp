#include "All.h"
#ifdef BACKWARDS_COMPATIBILITY

#include "Anti-Predictor.h"

#ifdef ENABLE_COMPRESSION_MODE_FAST

void CAntiPredictorFast0000To3320::AntiPredict(int *pInputArray, int *pOutputArray, int NumberOfElements) {

    //short frame handling
    if (NumberOfElements < 32) {
        memcpy(pOutputArray, pInputArray, NumberOfElements * 4);
        return;
    }

    //the initial
    pOutputArray[0] = pInputArray[0];
    pOutputArray[1] = pInputArray[1] + pOutputArray[0];
    pOutputArray[2] = pInputArray[2] + pOutputArray[1];
    pOutputArray[3] = pInputArray[3] + pOutputArray[2];
    pOutputArray[4] = pInputArray[4] + pOutputArray[3];
    pOutputArray[5] = pInputArray[5] + pOutputArray[4];
    pOutputArray[6] = pInputArray[6] + pOutputArray[5];
    pOutputArray[7] = pInputArray[7] + pOutputArray[6];

    //the rest
    int p, pw;
    int m = 4000;
    int *ip, *op, *op1;

    op1 = &pOutputArray[7];
    p = (*op1 * 2) - pOutputArray[6];
    pw = (p * m) >> 12;
        
    for (op = &pOutputArray[8], ip = &pInputArray[8]; ip < &pInputArray[NumberOfElements]; ip++, op++, op1++) {
        *op = *ip + pw;
                

        //adjust m
        if (*ip > 0)
            m += (p > 0) ? 4 : -4;
        else if (*ip < 0)
            m += (p > 0) ? -4 : 4;

        p = (*op * 2) - *op1;
        pw = (p * m) >> 12;
        
    }
}

///////note: no output - overwrites input/////////////////
void CAntiPredictorFast3320ToCurrent::AntiPredict(int *pInputArray, int *pOutputArray, int NumberOfElements) {

    //short frame handling
    if (NumberOfElements < 3) {
        return;
    }

    //variable declares
    int p;
    int m = 375;
    int *ip;
    int IP2 = pInputArray[1];
    int IP3 = pInputArray[0];
    int OP1 = pInputArray[1];

    //the decompression loop (order 2 followed by order 1)
    for (ip = &pInputArray[2]; ip < &pInputArray[NumberOfElements]; ip++) {
        
        //make a prediction for order 2
        p = IP2 + IP2 - IP3;
        
        //rollback the values
        IP3 = IP2;
        IP2 = *ip + ((p * m) >> 9);
        
        //adjust m for the order 2
        (*ip ^ p) > 0 ? m++ : m--;

        //set the output value
        *ip = IP2 + OP1;
        OP1 = *ip;
    }
}

#endif // #ifdef ENABLE_COMPRESSION_MODE_FAST

#endif // #ifdef BACKWARDS_COMPATIBILITY
