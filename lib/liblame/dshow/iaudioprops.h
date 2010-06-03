/*
 *  LAME MP3 encoder for DirectShow
 *  Interface definition
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

// A custom interface to allow the user to modify audio
// encoder properties
#ifndef __IAUDIOPROPERTIES__
#define __IAUDIOPROPERTIES__
#ifdef __cplusplus
extern "C" {
#endif
    // {ca7e9ef0-1cbe-11d3-8d29-00a0c94bbfee}
    DEFINE_GUID(IID_IAudioEncoderProperties, 
    0xca7e9ef0, 0x1cbe, 0x11d3, 0x8d, 0x29, 0x00, 0xa0, 0xc9, 0x4b, 0xbf, 0xee);
    //
    // Configuring MPEG audio encoder parameters with unspecified
    // input stream type may lead to misbehaviour and confusing
    // results. In most cases the specified parameters will be
    // overridden by defaults for the input media type.
    // To archive proper results use this interface on the
    // audio encoder filter with input pin connected to the valid
    // source.
    //
    DECLARE_INTERFACE_(IAudioEncoderProperties, IUnknown)
    {
        // Is PES output enabled? Return TRUE or FALSE
        STDMETHOD(get_PESOutputEnabled) (THIS_
            DWORD *dwEnabled
        ) PURE;
        // Enable/disable PES output
        STDMETHOD(set_PESOutputEnabled) (THIS_
            DWORD dwEnabled
        ) PURE;
        // Get target compression bitrate in Kbits/s
        STDMETHOD(get_Bitrate) (THIS_
            DWORD *dwBitrate
        ) PURE;
        // Set target compression bitrate in Kbits/s
        // Not all numbers available! See spec for details!
        STDMETHOD(set_Bitrate) (THIS_
            DWORD dwBitrate
        ) PURE;
        // Get variable bitrate flag
        STDMETHOD(get_Variable) (THIS_
            DWORD *dwVariable
        ) PURE;
        // Set variable bitrate flag
        STDMETHOD(set_Variable) (THIS_
            DWORD dwVariable
        ) PURE;
        // Get variable bitrate in Kbits/s
        STDMETHOD(get_VariableMin) (THIS_
            DWORD *dwmin
        ) PURE;
        // Set variable bitrate in Kbits/s
        // Not all numbers available! See spec for details!
        STDMETHOD(set_VariableMin) (THIS_
            DWORD dwmin
        ) PURE;
        // Get variable bitrate in Kbits/s
        STDMETHOD(get_VariableMax) (THIS_
            DWORD *dwmax
        ) PURE;
        // Set variable bitrate in Kbits/s
        // Not all numbers available! See spec for details!
        STDMETHOD(set_VariableMax) (THIS_
            DWORD dwmax
        ) PURE;
        // Get compression quality
        STDMETHOD(get_Quality) (THIS_
            DWORD *dwQuality
        ) PURE;
        // Set compression quality
        // Not all numbers available! See spec for details!
        STDMETHOD(set_Quality) (THIS_
            DWORD dwQuality
        ) PURE;
        // Get VBR quality
        STDMETHOD(get_VariableQ) (THIS_
            DWORD *dwVBRq
        ) PURE;
        // Set VBR quality
        // Not all numbers available! See spec for details!
        STDMETHOD(set_VariableQ) (THIS_
            DWORD dwVBRq
        ) PURE;
        // Get source sample rate. Return E_FAIL if input pin
        // in not connected.
        STDMETHOD(get_SourceSampleRate) (THIS_
            DWORD *dwSampleRate
        ) PURE;
        // Get source number of channels. Return E_FAIL if
        // input pin is not connected.
        STDMETHOD(get_SourceChannels) (THIS_
            DWORD *dwChannels
        ) PURE;
        // Get sample rate for compressed audio bitstream
        STDMETHOD(get_SampleRate) (THIS_
            DWORD *dwSampleRate
        ) PURE;
        // Set sample rate. See genaudio spec for details
        STDMETHOD(set_SampleRate) (THIS_
            DWORD dwSampleRate
        ) PURE;
        // Get channel mode. See genaudio.h for details
        STDMETHOD(get_ChannelMode) (THIS_
            DWORD *dwChannelMode
        ) PURE;
        // Set channel mode
        STDMETHOD(set_ChannelMode) (THIS_
            DWORD dwChannelMode
        ) PURE;
        // Is CRC enabled?
        STDMETHOD(get_CRCFlag) (THIS_
            DWORD *dwFlag
        ) PURE;
        // Enable/disable CRC
        STDMETHOD(set_CRCFlag) (THIS_
            DWORD dwFlag
        ) PURE;
        // Force mono
        STDMETHOD(get_ForceMono) (THIS_
            DWORD *dwFlag
        ) PURE;
        // Force mono
        STDMETHOD(set_ForceMono) (THIS_
            DWORD dwFlag
        ) PURE;
        // Set duration
        STDMETHOD(get_SetDuration) (THIS_
            DWORD *dwFlag
        ) PURE;
        // Set duration
        STDMETHOD(set_SetDuration) (THIS_
            DWORD dwFlag
        ) PURE;
        // Control 'original' flag
        STDMETHOD(get_OriginalFlag) (THIS_
            DWORD *dwFlag
        ) PURE;
        STDMETHOD(set_OriginalFlag) (THIS_
            DWORD dwFlag
            ) PURE;
        // Control 'copyright' flag
        STDMETHOD(get_CopyrightFlag) (THIS_
            DWORD *dwFlag
        ) PURE;
        STDMETHOD(set_CopyrightFlag) (THIS_
            DWORD dwFlag
        ) PURE;
        // Control 'Enforce VBR Minimum bitrate' flag
        STDMETHOD(get_EnforceVBRmin) (THIS_
            DWORD *dwFlag
        ) PURE;
        STDMETHOD(set_EnforceVBRmin) (THIS_
            DWORD dwFlag
        ) PURE;
        // Control 'Voice' flag
        STDMETHOD(get_VoiceMode) (THIS_
            DWORD *dwFlag
        ) PURE;
        STDMETHOD(set_VoiceMode) (THIS_
            DWORD dwFlag
        ) PURE;
        // Control 'Keep All Frequencies' flag
        STDMETHOD(get_KeepAllFreq) (THIS_
            DWORD *dwFlag
        ) PURE;
        STDMETHOD(set_KeepAllFreq) (THIS_
            DWORD dwFlag
        ) PURE;
        // Control 'Strict ISO complience' flag
        STDMETHOD(get_StrictISO) (THIS_
            DWORD *dwFlag
        ) PURE;
        STDMETHOD(set_StrictISO) (THIS_
            DWORD dwFlag
        ) PURE;
        // Control 'Disable short block' flag
        STDMETHOD(get_NoShortBlock) (THIS_
            DWORD *dwDisable
        ) PURE;
        STDMETHOD(set_NoShortBlock) (THIS_
            DWORD dwDisable
        ) PURE;
        // Control 'Xing VBR Tag' flag
        STDMETHOD(get_XingTag) (THIS_
            DWORD *dwXingTag
        ) PURE;
        STDMETHOD(set_XingTag) (THIS_
            DWORD dwXingTag
        ) PURE;
        // Control 'Forced mid/ side stereo' flag
        STDMETHOD(get_ForceMS) (THIS_
            DWORD *dwFlag
        ) PURE;
        STDMETHOD(set_ForceMS) (THIS_
            DWORD dwFlag
        ) PURE;
        // Control 'ModeFixed' flag
        STDMETHOD(get_ModeFixed) (THIS_
            DWORD *dwFlag
        ) PURE;
        STDMETHOD(set_ModeFixed) (THIS_
            DWORD dwFlag
        ) PURE;

        //Receive the block of encoder 
        //configuration parametres
        STDMETHOD(get_ParameterBlockSize) (THIS_
            BYTE *pcBlock, DWORD *pdwSize
        ) PURE;
        // Set encoder configuration parametres
        STDMETHOD(set_ParameterBlockSize) (THIS_
            BYTE *pcBlock, DWORD dwSize
        ) PURE;
        // Set default audio encoder parameters depending
        // on current input stream type
        STDMETHOD(DefaultAudioEncoderProperties) (THIS_
        ) PURE;
        // By default the modified properties are not saved to
        // the registry immediately, so the filter needs to be
        // forced to do this. Omitting this step may lead to
        // misbehavior and confusing results.
        STDMETHOD(LoadAudioEncoderPropertiesFromRegistry) (THIS_
        ) PURE;
        STDMETHOD(SaveAudioEncoderPropertiesToRegistry) (THIS_
        ) PURE;
        // Determine whether the filter can be configured. If this
        // function returns E_FAIL then input format hasn't been
        // specified and filter behavior is unpredictable. If S_OK,
        // the filter could be configured with correct values.
        STDMETHOD(InputTypeDefined) (THIS_
        ) PURE;
        // Reconnects output pin (crucial for Fraunhofer MPEG Layer-3 Decoder)
        STDMETHOD(ApplyChanges) (THIS_
        ) PURE;

        // Allow output sample overlap in terms of DirectShow
        // timestamps (i.e. when sample's start time is less
        // than previous sample's end time). Avi Mux doesn't like this
        STDMETHOD(set_SampleOverlap) (THIS_
            DWORD dwFlag
        ) PURE;
        STDMETHOD(get_SampleOverlap) (THIS_
            DWORD *dwFlag
        ) PURE;
    };
#ifdef __cplusplus
}
#endif
#endif // __IAUDIOPROPERTIES__



