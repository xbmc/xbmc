#ifndef APE_ASSEMBLY_H
#define APE_ASSEMBLY_H

extern "C" 
{
    void Adapt(short * pM, const short * pAdapt, int nDirection, int nOrder);
    int CalculateDotProduct(const short * pA, const short * pB, int nOrder);
    BOOL GetMMXAvailable();
}

#endif // #ifndef APE_ASSEMBLY_H

