#include "All.h"
#include "GlobalFunctions.h"
#include "NNFilter.h"
#include "Assembly/Assembly.h"

CNNFilter::CNNFilter(int nOrder, int nShift, int nVersion)
{
    if ((nOrder <= 0) || ((nOrder % 16) != 0)) throw(1);
    m_nOrder = nOrder;
    m_nShift = nShift;
    m_nVersion = nVersion;
    
    m_bMMXAvailable = false;//GetMMXAvailable();
    
    m_rbInput.Create(NN_WINDOW_ELEMENTS, m_nOrder);
    m_rbDeltaM.Create(NN_WINDOW_ELEMENTS, m_nOrder);
    m_paryM = new short [m_nOrder];

#ifdef NN_TEST_MMX
    srand(GetTickCount());
#endif
}

CNNFilter::~CNNFilter()
{
    SAFE_ARRAY_DELETE(m_paryM)
}

void CNNFilter::Flush()
{
    memset(&m_paryM[0], 0, m_nOrder * sizeof(short));
    m_rbInput.Flush();
    m_rbDeltaM.Flush();
    m_nRunningAverage = 0;
}

int CNNFilter::Compress(int nInput)
{
    // convert the input to a short and store it
    m_rbInput[0] = GetSaturatedShortFromInt(nInput);

    // figure a dot product
    int nDotProduct;
    if (m_bMMXAvailable)
        nDotProduct = CalculateDotProduct(&m_rbInput[-m_nOrder], &m_paryM[0], m_nOrder);
    else
        nDotProduct = CalculateDotProductNoMMX(&m_rbInput[-m_nOrder], &m_paryM[0], m_nOrder);

    // calculate the output
    int nOutput = nInput - ((nDotProduct + (1 << (m_nShift - 1))) >> m_nShift);

    // adapt
    if (m_bMMXAvailable)
        Adapt(&m_paryM[0], &m_rbDeltaM[-m_nOrder], -nOutput, m_nOrder);
    else
        AdaptNoMMX(&m_paryM[0], &m_rbDeltaM[-m_nOrder], nOutput, m_nOrder);

    int nTempABS = abs(nInput);

    if (nTempABS > (m_nRunningAverage * 3))
        m_rbDeltaM[0] = ((nInput >> 25) & 64) - 32;
    else if (nTempABS > (m_nRunningAverage * 4) / 3)
        m_rbDeltaM[0] = ((nInput >> 26) & 32) - 16;
    else if (nTempABS > 0)
        m_rbDeltaM[0] = ((nInput >> 27) & 16) - 8;
    else
        m_rbDeltaM[0] = 0;

    m_nRunningAverage += (nTempABS - m_nRunningAverage) / 16;

    m_rbDeltaM[-1] >>= 1;
    m_rbDeltaM[-2] >>= 1;
    m_rbDeltaM[-8] >>= 1;
        
    // increment and roll if necessary
    m_rbInput.IncrementSafe();
    m_rbDeltaM.IncrementSafe();

    return nOutput;
}

int CNNFilter::Decompress(int nInput)
{
    // figure a dot product
    int nDotProduct;

    if (m_bMMXAvailable)
        nDotProduct = CalculateDotProduct(&m_rbInput[-m_nOrder], &m_paryM[0], m_nOrder);
    else
        nDotProduct = CalculateDotProductNoMMX(&m_rbInput[-m_nOrder], &m_paryM[0], m_nOrder);
    
    // adapt
    if (m_bMMXAvailable)
        Adapt(&m_paryM[0], &m_rbDeltaM[-m_nOrder], -nInput, m_nOrder);
    else
        AdaptNoMMX(&m_paryM[0], &m_rbDeltaM[-m_nOrder], nInput, m_nOrder);

    // store the output value
    int nOutput = nInput + ((nDotProduct + (1 << (m_nShift - 1))) >> m_nShift);

    // update the input buffer
    m_rbInput[0] = GetSaturatedShortFromInt(nOutput);

    if (m_nVersion >= 3980)
    {
        int nTempABS = abs(nOutput);

        if (nTempABS > (m_nRunningAverage * 3))
            m_rbDeltaM[0] = ((nOutput >> 25) & 64) - 32;
        else if (nTempABS > (m_nRunningAverage * 4) / 3)
            m_rbDeltaM[0] = ((nOutput >> 26) & 32) - 16;
        else if (nTempABS > 0)
            m_rbDeltaM[0] = ((nOutput >> 27) & 16) - 8;
        else
            m_rbDeltaM[0] = 0;

        m_nRunningAverage += (nTempABS - m_nRunningAverage) / 16;

        m_rbDeltaM[-1] >>= 1;
        m_rbDeltaM[-2] >>= 1;
        m_rbDeltaM[-8] >>= 1;
    }
    else
    {
        m_rbDeltaM[0] = (nOutput == 0) ? 0 : ((nOutput >> 28) & 8) - 4;
        m_rbDeltaM[-4] >>= 1;
        m_rbDeltaM[-8] >>= 1;
    }

    // increment and roll if necessary
    m_rbInput.IncrementSafe();
    m_rbDeltaM.IncrementSafe();
    
    return nOutput;
}

void CNNFilter::AdaptNoMMX(short * pM, short * pAdapt, int nDirection, int nOrder)
{
    nOrder >>= 4;

    if (nDirection < 0) 
    {    
        while (nOrder--)
        {
            EXPAND_16_TIMES(*pM++ += *pAdapt++;)
        }
    }
    else if (nDirection > 0)
    {
        while (nOrder--)
        {
            EXPAND_16_TIMES(*pM++ -= *pAdapt++;)
        }
    }
}

int CNNFilter::CalculateDotProductNoMMX(short * pA, short * pB, int nOrder)
{
    int nDotProduct = 0;
    nOrder >>= 4;

    while (nOrder--)
    {
        EXPAND_16_TIMES(nDotProduct += *pA++ * *pB++;)
    }
    
    return nDotProduct;
}
