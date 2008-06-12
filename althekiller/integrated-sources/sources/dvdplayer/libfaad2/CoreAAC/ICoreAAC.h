#ifndef _ICoreAAC_H_
#define _ICoreAAC_H_

#ifdef __cplusplus
extern "C" {
#endif

	
// ICoreAACDec GUID
// {231E221B-AA58-4a83-A209-06C3526E7EE4}
DEFINE_GUID(IID_ICoreAACDec, 
	0x231e221b, 0xaa58, 0x4a83, 0xa2, 0x9, 0x6, 0xc3, 0x52, 0x6e, 0x7e, 0xe4);
	
//
// ICoreAACDec
//
DECLARE_INTERFACE_(ICoreAACDec, IUnknown)
{
	STDMETHOD(get_ProfileName)(THIS_ char** name) PURE;
	STDMETHOD(get_SampleRate)(THIS_ int* sample_rate) PURE;
	STDMETHOD(get_Channels)(THIS_ int *channels) PURE;
	STDMETHOD(get_BitsPerSample)(THIS_ int *bits_per_sample) PURE;
	STDMETHOD(get_Bitrate)(THIS_ int *bitrate) PURE;
	STDMETHOD(get_FramesDecoded)(THIS_ unsigned int *frames_decoded) PURE;
	STDMETHOD(get_DownMatrix)(THIS_ bool *down_matrix) PURE;
	STDMETHOD(set_DownMatrix)(THIS_ bool down_matrix) PURE;
};

#ifdef __cplusplus
}
#endif

#endif // _ICoreAAC_H_