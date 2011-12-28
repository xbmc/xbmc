#ifndef _IFFDSHOWDECAUDIO_H_
#define _IFFDSHOWDECAUDIO_H_

#define FFDSHOWAUDIO_NAME_L     L"ffdshow Audio Decoder"
#define FFDSHOWAUDIORAW_NAME_L  L"ffdshow Audio Processor"

class TaudioCodec;
class Twinamp2;
struct TsampleFormat;
class TaudioFilters;
class TffdshowDecAudioInputPin;
class TinputPin;
template<class tchar> DECLARE_INTERFACE_(IffdshowDecAudioT,IUnknown)
{
 STDMETHOD_(int,getVersion2)(void) PURE;
 STDMETHOD (getMovieSource)(const TaudioCodec* *moviePtr) PURE;
 STDMETHOD (inputSampleFormatDescription)(tchar *buf,size_t buflen) PURE;
 STDMETHOD (getWinamp2)(Twinamp2* *winamp2ptr) PURE;
 STDMETHOD (deliverSample_)(void *buf,size_t numsamples,const TsampleFormat &fmt,float postgain) PURE;
 STDMETHOD (storeMixerMatrixData_)(const double matrix[6][6]) PURE;
 STDMETHOD (getMixerMatrixData_)(double matrix[6][6]) PURE;
 STDMETHOD (deliverSampleSPDIF)(void *buf,size_t bufsize,int bit_rate,unsigned int sample_rate,int incRtDec) PURE;
 STDMETHOD (storeVolumeData_)(unsigned int nchannels,const int channels[],const int volumes[]) PURE;
 STDMETHOD (getVolumeData_)(unsigned int *nchannels,int channels[],int volumes[]) PURE;
 STDMETHOD (storeFFTdata_)(unsigned int num,const float *fft) PURE;
 STDMETHOD_(int,getFFTdataNum_)(void) PURE;
 STDMETHOD (getFFTdata_)(float *fft) PURE;
 STDMETHOD_(unsigned int,getNumStreams2)(void) PURE;
 STDMETHOD (getStreamDescr)(unsigned int i,tchar *buf,size_t buflen) PURE;
 STDMETHOD_(unsigned int,getCurrentStream2)(void) PURE;
 STDMETHOD (setCurrentStream)(unsigned int i) PURE;
 STDMETHOD (setAudioFilters)(TaudioFilters *audioFiltersPtr) PURE;
 STDMETHOD (inputSampleFormat)(unsigned int *nchannels,unsigned int *freq) PURE;
 STDMETHOD (getOutSpeakersDescr)(tchar *buf,size_t buflen,int shortcuts) PURE;
 STDMETHOD (currentSampleFormat)(unsigned int *nchannels,unsigned int *freq,int *sampleFormat) PURE;
 STDMETHOD_(int,getJitter)(void) PURE;
 STDMETHOD_(TffdshowDecAudioInputPin *, GetCurrentPin)(void) PURE;
 STDMETHOD_(TinputPin*, getInputPin)(void) PURE;
 STDMETHOD (deliverSampleBistream)(void *buf,size_t bufsize,int bit_rate,unsigned int sample_rate,int incRtDec,int frame_length,int iec_length) PURE;
 STDMETHOD_(CTransformOutputPin*, getOutputPin)(void) PURE;
 STDMETHOD_(TsampleFormat, getOutsf)(TsampleFormat &outsf)PURE;
 STDMETHOD (getInputTime)(REFERENCE_TIME &rtStart, REFERENCE_TIME &rtStop)PURE;
};

struct IffdshowDecAudioA :IffdshowDecAudioT<char> {};
struct IffdshowDecAudioW :IffdshowDecAudioT<wchar_t> {};

#ifndef DEFINE_TGUID
 #define DEFINE_TGUID(IID,I, l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) DEFINE_GUID(IID##_##I,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8);
#endif
// {0FBB9F54-D4CD-428f-A237-2FAE376EB5F5}
DEFINE_TGUID(IID,IffdshowDecAudioA    , 0x0fbb9f54, 0xd4cd, 0x428f, 0xa2, 0x37, 0x2f, 0xae, 0x37, 0x6e, 0xb5, 0xf5)
// {1FBB9F54-D4CD-428f-A237-2FAE376EB5F5}
DEFINE_TGUID(IID,IffdshowDecAudioW    , 0x1fbb9f54, 0xd4cd, 0x428f, 0xa2, 0x37, 0x2f, 0xae, 0x37, 0x6e, 0xb5, 0xf5)
// {0F40E1E5-4F79-4988-B1A9-CC98794E6B55}
DEFINE_GUID(CLSID_FFDSHOWAUDIO        , 0x0f40e1e5, 0x4f79, 0x4988, 0xb1, 0xa9, 0xcc, 0x98, 0x79, 0x4e, 0x6b, 0x55);
// {007FC171-01AA-4b3a-B2DB-062DEE815A1E}
DEFINE_GUID(CLSID_TFFDSHOWAUDIOPAGE   , 0x007fc171, 0x01aa, 0x4b3a, 0xb2, 0xdb, 0x06, 0x2d, 0xee, 0x81, 0x5a, 0x1e);
// {B86F6BEE-E7C0-4d03-8D52-5B4430CF6C88}
DEFINE_GUID(CLSID_FFDSHOWAUDIORAW     , 0xb86f6bee, 0xe7c0, 0x4d03, 0x8d, 0x52, 0x5b, 0x44, 0x30, 0xcf, 0x6c, 0x88);
// {3E3ECA90-4D6A-4344-98C3-1BB95BF24038}
DEFINE_GUID(CLSID_TFFDSHOWAUDIORAWPAGE, 0x3e3eca90, 0x4d6a, 0x4344, 0x98, 0xc3, 0x1b, 0xb9, 0x5b, 0xf2, 0x40, 0x38);

#endif
