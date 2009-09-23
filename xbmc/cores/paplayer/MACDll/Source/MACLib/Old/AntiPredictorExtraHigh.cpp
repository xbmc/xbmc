#include "All.h"
#ifdef BACKWARDS_COMPATIBILITY

#include "Anti-Predictor.h"

#ifdef ENABLE_COMPRESSION_MODE_EXTRA_HIGH

/*****************************************************************************************
Extra high 0000 to 3320 implementation
*****************************************************************************************/
void CAntiPredictorExtraHigh0000To3320::AntiPredict(int *pInputArray, int *pOutputArray, int NumberOfElements, int Iterations, unsigned int *pOffsetValueArrayA, unsigned int *pOffsetValueArrayB) {
    for (int z = Iterations; z >= 0; z--){
        AntiPredictorOffset(pInputArray, pOutputArray, NumberOfElements, pOffsetValueArrayB[z], -1, 64);
        AntiPredictorOffset(pOutputArray, pInputArray, NumberOfElements, pOffsetValueArrayA[z], 1, 64);
    }

    CAntiPredictorHigh0000To3320 AntiPredictor;
    AntiPredictor.AntiPredict(pInputArray, pOutputArray, NumberOfElements);
}

void CAntiPredictorExtraHigh0000To3320::AntiPredictorOffset(int* Input_Array, int* Output_Array, int Number_of_Elements, int g, int dm, int Max_Order)
{
    int q;

    if ((g==0) || (Number_of_Elements <= Max_Order)) {
        memcpy(Output_Array, Input_Array, Number_of_Elements * 4);
        return;
    }

    memcpy(Output_Array, Input_Array, Max_Order * 4);
    
    int m = 512;

    if (dm > 0)
        for (q = Max_Order; q < Number_of_Elements; q++) {
            Output_Array[q] = Input_Array[q] + (Output_Array[q - g] >> 3);
        }
            
    else
        for (q = Max_Order; q < Number_of_Elements; q++) {
            Output_Array[q] = Input_Array[q] - (Output_Array[q - g] >> 3);
        }
}


/*****************************************************************************************
Extra high 3320 to 3600 implementation
*****************************************************************************************/
void CAntiPredictorExtraHigh3320To3600::AntiPredict(int *pInputArray, int *pOutputArray, int NumberOfElements, int Iterations, unsigned int *pOffsetValueArrayA, unsigned int *pOffsetValueArrayB) 
{
    for (int z = Iterations; z >= 0; z--)
    {
        AntiPredictorOffset(pInputArray, pOutputArray, NumberOfElements, pOffsetValueArrayB[z], -1, 32);
        AntiPredictorOffset(pOutputArray, pInputArray, NumberOfElements, pOffsetValueArrayA[z], 1, 32);
    }

    CAntiPredictorHigh0000To3320 AntiPredictor;
    AntiPredictor.AntiPredict(pInputArray, pOutputArray, NumberOfElements);
}


void CAntiPredictorExtraHigh3320To3600::AntiPredictorOffset(int* Input_Array, int* Output_Array, int Number_of_Elements, int g, int dm, int Max_Order)
{
    
    int q;

    if ((g==0) || (Number_of_Elements <= Max_Order)) {
        memcpy(Output_Array, Input_Array, Number_of_Elements * 4);
        return;
    }

    memcpy(Output_Array, Input_Array, Max_Order * 4);
    
    int m = 512;

    if (dm > 0)
        for (q = Max_Order; q < Number_of_Elements; q++) {
            Output_Array[q] = Input_Array[q] + ((Output_Array[q - g] * m) >> 12);
            (Input_Array[q] ^ Output_Array[q - g]) > 0 ? m += 8 : m -= 8;
        }
            
    else
        for (q = Max_Order; q < Number_of_Elements; q++) {
            Output_Array[q] = Input_Array[q] - ((Output_Array[q - g] * m) >> 12);
            (Input_Array[q] ^ Output_Array[q - g]) > 0 ? m -= 8 : m += 8;
        }
}


/*****************************************************************************************
Extra high 3600 to 3700 implementation
*****************************************************************************************/
void CAntiPredictorExtraHigh3600To3700::AntiPredict(int *pInputArray, int *pOutputArray, int NumberOfElements, int Iterations, unsigned int *pOffsetValueArrayA, unsigned int *pOffsetValueArrayB) {
    for (int z = Iterations; z >= 0; ){

        AntiPredictorOffset(pInputArray, pOutputArray, NumberOfElements, pOffsetValueArrayA[z], pOffsetValueArrayB[z], 64);
        z--;

        if (z >= 0) {
            AntiPredictorOffset(pOutputArray, pInputArray, NumberOfElements, pOffsetValueArrayA[z], pOffsetValueArrayB[z], 64);
            z--;
        }
        else {
            memcpy(pInputArray, pOutputArray, NumberOfElements * 4);
            goto Exit_Loop;
            z--;
        }
    }

Exit_Loop:
    CAntiPredictorHigh3600To3700 AntiPredictor;
    AntiPredictor.AntiPredict(pInputArray, pOutputArray, NumberOfElements);
}

void CAntiPredictorExtraHigh3600To3700::AntiPredictorOffset(int* Input_Array, int* Output_Array, int Number_of_Elements, int g1, int g2, int Max_Order) {
    int q;

    if ((g1==0) || (g2==0) || (Number_of_Elements <= Max_Order)) {
        memcpy(Output_Array, Input_Array, Number_of_Elements * 4);
        return;
    }

    memcpy(Output_Array, Input_Array, Max_Order * 4);
    
    int m = 64;
    int m2 = 64;

    for (q = Max_Order; q < Number_of_Elements; q++) {
        Output_Array[q] = Input_Array[q] + ((Output_Array[q - g1] * m) >> 9) - ((Output_Array[q - g2] * m2) >> 9);
        (Input_Array[q] ^ Output_Array[q - g1]) > 0 ? m++ : m--;
        (Input_Array[q] ^ Output_Array[q - g2]) > 0 ? m2-- : m2++;
    }
}

/*****************************************************************************************
Extra high 3700 to 3800 implementation
*****************************************************************************************/
void CAntiPredictorExtraHigh3700To3800::AntiPredict(int *pInputArray, int *pOutputArray, int NumberOfElements, int Iterations, unsigned int *pOffsetValueArrayA, unsigned int *pOffsetValueArrayB) {
    for (int z = Iterations; z >= 0; ) {

        AntiPredictorOffset(pInputArray, pOutputArray, NumberOfElements, pOffsetValueArrayA[z], pOffsetValueArrayB[z], 64);
        z--;

        if (z >= 0) {
            AntiPredictorOffset(pOutputArray, pInputArray, NumberOfElements, pOffsetValueArrayA[z], pOffsetValueArrayB[z], 64);
            z--;
        }
        else {
            memcpy(pInputArray, pOutputArray, NumberOfElements * 4);
            goto Exit_Loop;
            z--;
        }
    }

Exit_Loop:
    CAntiPredictorHigh3700To3800 AntiPredictor;
    AntiPredictor.AntiPredict(pInputArray, pOutputArray, NumberOfElements);

}

void CAntiPredictorExtraHigh3700To3800::AntiPredictorOffset(int* Input_Array, int* Output_Array, int Number_of_Elements, int g1, int g2, int Max_Order) {
    int q;

    if ((g1==0) || (g2==0) || (Number_of_Elements <= Max_Order)) {
        memcpy(Output_Array, Input_Array, Number_of_Elements * 4);
        return;
    }

    memcpy(Output_Array, Input_Array, Max_Order * 4);
    
    int m = 64;
    int m2 = 64;

    for (q = Max_Order; q < Number_of_Elements; q++) {
        Output_Array[q] = Input_Array[q] + ((Output_Array[q - g1] * m) >> 9) - ((Output_Array[q - g2] * m2) >> 9);
        (Input_Array[q] ^ Output_Array[q - g1]) > 0 ? m++ : m--;
        (Input_Array[q] ^ Output_Array[q - g2]) > 0 ? m2-- : m2++;
    }
}

/*****************************************************************************************
Extra high 3800 to Current
*****************************************************************************************/
void CAntiPredictorExtraHigh3800ToCurrent::AntiPredict(int *pInputArray, int *pOutputArray, int NumberOfElements, BOOL bMMXAvailable, int CPULoadBalancingFactor, int nVersion) 
{
    const int nFilterStageElements = (nVersion < 3830) ? 128 : 256;
    const int nFilterStageShift = (nVersion < 3830) ? 11 : 12;
    const int nMaxElements = (nVersion < 3830) ? 134 : 262;
    const int nFirstElement = (nVersion < 3830) ? 128 : 256;
    const int nStageCShift = (nVersion < 3830) ? 10 : 11;
  
    //short frame handling
    if (NumberOfElements < nMaxElements) {
        memcpy(pOutputArray, pInputArray, NumberOfElements * 4);
        return;
    }

    //make the first five samples identical in both arrays
    memcpy(pOutputArray, pInputArray, nFirstElement * 4);
    
    //variable declares and initializations
    //short bm[nFirstElement]; memset(bm, 0, nFirstElement * 2);
    short bm[256]; memset(bm, 0, 256 * 2);
    int m2 = 64, m3 = 115, m4 = 64, m5 = 740, m6 = 0;
    int p4 = pInputArray[nFirstElement - 1];
    int p3 = (pInputArray[nFirstElement - 1] - pInputArray[nFirstElement - 2]) << 1;
    int p2 = pInputArray[nFirstElement - 1] + ((pInputArray[nFirstElement - 3] - pInputArray[nFirstElement - 2]) << 3);// - pInputArray[3] + pInputArray[2];
    int *op = &pOutputArray[nFirstElement];
    int *ip = &pInputArray[nFirstElement];
    int IPP2 = ip[-2];
    int IPP1 = ip[-1];
    int p7 = 2 * ip[-1] - ip[-2];
    int opp = op[-1];
    int Original;
    CAntiPredictorExtraHighHelper Helper;
    
    //undo the initial prediction stuff
    int q; // loop variable
    for (q = 1; q < nFirstElement; q++) {
        pOutputArray[q] += pOutputArray[q - 1];
    }

    //pump the primary loop
    short *IPAdaptFactor = (short *) calloc(NumberOfElements, 2);
    short *IPShort = (short *) calloc(NumberOfElements, 2);
    for (q = 0; q < nFirstElement; q++) {
        IPAdaptFactor[q] = ((pInputArray[q] >> 30) & 2) - 1;
        IPShort[q] = short(pInputArray[q]);
    }

    int FM[9]; memset(&FM[0], 0, 9 * 4);
    int FP[9]; memset(&FP[0], 0, 9 * 4);

    for (q = nFirstElement; op < &pOutputArray[NumberOfElements]; op++, ip++, q++) {
        //CPU load-balancing
        if (CPULoadBalancingFactor > 0) {
            if ((q % CPULoadBalancingFactor) == 0) { SLEEP(1); }
        }

        if (nVersion >= 3830)
        {
            int *pFP = &FP[8];
            int *pFM = &FM[8];
            int nDotProduct = 0;
            FP[0] = ip[0];
            
            if (FP[0] == 0)
            {
                EXPAND_8_TIMES(nDotProduct += *pFP * *pFM--; *pFP-- = *(pFP - 1);)
            }
            else if (FP[0] > 0)
            {
                EXPAND_8_TIMES(nDotProduct += *pFP * *pFM; *pFM-- += ((*pFP >> 30) & 2) - 1; *pFP-- = *(pFP - 1);)
            }
            else
            {
                EXPAND_8_TIMES(nDotProduct += *pFP * *pFM; *pFM-- -= ((*pFP >> 30) & 2) - 1; *pFP-- = *(pFP - 1);)
            }

            *ip -= nDotProduct >> 9;
        }

        Original = *ip;

        IPShort[q] = short(*ip);
        IPAdaptFactor[q] = ((ip[0] >> 30) & 2) - 1;

#ifdef ENABLE_ASSEMBLY
        if (bMMXAvailable && (Original != 0))
        {
            *ip -= (Helper.MMXDotProduct(&IPShort[q-nFirstElement], &bm[0], &IPAdaptFactor[q-nFirstElement], Original, nFilterStageElements) >> nFilterStageShift);
        }
        else
        {
            *ip -= (Helper.ConventionalDotProduct(&IPShort[q-nFirstElement], &bm[0], &IPAdaptFactor[q-nFirstElement], Original, nFilterStageElements) >> nFilterStageShift);
        }
#else
        *ip -= (Helper.ConventionalDotProduct(&IPShort[q-nFirstElement], &bm[0], &IPAdaptFactor[q-nFirstElement], Original, nFilterStageElements) >> nFilterStageShift);
#endif

        IPShort[q] = short(*ip);
        IPAdaptFactor[q] = ((ip[0] >> 30) & 2) - 1;

        /////////////////////////////////////////////
        *op = *ip + (((p2 * m2) + (p3 * m3) + (p4 * m4)) >> 11);

        if (*ip > 0) {
            m2 -= ((p2 >> 30) & 2) - 1;
            m3 -= ((p3 >> 28) & 8) - 4;
            m4 -= ((p4 >> 28) & 8) - 4;
        }
        else if (*ip < 0) {
            m2 += ((p2 >> 30) & 2) - 1;
            m3 += ((p3 >> 28) & 8) - 4;
            m4 += ((p4 >> 28) & 8) - 4;
        }

        
        p2 = *op + ((IPP2 - p4) << 3);
        p3 = (*op - p4) << 1;
        IPP2 = p4;
        p4 = *op;

        /////////////////////////////////////////////
        *op += (((p7 * m5) - (opp * m6)) >> nStageCShift);

        if (p4 > 0) {
            m5 -= ((p7 >> 29) & 4) - 2;
            m6 += ((opp >> 30) & 2) - 1;
        }
        else if (p4 < 0) {
            m5 += ((p7 >> 29) & 4) - 2;
            m6 -= ((opp >> 30) & 2) - 1;
        }

        p7 = 2 * *op - opp;
        opp = *op;

        /////////////////////////////////////////////
        *op += ((op[-1] * 31) >> 5);
        
    }

    free(IPAdaptFactor);
    free(IPShort);
}

#endif // #ifdef ENABLE_COMPRESSION_MODE_EXTRA_HIGH

#endif // #ifdef BACKWARDS_COMPATIBILITY
