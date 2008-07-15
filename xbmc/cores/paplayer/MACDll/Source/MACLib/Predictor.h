#ifndef APE_PREDICTOR_H
#define APE_PREDICTOR_H

/*************************************************************************************************
IPredictorCompress - the interface for compressing (predicting) data
*************************************************************************************************/
class IPredictorCompress
{
public:
    IPredictorCompress(int nCompressionLevel) {}
    virtual ~IPredictorCompress() {}

    virtual int CompressValue(int nA, int nB = 0) = 0;
    virtual int Flush() = 0;
};

/*************************************************************************************************
IPredictorDecompress - the interface for decompressing (un-predicting) data
*************************************************************************************************/
class IPredictorDecompress
{
public:
    IPredictorDecompress(int nCompressionLevel, int nVersion) {}
    virtual ~IPredictorDecompress() {}

    virtual int DecompressValue(int nA, int nB = 0) = 0;
    virtual int Flush() = 0;
};

#endif // #ifndef APE_PREDICTOR_H
