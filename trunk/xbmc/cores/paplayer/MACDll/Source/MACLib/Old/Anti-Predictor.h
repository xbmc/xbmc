#ifndef APE_ANTIPREDICTOR_H
#define APE_ANTIPREDICTOR_H

class CAntiPredictor;

CAntiPredictor * CreateAntiPredictor(int nCompressionLevel, int nVersion);

/*****************************************************************************************
Base class for all anti-predictors
*****************************************************************************************/
class CAntiPredictor 
{
public:

    // construction/destruction
    CAntiPredictor();
    ~CAntiPredictor();

    // functions
    virtual void AntiPredict(int *pInputArray, int *pOutputArray, int NumberOfElements);

};

/*****************************************************************************************
Offset anti-predictor
*****************************************************************************************/
class CAntiPredictorOffset : public CAntiPredictor 
{
public:

    // functions
    void AntiPredict(int *pInputArray, int *pOutputArray, int NumberOfElements, int Offset, int DeltaM);
};

#ifdef ENABLE_COMPRESSION_MODE_FAST

/*****************************************************************************************
Fast anti-predictor (from original 'fast' mode...updated for version 3.32)
*****************************************************************************************/
class CAntiPredictorFast0000To3320 : public CAntiPredictor {

public:

    // functions
    void AntiPredict(int *pInputArray, int *pOutputArray, int NumberOfElements);

};

/*****************************************************************************************
Fast anti-predictor (new 'fast' mode release with version 3.32)
*****************************************************************************************/
class CAntiPredictorFast3320ToCurrent : public CAntiPredictor {

public:

    // functions
    void AntiPredict(int *pInputArray, int *pOutputArray, int NumberOfElements);

};

#endif // #ifdef ENABLE_COMPRESSION_MODE_FAST

#ifdef ENABLE_COMPRESSION_MODE_NORMAL
/*****************************************************************************************
Normal anti-predictor
*****************************************************************************************/
class CAntiPredictorNormal0000To3320 : public CAntiPredictor {

public:

    // functions
    void AntiPredict(int *pInputArray, int *pOutputArray, int NumberOfElements);

};

/*****************************************************************************************
Normal anti-predictor
*****************************************************************************************/
class CAntiPredictorNormal3320To3800 : public CAntiPredictor {

public:

    // functions
    void AntiPredict(int *pInputArray, int *pOutputArray, int NumberOfElements);

};

/*****************************************************************************************
Normal anti-predictor
*****************************************************************************************/
class CAntiPredictorNormal3800ToCurrent : public CAntiPredictor {

public:

    // functions
    void AntiPredict(int *pInputArray, int *pOutputArray, int NumberOfElements);

};

#endif // #ifdef ENABLE_COMPRESSION_MODE_NORMAL

#ifdef ENABLE_COMPRESSION_MODE_HIGH

/*****************************************************************************************
High anti-predictor
*****************************************************************************************/
class CAntiPredictorHigh0000To3320 : public CAntiPredictor {

public:

    // functions
    void AntiPredict(int *pInputArray, int *pOutputArray, int NumberOfElements);

};

/*****************************************************************************************
High anti-predictor
*****************************************************************************************/
class CAntiPredictorHigh3320To3600 : public CAntiPredictor {

public:

    // functions
    void AntiPredict(int *pInputArray, int *pOutputArray, int NumberOfElements);

};

/*****************************************************************************************
High anti-predictor
*****************************************************************************************/
class CAntiPredictorHigh3600To3700 : public CAntiPredictor {

public:

    // functions
    void AntiPredict(int *pInputArray, int *pOutputArray, int NumberOfElements);

};

/*****************************************************************************************
High anti-predictor
*****************************************************************************************/
class CAntiPredictorHigh3700To3800 : public CAntiPredictor {

public:

    // functions
    void AntiPredict(int *pInputArray, int *pOutputArray, int NumberOfElements);

};

/*****************************************************************************************
High anti-predictor
*****************************************************************************************/
class CAntiPredictorHigh3800ToCurrent : public CAntiPredictor {

public:

    // functions
    void AntiPredict(int *pInputArray, int *pOutputArray, int NumberOfElements);

};

#endif // #ifdef ENABLE_COMPRESSION_MODE_HIGH

#ifdef ENABLE_COMPRESSION_MODE_EXTRA_HIGH

/*****************************************************************************************
Extra high helper
*****************************************************************************************/
class CAntiPredictorExtraHighHelper
{
public:
    int ConventionalDotProduct(short *bip, short *bbm, short *pIPAdaptFactor, int op, int nNumberOfIterations);

#ifdef ENABLE_ASSEMBLY
    int MMXDotProduct(short *bip, short *bbm, short *pIPAdaptFactor, int op, int nNumberOfIterations);
#endif // #ifdef ENABLE_ASSEMBLY
};


/*****************************************************************************************
Extra high anti-predictor
*****************************************************************************************/
class CAntiPredictorExtraHigh0000To3320 : public CAntiPredictor {

public:

    // functions
    void AntiPredict(int *pInputArray, int *pOutputArray, int NumberOfElements, int Iterations, unsigned int *pOffsetValueArrayA, unsigned int *pOffsetValueArrayB);

private:
    void AntiPredictorOffset(int* Input_Array, int* Output_Array, int Number_of_Elements, int g, int dm, int Max_Order);

};

/*****************************************************************************************
Extra high anti-predictor
*****************************************************************************************/
class CAntiPredictorExtraHigh3320To3600 : public CAntiPredictor {

public:

    // functions
    void AntiPredict(int *pInputArray, int *pOutputArray, int NumberOfElements, int Iterations, unsigned int *pOffsetValueArrayA, unsigned int *pOffsetValueArrayB);

private:
    void AntiPredictorOffset(int* Input_Array, int* Output_Array, int Number_of_Elements, int g, int dm, int Max_Order);
};

/*****************************************************************************************
Extra high anti-predictor
*****************************************************************************************/
class CAntiPredictorExtraHigh3600To3700 : public CAntiPredictor {

public:

    // functions
    void AntiPredict(int *pInputArray, int *pOutputArray, int NumberOfElements, int Iterations, unsigned int *pOffsetValueArrayA, unsigned int *pOffsetValueArrayB);

private:
    void AntiPredictorOffset(int* Input_Array, int* Output_Array, int Number_of_Elements, int g1, int g2, int Max_Order);

};

/*****************************************************************************************
Extra high anti-predictor
*****************************************************************************************/
class CAntiPredictorExtraHigh3700To3800 : public CAntiPredictor {

public:

    // functions
    void AntiPredict(int *pInputArray, int *pOutputArray, int NumberOfElements, int Iterations, unsigned int *pOffsetValueArrayA, unsigned int *pOffsetValueArrayB);

private:
    void AntiPredictorOffset(int* Input_Array, int* Output_Array, int Number_of_Elements, int g1, int g2, int Max_Order);
};

/*****************************************************************************************
Extra high anti-predictor
*****************************************************************************************/
class CAntiPredictorExtraHigh3800ToCurrent : public CAntiPredictor {

public:

    // functions
    void AntiPredict(int *pInputArray, int *pOutputArray, int NumberOfElements, BOOL bMMXAvailable, int CPULoadBalancingFactor, int nVersion);
};

#endif // #ifdef ENABLE_COMPRESSION_MODE_EXTRA_HIGH

#endif // #ifndef APE_ANTIPREDICTOR_H

