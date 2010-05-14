/*
 *  LAME MP3 encoder for DirectShow
 *  DirectShow filter implementation
 *
 *  Copyright (c) 2000-2005 Marie Orlova, Peter Gubanov, Vitaly Ivanov, Elecard Ltd.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <mmreg.h>
#include "Encoder.h"

#define KEY_LAME_ENCODER            "SOFTWARE\\GNU\\LAME MPEG Layer III Audio Encoder Filter"

#define VALUE_BITRATE               "Bitrate"
#define VALUE_VARIABLE              "Variable"
#define VALUE_VARIABLEMIN           "VariableMin"
#define VALUE_VARIABLEMAX           "VariableMax"
#define VALUE_QUALITY               "Quality"
#define VALUE_VBR_QUALITY           "VBR Quality"
#define VALUE_SAMPLE_RATE           "Sample Rate"

#define VALUE_STEREO_MODE           "Stereo Mode"
#define VALUE_FORCE_MS              "Force MS"

#define VALUE_LAYER                 "Layer"
#define VALUE_ORIGINAL              "Original"
#define VALUE_COPYRIGHT             "Copyright"
#define VALUE_CRC                   "CRC"
#define VALUE_FORCE_MONO            "Force Mono"
#define VALUE_SET_DURATION          "Set Duration"
#define VALUE_SAMPLE_OVERLAP        "Allow sample overlap"
#define VALUE_PES                   "PES"

#define VALUE_ENFORCE_MIN           "EnforceVBRmin"
#define VALUE_VOICE                 "Voice Mode"
#define VALUE_KEEP_ALL_FREQ         "Keep All Frequencies"
#define VALUE_STRICT_ISO            "Strict ISO"
#define VALUE_DISABLE_SHORT_BLOCK   "No Short Block"
#define VALUE_XING_TAG              "Xing Tag"
#define VALUE_MODE_FIXED            "Mode Fixed"


typedef struct 
{
    DWORD      nSampleRate;
    DWORD      nBitRate;
    MPEG_mode  ChMode;                       //Channel coding mode
} current_output_format_t;

typedef struct 
{
    DWORD   nSampleRate;
    DWORD   nBitRate;
} output_caps_t;

typedef struct
{
    LONGLONG        sample;
    REFERENCE_TIME  delta;

    BOOL            applied;
} resync_point_t;

#define RESYNC_COUNT    4

// The maximum number of capabilities that we can expose in our IAMStreamConfig
// implementation is currently set to 100. This number is larger than we
// should ever realistically need. However, a cleaner implementation might
// be to use a dynamically sized array like std::vector or CAtlArray to 
// hold this data.
#define MAX_IAMSTREAMCONFIG_CAPS 100

///////////////////////////////////////////////////////////////////
// CMpegAudEnc class - implementation for ITransformFilter interface
///////////////////////////////////////////////////////////////////
class CMpegAudEncOutPin;
class CMpegAudEncPropertyPage;
class CMpegAudEnc : public CTransformFilter,
                    public ISpecifyPropertyPages,
                    public IAudioEncoderProperties,
                    public CPersistStream
{
public:
    DECLARE_IUNKNOWN

    static CUnknown *CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);

    LPAMOVIESETUP_FILTER GetSetupData();

    HRESULT Reconnect();

    HRESULT Receive(IMediaSample *pSample);

    HRESULT CheckInputType(const CMediaType* mtIn);
    HRESULT CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut);
    HRESULT DecideBufferSize(IMemAllocator * pAllocator, ALLOCATOR_PROPERTIES *pprop);

    HRESULT GetMediaType  (int iPosition, CMediaType *pMediaType);
    HRESULT SetMediaType  (PIN_DIRECTION direction,const CMediaType *pmt);


    //
    HRESULT StartStreaming();
    HRESULT StopStreaming();
    HRESULT EndOfStream();
    HRESULT BeginFlush();

    ~CMpegAudEnc(void);

    // ISpecifyPropertyPages
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);
    STDMETHODIMP GetPages(CAUUID *pPages);

    // IAudioEncoderProperties
    STDMETHODIMP get_PESOutputEnabled(DWORD *dwEnabled);    // PES header. Obsolete
    STDMETHODIMP set_PESOutputEnabled(DWORD dwEnabled);     // PES header. Obsolete

    STDMETHODIMP get_MPEGLayer(DWORD *dwLayer);
    STDMETHODIMP set_MPEGLayer(DWORD dwLayer);

    STDMETHODIMP get_Bitrate(DWORD *dwBitrate);
    STDMETHODIMP set_Bitrate(DWORD dwBitrate);   
    STDMETHODIMP get_Variable(DWORD *dwVariable);
    STDMETHODIMP set_Variable(DWORD dwVariable);
    STDMETHODIMP get_VariableMin(DWORD *dwMin);
    STDMETHODIMP set_VariableMin(DWORD dwMin);
    STDMETHODIMP get_VariableMax(DWORD *dwMax);
    STDMETHODIMP set_VariableMax(DWORD dwMax);
    STDMETHODIMP get_Quality(DWORD *dwQuality);
    STDMETHODIMP set_Quality(DWORD dwQuality);
    STDMETHODIMP get_VariableQ(DWORD *dwVBRq);
    STDMETHODIMP set_VariableQ(DWORD dwVBRq);
    STDMETHODIMP get_SourceSampleRate(DWORD *dwSampleRate);
    STDMETHODIMP get_SourceChannels(DWORD *dwChannels);
    STDMETHODIMP get_SampleRate(DWORD *dwSampleRate);
    STDMETHODIMP set_SampleRate(DWORD dwSampleRate);

    STDMETHODIMP get_ChannelMode(DWORD *dwChannelMode);
    STDMETHODIMP set_ChannelMode(DWORD dwChannelMode);
    STDMETHODIMP get_ForceMS(DWORD *dwFlag);
    STDMETHODIMP set_ForceMS(DWORD dwFlag);
    STDMETHODIMP get_EnforceVBRmin(DWORD *dwFlag);
    STDMETHODIMP set_EnforceVBRmin(DWORD dwFlag);
    STDMETHODIMP get_VoiceMode(DWORD *dwFlag);
    STDMETHODIMP set_VoiceMode(DWORD dwFlag);
    STDMETHODIMP get_KeepAllFreq(DWORD *dwFlag);
    STDMETHODIMP set_KeepAllFreq(DWORD dwFlag);
    STDMETHODIMP get_StrictISO(DWORD *dwFlag);
    STDMETHODIMP set_StrictISO(DWORD dwFlag);
    STDMETHODIMP get_NoShortBlock(DWORD *dwNoShortBlock);
    STDMETHODIMP set_NoShortBlock(DWORD dwNoShortBlock);
    STDMETHODIMP get_XingTag(DWORD *dwXingTag);
    STDMETHODIMP set_XingTag(DWORD dwXingTag);
    STDMETHODIMP get_ModeFixed(DWORD *dwModeFixed);
    STDMETHODIMP set_ModeFixed(DWORD dwModeFixed);


    STDMETHODIMP get_CRCFlag(DWORD *dwFlag);
    STDMETHODIMP set_CRCFlag(DWORD dwFlag);
    STDMETHODIMP get_ForceMono(DWORD *dwFlag);
    STDMETHODIMP set_ForceMono(DWORD dwFlag);
    STDMETHODIMP get_SetDuration(DWORD *dwFlag);
    STDMETHODIMP set_SetDuration(DWORD dwFlag);
    STDMETHODIMP get_OriginalFlag(DWORD *dwFlag);
    STDMETHODIMP set_OriginalFlag(DWORD dwFlag);
    STDMETHODIMP get_CopyrightFlag(DWORD *dwFlag);
    STDMETHODIMP set_CopyrightFlag(DWORD dwFlag);
    STDMETHODIMP get_SampleOverlap(DWORD *dwFlag);
    STDMETHODIMP set_SampleOverlap(DWORD dwFlag);

    STDMETHODIMP get_ParameterBlockSize(BYTE *pcBlock, DWORD *pdwSize);
    STDMETHODIMP set_ParameterBlockSize(BYTE *pcBlock, DWORD dwSize);

    STDMETHODIMP DefaultAudioEncoderProperties();
    STDMETHODIMP LoadAudioEncoderPropertiesFromRegistry();
    STDMETHODIMP SaveAudioEncoderPropertiesToRegistry();
    STDMETHODIMP InputTypeDefined();

    STDMETHODIMP ApplyChanges();

    // CPersistStream
    HRESULT WriteToStream(IStream *pStream);
    HRESULT ReadFromStream(IStream *pStream);

    int SizeMax();
    STDMETHODIMP GetClassID(CLSID *pClsid);

private:
    CMpegAudEnc(LPUNKNOWN lpunk, HRESULT *phr);

    HRESULT FlushEncodedSamples();

    void ReadPresetSettings(MPEG_ENCODER_CONFIG *pmabsi);

    void LoadOutputCapabilities(DWORD sample_rate);

    // Encoder object
    CEncoder                    m_Encoder;

    REFERENCE_TIME              m_rtStreamTime;
    REFERENCE_TIME              m_rtFrameTime;
    REFERENCE_TIME              m_rtEstimated;

    // Synchronization data
    LONGLONG                    m_samplesIn;
    LONGLONG                    m_samplesOut;
    int                         m_samplesPerFrame;
    int                         m_bytesPerSample;
    float                       m_bytesToDuration;

    resync_point_t              m_sync[RESYNC_COUNT];
    int                         m_sync_in_idx;
    int                         m_sync_out_idx;

    BOOL                        m_hasFinished;

    CCritSec                    m_cs;

    DWORD                       m_setDuration;
    DWORD                       m_allowOverlap;

	REFERENCE_TIME m_rtBytePos;

	BOOL						m_bStreamOutput;      // Binary stream output
	long						m_cbStreamAlignment;  // Stream block size
    int                         m_CapsNum;
	int                         m_currentMediaTypeIndex;
    output_caps_t               OutputCaps[MAX_IAMSTREAMCONFIG_CAPS];

protected:
    friend class CMpegAudEncOutPin;
    friend class CMpegAudEncPropertyPage;
};


class CMpegAudEncOutPin : public CTransformOutputPin, public IAMStreamConfig
{
public:

    //////////////////////////////////////////////////////////////////////////
    //  IUnknown
    //////////////////////////////////////////////////////////////////////////
    DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

    //////////////////////////////////////////////////////////////////////////
    //  IAMStreamConfig
    //////////////////////////////////////////////////////////////////////////
    HRESULT STDMETHODCALLTYPE SetFormat(AM_MEDIA_TYPE *pmt);
    HRESULT STDMETHODCALLTYPE GetFormat(AM_MEDIA_TYPE **ppmt);
    HRESULT STDMETHODCALLTYPE GetNumberOfCapabilities(int *piCount, int *piSize);
    HRESULT STDMETHODCALLTYPE GetStreamCaps(int iIndex, AM_MEDIA_TYPE **pmt, BYTE *pSCC);

    //////////////////////////////////////////////////////////////////////////
    //  CTransformOutputPin
    //////////////////////////////////////////////////////////////////////////
    CMpegAudEncOutPin( CMpegAudEnc * pFilter, HRESULT * pHr );
    ~CMpegAudEncOutPin();

    HRESULT CheckMediaType(const CMediaType *pmtOut);
    HRESULT GetMediaType(int iPosition, CMediaType *pmt);
    HRESULT SetMediaType(const CMediaType *pmt);
    
private:
    BOOL        m_SetFormat;
    CMpegAudEnc *m_pFilter;

    current_output_format_t  m_CurrentOutputFormat;

protected:
    friend class CMpegAudEnc;

};
