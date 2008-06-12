//-----------------------------------------------------------------------------
#ifndef _COREAAC_H_
#define _COREAAC_H_
// ----------------------------------------------------------------------------

// {6AC7C19E-8CA0-4e3d-9A9F-2881DE29E0AC}
DEFINE_GUID(CLSID_DECODER,
			0x6ac7c19e, 0x8ca0, 0x4e3d, 0x9a, 0x9f, 0x28, 0x81, 0xde, 0x29, 0xe0, 0xac);

// Be compatible with 3ivx
#define WAVE_FORMAT_AAC 0x00FF
//#define WAVE_FORMAT_AAC 0xAAC0

// {000000FF-0000-0010-8000-00AA00389B71}
DEFINE_GUID(MEDIASUBTYPE_AAC,
			WAVE_FORMAT_AAC, 0x000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);

// {4665E44B-8B9A-4515-A086-E94ECE374608}
DEFINE_GUID(CLSID_CoreAACAboutProp,
			0x4665e44b, 0x8b9a, 0x4515, 0xa0, 0x86, 0xe9, 0x4e, 0xce, 0x37, 0x46, 0x8);

// {BBFC1A2A-D3A2-4610-847D-26592022F86E}
DEFINE_GUID(CLSID_CoreAACInfoProp, 
			0xbbfc1a2a, 0xd3a2, 0x4610, 0x84, 0x7d, 0x26, 0x59, 0x20, 0x22, 0xf8, 0x6e);

// ----------------------------------------------------------------------------
#endif // _COREAAC_H_
// ----------------------------------------------------------------------------