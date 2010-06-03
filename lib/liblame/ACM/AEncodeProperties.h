/**
 *
 * Lame ACM wrapper, encode/decode MP3 based RIFF/AVI files in MS Windows
 *
 *  Copyright (c) 2002 Steve Lhomme <steve.lhomme at free.fr>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
 
/*!
	\author Steve Lhomme
	\version \$Id: AEncodeProperties.h,v 1.5 2002/04/07 13:31:35 robux4 Exp $
*/

#if !defined(_AENCODEPROPERTIES_H__INCLUDED_)
#define _AENCODEPROPERTIES_H__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <windows.h>
#include <string>

#include "ADbg/ADbg.h"
//#include "BladeMP3EncDLL.h"
#include "tinyxml/tinyxml.h"
//#include "AParameters/AParameters.h"

typedef const struct {
	UINT id;
	const char *tip;
} ToolTipItem;
/**
  \class AEncodeProperties
  \brief the AEncodeProperties class is responsible for handling all the encoding properties
*/
class AEncodeProperties  
{
public:
	/**
		\brief default constructor

		\param the windows module with which you can retrieve many informations
	*/
	AEncodeProperties(HMODULE hModule);

	/**
		\brief default destructor
	*/
	virtual ~AEncodeProperties() {}

	/**
		\enum BRMode
		\brief A bitrate mode (CBR, VBR, ABR)
	*/
	enum BRMode { BR_CBR, BR_VBR, BR_ABR };

	/**
		\brief Handle all the commands that occur in the Config dialog box
	*/
	bool HandleDialogCommand(const HWND parentWnd, const WPARAM wParam, const LPARAM lParam);
	/**
		\brief check wether 2 instances are equal, ie have the same encoding parameters
	*/
	bool operator != (const AEncodeProperties & the_instance) const;

	/**
		\brief Check wether the Encode process should use the Copyright bit
	*/
	inline const bool GetCopyrightMode() const { return bCopyright; }
	/**
		\brief Check wether the Encode process should use the CRC bit
	*/
	inline const bool GetCRCMode() const { return bCRC; }
	/**
		\brief Check wether the Encode process should use the Original bit
	*/
	inline const bool GetOriginalMode() const { return bOriginal; }
	/**
		\brief Check wether the Encode process should use the Private bit
	*/
	inline const bool GetPrivateMode() const { return bPrivate; }
	/**
		\brief Check wether the Encode process should use the Smart Bitrate output
	*/
	inline const bool GetSmartOutputMode() const { return bSmartOutput; }
	/**
		\brief Check wether the Encode process should allow Average Bitrate output
	*/
	inline const bool GetAbrOutputMode() const { return bAbrOutput; }

	/**
		\brief Check wether the Encode process shouldn't use the Bit Reservoir
	*/
	inline const bool GetNoBiResMode() const { return bNoBitRes; }

	/**
		\brief Check wether the Encode process should force the channel mode (stereo or mono resampling)
	*/
	inline const bool GetForceChannelMode() const { return bForceChannel; }

	/**
		\brief Check wether the Encode process should use the VBR mode
	*/
	inline const BRMode GetVBRUseMode() const { return mBRmode; }
	/**
		\brief Check wether the Encode process should use the Xing frame in the VBR mode
		\note the Xing frame is a silent frame at the beginning that contain VBR statistics about the file.
	*/
	inline const bool GetXingFrameMode() const { return bXingFrame; }

	/**
		\brief Check wether the Encode process should resample before encoding
	*/
	inline const bool GetResampleMode() const { return bResample; }
	
	/**
		\brief Set wether the Encode process should use the Copyright bit
	*/
	inline void SetCopyrightMode(const bool bMode) { bCopyright = bMode; }
	/**
		\brief Set wether the Encode process should use the CRC bit
	*/
	inline void SetCRCMode(const bool bMode) { bCRC = bMode; }
	/**
		\brief Set wether the Encode process should use the Original bit
	*/
	inline void SetOriginalMode(const bool bMode) { bOriginal = bMode; }
	/**
		\brief Set wether the Encode process should use the Private bit
	*/
	inline void SetPrivateMode(const bool bMode) { bPrivate = bMode; }

	/**
		\brief Set wether the Encode process should use the Smart Bitrate output
	*/
	inline void SetSmartOutputMode(const bool bMode) { bSmartOutput = bMode; }
	/**
		\brief Set wether the Encode process should use the Average Bitrate output
	*/
	inline void SetAbrOutputMode(const bool bMode) { bAbrOutput = bMode; }


	/**
		\brief Set wether the Encode process shouldn't use the Bit Reservoir
	*/
	inline void SetNoBiResMode(const bool bMode) { bNoBitRes = bMode; }
	
	/**
		\brief Set wether the Encode process should force the channel mode (stereo or mono resampling)
	*/
	inline void SetForceChannelMode(const bool bMode) { bForceChannel = bMode; }
	
	/**
		\brief Set wether the Encode process should use the VBR mode
	*/
	inline void SetVBRUseMode(const BRMode mode) { mBRmode = mode; }

	/**
		\brief Set wether the Encode process should use the Xing frame in the VBR mode
		\note the Xing frame is a silent frame at the beginning that contain VBR statistics about the file.
	*/
	inline void SetXingFrameMode(const bool bMode) { bXingFrame = bMode; }

	/**
		\brief CBR : Get the bitrate to use         / 
		       VBR : Get the minimum bitrate value
	*/
	const unsigned int GetBitrateValue() const;

	/**
		\brief Get the current (VBR:min) bitrate for the specified MPEG version

		\param bitrate the data that will be filled with the bitrate
		\param MPEG_Version The MPEG version (MPEG1 or MPEG2)

		\return 0 if the bitrate is not found, 1 if the bitrate is found
	*/
	const int GetBitrateValue(DWORD & bitrate, const DWORD MPEG_Version) const;
	/**
		\brief Get the current (VBR:min) bitrate for MPEG I

		\param bitrate the data that will be filled with the bitrate

		\return 0 if the bitrate is not found, 1 if the bitrate is found
	*/
	const int GetBitrateValueMPEG1(DWORD & bitrate) const;
	/**
		\brief Get the current (VBR:min) bitrate for MPEG II

		\param bitrate the data that will be filled with the bitrate

		\return 0 if the bitrate is not found, 1 if the bitrate is found
	*/
	const int GetBitrateValueMPEG2(DWORD & bitrate) const;

	/**
		\brief Get the current (VBR:min) bitrate in the form of a string

		\param string the string that will be filled
		\param string_size the size of the string

		\return -1 if the bitrate is not found, and the number of char copied otherwise
	*/
	inline const int GetBitrateString(char * string, int string_size) const {return GetBitrateString(string,string_size,nMinBitrateIndex); }

	/**
		\brief Get the (VBR:min) bitrate corresponding to the specified index in the form of a string

		\param string the string that will be filled
		\param string_size the size of the string
		\param a_bitrateID the index in the Bitrate table

		\return -1 if the bitrate is not found, and the number of char copied otherwise
	*/
	const int GetBitrateString(char * string, int string_size, int a_bitrateID) const;

	/**
		\brief Get the number of possible bitrates
	*/
	inline const int GetBitrateLentgh() const { return sizeof(the_Bitrates) / sizeof(unsigned int); }
	/**
		\brief Get the number of possible sampling frequencies
	*/
	inline const unsigned int GetResampleFreq() const { return the_SamplingFreqs[nSamplingFreqIndex]; }
	/**
		\brief Get the max compression ratio allowed (1:15 default)
	*/
	inline double GetSmartRatio() const { return SmartRatioMax;}
	/**
		\brief Get the min ABR bitrate possible
	*/
	inline unsigned int GetAbrBitrateMin() const { return AverageBitrate_Min;}
	/**
		\brief Get the max ABR bitrate possible
	*/
	inline unsigned int GetAbrBitrateMax() const { return AverageBitrate_Max;}
	/**
		\brief Get the step between ABR bitrates
	*/
	inline unsigned int GetAbrBitrateStep() const { return AverageBitrate_Step;}

	/**
		\brief Get the VBR attributes for a specified MPEG version

		\param MaxBitrate receive the maximum bitrate possible in the VBR mode
		\param Quality receive the quality value (0 to 9 see Lame doc for more info)
		\param VBRHeader receive the value that indicates wether the VBR/Xing header should be filled
		\param MPEG_Version The MPEG version (MPEG1 or MPEG2)

		\return the VBR mode (Old, New, ABR, MTRH, Default or None)
	*/
//	VBRMETHOD GetVBRValue(DWORD & MaxBitrate, int & Quality, DWORD & AbrBitrate, BOOL & VBRHeader, const DWORD MPEG_Version) const;

	/**
		\brief Get the Lame DLL Location
	*/
//	const char * GetDllLocation() const { return DllLocation.c_str(); }
	/**
		\brief Set the Lame DLL Location
	*/
//	void SetDllLocation( const char * the_string ) { DllLocation = the_string; }

	/**
		\brief Get the output directory for encoding
	*/
//	const char * GetOutputDirectory() const { return OutputDir.c_str(); }
	/**
		\brief Set the output directory for encoding
	*/
//	void SetOutputDirectory( const char * the_string ) { OutputDir = the_string; }

	/**
		\brief Get the current channel mode to use
	*/
	const unsigned int GetChannelModeValue() const;
	/**
		\brief Get the current channel mode in the form of a string
	*/
	inline const char * GetChannelModeString() const {return GetChannelModeString(nChannelIndex); }
	/**
		\brief Get the channel mode in the form of a string for the specified Channel mode index

		\param a_channelID the Channel mode index (see GetChannelLentgh())
	*/
	const char * GetChannelModeString(const int a_channelID) const;
	/**
		\brief Get the number of possible channel mode
	*/
	inline const int GetChannelLentgh() const { return 3; }

	/**
		\brief Get the current preset to use, see lame documentation/code for more info on the possible presets
	*/
//	const LAME_QUALTIY_PRESET GetPresetModeValue() const;
	/**
		\brief Get the preset in the form of a string for the specified Channel mode index

		\param a_presetID the preset index (see GetPresetLentgh())
	*/
	const char * GetPresetModeString(const int a_presetID) const;
	/**
		\brief Get the number of possible presets
	*/
//	inline const int GetPresetLentgh() const { return sizeof(the_Presets) / sizeof(LAME_QUALTIY_PRESET); }

	/**
		\brief Start the user configuration process (called by AOut::config())
	*/
	bool Config(const HINSTANCE hInstance, const HWND HwndParent);

	/**
		\brief Init the config dialog box with the right texts and choices
	*/
	bool InitConfigDlg(HWND hDialog);

	/**
		\brief Update the instance parameters from the config dialog box
	*/
	bool UpdateValueFromDlg(HWND hDialog);
	/**
		\brief Update the config dialog box from the instance parameters
	*/
	bool UpdateDlgFromValue(HWND hDialog);

	/**
		\brief Update the config dialog box with the BitRate mode
	*/
	static void DisplayVbrOptions(const HWND hDialog, const BRMode the_mode);

	/**
		\brief Handle the saving of parameters when something has changed in the config dialog box
	*/
	void SaveParams(const HWND hDialog);

	/**
		\brief Save the current parameters (current config in use)
	*/
	void ParamsSave(void);
	/**
		\brief Load the parameters (current config in use)
	*/
	void ParamsRestore(void);

	/**
		\brief Select the specified config name as the new default one
	*/
	void SelectSavedParams(const std::string config_name);
	/**
		\brief Save the current parameters to the specified config name
	*/
	void SaveValuesToStringKey(const std::string & config_name);
	/**
		\brief Rename the current config name to something else
	*/
	bool RenameCurrentTo(const std::string & new_config_name);
	/**
		\brief Delete the config name from the saved configs
	*/
	bool DeleteConfig(const std::string & config_name);

	ADbg              my_debug;

	/**
		\brief Update the slides value (on scroll)
	*/
	void UpdateDlgFromSlides(HWND parent_window) const;

	static ToolTipItem Tooltips[13];
private:

	bool bCopyright;
	bool bCRC;
	bool bOriginal;
	bool bPrivate;
	bool bNoBitRes;
	BRMode mBRmode;
	bool bXingFrame;
	bool bForceChannel;
	bool bResample;
	bool bSmartOutput;
	bool bAbrOutput;

	int VbrQuality;
	unsigned int AverageBitrate_Min;
	unsigned int AverageBitrate_Max;
	unsigned int AverageBitrate_Step;

	double SmartRatioMax;

	static const unsigned int the_ChannelModes[3];
	int nChannelIndex;

	static const unsigned int the_Bitrates[18];
	static const unsigned int the_MPEG1_Bitrates[14];
	static const unsigned int the_MPEG2_Bitrates[14];
	int nMinBitrateIndex; // CBR and VBR
	int nMaxBitrateIndex; // only used in VBR mode

	static const unsigned int the_SamplingFreqs[9];
	int nSamplingFreqIndex;

//	static const LAME_QUALTIY_PRESET the_Presets[17];
	int nPresetIndex;

//	char DllLocation[512];
//	std::string DllLocation;
//	char OutputDir[MAX_PATH];
//	std::string OutputDir;

//	AParameters my_base_parameters;
	TiXmlDocument my_stored_data;
	std::string my_store_location;
	std::string my_current_config;

//	HINSTANCE hDllInstance;

	void SaveValuesToElement(TiXmlElement * the_element) const;
	inline void SetAttributeBool(TiXmlElement * the_elt,const std::string & the_string, const bool the_value) const;
	void UpdateConfigs(const HWND HwndDlg);
	void EnableAbrOptions(HWND hDialog, bool enable);

	HMODULE my_hModule;

	/**
		\brief

		\param config_name
		\param parentNode
	*/
	void GetValuesFromKey(const std::string & config_name, const TiXmlNode & parentNode);
};

#endif // !defined(_AENCODEPROPERTIES_H__INCLUDED_)
