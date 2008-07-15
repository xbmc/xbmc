#include "All.h"
#include "Prepare.h"

const uint32 CRC32_TABLE[256] = {0,1996959894,3993919788,2567524794,124634137,1886057615,3915621685,2657392035,249268274,2044508324,3772115230,2547177864,162941995,2125561021,3887607047,2428444049,498536548,1789927666,4089016648,2227061214,450548861,1843258603,4107580753,2211677639,325883990,1684777152,4251122042,2321926636,335633487,1661365465,4195302755,2366115317,997073096,1281953886,3579855332,2724688242,1006888145,1258607687,3524101629,2768942443,901097722,1119000684,3686517206,2898065728,853044451,1172266101,3705015759,2882616665,651767980,1373503546,3369554304,3218104598,565507253,1454621731,3485111705,3099436303,671266974,1594198024,3322730930,2970347812,795835527,1483230225,3244367275,3060149565,1994146192,31158534,2563907772,4023717930,1907459465,112637215,2680153253,3904427059,2013776290,251722036,2517215374,3775830040,2137656763,141376813,2439277719,3865271297,1802195444,476864866,2238001368,
    4066508878,1812370925,453092731,2181625025,4111451223,1706088902,314042704,2344532202,4240017532,1658658271,366619977,2362670323,4224994405,1303535960,984961486,2747007092,3569037538,1256170817,1037604311,2765210733,3554079995,1131014506,879679996,2909243462,3663771856,1141124467,855842277,2852801631,3708648649,1342533948,654459306,3188396048,3373015174,1466479909,544179635,3110523913,3462522015,1591671054,702138776,2966460450,3352799412,1504918807,783551873,3082640443,3233442989,3988292384,2596254646,62317068,1957810842,3939845945,2647816111,81470997,1943803523,3814918930,2489596804,225274430,2053790376,3826175755,2466906013,167816743,2097651377,4027552580,2265490386,503444072,1762050814,4150417245,2154129355,426522225,1852507879,4275313526,2312317920,282753626,1742555852,4189708143,2394877945,397917763,1622183637,3604390888,2714866558,953729732,1340076626,3518719985,2797360999,1068828381,1219638859,3624741850,
    2936675148,906185462,1090812512,3747672003,2825379669,829329135,1181335161,3412177804,3160834842,628085408,1382605366,3423369109,3138078467,570562233,1426400815,3317316542,2998733608,733239954,1555261956,3268935591,3050360625,752459403,1541320221,2607071920,3965973030,1969922972,40735498,2617837225,3943577151,1913087877,83908371,2512341634,3803740692,2075208622,213261112,2463272603,3855990285,2094854071,198958881,2262029012,4057260610,1759359992,534414190,2176718541,4139329115,1873836001,414664567,2282248934,4279200368,1711684554,285281116,2405801727,4167216745,1634467795,376229701,2685067896,3608007406,1308918612,956543938,2808555105,3495958263,1231636301,1047427035,2932959818,3654703836,1088359270,936918000,2847714899,3736837829,1202900863,817233897,3183342108,3401237130,1404277552,615818150,3134207493,3453421203,1423857449,601450431,3009837614,3294710456,1567103746,711928724,3020668471,3272380065,1510334235,755167117};

int CPrepare::Prepare(const unsigned char * pRawData, int nBytes, const WAVEFORMATEX * pWaveFormatEx, int * pOutputX, int *pOutputY, unsigned int *pCRC, int *pSpecialCodes, int *pPeakLevel)
{
    // error check the parameters
    if (pRawData == NULL || pWaveFormatEx == NULL)
        return ERROR_BAD_PARAMETER;

    // initialize the pointers that got passed in
    *pCRC = 0xFFFFFFFF;
    *pSpecialCodes = 0;

    // variables
    uint32 CRC = 0xFFFFFFFF;
    const int nTotalBlocks = nBytes / pWaveFormatEx->nBlockAlign;
    int R,L;

    // the prepare code

    if (pWaveFormatEx->wBitsPerSample == 8) 
    {
        if (pWaveFormatEx->nChannels == 2) 
        {
            for (int nBlockIndex = 0; nBlockIndex < nTotalBlocks; nBlockIndex++) 
            {
                R = (int) (*((unsigned char *) pRawData) - 128);
                L = (int) (*((unsigned char *) (pRawData + 1)) - 128);

                CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *pRawData++];
                CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *pRawData++];
                
                // check the peak
                if (labs(L) > *pPeakLevel)
                    *pPeakLevel = labs(L);
                if (labs(R) > *pPeakLevel)
                    *pPeakLevel = labs(R);

                // convert to x,y
                pOutputY[nBlockIndex] = L - R;
                pOutputX[nBlockIndex] = R + (pOutputY[nBlockIndex] / 2);
            }
        }
        else if (pWaveFormatEx->nChannels == 1) 
        {
            for (int nBlockIndex = 0; nBlockIndex < nTotalBlocks; nBlockIndex++) 
            {
                R = (int) (*((unsigned char *) pRawData) - 128);
                                
                CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *pRawData++];
                
                // check the peak
                if (labs(R) > *pPeakLevel)
                    *pPeakLevel = labs(R);

                // convert to x,y
                pOutputX[nBlockIndex] = R;
            }
        }
    }
    else if (pWaveFormatEx->wBitsPerSample == 24) 
    {
        if (pWaveFormatEx->nChannels == 2) 
        {
            for (int nBlockIndex = 0; nBlockIndex < nTotalBlocks; nBlockIndex++) 
            {
                uint32 nTemp = 0;
                
                nTemp |= (*pRawData << 0);
                CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *pRawData++];

                nTemp |= (*pRawData << 8);
                CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *pRawData++];

                nTemp |= (*pRawData << 16);
                CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *pRawData++];

                if (nTemp & 0x800000)
                    R = (int) (nTemp & 0x7FFFFF) - 0x800000;
                else
                    R = (int) (nTemp & 0x7FFFFF);

                nTemp = 0;

                nTemp |= (*pRawData << 0);
                CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *pRawData++];
                
                nTemp |= (*pRawData << 8);
                CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *pRawData++];
                
                nTemp |= (*pRawData << 16);
                CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *pRawData++];
                                
                if (nTemp & 0x800000)
                    L = (int) (nTemp & 0x7FFFFF) - 0x800000;
                else
                    L = (int) (nTemp & 0x7FFFFF);

                // check the peak
                if (labs(L) > *pPeakLevel)
                    *pPeakLevel = labs(L);
                if (labs(R) > *pPeakLevel)
                    *pPeakLevel = labs(R);

                // convert to x,y
                pOutputY[nBlockIndex] = L - R;
                pOutputX[nBlockIndex] = R + (pOutputY[nBlockIndex] / 2);

            }
        }
        else if (pWaveFormatEx->nChannels == 1) 
        {
            for (int nBlockIndex = 0; nBlockIndex < nTotalBlocks; nBlockIndex++) 
            {
                uint32 nTemp = 0;
                
                nTemp |= (*pRawData << 0);
                CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *pRawData++];
                
                nTemp |= (*pRawData << 8);
                CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *pRawData++];
                
                nTemp |= (*pRawData << 16);
                CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *pRawData++];
                
                if (nTemp & 0x800000)
                    R = (int) (nTemp & 0x7FFFFF) - 0x800000;
                else
                    R = (int) (nTemp & 0x7FFFFF);
    
                // check the peak
                if (labs(R) > *pPeakLevel)
                    *pPeakLevel = labs(R);

                // convert to x,y
                pOutputX[nBlockIndex] = R;
            }
        }
    }
    else 
    {
        if (pWaveFormatEx->nChannels == 2) 
        {
            int LPeak = 0;
            int RPeak = 0;
            int nBlockIndex = 0;
            for (nBlockIndex = 0; nBlockIndex < nTotalBlocks; nBlockIndex++) 
            {

                R = (int) *((int16 *) pRawData);
                CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *pRawData++];
                CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *pRawData++];

                L = (int) *((int16 *) pRawData);
                CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *pRawData++];
                CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *pRawData++];

                // check the peak
                if (labs(L) > LPeak)
                    LPeak = labs(L);
                if (labs(R) > RPeak)
                    RPeak = labs(R);

                // convert to x,y
                pOutputY[nBlockIndex] = L - R;
                pOutputX[nBlockIndex] = R + (pOutputY[nBlockIndex] / 2);
            }

            if (LPeak == 0) { *pSpecialCodes |= SPECIAL_FRAME_LEFT_SILENCE; }
            if (RPeak == 0) { *pSpecialCodes |= SPECIAL_FRAME_RIGHT_SILENCE; }
            if (max(LPeak, RPeak) > *pPeakLevel) 
            {
                *pPeakLevel = max(LPeak, RPeak);
            }

            // check for pseudo-stereo files
            nBlockIndex = 0;
            while (pOutputY[nBlockIndex++] == 0) 
            {
                if (nBlockIndex == (nBytes / 4)) 
                {
                    *pSpecialCodes |= SPECIAL_FRAME_PSEUDO_STEREO;
                    break;
                }
            }

        }
        else if (pWaveFormatEx->nChannels == 1) 
        {
            int nPeak = 0;
            for (int nBlockIndex = 0; nBlockIndex < nTotalBlocks; nBlockIndex++) 
            {
                R = (int) *((int16 *) pRawData);
                
                CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *pRawData++];
                CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *pRawData++];
                
                // check the peak
                if (labs(R) > nPeak)
                    nPeak = labs(R);

                //convert to x,y
                pOutputX[nBlockIndex] = R;
            }

            if (nPeak > *pPeakLevel)
                *pPeakLevel = nPeak;
            if (nPeak == 0) { *pSpecialCodes |= SPECIAL_FRAME_MONO_SILENCE; }
        }
    }

    CRC = CRC ^ 0xFFFFFFFF;

    // add the special code
    CRC >>= 1;

    if (*pSpecialCodes != 0) 
    {
        CRC |= (1 << 31);
    }

    *pCRC = CRC;

    return ERROR_SUCCESS;
}

void CPrepare::Unprepare(int X, int Y, const WAVEFORMATEX * pWaveFormatEx, unsigned char * pOutput, unsigned int * pCRC)
{
    #define CALCULATE_CRC_BYTE    *pCRC = (*pCRC >> 8) ^ CRC32_TABLE[(*pCRC & 0xFF) ^ *pOutput++];
    // decompress and convert from (x,y) -> (l,r)
    // sort of long and ugly.... sorry
    
    if (pWaveFormatEx->nChannels == 2) 
    {
        if (pWaveFormatEx->wBitsPerSample == 16) 
        {
            // get the right and left values
            int nR = X - (Y / 2);
            int nL = nR + Y;

            // error check (for overflows)
            if ((nR < -32768) || (nR > 32767) || (nL < -32768) || (nL > 32767))
            {
                throw(-1);
            }

            *(int16 *) pOutput = (int16) nR;
            CALCULATE_CRC_BYTE
            CALCULATE_CRC_BYTE
                
            *(int16 *) pOutput = (int16) nL;
            CALCULATE_CRC_BYTE
            CALCULATE_CRC_BYTE
        }
        else if (pWaveFormatEx->wBitsPerSample == 8) 
        {
            unsigned char R = (X - (Y / 2) + 128);
            *pOutput = R;
            CALCULATE_CRC_BYTE
            *pOutput = (unsigned char) (R + Y);
            CALCULATE_CRC_BYTE
        }
        else if (pWaveFormatEx->wBitsPerSample == 24) 
        {
            int32 RV, LV;

            RV = X - (Y / 2);
            LV = RV + Y;
            
            uint32 nTemp = 0;
            if (RV < 0)
                nTemp = ((uint32) (RV + 0x800000)) | 0x800000;
            else
                nTemp = (uint32) RV;    
            
            *pOutput = (unsigned char) ((nTemp >> 0) & 0xFF);
            CALCULATE_CRC_BYTE
            *pOutput = (unsigned char) ((nTemp >> 8) & 0xFF);
            CALCULATE_CRC_BYTE
            *pOutput = (unsigned char) ((nTemp >> 16) & 0xFF);
            CALCULATE_CRC_BYTE

            nTemp = 0;
            if (LV < 0)
                nTemp = ((uint32) (LV + 0x800000)) | 0x800000;
            else
                nTemp = (uint32) LV;    
            
            *pOutput = (unsigned char) ((nTemp >> 0) & 0xFF);
            CALCULATE_CRC_BYTE
            
            *pOutput = (unsigned char) ((nTemp >> 8) & 0xFF);
            CALCULATE_CRC_BYTE
            
            *pOutput = (unsigned char) ((nTemp >> 16) & 0xFF);
            CALCULATE_CRC_BYTE
        }
    }
    else if (pWaveFormatEx->nChannels == 1) 
    {
        if (pWaveFormatEx->wBitsPerSample == 16) 
        {
            int16 R = X;
                
            *(int16 *) pOutput = (int16) R;
            CALCULATE_CRC_BYTE
            CALCULATE_CRC_BYTE
        }
        else if (pWaveFormatEx->wBitsPerSample == 8) 
        {
            unsigned char R = X + 128;
            *pOutput = R;
            CALCULATE_CRC_BYTE
        }
        else if (pWaveFormatEx->wBitsPerSample == 24) 
        {
            int32 RV = X;
            
            uint32 nTemp = 0;
            if (RV < 0)
                nTemp = ((uint32) (RV + 0x800000)) | 0x800000;
            else
                nTemp = (uint32) RV;    
            
            *pOutput = (unsigned char) ((nTemp >> 0) & 0xFF);
            CALCULATE_CRC_BYTE
            *pOutput = (unsigned char) ((nTemp >> 8) & 0xFF);
            CALCULATE_CRC_BYTE
            *pOutput = (unsigned char) ((nTemp >> 16) & 0xFF);
            CALCULATE_CRC_BYTE
        }
    }
}

#ifdef BACKWARDS_COMPATIBILITY

int CPrepare::UnprepareOld(int *pInputX, int *pInputY, int nBlocks, const WAVEFORMATEX *pWaveFormatEx, unsigned char *pRawData, unsigned int *pCRC, int *pSpecialCodes, int nFileVersion)
{
    // the CRC that will be figured during decompression
    uint32 CRC = 0xFFFFFFFF;

    // decompress and convert from (x,y) -> (l,r)
    // sort of int and ugly.... sorry
    if (pWaveFormatEx->nChannels == 2) 
    {
        // convert the x,y data to raw data
        if (pWaveFormatEx->wBitsPerSample == 16) 
        {
            int16 R;
            unsigned char *Buffer = &pRawData[0];
            int *pX = pInputX;
            int *pY = pInputY;

            for (; pX < &pInputX[nBlocks]; pX++, pY++) 
            {
                R = *pX - (*pY / 2);
                
                *(int16 *) Buffer = (int16) R;
                CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *Buffer++];
                CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *Buffer++];
                
                *(int16 *) Buffer = (int16) R + *pY;
                CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *Buffer++];
                CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *Buffer++];
            }
        }
        else if (pWaveFormatEx->wBitsPerSample == 8) 
        {
            unsigned char *R = (unsigned char *) &pRawData[0];
            unsigned char *L = (unsigned char *) &pRawData[1];

            if (nFileVersion > 3830) 
            {
                for (int SampleIndex = 0; SampleIndex < nBlocks; SampleIndex++, L+=2, R+=2) 
                {
                    *R = (unsigned char) (pInputX[SampleIndex] - (pInputY[SampleIndex] / 2) + 128);
                    CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *R];
                    *L = (unsigned char) (*R + pInputY[SampleIndex]);
                    CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *L];
                }
            }
            else 
            {
                for (int SampleIndex = 0; SampleIndex < nBlocks; SampleIndex++, L+=2, R+=2)
                {
                    *R = (unsigned char) (pInputX[SampleIndex] - (pInputY[SampleIndex] / 2));
                    CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *R];
                    *L = (unsigned char) (*R + pInputY[SampleIndex]);
                    CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *L];

                }
            }
        }
        else if (pWaveFormatEx->wBitsPerSample == 24) 
        {
            unsigned char *Buffer = (unsigned char *) &pRawData[0];
            int32 RV, LV;

            for (int SampleIndex = 0; SampleIndex < nBlocks; SampleIndex++)
            {
                RV = pInputX[SampleIndex] - (pInputY[SampleIndex] / 2);
                LV = RV + pInputY[SampleIndex];
                
                uint32 nTemp = 0;
                if (RV < 0)
                    nTemp = ((uint32) (RV + 0x800000)) | 0x800000;
                else
                    nTemp = (uint32) RV;    
                
                *Buffer = (unsigned char) ((nTemp >> 0) & 0xFF);
                CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *Buffer++];
                
                *Buffer = (unsigned char) ((nTemp >> 8) & 0xFF);
                CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *Buffer++];

                *Buffer = (unsigned char) ((nTemp >> 16) & 0xFF);
                CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *Buffer++];

                nTemp = 0;
                if (LV < 0)
                    nTemp = ((uint32) (LV + 0x800000)) | 0x800000;
                else
                    nTemp = (uint32) LV;    
                
                *Buffer = (unsigned char) ((nTemp >> 0) & 0xFF);
                CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *Buffer++];
                
                *Buffer = (unsigned char) ((nTemp >> 8) & 0xFF);
                CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *Buffer++];
                
                *Buffer = (unsigned char) ((nTemp >> 16) & 0xFF);
                CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *Buffer++];
            }
        }
    }
    else if (pWaveFormatEx->nChannels == 1) 
    {
        // convert to raw data
        if (pWaveFormatEx->wBitsPerSample == 8) 
        {
            unsigned char *R = (unsigned char *) &pRawData[0];

            if (nFileVersion > 3830) 
            {
                for (int SampleIndex = 0; SampleIndex < nBlocks; SampleIndex++, R++)
                {
                    *R = pInputX[SampleIndex] + 128;
                    CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *R];
                }
            }
            else 
            {
                for (int SampleIndex = 0; SampleIndex < nBlocks; SampleIndex++, R++)
                {
                    *R = (unsigned char) (pInputX[SampleIndex]);
                    CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *R];
                }
            }

        }
        else if (pWaveFormatEx->wBitsPerSample == 24) 
        {

            unsigned char *Buffer = (unsigned char *) &pRawData[0];
            int32 RV;
            for (int SampleIndex = 0; SampleIndex<nBlocks; SampleIndex++) 
            {
                RV = pInputX[SampleIndex];

                uint32 nTemp = 0;
                if (RV < 0)
                    nTemp = ((uint32) (RV + 0x800000)) | 0x800000;
                else
                    nTemp = (uint32) RV;    
                
                *Buffer = (unsigned char) ((nTemp >> 0) & 0xFF);
                CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *Buffer++];
                
                *Buffer = (unsigned char) ((nTemp >> 8) & 0xFF);
                CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *Buffer++];
                
                *Buffer = (unsigned char) ((nTemp >> 16) & 0xFF);
                CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *Buffer++];
            }
        }
        else 
        {
            unsigned char *Buffer = &pRawData[0];

            for (int SampleIndex = 0; SampleIndex < nBlocks; SampleIndex++) 
            {
                *(int16 *) Buffer = (int16) (pInputX[SampleIndex]);
                CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *Buffer++];
                CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *Buffer++];
            }
        }
    }

    CRC = CRC ^ 0xFFFFFFFF;

    *pCRC = CRC;

    return 0;
}

#endif // #ifdef BACKWARDS_COMPATIBILITY
