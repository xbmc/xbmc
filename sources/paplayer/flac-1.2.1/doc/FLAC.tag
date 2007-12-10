<?xml version='1.0' encoding='ISO-8859-1' standalone='yes' ?>
<tagfile>
  <compound kind="page">
    <name>index</name>
    <title></title>
    <filename>index</filename>
    <docanchor file="index">cpp_api</docanchor>
    <docanchor file="index">intro</docanchor>
    <docanchor file="index">getting_started</docanchor>
    <docanchor file="index">c_api</docanchor>
    <docanchor file="index">porting_guide</docanchor>
    <docanchor file="index">embedded_developers</docanchor>
  </compound>
  <compound kind="file">
    <name>decoder.h</name>
    <path>/home/jcoalson/flac/build-1.2.1/include/FLAC++/</path>
    <filename>decoder_8h</filename>
    <includes id="+_2export_8h" name="export.h" local="yes" imported="no">export.h</includes>
    <includes id="stream__decoder_8h" name="stream_decoder.h" local="yes" imported="no">FLAC/stream_decoder.h</includes>
  </compound>
  <compound kind="file">
    <name>encoder.h</name>
    <path>/home/jcoalson/flac/build-1.2.1/include/FLAC++/</path>
    <filename>encoder_8h</filename>
    <includes id="+_2export_8h" name="export.h" local="yes" imported="no">export.h</includes>
    <includes id="stream__encoder_8h" name="stream_encoder.h" local="yes" imported="no">FLAC/stream_encoder.h</includes>
    <includes id="decoder_8h" name="decoder.h" local="yes" imported="no">decoder.h</includes>
    <includes id="+_2metadata_8h" name="metadata.h" local="yes" imported="no">metadata.h</includes>
  </compound>
  <compound kind="file">
    <name>callback.h</name>
    <path>/home/jcoalson/flac/build-1.2.1/include/FLAC/</path>
    <filename>callback_8h</filename>
    <member kind="typedef">
      <type>void *</type>
      <name>FLAC__IOHandle</name>
      <anchorfile>group__flac__callbacks.html</anchorfile>
      <anchor>ga0</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>size_t(*</type>
      <name>FLAC__IOCallback_Read</name>
      <anchorfile>group__flac__callbacks.html</anchorfile>
      <anchor>ga1</anchor>
      <arglist>)(void *ptr, size_t size, size_t nmemb, FLAC__IOHandle handle)</arglist>
    </member>
    <member kind="typedef">
      <type>size_t(*</type>
      <name>FLAC__IOCallback_Write</name>
      <anchorfile>group__flac__callbacks.html</anchorfile>
      <anchor>ga2</anchor>
      <arglist>)(const void *ptr, size_t size, size_t nmemb, FLAC__IOHandle handle)</arglist>
    </member>
    <member kind="typedef">
      <type>int(*</type>
      <name>FLAC__IOCallback_Seek</name>
      <anchorfile>group__flac__callbacks.html</anchorfile>
      <anchor>ga3</anchor>
      <arglist>)(FLAC__IOHandle handle, FLAC__int64 offset, int whence)</arglist>
    </member>
    <member kind="typedef">
      <type>FLAC__int64(*</type>
      <name>FLAC__IOCallback_Tell</name>
      <anchorfile>group__flac__callbacks.html</anchorfile>
      <anchor>ga4</anchor>
      <arglist>)(FLAC__IOHandle handle)</arglist>
    </member>
    <member kind="typedef">
      <type>int(*</type>
      <name>FLAC__IOCallback_Eof</name>
      <anchorfile>group__flac__callbacks.html</anchorfile>
      <anchor>ga5</anchor>
      <arglist>)(FLAC__IOHandle handle)</arglist>
    </member>
    <member kind="typedef">
      <type>int(*</type>
      <name>FLAC__IOCallback_Close</name>
      <anchorfile>group__flac__callbacks.html</anchorfile>
      <anchor>ga6</anchor>
      <arglist>)(FLAC__IOHandle handle)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>export.h</name>
    <path>/home/jcoalson/flac/build-1.2.1/include/FLAC/</path>
    <filename>export_8h</filename>
    <member kind="define">
      <type>#define</type>
      <name>FLAC_API</name>
      <anchorfile>group__flac__export.html</anchorfile>
      <anchor>ga1</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FLAC_API_VERSION_CURRENT</name>
      <anchorfile>group__flac__export.html</anchorfile>
      <anchor>ga2</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FLAC_API_VERSION_REVISION</name>
      <anchorfile>group__flac__export.html</anchorfile>
      <anchor>ga3</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FLAC_API_VERSION_AGE</name>
      <anchorfile>group__flac__export.html</anchorfile>
      <anchor>ga4</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>FLAC_API_SUPPORTS_OGG_FLAC</name>
      <anchorfile>group__flac__export.html</anchorfile>
      <anchor>ga0</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>export.h</name>
    <path>/home/jcoalson/flac/build-1.2.1/include/FLAC++/</path>
    <filename>+_2export_8h</filename>
    <member kind="define">
      <type>#define</type>
      <name>FLACPP_API</name>
      <anchorfile>group__flacpp__export.html</anchorfile>
      <anchor>ga0</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FLACPP_API_VERSION_CURRENT</name>
      <anchorfile>group__flacpp__export.html</anchorfile>
      <anchor>ga1</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FLACPP_API_VERSION_REVISION</name>
      <anchorfile>group__flacpp__export.html</anchorfile>
      <anchor>ga2</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FLACPP_API_VERSION_AGE</name>
      <anchorfile>group__flacpp__export.html</anchorfile>
      <anchor>ga3</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>format.h</name>
    <path>/home/jcoalson/flac/build-1.2.1/include/FLAC/</path>
    <filename>format_8h</filename>
    <includes id="export_8h" name="export.h" local="yes" imported="no">export.h</includes>
    <member kind="define">
      <type>#define</type>
      <name>FLAC__MAX_METADATA_TYPE_CODE</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga89</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FLAC__MIN_BLOCK_SIZE</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga90</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FLAC__MAX_BLOCK_SIZE</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga91</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FLAC__SUBSET_MAX_BLOCK_SIZE_48000HZ</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga92</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FLAC__MAX_CHANNELS</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga93</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FLAC__MIN_BITS_PER_SAMPLE</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga94</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FLAC__MAX_BITS_PER_SAMPLE</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga95</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FLAC__REFERENCE_CODEC_MAX_BITS_PER_SAMPLE</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga96</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FLAC__MAX_SAMPLE_RATE</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga97</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FLAC__MAX_LPC_ORDER</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga98</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FLAC__SUBSET_MAX_LPC_ORDER_48000HZ</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga99</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FLAC__MIN_QLP_COEFF_PRECISION</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga100</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FLAC__MAX_QLP_COEFF_PRECISION</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga101</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FLAC__MAX_FIXED_ORDER</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga102</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FLAC__MAX_RICE_PARTITION_ORDER</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga103</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FLAC__SUBSET_MAX_RICE_PARTITION_ORDER</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga104</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FLAC__STREAM_SYNC_LENGTH</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga105</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FLAC__STREAM_METADATA_STREAMINFO_LENGTH</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga106</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FLAC__STREAM_METADATA_SEEKPOINT_LENGTH</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga107</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FLAC__STREAM_METADATA_HEADER_LENGTH</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga108</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <name>FLAC__EntropyCodingMethodType</name>
      <anchor>ga109</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE</name>
      <anchor>gga109a100</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE2</name>
      <anchor>gga109a101</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <name>FLAC__SubframeType</name>
      <anchor>ga110</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__SUBFRAME_TYPE_CONSTANT</name>
      <anchor>gga110a102</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__SUBFRAME_TYPE_VERBATIM</name>
      <anchor>gga110a103</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__SUBFRAME_TYPE_FIXED</name>
      <anchor>gga110a104</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__SUBFRAME_TYPE_LPC</name>
      <anchor>gga110a105</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <name>FLAC__ChannelAssignment</name>
      <anchor>ga111</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__CHANNEL_ASSIGNMENT_INDEPENDENT</name>
      <anchor>gga111a106</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__CHANNEL_ASSIGNMENT_LEFT_SIDE</name>
      <anchor>gga111a107</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__CHANNEL_ASSIGNMENT_RIGHT_SIDE</name>
      <anchor>gga111a108</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__CHANNEL_ASSIGNMENT_MID_SIDE</name>
      <anchor>gga111a109</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <name>FLAC__FrameNumberType</name>
      <anchor>ga112</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__FRAME_NUMBER_TYPE_FRAME_NUMBER</name>
      <anchor>gga112a110</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__FRAME_NUMBER_TYPE_SAMPLE_NUMBER</name>
      <anchor>gga112a111</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <name>FLAC__MetadataType</name>
      <anchor>ga113</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_TYPE_STREAMINFO</name>
      <anchor>gga113a112</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_TYPE_PADDING</name>
      <anchor>gga113a113</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_TYPE_APPLICATION</name>
      <anchor>gga113a114</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_TYPE_SEEKTABLE</name>
      <anchor>gga113a115</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_TYPE_VORBIS_COMMENT</name>
      <anchor>gga113a116</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_TYPE_CUESHEET</name>
      <anchor>gga113a117</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_TYPE_PICTURE</name>
      <anchor>gga113a118</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_TYPE_UNDEFINED</name>
      <anchor>gga113a119</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <name>FLAC__StreamMetadata_Picture_Type</name>
      <anchor>ga114</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_METADATA_PICTURE_TYPE_OTHER</name>
      <anchor>gga114a120</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_METADATA_PICTURE_TYPE_FILE_ICON_STANDARD</name>
      <anchor>gga114a121</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_METADATA_PICTURE_TYPE_FILE_ICON</name>
      <anchor>gga114a122</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_METADATA_PICTURE_TYPE_FRONT_COVER</name>
      <anchor>gga114a123</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_METADATA_PICTURE_TYPE_BACK_COVER</name>
      <anchor>gga114a124</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_METADATA_PICTURE_TYPE_LEAFLET_PAGE</name>
      <anchor>gga114a125</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_METADATA_PICTURE_TYPE_MEDIA</name>
      <anchor>gga114a126</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_METADATA_PICTURE_TYPE_LEAD_ARTIST</name>
      <anchor>gga114a127</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_METADATA_PICTURE_TYPE_ARTIST</name>
      <anchor>gga114a128</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_METADATA_PICTURE_TYPE_CONDUCTOR</name>
      <anchor>gga114a129</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_METADATA_PICTURE_TYPE_BAND</name>
      <anchor>gga114a130</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_METADATA_PICTURE_TYPE_COMPOSER</name>
      <anchor>gga114a131</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_METADATA_PICTURE_TYPE_LYRICIST</name>
      <anchor>gga114a132</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_METADATA_PICTURE_TYPE_RECORDING_LOCATION</name>
      <anchor>gga114a133</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_METADATA_PICTURE_TYPE_DURING_RECORDING</name>
      <anchor>gga114a134</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_METADATA_PICTURE_TYPE_DURING_PERFORMANCE</name>
      <anchor>gga114a135</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_METADATA_PICTURE_TYPE_VIDEO_SCREEN_CAPTURE</name>
      <anchor>gga114a136</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_METADATA_PICTURE_TYPE_FISH</name>
      <anchor>gga114a137</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_METADATA_PICTURE_TYPE_ILLUSTRATION</name>
      <anchor>gga114a138</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_METADATA_PICTURE_TYPE_BAND_LOGOTYPE</name>
      <anchor>gga114a139</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_METADATA_PICTURE_TYPE_PUBLISHER_LOGOTYPE</name>
      <anchor>gga114a140</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__format_sample_rate_is_valid</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga80</anchor>
      <arglist>(unsigned sample_rate)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__format_sample_rate_is_subset</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga81</anchor>
      <arglist>(unsigned sample_rate)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__format_vorbiscomment_entry_name_is_legal</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga82</anchor>
      <arglist>(const char *name)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__format_vorbiscomment_entry_value_is_legal</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga83</anchor>
      <arglist>(const FLAC__byte *value, unsigned length)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__format_vorbiscomment_entry_is_legal</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga84</anchor>
      <arglist>(const FLAC__byte *entry, unsigned length)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__format_seektable_is_legal</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga85</anchor>
      <arglist>(const FLAC__StreamMetadata_SeekTable *seek_table)</arglist>
    </member>
    <member kind="function">
      <type>unsigned</type>
      <name>FLAC__format_seektable_sort</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga86</anchor>
      <arglist>(FLAC__StreamMetadata_SeekTable *seek_table)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__format_cuesheet_is_legal</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga87</anchor>
      <arglist>(const FLAC__StreamMetadata_CueSheet *cue_sheet, FLAC__bool check_cd_da_subset, const char **violation)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__format_picture_is_legal</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga88</anchor>
      <arglist>(const FLAC__StreamMetadata_Picture *picture, const char **violation)</arglist>
    </member>
    <member kind="variable">
      <type>const char *</type>
      <name>FLAC__VERSION_STRING</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga0</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const char *</type>
      <name>FLAC__VENDOR_STRING</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga1</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const FLAC__byte</type>
      <name>FLAC__STREAM_SYNC_STRING</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga2</anchor>
      <arglist>[4]</arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_SYNC</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga3</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_SYNC_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga4</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const char *const</type>
      <name>FLAC__EntropyCodingMethodTypeString</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga5</anchor>
      <arglist>[]</arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_ORDER_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga6</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_PARAMETER_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga7</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE2_PARAMETER_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga8</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_RAW_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga9</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_ESCAPE_PARAMETER</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga10</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE2_ESCAPE_PARAMETER</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga11</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__ENTROPY_CODING_METHOD_TYPE_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga12</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const char *const</type>
      <name>FLAC__SubframeTypeString</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga13</anchor>
      <arglist>[]</arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__SUBFRAME_LPC_QLP_COEFF_PRECISION_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga14</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__SUBFRAME_LPC_QLP_SHIFT_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga15</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__SUBFRAME_ZERO_PAD_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga16</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__SUBFRAME_TYPE_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga17</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__SUBFRAME_WASTED_BITS_FLAG_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga18</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__SUBFRAME_TYPE_CONSTANT_BYTE_ALIGNED_MASK</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga19</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__SUBFRAME_TYPE_VERBATIM_BYTE_ALIGNED_MASK</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga20</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__SUBFRAME_TYPE_FIXED_BYTE_ALIGNED_MASK</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga21</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__SUBFRAME_TYPE_LPC_BYTE_ALIGNED_MASK</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga22</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const char *const</type>
      <name>FLAC__ChannelAssignmentString</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga23</anchor>
      <arglist>[]</arglist>
    </member>
    <member kind="variable">
      <type>const char *const</type>
      <name>FLAC__FrameNumberTypeString</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga24</anchor>
      <arglist>[]</arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__FRAME_HEADER_SYNC</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga25</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__FRAME_HEADER_SYNC_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga26</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__FRAME_HEADER_RESERVED_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga27</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__FRAME_HEADER_BLOCKING_STRATEGY_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga28</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__FRAME_HEADER_BLOCK_SIZE_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga29</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__FRAME_HEADER_SAMPLE_RATE_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga30</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__FRAME_HEADER_CHANNEL_ASSIGNMENT_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga31</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__FRAME_HEADER_BITS_PER_SAMPLE_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga32</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__FRAME_HEADER_ZERO_PAD_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga33</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__FRAME_HEADER_CRC_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga34</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__FRAME_FOOTER_CRC_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga35</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const char *const</type>
      <name>FLAC__MetadataTypeString</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga36</anchor>
      <arglist>[]</arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_STREAMINFO_MIN_BLOCK_SIZE_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga37</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_STREAMINFO_MAX_BLOCK_SIZE_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga38</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_STREAMINFO_MIN_FRAME_SIZE_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga39</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_STREAMINFO_MAX_FRAME_SIZE_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga40</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_STREAMINFO_SAMPLE_RATE_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga41</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_STREAMINFO_CHANNELS_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga42</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_STREAMINFO_BITS_PER_SAMPLE_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga43</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_STREAMINFO_TOTAL_SAMPLES_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga44</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_STREAMINFO_MD5SUM_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga45</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_APPLICATION_ID_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga46</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_SEEKPOINT_SAMPLE_NUMBER_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga47</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_SEEKPOINT_STREAM_OFFSET_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga48</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_SEEKPOINT_FRAME_SAMPLES_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga49</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const FLAC__uint64</type>
      <name>FLAC__STREAM_METADATA_SEEKPOINT_PLACEHOLDER</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga50</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_VORBIS_COMMENT_ENTRY_LENGTH_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga51</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_VORBIS_COMMENT_NUM_COMMENTS_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga52</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_CUESHEET_INDEX_OFFSET_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga53</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_CUESHEET_INDEX_NUMBER_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga54</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_CUESHEET_INDEX_RESERVED_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga55</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_CUESHEET_TRACK_OFFSET_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga56</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_CUESHEET_TRACK_NUMBER_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga57</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_CUESHEET_TRACK_ISRC_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga58</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_CUESHEET_TRACK_TYPE_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga59</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_CUESHEET_TRACK_PRE_EMPHASIS_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga60</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_CUESHEET_TRACK_RESERVED_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga61</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_CUESHEET_TRACK_NUM_INDICES_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga62</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_CUESHEET_MEDIA_CATALOG_NUMBER_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga63</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_CUESHEET_LEAD_IN_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga64</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_CUESHEET_IS_CD_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga65</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_CUESHEET_RESERVED_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga66</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_CUESHEET_NUM_TRACKS_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga67</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const char *const</type>
      <name>FLAC__StreamMetadata_Picture_TypeString</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga68</anchor>
      <arglist>[]</arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_PICTURE_TYPE_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga69</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_PICTURE_MIME_TYPE_LENGTH_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga70</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_PICTURE_DESCRIPTION_LENGTH_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga71</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_PICTURE_WIDTH_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga72</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_PICTURE_HEIGHT_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga73</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_PICTURE_DEPTH_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga74</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_PICTURE_COLORS_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga75</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_PICTURE_DATA_LENGTH_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga76</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_IS_LAST_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga77</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_TYPE_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga78</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_LENGTH_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga79</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>metadata.h</name>
    <path>/home/jcoalson/flac/build-1.2.1/include/FLAC/</path>
    <filename>metadata_8h</filename>
    <includes id="export_8h" name="export.h" local="yes" imported="no">export.h</includes>
    <includes id="callback_8h" name="callback.h" local="yes" imported="no">callback.h</includes>
    <includes id="format_8h" name="format.h" local="yes" imported="no">format.h</includes>
    <member kind="typedef">
      <type>FLAC__Metadata_SimpleIterator</type>
      <name>FLAC__Metadata_SimpleIterator</name>
      <anchorfile>group__flac__metadata__level1.html</anchorfile>
      <anchor>ga0</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>FLAC__Metadata_Chain</type>
      <name>FLAC__Metadata_Chain</name>
      <anchorfile>group__flac__metadata__level2.html</anchorfile>
      <anchor>ga0</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>FLAC__Metadata_Iterator</type>
      <name>FLAC__Metadata_Iterator</name>
      <anchorfile>group__flac__metadata__level2.html</anchorfile>
      <anchor>ga1</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <name>FLAC__Metadata_SimpleIteratorStatus</name>
      <anchor>ga18</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_SIMPLE_ITERATOR_STATUS_OK</name>
      <anchor>gga18a5</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_SIMPLE_ITERATOR_STATUS_ILLEGAL_INPUT</name>
      <anchor>gga18a6</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_SIMPLE_ITERATOR_STATUS_ERROR_OPENING_FILE</name>
      <anchor>gga18a7</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_SIMPLE_ITERATOR_STATUS_NOT_A_FLAC_FILE</name>
      <anchor>gga18a8</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_SIMPLE_ITERATOR_STATUS_NOT_WRITABLE</name>
      <anchor>gga18a9</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_SIMPLE_ITERATOR_STATUS_BAD_METADATA</name>
      <anchor>gga18a10</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_SIMPLE_ITERATOR_STATUS_READ_ERROR</name>
      <anchor>gga18a11</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_SIMPLE_ITERATOR_STATUS_SEEK_ERROR</name>
      <anchor>gga18a12</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_SIMPLE_ITERATOR_STATUS_WRITE_ERROR</name>
      <anchor>gga18a13</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_SIMPLE_ITERATOR_STATUS_RENAME_ERROR</name>
      <anchor>gga18a14</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_SIMPLE_ITERATOR_STATUS_UNLINK_ERROR</name>
      <anchor>gga18a15</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_SIMPLE_ITERATOR_STATUS_MEMORY_ALLOCATION_ERROR</name>
      <anchor>gga18a16</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_SIMPLE_ITERATOR_STATUS_INTERNAL_ERROR</name>
      <anchor>gga18a17</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <name>FLAC__Metadata_ChainStatus</name>
      <anchor>ga27</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_CHAIN_STATUS_OK</name>
      <anchor>gga27a18</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_CHAIN_STATUS_ILLEGAL_INPUT</name>
      <anchor>gga27a19</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_CHAIN_STATUS_ERROR_OPENING_FILE</name>
      <anchor>gga27a20</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_CHAIN_STATUS_NOT_A_FLAC_FILE</name>
      <anchor>gga27a21</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_CHAIN_STATUS_NOT_WRITABLE</name>
      <anchor>gga27a22</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_CHAIN_STATUS_BAD_METADATA</name>
      <anchor>gga27a23</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_CHAIN_STATUS_READ_ERROR</name>
      <anchor>gga27a24</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_CHAIN_STATUS_SEEK_ERROR</name>
      <anchor>gga27a25</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_CHAIN_STATUS_WRITE_ERROR</name>
      <anchor>gga27a26</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_CHAIN_STATUS_RENAME_ERROR</name>
      <anchor>gga27a27</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_CHAIN_STATUS_UNLINK_ERROR</name>
      <anchor>gga27a28</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_CHAIN_STATUS_MEMORY_ALLOCATION_ERROR</name>
      <anchor>gga27a29</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_CHAIN_STATUS_INTERNAL_ERROR</name>
      <anchor>gga27a30</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_CHAIN_STATUS_INVALID_CALLBACKS</name>
      <anchor>gga27a31</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_CHAIN_STATUS_READ_WRITE_MISMATCH</name>
      <anchor>gga27a32</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_CHAIN_STATUS_WRONG_WRITE_CALL</name>
      <anchor>gga27a33</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_get_streaminfo</name>
      <anchorfile>group__flac__metadata__level0.html</anchorfile>
      <anchor>ga0</anchor>
      <arglist>(const char *filename, FLAC__StreamMetadata *streaminfo)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_get_tags</name>
      <anchorfile>group__flac__metadata__level0.html</anchorfile>
      <anchor>ga1</anchor>
      <arglist>(const char *filename, FLAC__StreamMetadata **tags)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_get_cuesheet</name>
      <anchorfile>group__flac__metadata__level0.html</anchorfile>
      <anchor>ga2</anchor>
      <arglist>(const char *filename, FLAC__StreamMetadata **cuesheet)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_get_picture</name>
      <anchorfile>group__flac__metadata__level0.html</anchorfile>
      <anchor>ga3</anchor>
      <arglist>(const char *filename, FLAC__StreamMetadata **picture, FLAC__StreamMetadata_Picture_Type type, const char *mime_type, const FLAC__byte *description, unsigned max_width, unsigned max_height, unsigned max_depth, unsigned max_colors)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__Metadata_SimpleIterator *</type>
      <name>FLAC__metadata_simple_iterator_new</name>
      <anchorfile>group__flac__metadata__level1.html</anchorfile>
      <anchor>ga2</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>FLAC__metadata_simple_iterator_delete</name>
      <anchorfile>group__flac__metadata__level1.html</anchorfile>
      <anchor>ga3</anchor>
      <arglist>(FLAC__Metadata_SimpleIterator *iterator)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__Metadata_SimpleIteratorStatus</type>
      <name>FLAC__metadata_simple_iterator_status</name>
      <anchorfile>group__flac__metadata__level1.html</anchorfile>
      <anchor>ga4</anchor>
      <arglist>(FLAC__Metadata_SimpleIterator *iterator)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_simple_iterator_init</name>
      <anchorfile>group__flac__metadata__level1.html</anchorfile>
      <anchor>ga5</anchor>
      <arglist>(FLAC__Metadata_SimpleIterator *iterator, const char *filename, FLAC__bool read_only, FLAC__bool preserve_file_stats)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_simple_iterator_is_writable</name>
      <anchorfile>group__flac__metadata__level1.html</anchorfile>
      <anchor>ga6</anchor>
      <arglist>(const FLAC__Metadata_SimpleIterator *iterator)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_simple_iterator_next</name>
      <anchorfile>group__flac__metadata__level1.html</anchorfile>
      <anchor>ga7</anchor>
      <arglist>(FLAC__Metadata_SimpleIterator *iterator)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_simple_iterator_prev</name>
      <anchorfile>group__flac__metadata__level1.html</anchorfile>
      <anchor>ga8</anchor>
      <arglist>(FLAC__Metadata_SimpleIterator *iterator)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_simple_iterator_is_last</name>
      <anchorfile>group__flac__metadata__level1.html</anchorfile>
      <anchor>ga9</anchor>
      <arglist>(const FLAC__Metadata_SimpleIterator *iterator)</arglist>
    </member>
    <member kind="function">
      <type>off_t</type>
      <name>FLAC__metadata_simple_iterator_get_block_offset</name>
      <anchorfile>group__flac__metadata__level1.html</anchorfile>
      <anchor>ga10</anchor>
      <arglist>(const FLAC__Metadata_SimpleIterator *iterator)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__MetadataType</type>
      <name>FLAC__metadata_simple_iterator_get_block_type</name>
      <anchorfile>group__flac__metadata__level1.html</anchorfile>
      <anchor>ga11</anchor>
      <arglist>(const FLAC__Metadata_SimpleIterator *iterator)</arglist>
    </member>
    <member kind="function">
      <type>unsigned</type>
      <name>FLAC__metadata_simple_iterator_get_block_length</name>
      <anchorfile>group__flac__metadata__level1.html</anchorfile>
      <anchor>ga12</anchor>
      <arglist>(const FLAC__Metadata_SimpleIterator *iterator)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_simple_iterator_get_application_id</name>
      <anchorfile>group__flac__metadata__level1.html</anchorfile>
      <anchor>ga13</anchor>
      <arglist>(FLAC__Metadata_SimpleIterator *iterator, FLAC__byte *id)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__StreamMetadata *</type>
      <name>FLAC__metadata_simple_iterator_get_block</name>
      <anchorfile>group__flac__metadata__level1.html</anchorfile>
      <anchor>ga14</anchor>
      <arglist>(FLAC__Metadata_SimpleIterator *iterator)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_simple_iterator_set_block</name>
      <anchorfile>group__flac__metadata__level1.html</anchorfile>
      <anchor>ga15</anchor>
      <arglist>(FLAC__Metadata_SimpleIterator *iterator, FLAC__StreamMetadata *block, FLAC__bool use_padding)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_simple_iterator_insert_block_after</name>
      <anchorfile>group__flac__metadata__level1.html</anchorfile>
      <anchor>ga16</anchor>
      <arglist>(FLAC__Metadata_SimpleIterator *iterator, FLAC__StreamMetadata *block, FLAC__bool use_padding)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_simple_iterator_delete_block</name>
      <anchorfile>group__flac__metadata__level1.html</anchorfile>
      <anchor>ga17</anchor>
      <arglist>(FLAC__Metadata_SimpleIterator *iterator, FLAC__bool use_padding)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__Metadata_Chain *</type>
      <name>FLAC__metadata_chain_new</name>
      <anchorfile>group__flac__metadata__level2.html</anchorfile>
      <anchor>ga3</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>FLAC__metadata_chain_delete</name>
      <anchorfile>group__flac__metadata__level2.html</anchorfile>
      <anchor>ga4</anchor>
      <arglist>(FLAC__Metadata_Chain *chain)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__Metadata_ChainStatus</type>
      <name>FLAC__metadata_chain_status</name>
      <anchorfile>group__flac__metadata__level2.html</anchorfile>
      <anchor>ga5</anchor>
      <arglist>(FLAC__Metadata_Chain *chain)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_chain_read</name>
      <anchorfile>group__flac__metadata__level2.html</anchorfile>
      <anchor>ga6</anchor>
      <arglist>(FLAC__Metadata_Chain *chain, const char *filename)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_chain_read_ogg</name>
      <anchorfile>group__flac__metadata__level2.html</anchorfile>
      <anchor>ga7</anchor>
      <arglist>(FLAC__Metadata_Chain *chain, const char *filename)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_chain_read_with_callbacks</name>
      <anchorfile>group__flac__metadata__level2.html</anchorfile>
      <anchor>ga8</anchor>
      <arglist>(FLAC__Metadata_Chain *chain, FLAC__IOHandle handle, FLAC__IOCallbacks callbacks)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_chain_read_ogg_with_callbacks</name>
      <anchorfile>group__flac__metadata__level2.html</anchorfile>
      <anchor>ga9</anchor>
      <arglist>(FLAC__Metadata_Chain *chain, FLAC__IOHandle handle, FLAC__IOCallbacks callbacks)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_chain_check_if_tempfile_needed</name>
      <anchorfile>group__flac__metadata__level2.html</anchorfile>
      <anchor>ga10</anchor>
      <arglist>(FLAC__Metadata_Chain *chain, FLAC__bool use_padding)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_chain_write</name>
      <anchorfile>group__flac__metadata__level2.html</anchorfile>
      <anchor>ga11</anchor>
      <arglist>(FLAC__Metadata_Chain *chain, FLAC__bool use_padding, FLAC__bool preserve_file_stats)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_chain_write_with_callbacks</name>
      <anchorfile>group__flac__metadata__level2.html</anchorfile>
      <anchor>ga12</anchor>
      <arglist>(FLAC__Metadata_Chain *chain, FLAC__bool use_padding, FLAC__IOHandle handle, FLAC__IOCallbacks callbacks)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_chain_write_with_callbacks_and_tempfile</name>
      <anchorfile>group__flac__metadata__level2.html</anchorfile>
      <anchor>ga13</anchor>
      <arglist>(FLAC__Metadata_Chain *chain, FLAC__bool use_padding, FLAC__IOHandle handle, FLAC__IOCallbacks callbacks, FLAC__IOHandle temp_handle, FLAC__IOCallbacks temp_callbacks)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>FLAC__metadata_chain_merge_padding</name>
      <anchorfile>group__flac__metadata__level2.html</anchorfile>
      <anchor>ga14</anchor>
      <arglist>(FLAC__Metadata_Chain *chain)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>FLAC__metadata_chain_sort_padding</name>
      <anchorfile>group__flac__metadata__level2.html</anchorfile>
      <anchor>ga15</anchor>
      <arglist>(FLAC__Metadata_Chain *chain)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__Metadata_Iterator *</type>
      <name>FLAC__metadata_iterator_new</name>
      <anchorfile>group__flac__metadata__level2.html</anchorfile>
      <anchor>ga16</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>FLAC__metadata_iterator_delete</name>
      <anchorfile>group__flac__metadata__level2.html</anchorfile>
      <anchor>ga17</anchor>
      <arglist>(FLAC__Metadata_Iterator *iterator)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>FLAC__metadata_iterator_init</name>
      <anchorfile>group__flac__metadata__level2.html</anchorfile>
      <anchor>ga18</anchor>
      <arglist>(FLAC__Metadata_Iterator *iterator, FLAC__Metadata_Chain *chain)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_iterator_next</name>
      <anchorfile>group__flac__metadata__level2.html</anchorfile>
      <anchor>ga19</anchor>
      <arglist>(FLAC__Metadata_Iterator *iterator)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_iterator_prev</name>
      <anchorfile>group__flac__metadata__level2.html</anchorfile>
      <anchor>ga20</anchor>
      <arglist>(FLAC__Metadata_Iterator *iterator)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__MetadataType</type>
      <name>FLAC__metadata_iterator_get_block_type</name>
      <anchorfile>group__flac__metadata__level2.html</anchorfile>
      <anchor>ga21</anchor>
      <arglist>(const FLAC__Metadata_Iterator *iterator)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__StreamMetadata *</type>
      <name>FLAC__metadata_iterator_get_block</name>
      <anchorfile>group__flac__metadata__level2.html</anchorfile>
      <anchor>ga22</anchor>
      <arglist>(FLAC__Metadata_Iterator *iterator)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_iterator_set_block</name>
      <anchorfile>group__flac__metadata__level2.html</anchorfile>
      <anchor>ga23</anchor>
      <arglist>(FLAC__Metadata_Iterator *iterator, FLAC__StreamMetadata *block)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_iterator_delete_block</name>
      <anchorfile>group__flac__metadata__level2.html</anchorfile>
      <anchor>ga24</anchor>
      <arglist>(FLAC__Metadata_Iterator *iterator, FLAC__bool replace_with_padding)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_iterator_insert_block_before</name>
      <anchorfile>group__flac__metadata__level2.html</anchorfile>
      <anchor>ga25</anchor>
      <arglist>(FLAC__Metadata_Iterator *iterator, FLAC__StreamMetadata *block)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_iterator_insert_block_after</name>
      <anchorfile>group__flac__metadata__level2.html</anchorfile>
      <anchor>ga26</anchor>
      <arglist>(FLAC__Metadata_Iterator *iterator, FLAC__StreamMetadata *block)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__StreamMetadata *</type>
      <name>FLAC__metadata_object_new</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga0</anchor>
      <arglist>(FLAC__MetadataType type)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__StreamMetadata *</type>
      <name>FLAC__metadata_object_clone</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga1</anchor>
      <arglist>(const FLAC__StreamMetadata *object)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>FLAC__metadata_object_delete</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga2</anchor>
      <arglist>(FLAC__StreamMetadata *object)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_is_equal</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga3</anchor>
      <arglist>(const FLAC__StreamMetadata *block1, const FLAC__StreamMetadata *block2)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_application_set_data</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga4</anchor>
      <arglist>(FLAC__StreamMetadata *object, FLAC__byte *data, unsigned length, FLAC__bool copy)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_seektable_resize_points</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga5</anchor>
      <arglist>(FLAC__StreamMetadata *object, unsigned new_num_points)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>FLAC__metadata_object_seektable_set_point</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga6</anchor>
      <arglist>(FLAC__StreamMetadata *object, unsigned point_num, FLAC__StreamMetadata_SeekPoint point)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_seektable_insert_point</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga7</anchor>
      <arglist>(FLAC__StreamMetadata *object, unsigned point_num, FLAC__StreamMetadata_SeekPoint point)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_seektable_delete_point</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga8</anchor>
      <arglist>(FLAC__StreamMetadata *object, unsigned point_num)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_seektable_is_legal</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga9</anchor>
      <arglist>(const FLAC__StreamMetadata *object)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_seektable_template_append_placeholders</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga10</anchor>
      <arglist>(FLAC__StreamMetadata *object, unsigned num)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_seektable_template_append_point</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga11</anchor>
      <arglist>(FLAC__StreamMetadata *object, FLAC__uint64 sample_number)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_seektable_template_append_points</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga12</anchor>
      <arglist>(FLAC__StreamMetadata *object, FLAC__uint64 sample_numbers[], unsigned num)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_seektable_template_append_spaced_points</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga13</anchor>
      <arglist>(FLAC__StreamMetadata *object, unsigned num, FLAC__uint64 total_samples)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_seektable_template_append_spaced_points_by_samples</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga14</anchor>
      <arglist>(FLAC__StreamMetadata *object, unsigned samples, FLAC__uint64 total_samples)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_seektable_template_sort</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga15</anchor>
      <arglist>(FLAC__StreamMetadata *object, FLAC__bool compact)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_vorbiscomment_set_vendor_string</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga16</anchor>
      <arglist>(FLAC__StreamMetadata *object, FLAC__StreamMetadata_VorbisComment_Entry entry, FLAC__bool copy)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_vorbiscomment_resize_comments</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga17</anchor>
      <arglist>(FLAC__StreamMetadata *object, unsigned new_num_comments)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_vorbiscomment_set_comment</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga18</anchor>
      <arglist>(FLAC__StreamMetadata *object, unsigned comment_num, FLAC__StreamMetadata_VorbisComment_Entry entry, FLAC__bool copy)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_vorbiscomment_insert_comment</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga19</anchor>
      <arglist>(FLAC__StreamMetadata *object, unsigned comment_num, FLAC__StreamMetadata_VorbisComment_Entry entry, FLAC__bool copy)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_vorbiscomment_append_comment</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga20</anchor>
      <arglist>(FLAC__StreamMetadata *object, FLAC__StreamMetadata_VorbisComment_Entry entry, FLAC__bool copy)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_vorbiscomment_replace_comment</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga21</anchor>
      <arglist>(FLAC__StreamMetadata *object, FLAC__StreamMetadata_VorbisComment_Entry entry, FLAC__bool all, FLAC__bool copy)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_vorbiscomment_delete_comment</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga22</anchor>
      <arglist>(FLAC__StreamMetadata *object, unsigned comment_num)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga23</anchor>
      <arglist>(FLAC__StreamMetadata_VorbisComment_Entry *entry, const char *field_name, const char *field_value)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_vorbiscomment_entry_to_name_value_pair</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga24</anchor>
      <arglist>(const FLAC__StreamMetadata_VorbisComment_Entry entry, char **field_name, char **field_value)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_vorbiscomment_entry_matches</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga25</anchor>
      <arglist>(const FLAC__StreamMetadata_VorbisComment_Entry entry, const char *field_name, unsigned field_name_length)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>FLAC__metadata_object_vorbiscomment_find_entry_from</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga26</anchor>
      <arglist>(const FLAC__StreamMetadata *object, unsigned offset, const char *field_name)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>FLAC__metadata_object_vorbiscomment_remove_entry_matching</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga27</anchor>
      <arglist>(FLAC__StreamMetadata *object, const char *field_name)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>FLAC__metadata_object_vorbiscomment_remove_entries_matching</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga28</anchor>
      <arglist>(FLAC__StreamMetadata *object, const char *field_name)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__StreamMetadata_CueSheet_Track *</type>
      <name>FLAC__metadata_object_cuesheet_track_new</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga29</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__StreamMetadata_CueSheet_Track *</type>
      <name>FLAC__metadata_object_cuesheet_track_clone</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga30</anchor>
      <arglist>(const FLAC__StreamMetadata_CueSheet_Track *object)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>FLAC__metadata_object_cuesheet_track_delete</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga31</anchor>
      <arglist>(FLAC__StreamMetadata_CueSheet_Track *object)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_cuesheet_track_resize_indices</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga32</anchor>
      <arglist>(FLAC__StreamMetadata *object, unsigned track_num, unsigned new_num_indices)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_cuesheet_track_insert_index</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga33</anchor>
      <arglist>(FLAC__StreamMetadata *object, unsigned track_num, unsigned index_num, FLAC__StreamMetadata_CueSheet_Index index)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_cuesheet_track_insert_blank_index</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga34</anchor>
      <arglist>(FLAC__StreamMetadata *object, unsigned track_num, unsigned index_num)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_cuesheet_track_delete_index</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga35</anchor>
      <arglist>(FLAC__StreamMetadata *object, unsigned track_num, unsigned index_num)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_cuesheet_resize_tracks</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga36</anchor>
      <arglist>(FLAC__StreamMetadata *object, unsigned new_num_tracks)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_cuesheet_insert_track</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga37</anchor>
      <arglist>(FLAC__StreamMetadata *object, unsigned track_num, FLAC__StreamMetadata_CueSheet_Track *track, FLAC__bool copy)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_cuesheet_insert_blank_track</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga38</anchor>
      <arglist>(FLAC__StreamMetadata *object, unsigned track_num)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_cuesheet_delete_track</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga39</anchor>
      <arglist>(FLAC__StreamMetadata *object, unsigned track_num)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_cuesheet_is_legal</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga40</anchor>
      <arglist>(const FLAC__StreamMetadata *object, FLAC__bool check_cd_da_subset, const char **violation)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__uint32</type>
      <name>FLAC__metadata_object_cuesheet_calculate_cddb_id</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga41</anchor>
      <arglist>(const FLAC__StreamMetadata *object)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_picture_set_mime_type</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga42</anchor>
      <arglist>(FLAC__StreamMetadata *object, char *mime_type, FLAC__bool copy)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_picture_set_description</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga43</anchor>
      <arglist>(FLAC__StreamMetadata *object, FLAC__byte *description, FLAC__bool copy)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_picture_set_data</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga44</anchor>
      <arglist>(FLAC__StreamMetadata *object, FLAC__byte *data, FLAC__uint32 length, FLAC__bool copy)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_picture_is_legal</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga45</anchor>
      <arglist>(const FLAC__StreamMetadata *object, const char **violation)</arglist>
    </member>
    <member kind="variable">
      <type>const char *const</type>
      <name>FLAC__Metadata_SimpleIteratorStatusString</name>
      <anchorfile>group__flac__metadata__level1.html</anchorfile>
      <anchor>ga1</anchor>
      <arglist>[]</arglist>
    </member>
    <member kind="variable">
      <type>const char *const</type>
      <name>FLAC__Metadata_ChainStatusString</name>
      <anchorfile>group__flac__metadata__level2.html</anchorfile>
      <anchor>ga2</anchor>
      <arglist>[]</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>metadata.h</name>
    <path>/home/jcoalson/flac/build-1.2.1/include/FLAC++/</path>
    <filename>+_2metadata_8h</filename>
    <includes id="+_2export_8h" name="export.h" local="yes" imported="no">export.h</includes>
    <includes id="metadata_8h" name="metadata.h" local="yes" imported="no">FLAC/metadata.h</includes>
    <member kind="function">
      <type>Prototype *</type>
      <name>clone</name>
      <anchorfile>group__flacpp__metadata__object.html</anchorfile>
      <anchor>ga0</anchor>
      <arglist>(const Prototype *)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>get_streaminfo</name>
      <anchorfile>group__flacpp__metadata__level0.html</anchorfile>
      <anchor>ga0</anchor>
      <arglist>(const char *filename, StreamInfo &amp;streaminfo)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>get_tags</name>
      <anchorfile>group__flacpp__metadata__level0.html</anchorfile>
      <anchor>ga1</anchor>
      <arglist>(const char *filename, VorbisComment *&amp;tags)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>get_tags</name>
      <anchorfile>group__flacpp__metadata__level0.html</anchorfile>
      <anchor>ga2</anchor>
      <arglist>(const char *filename, VorbisComment &amp;tags)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>get_cuesheet</name>
      <anchorfile>group__flacpp__metadata__level0.html</anchorfile>
      <anchor>ga3</anchor>
      <arglist>(const char *filename, CueSheet *&amp;cuesheet)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>get_cuesheet</name>
      <anchorfile>group__flacpp__metadata__level0.html</anchorfile>
      <anchor>ga4</anchor>
      <arglist>(const char *filename, CueSheet &amp;cuesheet)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>get_picture</name>
      <anchorfile>group__flacpp__metadata__level0.html</anchorfile>
      <anchor>ga5</anchor>
      <arglist>(const char *filename, Picture *&amp;picture,::FLAC__StreamMetadata_Picture_Type type, const char *mime_type, const FLAC__byte *description, unsigned max_width, unsigned max_height, unsigned max_depth, unsigned max_colors)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>get_picture</name>
      <anchorfile>group__flacpp__metadata__level0.html</anchorfile>
      <anchor>ga6</anchor>
      <arglist>(const char *filename, Picture &amp;picture,::FLAC__StreamMetadata_Picture_Type type, const char *mime_type, const FLAC__byte *description, unsigned max_width, unsigned max_height, unsigned max_depth, unsigned max_colors)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>stream_decoder.h</name>
    <path>/home/jcoalson/flac/build-1.2.1/include/FLAC/</path>
    <filename>stream__decoder_8h</filename>
    <includes id="export_8h" name="export.h" local="yes" imported="no">export.h</includes>
    <includes id="format_8h" name="format.h" local="yes" imported="no">format.h</includes>
    <member kind="typedef">
      <type>FLAC__StreamDecoderReadStatus(*</type>
      <name>FLAC__StreamDecoderReadCallback</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga8</anchor>
      <arglist>)(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], size_t *bytes, void *client_data)</arglist>
    </member>
    <member kind="typedef">
      <type>FLAC__StreamDecoderSeekStatus(*</type>
      <name>FLAC__StreamDecoderSeekCallback</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga9</anchor>
      <arglist>)(const FLAC__StreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data)</arglist>
    </member>
    <member kind="typedef">
      <type>FLAC__StreamDecoderTellStatus(*</type>
      <name>FLAC__StreamDecoderTellCallback</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga10</anchor>
      <arglist>)(const FLAC__StreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data)</arglist>
    </member>
    <member kind="typedef">
      <type>FLAC__StreamDecoderLengthStatus(*</type>
      <name>FLAC__StreamDecoderLengthCallback</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga11</anchor>
      <arglist>)(const FLAC__StreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data)</arglist>
    </member>
    <member kind="typedef">
      <type>FLAC__bool(*</type>
      <name>FLAC__StreamDecoderEofCallback</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga12</anchor>
      <arglist>)(const FLAC__StreamDecoder *decoder, void *client_data)</arglist>
    </member>
    <member kind="typedef">
      <type>FLAC__StreamDecoderWriteStatus(*</type>
      <name>FLAC__StreamDecoderWriteCallback</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga13</anchor>
      <arglist>)(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 *const buffer[], void *client_data)</arglist>
    </member>
    <member kind="typedef">
      <type>void(*</type>
      <name>FLAC__StreamDecoderMetadataCallback</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga14</anchor>
      <arglist>)(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data)</arglist>
    </member>
    <member kind="typedef">
      <type>void(*</type>
      <name>FLAC__StreamDecoderErrorCallback</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga15</anchor>
      <arglist>)(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)</arglist>
    </member>
    <member kind="enumeration">
      <name>FLAC__StreamDecoderState</name>
      <anchor>ga50</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_SEARCH_FOR_METADATA</name>
      <anchor>gga50a16</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_READ_METADATA</name>
      <anchor>gga50a17</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC</name>
      <anchor>gga50a18</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_READ_FRAME</name>
      <anchor>gga50a19</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_END_OF_STREAM</name>
      <anchor>gga50a20</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_OGG_ERROR</name>
      <anchor>gga50a21</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_SEEK_ERROR</name>
      <anchor>gga50a22</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_ABORTED</name>
      <anchor>gga50a23</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_MEMORY_ALLOCATION_ERROR</name>
      <anchor>gga50a24</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_UNINITIALIZED</name>
      <anchor>gga50a25</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <name>FLAC__StreamDecoderInitStatus</name>
      <anchor>ga51</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_INIT_STATUS_OK</name>
      <anchor>gga51a26</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_INIT_STATUS_UNSUPPORTED_CONTAINER</name>
      <anchor>gga51a27</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_INIT_STATUS_INVALID_CALLBACKS</name>
      <anchor>gga51a28</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_INIT_STATUS_MEMORY_ALLOCATION_ERROR</name>
      <anchor>gga51a29</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_INIT_STATUS_ERROR_OPENING_FILE</name>
      <anchor>gga51a30</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_INIT_STATUS_ALREADY_INITIALIZED</name>
      <anchor>gga51a31</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <name>FLAC__StreamDecoderReadStatus</name>
      <anchor>ga52</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_READ_STATUS_CONTINUE</name>
      <anchor>gga52a32</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM</name>
      <anchor>gga52a33</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_READ_STATUS_ABORT</name>
      <anchor>gga52a34</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <name>FLAC__StreamDecoderSeekStatus</name>
      <anchor>ga53</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_SEEK_STATUS_OK</name>
      <anchor>gga53a35</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_SEEK_STATUS_ERROR</name>
      <anchor>gga53a36</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_SEEK_STATUS_UNSUPPORTED</name>
      <anchor>gga53a37</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <name>FLAC__StreamDecoderTellStatus</name>
      <anchor>ga54</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_TELL_STATUS_OK</name>
      <anchor>gga54a38</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_TELL_STATUS_ERROR</name>
      <anchor>gga54a39</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_TELL_STATUS_UNSUPPORTED</name>
      <anchor>gga54a40</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <name>FLAC__StreamDecoderLengthStatus</name>
      <anchor>ga55</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_LENGTH_STATUS_OK</name>
      <anchor>gga55a41</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_LENGTH_STATUS_ERROR</name>
      <anchor>gga55a42</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_LENGTH_STATUS_UNSUPPORTED</name>
      <anchor>gga55a43</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <name>FLAC__StreamDecoderWriteStatus</name>
      <anchor>ga56</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE</name>
      <anchor>gga56a44</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_WRITE_STATUS_ABORT</name>
      <anchor>gga56a45</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <name>FLAC__StreamDecoderErrorStatus</name>
      <anchor>ga57</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_ERROR_STATUS_LOST_SYNC</name>
      <anchor>gga57a46</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_ERROR_STATUS_BAD_HEADER</name>
      <anchor>gga57a47</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_ERROR_STATUS_FRAME_CRC_MISMATCH</name>
      <anchor>gga57a48</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_ERROR_STATUS_UNPARSEABLE_STREAM</name>
      <anchor>gga57a49</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>FLAC__StreamDecoder *</type>
      <name>FLAC__stream_decoder_new</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga16</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>FLAC__stream_decoder_delete</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga17</anchor>
      <arglist>(FLAC__StreamDecoder *decoder)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_decoder_set_ogg_serial_number</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga18</anchor>
      <arglist>(FLAC__StreamDecoder *decoder, long serial_number)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_decoder_set_md5_checking</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga19</anchor>
      <arglist>(FLAC__StreamDecoder *decoder, FLAC__bool value)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_decoder_set_metadata_respond</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga20</anchor>
      <arglist>(FLAC__StreamDecoder *decoder, FLAC__MetadataType type)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_decoder_set_metadata_respond_application</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga21</anchor>
      <arglist>(FLAC__StreamDecoder *decoder, const FLAC__byte id[4])</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_decoder_set_metadata_respond_all</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga22</anchor>
      <arglist>(FLAC__StreamDecoder *decoder)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_decoder_set_metadata_ignore</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga23</anchor>
      <arglist>(FLAC__StreamDecoder *decoder, FLAC__MetadataType type)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_decoder_set_metadata_ignore_application</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga24</anchor>
      <arglist>(FLAC__StreamDecoder *decoder, const FLAC__byte id[4])</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_decoder_set_metadata_ignore_all</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga25</anchor>
      <arglist>(FLAC__StreamDecoder *decoder)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__StreamDecoderState</type>
      <name>FLAC__stream_decoder_get_state</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga26</anchor>
      <arglist>(const FLAC__StreamDecoder *decoder)</arglist>
    </member>
    <member kind="function">
      <type>const char *</type>
      <name>FLAC__stream_decoder_get_resolved_state_string</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga27</anchor>
      <arglist>(const FLAC__StreamDecoder *decoder)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_decoder_get_md5_checking</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga28</anchor>
      <arglist>(const FLAC__StreamDecoder *decoder)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__uint64</type>
      <name>FLAC__stream_decoder_get_total_samples</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga29</anchor>
      <arglist>(const FLAC__StreamDecoder *decoder)</arglist>
    </member>
    <member kind="function">
      <type>unsigned</type>
      <name>FLAC__stream_decoder_get_channels</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga30</anchor>
      <arglist>(const FLAC__StreamDecoder *decoder)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__ChannelAssignment</type>
      <name>FLAC__stream_decoder_get_channel_assignment</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga31</anchor>
      <arglist>(const FLAC__StreamDecoder *decoder)</arglist>
    </member>
    <member kind="function">
      <type>unsigned</type>
      <name>FLAC__stream_decoder_get_bits_per_sample</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga32</anchor>
      <arglist>(const FLAC__StreamDecoder *decoder)</arglist>
    </member>
    <member kind="function">
      <type>unsigned</type>
      <name>FLAC__stream_decoder_get_sample_rate</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga33</anchor>
      <arglist>(const FLAC__StreamDecoder *decoder)</arglist>
    </member>
    <member kind="function">
      <type>unsigned</type>
      <name>FLAC__stream_decoder_get_blocksize</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga34</anchor>
      <arglist>(const FLAC__StreamDecoder *decoder)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_decoder_get_decode_position</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga35</anchor>
      <arglist>(const FLAC__StreamDecoder *decoder, FLAC__uint64 *position)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__StreamDecoderInitStatus</type>
      <name>FLAC__stream_decoder_init_stream</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga36</anchor>
      <arglist>(FLAC__StreamDecoder *decoder, FLAC__StreamDecoderReadCallback read_callback, FLAC__StreamDecoderSeekCallback seek_callback, FLAC__StreamDecoderTellCallback tell_callback, FLAC__StreamDecoderLengthCallback length_callback, FLAC__StreamDecoderEofCallback eof_callback, FLAC__StreamDecoderWriteCallback write_callback, FLAC__StreamDecoderMetadataCallback metadata_callback, FLAC__StreamDecoderErrorCallback error_callback, void *client_data)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__StreamDecoderInitStatus</type>
      <name>FLAC__stream_decoder_init_ogg_stream</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga37</anchor>
      <arglist>(FLAC__StreamDecoder *decoder, FLAC__StreamDecoderReadCallback read_callback, FLAC__StreamDecoderSeekCallback seek_callback, FLAC__StreamDecoderTellCallback tell_callback, FLAC__StreamDecoderLengthCallback length_callback, FLAC__StreamDecoderEofCallback eof_callback, FLAC__StreamDecoderWriteCallback write_callback, FLAC__StreamDecoderMetadataCallback metadata_callback, FLAC__StreamDecoderErrorCallback error_callback, void *client_data)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__StreamDecoderInitStatus</type>
      <name>FLAC__stream_decoder_init_FILE</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga38</anchor>
      <arglist>(FLAC__StreamDecoder *decoder, FILE *file, FLAC__StreamDecoderWriteCallback write_callback, FLAC__StreamDecoderMetadataCallback metadata_callback, FLAC__StreamDecoderErrorCallback error_callback, void *client_data)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__StreamDecoderInitStatus</type>
      <name>FLAC__stream_decoder_init_ogg_FILE</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga39</anchor>
      <arglist>(FLAC__StreamDecoder *decoder, FILE *file, FLAC__StreamDecoderWriteCallback write_callback, FLAC__StreamDecoderMetadataCallback metadata_callback, FLAC__StreamDecoderErrorCallback error_callback, void *client_data)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__StreamDecoderInitStatus</type>
      <name>FLAC__stream_decoder_init_file</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga40</anchor>
      <arglist>(FLAC__StreamDecoder *decoder, const char *filename, FLAC__StreamDecoderWriteCallback write_callback, FLAC__StreamDecoderMetadataCallback metadata_callback, FLAC__StreamDecoderErrorCallback error_callback, void *client_data)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__StreamDecoderInitStatus</type>
      <name>FLAC__stream_decoder_init_ogg_file</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga41</anchor>
      <arglist>(FLAC__StreamDecoder *decoder, const char *filename, FLAC__StreamDecoderWriteCallback write_callback, FLAC__StreamDecoderMetadataCallback metadata_callback, FLAC__StreamDecoderErrorCallback error_callback, void *client_data)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_decoder_finish</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga42</anchor>
      <arglist>(FLAC__StreamDecoder *decoder)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_decoder_flush</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga43</anchor>
      <arglist>(FLAC__StreamDecoder *decoder)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_decoder_reset</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga44</anchor>
      <arglist>(FLAC__StreamDecoder *decoder)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_decoder_process_single</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga45</anchor>
      <arglist>(FLAC__StreamDecoder *decoder)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_decoder_process_until_end_of_metadata</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga46</anchor>
      <arglist>(FLAC__StreamDecoder *decoder)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_decoder_process_until_end_of_stream</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga47</anchor>
      <arglist>(FLAC__StreamDecoder *decoder)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_decoder_skip_single_frame</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga48</anchor>
      <arglist>(FLAC__StreamDecoder *decoder)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_decoder_seek_absolute</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga49</anchor>
      <arglist>(FLAC__StreamDecoder *decoder, FLAC__uint64 sample)</arglist>
    </member>
    <member kind="variable">
      <type>const char *const</type>
      <name>FLAC__StreamDecoderStateString</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga0</anchor>
      <arglist>[]</arglist>
    </member>
    <member kind="variable">
      <type>const char *const</type>
      <name>FLAC__StreamDecoderInitStatusString</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga1</anchor>
      <arglist>[]</arglist>
    </member>
    <member kind="variable">
      <type>const char *const</type>
      <name>FLAC__StreamDecoderReadStatusString</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga2</anchor>
      <arglist>[]</arglist>
    </member>
    <member kind="variable">
      <type>const char *const</type>
      <name>FLAC__StreamDecoderSeekStatusString</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga3</anchor>
      <arglist>[]</arglist>
    </member>
    <member kind="variable">
      <type>const char *const</type>
      <name>FLAC__StreamDecoderTellStatusString</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga4</anchor>
      <arglist>[]</arglist>
    </member>
    <member kind="variable">
      <type>const char *const</type>
      <name>FLAC__StreamDecoderLengthStatusString</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga5</anchor>
      <arglist>[]</arglist>
    </member>
    <member kind="variable">
      <type>const char *const</type>
      <name>FLAC__StreamDecoderWriteStatusString</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga6</anchor>
      <arglist>[]</arglist>
    </member>
    <member kind="variable">
      <type>const char *const</type>
      <name>FLAC__StreamDecoderErrorStatusString</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga7</anchor>
      <arglist>[]</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>stream_encoder.h</name>
    <path>/home/jcoalson/flac/build-1.2.1/include/FLAC/</path>
    <filename>stream__encoder_8h</filename>
    <includes id="export_8h" name="export.h" local="yes" imported="no">export.h</includes>
    <includes id="format_8h" name="format.h" local="yes" imported="no">format.h</includes>
    <includes id="stream__decoder_8h" name="stream_decoder.h" local="yes" imported="no">stream_decoder.h</includes>
    <member kind="typedef">
      <type>FLAC__StreamEncoderReadStatus(*</type>
      <name>FLAC__StreamEncoderReadCallback</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga6</anchor>
      <arglist>)(const FLAC__StreamEncoder *encoder, FLAC__byte buffer[], size_t *bytes, void *client_data)</arglist>
    </member>
    <member kind="typedef">
      <type>FLAC__StreamEncoderWriteStatus(*</type>
      <name>FLAC__StreamEncoderWriteCallback</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga7</anchor>
      <arglist>)(const FLAC__StreamEncoder *encoder, const FLAC__byte buffer[], size_t bytes, unsigned samples, unsigned current_frame, void *client_data)</arglist>
    </member>
    <member kind="typedef">
      <type>FLAC__StreamEncoderSeekStatus(*</type>
      <name>FLAC__StreamEncoderSeekCallback</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga8</anchor>
      <arglist>)(const FLAC__StreamEncoder *encoder, FLAC__uint64 absolute_byte_offset, void *client_data)</arglist>
    </member>
    <member kind="typedef">
      <type>FLAC__StreamEncoderTellStatus(*</type>
      <name>FLAC__StreamEncoderTellCallback</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga9</anchor>
      <arglist>)(const FLAC__StreamEncoder *encoder, FLAC__uint64 *absolute_byte_offset, void *client_data)</arglist>
    </member>
    <member kind="typedef">
      <type>void(*</type>
      <name>FLAC__StreamEncoderMetadataCallback</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga10</anchor>
      <arglist>)(const FLAC__StreamEncoder *encoder, const FLAC__StreamMetadata *metadata, void *client_data)</arglist>
    </member>
    <member kind="typedef">
      <type>void(*</type>
      <name>FLAC__StreamEncoderProgressCallback</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga11</anchor>
      <arglist>)(const FLAC__StreamEncoder *encoder, FLAC__uint64 bytes_written, FLAC__uint64 samples_written, unsigned frames_written, unsigned total_frames_estimate, void *client_data)</arglist>
    </member>
    <member kind="enumeration">
      <name>FLAC__StreamEncoderState</name>
      <anchor>ga65</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_OK</name>
      <anchor>gga65a12</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_UNINITIALIZED</name>
      <anchor>gga65a13</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_OGG_ERROR</name>
      <anchor>gga65a14</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_VERIFY_DECODER_ERROR</name>
      <anchor>gga65a15</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_VERIFY_MISMATCH_IN_AUDIO_DATA</name>
      <anchor>gga65a16</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_CLIENT_ERROR</name>
      <anchor>gga65a17</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_IO_ERROR</name>
      <anchor>gga65a18</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_FRAMING_ERROR</name>
      <anchor>gga65a19</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_MEMORY_ALLOCATION_ERROR</name>
      <anchor>gga65a20</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <name>FLAC__StreamEncoderInitStatus</name>
      <anchor>ga66</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_INIT_STATUS_OK</name>
      <anchor>gga66a21</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_INIT_STATUS_ENCODER_ERROR</name>
      <anchor>gga66a22</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_INIT_STATUS_UNSUPPORTED_CONTAINER</name>
      <anchor>gga66a23</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_INIT_STATUS_INVALID_CALLBACKS</name>
      <anchor>gga66a24</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_INIT_STATUS_INVALID_NUMBER_OF_CHANNELS</name>
      <anchor>gga66a25</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_INIT_STATUS_INVALID_BITS_PER_SAMPLE</name>
      <anchor>gga66a26</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_INIT_STATUS_INVALID_SAMPLE_RATE</name>
      <anchor>gga66a27</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_INIT_STATUS_INVALID_BLOCK_SIZE</name>
      <anchor>gga66a28</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_INIT_STATUS_INVALID_MAX_LPC_ORDER</name>
      <anchor>gga66a29</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_INIT_STATUS_INVALID_QLP_COEFF_PRECISION</name>
      <anchor>gga66a30</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_INIT_STATUS_BLOCK_SIZE_TOO_SMALL_FOR_LPC_ORDER</name>
      <anchor>gga66a31</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_INIT_STATUS_NOT_STREAMABLE</name>
      <anchor>gga66a32</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_INIT_STATUS_INVALID_METADATA</name>
      <anchor>gga66a33</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_INIT_STATUS_ALREADY_INITIALIZED</name>
      <anchor>gga66a34</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <name>FLAC__StreamEncoderReadStatus</name>
      <anchor>ga67</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_READ_STATUS_CONTINUE</name>
      <anchor>gga67a35</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_READ_STATUS_END_OF_STREAM</name>
      <anchor>gga67a36</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_READ_STATUS_ABORT</name>
      <anchor>gga67a37</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_READ_STATUS_UNSUPPORTED</name>
      <anchor>gga67a38</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <name>FLAC__StreamEncoderWriteStatus</name>
      <anchor>ga68</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_WRITE_STATUS_OK</name>
      <anchor>gga68a39</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR</name>
      <anchor>gga68a40</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <name>FLAC__StreamEncoderSeekStatus</name>
      <anchor>ga69</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_SEEK_STATUS_OK</name>
      <anchor>gga69a41</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_SEEK_STATUS_ERROR</name>
      <anchor>gga69a42</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_SEEK_STATUS_UNSUPPORTED</name>
      <anchor>gga69a43</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <name>FLAC__StreamEncoderTellStatus</name>
      <anchor>ga70</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_TELL_STATUS_OK</name>
      <anchor>gga70a44</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_TELL_STATUS_ERROR</name>
      <anchor>gga70a45</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_TELL_STATUS_UNSUPPORTED</name>
      <anchor>gga70a46</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>FLAC__StreamEncoder *</type>
      <name>FLAC__stream_encoder_new</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga12</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>FLAC__stream_encoder_delete</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga13</anchor>
      <arglist>(FLAC__StreamEncoder *encoder)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_encoder_set_ogg_serial_number</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga14</anchor>
      <arglist>(FLAC__StreamEncoder *encoder, long serial_number)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_encoder_set_verify</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga15</anchor>
      <arglist>(FLAC__StreamEncoder *encoder, FLAC__bool value)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_encoder_set_streamable_subset</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga16</anchor>
      <arglist>(FLAC__StreamEncoder *encoder, FLAC__bool value)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_encoder_set_channels</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga17</anchor>
      <arglist>(FLAC__StreamEncoder *encoder, unsigned value)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_encoder_set_bits_per_sample</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga18</anchor>
      <arglist>(FLAC__StreamEncoder *encoder, unsigned value)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_encoder_set_sample_rate</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga19</anchor>
      <arglist>(FLAC__StreamEncoder *encoder, unsigned value)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_encoder_set_compression_level</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga20</anchor>
      <arglist>(FLAC__StreamEncoder *encoder, unsigned value)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_encoder_set_blocksize</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga21</anchor>
      <arglist>(FLAC__StreamEncoder *encoder, unsigned value)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_encoder_set_do_mid_side_stereo</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga22</anchor>
      <arglist>(FLAC__StreamEncoder *encoder, FLAC__bool value)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_encoder_set_loose_mid_side_stereo</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga23</anchor>
      <arglist>(FLAC__StreamEncoder *encoder, FLAC__bool value)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_encoder_set_apodization</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga24</anchor>
      <arglist>(FLAC__StreamEncoder *encoder, const char *specification)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_encoder_set_max_lpc_order</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga25</anchor>
      <arglist>(FLAC__StreamEncoder *encoder, unsigned value)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_encoder_set_qlp_coeff_precision</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga26</anchor>
      <arglist>(FLAC__StreamEncoder *encoder, unsigned value)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_encoder_set_do_qlp_coeff_prec_search</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga27</anchor>
      <arglist>(FLAC__StreamEncoder *encoder, FLAC__bool value)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_encoder_set_do_escape_coding</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga28</anchor>
      <arglist>(FLAC__StreamEncoder *encoder, FLAC__bool value)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_encoder_set_do_exhaustive_model_search</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga29</anchor>
      <arglist>(FLAC__StreamEncoder *encoder, FLAC__bool value)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_encoder_set_min_residual_partition_order</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga30</anchor>
      <arglist>(FLAC__StreamEncoder *encoder, unsigned value)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_encoder_set_max_residual_partition_order</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga31</anchor>
      <arglist>(FLAC__StreamEncoder *encoder, unsigned value)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_encoder_set_rice_parameter_search_dist</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga32</anchor>
      <arglist>(FLAC__StreamEncoder *encoder, unsigned value)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_encoder_set_total_samples_estimate</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga33</anchor>
      <arglist>(FLAC__StreamEncoder *encoder, FLAC__uint64 value)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_encoder_set_metadata</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga34</anchor>
      <arglist>(FLAC__StreamEncoder *encoder, FLAC__StreamMetadata **metadata, unsigned num_blocks)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__StreamEncoderState</type>
      <name>FLAC__stream_encoder_get_state</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga35</anchor>
      <arglist>(const FLAC__StreamEncoder *encoder)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__StreamDecoderState</type>
      <name>FLAC__stream_encoder_get_verify_decoder_state</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga36</anchor>
      <arglist>(const FLAC__StreamEncoder *encoder)</arglist>
    </member>
    <member kind="function">
      <type>const char *</type>
      <name>FLAC__stream_encoder_get_resolved_state_string</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga37</anchor>
      <arglist>(const FLAC__StreamEncoder *encoder)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>FLAC__stream_encoder_get_verify_decoder_error_stats</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga38</anchor>
      <arglist>(const FLAC__StreamEncoder *encoder, FLAC__uint64 *absolute_sample, unsigned *frame_number, unsigned *channel, unsigned *sample, FLAC__int32 *expected, FLAC__int32 *got)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_encoder_get_verify</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga39</anchor>
      <arglist>(const FLAC__StreamEncoder *encoder)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_encoder_get_streamable_subset</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga40</anchor>
      <arglist>(const FLAC__StreamEncoder *encoder)</arglist>
    </member>
    <member kind="function">
      <type>unsigned</type>
      <name>FLAC__stream_encoder_get_channels</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga41</anchor>
      <arglist>(const FLAC__StreamEncoder *encoder)</arglist>
    </member>
    <member kind="function">
      <type>unsigned</type>
      <name>FLAC__stream_encoder_get_bits_per_sample</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga42</anchor>
      <arglist>(const FLAC__StreamEncoder *encoder)</arglist>
    </member>
    <member kind="function">
      <type>unsigned</type>
      <name>FLAC__stream_encoder_get_sample_rate</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga43</anchor>
      <arglist>(const FLAC__StreamEncoder *encoder)</arglist>
    </member>
    <member kind="function">
      <type>unsigned</type>
      <name>FLAC__stream_encoder_get_blocksize</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga44</anchor>
      <arglist>(const FLAC__StreamEncoder *encoder)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_encoder_get_do_mid_side_stereo</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga45</anchor>
      <arglist>(const FLAC__StreamEncoder *encoder)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_encoder_get_loose_mid_side_stereo</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga46</anchor>
      <arglist>(const FLAC__StreamEncoder *encoder)</arglist>
    </member>
    <member kind="function">
      <type>unsigned</type>
      <name>FLAC__stream_encoder_get_max_lpc_order</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga47</anchor>
      <arglist>(const FLAC__StreamEncoder *encoder)</arglist>
    </member>
    <member kind="function">
      <type>unsigned</type>
      <name>FLAC__stream_encoder_get_qlp_coeff_precision</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga48</anchor>
      <arglist>(const FLAC__StreamEncoder *encoder)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_encoder_get_do_qlp_coeff_prec_search</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga49</anchor>
      <arglist>(const FLAC__StreamEncoder *encoder)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_encoder_get_do_escape_coding</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga50</anchor>
      <arglist>(const FLAC__StreamEncoder *encoder)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_encoder_get_do_exhaustive_model_search</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga51</anchor>
      <arglist>(const FLAC__StreamEncoder *encoder)</arglist>
    </member>
    <member kind="function">
      <type>unsigned</type>
      <name>FLAC__stream_encoder_get_min_residual_partition_order</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga52</anchor>
      <arglist>(const FLAC__StreamEncoder *encoder)</arglist>
    </member>
    <member kind="function">
      <type>unsigned</type>
      <name>FLAC__stream_encoder_get_max_residual_partition_order</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga53</anchor>
      <arglist>(const FLAC__StreamEncoder *encoder)</arglist>
    </member>
    <member kind="function">
      <type>unsigned</type>
      <name>FLAC__stream_encoder_get_rice_parameter_search_dist</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga54</anchor>
      <arglist>(const FLAC__StreamEncoder *encoder)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__uint64</type>
      <name>FLAC__stream_encoder_get_total_samples_estimate</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga55</anchor>
      <arglist>(const FLAC__StreamEncoder *encoder)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__StreamEncoderInitStatus</type>
      <name>FLAC__stream_encoder_init_stream</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga56</anchor>
      <arglist>(FLAC__StreamEncoder *encoder, FLAC__StreamEncoderWriteCallback write_callback, FLAC__StreamEncoderSeekCallback seek_callback, FLAC__StreamEncoderTellCallback tell_callback, FLAC__StreamEncoderMetadataCallback metadata_callback, void *client_data)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__StreamEncoderInitStatus</type>
      <name>FLAC__stream_encoder_init_ogg_stream</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga57</anchor>
      <arglist>(FLAC__StreamEncoder *encoder, FLAC__StreamEncoderReadCallback read_callback, FLAC__StreamEncoderWriteCallback write_callback, FLAC__StreamEncoderSeekCallback seek_callback, FLAC__StreamEncoderTellCallback tell_callback, FLAC__StreamEncoderMetadataCallback metadata_callback, void *client_data)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__StreamEncoderInitStatus</type>
      <name>FLAC__stream_encoder_init_FILE</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga58</anchor>
      <arglist>(FLAC__StreamEncoder *encoder, FILE *file, FLAC__StreamEncoderProgressCallback progress_callback, void *client_data)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__StreamEncoderInitStatus</type>
      <name>FLAC__stream_encoder_init_ogg_FILE</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga59</anchor>
      <arglist>(FLAC__StreamEncoder *encoder, FILE *file, FLAC__StreamEncoderProgressCallback progress_callback, void *client_data)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__StreamEncoderInitStatus</type>
      <name>FLAC__stream_encoder_init_file</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga60</anchor>
      <arglist>(FLAC__StreamEncoder *encoder, const char *filename, FLAC__StreamEncoderProgressCallback progress_callback, void *client_data)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__StreamEncoderInitStatus</type>
      <name>FLAC__stream_encoder_init_ogg_file</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga61</anchor>
      <arglist>(FLAC__StreamEncoder *encoder, const char *filename, FLAC__StreamEncoderProgressCallback progress_callback, void *client_data)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_encoder_finish</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga62</anchor>
      <arglist>(FLAC__StreamEncoder *encoder)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_encoder_process</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga63</anchor>
      <arglist>(FLAC__StreamEncoder *encoder, const FLAC__int32 *const buffer[], unsigned samples)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_encoder_process_interleaved</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga64</anchor>
      <arglist>(FLAC__StreamEncoder *encoder, const FLAC__int32 buffer[], unsigned samples)</arglist>
    </member>
    <member kind="variable">
      <type>const char *const</type>
      <name>FLAC__StreamEncoderStateString</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga0</anchor>
      <arglist>[]</arglist>
    </member>
    <member kind="variable">
      <type>const char *const</type>
      <name>FLAC__StreamEncoderInitStatusString</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga1</anchor>
      <arglist>[]</arglist>
    </member>
    <member kind="variable">
      <type>const char *const</type>
      <name>FLAC__StreamEncoderReadStatusString</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga2</anchor>
      <arglist>[]</arglist>
    </member>
    <member kind="variable">
      <type>const char *const</type>
      <name>FLAC__StreamEncoderWriteStatusString</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga3</anchor>
      <arglist>[]</arglist>
    </member>
    <member kind="variable">
      <type>const char *const</type>
      <name>FLAC__StreamEncoderSeekStatusString</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga4</anchor>
      <arglist>[]</arglist>
    </member>
    <member kind="variable">
      <type>const char *const</type>
      <name>FLAC__StreamEncoderTellStatusString</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga5</anchor>
      <arglist>[]</arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>FLAC__EntropyCodingMethod</name>
    <filename>structFLAC____EntropyCodingMethod.html</filename>
    <member kind="variable">
      <type>FLAC__EntropyCodingMethodType</type>
      <name>type</name>
      <anchorfile>structFLAC____EntropyCodingMethod.html</anchorfile>
      <anchor>FLAC____EntropyCodingMethodo0</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>FLAC__EntropyCodingMethod::@0</type>
      <name>data</name>
      <anchorfile>structFLAC____EntropyCodingMethod.html</anchorfile>
      <anchor>FLAC____EntropyCodingMethodo2</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>FLAC__EntropyCodingMethod_PartitionedRice</type>
      <name>partitioned_rice</name>
      <anchorfile>unionFLAC____EntropyCodingMethod_1_1@0.html</anchorfile>
      <anchor>FLAC____EntropyCodingMethod_1_1@0o0</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>FLAC__EntropyCodingMethod_PartitionedRice</name>
    <filename>structFLAC____EntropyCodingMethod__PartitionedRice.html</filename>
    <member kind="variable">
      <type>unsigned</type>
      <name>order</name>
      <anchorfile>structFLAC____EntropyCodingMethod__PartitionedRice.html</anchorfile>
      <anchor>FLAC____EntropyCodingMethod__PartitionedRiceo0</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const FLAC__EntropyCodingMethod_PartitionedRiceContents *</type>
      <name>contents</name>
      <anchorfile>structFLAC____EntropyCodingMethod__PartitionedRice.html</anchorfile>
      <anchor>FLAC____EntropyCodingMethod__PartitionedRiceo1</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>FLAC__EntropyCodingMethod_PartitionedRiceContents</name>
    <filename>structFLAC____EntropyCodingMethod__PartitionedRiceContents.html</filename>
    <member kind="variable">
      <type>unsigned *</type>
      <name>parameters</name>
      <anchorfile>structFLAC____EntropyCodingMethod__PartitionedRiceContents.html</anchorfile>
      <anchor>FLAC____EntropyCodingMethod__PartitionedRiceContentso0</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>unsigned *</type>
      <name>raw_bits</name>
      <anchorfile>structFLAC____EntropyCodingMethod__PartitionedRiceContents.html</anchorfile>
      <anchor>FLAC____EntropyCodingMethod__PartitionedRiceContentso1</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>unsigned</type>
      <name>capacity_by_order</name>
      <anchorfile>structFLAC____EntropyCodingMethod__PartitionedRiceContents.html</anchorfile>
      <anchor>FLAC____EntropyCodingMethod__PartitionedRiceContentso2</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>FLAC__Frame</name>
    <filename>structFLAC____Frame.html</filename>
    <member kind="variable">
      <type>FLAC__FrameHeader</type>
      <name>header</name>
      <anchorfile>structFLAC____Frame.html</anchorfile>
      <anchor>FLAC____Frameo0</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>FLAC__Subframe</type>
      <name>subframes</name>
      <anchorfile>structFLAC____Frame.html</anchorfile>
      <anchor>FLAC____Frameo1</anchor>
      <arglist>[FLAC__MAX_CHANNELS]</arglist>
    </member>
    <member kind="variable">
      <type>FLAC__FrameFooter</type>
      <name>footer</name>
      <anchorfile>structFLAC____Frame.html</anchorfile>
      <anchor>FLAC____Frameo2</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>FLAC__FrameFooter</name>
    <filename>structFLAC____FrameFooter.html</filename>
    <member kind="variable">
      <type>FLAC__uint16</type>
      <name>crc</name>
      <anchorfile>structFLAC____FrameFooter.html</anchorfile>
      <anchor>FLAC____FrameFootero0</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>FLAC__FrameHeader</name>
    <filename>structFLAC____FrameHeader.html</filename>
    <member kind="variable">
      <type>unsigned</type>
      <name>blocksize</name>
      <anchorfile>structFLAC____FrameHeader.html</anchorfile>
      <anchor>FLAC____FrameHeadero0</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>unsigned</type>
      <name>sample_rate</name>
      <anchorfile>structFLAC____FrameHeader.html</anchorfile>
      <anchor>FLAC____FrameHeadero1</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>unsigned</type>
      <name>channels</name>
      <anchorfile>structFLAC____FrameHeader.html</anchorfile>
      <anchor>FLAC____FrameHeadero2</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>FLAC__ChannelAssignment</type>
      <name>channel_assignment</name>
      <anchorfile>structFLAC____FrameHeader.html</anchorfile>
      <anchor>FLAC____FrameHeadero3</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>unsigned</type>
      <name>bits_per_sample</name>
      <anchorfile>structFLAC____FrameHeader.html</anchorfile>
      <anchor>FLAC____FrameHeadero4</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>FLAC__FrameNumberType</type>
      <name>number_type</name>
      <anchorfile>structFLAC____FrameHeader.html</anchorfile>
      <anchor>FLAC____FrameHeadero5</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>FLAC__FrameHeader::@2</type>
      <name>number</name>
      <anchorfile>structFLAC____FrameHeader.html</anchorfile>
      <anchor>FLAC____FrameHeadero8</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>FLAC__uint32</type>
      <name>frame_number</name>
      <anchorfile>unionFLAC____FrameHeader_1_1@2.html</anchorfile>
      <anchor>FLAC____FrameHeader_1_1@2o0</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>FLAC__uint64</type>
      <name>sample_number</name>
      <anchorfile>unionFLAC____FrameHeader_1_1@2.html</anchorfile>
      <anchor>FLAC____FrameHeader_1_1@2o1</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>FLAC__uint8</type>
      <name>crc</name>
      <anchorfile>structFLAC____FrameHeader.html</anchorfile>
      <anchor>FLAC____FrameHeadero9</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>FLAC__IOCallbacks</name>
    <filename>structFLAC____IOCallbacks.html</filename>
    <member kind="variable">
      <type>FLAC__IOCallback_Read</type>
      <name>read</name>
      <anchorfile>structFLAC____IOCallbacks.html</anchorfile>
      <anchor>FLAC____IOCallbackso0</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>FLAC__IOCallback_Write</type>
      <name>write</name>
      <anchorfile>structFLAC____IOCallbacks.html</anchorfile>
      <anchor>FLAC____IOCallbackso1</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>FLAC__IOCallback_Seek</type>
      <name>seek</name>
      <anchorfile>structFLAC____IOCallbacks.html</anchorfile>
      <anchor>FLAC____IOCallbackso2</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>FLAC__IOCallback_Tell</type>
      <name>tell</name>
      <anchorfile>structFLAC____IOCallbacks.html</anchorfile>
      <anchor>FLAC____IOCallbackso3</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>FLAC__IOCallback_Eof</type>
      <name>eof</name>
      <anchorfile>structFLAC____IOCallbacks.html</anchorfile>
      <anchor>FLAC____IOCallbackso4</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>FLAC__IOCallback_Close</type>
      <name>close</name>
      <anchorfile>structFLAC____IOCallbacks.html</anchorfile>
      <anchor>FLAC____IOCallbackso5</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>FLAC__StreamDecoder</name>
    <filename>structFLAC____StreamDecoder.html</filename>
    <member kind="variable">
      <type>FLAC__StreamDecoderProtected *</type>
      <name>protected_</name>
      <anchorfile>structFLAC____StreamDecoder.html</anchorfile>
      <anchor>FLAC____StreamDecodero0</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>FLAC__StreamDecoderPrivate *</type>
      <name>private_</name>
      <anchorfile>structFLAC____StreamDecoder.html</anchorfile>
      <anchor>FLAC____StreamDecodero1</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>FLAC__StreamEncoder</name>
    <filename>structFLAC____StreamEncoder.html</filename>
    <member kind="variable">
      <type>FLAC__StreamEncoderProtected *</type>
      <name>protected_</name>
      <anchorfile>structFLAC____StreamEncoder.html</anchorfile>
      <anchor>FLAC____StreamEncodero0</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>FLAC__StreamEncoderPrivate *</type>
      <name>private_</name>
      <anchorfile>structFLAC____StreamEncoder.html</anchorfile>
      <anchor>FLAC____StreamEncodero1</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>FLAC__StreamMetadata</name>
    <filename>structFLAC____StreamMetadata.html</filename>
    <member kind="variable">
      <type>FLAC__MetadataType</type>
      <name>type</name>
      <anchorfile>structFLAC____StreamMetadata.html</anchorfile>
      <anchor>FLAC____StreamMetadatao0</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>FLAC__bool</type>
      <name>is_last</name>
      <anchorfile>structFLAC____StreamMetadata.html</anchorfile>
      <anchor>FLAC____StreamMetadatao1</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>unsigned</type>
      <name>length</name>
      <anchorfile>structFLAC____StreamMetadata.html</anchorfile>
      <anchor>FLAC____StreamMetadatao2</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>FLAC__StreamMetadata::@3</type>
      <name>data</name>
      <anchorfile>structFLAC____StreamMetadata.html</anchorfile>
      <anchor>FLAC____StreamMetadatao11</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>FLAC__StreamMetadata_StreamInfo</type>
      <name>stream_info</name>
      <anchorfile>unionFLAC____StreamMetadata_1_1@3.html</anchorfile>
      <anchor>FLAC____StreamMetadata_1_1@3o0</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>FLAC__StreamMetadata_Padding</type>
      <name>padding</name>
      <anchorfile>unionFLAC____StreamMetadata_1_1@3.html</anchorfile>
      <anchor>FLAC____StreamMetadata_1_1@3o1</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>FLAC__StreamMetadata_Application</type>
      <name>application</name>
      <anchorfile>unionFLAC____StreamMetadata_1_1@3.html</anchorfile>
      <anchor>FLAC____StreamMetadata_1_1@3o2</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>FLAC__StreamMetadata_SeekTable</type>
      <name>seek_table</name>
      <anchorfile>unionFLAC____StreamMetadata_1_1@3.html</anchorfile>
      <anchor>FLAC____StreamMetadata_1_1@3o3</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>FLAC__StreamMetadata_VorbisComment</type>
      <name>vorbis_comment</name>
      <anchorfile>unionFLAC____StreamMetadata_1_1@3.html</anchorfile>
      <anchor>FLAC____StreamMetadata_1_1@3o4</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>FLAC__StreamMetadata_CueSheet</type>
      <name>cue_sheet</name>
      <anchorfile>unionFLAC____StreamMetadata_1_1@3.html</anchorfile>
      <anchor>FLAC____StreamMetadata_1_1@3o5</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>FLAC__StreamMetadata_Picture</type>
      <name>picture</name>
      <anchorfile>unionFLAC____StreamMetadata_1_1@3.html</anchorfile>
      <anchor>FLAC____StreamMetadata_1_1@3o6</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>FLAC__StreamMetadata_Unknown</type>
      <name>unknown</name>
      <anchorfile>unionFLAC____StreamMetadata_1_1@3.html</anchorfile>
      <anchor>FLAC____StreamMetadata_1_1@3o7</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>FLAC__StreamMetadata_Application</name>
    <filename>structFLAC____StreamMetadata__Application.html</filename>
    <member kind="variable">
      <type>FLAC__byte</type>
      <name>id</name>
      <anchorfile>structFLAC____StreamMetadata__Application.html</anchorfile>
      <anchor>FLAC____StreamMetadata__Applicationo0</anchor>
      <arglist>[4]</arglist>
    </member>
    <member kind="variable">
      <type>FLAC__byte *</type>
      <name>data</name>
      <anchorfile>structFLAC____StreamMetadata__Application.html</anchorfile>
      <anchor>FLAC____StreamMetadata__Applicationo1</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>FLAC__StreamMetadata_CueSheet</name>
    <filename>structFLAC____StreamMetadata__CueSheet.html</filename>
    <member kind="variable">
      <type>char</type>
      <name>media_catalog_number</name>
      <anchorfile>structFLAC____StreamMetadata__CueSheet.html</anchorfile>
      <anchor>FLAC____StreamMetadata__CueSheeto0</anchor>
      <arglist>[129]</arglist>
    </member>
    <member kind="variable">
      <type>FLAC__uint64</type>
      <name>lead_in</name>
      <anchorfile>structFLAC____StreamMetadata__CueSheet.html</anchorfile>
      <anchor>FLAC____StreamMetadata__CueSheeto1</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>FLAC__bool</type>
      <name>is_cd</name>
      <anchorfile>structFLAC____StreamMetadata__CueSheet.html</anchorfile>
      <anchor>FLAC____StreamMetadata__CueSheeto2</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>unsigned</type>
      <name>num_tracks</name>
      <anchorfile>structFLAC____StreamMetadata__CueSheet.html</anchorfile>
      <anchor>FLAC____StreamMetadata__CueSheeto3</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>FLAC__StreamMetadata_CueSheet_Track *</type>
      <name>tracks</name>
      <anchorfile>structFLAC____StreamMetadata__CueSheet.html</anchorfile>
      <anchor>FLAC____StreamMetadata__CueSheeto4</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>FLAC__StreamMetadata_CueSheet_Index</name>
    <filename>structFLAC____StreamMetadata__CueSheet__Index.html</filename>
    <member kind="variable">
      <type>FLAC__uint64</type>
      <name>offset</name>
      <anchorfile>structFLAC____StreamMetadata__CueSheet__Index.html</anchorfile>
      <anchor>FLAC____StreamMetadata__CueSheet__Indexo0</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>FLAC__byte</type>
      <name>number</name>
      <anchorfile>structFLAC____StreamMetadata__CueSheet__Index.html</anchorfile>
      <anchor>FLAC____StreamMetadata__CueSheet__Indexo1</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>FLAC__StreamMetadata_CueSheet_Track</name>
    <filename>structFLAC____StreamMetadata__CueSheet__Track.html</filename>
    <member kind="variable">
      <type>FLAC__uint64</type>
      <name>offset</name>
      <anchorfile>structFLAC____StreamMetadata__CueSheet__Track.html</anchorfile>
      <anchor>FLAC____StreamMetadata__CueSheet__Tracko0</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>FLAC__byte</type>
      <name>number</name>
      <anchorfile>structFLAC____StreamMetadata__CueSheet__Track.html</anchorfile>
      <anchor>FLAC____StreamMetadata__CueSheet__Tracko1</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>char</type>
      <name>isrc</name>
      <anchorfile>structFLAC____StreamMetadata__CueSheet__Track.html</anchorfile>
      <anchor>FLAC____StreamMetadata__CueSheet__Tracko2</anchor>
      <arglist>[13]</arglist>
    </member>
    <member kind="variable">
      <type>unsigned</type>
      <name>type</name>
      <anchorfile>structFLAC____StreamMetadata__CueSheet__Track.html</anchorfile>
      <anchor>FLAC____StreamMetadata__CueSheet__Tracko3</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>unsigned</type>
      <name>pre_emphasis</name>
      <anchorfile>structFLAC____StreamMetadata__CueSheet__Track.html</anchorfile>
      <anchor>FLAC____StreamMetadata__CueSheet__Tracko4</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>FLAC__byte</type>
      <name>num_indices</name>
      <anchorfile>structFLAC____StreamMetadata__CueSheet__Track.html</anchorfile>
      <anchor>FLAC____StreamMetadata__CueSheet__Tracko5</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>FLAC__StreamMetadata_CueSheet_Index *</type>
      <name>indices</name>
      <anchorfile>structFLAC____StreamMetadata__CueSheet__Track.html</anchorfile>
      <anchor>FLAC____StreamMetadata__CueSheet__Tracko6</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>FLAC__StreamMetadata_Padding</name>
    <filename>structFLAC____StreamMetadata__Padding.html</filename>
    <member kind="variable">
      <type>int</type>
      <name>dummy</name>
      <anchorfile>structFLAC____StreamMetadata__Padding.html</anchorfile>
      <anchor>FLAC____StreamMetadata__Paddingo0</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>FLAC__StreamMetadata_Picture</name>
    <filename>structFLAC____StreamMetadata__Picture.html</filename>
    <member kind="variable">
      <type>FLAC__StreamMetadata_Picture_Type</type>
      <name>type</name>
      <anchorfile>structFLAC____StreamMetadata__Picture.html</anchorfile>
      <anchor>FLAC____StreamMetadata__Pictureo0</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>char *</type>
      <name>mime_type</name>
      <anchorfile>structFLAC____StreamMetadata__Picture.html</anchorfile>
      <anchor>FLAC____StreamMetadata__Pictureo1</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>FLAC__byte *</type>
      <name>description</name>
      <anchorfile>structFLAC____StreamMetadata__Picture.html</anchorfile>
      <anchor>FLAC____StreamMetadata__Pictureo2</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>FLAC__uint32</type>
      <name>width</name>
      <anchorfile>structFLAC____StreamMetadata__Picture.html</anchorfile>
      <anchor>FLAC____StreamMetadata__Pictureo3</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>FLAC__uint32</type>
      <name>height</name>
      <anchorfile>structFLAC____StreamMetadata__Picture.html</anchorfile>
      <anchor>FLAC____StreamMetadata__Pictureo4</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>FLAC__uint32</type>
      <name>depth</name>
      <anchorfile>structFLAC____StreamMetadata__Picture.html</anchorfile>
      <anchor>FLAC____StreamMetadata__Pictureo5</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>FLAC__uint32</type>
      <name>colors</name>
      <anchorfile>structFLAC____StreamMetadata__Picture.html</anchorfile>
      <anchor>FLAC____StreamMetadata__Pictureo6</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>FLAC__uint32</type>
      <name>data_length</name>
      <anchorfile>structFLAC____StreamMetadata__Picture.html</anchorfile>
      <anchor>FLAC____StreamMetadata__Pictureo7</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>FLAC__byte *</type>
      <name>data</name>
      <anchorfile>structFLAC____StreamMetadata__Picture.html</anchorfile>
      <anchor>FLAC____StreamMetadata__Pictureo8</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>FLAC__StreamMetadata_SeekPoint</name>
    <filename>structFLAC____StreamMetadata__SeekPoint.html</filename>
    <member kind="variable">
      <type>FLAC__uint64</type>
      <name>sample_number</name>
      <anchorfile>structFLAC____StreamMetadata__SeekPoint.html</anchorfile>
      <anchor>FLAC____StreamMetadata__SeekPointo0</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>FLAC__uint64</type>
      <name>stream_offset</name>
      <anchorfile>structFLAC____StreamMetadata__SeekPoint.html</anchorfile>
      <anchor>FLAC____StreamMetadata__SeekPointo1</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>unsigned</type>
      <name>frame_samples</name>
      <anchorfile>structFLAC____StreamMetadata__SeekPoint.html</anchorfile>
      <anchor>FLAC____StreamMetadata__SeekPointo2</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>FLAC__StreamMetadata_SeekTable</name>
    <filename>structFLAC____StreamMetadata__SeekTable.html</filename>
    <member kind="variable">
      <type>unsigned</type>
      <name>num_points</name>
      <anchorfile>structFLAC____StreamMetadata__SeekTable.html</anchorfile>
      <anchor>FLAC____StreamMetadata__SeekTableo0</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>FLAC__StreamMetadata_SeekPoint *</type>
      <name>points</name>
      <anchorfile>structFLAC____StreamMetadata__SeekTable.html</anchorfile>
      <anchor>FLAC____StreamMetadata__SeekTableo1</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>FLAC__StreamMetadata_StreamInfo</name>
    <filename>structFLAC____StreamMetadata__StreamInfo.html</filename>
    <member kind="variable">
      <type>unsigned</type>
      <name>min_blocksize</name>
      <anchorfile>structFLAC____StreamMetadata__StreamInfo.html</anchorfile>
      <anchor>FLAC____StreamMetadata__StreamInfoo0</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>unsigned</type>
      <name>max_blocksize</name>
      <anchorfile>structFLAC____StreamMetadata__StreamInfo.html</anchorfile>
      <anchor>FLAC____StreamMetadata__StreamInfoo1</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>unsigned</type>
      <name>min_framesize</name>
      <anchorfile>structFLAC____StreamMetadata__StreamInfo.html</anchorfile>
      <anchor>FLAC____StreamMetadata__StreamInfoo2</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>unsigned</type>
      <name>max_framesize</name>
      <anchorfile>structFLAC____StreamMetadata__StreamInfo.html</anchorfile>
      <anchor>FLAC____StreamMetadata__StreamInfoo3</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>unsigned</type>
      <name>sample_rate</name>
      <anchorfile>structFLAC____StreamMetadata__StreamInfo.html</anchorfile>
      <anchor>FLAC____StreamMetadata__StreamInfoo4</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>unsigned</type>
      <name>channels</name>
      <anchorfile>structFLAC____StreamMetadata__StreamInfo.html</anchorfile>
      <anchor>FLAC____StreamMetadata__StreamInfoo5</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>unsigned</type>
      <name>bits_per_sample</name>
      <anchorfile>structFLAC____StreamMetadata__StreamInfo.html</anchorfile>
      <anchor>FLAC____StreamMetadata__StreamInfoo6</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>FLAC__uint64</type>
      <name>total_samples</name>
      <anchorfile>structFLAC____StreamMetadata__StreamInfo.html</anchorfile>
      <anchor>FLAC____StreamMetadata__StreamInfoo7</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>FLAC__byte</type>
      <name>md5sum</name>
      <anchorfile>structFLAC____StreamMetadata__StreamInfo.html</anchorfile>
      <anchor>FLAC____StreamMetadata__StreamInfoo8</anchor>
      <arglist>[16]</arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>FLAC__StreamMetadata_Unknown</name>
    <filename>structFLAC____StreamMetadata__Unknown.html</filename>
    <member kind="variable">
      <type>FLAC__byte *</type>
      <name>data</name>
      <anchorfile>structFLAC____StreamMetadata__Unknown.html</anchorfile>
      <anchor>FLAC____StreamMetadata__Unknowno0</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>FLAC__StreamMetadata_VorbisComment</name>
    <filename>structFLAC____StreamMetadata__VorbisComment.html</filename>
    <member kind="variable">
      <type>FLAC__StreamMetadata_VorbisComment_Entry</type>
      <name>vendor_string</name>
      <anchorfile>structFLAC____StreamMetadata__VorbisComment.html</anchorfile>
      <anchor>FLAC____StreamMetadata__VorbisCommento0</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>FLAC__uint32</type>
      <name>num_comments</name>
      <anchorfile>structFLAC____StreamMetadata__VorbisComment.html</anchorfile>
      <anchor>FLAC____StreamMetadata__VorbisCommento1</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>FLAC__StreamMetadata_VorbisComment_Entry *</type>
      <name>comments</name>
      <anchorfile>structFLAC____StreamMetadata__VorbisComment.html</anchorfile>
      <anchor>FLAC____StreamMetadata__VorbisCommento2</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>FLAC__StreamMetadata_VorbisComment_Entry</name>
    <filename>structFLAC____StreamMetadata__VorbisComment__Entry.html</filename>
    <member kind="variable">
      <type>FLAC__uint32</type>
      <name>length</name>
      <anchorfile>structFLAC____StreamMetadata__VorbisComment__Entry.html</anchorfile>
      <anchor>FLAC____StreamMetadata__VorbisComment__Entryo0</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>FLAC__byte *</type>
      <name>entry</name>
      <anchorfile>structFLAC____StreamMetadata__VorbisComment__Entry.html</anchorfile>
      <anchor>FLAC____StreamMetadata__VorbisComment__Entryo1</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>FLAC__Subframe</name>
    <filename>structFLAC____Subframe.html</filename>
    <member kind="variable">
      <type>FLAC__SubframeType</type>
      <name>type</name>
      <anchorfile>structFLAC____Subframe.html</anchorfile>
      <anchor>FLAC____Subframeo0</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>FLAC__Subframe::@1</type>
      <name>data</name>
      <anchorfile>structFLAC____Subframe.html</anchorfile>
      <anchor>FLAC____Subframeo5</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>FLAC__Subframe_Constant</type>
      <name>constant</name>
      <anchorfile>unionFLAC____Subframe_1_1@1.html</anchorfile>
      <anchor>FLAC____Subframe_1_1@1o0</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>FLAC__Subframe_Fixed</type>
      <name>fixed</name>
      <anchorfile>unionFLAC____Subframe_1_1@1.html</anchorfile>
      <anchor>FLAC____Subframe_1_1@1o1</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>FLAC__Subframe_LPC</type>
      <name>lpc</name>
      <anchorfile>unionFLAC____Subframe_1_1@1.html</anchorfile>
      <anchor>FLAC____Subframe_1_1@1o2</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>FLAC__Subframe_Verbatim</type>
      <name>verbatim</name>
      <anchorfile>unionFLAC____Subframe_1_1@1.html</anchorfile>
      <anchor>FLAC____Subframe_1_1@1o3</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>unsigned</type>
      <name>wasted_bits</name>
      <anchorfile>structFLAC____Subframe.html</anchorfile>
      <anchor>FLAC____Subframeo6</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>FLAC__Subframe_Constant</name>
    <filename>structFLAC____Subframe__Constant.html</filename>
    <member kind="variable">
      <type>FLAC__int32</type>
      <name>value</name>
      <anchorfile>structFLAC____Subframe__Constant.html</anchorfile>
      <anchor>FLAC____Subframe__Constanto0</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>FLAC__Subframe_Fixed</name>
    <filename>structFLAC____Subframe__Fixed.html</filename>
    <member kind="variable">
      <type>FLAC__EntropyCodingMethod</type>
      <name>entropy_coding_method</name>
      <anchorfile>structFLAC____Subframe__Fixed.html</anchorfile>
      <anchor>FLAC____Subframe__Fixedo0</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>unsigned</type>
      <name>order</name>
      <anchorfile>structFLAC____Subframe__Fixed.html</anchorfile>
      <anchor>FLAC____Subframe__Fixedo1</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>FLAC__int32</type>
      <name>warmup</name>
      <anchorfile>structFLAC____Subframe__Fixed.html</anchorfile>
      <anchor>FLAC____Subframe__Fixedo2</anchor>
      <arglist>[FLAC__MAX_FIXED_ORDER]</arglist>
    </member>
    <member kind="variable">
      <type>const FLAC__int32 *</type>
      <name>residual</name>
      <anchorfile>structFLAC____Subframe__Fixed.html</anchorfile>
      <anchor>FLAC____Subframe__Fixedo3</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>FLAC__Subframe_LPC</name>
    <filename>structFLAC____Subframe__LPC.html</filename>
    <member kind="variable">
      <type>FLAC__EntropyCodingMethod</type>
      <name>entropy_coding_method</name>
      <anchorfile>structFLAC____Subframe__LPC.html</anchorfile>
      <anchor>FLAC____Subframe__LPCo0</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>unsigned</type>
      <name>order</name>
      <anchorfile>structFLAC____Subframe__LPC.html</anchorfile>
      <anchor>FLAC____Subframe__LPCo1</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>unsigned</type>
      <name>qlp_coeff_precision</name>
      <anchorfile>structFLAC____Subframe__LPC.html</anchorfile>
      <anchor>FLAC____Subframe__LPCo2</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>quantization_level</name>
      <anchorfile>structFLAC____Subframe__LPC.html</anchorfile>
      <anchor>FLAC____Subframe__LPCo3</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>FLAC__int32</type>
      <name>qlp_coeff</name>
      <anchorfile>structFLAC____Subframe__LPC.html</anchorfile>
      <anchor>FLAC____Subframe__LPCo4</anchor>
      <arglist>[FLAC__MAX_LPC_ORDER]</arglist>
    </member>
    <member kind="variable">
      <type>FLAC__int32</type>
      <name>warmup</name>
      <anchorfile>structFLAC____Subframe__LPC.html</anchorfile>
      <anchor>FLAC____Subframe__LPCo5</anchor>
      <arglist>[FLAC__MAX_LPC_ORDER]</arglist>
    </member>
    <member kind="variable">
      <type>const FLAC__int32 *</type>
      <name>residual</name>
      <anchorfile>structFLAC____Subframe__LPC.html</anchorfile>
      <anchor>FLAC____Subframe__LPCo6</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>FLAC__Subframe_Verbatim</name>
    <filename>structFLAC____Subframe__Verbatim.html</filename>
    <member kind="variable">
      <type>const FLAC__int32 *</type>
      <name>data</name>
      <anchorfile>structFLAC____Subframe__Verbatim.html</anchorfile>
      <anchor>FLAC____Subframe__Verbatimo0</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="group">
    <name>porting</name>
    <title>Porting Guide for New Versions</title>
    <filename>group__porting.html</filename>
    <subgroup>porting_1_1_2_to_1_1_3</subgroup>
    <subgroup>porting_1_1_3_to_1_1_4</subgroup>
    <subgroup>porting_1_1_4_to_1_2_0</subgroup>
  </compound>
  <compound kind="group">
    <name>porting_1_1_2_to_1_1_3</name>
    <title>Porting from FLAC 1.1.2 to 1.1.3</title>
    <filename>group__porting__1__1__2__to__1__1__3.html</filename>
  </compound>
  <compound kind="group">
    <name>porting_1_1_3_to_1_1_4</name>
    <title>Porting from FLAC 1.1.3 to 1.1.4</title>
    <filename>group__porting__1__1__3__to__1__1__4.html</filename>
  </compound>
  <compound kind="group">
    <name>porting_1_1_4_to_1_2_0</name>
    <title>Porting from FLAC 1.1.4 to 1.2.0</title>
    <filename>group__porting__1__1__4__to__1__2__0.html</filename>
  </compound>
  <compound kind="group">
    <name>flac</name>
    <title>FLAC C API</title>
    <filename>group__flac.html</filename>
    <subgroup>flac_callbacks</subgroup>
    <subgroup>flac_export</subgroup>
    <subgroup>flac_format</subgroup>
    <subgroup>flac_metadata</subgroup>
    <subgroup>flac_decoder</subgroup>
    <subgroup>flac_encoder</subgroup>
  </compound>
  <compound kind="group">
    <name>flac_callbacks</name>
    <title>FLAC/callback.h: I/O callback structures</title>
    <filename>group__flac__callbacks.html</filename>
    <class kind="struct">FLAC__IOCallbacks</class>
    <member kind="typedef">
      <type>void *</type>
      <name>FLAC__IOHandle</name>
      <anchorfile>group__flac__callbacks.html</anchorfile>
      <anchor>ga0</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>size_t(*</type>
      <name>FLAC__IOCallback_Read</name>
      <anchorfile>group__flac__callbacks.html</anchorfile>
      <anchor>ga1</anchor>
      <arglist>)(void *ptr, size_t size, size_t nmemb, FLAC__IOHandle handle)</arglist>
    </member>
    <member kind="typedef">
      <type>size_t(*</type>
      <name>FLAC__IOCallback_Write</name>
      <anchorfile>group__flac__callbacks.html</anchorfile>
      <anchor>ga2</anchor>
      <arglist>)(const void *ptr, size_t size, size_t nmemb, FLAC__IOHandle handle)</arglist>
    </member>
    <member kind="typedef">
      <type>int(*</type>
      <name>FLAC__IOCallback_Seek</name>
      <anchorfile>group__flac__callbacks.html</anchorfile>
      <anchor>ga3</anchor>
      <arglist>)(FLAC__IOHandle handle, FLAC__int64 offset, int whence)</arglist>
    </member>
    <member kind="typedef">
      <type>FLAC__int64(*</type>
      <name>FLAC__IOCallback_Tell</name>
      <anchorfile>group__flac__callbacks.html</anchorfile>
      <anchor>ga4</anchor>
      <arglist>)(FLAC__IOHandle handle)</arglist>
    </member>
    <member kind="typedef">
      <type>int(*</type>
      <name>FLAC__IOCallback_Eof</name>
      <anchorfile>group__flac__callbacks.html</anchorfile>
      <anchor>ga5</anchor>
      <arglist>)(FLAC__IOHandle handle)</arglist>
    </member>
    <member kind="typedef">
      <type>int(*</type>
      <name>FLAC__IOCallback_Close</name>
      <anchorfile>group__flac__callbacks.html</anchorfile>
      <anchor>ga6</anchor>
      <arglist>)(FLAC__IOHandle handle)</arglist>
    </member>
  </compound>
  <compound kind="group">
    <name>flac_export</name>
    <title>FLAC/export.h: export symbols</title>
    <filename>group__flac__export.html</filename>
    <member kind="define">
      <type>#define</type>
      <name>FLAC_API</name>
      <anchorfile>group__flac__export.html</anchorfile>
      <anchor>ga1</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FLAC_API_VERSION_CURRENT</name>
      <anchorfile>group__flac__export.html</anchorfile>
      <anchor>ga2</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FLAC_API_VERSION_REVISION</name>
      <anchorfile>group__flac__export.html</anchorfile>
      <anchor>ga3</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FLAC_API_VERSION_AGE</name>
      <anchorfile>group__flac__export.html</anchorfile>
      <anchor>ga4</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>FLAC_API_SUPPORTS_OGG_FLAC</name>
      <anchorfile>group__flac__export.html</anchorfile>
      <anchor>ga0</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="group">
    <name>flac_format</name>
    <title>FLAC/format.h: format components</title>
    <filename>group__flac__format.html</filename>
    <class kind="struct">FLAC__EntropyCodingMethod_PartitionedRiceContents</class>
    <class kind="struct">FLAC__EntropyCodingMethod_PartitionedRice</class>
    <class kind="struct">FLAC__EntropyCodingMethod</class>
    <class kind="struct">FLAC__Subframe_Constant</class>
    <class kind="struct">FLAC__Subframe_Verbatim</class>
    <class kind="struct">FLAC__Subframe_Fixed</class>
    <class kind="struct">FLAC__Subframe_LPC</class>
    <class kind="struct">FLAC__Subframe</class>
    <class kind="struct">FLAC__FrameHeader</class>
    <class kind="struct">FLAC__FrameFooter</class>
    <class kind="struct">FLAC__Frame</class>
    <class kind="struct">FLAC__StreamMetadata_StreamInfo</class>
    <class kind="struct">FLAC__StreamMetadata_Padding</class>
    <class kind="struct">FLAC__StreamMetadata_Application</class>
    <class kind="struct">FLAC__StreamMetadata_SeekPoint</class>
    <class kind="struct">FLAC__StreamMetadata_SeekTable</class>
    <class kind="struct">FLAC__StreamMetadata_VorbisComment_Entry</class>
    <class kind="struct">FLAC__StreamMetadata_VorbisComment</class>
    <class kind="struct">FLAC__StreamMetadata_CueSheet_Index</class>
    <class kind="struct">FLAC__StreamMetadata_CueSheet_Track</class>
    <class kind="struct">FLAC__StreamMetadata_CueSheet</class>
    <class kind="struct">FLAC__StreamMetadata_Picture</class>
    <class kind="struct">FLAC__StreamMetadata_Unknown</class>
    <class kind="struct">FLAC__StreamMetadata</class>
    <member kind="define">
      <type>#define</type>
      <name>FLAC__MAX_METADATA_TYPE_CODE</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga89</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FLAC__MIN_BLOCK_SIZE</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga90</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FLAC__MAX_BLOCK_SIZE</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga91</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FLAC__SUBSET_MAX_BLOCK_SIZE_48000HZ</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga92</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FLAC__MAX_CHANNELS</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga93</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FLAC__MIN_BITS_PER_SAMPLE</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga94</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FLAC__MAX_BITS_PER_SAMPLE</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga95</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FLAC__REFERENCE_CODEC_MAX_BITS_PER_SAMPLE</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga96</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FLAC__MAX_SAMPLE_RATE</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga97</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FLAC__MAX_LPC_ORDER</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga98</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FLAC__SUBSET_MAX_LPC_ORDER_48000HZ</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga99</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FLAC__MIN_QLP_COEFF_PRECISION</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga100</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FLAC__MAX_QLP_COEFF_PRECISION</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga101</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FLAC__MAX_FIXED_ORDER</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga102</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FLAC__MAX_RICE_PARTITION_ORDER</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga103</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FLAC__SUBSET_MAX_RICE_PARTITION_ORDER</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga104</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FLAC__STREAM_SYNC_LENGTH</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga105</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FLAC__STREAM_METADATA_STREAMINFO_LENGTH</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga106</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FLAC__STREAM_METADATA_SEEKPOINT_LENGTH</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga107</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FLAC__STREAM_METADATA_HEADER_LENGTH</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga108</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <name>FLAC__EntropyCodingMethodType</name>
      <anchor>ga109</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE</name>
      <anchor>gga109a100</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE2</name>
      <anchor>gga109a101</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <name>FLAC__SubframeType</name>
      <anchor>ga110</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__SUBFRAME_TYPE_CONSTANT</name>
      <anchor>gga110a102</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__SUBFRAME_TYPE_VERBATIM</name>
      <anchor>gga110a103</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__SUBFRAME_TYPE_FIXED</name>
      <anchor>gga110a104</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__SUBFRAME_TYPE_LPC</name>
      <anchor>gga110a105</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <name>FLAC__ChannelAssignment</name>
      <anchor>ga111</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__CHANNEL_ASSIGNMENT_INDEPENDENT</name>
      <anchor>gga111a106</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__CHANNEL_ASSIGNMENT_LEFT_SIDE</name>
      <anchor>gga111a107</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__CHANNEL_ASSIGNMENT_RIGHT_SIDE</name>
      <anchor>gga111a108</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__CHANNEL_ASSIGNMENT_MID_SIDE</name>
      <anchor>gga111a109</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <name>FLAC__FrameNumberType</name>
      <anchor>ga112</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__FRAME_NUMBER_TYPE_FRAME_NUMBER</name>
      <anchor>gga112a110</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__FRAME_NUMBER_TYPE_SAMPLE_NUMBER</name>
      <anchor>gga112a111</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <name>FLAC__MetadataType</name>
      <anchor>ga113</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_TYPE_STREAMINFO</name>
      <anchor>gga113a112</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_TYPE_PADDING</name>
      <anchor>gga113a113</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_TYPE_APPLICATION</name>
      <anchor>gga113a114</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_TYPE_SEEKTABLE</name>
      <anchor>gga113a115</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_TYPE_VORBIS_COMMENT</name>
      <anchor>gga113a116</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_TYPE_CUESHEET</name>
      <anchor>gga113a117</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_TYPE_PICTURE</name>
      <anchor>gga113a118</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_TYPE_UNDEFINED</name>
      <anchor>gga113a119</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <name>FLAC__StreamMetadata_Picture_Type</name>
      <anchor>ga114</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_METADATA_PICTURE_TYPE_OTHER</name>
      <anchor>gga114a120</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_METADATA_PICTURE_TYPE_FILE_ICON_STANDARD</name>
      <anchor>gga114a121</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_METADATA_PICTURE_TYPE_FILE_ICON</name>
      <anchor>gga114a122</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_METADATA_PICTURE_TYPE_FRONT_COVER</name>
      <anchor>gga114a123</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_METADATA_PICTURE_TYPE_BACK_COVER</name>
      <anchor>gga114a124</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_METADATA_PICTURE_TYPE_LEAFLET_PAGE</name>
      <anchor>gga114a125</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_METADATA_PICTURE_TYPE_MEDIA</name>
      <anchor>gga114a126</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_METADATA_PICTURE_TYPE_LEAD_ARTIST</name>
      <anchor>gga114a127</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_METADATA_PICTURE_TYPE_ARTIST</name>
      <anchor>gga114a128</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_METADATA_PICTURE_TYPE_CONDUCTOR</name>
      <anchor>gga114a129</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_METADATA_PICTURE_TYPE_BAND</name>
      <anchor>gga114a130</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_METADATA_PICTURE_TYPE_COMPOSER</name>
      <anchor>gga114a131</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_METADATA_PICTURE_TYPE_LYRICIST</name>
      <anchor>gga114a132</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_METADATA_PICTURE_TYPE_RECORDING_LOCATION</name>
      <anchor>gga114a133</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_METADATA_PICTURE_TYPE_DURING_RECORDING</name>
      <anchor>gga114a134</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_METADATA_PICTURE_TYPE_DURING_PERFORMANCE</name>
      <anchor>gga114a135</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_METADATA_PICTURE_TYPE_VIDEO_SCREEN_CAPTURE</name>
      <anchor>gga114a136</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_METADATA_PICTURE_TYPE_FISH</name>
      <anchor>gga114a137</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_METADATA_PICTURE_TYPE_ILLUSTRATION</name>
      <anchor>gga114a138</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_METADATA_PICTURE_TYPE_BAND_LOGOTYPE</name>
      <anchor>gga114a139</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_METADATA_PICTURE_TYPE_PUBLISHER_LOGOTYPE</name>
      <anchor>gga114a140</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__format_sample_rate_is_valid</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga80</anchor>
      <arglist>(unsigned sample_rate)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__format_sample_rate_is_subset</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga81</anchor>
      <arglist>(unsigned sample_rate)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__format_vorbiscomment_entry_name_is_legal</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga82</anchor>
      <arglist>(const char *name)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__format_vorbiscomment_entry_value_is_legal</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga83</anchor>
      <arglist>(const FLAC__byte *value, unsigned length)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__format_vorbiscomment_entry_is_legal</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga84</anchor>
      <arglist>(const FLAC__byte *entry, unsigned length)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__format_seektable_is_legal</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga85</anchor>
      <arglist>(const FLAC__StreamMetadata_SeekTable *seek_table)</arglist>
    </member>
    <member kind="function">
      <type>unsigned</type>
      <name>FLAC__format_seektable_sort</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga86</anchor>
      <arglist>(FLAC__StreamMetadata_SeekTable *seek_table)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__format_cuesheet_is_legal</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga87</anchor>
      <arglist>(const FLAC__StreamMetadata_CueSheet *cue_sheet, FLAC__bool check_cd_da_subset, const char **violation)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__format_picture_is_legal</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga88</anchor>
      <arglist>(const FLAC__StreamMetadata_Picture *picture, const char **violation)</arglist>
    </member>
    <member kind="variable">
      <type>const char *</type>
      <name>FLAC__VERSION_STRING</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga0</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const char *</type>
      <name>FLAC__VENDOR_STRING</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga1</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const FLAC__byte</type>
      <name>FLAC__STREAM_SYNC_STRING</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga2</anchor>
      <arglist>[4]</arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_SYNC</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga3</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_SYNC_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga4</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const char *const</type>
      <name>FLAC__EntropyCodingMethodTypeString</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga5</anchor>
      <arglist>[]</arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_ORDER_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga6</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_PARAMETER_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga7</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE2_PARAMETER_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga8</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_RAW_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga9</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_ESCAPE_PARAMETER</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga10</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE2_ESCAPE_PARAMETER</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga11</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__ENTROPY_CODING_METHOD_TYPE_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga12</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const char *const</type>
      <name>FLAC__SubframeTypeString</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga13</anchor>
      <arglist>[]</arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__SUBFRAME_LPC_QLP_COEFF_PRECISION_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga14</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__SUBFRAME_LPC_QLP_SHIFT_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga15</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__SUBFRAME_ZERO_PAD_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga16</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__SUBFRAME_TYPE_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga17</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__SUBFRAME_WASTED_BITS_FLAG_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga18</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__SUBFRAME_TYPE_CONSTANT_BYTE_ALIGNED_MASK</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga19</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__SUBFRAME_TYPE_VERBATIM_BYTE_ALIGNED_MASK</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga20</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__SUBFRAME_TYPE_FIXED_BYTE_ALIGNED_MASK</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga21</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__SUBFRAME_TYPE_LPC_BYTE_ALIGNED_MASK</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga22</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const char *const</type>
      <name>FLAC__ChannelAssignmentString</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga23</anchor>
      <arglist>[]</arglist>
    </member>
    <member kind="variable">
      <type>const char *const</type>
      <name>FLAC__FrameNumberTypeString</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga24</anchor>
      <arglist>[]</arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__FRAME_HEADER_SYNC</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga25</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__FRAME_HEADER_SYNC_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga26</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__FRAME_HEADER_RESERVED_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga27</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__FRAME_HEADER_BLOCKING_STRATEGY_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga28</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__FRAME_HEADER_BLOCK_SIZE_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga29</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__FRAME_HEADER_SAMPLE_RATE_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga30</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__FRAME_HEADER_CHANNEL_ASSIGNMENT_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga31</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__FRAME_HEADER_BITS_PER_SAMPLE_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga32</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__FRAME_HEADER_ZERO_PAD_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga33</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__FRAME_HEADER_CRC_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga34</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__FRAME_FOOTER_CRC_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga35</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const char *const</type>
      <name>FLAC__MetadataTypeString</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga36</anchor>
      <arglist>[]</arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_STREAMINFO_MIN_BLOCK_SIZE_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga37</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_STREAMINFO_MAX_BLOCK_SIZE_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga38</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_STREAMINFO_MIN_FRAME_SIZE_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga39</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_STREAMINFO_MAX_FRAME_SIZE_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga40</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_STREAMINFO_SAMPLE_RATE_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga41</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_STREAMINFO_CHANNELS_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga42</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_STREAMINFO_BITS_PER_SAMPLE_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga43</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_STREAMINFO_TOTAL_SAMPLES_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga44</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_STREAMINFO_MD5SUM_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga45</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_APPLICATION_ID_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga46</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_SEEKPOINT_SAMPLE_NUMBER_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga47</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_SEEKPOINT_STREAM_OFFSET_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga48</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_SEEKPOINT_FRAME_SAMPLES_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga49</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const FLAC__uint64</type>
      <name>FLAC__STREAM_METADATA_SEEKPOINT_PLACEHOLDER</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga50</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_VORBIS_COMMENT_ENTRY_LENGTH_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga51</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_VORBIS_COMMENT_NUM_COMMENTS_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga52</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_CUESHEET_INDEX_OFFSET_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga53</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_CUESHEET_INDEX_NUMBER_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga54</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_CUESHEET_INDEX_RESERVED_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga55</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_CUESHEET_TRACK_OFFSET_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga56</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_CUESHEET_TRACK_NUMBER_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga57</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_CUESHEET_TRACK_ISRC_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga58</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_CUESHEET_TRACK_TYPE_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga59</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_CUESHEET_TRACK_PRE_EMPHASIS_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga60</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_CUESHEET_TRACK_RESERVED_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga61</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_CUESHEET_TRACK_NUM_INDICES_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga62</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_CUESHEET_MEDIA_CATALOG_NUMBER_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga63</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_CUESHEET_LEAD_IN_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga64</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_CUESHEET_IS_CD_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga65</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_CUESHEET_RESERVED_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga66</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_CUESHEET_NUM_TRACKS_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga67</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const char *const</type>
      <name>FLAC__StreamMetadata_Picture_TypeString</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga68</anchor>
      <arglist>[]</arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_PICTURE_TYPE_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga69</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_PICTURE_MIME_TYPE_LENGTH_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga70</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_PICTURE_DESCRIPTION_LENGTH_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga71</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_PICTURE_WIDTH_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga72</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_PICTURE_HEIGHT_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga73</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_PICTURE_DEPTH_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga74</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_PICTURE_COLORS_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga75</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_PICTURE_DATA_LENGTH_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga76</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_IS_LAST_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga77</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_TYPE_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga78</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const unsigned</type>
      <name>FLAC__STREAM_METADATA_LENGTH_LEN</name>
      <anchorfile>group__flac__format.html</anchorfile>
      <anchor>ga79</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="group">
    <name>flac_metadata</name>
    <title>FLAC/metadata.h: metadata interfaces</title>
    <filename>group__flac__metadata.html</filename>
    <subgroup>flac_metadata_level0</subgroup>
    <subgroup>flac_metadata_level1</subgroup>
    <subgroup>flac_metadata_level2</subgroup>
    <subgroup>flac_metadata_object</subgroup>
  </compound>
  <compound kind="group">
    <name>flac_metadata_level0</name>
    <title>FLAC/metadata.h: metadata level 0 interface</title>
    <filename>group__flac__metadata__level0.html</filename>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_get_streaminfo</name>
      <anchorfile>group__flac__metadata__level0.html</anchorfile>
      <anchor>ga0</anchor>
      <arglist>(const char *filename, FLAC__StreamMetadata *streaminfo)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_get_tags</name>
      <anchorfile>group__flac__metadata__level0.html</anchorfile>
      <anchor>ga1</anchor>
      <arglist>(const char *filename, FLAC__StreamMetadata **tags)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_get_cuesheet</name>
      <anchorfile>group__flac__metadata__level0.html</anchorfile>
      <anchor>ga2</anchor>
      <arglist>(const char *filename, FLAC__StreamMetadata **cuesheet)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_get_picture</name>
      <anchorfile>group__flac__metadata__level0.html</anchorfile>
      <anchor>ga3</anchor>
      <arglist>(const char *filename, FLAC__StreamMetadata **picture, FLAC__StreamMetadata_Picture_Type type, const char *mime_type, const FLAC__byte *description, unsigned max_width, unsigned max_height, unsigned max_depth, unsigned max_colors)</arglist>
    </member>
  </compound>
  <compound kind="group">
    <name>flac_metadata_level1</name>
    <title>FLAC/metadata.h: metadata level 1 interface</title>
    <filename>group__flac__metadata__level1.html</filename>
    <member kind="typedef">
      <type>FLAC__Metadata_SimpleIterator</type>
      <name>FLAC__Metadata_SimpleIterator</name>
      <anchorfile>group__flac__metadata__level1.html</anchorfile>
      <anchor>ga0</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <name>FLAC__Metadata_SimpleIteratorStatus</name>
      <anchor>ga18</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_SIMPLE_ITERATOR_STATUS_OK</name>
      <anchor>gga18a5</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_SIMPLE_ITERATOR_STATUS_ILLEGAL_INPUT</name>
      <anchor>gga18a6</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_SIMPLE_ITERATOR_STATUS_ERROR_OPENING_FILE</name>
      <anchor>gga18a7</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_SIMPLE_ITERATOR_STATUS_NOT_A_FLAC_FILE</name>
      <anchor>gga18a8</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_SIMPLE_ITERATOR_STATUS_NOT_WRITABLE</name>
      <anchor>gga18a9</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_SIMPLE_ITERATOR_STATUS_BAD_METADATA</name>
      <anchor>gga18a10</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_SIMPLE_ITERATOR_STATUS_READ_ERROR</name>
      <anchor>gga18a11</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_SIMPLE_ITERATOR_STATUS_SEEK_ERROR</name>
      <anchor>gga18a12</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_SIMPLE_ITERATOR_STATUS_WRITE_ERROR</name>
      <anchor>gga18a13</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_SIMPLE_ITERATOR_STATUS_RENAME_ERROR</name>
      <anchor>gga18a14</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_SIMPLE_ITERATOR_STATUS_UNLINK_ERROR</name>
      <anchor>gga18a15</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_SIMPLE_ITERATOR_STATUS_MEMORY_ALLOCATION_ERROR</name>
      <anchor>gga18a16</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_SIMPLE_ITERATOR_STATUS_INTERNAL_ERROR</name>
      <anchor>gga18a17</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>FLAC__Metadata_SimpleIterator *</type>
      <name>FLAC__metadata_simple_iterator_new</name>
      <anchorfile>group__flac__metadata__level1.html</anchorfile>
      <anchor>ga2</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>FLAC__metadata_simple_iterator_delete</name>
      <anchorfile>group__flac__metadata__level1.html</anchorfile>
      <anchor>ga3</anchor>
      <arglist>(FLAC__Metadata_SimpleIterator *iterator)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__Metadata_SimpleIteratorStatus</type>
      <name>FLAC__metadata_simple_iterator_status</name>
      <anchorfile>group__flac__metadata__level1.html</anchorfile>
      <anchor>ga4</anchor>
      <arglist>(FLAC__Metadata_SimpleIterator *iterator)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_simple_iterator_init</name>
      <anchorfile>group__flac__metadata__level1.html</anchorfile>
      <anchor>ga5</anchor>
      <arglist>(FLAC__Metadata_SimpleIterator *iterator, const char *filename, FLAC__bool read_only, FLAC__bool preserve_file_stats)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_simple_iterator_is_writable</name>
      <anchorfile>group__flac__metadata__level1.html</anchorfile>
      <anchor>ga6</anchor>
      <arglist>(const FLAC__Metadata_SimpleIterator *iterator)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_simple_iterator_next</name>
      <anchorfile>group__flac__metadata__level1.html</anchorfile>
      <anchor>ga7</anchor>
      <arglist>(FLAC__Metadata_SimpleIterator *iterator)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_simple_iterator_prev</name>
      <anchorfile>group__flac__metadata__level1.html</anchorfile>
      <anchor>ga8</anchor>
      <arglist>(FLAC__Metadata_SimpleIterator *iterator)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_simple_iterator_is_last</name>
      <anchorfile>group__flac__metadata__level1.html</anchorfile>
      <anchor>ga9</anchor>
      <arglist>(const FLAC__Metadata_SimpleIterator *iterator)</arglist>
    </member>
    <member kind="function">
      <type>off_t</type>
      <name>FLAC__metadata_simple_iterator_get_block_offset</name>
      <anchorfile>group__flac__metadata__level1.html</anchorfile>
      <anchor>ga10</anchor>
      <arglist>(const FLAC__Metadata_SimpleIterator *iterator)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__MetadataType</type>
      <name>FLAC__metadata_simple_iterator_get_block_type</name>
      <anchorfile>group__flac__metadata__level1.html</anchorfile>
      <anchor>ga11</anchor>
      <arglist>(const FLAC__Metadata_SimpleIterator *iterator)</arglist>
    </member>
    <member kind="function">
      <type>unsigned</type>
      <name>FLAC__metadata_simple_iterator_get_block_length</name>
      <anchorfile>group__flac__metadata__level1.html</anchorfile>
      <anchor>ga12</anchor>
      <arglist>(const FLAC__Metadata_SimpleIterator *iterator)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_simple_iterator_get_application_id</name>
      <anchorfile>group__flac__metadata__level1.html</anchorfile>
      <anchor>ga13</anchor>
      <arglist>(FLAC__Metadata_SimpleIterator *iterator, FLAC__byte *id)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__StreamMetadata *</type>
      <name>FLAC__metadata_simple_iterator_get_block</name>
      <anchorfile>group__flac__metadata__level1.html</anchorfile>
      <anchor>ga14</anchor>
      <arglist>(FLAC__Metadata_SimpleIterator *iterator)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_simple_iterator_set_block</name>
      <anchorfile>group__flac__metadata__level1.html</anchorfile>
      <anchor>ga15</anchor>
      <arglist>(FLAC__Metadata_SimpleIterator *iterator, FLAC__StreamMetadata *block, FLAC__bool use_padding)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_simple_iterator_insert_block_after</name>
      <anchorfile>group__flac__metadata__level1.html</anchorfile>
      <anchor>ga16</anchor>
      <arglist>(FLAC__Metadata_SimpleIterator *iterator, FLAC__StreamMetadata *block, FLAC__bool use_padding)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_simple_iterator_delete_block</name>
      <anchorfile>group__flac__metadata__level1.html</anchorfile>
      <anchor>ga17</anchor>
      <arglist>(FLAC__Metadata_SimpleIterator *iterator, FLAC__bool use_padding)</arglist>
    </member>
    <member kind="variable">
      <type>const char *const</type>
      <name>FLAC__Metadata_SimpleIteratorStatusString</name>
      <anchorfile>group__flac__metadata__level1.html</anchorfile>
      <anchor>ga1</anchor>
      <arglist>[]</arglist>
    </member>
  </compound>
  <compound kind="group">
    <name>flac_metadata_level2</name>
    <title>FLAC/metadata.h: metadata level 2 interface</title>
    <filename>group__flac__metadata__level2.html</filename>
    <member kind="typedef">
      <type>FLAC__Metadata_Chain</type>
      <name>FLAC__Metadata_Chain</name>
      <anchorfile>group__flac__metadata__level2.html</anchorfile>
      <anchor>ga0</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>FLAC__Metadata_Iterator</type>
      <name>FLAC__Metadata_Iterator</name>
      <anchorfile>group__flac__metadata__level2.html</anchorfile>
      <anchor>ga1</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <name>FLAC__Metadata_ChainStatus</name>
      <anchor>ga27</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_CHAIN_STATUS_OK</name>
      <anchor>gga27a18</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_CHAIN_STATUS_ILLEGAL_INPUT</name>
      <anchor>gga27a19</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_CHAIN_STATUS_ERROR_OPENING_FILE</name>
      <anchor>gga27a20</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_CHAIN_STATUS_NOT_A_FLAC_FILE</name>
      <anchor>gga27a21</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_CHAIN_STATUS_NOT_WRITABLE</name>
      <anchor>gga27a22</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_CHAIN_STATUS_BAD_METADATA</name>
      <anchor>gga27a23</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_CHAIN_STATUS_READ_ERROR</name>
      <anchor>gga27a24</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_CHAIN_STATUS_SEEK_ERROR</name>
      <anchor>gga27a25</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_CHAIN_STATUS_WRITE_ERROR</name>
      <anchor>gga27a26</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_CHAIN_STATUS_RENAME_ERROR</name>
      <anchor>gga27a27</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_CHAIN_STATUS_UNLINK_ERROR</name>
      <anchor>gga27a28</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_CHAIN_STATUS_MEMORY_ALLOCATION_ERROR</name>
      <anchor>gga27a29</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_CHAIN_STATUS_INTERNAL_ERROR</name>
      <anchor>gga27a30</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_CHAIN_STATUS_INVALID_CALLBACKS</name>
      <anchor>gga27a31</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_CHAIN_STATUS_READ_WRITE_MISMATCH</name>
      <anchor>gga27a32</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__METADATA_CHAIN_STATUS_WRONG_WRITE_CALL</name>
      <anchor>gga27a33</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>FLAC__Metadata_Chain *</type>
      <name>FLAC__metadata_chain_new</name>
      <anchorfile>group__flac__metadata__level2.html</anchorfile>
      <anchor>ga3</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>FLAC__metadata_chain_delete</name>
      <anchorfile>group__flac__metadata__level2.html</anchorfile>
      <anchor>ga4</anchor>
      <arglist>(FLAC__Metadata_Chain *chain)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__Metadata_ChainStatus</type>
      <name>FLAC__metadata_chain_status</name>
      <anchorfile>group__flac__metadata__level2.html</anchorfile>
      <anchor>ga5</anchor>
      <arglist>(FLAC__Metadata_Chain *chain)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_chain_read</name>
      <anchorfile>group__flac__metadata__level2.html</anchorfile>
      <anchor>ga6</anchor>
      <arglist>(FLAC__Metadata_Chain *chain, const char *filename)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_chain_read_ogg</name>
      <anchorfile>group__flac__metadata__level2.html</anchorfile>
      <anchor>ga7</anchor>
      <arglist>(FLAC__Metadata_Chain *chain, const char *filename)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_chain_read_with_callbacks</name>
      <anchorfile>group__flac__metadata__level2.html</anchorfile>
      <anchor>ga8</anchor>
      <arglist>(FLAC__Metadata_Chain *chain, FLAC__IOHandle handle, FLAC__IOCallbacks callbacks)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_chain_read_ogg_with_callbacks</name>
      <anchorfile>group__flac__metadata__level2.html</anchorfile>
      <anchor>ga9</anchor>
      <arglist>(FLAC__Metadata_Chain *chain, FLAC__IOHandle handle, FLAC__IOCallbacks callbacks)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_chain_check_if_tempfile_needed</name>
      <anchorfile>group__flac__metadata__level2.html</anchorfile>
      <anchor>ga10</anchor>
      <arglist>(FLAC__Metadata_Chain *chain, FLAC__bool use_padding)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_chain_write</name>
      <anchorfile>group__flac__metadata__level2.html</anchorfile>
      <anchor>ga11</anchor>
      <arglist>(FLAC__Metadata_Chain *chain, FLAC__bool use_padding, FLAC__bool preserve_file_stats)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_chain_write_with_callbacks</name>
      <anchorfile>group__flac__metadata__level2.html</anchorfile>
      <anchor>ga12</anchor>
      <arglist>(FLAC__Metadata_Chain *chain, FLAC__bool use_padding, FLAC__IOHandle handle, FLAC__IOCallbacks callbacks)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_chain_write_with_callbacks_and_tempfile</name>
      <anchorfile>group__flac__metadata__level2.html</anchorfile>
      <anchor>ga13</anchor>
      <arglist>(FLAC__Metadata_Chain *chain, FLAC__bool use_padding, FLAC__IOHandle handle, FLAC__IOCallbacks callbacks, FLAC__IOHandle temp_handle, FLAC__IOCallbacks temp_callbacks)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>FLAC__metadata_chain_merge_padding</name>
      <anchorfile>group__flac__metadata__level2.html</anchorfile>
      <anchor>ga14</anchor>
      <arglist>(FLAC__Metadata_Chain *chain)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>FLAC__metadata_chain_sort_padding</name>
      <anchorfile>group__flac__metadata__level2.html</anchorfile>
      <anchor>ga15</anchor>
      <arglist>(FLAC__Metadata_Chain *chain)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__Metadata_Iterator *</type>
      <name>FLAC__metadata_iterator_new</name>
      <anchorfile>group__flac__metadata__level2.html</anchorfile>
      <anchor>ga16</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>FLAC__metadata_iterator_delete</name>
      <anchorfile>group__flac__metadata__level2.html</anchorfile>
      <anchor>ga17</anchor>
      <arglist>(FLAC__Metadata_Iterator *iterator)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>FLAC__metadata_iterator_init</name>
      <anchorfile>group__flac__metadata__level2.html</anchorfile>
      <anchor>ga18</anchor>
      <arglist>(FLAC__Metadata_Iterator *iterator, FLAC__Metadata_Chain *chain)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_iterator_next</name>
      <anchorfile>group__flac__metadata__level2.html</anchorfile>
      <anchor>ga19</anchor>
      <arglist>(FLAC__Metadata_Iterator *iterator)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_iterator_prev</name>
      <anchorfile>group__flac__metadata__level2.html</anchorfile>
      <anchor>ga20</anchor>
      <arglist>(FLAC__Metadata_Iterator *iterator)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__MetadataType</type>
      <name>FLAC__metadata_iterator_get_block_type</name>
      <anchorfile>group__flac__metadata__level2.html</anchorfile>
      <anchor>ga21</anchor>
      <arglist>(const FLAC__Metadata_Iterator *iterator)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__StreamMetadata *</type>
      <name>FLAC__metadata_iterator_get_block</name>
      <anchorfile>group__flac__metadata__level2.html</anchorfile>
      <anchor>ga22</anchor>
      <arglist>(FLAC__Metadata_Iterator *iterator)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_iterator_set_block</name>
      <anchorfile>group__flac__metadata__level2.html</anchorfile>
      <anchor>ga23</anchor>
      <arglist>(FLAC__Metadata_Iterator *iterator, FLAC__StreamMetadata *block)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_iterator_delete_block</name>
      <anchorfile>group__flac__metadata__level2.html</anchorfile>
      <anchor>ga24</anchor>
      <arglist>(FLAC__Metadata_Iterator *iterator, FLAC__bool replace_with_padding)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_iterator_insert_block_before</name>
      <anchorfile>group__flac__metadata__level2.html</anchorfile>
      <anchor>ga25</anchor>
      <arglist>(FLAC__Metadata_Iterator *iterator, FLAC__StreamMetadata *block)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_iterator_insert_block_after</name>
      <anchorfile>group__flac__metadata__level2.html</anchorfile>
      <anchor>ga26</anchor>
      <arglist>(FLAC__Metadata_Iterator *iterator, FLAC__StreamMetadata *block)</arglist>
    </member>
    <member kind="variable">
      <type>const char *const</type>
      <name>FLAC__Metadata_ChainStatusString</name>
      <anchorfile>group__flac__metadata__level2.html</anchorfile>
      <anchor>ga2</anchor>
      <arglist>[]</arglist>
    </member>
  </compound>
  <compound kind="group">
    <name>flac_metadata_object</name>
    <title>FLAC/metadata.h: metadata object methods</title>
    <filename>group__flac__metadata__object.html</filename>
    <member kind="function">
      <type>FLAC__StreamMetadata *</type>
      <name>FLAC__metadata_object_new</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga0</anchor>
      <arglist>(FLAC__MetadataType type)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__StreamMetadata *</type>
      <name>FLAC__metadata_object_clone</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga1</anchor>
      <arglist>(const FLAC__StreamMetadata *object)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>FLAC__metadata_object_delete</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga2</anchor>
      <arglist>(FLAC__StreamMetadata *object)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_is_equal</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga3</anchor>
      <arglist>(const FLAC__StreamMetadata *block1, const FLAC__StreamMetadata *block2)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_application_set_data</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga4</anchor>
      <arglist>(FLAC__StreamMetadata *object, FLAC__byte *data, unsigned length, FLAC__bool copy)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_seektable_resize_points</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga5</anchor>
      <arglist>(FLAC__StreamMetadata *object, unsigned new_num_points)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>FLAC__metadata_object_seektable_set_point</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga6</anchor>
      <arglist>(FLAC__StreamMetadata *object, unsigned point_num, FLAC__StreamMetadata_SeekPoint point)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_seektable_insert_point</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga7</anchor>
      <arglist>(FLAC__StreamMetadata *object, unsigned point_num, FLAC__StreamMetadata_SeekPoint point)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_seektable_delete_point</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga8</anchor>
      <arglist>(FLAC__StreamMetadata *object, unsigned point_num)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_seektable_is_legal</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga9</anchor>
      <arglist>(const FLAC__StreamMetadata *object)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_seektable_template_append_placeholders</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga10</anchor>
      <arglist>(FLAC__StreamMetadata *object, unsigned num)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_seektable_template_append_point</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga11</anchor>
      <arglist>(FLAC__StreamMetadata *object, FLAC__uint64 sample_number)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_seektable_template_append_points</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga12</anchor>
      <arglist>(FLAC__StreamMetadata *object, FLAC__uint64 sample_numbers[], unsigned num)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_seektable_template_append_spaced_points</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga13</anchor>
      <arglist>(FLAC__StreamMetadata *object, unsigned num, FLAC__uint64 total_samples)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_seektable_template_append_spaced_points_by_samples</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga14</anchor>
      <arglist>(FLAC__StreamMetadata *object, unsigned samples, FLAC__uint64 total_samples)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_seektable_template_sort</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga15</anchor>
      <arglist>(FLAC__StreamMetadata *object, FLAC__bool compact)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_vorbiscomment_set_vendor_string</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga16</anchor>
      <arglist>(FLAC__StreamMetadata *object, FLAC__StreamMetadata_VorbisComment_Entry entry, FLAC__bool copy)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_vorbiscomment_resize_comments</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga17</anchor>
      <arglist>(FLAC__StreamMetadata *object, unsigned new_num_comments)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_vorbiscomment_set_comment</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga18</anchor>
      <arglist>(FLAC__StreamMetadata *object, unsigned comment_num, FLAC__StreamMetadata_VorbisComment_Entry entry, FLAC__bool copy)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_vorbiscomment_insert_comment</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga19</anchor>
      <arglist>(FLAC__StreamMetadata *object, unsigned comment_num, FLAC__StreamMetadata_VorbisComment_Entry entry, FLAC__bool copy)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_vorbiscomment_append_comment</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga20</anchor>
      <arglist>(FLAC__StreamMetadata *object, FLAC__StreamMetadata_VorbisComment_Entry entry, FLAC__bool copy)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_vorbiscomment_replace_comment</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga21</anchor>
      <arglist>(FLAC__StreamMetadata *object, FLAC__StreamMetadata_VorbisComment_Entry entry, FLAC__bool all, FLAC__bool copy)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_vorbiscomment_delete_comment</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga22</anchor>
      <arglist>(FLAC__StreamMetadata *object, unsigned comment_num)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga23</anchor>
      <arglist>(FLAC__StreamMetadata_VorbisComment_Entry *entry, const char *field_name, const char *field_value)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_vorbiscomment_entry_to_name_value_pair</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga24</anchor>
      <arglist>(const FLAC__StreamMetadata_VorbisComment_Entry entry, char **field_name, char **field_value)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_vorbiscomment_entry_matches</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga25</anchor>
      <arglist>(const FLAC__StreamMetadata_VorbisComment_Entry entry, const char *field_name, unsigned field_name_length)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>FLAC__metadata_object_vorbiscomment_find_entry_from</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga26</anchor>
      <arglist>(const FLAC__StreamMetadata *object, unsigned offset, const char *field_name)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>FLAC__metadata_object_vorbiscomment_remove_entry_matching</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga27</anchor>
      <arglist>(FLAC__StreamMetadata *object, const char *field_name)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>FLAC__metadata_object_vorbiscomment_remove_entries_matching</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga28</anchor>
      <arglist>(FLAC__StreamMetadata *object, const char *field_name)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__StreamMetadata_CueSheet_Track *</type>
      <name>FLAC__metadata_object_cuesheet_track_new</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga29</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__StreamMetadata_CueSheet_Track *</type>
      <name>FLAC__metadata_object_cuesheet_track_clone</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga30</anchor>
      <arglist>(const FLAC__StreamMetadata_CueSheet_Track *object)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>FLAC__metadata_object_cuesheet_track_delete</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga31</anchor>
      <arglist>(FLAC__StreamMetadata_CueSheet_Track *object)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_cuesheet_track_resize_indices</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga32</anchor>
      <arglist>(FLAC__StreamMetadata *object, unsigned track_num, unsigned new_num_indices)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_cuesheet_track_insert_index</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga33</anchor>
      <arglist>(FLAC__StreamMetadata *object, unsigned track_num, unsigned index_num, FLAC__StreamMetadata_CueSheet_Index index)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_cuesheet_track_insert_blank_index</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga34</anchor>
      <arglist>(FLAC__StreamMetadata *object, unsigned track_num, unsigned index_num)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_cuesheet_track_delete_index</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga35</anchor>
      <arglist>(FLAC__StreamMetadata *object, unsigned track_num, unsigned index_num)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_cuesheet_resize_tracks</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga36</anchor>
      <arglist>(FLAC__StreamMetadata *object, unsigned new_num_tracks)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_cuesheet_insert_track</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga37</anchor>
      <arglist>(FLAC__StreamMetadata *object, unsigned track_num, FLAC__StreamMetadata_CueSheet_Track *track, FLAC__bool copy)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_cuesheet_insert_blank_track</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga38</anchor>
      <arglist>(FLAC__StreamMetadata *object, unsigned track_num)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_cuesheet_delete_track</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga39</anchor>
      <arglist>(FLAC__StreamMetadata *object, unsigned track_num)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_cuesheet_is_legal</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga40</anchor>
      <arglist>(const FLAC__StreamMetadata *object, FLAC__bool check_cd_da_subset, const char **violation)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__uint32</type>
      <name>FLAC__metadata_object_cuesheet_calculate_cddb_id</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga41</anchor>
      <arglist>(const FLAC__StreamMetadata *object)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_picture_set_mime_type</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga42</anchor>
      <arglist>(FLAC__StreamMetadata *object, char *mime_type, FLAC__bool copy)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_picture_set_description</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga43</anchor>
      <arglist>(FLAC__StreamMetadata *object, FLAC__byte *description, FLAC__bool copy)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_picture_set_data</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga44</anchor>
      <arglist>(FLAC__StreamMetadata *object, FLAC__byte *data, FLAC__uint32 length, FLAC__bool copy)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__metadata_object_picture_is_legal</name>
      <anchorfile>group__flac__metadata__object.html</anchorfile>
      <anchor>ga45</anchor>
      <arglist>(const FLAC__StreamMetadata *object, const char **violation)</arglist>
    </member>
  </compound>
  <compound kind="group">
    <name>flac_decoder</name>
    <title>FLAC/_decoder.h: decoder interfaces</title>
    <filename>group__flac__decoder.html</filename>
    <subgroup>flac_stream_decoder</subgroup>
  </compound>
  <compound kind="group">
    <name>flac_stream_decoder</name>
    <title>FLAC/stream_decoder.h: stream decoder interface</title>
    <filename>group__flac__stream__decoder.html</filename>
    <class kind="struct">FLAC__StreamDecoder</class>
    <member kind="typedef">
      <type>FLAC__StreamDecoderReadStatus(*</type>
      <name>FLAC__StreamDecoderReadCallback</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga8</anchor>
      <arglist>)(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], size_t *bytes, void *client_data)</arglist>
    </member>
    <member kind="typedef">
      <type>FLAC__StreamDecoderSeekStatus(*</type>
      <name>FLAC__StreamDecoderSeekCallback</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga9</anchor>
      <arglist>)(const FLAC__StreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data)</arglist>
    </member>
    <member kind="typedef">
      <type>FLAC__StreamDecoderTellStatus(*</type>
      <name>FLAC__StreamDecoderTellCallback</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga10</anchor>
      <arglist>)(const FLAC__StreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data)</arglist>
    </member>
    <member kind="typedef">
      <type>FLAC__StreamDecoderLengthStatus(*</type>
      <name>FLAC__StreamDecoderLengthCallback</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga11</anchor>
      <arglist>)(const FLAC__StreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data)</arglist>
    </member>
    <member kind="typedef">
      <type>FLAC__bool(*</type>
      <name>FLAC__StreamDecoderEofCallback</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga12</anchor>
      <arglist>)(const FLAC__StreamDecoder *decoder, void *client_data)</arglist>
    </member>
    <member kind="typedef">
      <type>FLAC__StreamDecoderWriteStatus(*</type>
      <name>FLAC__StreamDecoderWriteCallback</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga13</anchor>
      <arglist>)(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 *const buffer[], void *client_data)</arglist>
    </member>
    <member kind="typedef">
      <type>void(*</type>
      <name>FLAC__StreamDecoderMetadataCallback</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga14</anchor>
      <arglist>)(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data)</arglist>
    </member>
    <member kind="typedef">
      <type>void(*</type>
      <name>FLAC__StreamDecoderErrorCallback</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga15</anchor>
      <arglist>)(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)</arglist>
    </member>
    <member kind="enumeration">
      <name>FLAC__StreamDecoderState</name>
      <anchor>ga50</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_SEARCH_FOR_METADATA</name>
      <anchor>gga50a16</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_READ_METADATA</name>
      <anchor>gga50a17</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC</name>
      <anchor>gga50a18</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_READ_FRAME</name>
      <anchor>gga50a19</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_END_OF_STREAM</name>
      <anchor>gga50a20</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_OGG_ERROR</name>
      <anchor>gga50a21</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_SEEK_ERROR</name>
      <anchor>gga50a22</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_ABORTED</name>
      <anchor>gga50a23</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_MEMORY_ALLOCATION_ERROR</name>
      <anchor>gga50a24</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_UNINITIALIZED</name>
      <anchor>gga50a25</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <name>FLAC__StreamDecoderInitStatus</name>
      <anchor>ga51</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_INIT_STATUS_OK</name>
      <anchor>gga51a26</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_INIT_STATUS_UNSUPPORTED_CONTAINER</name>
      <anchor>gga51a27</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_INIT_STATUS_INVALID_CALLBACKS</name>
      <anchor>gga51a28</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_INIT_STATUS_MEMORY_ALLOCATION_ERROR</name>
      <anchor>gga51a29</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_INIT_STATUS_ERROR_OPENING_FILE</name>
      <anchor>gga51a30</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_INIT_STATUS_ALREADY_INITIALIZED</name>
      <anchor>gga51a31</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <name>FLAC__StreamDecoderReadStatus</name>
      <anchor>ga52</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_READ_STATUS_CONTINUE</name>
      <anchor>gga52a32</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM</name>
      <anchor>gga52a33</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_READ_STATUS_ABORT</name>
      <anchor>gga52a34</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <name>FLAC__StreamDecoderSeekStatus</name>
      <anchor>ga53</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_SEEK_STATUS_OK</name>
      <anchor>gga53a35</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_SEEK_STATUS_ERROR</name>
      <anchor>gga53a36</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_SEEK_STATUS_UNSUPPORTED</name>
      <anchor>gga53a37</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <name>FLAC__StreamDecoderTellStatus</name>
      <anchor>ga54</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_TELL_STATUS_OK</name>
      <anchor>gga54a38</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_TELL_STATUS_ERROR</name>
      <anchor>gga54a39</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_TELL_STATUS_UNSUPPORTED</name>
      <anchor>gga54a40</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <name>FLAC__StreamDecoderLengthStatus</name>
      <anchor>ga55</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_LENGTH_STATUS_OK</name>
      <anchor>gga55a41</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_LENGTH_STATUS_ERROR</name>
      <anchor>gga55a42</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_LENGTH_STATUS_UNSUPPORTED</name>
      <anchor>gga55a43</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <name>FLAC__StreamDecoderWriteStatus</name>
      <anchor>ga56</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE</name>
      <anchor>gga56a44</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_WRITE_STATUS_ABORT</name>
      <anchor>gga56a45</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <name>FLAC__StreamDecoderErrorStatus</name>
      <anchor>ga57</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_ERROR_STATUS_LOST_SYNC</name>
      <anchor>gga57a46</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_ERROR_STATUS_BAD_HEADER</name>
      <anchor>gga57a47</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_ERROR_STATUS_FRAME_CRC_MISMATCH</name>
      <anchor>gga57a48</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_DECODER_ERROR_STATUS_UNPARSEABLE_STREAM</name>
      <anchor>gga57a49</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>FLAC__StreamDecoder *</type>
      <name>FLAC__stream_decoder_new</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga16</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>FLAC__stream_decoder_delete</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga17</anchor>
      <arglist>(FLAC__StreamDecoder *decoder)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_decoder_set_ogg_serial_number</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga18</anchor>
      <arglist>(FLAC__StreamDecoder *decoder, long serial_number)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_decoder_set_md5_checking</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga19</anchor>
      <arglist>(FLAC__StreamDecoder *decoder, FLAC__bool value)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_decoder_set_metadata_respond</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga20</anchor>
      <arglist>(FLAC__StreamDecoder *decoder, FLAC__MetadataType type)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_decoder_set_metadata_respond_application</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga21</anchor>
      <arglist>(FLAC__StreamDecoder *decoder, const FLAC__byte id[4])</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_decoder_set_metadata_respond_all</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga22</anchor>
      <arglist>(FLAC__StreamDecoder *decoder)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_decoder_set_metadata_ignore</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga23</anchor>
      <arglist>(FLAC__StreamDecoder *decoder, FLAC__MetadataType type)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_decoder_set_metadata_ignore_application</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga24</anchor>
      <arglist>(FLAC__StreamDecoder *decoder, const FLAC__byte id[4])</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_decoder_set_metadata_ignore_all</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga25</anchor>
      <arglist>(FLAC__StreamDecoder *decoder)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__StreamDecoderState</type>
      <name>FLAC__stream_decoder_get_state</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga26</anchor>
      <arglist>(const FLAC__StreamDecoder *decoder)</arglist>
    </member>
    <member kind="function">
      <type>const char *</type>
      <name>FLAC__stream_decoder_get_resolved_state_string</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga27</anchor>
      <arglist>(const FLAC__StreamDecoder *decoder)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_decoder_get_md5_checking</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga28</anchor>
      <arglist>(const FLAC__StreamDecoder *decoder)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__uint64</type>
      <name>FLAC__stream_decoder_get_total_samples</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga29</anchor>
      <arglist>(const FLAC__StreamDecoder *decoder)</arglist>
    </member>
    <member kind="function">
      <type>unsigned</type>
      <name>FLAC__stream_decoder_get_channels</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga30</anchor>
      <arglist>(const FLAC__StreamDecoder *decoder)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__ChannelAssignment</type>
      <name>FLAC__stream_decoder_get_channel_assignment</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga31</anchor>
      <arglist>(const FLAC__StreamDecoder *decoder)</arglist>
    </member>
    <member kind="function">
      <type>unsigned</type>
      <name>FLAC__stream_decoder_get_bits_per_sample</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga32</anchor>
      <arglist>(const FLAC__StreamDecoder *decoder)</arglist>
    </member>
    <member kind="function">
      <type>unsigned</type>
      <name>FLAC__stream_decoder_get_sample_rate</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga33</anchor>
      <arglist>(const FLAC__StreamDecoder *decoder)</arglist>
    </member>
    <member kind="function">
      <type>unsigned</type>
      <name>FLAC__stream_decoder_get_blocksize</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga34</anchor>
      <arglist>(const FLAC__StreamDecoder *decoder)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_decoder_get_decode_position</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga35</anchor>
      <arglist>(const FLAC__StreamDecoder *decoder, FLAC__uint64 *position)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__StreamDecoderInitStatus</type>
      <name>FLAC__stream_decoder_init_stream</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga36</anchor>
      <arglist>(FLAC__StreamDecoder *decoder, FLAC__StreamDecoderReadCallback read_callback, FLAC__StreamDecoderSeekCallback seek_callback, FLAC__StreamDecoderTellCallback tell_callback, FLAC__StreamDecoderLengthCallback length_callback, FLAC__StreamDecoderEofCallback eof_callback, FLAC__StreamDecoderWriteCallback write_callback, FLAC__StreamDecoderMetadataCallback metadata_callback, FLAC__StreamDecoderErrorCallback error_callback, void *client_data)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__StreamDecoderInitStatus</type>
      <name>FLAC__stream_decoder_init_ogg_stream</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga37</anchor>
      <arglist>(FLAC__StreamDecoder *decoder, FLAC__StreamDecoderReadCallback read_callback, FLAC__StreamDecoderSeekCallback seek_callback, FLAC__StreamDecoderTellCallback tell_callback, FLAC__StreamDecoderLengthCallback length_callback, FLAC__StreamDecoderEofCallback eof_callback, FLAC__StreamDecoderWriteCallback write_callback, FLAC__StreamDecoderMetadataCallback metadata_callback, FLAC__StreamDecoderErrorCallback error_callback, void *client_data)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__StreamDecoderInitStatus</type>
      <name>FLAC__stream_decoder_init_FILE</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga38</anchor>
      <arglist>(FLAC__StreamDecoder *decoder, FILE *file, FLAC__StreamDecoderWriteCallback write_callback, FLAC__StreamDecoderMetadataCallback metadata_callback, FLAC__StreamDecoderErrorCallback error_callback, void *client_data)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__StreamDecoderInitStatus</type>
      <name>FLAC__stream_decoder_init_ogg_FILE</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga39</anchor>
      <arglist>(FLAC__StreamDecoder *decoder, FILE *file, FLAC__StreamDecoderWriteCallback write_callback, FLAC__StreamDecoderMetadataCallback metadata_callback, FLAC__StreamDecoderErrorCallback error_callback, void *client_data)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__StreamDecoderInitStatus</type>
      <name>FLAC__stream_decoder_init_file</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga40</anchor>
      <arglist>(FLAC__StreamDecoder *decoder, const char *filename, FLAC__StreamDecoderWriteCallback write_callback, FLAC__StreamDecoderMetadataCallback metadata_callback, FLAC__StreamDecoderErrorCallback error_callback, void *client_data)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__StreamDecoderInitStatus</type>
      <name>FLAC__stream_decoder_init_ogg_file</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga41</anchor>
      <arglist>(FLAC__StreamDecoder *decoder, const char *filename, FLAC__StreamDecoderWriteCallback write_callback, FLAC__StreamDecoderMetadataCallback metadata_callback, FLAC__StreamDecoderErrorCallback error_callback, void *client_data)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_decoder_finish</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga42</anchor>
      <arglist>(FLAC__StreamDecoder *decoder)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_decoder_flush</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga43</anchor>
      <arglist>(FLAC__StreamDecoder *decoder)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_decoder_reset</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga44</anchor>
      <arglist>(FLAC__StreamDecoder *decoder)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_decoder_process_single</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga45</anchor>
      <arglist>(FLAC__StreamDecoder *decoder)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_decoder_process_until_end_of_metadata</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga46</anchor>
      <arglist>(FLAC__StreamDecoder *decoder)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_decoder_process_until_end_of_stream</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga47</anchor>
      <arglist>(FLAC__StreamDecoder *decoder)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_decoder_skip_single_frame</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga48</anchor>
      <arglist>(FLAC__StreamDecoder *decoder)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_decoder_seek_absolute</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga49</anchor>
      <arglist>(FLAC__StreamDecoder *decoder, FLAC__uint64 sample)</arglist>
    </member>
    <member kind="variable">
      <type>const char *const</type>
      <name>FLAC__StreamDecoderStateString</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga0</anchor>
      <arglist>[]</arglist>
    </member>
    <member kind="variable">
      <type>const char *const</type>
      <name>FLAC__StreamDecoderInitStatusString</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga1</anchor>
      <arglist>[]</arglist>
    </member>
    <member kind="variable">
      <type>const char *const</type>
      <name>FLAC__StreamDecoderReadStatusString</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga2</anchor>
      <arglist>[]</arglist>
    </member>
    <member kind="variable">
      <type>const char *const</type>
      <name>FLAC__StreamDecoderSeekStatusString</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga3</anchor>
      <arglist>[]</arglist>
    </member>
    <member kind="variable">
      <type>const char *const</type>
      <name>FLAC__StreamDecoderTellStatusString</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga4</anchor>
      <arglist>[]</arglist>
    </member>
    <member kind="variable">
      <type>const char *const</type>
      <name>FLAC__StreamDecoderLengthStatusString</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga5</anchor>
      <arglist>[]</arglist>
    </member>
    <member kind="variable">
      <type>const char *const</type>
      <name>FLAC__StreamDecoderWriteStatusString</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga6</anchor>
      <arglist>[]</arglist>
    </member>
    <member kind="variable">
      <type>const char *const</type>
      <name>FLAC__StreamDecoderErrorStatusString</name>
      <anchorfile>group__flac__stream__decoder.html</anchorfile>
      <anchor>ga7</anchor>
      <arglist>[]</arglist>
    </member>
  </compound>
  <compound kind="group">
    <name>flac_encoder</name>
    <title>FLAC/_encoder.h: encoder interfaces</title>
    <filename>group__flac__encoder.html</filename>
    <subgroup>flac_stream_encoder</subgroup>
  </compound>
  <compound kind="group">
    <name>flac_stream_encoder</name>
    <title>FLAC/stream_encoder.h: stream encoder interface</title>
    <filename>group__flac__stream__encoder.html</filename>
    <class kind="struct">FLAC__StreamEncoder</class>
    <member kind="typedef">
      <type>FLAC__StreamEncoderReadStatus(*</type>
      <name>FLAC__StreamEncoderReadCallback</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga6</anchor>
      <arglist>)(const FLAC__StreamEncoder *encoder, FLAC__byte buffer[], size_t *bytes, void *client_data)</arglist>
    </member>
    <member kind="typedef">
      <type>FLAC__StreamEncoderWriteStatus(*</type>
      <name>FLAC__StreamEncoderWriteCallback</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga7</anchor>
      <arglist>)(const FLAC__StreamEncoder *encoder, const FLAC__byte buffer[], size_t bytes, unsigned samples, unsigned current_frame, void *client_data)</arglist>
    </member>
    <member kind="typedef">
      <type>FLAC__StreamEncoderSeekStatus(*</type>
      <name>FLAC__StreamEncoderSeekCallback</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga8</anchor>
      <arglist>)(const FLAC__StreamEncoder *encoder, FLAC__uint64 absolute_byte_offset, void *client_data)</arglist>
    </member>
    <member kind="typedef">
      <type>FLAC__StreamEncoderTellStatus(*</type>
      <name>FLAC__StreamEncoderTellCallback</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga9</anchor>
      <arglist>)(const FLAC__StreamEncoder *encoder, FLAC__uint64 *absolute_byte_offset, void *client_data)</arglist>
    </member>
    <member kind="typedef">
      <type>void(*</type>
      <name>FLAC__StreamEncoderMetadataCallback</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga10</anchor>
      <arglist>)(const FLAC__StreamEncoder *encoder, const FLAC__StreamMetadata *metadata, void *client_data)</arglist>
    </member>
    <member kind="typedef">
      <type>void(*</type>
      <name>FLAC__StreamEncoderProgressCallback</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga11</anchor>
      <arglist>)(const FLAC__StreamEncoder *encoder, FLAC__uint64 bytes_written, FLAC__uint64 samples_written, unsigned frames_written, unsigned total_frames_estimate, void *client_data)</arglist>
    </member>
    <member kind="enumeration">
      <name>FLAC__StreamEncoderState</name>
      <anchor>ga65</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_OK</name>
      <anchor>gga65a12</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_UNINITIALIZED</name>
      <anchor>gga65a13</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_OGG_ERROR</name>
      <anchor>gga65a14</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_VERIFY_DECODER_ERROR</name>
      <anchor>gga65a15</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_VERIFY_MISMATCH_IN_AUDIO_DATA</name>
      <anchor>gga65a16</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_CLIENT_ERROR</name>
      <anchor>gga65a17</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_IO_ERROR</name>
      <anchor>gga65a18</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_FRAMING_ERROR</name>
      <anchor>gga65a19</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_MEMORY_ALLOCATION_ERROR</name>
      <anchor>gga65a20</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <name>FLAC__StreamEncoderInitStatus</name>
      <anchor>ga66</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_INIT_STATUS_OK</name>
      <anchor>gga66a21</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_INIT_STATUS_ENCODER_ERROR</name>
      <anchor>gga66a22</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_INIT_STATUS_UNSUPPORTED_CONTAINER</name>
      <anchor>gga66a23</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_INIT_STATUS_INVALID_CALLBACKS</name>
      <anchor>gga66a24</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_INIT_STATUS_INVALID_NUMBER_OF_CHANNELS</name>
      <anchor>gga66a25</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_INIT_STATUS_INVALID_BITS_PER_SAMPLE</name>
      <anchor>gga66a26</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_INIT_STATUS_INVALID_SAMPLE_RATE</name>
      <anchor>gga66a27</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_INIT_STATUS_INVALID_BLOCK_SIZE</name>
      <anchor>gga66a28</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_INIT_STATUS_INVALID_MAX_LPC_ORDER</name>
      <anchor>gga66a29</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_INIT_STATUS_INVALID_QLP_COEFF_PRECISION</name>
      <anchor>gga66a30</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_INIT_STATUS_BLOCK_SIZE_TOO_SMALL_FOR_LPC_ORDER</name>
      <anchor>gga66a31</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_INIT_STATUS_NOT_STREAMABLE</name>
      <anchor>gga66a32</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_INIT_STATUS_INVALID_METADATA</name>
      <anchor>gga66a33</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_INIT_STATUS_ALREADY_INITIALIZED</name>
      <anchor>gga66a34</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <name>FLAC__StreamEncoderReadStatus</name>
      <anchor>ga67</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_READ_STATUS_CONTINUE</name>
      <anchor>gga67a35</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_READ_STATUS_END_OF_STREAM</name>
      <anchor>gga67a36</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_READ_STATUS_ABORT</name>
      <anchor>gga67a37</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_READ_STATUS_UNSUPPORTED</name>
      <anchor>gga67a38</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <name>FLAC__StreamEncoderWriteStatus</name>
      <anchor>ga68</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_WRITE_STATUS_OK</name>
      <anchor>gga68a39</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR</name>
      <anchor>gga68a40</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <name>FLAC__StreamEncoderSeekStatus</name>
      <anchor>ga69</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_SEEK_STATUS_OK</name>
      <anchor>gga69a41</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_SEEK_STATUS_ERROR</name>
      <anchor>gga69a42</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_SEEK_STATUS_UNSUPPORTED</name>
      <anchor>gga69a43</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <name>FLAC__StreamEncoderTellStatus</name>
      <anchor>ga70</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_TELL_STATUS_OK</name>
      <anchor>gga70a44</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_TELL_STATUS_ERROR</name>
      <anchor>gga70a45</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FLAC__STREAM_ENCODER_TELL_STATUS_UNSUPPORTED</name>
      <anchor>gga70a46</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>FLAC__StreamEncoder *</type>
      <name>FLAC__stream_encoder_new</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga12</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>FLAC__stream_encoder_delete</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga13</anchor>
      <arglist>(FLAC__StreamEncoder *encoder)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_encoder_set_ogg_serial_number</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga14</anchor>
      <arglist>(FLAC__StreamEncoder *encoder, long serial_number)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_encoder_set_verify</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga15</anchor>
      <arglist>(FLAC__StreamEncoder *encoder, FLAC__bool value)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_encoder_set_streamable_subset</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga16</anchor>
      <arglist>(FLAC__StreamEncoder *encoder, FLAC__bool value)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_encoder_set_channels</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga17</anchor>
      <arglist>(FLAC__StreamEncoder *encoder, unsigned value)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_encoder_set_bits_per_sample</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga18</anchor>
      <arglist>(FLAC__StreamEncoder *encoder, unsigned value)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_encoder_set_sample_rate</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga19</anchor>
      <arglist>(FLAC__StreamEncoder *encoder, unsigned value)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_encoder_set_compression_level</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga20</anchor>
      <arglist>(FLAC__StreamEncoder *encoder, unsigned value)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_encoder_set_blocksize</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga21</anchor>
      <arglist>(FLAC__StreamEncoder *encoder, unsigned value)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_encoder_set_do_mid_side_stereo</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga22</anchor>
      <arglist>(FLAC__StreamEncoder *encoder, FLAC__bool value)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_encoder_set_loose_mid_side_stereo</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga23</anchor>
      <arglist>(FLAC__StreamEncoder *encoder, FLAC__bool value)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_encoder_set_apodization</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga24</anchor>
      <arglist>(FLAC__StreamEncoder *encoder, const char *specification)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_encoder_set_max_lpc_order</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga25</anchor>
      <arglist>(FLAC__StreamEncoder *encoder, unsigned value)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_encoder_set_qlp_coeff_precision</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga26</anchor>
      <arglist>(FLAC__StreamEncoder *encoder, unsigned value)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_encoder_set_do_qlp_coeff_prec_search</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga27</anchor>
      <arglist>(FLAC__StreamEncoder *encoder, FLAC__bool value)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_encoder_set_do_escape_coding</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga28</anchor>
      <arglist>(FLAC__StreamEncoder *encoder, FLAC__bool value)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_encoder_set_do_exhaustive_model_search</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga29</anchor>
      <arglist>(FLAC__StreamEncoder *encoder, FLAC__bool value)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_encoder_set_min_residual_partition_order</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga30</anchor>
      <arglist>(FLAC__StreamEncoder *encoder, unsigned value)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_encoder_set_max_residual_partition_order</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga31</anchor>
      <arglist>(FLAC__StreamEncoder *encoder, unsigned value)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_encoder_set_rice_parameter_search_dist</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga32</anchor>
      <arglist>(FLAC__StreamEncoder *encoder, unsigned value)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_encoder_set_total_samples_estimate</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga33</anchor>
      <arglist>(FLAC__StreamEncoder *encoder, FLAC__uint64 value)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_encoder_set_metadata</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga34</anchor>
      <arglist>(FLAC__StreamEncoder *encoder, FLAC__StreamMetadata **metadata, unsigned num_blocks)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__StreamEncoderState</type>
      <name>FLAC__stream_encoder_get_state</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga35</anchor>
      <arglist>(const FLAC__StreamEncoder *encoder)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__StreamDecoderState</type>
      <name>FLAC__stream_encoder_get_verify_decoder_state</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga36</anchor>
      <arglist>(const FLAC__StreamEncoder *encoder)</arglist>
    </member>
    <member kind="function">
      <type>const char *</type>
      <name>FLAC__stream_encoder_get_resolved_state_string</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga37</anchor>
      <arglist>(const FLAC__StreamEncoder *encoder)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>FLAC__stream_encoder_get_verify_decoder_error_stats</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga38</anchor>
      <arglist>(const FLAC__StreamEncoder *encoder, FLAC__uint64 *absolute_sample, unsigned *frame_number, unsigned *channel, unsigned *sample, FLAC__int32 *expected, FLAC__int32 *got)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_encoder_get_verify</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga39</anchor>
      <arglist>(const FLAC__StreamEncoder *encoder)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_encoder_get_streamable_subset</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga40</anchor>
      <arglist>(const FLAC__StreamEncoder *encoder)</arglist>
    </member>
    <member kind="function">
      <type>unsigned</type>
      <name>FLAC__stream_encoder_get_channels</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga41</anchor>
      <arglist>(const FLAC__StreamEncoder *encoder)</arglist>
    </member>
    <member kind="function">
      <type>unsigned</type>
      <name>FLAC__stream_encoder_get_bits_per_sample</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga42</anchor>
      <arglist>(const FLAC__StreamEncoder *encoder)</arglist>
    </member>
    <member kind="function">
      <type>unsigned</type>
      <name>FLAC__stream_encoder_get_sample_rate</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga43</anchor>
      <arglist>(const FLAC__StreamEncoder *encoder)</arglist>
    </member>
    <member kind="function">
      <type>unsigned</type>
      <name>FLAC__stream_encoder_get_blocksize</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga44</anchor>
      <arglist>(const FLAC__StreamEncoder *encoder)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_encoder_get_do_mid_side_stereo</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga45</anchor>
      <arglist>(const FLAC__StreamEncoder *encoder)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_encoder_get_loose_mid_side_stereo</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga46</anchor>
      <arglist>(const FLAC__StreamEncoder *encoder)</arglist>
    </member>
    <member kind="function">
      <type>unsigned</type>
      <name>FLAC__stream_encoder_get_max_lpc_order</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga47</anchor>
      <arglist>(const FLAC__StreamEncoder *encoder)</arglist>
    </member>
    <member kind="function">
      <type>unsigned</type>
      <name>FLAC__stream_encoder_get_qlp_coeff_precision</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga48</anchor>
      <arglist>(const FLAC__StreamEncoder *encoder)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_encoder_get_do_qlp_coeff_prec_search</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga49</anchor>
      <arglist>(const FLAC__StreamEncoder *encoder)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_encoder_get_do_escape_coding</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga50</anchor>
      <arglist>(const FLAC__StreamEncoder *encoder)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_encoder_get_do_exhaustive_model_search</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga51</anchor>
      <arglist>(const FLAC__StreamEncoder *encoder)</arglist>
    </member>
    <member kind="function">
      <type>unsigned</type>
      <name>FLAC__stream_encoder_get_min_residual_partition_order</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga52</anchor>
      <arglist>(const FLAC__StreamEncoder *encoder)</arglist>
    </member>
    <member kind="function">
      <type>unsigned</type>
      <name>FLAC__stream_encoder_get_max_residual_partition_order</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga53</anchor>
      <arglist>(const FLAC__StreamEncoder *encoder)</arglist>
    </member>
    <member kind="function">
      <type>unsigned</type>
      <name>FLAC__stream_encoder_get_rice_parameter_search_dist</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga54</anchor>
      <arglist>(const FLAC__StreamEncoder *encoder)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__uint64</type>
      <name>FLAC__stream_encoder_get_total_samples_estimate</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga55</anchor>
      <arglist>(const FLAC__StreamEncoder *encoder)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__StreamEncoderInitStatus</type>
      <name>FLAC__stream_encoder_init_stream</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga56</anchor>
      <arglist>(FLAC__StreamEncoder *encoder, FLAC__StreamEncoderWriteCallback write_callback, FLAC__StreamEncoderSeekCallback seek_callback, FLAC__StreamEncoderTellCallback tell_callback, FLAC__StreamEncoderMetadataCallback metadata_callback, void *client_data)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__StreamEncoderInitStatus</type>
      <name>FLAC__stream_encoder_init_ogg_stream</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga57</anchor>
      <arglist>(FLAC__StreamEncoder *encoder, FLAC__StreamEncoderReadCallback read_callback, FLAC__StreamEncoderWriteCallback write_callback, FLAC__StreamEncoderSeekCallback seek_callback, FLAC__StreamEncoderTellCallback tell_callback, FLAC__StreamEncoderMetadataCallback metadata_callback, void *client_data)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__StreamEncoderInitStatus</type>
      <name>FLAC__stream_encoder_init_FILE</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga58</anchor>
      <arglist>(FLAC__StreamEncoder *encoder, FILE *file, FLAC__StreamEncoderProgressCallback progress_callback, void *client_data)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__StreamEncoderInitStatus</type>
      <name>FLAC__stream_encoder_init_ogg_FILE</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga59</anchor>
      <arglist>(FLAC__StreamEncoder *encoder, FILE *file, FLAC__StreamEncoderProgressCallback progress_callback, void *client_data)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__StreamEncoderInitStatus</type>
      <name>FLAC__stream_encoder_init_file</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga60</anchor>
      <arglist>(FLAC__StreamEncoder *encoder, const char *filename, FLAC__StreamEncoderProgressCallback progress_callback, void *client_data)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__StreamEncoderInitStatus</type>
      <name>FLAC__stream_encoder_init_ogg_file</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga61</anchor>
      <arglist>(FLAC__StreamEncoder *encoder, const char *filename, FLAC__StreamEncoderProgressCallback progress_callback, void *client_data)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_encoder_finish</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga62</anchor>
      <arglist>(FLAC__StreamEncoder *encoder)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_encoder_process</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga63</anchor>
      <arglist>(FLAC__StreamEncoder *encoder, const FLAC__int32 *const buffer[], unsigned samples)</arglist>
    </member>
    <member kind="function">
      <type>FLAC__bool</type>
      <name>FLAC__stream_encoder_process_interleaved</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga64</anchor>
      <arglist>(FLAC__StreamEncoder *encoder, const FLAC__int32 buffer[], unsigned samples)</arglist>
    </member>
    <member kind="variable">
      <type>const char *const</type>
      <name>FLAC__StreamEncoderStateString</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga0</anchor>
      <arglist>[]</arglist>
    </member>
    <member kind="variable">
      <type>const char *const</type>
      <name>FLAC__StreamEncoderInitStatusString</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga1</anchor>
      <arglist>[]</arglist>
    </member>
    <member kind="variable">
      <type>const char *const</type>
      <name>FLAC__StreamEncoderReadStatusString</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga2</anchor>
      <arglist>[]</arglist>
    </member>
    <member kind="variable">
      <type>const char *const</type>
      <name>FLAC__StreamEncoderWriteStatusString</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga3</anchor>
      <arglist>[]</arglist>
    </member>
    <member kind="variable">
      <type>const char *const</type>
      <name>FLAC__StreamEncoderSeekStatusString</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga4</anchor>
      <arglist>[]</arglist>
    </member>
    <member kind="variable">
      <type>const char *const</type>
      <name>FLAC__StreamEncoderTellStatusString</name>
      <anchorfile>group__flac__stream__encoder.html</anchorfile>
      <anchor>ga5</anchor>
      <arglist>[]</arglist>
    </member>
  </compound>
  <compound kind="group">
    <name>flacpp</name>
    <title>FLAC C++ API</title>
    <filename>group__flacpp.html</filename>
    <subgroup>flacpp_decoder</subgroup>
    <subgroup>flacpp_encoder</subgroup>
    <subgroup>flacpp_export</subgroup>
    <subgroup>flacpp_metadata</subgroup>
  </compound>
  <compound kind="group">
    <name>flacpp_decoder</name>
    <title>FLAC++/decoder.h: decoder classes</title>
    <filename>group__flacpp__decoder.html</filename>
    <class kind="class">FLAC::Decoder::Stream</class>
    <class kind="class">FLAC::Decoder::File</class>
  </compound>
  <compound kind="group">
    <name>flacpp_encoder</name>
    <title>FLAC++/encoder.h: encoder classes</title>
    <filename>group__flacpp__encoder.html</filename>
    <class kind="class">FLAC::Encoder::Stream</class>
    <class kind="class">FLAC::Encoder::File</class>
  </compound>
  <compound kind="group">
    <name>flacpp_export</name>
    <title>FLAC++/export.h: export symbols</title>
    <filename>group__flacpp__export.html</filename>
    <member kind="define">
      <type>#define</type>
      <name>FLACPP_API</name>
      <anchorfile>group__flacpp__export.html</anchorfile>
      <anchor>ga0</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FLACPP_API_VERSION_CURRENT</name>
      <anchorfile>group__flacpp__export.html</anchorfile>
      <anchor>ga1</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FLACPP_API_VERSION_REVISION</name>
      <anchorfile>group__flacpp__export.html</anchorfile>
      <anchor>ga2</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FLACPP_API_VERSION_AGE</name>
      <anchorfile>group__flacpp__export.html</anchorfile>
      <anchor>ga3</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="group">
    <name>flacpp_metadata</name>
    <title>FLAC++/metadata.h: metadata interfaces</title>
    <filename>group__flacpp__metadata.html</filename>
    <subgroup>flacpp_metadata_object</subgroup>
    <subgroup>flacpp_metadata_level0</subgroup>
    <subgroup>flacpp_metadata_level1</subgroup>
    <subgroup>flacpp_metadata_level2</subgroup>
  </compound>
  <compound kind="group">
    <name>flacpp_metadata_object</name>
    <title>FLAC++/metadata.h: metadata object classes</title>
    <filename>group__flacpp__metadata__object.html</filename>
    <class kind="class">FLAC::Metadata::Prototype</class>
    <class kind="class">FLAC::Metadata::StreamInfo</class>
    <class kind="class">FLAC::Metadata::Padding</class>
    <class kind="class">FLAC::Metadata::Application</class>
    <class kind="class">FLAC::Metadata::SeekTable</class>
    <class kind="class">FLAC::Metadata::VorbisComment</class>
    <class kind="class">FLAC::Metadata::CueSheet</class>
    <class kind="class">FLAC::Metadata::Picture</class>
    <class kind="class">FLAC::Metadata::Unknown</class>
    <member kind="function">
      <type>Prototype *</type>
      <name>clone</name>
      <anchorfile>group__flacpp__metadata__object.html</anchorfile>
      <anchor>ga0</anchor>
      <arglist>(const Prototype *)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator==</name>
      <anchorfile>group__flacpp__metadata__object.html</anchorfile>
      <anchor>ga1</anchor>
      <arglist>(const Prototype &amp;) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator==</name>
      <anchorfile>group__flacpp__metadata__object.html</anchorfile>
      <anchor>ga2</anchor>
      <arglist>(const ::FLAC__StreamMetadata &amp;) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator==</name>
      <anchorfile>group__flacpp__metadata__object.html</anchorfile>
      <anchor>ga3</anchor>
      <arglist>(const ::FLAC__StreamMetadata *) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator!=</name>
      <anchorfile>group__flacpp__metadata__object.html</anchorfile>
      <anchor>ga4</anchor>
      <arglist>(const Prototype &amp;) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator!=</name>
      <anchorfile>group__flacpp__metadata__object.html</anchorfile>
      <anchor>ga5</anchor>
      <arglist>(const ::FLAC__StreamMetadata &amp;) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator!=</name>
      <anchorfile>group__flacpp__metadata__object.html</anchorfile>
      <anchor>ga6</anchor>
      <arglist>(const ::FLAC__StreamMetadata *) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>is_valid</name>
      <anchorfile>group__flacpp__metadata__object.html</anchorfile>
      <anchor>ga7</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>operator const ::FLAC__StreamMetadata *</name>
      <anchorfile>group__flacpp__metadata__object.html</anchorfile>
      <anchor>ga8</anchor>
      <arglist>() const </arglist>
    </member>
  </compound>
  <compound kind="group">
    <name>flacpp_metadata_level0</name>
    <title>FLAC++/metadata.h: metadata level 0 interface</title>
    <filename>group__flacpp__metadata__level0.html</filename>
    <member kind="function">
      <type>bool</type>
      <name>get_streaminfo</name>
      <anchorfile>group__flacpp__metadata__level0.html</anchorfile>
      <anchor>ga0</anchor>
      <arglist>(const char *filename, StreamInfo &amp;streaminfo)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>get_tags</name>
      <anchorfile>group__flacpp__metadata__level0.html</anchorfile>
      <anchor>ga1</anchor>
      <arglist>(const char *filename, VorbisComment *&amp;tags)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>get_tags</name>
      <anchorfile>group__flacpp__metadata__level0.html</anchorfile>
      <anchor>ga2</anchor>
      <arglist>(const char *filename, VorbisComment &amp;tags)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>get_cuesheet</name>
      <anchorfile>group__flacpp__metadata__level0.html</anchorfile>
      <anchor>ga3</anchor>
      <arglist>(const char *filename, CueSheet *&amp;cuesheet)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>get_cuesheet</name>
      <anchorfile>group__flacpp__metadata__level0.html</anchorfile>
      <anchor>ga4</anchor>
      <arglist>(const char *filename, CueSheet &amp;cuesheet)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>get_picture</name>
      <anchorfile>group__flacpp__metadata__level0.html</anchorfile>
      <anchor>ga5</anchor>
      <arglist>(const char *filename, Picture *&amp;picture,::FLAC__StreamMetadata_Picture_Type type, const char *mime_type, const FLAC__byte *description, unsigned max_width, unsigned max_height, unsigned max_depth, unsigned max_colors)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>get_picture</name>
      <anchorfile>group__flacpp__metadata__level0.html</anchorfile>
      <anchor>ga6</anchor>
      <arglist>(const char *filename, Picture &amp;picture,::FLAC__StreamMetadata_Picture_Type type, const char *mime_type, const FLAC__byte *description, unsigned max_width, unsigned max_height, unsigned max_depth, unsigned max_colors)</arglist>
    </member>
  </compound>
  <compound kind="group">
    <name>flacpp_metadata_level1</name>
    <title>FLAC++/metadata.h: metadata level 1 interface</title>
    <filename>group__flacpp__metadata__level1.html</filename>
    <class kind="class">FLAC::Metadata::SimpleIterator</class>
  </compound>
  <compound kind="group">
    <name>flacpp_metadata_level2</name>
    <title>FLAC++/metadata.h: metadata level 2 interface</title>
    <filename>group__flacpp__metadata__level2.html</filename>
    <class kind="class">FLAC::Metadata::Chain</class>
    <class kind="class">FLAC::Metadata::Iterator</class>
  </compound>
  <compound kind="dir">
    <name>include/FLAC/</name>
    <path>/home/jcoalson/flac/build-1.2.1/include/FLAC/</path>
    <filename>dir_000002.html</filename>
    <file>all.h</file>
    <file>assert.h</file>
    <file>callback.h</file>
    <file>export.h</file>
    <file>format.h</file>
    <file>metadata.h</file>
    <file>ordinals.h</file>
    <file>stream_decoder.h</file>
    <file>stream_encoder.h</file>
  </compound>
  <compound kind="dir">
    <name>include/FLAC++/</name>
    <path>/home/jcoalson/flac/build-1.2.1/include/FLAC++/</path>
    <filename>dir_000001.html</filename>
    <file>all.h</file>
    <file>decoder.h</file>
    <file>encoder.h</file>
    <file>export.h</file>
    <file>metadata.h</file>
  </compound>
  <compound kind="dir">
    <name>include/</name>
    <path>/home/jcoalson/flac/build-1.2.1/include/</path>
    <filename>dir_000000.html</filename>
    <dir>include/FLAC/</dir>
    <dir>include/FLAC++/</dir>
  </compound>
  <compound kind="class">
    <name>FLAC::Decoder::Stream</name>
    <filename>classFLAC_1_1Decoder_1_1Stream.html</filename>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>is_valid</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streama2</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>operator bool</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streama3</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>set_ogg_serial_number</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streama4</anchor>
      <arglist>(long value)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>set_md5_checking</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streama5</anchor>
      <arglist>(bool value)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>set_metadata_respond</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streama6</anchor>
      <arglist>(::FLAC__MetadataType type)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>set_metadata_respond_application</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streama7</anchor>
      <arglist>(const FLAC__byte id[4])</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>set_metadata_respond_all</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streama8</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>set_metadata_ignore</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streama9</anchor>
      <arglist>(::FLAC__MetadataType type)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>set_metadata_ignore_application</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streama10</anchor>
      <arglist>(const FLAC__byte id[4])</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>set_metadata_ignore_all</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streama11</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>State</type>
      <name>get_state</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streama12</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>get_md5_checking</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streama13</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual FLAC__uint64</type>
      <name>get_total_samples</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streama14</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual unsigned</type>
      <name>get_channels</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streama15</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual ::FLAC__ChannelAssignment</type>
      <name>get_channel_assignment</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streama16</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual unsigned</type>
      <name>get_bits_per_sample</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streama17</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual unsigned</type>
      <name>get_sample_rate</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streama18</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual unsigned</type>
      <name>get_blocksize</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streama19</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>get_decode_position</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streama20</anchor>
      <arglist>(FLAC__uint64 *position) const </arglist>
    </member>
    <member kind="function">
      <type>virtual ::FLAC__StreamDecoderInitStatus</type>
      <name>init</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streama21</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual ::FLAC__StreamDecoderInitStatus</type>
      <name>init_ogg</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streama22</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>finish</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streama23</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>flush</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streama24</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>reset</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streama25</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>process_single</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streama26</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>process_until_end_of_metadata</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streama27</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>process_until_end_of_stream</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streama28</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>skip_single_frame</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streama29</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>seek_absolute</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streama30</anchor>
      <arglist>(FLAC__uint64 sample)</arglist>
    </member>
    <member kind="function" protection="protected" virtualness="pure">
      <type>virtual ::FLAC__StreamDecoderReadStatus</type>
      <name>read_callback</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streamb0</anchor>
      <arglist>(FLAC__byte buffer[], size_t *bytes)=0</arglist>
    </member>
    <member kind="function" protection="protected" virtualness="virtual">
      <type>virtual ::FLAC__StreamDecoderSeekStatus</type>
      <name>seek_callback</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streamb1</anchor>
      <arglist>(FLAC__uint64 absolute_byte_offset)</arglist>
    </member>
    <member kind="function" protection="protected" virtualness="virtual">
      <type>virtual ::FLAC__StreamDecoderTellStatus</type>
      <name>tell_callback</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streamb2</anchor>
      <arglist>(FLAC__uint64 *absolute_byte_offset)</arglist>
    </member>
    <member kind="function" protection="protected" virtualness="virtual">
      <type>virtual ::FLAC__StreamDecoderLengthStatus</type>
      <name>length_callback</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streamb3</anchor>
      <arglist>(FLAC__uint64 *stream_length)</arglist>
    </member>
    <member kind="function" protection="protected" virtualness="virtual">
      <type>virtual bool</type>
      <name>eof_callback</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streamb4</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" protection="protected" virtualness="pure">
      <type>virtual ::FLAC__StreamDecoderWriteStatus</type>
      <name>write_callback</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streamb5</anchor>
      <arglist>(const ::FLAC__Frame *frame, const FLAC__int32 *const buffer[])=0</arglist>
    </member>
    <member kind="function" protection="protected" virtualness="virtual">
      <type>virtual void</type>
      <name>metadata_callback</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streamb6</anchor>
      <arglist>(const ::FLAC__StreamMetadata *metadata)</arglist>
    </member>
    <member kind="function" protection="protected" virtualness="pure">
      <type>virtual void</type>
      <name>error_callback</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streamb7</anchor>
      <arglist>(::FLAC__StreamDecoderErrorStatus status)=0</arglist>
    </member>
    <member kind="function" protection="protected" static="yes">
      <type>::FLAC__StreamDecoderReadStatus</type>
      <name>read_callback_</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streamf0</anchor>
      <arglist>(const ::FLAC__StreamDecoder *decoder, FLAC__byte buffer[], size_t *bytes, void *client_data)</arglist>
    </member>
    <member kind="function" protection="protected" static="yes">
      <type>::FLAC__StreamDecoderSeekStatus</type>
      <name>seek_callback_</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streamf1</anchor>
      <arglist>(const ::FLAC__StreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data)</arglist>
    </member>
    <member kind="function" protection="protected" static="yes">
      <type>::FLAC__StreamDecoderTellStatus</type>
      <name>tell_callback_</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streamf2</anchor>
      <arglist>(const ::FLAC__StreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data)</arglist>
    </member>
    <member kind="function" protection="protected" static="yes">
      <type>::FLAC__StreamDecoderLengthStatus</type>
      <name>length_callback_</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streamf3</anchor>
      <arglist>(const ::FLAC__StreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data)</arglist>
    </member>
    <member kind="function" protection="protected" static="yes">
      <type>static FLAC__bool</type>
      <name>eof_callback_</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streamf4</anchor>
      <arglist>(const ::FLAC__StreamDecoder *decoder, void *client_data)</arglist>
    </member>
    <member kind="function" protection="protected" static="yes">
      <type>::FLAC__StreamDecoderWriteStatus</type>
      <name>write_callback_</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streamf5</anchor>
      <arglist>(const ::FLAC__StreamDecoder *decoder, const ::FLAC__Frame *frame, const FLAC__int32 *const buffer[], void *client_data)</arglist>
    </member>
    <member kind="function" protection="protected" static="yes">
      <type>static void</type>
      <name>metadata_callback_</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streamf6</anchor>
      <arglist>(const ::FLAC__StreamDecoder *decoder, const ::FLAC__StreamMetadata *metadata, void *client_data)</arglist>
    </member>
    <member kind="function" protection="protected" static="yes">
      <type>static void</type>
      <name>error_callback_</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streamf7</anchor>
      <arglist>(const ::FLAC__StreamDecoder *decoder,::FLAC__StreamDecoderErrorStatus status, void *client_data)</arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>::FLAC__StreamDecoder *</type>
      <name>decoder_</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streamp0</anchor>
      <arglist></arglist>
    </member>
    <class kind="class">FLAC::Decoder::Stream::State</class>
  </compound>
  <compound kind="class">
    <name>FLAC::Decoder::Stream::State</name>
    <filename>classFLAC_1_1Decoder_1_1Stream_1_1State.html</filename>
    <member kind="function">
      <type></type>
      <name>State</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream_1_1State.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Stream_1_1Statea0</anchor>
      <arglist>(::FLAC__StreamDecoderState state)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>operator::FLAC__StreamDecoderState</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream_1_1State.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Stream_1_1Statea1</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>const char *</type>
      <name>as_cstring</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream_1_1State.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Stream_1_1Statea2</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>const char *</type>
      <name>resolved_as_cstring</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream_1_1State.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Stream_1_1Statea3</anchor>
      <arglist>(const Stream &amp;decoder) const </arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>::FLAC__StreamDecoderState</type>
      <name>state_</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream_1_1State.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Stream_1_1Statep0</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>FLAC::Decoder::File</name>
    <filename>classFLAC_1_1Decoder_1_1File.html</filename>
    <base>FLAC::Decoder::Stream</base>
    <member kind="function">
      <type>virtual ::FLAC__StreamDecoderInitStatus</type>
      <name>init</name>
      <anchorfile>classFLAC_1_1Decoder_1_1File.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Filea2</anchor>
      <arglist>(FILE *file)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual ::FLAC__StreamDecoderInitStatus</type>
      <name>init</name>
      <anchorfile>classFLAC_1_1Decoder_1_1File.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Filea3</anchor>
      <arglist>(const char *filename)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual ::FLAC__StreamDecoderInitStatus</type>
      <name>init</name>
      <anchorfile>classFLAC_1_1Decoder_1_1File.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Filea4</anchor>
      <arglist>(const std::string &amp;filename)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual ::FLAC__StreamDecoderInitStatus</type>
      <name>init_ogg</name>
      <anchorfile>classFLAC_1_1Decoder_1_1File.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Filea5</anchor>
      <arglist>(FILE *file)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual ::FLAC__StreamDecoderInitStatus</type>
      <name>init_ogg</name>
      <anchorfile>classFLAC_1_1Decoder_1_1File.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Filea6</anchor>
      <arglist>(const char *filename)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual ::FLAC__StreamDecoderInitStatus</type>
      <name>init_ogg</name>
      <anchorfile>classFLAC_1_1Decoder_1_1File.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Filea7</anchor>
      <arglist>(const std::string &amp;filename)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>is_valid</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streama2</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>operator bool</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streama3</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>set_ogg_serial_number</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streama4</anchor>
      <arglist>(long value)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>set_md5_checking</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streama5</anchor>
      <arglist>(bool value)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>set_metadata_respond</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streama6</anchor>
      <arglist>(::FLAC__MetadataType type)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>set_metadata_respond_application</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streama7</anchor>
      <arglist>(const FLAC__byte id[4])</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>set_metadata_respond_all</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streama8</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>set_metadata_ignore</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streama9</anchor>
      <arglist>(::FLAC__MetadataType type)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>set_metadata_ignore_application</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streama10</anchor>
      <arglist>(const FLAC__byte id[4])</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>set_metadata_ignore_all</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streama11</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>State</type>
      <name>get_state</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streama12</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>get_md5_checking</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streama13</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual FLAC__uint64</type>
      <name>get_total_samples</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streama14</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual unsigned</type>
      <name>get_channels</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streama15</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual ::FLAC__ChannelAssignment</type>
      <name>get_channel_assignment</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streama16</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual unsigned</type>
      <name>get_bits_per_sample</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streama17</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual unsigned</type>
      <name>get_sample_rate</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streama18</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual unsigned</type>
      <name>get_blocksize</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streama19</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>get_decode_position</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streama20</anchor>
      <arglist>(FLAC__uint64 *position) const </arglist>
    </member>
    <member kind="function">
      <type>virtual ::FLAC__StreamDecoderInitStatus</type>
      <name>init</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streama21</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual ::FLAC__StreamDecoderInitStatus</type>
      <name>init_ogg</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streama22</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>finish</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streama23</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>flush</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streama24</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>reset</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streama25</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>process_single</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streama26</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>process_until_end_of_metadata</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streama27</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>process_until_end_of_stream</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streama28</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>skip_single_frame</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streama29</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>seek_absolute</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streama30</anchor>
      <arglist>(FLAC__uint64 sample)</arglist>
    </member>
    <member kind="function" protection="protected">
      <type>virtual ::FLAC__StreamDecoderReadStatus</type>
      <name>read_callback</name>
      <anchorfile>classFLAC_1_1Decoder_1_1File.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Fileb0</anchor>
      <arglist>(FLAC__byte buffer[], size_t *bytes)</arglist>
    </member>
    <member kind="function" protection="protected" virtualness="virtual">
      <type>virtual ::FLAC__StreamDecoderSeekStatus</type>
      <name>seek_callback</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streamb1</anchor>
      <arglist>(FLAC__uint64 absolute_byte_offset)</arglist>
    </member>
    <member kind="function" protection="protected" virtualness="virtual">
      <type>virtual ::FLAC__StreamDecoderTellStatus</type>
      <name>tell_callback</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streamb2</anchor>
      <arglist>(FLAC__uint64 *absolute_byte_offset)</arglist>
    </member>
    <member kind="function" protection="protected" virtualness="virtual">
      <type>virtual ::FLAC__StreamDecoderLengthStatus</type>
      <name>length_callback</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streamb3</anchor>
      <arglist>(FLAC__uint64 *stream_length)</arglist>
    </member>
    <member kind="function" protection="protected" virtualness="virtual">
      <type>virtual bool</type>
      <name>eof_callback</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streamb4</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" protection="protected" virtualness="pure">
      <type>virtual ::FLAC__StreamDecoderWriteStatus</type>
      <name>write_callback</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streamb5</anchor>
      <arglist>(const ::FLAC__Frame *frame, const FLAC__int32 *const buffer[])=0</arglist>
    </member>
    <member kind="function" protection="protected" virtualness="virtual">
      <type>virtual void</type>
      <name>metadata_callback</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streamb6</anchor>
      <arglist>(const ::FLAC__StreamMetadata *metadata)</arglist>
    </member>
    <member kind="function" protection="protected" virtualness="pure">
      <type>virtual void</type>
      <name>error_callback</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streamb7</anchor>
      <arglist>(::FLAC__StreamDecoderErrorStatus status)=0</arglist>
    </member>
    <member kind="function" protection="protected" static="yes">
      <type>::FLAC__StreamDecoderReadStatus</type>
      <name>read_callback_</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streamf0</anchor>
      <arglist>(const ::FLAC__StreamDecoder *decoder, FLAC__byte buffer[], size_t *bytes, void *client_data)</arglist>
    </member>
    <member kind="function" protection="protected" static="yes">
      <type>::FLAC__StreamDecoderSeekStatus</type>
      <name>seek_callback_</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streamf1</anchor>
      <arglist>(const ::FLAC__StreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data)</arglist>
    </member>
    <member kind="function" protection="protected" static="yes">
      <type>::FLAC__StreamDecoderTellStatus</type>
      <name>tell_callback_</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streamf2</anchor>
      <arglist>(const ::FLAC__StreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data)</arglist>
    </member>
    <member kind="function" protection="protected" static="yes">
      <type>::FLAC__StreamDecoderLengthStatus</type>
      <name>length_callback_</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streamf3</anchor>
      <arglist>(const ::FLAC__StreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data)</arglist>
    </member>
    <member kind="function" protection="protected" static="yes">
      <type>static FLAC__bool</type>
      <name>eof_callback_</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streamf4</anchor>
      <arglist>(const ::FLAC__StreamDecoder *decoder, void *client_data)</arglist>
    </member>
    <member kind="function" protection="protected" static="yes">
      <type>::FLAC__StreamDecoderWriteStatus</type>
      <name>write_callback_</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streamf5</anchor>
      <arglist>(const ::FLAC__StreamDecoder *decoder, const ::FLAC__Frame *frame, const FLAC__int32 *const buffer[], void *client_data)</arglist>
    </member>
    <member kind="function" protection="protected" static="yes">
      <type>static void</type>
      <name>metadata_callback_</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streamf6</anchor>
      <arglist>(const ::FLAC__StreamDecoder *decoder, const ::FLAC__StreamMetadata *metadata, void *client_data)</arglist>
    </member>
    <member kind="function" protection="protected" static="yes">
      <type>static void</type>
      <name>error_callback_</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streamf7</anchor>
      <arglist>(const ::FLAC__StreamDecoder *decoder,::FLAC__StreamDecoderErrorStatus status, void *client_data)</arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>::FLAC__StreamDecoder *</type>
      <name>decoder_</name>
      <anchorfile>classFLAC_1_1Decoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Decoder_1_1Streamp0</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>FLAC::Encoder::Stream</name>
    <filename>classFLAC_1_1Encoder_1_1Stream.html</filename>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>is_valid</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama2</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>operator bool</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama3</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>set_ogg_serial_number</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama4</anchor>
      <arglist>(long value)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>set_verify</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama5</anchor>
      <arglist>(bool value)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>set_streamable_subset</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama6</anchor>
      <arglist>(bool value)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>set_channels</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama7</anchor>
      <arglist>(unsigned value)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>set_bits_per_sample</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama8</anchor>
      <arglist>(unsigned value)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>set_sample_rate</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama9</anchor>
      <arglist>(unsigned value)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>set_compression_level</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama10</anchor>
      <arglist>(unsigned value)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>set_blocksize</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama11</anchor>
      <arglist>(unsigned value)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>set_do_mid_side_stereo</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama12</anchor>
      <arglist>(bool value)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>set_loose_mid_side_stereo</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama13</anchor>
      <arglist>(bool value)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>set_apodization</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama14</anchor>
      <arglist>(const char *specification)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>set_max_lpc_order</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama15</anchor>
      <arglist>(unsigned value)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>set_qlp_coeff_precision</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama16</anchor>
      <arglist>(unsigned value)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>set_do_qlp_coeff_prec_search</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama17</anchor>
      <arglist>(bool value)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>set_do_escape_coding</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama18</anchor>
      <arglist>(bool value)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>set_do_exhaustive_model_search</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama19</anchor>
      <arglist>(bool value)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>set_min_residual_partition_order</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama20</anchor>
      <arglist>(unsigned value)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>set_max_residual_partition_order</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama21</anchor>
      <arglist>(unsigned value)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>set_rice_parameter_search_dist</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama22</anchor>
      <arglist>(unsigned value)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>set_total_samples_estimate</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama23</anchor>
      <arglist>(FLAC__uint64 value)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>set_metadata</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama24</anchor>
      <arglist>(::FLAC__StreamMetadata **metadata, unsigned num_blocks)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>set_metadata</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama25</anchor>
      <arglist>(FLAC::Metadata::Prototype **metadata, unsigned num_blocks)</arglist>
    </member>
    <member kind="function">
      <type>State</type>
      <name>get_state</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama26</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual Decoder::Stream::State</type>
      <name>get_verify_decoder_state</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama27</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual void</type>
      <name>get_verify_decoder_error_stats</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama28</anchor>
      <arglist>(FLAC__uint64 *absolute_sample, unsigned *frame_number, unsigned *channel, unsigned *sample, FLAC__int32 *expected, FLAC__int32 *got)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>get_verify</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama29</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>get_streamable_subset</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama30</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>get_do_mid_side_stereo</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama31</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>get_loose_mid_side_stereo</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama32</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual unsigned</type>
      <name>get_channels</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama33</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual unsigned</type>
      <name>get_bits_per_sample</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama34</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual unsigned</type>
      <name>get_sample_rate</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama35</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual unsigned</type>
      <name>get_blocksize</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama36</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual unsigned</type>
      <name>get_max_lpc_order</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama37</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual unsigned</type>
      <name>get_qlp_coeff_precision</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama38</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>get_do_qlp_coeff_prec_search</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama39</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>get_do_escape_coding</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama40</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>get_do_exhaustive_model_search</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama41</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual unsigned</type>
      <name>get_min_residual_partition_order</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama42</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual unsigned</type>
      <name>get_max_residual_partition_order</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama43</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual unsigned</type>
      <name>get_rice_parameter_search_dist</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama44</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual FLAC__uint64</type>
      <name>get_total_samples_estimate</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama45</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>virtual ::FLAC__StreamEncoderInitStatus</type>
      <name>init</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama46</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual ::FLAC__StreamEncoderInitStatus</type>
      <name>init_ogg</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama47</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>finish</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama48</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>process</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama49</anchor>
      <arglist>(const FLAC__int32 *const buffer[], unsigned samples)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>process_interleaved</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama50</anchor>
      <arglist>(const FLAC__int32 buffer[], unsigned samples)</arglist>
    </member>
    <member kind="function" protection="protected" virtualness="virtual">
      <type>virtual ::FLAC__StreamEncoderReadStatus</type>
      <name>read_callback</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streamb0</anchor>
      <arglist>(FLAC__byte buffer[], size_t *bytes)</arglist>
    </member>
    <member kind="function" protection="protected" virtualness="pure">
      <type>virtual ::FLAC__StreamEncoderWriteStatus</type>
      <name>write_callback</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streamb1</anchor>
      <arglist>(const FLAC__byte buffer[], size_t bytes, unsigned samples, unsigned current_frame)=0</arglist>
    </member>
    <member kind="function" protection="protected" virtualness="virtual">
      <type>virtual ::FLAC__StreamEncoderSeekStatus</type>
      <name>seek_callback</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streamb2</anchor>
      <arglist>(FLAC__uint64 absolute_byte_offset)</arglist>
    </member>
    <member kind="function" protection="protected" virtualness="virtual">
      <type>virtual ::FLAC__StreamEncoderTellStatus</type>
      <name>tell_callback</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streamb3</anchor>
      <arglist>(FLAC__uint64 *absolute_byte_offset)</arglist>
    </member>
    <member kind="function" protection="protected" virtualness="virtual">
      <type>virtual void</type>
      <name>metadata_callback</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streamb4</anchor>
      <arglist>(const ::FLAC__StreamMetadata *metadata)</arglist>
    </member>
    <member kind="function" protection="protected" static="yes">
      <type>::FLAC__StreamEncoderReadStatus</type>
      <name>read_callback_</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streamf0</anchor>
      <arglist>(const ::FLAC__StreamEncoder *encoder, FLAC__byte buffer[], size_t *bytes, void *client_data)</arglist>
    </member>
    <member kind="function" protection="protected" static="yes">
      <type>::FLAC__StreamEncoderWriteStatus</type>
      <name>write_callback_</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streamf1</anchor>
      <arglist>(const ::FLAC__StreamEncoder *encoder, const FLAC__byte buffer[], size_t bytes, unsigned samples, unsigned current_frame, void *client_data)</arglist>
    </member>
    <member kind="function" protection="protected" static="yes">
      <type>::FLAC__StreamEncoderSeekStatus</type>
      <name>seek_callback_</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streamf2</anchor>
      <arglist>(const FLAC__StreamEncoder *encoder, FLAC__uint64 absolute_byte_offset, void *client_data)</arglist>
    </member>
    <member kind="function" protection="protected" static="yes">
      <type>::FLAC__StreamEncoderTellStatus</type>
      <name>tell_callback_</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streamf3</anchor>
      <arglist>(const FLAC__StreamEncoder *encoder, FLAC__uint64 *absolute_byte_offset, void *client_data)</arglist>
    </member>
    <member kind="function" protection="protected" static="yes">
      <type>static void</type>
      <name>metadata_callback_</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streamf4</anchor>
      <arglist>(const ::FLAC__StreamEncoder *encoder, const ::FLAC__StreamMetadata *metadata, void *client_data)</arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>::FLAC__StreamEncoder *</type>
      <name>encoder_</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streamp0</anchor>
      <arglist></arglist>
    </member>
    <class kind="class">FLAC::Encoder::Stream::State</class>
  </compound>
  <compound kind="class">
    <name>FLAC::Encoder::Stream::State</name>
    <filename>classFLAC_1_1Encoder_1_1Stream_1_1State.html</filename>
    <member kind="function">
      <type></type>
      <name>State</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream_1_1State.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Stream_1_1Statea0</anchor>
      <arglist>(::FLAC__StreamEncoderState state)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>operator::FLAC__StreamEncoderState</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream_1_1State.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Stream_1_1Statea1</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>const char *</type>
      <name>as_cstring</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream_1_1State.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Stream_1_1Statea2</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>const char *</type>
      <name>resolved_as_cstring</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream_1_1State.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Stream_1_1Statea3</anchor>
      <arglist>(const Stream &amp;encoder) const </arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>::FLAC__StreamEncoderState</type>
      <name>state_</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream_1_1State.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Stream_1_1Statep0</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>FLAC::Encoder::File</name>
    <filename>classFLAC_1_1Encoder_1_1File.html</filename>
    <base>FLAC::Encoder::Stream</base>
    <member kind="function">
      <type>virtual ::FLAC__StreamEncoderInitStatus</type>
      <name>init</name>
      <anchorfile>classFLAC_1_1Encoder_1_1File.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Filea2</anchor>
      <arglist>(FILE *file)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual ::FLAC__StreamEncoderInitStatus</type>
      <name>init</name>
      <anchorfile>classFLAC_1_1Encoder_1_1File.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Filea3</anchor>
      <arglist>(const char *filename)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual ::FLAC__StreamEncoderInitStatus</type>
      <name>init</name>
      <anchorfile>classFLAC_1_1Encoder_1_1File.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Filea4</anchor>
      <arglist>(const std::string &amp;filename)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual ::FLAC__StreamEncoderInitStatus</type>
      <name>init_ogg</name>
      <anchorfile>classFLAC_1_1Encoder_1_1File.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Filea5</anchor>
      <arglist>(FILE *file)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual ::FLAC__StreamEncoderInitStatus</type>
      <name>init_ogg</name>
      <anchorfile>classFLAC_1_1Encoder_1_1File.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Filea6</anchor>
      <arglist>(const char *filename)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual ::FLAC__StreamEncoderInitStatus</type>
      <name>init_ogg</name>
      <anchorfile>classFLAC_1_1Encoder_1_1File.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Filea7</anchor>
      <arglist>(const std::string &amp;filename)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>is_valid</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama2</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>operator bool</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama3</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>set_ogg_serial_number</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama4</anchor>
      <arglist>(long value)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>set_verify</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama5</anchor>
      <arglist>(bool value)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>set_streamable_subset</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama6</anchor>
      <arglist>(bool value)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>set_channels</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama7</anchor>
      <arglist>(unsigned value)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>set_bits_per_sample</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama8</anchor>
      <arglist>(unsigned value)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>set_sample_rate</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama9</anchor>
      <arglist>(unsigned value)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>set_compression_level</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama10</anchor>
      <arglist>(unsigned value)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>set_blocksize</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama11</anchor>
      <arglist>(unsigned value)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>set_do_mid_side_stereo</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama12</anchor>
      <arglist>(bool value)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>set_loose_mid_side_stereo</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama13</anchor>
      <arglist>(bool value)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>set_apodization</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama14</anchor>
      <arglist>(const char *specification)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>set_max_lpc_order</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama15</anchor>
      <arglist>(unsigned value)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>set_qlp_coeff_precision</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama16</anchor>
      <arglist>(unsigned value)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>set_do_qlp_coeff_prec_search</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama17</anchor>
      <arglist>(bool value)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>set_do_escape_coding</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama18</anchor>
      <arglist>(bool value)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>set_do_exhaustive_model_search</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama19</anchor>
      <arglist>(bool value)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>set_min_residual_partition_order</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama20</anchor>
      <arglist>(unsigned value)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>set_max_residual_partition_order</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama21</anchor>
      <arglist>(unsigned value)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>set_rice_parameter_search_dist</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama22</anchor>
      <arglist>(unsigned value)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>set_total_samples_estimate</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama23</anchor>
      <arglist>(FLAC__uint64 value)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>set_metadata</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama24</anchor>
      <arglist>(::FLAC__StreamMetadata **metadata, unsigned num_blocks)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>set_metadata</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama25</anchor>
      <arglist>(FLAC::Metadata::Prototype **metadata, unsigned num_blocks)</arglist>
    </member>
    <member kind="function">
      <type>State</type>
      <name>get_state</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama26</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual Decoder::Stream::State</type>
      <name>get_verify_decoder_state</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama27</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual void</type>
      <name>get_verify_decoder_error_stats</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama28</anchor>
      <arglist>(FLAC__uint64 *absolute_sample, unsigned *frame_number, unsigned *channel, unsigned *sample, FLAC__int32 *expected, FLAC__int32 *got)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>get_verify</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama29</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>get_streamable_subset</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama30</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>get_do_mid_side_stereo</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama31</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>get_loose_mid_side_stereo</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama32</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual unsigned</type>
      <name>get_channels</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama33</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual unsigned</type>
      <name>get_bits_per_sample</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama34</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual unsigned</type>
      <name>get_sample_rate</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama35</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual unsigned</type>
      <name>get_blocksize</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama36</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual unsigned</type>
      <name>get_max_lpc_order</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama37</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual unsigned</type>
      <name>get_qlp_coeff_precision</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama38</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>get_do_qlp_coeff_prec_search</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama39</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>get_do_escape_coding</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama40</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>get_do_exhaustive_model_search</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama41</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual unsigned</type>
      <name>get_min_residual_partition_order</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama42</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual unsigned</type>
      <name>get_max_residual_partition_order</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama43</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual unsigned</type>
      <name>get_rice_parameter_search_dist</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama44</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual FLAC__uint64</type>
      <name>get_total_samples_estimate</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama45</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>virtual ::FLAC__StreamEncoderInitStatus</type>
      <name>init</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama46</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual ::FLAC__StreamEncoderInitStatus</type>
      <name>init_ogg</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama47</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>finish</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama48</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>process</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama49</anchor>
      <arglist>(const FLAC__int32 *const buffer[], unsigned samples)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>process_interleaved</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streama50</anchor>
      <arglist>(const FLAC__int32 buffer[], unsigned samples)</arglist>
    </member>
    <member kind="function" protection="protected" virtualness="virtual">
      <type>virtual void</type>
      <name>progress_callback</name>
      <anchorfile>classFLAC_1_1Encoder_1_1File.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Fileb0</anchor>
      <arglist>(FLAC__uint64 bytes_written, FLAC__uint64 samples_written, unsigned frames_written, unsigned total_frames_estimate)</arglist>
    </member>
    <member kind="function" protection="protected" virtualness="virtual">
      <type>virtual ::FLAC__StreamEncoderWriteStatus</type>
      <name>write_callback</name>
      <anchorfile>classFLAC_1_1Encoder_1_1File.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Fileb1</anchor>
      <arglist>(const FLAC__byte buffer[], size_t bytes, unsigned samples, unsigned current_frame)</arglist>
    </member>
    <member kind="function" protection="protected" virtualness="virtual">
      <type>virtual ::FLAC__StreamEncoderReadStatus</type>
      <name>read_callback</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streamb0</anchor>
      <arglist>(FLAC__byte buffer[], size_t *bytes)</arglist>
    </member>
    <member kind="function" protection="protected" virtualness="virtual">
      <type>virtual ::FLAC__StreamEncoderSeekStatus</type>
      <name>seek_callback</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streamb2</anchor>
      <arglist>(FLAC__uint64 absolute_byte_offset)</arglist>
    </member>
    <member kind="function" protection="protected" virtualness="virtual">
      <type>virtual ::FLAC__StreamEncoderTellStatus</type>
      <name>tell_callback</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streamb3</anchor>
      <arglist>(FLAC__uint64 *absolute_byte_offset)</arglist>
    </member>
    <member kind="function" protection="protected" virtualness="virtual">
      <type>virtual void</type>
      <name>metadata_callback</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streamb4</anchor>
      <arglist>(const ::FLAC__StreamMetadata *metadata)</arglist>
    </member>
    <member kind="function" protection="protected" static="yes">
      <type>::FLAC__StreamEncoderReadStatus</type>
      <name>read_callback_</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streamf0</anchor>
      <arglist>(const ::FLAC__StreamEncoder *encoder, FLAC__byte buffer[], size_t *bytes, void *client_data)</arglist>
    </member>
    <member kind="function" protection="protected" static="yes">
      <type>::FLAC__StreamEncoderWriteStatus</type>
      <name>write_callback_</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streamf1</anchor>
      <arglist>(const ::FLAC__StreamEncoder *encoder, const FLAC__byte buffer[], size_t bytes, unsigned samples, unsigned current_frame, void *client_data)</arglist>
    </member>
    <member kind="function" protection="protected" static="yes">
      <type>::FLAC__StreamEncoderSeekStatus</type>
      <name>seek_callback_</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streamf2</anchor>
      <arglist>(const FLAC__StreamEncoder *encoder, FLAC__uint64 absolute_byte_offset, void *client_data)</arglist>
    </member>
    <member kind="function" protection="protected" static="yes">
      <type>::FLAC__StreamEncoderTellStatus</type>
      <name>tell_callback_</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streamf3</anchor>
      <arglist>(const FLAC__StreamEncoder *encoder, FLAC__uint64 *absolute_byte_offset, void *client_data)</arglist>
    </member>
    <member kind="function" protection="protected" static="yes">
      <type>static void</type>
      <name>metadata_callback_</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streamf4</anchor>
      <arglist>(const ::FLAC__StreamEncoder *encoder, const ::FLAC__StreamMetadata *metadata, void *client_data)</arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>::FLAC__StreamEncoder *</type>
      <name>encoder_</name>
      <anchorfile>classFLAC_1_1Encoder_1_1Stream.html</anchorfile>
      <anchor>FLAC_1_1Encoder_1_1Streamp0</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>FLAC::Metadata::Prototype</name>
    <filename>classFLAC_1_1Metadata_1_1Prototype.html</filename>
    <member kind="function" virtualness="virtual">
      <type>virtual</type>
      <name>~Prototype</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Prototypea0</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator==</name>
      <anchorfile>group__flacpp__metadata__object.html</anchorfile>
      <anchor>ga1</anchor>
      <arglist>(const Prototype &amp;) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator==</name>
      <anchorfile>group__flacpp__metadata__object.html</anchorfile>
      <anchor>ga2</anchor>
      <arglist>(const ::FLAC__StreamMetadata &amp;) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator==</name>
      <anchorfile>group__flacpp__metadata__object.html</anchorfile>
      <anchor>ga3</anchor>
      <arglist>(const ::FLAC__StreamMetadata *) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator!=</name>
      <anchorfile>group__flacpp__metadata__object.html</anchorfile>
      <anchor>ga4</anchor>
      <arglist>(const Prototype &amp;) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator!=</name>
      <anchorfile>group__flacpp__metadata__object.html</anchorfile>
      <anchor>ga5</anchor>
      <arglist>(const ::FLAC__StreamMetadata &amp;) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator!=</name>
      <anchorfile>group__flacpp__metadata__object.html</anchorfile>
      <anchor>ga6</anchor>
      <arglist>(const ::FLAC__StreamMetadata *) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>is_valid</name>
      <anchorfile>group__flacpp__metadata__object.html</anchorfile>
      <anchor>ga7</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>get_is_last</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommenta27</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>::FLAC__MetadataType</type>
      <name>get_type</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommenta28</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>unsigned</type>
      <name>get_length</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommenta29</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>set_is_last</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommenta30</anchor>
      <arglist>(bool)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>operator const ::FLAC__StreamMetadata *</name>
      <anchorfile>group__flacpp__metadata__object.html</anchorfile>
      <anchor>ga8</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" protection="protected">
      <type></type>
      <name>Prototype</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Prototypeb0</anchor>
      <arglist>(const Prototype &amp;)</arglist>
    </member>
    <member kind="function" protection="protected">
      <type></type>
      <name>Prototype</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Prototypeb1</anchor>
      <arglist>(const ::FLAC__StreamMetadata &amp;)</arglist>
    </member>
    <member kind="function" protection="protected">
      <type></type>
      <name>Prototype</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Prototypeb2</anchor>
      <arglist>(const ::FLAC__StreamMetadata *)</arglist>
    </member>
    <member kind="function" protection="protected">
      <type></type>
      <name>Prototype</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Prototypeb3</anchor>
      <arglist>(::FLAC__StreamMetadata *object, bool copy)</arglist>
    </member>
    <member kind="function" protection="protected">
      <type>Prototype &amp;</type>
      <name>operator=</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Prototypeb4</anchor>
      <arglist>(const Prototype &amp;)</arglist>
    </member>
    <member kind="function" protection="protected">
      <type>Prototype &amp;</type>
      <name>operator=</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Prototypeb5</anchor>
      <arglist>(const ::FLAC__StreamMetadata &amp;)</arglist>
    </member>
    <member kind="function" protection="protected">
      <type>Prototype &amp;</type>
      <name>operator=</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Prototypeb6</anchor>
      <arglist>(const ::FLAC__StreamMetadata *)</arglist>
    </member>
    <member kind="function" protection="protected">
      <type>Prototype &amp;</type>
      <name>assign_object</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommentb0</anchor>
      <arglist>(::FLAC__StreamMetadata *object, bool copy)</arglist>
    </member>
    <member kind="function" protection="protected" virtualness="virtual">
      <type>virtual void</type>
      <name>clear</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommentb1</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>::FLAC__StreamMetadata *</type>
      <name>object_</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommentp0</anchor>
      <arglist></arglist>
    </member>
    <member kind="friend">
      <type>friend class</type>
      <name>SimpleIterator</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommentn0</anchor>
      <arglist></arglist>
    </member>
    <member kind="friend">
      <type>friend class</type>
      <name>Iterator</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommentn1</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>FLAC::Metadata::StreamInfo</name>
    <filename>classFLAC_1_1Metadata_1_1StreamInfo.html</filename>
    <base>FLAC::Metadata::Prototype</base>
    <member kind="function">
      <type></type>
      <name>StreamInfo</name>
      <anchorfile>classFLAC_1_1Metadata_1_1StreamInfo.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1StreamInfoa1</anchor>
      <arglist>(const StreamInfo &amp;object)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>StreamInfo</name>
      <anchorfile>classFLAC_1_1Metadata_1_1StreamInfo.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1StreamInfoa2</anchor>
      <arglist>(const ::FLAC__StreamMetadata &amp;object)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>StreamInfo</name>
      <anchorfile>classFLAC_1_1Metadata_1_1StreamInfo.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1StreamInfoa3</anchor>
      <arglist>(const ::FLAC__StreamMetadata *object)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>StreamInfo</name>
      <anchorfile>classFLAC_1_1Metadata_1_1StreamInfo.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1StreamInfoa4</anchor>
      <arglist>(::FLAC__StreamMetadata *object, bool copy)</arglist>
    </member>
    <member kind="function">
      <type>StreamInfo &amp;</type>
      <name>operator=</name>
      <anchorfile>classFLAC_1_1Metadata_1_1StreamInfo.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1StreamInfoa6</anchor>
      <arglist>(const StreamInfo &amp;object)</arglist>
    </member>
    <member kind="function">
      <type>StreamInfo &amp;</type>
      <name>operator=</name>
      <anchorfile>classFLAC_1_1Metadata_1_1StreamInfo.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1StreamInfoa7</anchor>
      <arglist>(const ::FLAC__StreamMetadata &amp;object)</arglist>
    </member>
    <member kind="function">
      <type>StreamInfo &amp;</type>
      <name>operator=</name>
      <anchorfile>classFLAC_1_1Metadata_1_1StreamInfo.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1StreamInfoa8</anchor>
      <arglist>(const ::FLAC__StreamMetadata *object)</arglist>
    </member>
    <member kind="function">
      <type>StreamInfo &amp;</type>
      <name>assign</name>
      <anchorfile>classFLAC_1_1Metadata_1_1StreamInfo.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1StreamInfoa9</anchor>
      <arglist>(::FLAC__StreamMetadata *object, bool copy)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator==</name>
      <anchorfile>classFLAC_1_1Metadata_1_1StreamInfo.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1StreamInfoa10</anchor>
      <arglist>(const StreamInfo &amp;object) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator==</name>
      <anchorfile>classFLAC_1_1Metadata_1_1StreamInfo.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1StreamInfoa11</anchor>
      <arglist>(const ::FLAC__StreamMetadata &amp;object) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator==</name>
      <anchorfile>classFLAC_1_1Metadata_1_1StreamInfo.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1StreamInfoa12</anchor>
      <arglist>(const ::FLAC__StreamMetadata *object) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator!=</name>
      <anchorfile>classFLAC_1_1Metadata_1_1StreamInfo.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1StreamInfoa13</anchor>
      <arglist>(const StreamInfo &amp;object) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator!=</name>
      <anchorfile>classFLAC_1_1Metadata_1_1StreamInfo.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1StreamInfoa14</anchor>
      <arglist>(const ::FLAC__StreamMetadata &amp;object) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator!=</name>
      <anchorfile>classFLAC_1_1Metadata_1_1StreamInfo.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1StreamInfoa15</anchor>
      <arglist>(const ::FLAC__StreamMetadata *object) const </arglist>
    </member>
    <member kind="function">
      <type>unsigned</type>
      <name>get_min_blocksize</name>
      <anchorfile>classFLAC_1_1Metadata_1_1StreamInfo.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1StreamInfoa16</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>unsigned</type>
      <name>get_max_blocksize</name>
      <anchorfile>classFLAC_1_1Metadata_1_1StreamInfo.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1StreamInfoa17</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>unsigned</type>
      <name>get_min_framesize</name>
      <anchorfile>classFLAC_1_1Metadata_1_1StreamInfo.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1StreamInfoa18</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>unsigned</type>
      <name>get_max_framesize</name>
      <anchorfile>classFLAC_1_1Metadata_1_1StreamInfo.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1StreamInfoa19</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>unsigned</type>
      <name>get_sample_rate</name>
      <anchorfile>classFLAC_1_1Metadata_1_1StreamInfo.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1StreamInfoa20</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>unsigned</type>
      <name>get_channels</name>
      <anchorfile>classFLAC_1_1Metadata_1_1StreamInfo.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1StreamInfoa21</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>unsigned</type>
      <name>get_bits_per_sample</name>
      <anchorfile>classFLAC_1_1Metadata_1_1StreamInfo.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1StreamInfoa22</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>FLAC__uint64</type>
      <name>get_total_samples</name>
      <anchorfile>classFLAC_1_1Metadata_1_1StreamInfo.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1StreamInfoa23</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>const FLAC__byte *</type>
      <name>get_md5sum</name>
      <anchorfile>classFLAC_1_1Metadata_1_1StreamInfo.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1StreamInfoa24</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>set_min_blocksize</name>
      <anchorfile>classFLAC_1_1Metadata_1_1StreamInfo.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1StreamInfoa25</anchor>
      <arglist>(unsigned value)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>set_max_blocksize</name>
      <anchorfile>classFLAC_1_1Metadata_1_1StreamInfo.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1StreamInfoa26</anchor>
      <arglist>(unsigned value)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>set_min_framesize</name>
      <anchorfile>classFLAC_1_1Metadata_1_1StreamInfo.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1StreamInfoa27</anchor>
      <arglist>(unsigned value)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>set_max_framesize</name>
      <anchorfile>classFLAC_1_1Metadata_1_1StreamInfo.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1StreamInfoa28</anchor>
      <arglist>(unsigned value)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>set_sample_rate</name>
      <anchorfile>classFLAC_1_1Metadata_1_1StreamInfo.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1StreamInfoa29</anchor>
      <arglist>(unsigned value)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>set_channels</name>
      <anchorfile>classFLAC_1_1Metadata_1_1StreamInfo.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1StreamInfoa30</anchor>
      <arglist>(unsigned value)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>set_bits_per_sample</name>
      <anchorfile>classFLAC_1_1Metadata_1_1StreamInfo.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1StreamInfoa31</anchor>
      <arglist>(unsigned value)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>set_total_samples</name>
      <anchorfile>classFLAC_1_1Metadata_1_1StreamInfo.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1StreamInfoa32</anchor>
      <arglist>(FLAC__uint64 value)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>set_md5sum</name>
      <anchorfile>classFLAC_1_1Metadata_1_1StreamInfo.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1StreamInfoa33</anchor>
      <arglist>(const FLAC__byte value[16])</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator==</name>
      <anchorfile>group__flacpp__metadata__object.html</anchorfile>
      <anchor>ga1</anchor>
      <arglist>(const Prototype &amp;) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator!=</name>
      <anchorfile>group__flacpp__metadata__object.html</anchorfile>
      <anchor>ga4</anchor>
      <arglist>(const Prototype &amp;) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>is_valid</name>
      <anchorfile>group__flacpp__metadata__object.html</anchorfile>
      <anchor>ga7</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>get_is_last</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommenta27</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>::FLAC__MetadataType</type>
      <name>get_type</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommenta28</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>unsigned</type>
      <name>get_length</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommenta29</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>set_is_last</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommenta30</anchor>
      <arglist>(bool)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>operator const ::FLAC__StreamMetadata *</name>
      <anchorfile>group__flacpp__metadata__object.html</anchorfile>
      <anchor>ga8</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" protection="protected">
      <type>Prototype &amp;</type>
      <name>assign_object</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommentb0</anchor>
      <arglist>(::FLAC__StreamMetadata *object, bool copy)</arglist>
    </member>
    <member kind="function" protection="protected" virtualness="virtual">
      <type>virtual void</type>
      <name>clear</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommentb1</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>::FLAC__StreamMetadata *</type>
      <name>object_</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommentp0</anchor>
      <arglist></arglist>
    </member>
    <member kind="friend">
      <type>friend class</type>
      <name>SimpleIterator</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommentn0</anchor>
      <arglist></arglist>
    </member>
    <member kind="friend">
      <type>friend class</type>
      <name>Iterator</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommentn1</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>FLAC::Metadata::Padding</name>
    <filename>classFLAC_1_1Metadata_1_1Padding.html</filename>
    <base>FLAC::Metadata::Prototype</base>
    <member kind="function">
      <type></type>
      <name>Padding</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Padding.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Paddinga1</anchor>
      <arglist>(const Padding &amp;object)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Padding</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Padding.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Paddinga2</anchor>
      <arglist>(const ::FLAC__StreamMetadata &amp;object)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Padding</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Padding.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Paddinga3</anchor>
      <arglist>(const ::FLAC__StreamMetadata *object)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Padding</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Padding.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Paddinga4</anchor>
      <arglist>(::FLAC__StreamMetadata *object, bool copy)</arglist>
    </member>
    <member kind="function">
      <type>Padding &amp;</type>
      <name>operator=</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Padding.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Paddinga6</anchor>
      <arglist>(const Padding &amp;object)</arglist>
    </member>
    <member kind="function">
      <type>Padding &amp;</type>
      <name>operator=</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Padding.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Paddinga7</anchor>
      <arglist>(const ::FLAC__StreamMetadata &amp;object)</arglist>
    </member>
    <member kind="function">
      <type>Padding &amp;</type>
      <name>operator=</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Padding.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Paddinga8</anchor>
      <arglist>(const ::FLAC__StreamMetadata *object)</arglist>
    </member>
    <member kind="function">
      <type>Padding &amp;</type>
      <name>assign</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Padding.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Paddinga9</anchor>
      <arglist>(::FLAC__StreamMetadata *object, bool copy)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator==</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Padding.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Paddinga10</anchor>
      <arglist>(const Padding &amp;object) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator==</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Padding.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Paddinga11</anchor>
      <arglist>(const ::FLAC__StreamMetadata &amp;object) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator==</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Padding.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Paddinga12</anchor>
      <arglist>(const ::FLAC__StreamMetadata *object) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator!=</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Padding.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Paddinga13</anchor>
      <arglist>(const Padding &amp;object) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator!=</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Padding.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Paddinga14</anchor>
      <arglist>(const ::FLAC__StreamMetadata &amp;object) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator!=</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Padding.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Paddinga15</anchor>
      <arglist>(const ::FLAC__StreamMetadata *object) const </arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>set_length</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Padding.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Paddinga16</anchor>
      <arglist>(unsigned length)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator==</name>
      <anchorfile>group__flacpp__metadata__object.html</anchorfile>
      <anchor>ga1</anchor>
      <arglist>(const Prototype &amp;) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator!=</name>
      <anchorfile>group__flacpp__metadata__object.html</anchorfile>
      <anchor>ga4</anchor>
      <arglist>(const Prototype &amp;) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>is_valid</name>
      <anchorfile>group__flacpp__metadata__object.html</anchorfile>
      <anchor>ga7</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>get_is_last</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommenta27</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>::FLAC__MetadataType</type>
      <name>get_type</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommenta28</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>unsigned</type>
      <name>get_length</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommenta29</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>set_is_last</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommenta30</anchor>
      <arglist>(bool)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>operator const ::FLAC__StreamMetadata *</name>
      <anchorfile>group__flacpp__metadata__object.html</anchorfile>
      <anchor>ga8</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" protection="protected">
      <type>Prototype &amp;</type>
      <name>assign_object</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommentb0</anchor>
      <arglist>(::FLAC__StreamMetadata *object, bool copy)</arglist>
    </member>
    <member kind="function" protection="protected" virtualness="virtual">
      <type>virtual void</type>
      <name>clear</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommentb1</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>::FLAC__StreamMetadata *</type>
      <name>object_</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommentp0</anchor>
      <arglist></arglist>
    </member>
    <member kind="friend">
      <type>friend class</type>
      <name>SimpleIterator</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommentn0</anchor>
      <arglist></arglist>
    </member>
    <member kind="friend">
      <type>friend class</type>
      <name>Iterator</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommentn1</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>FLAC::Metadata::Application</name>
    <filename>classFLAC_1_1Metadata_1_1Application.html</filename>
    <base>FLAC::Metadata::Prototype</base>
    <member kind="function">
      <type></type>
      <name>Application</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Application.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Applicationa1</anchor>
      <arglist>(const Application &amp;object)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Application</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Application.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Applicationa2</anchor>
      <arglist>(const ::FLAC__StreamMetadata &amp;object)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Application</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Application.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Applicationa3</anchor>
      <arglist>(const ::FLAC__StreamMetadata *object)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Application</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Application.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Applicationa4</anchor>
      <arglist>(::FLAC__StreamMetadata *object, bool copy)</arglist>
    </member>
    <member kind="function">
      <type>Application &amp;</type>
      <name>operator=</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Application.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Applicationa6</anchor>
      <arglist>(const Application &amp;object)</arglist>
    </member>
    <member kind="function">
      <type>Application &amp;</type>
      <name>operator=</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Application.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Applicationa7</anchor>
      <arglist>(const ::FLAC__StreamMetadata &amp;object)</arglist>
    </member>
    <member kind="function">
      <type>Application &amp;</type>
      <name>operator=</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Application.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Applicationa8</anchor>
      <arglist>(const ::FLAC__StreamMetadata *object)</arglist>
    </member>
    <member kind="function">
      <type>Application &amp;</type>
      <name>assign</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Application.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Applicationa9</anchor>
      <arglist>(::FLAC__StreamMetadata *object, bool copy)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator==</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Application.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Applicationa10</anchor>
      <arglist>(const Application &amp;object) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator==</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Application.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Applicationa11</anchor>
      <arglist>(const ::FLAC__StreamMetadata &amp;object) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator==</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Application.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Applicationa12</anchor>
      <arglist>(const ::FLAC__StreamMetadata *object) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator!=</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Application.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Applicationa13</anchor>
      <arglist>(const Application &amp;object) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator!=</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Application.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Applicationa14</anchor>
      <arglist>(const ::FLAC__StreamMetadata &amp;object) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator!=</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Application.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Applicationa15</anchor>
      <arglist>(const ::FLAC__StreamMetadata *object) const </arglist>
    </member>
    <member kind="function">
      <type>const FLAC__byte *</type>
      <name>get_id</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Application.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Applicationa16</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>const FLAC__byte *</type>
      <name>get_data</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Application.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Applicationa17</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>set_id</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Application.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Applicationa18</anchor>
      <arglist>(const FLAC__byte value[4])</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>set_data</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Application.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Applicationa19</anchor>
      <arglist>(const FLAC__byte *data, unsigned length)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>set_data</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Application.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Applicationa20</anchor>
      <arglist>(FLAC__byte *data, unsigned length, bool copy)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator==</name>
      <anchorfile>group__flacpp__metadata__object.html</anchorfile>
      <anchor>ga1</anchor>
      <arglist>(const Prototype &amp;) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator!=</name>
      <anchorfile>group__flacpp__metadata__object.html</anchorfile>
      <anchor>ga4</anchor>
      <arglist>(const Prototype &amp;) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>is_valid</name>
      <anchorfile>group__flacpp__metadata__object.html</anchorfile>
      <anchor>ga7</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>get_is_last</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommenta27</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>::FLAC__MetadataType</type>
      <name>get_type</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommenta28</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>unsigned</type>
      <name>get_length</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommenta29</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>set_is_last</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommenta30</anchor>
      <arglist>(bool)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>operator const ::FLAC__StreamMetadata *</name>
      <anchorfile>group__flacpp__metadata__object.html</anchorfile>
      <anchor>ga8</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" protection="protected">
      <type>Prototype &amp;</type>
      <name>assign_object</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommentb0</anchor>
      <arglist>(::FLAC__StreamMetadata *object, bool copy)</arglist>
    </member>
    <member kind="function" protection="protected" virtualness="virtual">
      <type>virtual void</type>
      <name>clear</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommentb1</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>::FLAC__StreamMetadata *</type>
      <name>object_</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommentp0</anchor>
      <arglist></arglist>
    </member>
    <member kind="friend">
      <type>friend class</type>
      <name>SimpleIterator</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommentn0</anchor>
      <arglist></arglist>
    </member>
    <member kind="friend">
      <type>friend class</type>
      <name>Iterator</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommentn1</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>FLAC::Metadata::SeekTable</name>
    <filename>classFLAC_1_1Metadata_1_1SeekTable.html</filename>
    <base>FLAC::Metadata::Prototype</base>
    <member kind="function">
      <type></type>
      <name>SeekTable</name>
      <anchorfile>classFLAC_1_1Metadata_1_1SeekTable.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1SeekTablea1</anchor>
      <arglist>(const SeekTable &amp;object)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>SeekTable</name>
      <anchorfile>classFLAC_1_1Metadata_1_1SeekTable.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1SeekTablea2</anchor>
      <arglist>(const ::FLAC__StreamMetadata &amp;object)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>SeekTable</name>
      <anchorfile>classFLAC_1_1Metadata_1_1SeekTable.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1SeekTablea3</anchor>
      <arglist>(const ::FLAC__StreamMetadata *object)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>SeekTable</name>
      <anchorfile>classFLAC_1_1Metadata_1_1SeekTable.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1SeekTablea4</anchor>
      <arglist>(::FLAC__StreamMetadata *object, bool copy)</arglist>
    </member>
    <member kind="function">
      <type>SeekTable &amp;</type>
      <name>operator=</name>
      <anchorfile>classFLAC_1_1Metadata_1_1SeekTable.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1SeekTablea6</anchor>
      <arglist>(const SeekTable &amp;object)</arglist>
    </member>
    <member kind="function">
      <type>SeekTable &amp;</type>
      <name>operator=</name>
      <anchorfile>classFLAC_1_1Metadata_1_1SeekTable.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1SeekTablea7</anchor>
      <arglist>(const ::FLAC__StreamMetadata &amp;object)</arglist>
    </member>
    <member kind="function">
      <type>SeekTable &amp;</type>
      <name>operator=</name>
      <anchorfile>classFLAC_1_1Metadata_1_1SeekTable.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1SeekTablea8</anchor>
      <arglist>(const ::FLAC__StreamMetadata *object)</arglist>
    </member>
    <member kind="function">
      <type>SeekTable &amp;</type>
      <name>assign</name>
      <anchorfile>classFLAC_1_1Metadata_1_1SeekTable.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1SeekTablea9</anchor>
      <arglist>(::FLAC__StreamMetadata *object, bool copy)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator==</name>
      <anchorfile>classFLAC_1_1Metadata_1_1SeekTable.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1SeekTablea10</anchor>
      <arglist>(const SeekTable &amp;object) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator==</name>
      <anchorfile>classFLAC_1_1Metadata_1_1SeekTable.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1SeekTablea11</anchor>
      <arglist>(const ::FLAC__StreamMetadata &amp;object) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator==</name>
      <anchorfile>classFLAC_1_1Metadata_1_1SeekTable.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1SeekTablea12</anchor>
      <arglist>(const ::FLAC__StreamMetadata *object) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator!=</name>
      <anchorfile>classFLAC_1_1Metadata_1_1SeekTable.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1SeekTablea13</anchor>
      <arglist>(const SeekTable &amp;object) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator!=</name>
      <anchorfile>classFLAC_1_1Metadata_1_1SeekTable.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1SeekTablea14</anchor>
      <arglist>(const ::FLAC__StreamMetadata &amp;object) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator!=</name>
      <anchorfile>classFLAC_1_1Metadata_1_1SeekTable.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1SeekTablea15</anchor>
      <arglist>(const ::FLAC__StreamMetadata *object) const </arglist>
    </member>
    <member kind="function">
      <type>unsigned</type>
      <name>get_num_points</name>
      <anchorfile>classFLAC_1_1Metadata_1_1SeekTable.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1SeekTablea16</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>::FLAC__StreamMetadata_SeekPoint</type>
      <name>get_point</name>
      <anchorfile>classFLAC_1_1Metadata_1_1SeekTable.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1SeekTablea17</anchor>
      <arglist>(unsigned index) const </arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>set_point</name>
      <anchorfile>classFLAC_1_1Metadata_1_1SeekTable.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1SeekTablea18</anchor>
      <arglist>(unsigned index, const ::FLAC__StreamMetadata_SeekPoint &amp;point)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>insert_point</name>
      <anchorfile>classFLAC_1_1Metadata_1_1SeekTable.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1SeekTablea19</anchor>
      <arglist>(unsigned index, const ::FLAC__StreamMetadata_SeekPoint &amp;point)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>delete_point</name>
      <anchorfile>classFLAC_1_1Metadata_1_1SeekTable.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1SeekTablea20</anchor>
      <arglist>(unsigned index)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>is_legal</name>
      <anchorfile>classFLAC_1_1Metadata_1_1SeekTable.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1SeekTablea21</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator==</name>
      <anchorfile>group__flacpp__metadata__object.html</anchorfile>
      <anchor>ga1</anchor>
      <arglist>(const Prototype &amp;) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator!=</name>
      <anchorfile>group__flacpp__metadata__object.html</anchorfile>
      <anchor>ga4</anchor>
      <arglist>(const Prototype &amp;) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>is_valid</name>
      <anchorfile>group__flacpp__metadata__object.html</anchorfile>
      <anchor>ga7</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>get_is_last</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommenta27</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>::FLAC__MetadataType</type>
      <name>get_type</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommenta28</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>unsigned</type>
      <name>get_length</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommenta29</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>set_is_last</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommenta30</anchor>
      <arglist>(bool)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>operator const ::FLAC__StreamMetadata *</name>
      <anchorfile>group__flacpp__metadata__object.html</anchorfile>
      <anchor>ga8</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" protection="protected">
      <type>Prototype &amp;</type>
      <name>assign_object</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommentb0</anchor>
      <arglist>(::FLAC__StreamMetadata *object, bool copy)</arglist>
    </member>
    <member kind="function" protection="protected" virtualness="virtual">
      <type>virtual void</type>
      <name>clear</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommentb1</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>::FLAC__StreamMetadata *</type>
      <name>object_</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommentp0</anchor>
      <arglist></arglist>
    </member>
    <member kind="friend">
      <type>friend class</type>
      <name>SimpleIterator</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommentn0</anchor>
      <arglist></arglist>
    </member>
    <member kind="friend">
      <type>friend class</type>
      <name>Iterator</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommentn1</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>FLAC::Metadata::VorbisComment</name>
    <filename>classFLAC_1_1Metadata_1_1VorbisComment.html</filename>
    <base>FLAC::Metadata::Prototype</base>
    <member kind="function">
      <type></type>
      <name>VorbisComment</name>
      <anchorfile>classFLAC_1_1Metadata_1_1VorbisComment.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommenta1</anchor>
      <arglist>(const VorbisComment &amp;object)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>VorbisComment</name>
      <anchorfile>classFLAC_1_1Metadata_1_1VorbisComment.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommenta2</anchor>
      <arglist>(const ::FLAC__StreamMetadata &amp;object)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>VorbisComment</name>
      <anchorfile>classFLAC_1_1Metadata_1_1VorbisComment.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommenta3</anchor>
      <arglist>(const ::FLAC__StreamMetadata *object)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>VorbisComment</name>
      <anchorfile>classFLAC_1_1Metadata_1_1VorbisComment.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommenta4</anchor>
      <arglist>(::FLAC__StreamMetadata *object, bool copy)</arglist>
    </member>
    <member kind="function">
      <type>VorbisComment &amp;</type>
      <name>operator=</name>
      <anchorfile>classFLAC_1_1Metadata_1_1VorbisComment.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommenta6</anchor>
      <arglist>(const VorbisComment &amp;object)</arglist>
    </member>
    <member kind="function">
      <type>VorbisComment &amp;</type>
      <name>operator=</name>
      <anchorfile>classFLAC_1_1Metadata_1_1VorbisComment.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommenta7</anchor>
      <arglist>(const ::FLAC__StreamMetadata &amp;object)</arglist>
    </member>
    <member kind="function">
      <type>VorbisComment &amp;</type>
      <name>operator=</name>
      <anchorfile>classFLAC_1_1Metadata_1_1VorbisComment.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommenta8</anchor>
      <arglist>(const ::FLAC__StreamMetadata *object)</arglist>
    </member>
    <member kind="function">
      <type>VorbisComment &amp;</type>
      <name>assign</name>
      <anchorfile>classFLAC_1_1Metadata_1_1VorbisComment.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommenta9</anchor>
      <arglist>(::FLAC__StreamMetadata *object, bool copy)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator==</name>
      <anchorfile>classFLAC_1_1Metadata_1_1VorbisComment.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommenta10</anchor>
      <arglist>(const VorbisComment &amp;object) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator==</name>
      <anchorfile>classFLAC_1_1Metadata_1_1VorbisComment.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommenta11</anchor>
      <arglist>(const ::FLAC__StreamMetadata &amp;object) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator==</name>
      <anchorfile>classFLAC_1_1Metadata_1_1VorbisComment.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommenta12</anchor>
      <arglist>(const ::FLAC__StreamMetadata *object) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator!=</name>
      <anchorfile>classFLAC_1_1Metadata_1_1VorbisComment.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommenta13</anchor>
      <arglist>(const VorbisComment &amp;object) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator!=</name>
      <anchorfile>classFLAC_1_1Metadata_1_1VorbisComment.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommenta14</anchor>
      <arglist>(const ::FLAC__StreamMetadata &amp;object) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator!=</name>
      <anchorfile>classFLAC_1_1Metadata_1_1VorbisComment.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommenta15</anchor>
      <arglist>(const ::FLAC__StreamMetadata *object) const </arglist>
    </member>
    <member kind="function">
      <type>unsigned</type>
      <name>get_num_comments</name>
      <anchorfile>classFLAC_1_1Metadata_1_1VorbisComment.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommenta16</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>const FLAC__byte *</type>
      <name>get_vendor_string</name>
      <anchorfile>classFLAC_1_1Metadata_1_1VorbisComment.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommenta17</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>Entry</type>
      <name>get_comment</name>
      <anchorfile>classFLAC_1_1Metadata_1_1VorbisComment.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommenta18</anchor>
      <arglist>(unsigned index) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>set_vendor_string</name>
      <anchorfile>classFLAC_1_1Metadata_1_1VorbisComment.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommenta19</anchor>
      <arglist>(const FLAC__byte *string)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>set_comment</name>
      <anchorfile>classFLAC_1_1Metadata_1_1VorbisComment.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommenta20</anchor>
      <arglist>(unsigned index, const Entry &amp;entry)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>insert_comment</name>
      <anchorfile>classFLAC_1_1Metadata_1_1VorbisComment.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommenta21</anchor>
      <arglist>(unsigned index, const Entry &amp;entry)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>append_comment</name>
      <anchorfile>classFLAC_1_1Metadata_1_1VorbisComment.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommenta22</anchor>
      <arglist>(const Entry &amp;entry)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>delete_comment</name>
      <anchorfile>classFLAC_1_1Metadata_1_1VorbisComment.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommenta23</anchor>
      <arglist>(unsigned index)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator==</name>
      <anchorfile>group__flacpp__metadata__object.html</anchorfile>
      <anchor>ga1</anchor>
      <arglist>(const Prototype &amp;) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator!=</name>
      <anchorfile>group__flacpp__metadata__object.html</anchorfile>
      <anchor>ga4</anchor>
      <arglist>(const Prototype &amp;) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>is_valid</name>
      <anchorfile>group__flacpp__metadata__object.html</anchorfile>
      <anchor>ga7</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>get_is_last</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommenta27</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>::FLAC__MetadataType</type>
      <name>get_type</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommenta28</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>unsigned</type>
      <name>get_length</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommenta29</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>set_is_last</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommenta30</anchor>
      <arglist>(bool)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>operator const ::FLAC__StreamMetadata *</name>
      <anchorfile>group__flacpp__metadata__object.html</anchorfile>
      <anchor>ga8</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" protection="protected">
      <type>Prototype &amp;</type>
      <name>assign_object</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommentb0</anchor>
      <arglist>(::FLAC__StreamMetadata *object, bool copy)</arglist>
    </member>
    <member kind="function" protection="protected" virtualness="virtual">
      <type>virtual void</type>
      <name>clear</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommentb1</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>::FLAC__StreamMetadata *</type>
      <name>object_</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommentp0</anchor>
      <arglist></arglist>
    </member>
    <member kind="friend">
      <type>friend class</type>
      <name>SimpleIterator</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommentn0</anchor>
      <arglist></arglist>
    </member>
    <member kind="friend">
      <type>friend class</type>
      <name>Iterator</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommentn1</anchor>
      <arglist></arglist>
    </member>
    <class kind="class">FLAC::Metadata::VorbisComment::Entry</class>
  </compound>
  <compound kind="class">
    <name>FLAC::Metadata::VorbisComment::Entry</name>
    <filename>classFLAC_1_1Metadata_1_1VorbisComment_1_1Entry.html</filename>
    <member kind="function">
      <type></type>
      <name>Entry</name>
      <anchorfile>classFLAC_1_1Metadata_1_1VorbisComment_1_1Entry.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisComment_1_1Entrya1</anchor>
      <arglist>(const char *field, unsigned field_length)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Entry</name>
      <anchorfile>classFLAC_1_1Metadata_1_1VorbisComment_1_1Entry.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisComment_1_1Entrya2</anchor>
      <arglist>(const char *field)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Entry</name>
      <anchorfile>classFLAC_1_1Metadata_1_1VorbisComment_1_1Entry.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisComment_1_1Entrya3</anchor>
      <arglist>(const char *field_name, const char *field_value, unsigned field_value_length)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Entry</name>
      <anchorfile>classFLAC_1_1Metadata_1_1VorbisComment_1_1Entry.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisComment_1_1Entrya4</anchor>
      <arglist>(const char *field_name, const char *field_value)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Entry</name>
      <anchorfile>classFLAC_1_1Metadata_1_1VorbisComment_1_1Entry.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisComment_1_1Entrya5</anchor>
      <arglist>(const Entry &amp;entry)</arglist>
    </member>
    <member kind="function">
      <type>Entry &amp;</type>
      <name>operator=</name>
      <anchorfile>classFLAC_1_1Metadata_1_1VorbisComment_1_1Entry.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisComment_1_1Entrya6</anchor>
      <arglist>(const Entry &amp;entry)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>is_valid</name>
      <anchorfile>classFLAC_1_1Metadata_1_1VorbisComment_1_1Entry.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisComment_1_1Entrya8</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>unsigned</type>
      <name>get_field_length</name>
      <anchorfile>classFLAC_1_1Metadata_1_1VorbisComment_1_1Entry.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisComment_1_1Entrya9</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>unsigned</type>
      <name>get_field_name_length</name>
      <anchorfile>classFLAC_1_1Metadata_1_1VorbisComment_1_1Entry.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisComment_1_1Entrya10</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>unsigned</type>
      <name>get_field_value_length</name>
      <anchorfile>classFLAC_1_1Metadata_1_1VorbisComment_1_1Entry.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisComment_1_1Entrya11</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>::FLAC__StreamMetadata_VorbisComment_Entry</type>
      <name>get_entry</name>
      <anchorfile>classFLAC_1_1Metadata_1_1VorbisComment_1_1Entry.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisComment_1_1Entrya12</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>const char *</type>
      <name>get_field</name>
      <anchorfile>classFLAC_1_1Metadata_1_1VorbisComment_1_1Entry.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisComment_1_1Entrya13</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>const char *</type>
      <name>get_field_name</name>
      <anchorfile>classFLAC_1_1Metadata_1_1VorbisComment_1_1Entry.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisComment_1_1Entrya14</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>const char *</type>
      <name>get_field_value</name>
      <anchorfile>classFLAC_1_1Metadata_1_1VorbisComment_1_1Entry.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisComment_1_1Entrya15</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>set_field</name>
      <anchorfile>classFLAC_1_1Metadata_1_1VorbisComment_1_1Entry.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisComment_1_1Entrya16</anchor>
      <arglist>(const char *field, unsigned field_length)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>set_field</name>
      <anchorfile>classFLAC_1_1Metadata_1_1VorbisComment_1_1Entry.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisComment_1_1Entrya17</anchor>
      <arglist>(const char *field)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>set_field_name</name>
      <anchorfile>classFLAC_1_1Metadata_1_1VorbisComment_1_1Entry.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisComment_1_1Entrya18</anchor>
      <arglist>(const char *field_name)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>set_field_value</name>
      <anchorfile>classFLAC_1_1Metadata_1_1VorbisComment_1_1Entry.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisComment_1_1Entrya19</anchor>
      <arglist>(const char *field_value, unsigned field_value_length)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>set_field_value</name>
      <anchorfile>classFLAC_1_1Metadata_1_1VorbisComment_1_1Entry.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisComment_1_1Entrya20</anchor>
      <arglist>(const char *field_value)</arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>bool</type>
      <name>is_valid_</name>
      <anchorfile>classFLAC_1_1Metadata_1_1VorbisComment_1_1Entry.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisComment_1_1Entryp0</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>::FLAC__StreamMetadata_VorbisComment_Entry</type>
      <name>entry_</name>
      <anchorfile>classFLAC_1_1Metadata_1_1VorbisComment_1_1Entry.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisComment_1_1Entryp1</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>char *</type>
      <name>field_name_</name>
      <anchorfile>classFLAC_1_1Metadata_1_1VorbisComment_1_1Entry.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisComment_1_1Entryp2</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>unsigned</type>
      <name>field_name_length_</name>
      <anchorfile>classFLAC_1_1Metadata_1_1VorbisComment_1_1Entry.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisComment_1_1Entryp3</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>char *</type>
      <name>field_value_</name>
      <anchorfile>classFLAC_1_1Metadata_1_1VorbisComment_1_1Entry.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisComment_1_1Entryp4</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>unsigned</type>
      <name>field_value_length_</name>
      <anchorfile>classFLAC_1_1Metadata_1_1VorbisComment_1_1Entry.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisComment_1_1Entryp5</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>FLAC::Metadata::CueSheet</name>
    <filename>classFLAC_1_1Metadata_1_1CueSheet.html</filename>
    <base>FLAC::Metadata::Prototype</base>
    <member kind="function">
      <type></type>
      <name>CueSheet</name>
      <anchorfile>classFLAC_1_1Metadata_1_1CueSheet.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1CueSheeta1</anchor>
      <arglist>(const CueSheet &amp;object)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>CueSheet</name>
      <anchorfile>classFLAC_1_1Metadata_1_1CueSheet.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1CueSheeta2</anchor>
      <arglist>(const ::FLAC__StreamMetadata &amp;object)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>CueSheet</name>
      <anchorfile>classFLAC_1_1Metadata_1_1CueSheet.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1CueSheeta3</anchor>
      <arglist>(const ::FLAC__StreamMetadata *object)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>CueSheet</name>
      <anchorfile>classFLAC_1_1Metadata_1_1CueSheet.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1CueSheeta4</anchor>
      <arglist>(::FLAC__StreamMetadata *object, bool copy)</arglist>
    </member>
    <member kind="function">
      <type>CueSheet &amp;</type>
      <name>operator=</name>
      <anchorfile>classFLAC_1_1Metadata_1_1CueSheet.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1CueSheeta6</anchor>
      <arglist>(const CueSheet &amp;object)</arglist>
    </member>
    <member kind="function">
      <type>CueSheet &amp;</type>
      <name>operator=</name>
      <anchorfile>classFLAC_1_1Metadata_1_1CueSheet.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1CueSheeta7</anchor>
      <arglist>(const ::FLAC__StreamMetadata &amp;object)</arglist>
    </member>
    <member kind="function">
      <type>CueSheet &amp;</type>
      <name>operator=</name>
      <anchorfile>classFLAC_1_1Metadata_1_1CueSheet.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1CueSheeta8</anchor>
      <arglist>(const ::FLAC__StreamMetadata *object)</arglist>
    </member>
    <member kind="function">
      <type>CueSheet &amp;</type>
      <name>assign</name>
      <anchorfile>classFLAC_1_1Metadata_1_1CueSheet.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1CueSheeta9</anchor>
      <arglist>(::FLAC__StreamMetadata *object, bool copy)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator==</name>
      <anchorfile>classFLAC_1_1Metadata_1_1CueSheet.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1CueSheeta10</anchor>
      <arglist>(const CueSheet &amp;object) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator==</name>
      <anchorfile>classFLAC_1_1Metadata_1_1CueSheet.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1CueSheeta11</anchor>
      <arglist>(const ::FLAC__StreamMetadata &amp;object) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator==</name>
      <anchorfile>classFLAC_1_1Metadata_1_1CueSheet.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1CueSheeta12</anchor>
      <arglist>(const ::FLAC__StreamMetadata *object) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator!=</name>
      <anchorfile>classFLAC_1_1Metadata_1_1CueSheet.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1CueSheeta13</anchor>
      <arglist>(const CueSheet &amp;object) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator!=</name>
      <anchorfile>classFLAC_1_1Metadata_1_1CueSheet.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1CueSheeta14</anchor>
      <arglist>(const ::FLAC__StreamMetadata &amp;object) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator!=</name>
      <anchorfile>classFLAC_1_1Metadata_1_1CueSheet.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1CueSheeta15</anchor>
      <arglist>(const ::FLAC__StreamMetadata *object) const </arglist>
    </member>
    <member kind="function">
      <type>const char *</type>
      <name>get_media_catalog_number</name>
      <anchorfile>classFLAC_1_1Metadata_1_1CueSheet.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1CueSheeta16</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>FLAC__uint64</type>
      <name>get_lead_in</name>
      <anchorfile>classFLAC_1_1Metadata_1_1CueSheet.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1CueSheeta17</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>get_is_cd</name>
      <anchorfile>classFLAC_1_1Metadata_1_1CueSheet.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1CueSheeta18</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>unsigned</type>
      <name>get_num_tracks</name>
      <anchorfile>classFLAC_1_1Metadata_1_1CueSheet.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1CueSheeta19</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>Track</type>
      <name>get_track</name>
      <anchorfile>classFLAC_1_1Metadata_1_1CueSheet.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1CueSheeta20</anchor>
      <arglist>(unsigned i) const </arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>set_media_catalog_number</name>
      <anchorfile>classFLAC_1_1Metadata_1_1CueSheet.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1CueSheeta21</anchor>
      <arglist>(const char value[128])</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>set_lead_in</name>
      <anchorfile>classFLAC_1_1Metadata_1_1CueSheet.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1CueSheeta22</anchor>
      <arglist>(FLAC__uint64 value)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>set_is_cd</name>
      <anchorfile>classFLAC_1_1Metadata_1_1CueSheet.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1CueSheeta23</anchor>
      <arglist>(bool value)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>set_index</name>
      <anchorfile>classFLAC_1_1Metadata_1_1CueSheet.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1CueSheeta24</anchor>
      <arglist>(unsigned track_num, unsigned index_num, const ::FLAC__StreamMetadata_CueSheet_Index &amp;index)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>insert_index</name>
      <anchorfile>classFLAC_1_1Metadata_1_1CueSheet.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1CueSheeta25</anchor>
      <arglist>(unsigned track_num, unsigned index_num, const ::FLAC__StreamMetadata_CueSheet_Index &amp;index)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>delete_index</name>
      <anchorfile>classFLAC_1_1Metadata_1_1CueSheet.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1CueSheeta26</anchor>
      <arglist>(unsigned track_num, unsigned index_num)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>set_track</name>
      <anchorfile>classFLAC_1_1Metadata_1_1CueSheet.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1CueSheeta27</anchor>
      <arglist>(unsigned i, const Track &amp;track)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>insert_track</name>
      <anchorfile>classFLAC_1_1Metadata_1_1CueSheet.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1CueSheeta28</anchor>
      <arglist>(unsigned i, const Track &amp;track)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>delete_track</name>
      <anchorfile>classFLAC_1_1Metadata_1_1CueSheet.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1CueSheeta29</anchor>
      <arglist>(unsigned i)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>is_legal</name>
      <anchorfile>classFLAC_1_1Metadata_1_1CueSheet.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1CueSheeta30</anchor>
      <arglist>(bool check_cd_da_subset=false, const char **violation=0) const </arglist>
    </member>
    <member kind="function">
      <type>FLAC__uint32</type>
      <name>calculate_cddb_id</name>
      <anchorfile>classFLAC_1_1Metadata_1_1CueSheet.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1CueSheeta31</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator==</name>
      <anchorfile>group__flacpp__metadata__object.html</anchorfile>
      <anchor>ga1</anchor>
      <arglist>(const Prototype &amp;) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator!=</name>
      <anchorfile>group__flacpp__metadata__object.html</anchorfile>
      <anchor>ga4</anchor>
      <arglist>(const Prototype &amp;) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>is_valid</name>
      <anchorfile>group__flacpp__metadata__object.html</anchorfile>
      <anchor>ga7</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>get_is_last</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommenta27</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>::FLAC__MetadataType</type>
      <name>get_type</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommenta28</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>unsigned</type>
      <name>get_length</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommenta29</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>set_is_last</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommenta30</anchor>
      <arglist>(bool)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>operator const ::FLAC__StreamMetadata *</name>
      <anchorfile>group__flacpp__metadata__object.html</anchorfile>
      <anchor>ga8</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" protection="protected">
      <type>Prototype &amp;</type>
      <name>assign_object</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommentb0</anchor>
      <arglist>(::FLAC__StreamMetadata *object, bool copy)</arglist>
    </member>
    <member kind="function" protection="protected" virtualness="virtual">
      <type>virtual void</type>
      <name>clear</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommentb1</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>::FLAC__StreamMetadata *</type>
      <name>object_</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommentp0</anchor>
      <arglist></arglist>
    </member>
    <member kind="friend">
      <type>friend class</type>
      <name>SimpleIterator</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommentn0</anchor>
      <arglist></arglist>
    </member>
    <member kind="friend">
      <type>friend class</type>
      <name>Iterator</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommentn1</anchor>
      <arglist></arglist>
    </member>
    <class kind="class">FLAC::Metadata::CueSheet::Track</class>
  </compound>
  <compound kind="class">
    <name>FLAC::Metadata::CueSheet::Track</name>
    <filename>classFLAC_1_1Metadata_1_1CueSheet_1_1Track.html</filename>
    <member kind="function">
      <type></type>
      <name>Track</name>
      <anchorfile>classFLAC_1_1Metadata_1_1CueSheet_1_1Track.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1CueSheet_1_1Tracka1</anchor>
      <arglist>(const ::FLAC__StreamMetadata_CueSheet_Track *track)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Track</name>
      <anchorfile>classFLAC_1_1Metadata_1_1CueSheet_1_1Track.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1CueSheet_1_1Tracka2</anchor>
      <arglist>(const Track &amp;track)</arglist>
    </member>
    <member kind="function">
      <type>Track &amp;</type>
      <name>operator=</name>
      <anchorfile>classFLAC_1_1Metadata_1_1CueSheet_1_1Track.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1CueSheet_1_1Tracka3</anchor>
      <arglist>(const Track &amp;track)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>is_valid</name>
      <anchorfile>classFLAC_1_1Metadata_1_1CueSheet_1_1Track.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1CueSheet_1_1Tracka5</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>FLAC__uint64</type>
      <name>get_offset</name>
      <anchorfile>classFLAC_1_1Metadata_1_1CueSheet_1_1Track.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1CueSheet_1_1Tracka6</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>FLAC__byte</type>
      <name>get_number</name>
      <anchorfile>classFLAC_1_1Metadata_1_1CueSheet_1_1Track.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1CueSheet_1_1Tracka7</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>const char *</type>
      <name>get_isrc</name>
      <anchorfile>classFLAC_1_1Metadata_1_1CueSheet_1_1Track.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1CueSheet_1_1Tracka8</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>unsigned</type>
      <name>get_type</name>
      <anchorfile>classFLAC_1_1Metadata_1_1CueSheet_1_1Track.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1CueSheet_1_1Tracka9</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>get_pre_emphasis</name>
      <anchorfile>classFLAC_1_1Metadata_1_1CueSheet_1_1Track.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1CueSheet_1_1Tracka10</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>FLAC__byte</type>
      <name>get_num_indices</name>
      <anchorfile>classFLAC_1_1Metadata_1_1CueSheet_1_1Track.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1CueSheet_1_1Tracka11</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>::FLAC__StreamMetadata_CueSheet_Index</type>
      <name>get_index</name>
      <anchorfile>classFLAC_1_1Metadata_1_1CueSheet_1_1Track.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1CueSheet_1_1Tracka12</anchor>
      <arglist>(unsigned i) const </arglist>
    </member>
    <member kind="function">
      <type>const ::FLAC__StreamMetadata_CueSheet_Track *</type>
      <name>get_track</name>
      <anchorfile>classFLAC_1_1Metadata_1_1CueSheet_1_1Track.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1CueSheet_1_1Tracka13</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>set_offset</name>
      <anchorfile>classFLAC_1_1Metadata_1_1CueSheet_1_1Track.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1CueSheet_1_1Tracka14</anchor>
      <arglist>(FLAC__uint64 value)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>set_number</name>
      <anchorfile>classFLAC_1_1Metadata_1_1CueSheet_1_1Track.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1CueSheet_1_1Tracka15</anchor>
      <arglist>(FLAC__byte value)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>set_isrc</name>
      <anchorfile>classFLAC_1_1Metadata_1_1CueSheet_1_1Track.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1CueSheet_1_1Tracka16</anchor>
      <arglist>(const char value[12])</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>set_type</name>
      <anchorfile>classFLAC_1_1Metadata_1_1CueSheet_1_1Track.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1CueSheet_1_1Tracka17</anchor>
      <arglist>(unsigned value)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>set_pre_emphasis</name>
      <anchorfile>classFLAC_1_1Metadata_1_1CueSheet_1_1Track.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1CueSheet_1_1Tracka18</anchor>
      <arglist>(bool value)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>set_index</name>
      <anchorfile>classFLAC_1_1Metadata_1_1CueSheet_1_1Track.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1CueSheet_1_1Tracka19</anchor>
      <arglist>(unsigned i, const ::FLAC__StreamMetadata_CueSheet_Index &amp;index)</arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>::FLAC__StreamMetadata_CueSheet_Track *</type>
      <name>object_</name>
      <anchorfile>classFLAC_1_1Metadata_1_1CueSheet_1_1Track.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1CueSheet_1_1Trackp0</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>FLAC::Metadata::Picture</name>
    <filename>classFLAC_1_1Metadata_1_1Picture.html</filename>
    <base>FLAC::Metadata::Prototype</base>
    <member kind="function">
      <type></type>
      <name>Picture</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Picture.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Picturea1</anchor>
      <arglist>(const Picture &amp;object)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Picture</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Picture.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Picturea2</anchor>
      <arglist>(const ::FLAC__StreamMetadata &amp;object)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Picture</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Picture.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Picturea3</anchor>
      <arglist>(const ::FLAC__StreamMetadata *object)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Picture</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Picture.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Picturea4</anchor>
      <arglist>(::FLAC__StreamMetadata *object, bool copy)</arglist>
    </member>
    <member kind="function">
      <type>Picture &amp;</type>
      <name>operator=</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Picture.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Picturea6</anchor>
      <arglist>(const Picture &amp;object)</arglist>
    </member>
    <member kind="function">
      <type>Picture &amp;</type>
      <name>operator=</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Picture.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Picturea7</anchor>
      <arglist>(const ::FLAC__StreamMetadata &amp;object)</arglist>
    </member>
    <member kind="function">
      <type>Picture &amp;</type>
      <name>operator=</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Picture.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Picturea8</anchor>
      <arglist>(const ::FLAC__StreamMetadata *object)</arglist>
    </member>
    <member kind="function">
      <type>Picture &amp;</type>
      <name>assign</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Picture.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Picturea9</anchor>
      <arglist>(::FLAC__StreamMetadata *object, bool copy)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator==</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Picture.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Picturea10</anchor>
      <arglist>(const Picture &amp;object) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator==</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Picture.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Picturea11</anchor>
      <arglist>(const ::FLAC__StreamMetadata &amp;object) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator==</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Picture.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Picturea12</anchor>
      <arglist>(const ::FLAC__StreamMetadata *object) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator!=</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Picture.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Picturea13</anchor>
      <arglist>(const Picture &amp;object) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator!=</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Picture.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Picturea14</anchor>
      <arglist>(const ::FLAC__StreamMetadata &amp;object) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator!=</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Picture.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Picturea15</anchor>
      <arglist>(const ::FLAC__StreamMetadata *object) const </arglist>
    </member>
    <member kind="function">
      <type>::FLAC__StreamMetadata_Picture_Type</type>
      <name>get_type</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Picture.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Picturea16</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>const char *</type>
      <name>get_mime_type</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Picture.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Picturea17</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>const FLAC__byte *</type>
      <name>get_description</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Picture.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Picturea18</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>FLAC__uint32</type>
      <name>get_width</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Picture.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Picturea19</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>FLAC__uint32</type>
      <name>get_height</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Picture.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Picturea20</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>FLAC__uint32</type>
      <name>get_depth</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Picture.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Picturea21</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>FLAC__uint32</type>
      <name>get_colors</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Picture.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Picturea22</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>FLAC__uint32</type>
      <name>get_data_length</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Picture.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Picturea23</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>const FLAC__byte *</type>
      <name>get_data</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Picture.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Picturea24</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>set_type</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Picture.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Picturea25</anchor>
      <arglist>(::FLAC__StreamMetadata_Picture_Type type)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>set_mime_type</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Picture.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Picturea26</anchor>
      <arglist>(const char *string)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>set_description</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Picture.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Picturea27</anchor>
      <arglist>(const FLAC__byte *string)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>set_width</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Picture.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Picturea28</anchor>
      <arglist>(FLAC__uint32 value) const </arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>set_height</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Picture.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Picturea29</anchor>
      <arglist>(FLAC__uint32 value) const </arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>set_depth</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Picture.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Picturea30</anchor>
      <arglist>(FLAC__uint32 value) const </arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>set_colors</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Picture.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Picturea31</anchor>
      <arglist>(FLAC__uint32 value) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>set_data</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Picture.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Picturea32</anchor>
      <arglist>(const FLAC__byte *data, FLAC__uint32 data_length)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator==</name>
      <anchorfile>group__flacpp__metadata__object.html</anchorfile>
      <anchor>ga1</anchor>
      <arglist>(const Prototype &amp;) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator!=</name>
      <anchorfile>group__flacpp__metadata__object.html</anchorfile>
      <anchor>ga4</anchor>
      <arglist>(const Prototype &amp;) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>is_valid</name>
      <anchorfile>group__flacpp__metadata__object.html</anchorfile>
      <anchor>ga7</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>get_is_last</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommenta27</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>unsigned</type>
      <name>get_length</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommenta29</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>set_is_last</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommenta30</anchor>
      <arglist>(bool)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>operator const ::FLAC__StreamMetadata *</name>
      <anchorfile>group__flacpp__metadata__object.html</anchorfile>
      <anchor>ga8</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" protection="protected">
      <type>Prototype &amp;</type>
      <name>assign_object</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommentb0</anchor>
      <arglist>(::FLAC__StreamMetadata *object, bool copy)</arglist>
    </member>
    <member kind="function" protection="protected" virtualness="virtual">
      <type>virtual void</type>
      <name>clear</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommentb1</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>::FLAC__StreamMetadata *</type>
      <name>object_</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommentp0</anchor>
      <arglist></arglist>
    </member>
    <member kind="friend">
      <type>friend class</type>
      <name>SimpleIterator</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommentn0</anchor>
      <arglist></arglist>
    </member>
    <member kind="friend">
      <type>friend class</type>
      <name>Iterator</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommentn1</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>FLAC::Metadata::Unknown</name>
    <filename>classFLAC_1_1Metadata_1_1Unknown.html</filename>
    <base>FLAC::Metadata::Prototype</base>
    <member kind="function">
      <type></type>
      <name>Unknown</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Unknown.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Unknowna1</anchor>
      <arglist>(const Unknown &amp;object)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Unknown</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Unknown.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Unknowna2</anchor>
      <arglist>(const ::FLAC__StreamMetadata &amp;object)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Unknown</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Unknown.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Unknowna3</anchor>
      <arglist>(const ::FLAC__StreamMetadata *object)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Unknown</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Unknown.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Unknowna4</anchor>
      <arglist>(::FLAC__StreamMetadata *object, bool copy)</arglist>
    </member>
    <member kind="function">
      <type>Unknown &amp;</type>
      <name>operator=</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Unknown.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Unknowna6</anchor>
      <arglist>(const Unknown &amp;object)</arglist>
    </member>
    <member kind="function">
      <type>Unknown &amp;</type>
      <name>operator=</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Unknown.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Unknowna7</anchor>
      <arglist>(const ::FLAC__StreamMetadata &amp;object)</arglist>
    </member>
    <member kind="function">
      <type>Unknown &amp;</type>
      <name>operator=</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Unknown.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Unknowna8</anchor>
      <arglist>(const ::FLAC__StreamMetadata *object)</arglist>
    </member>
    <member kind="function">
      <type>Unknown &amp;</type>
      <name>assign</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Unknown.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Unknowna9</anchor>
      <arglist>(::FLAC__StreamMetadata *object, bool copy)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator==</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Unknown.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Unknowna10</anchor>
      <arglist>(const Unknown &amp;object) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator==</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Unknown.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Unknowna11</anchor>
      <arglist>(const ::FLAC__StreamMetadata &amp;object) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator==</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Unknown.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Unknowna12</anchor>
      <arglist>(const ::FLAC__StreamMetadata *object) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator!=</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Unknown.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Unknowna13</anchor>
      <arglist>(const Unknown &amp;object) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator!=</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Unknown.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Unknowna14</anchor>
      <arglist>(const ::FLAC__StreamMetadata &amp;object) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator!=</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Unknown.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Unknowna15</anchor>
      <arglist>(const ::FLAC__StreamMetadata *object) const </arglist>
    </member>
    <member kind="function">
      <type>const FLAC__byte *</type>
      <name>get_data</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Unknown.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Unknowna16</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>set_data</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Unknown.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Unknowna17</anchor>
      <arglist>(const FLAC__byte *data, unsigned length)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>set_data</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Unknown.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Unknowna18</anchor>
      <arglist>(FLAC__byte *data, unsigned length, bool copy)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator==</name>
      <anchorfile>group__flacpp__metadata__object.html</anchorfile>
      <anchor>ga1</anchor>
      <arglist>(const Prototype &amp;) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator!=</name>
      <anchorfile>group__flacpp__metadata__object.html</anchorfile>
      <anchor>ga4</anchor>
      <arglist>(const Prototype &amp;) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>is_valid</name>
      <anchorfile>group__flacpp__metadata__object.html</anchorfile>
      <anchor>ga7</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>get_is_last</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommenta27</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>::FLAC__MetadataType</type>
      <name>get_type</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommenta28</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>unsigned</type>
      <name>get_length</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommenta29</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>set_is_last</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommenta30</anchor>
      <arglist>(bool)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>operator const ::FLAC__StreamMetadata *</name>
      <anchorfile>group__flacpp__metadata__object.html</anchorfile>
      <anchor>ga8</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" protection="protected">
      <type>Prototype &amp;</type>
      <name>assign_object</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommentb0</anchor>
      <arglist>(::FLAC__StreamMetadata *object, bool copy)</arglist>
    </member>
    <member kind="function" protection="protected" virtualness="virtual">
      <type>virtual void</type>
      <name>clear</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommentb1</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>::FLAC__StreamMetadata *</type>
      <name>object_</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommentp0</anchor>
      <arglist></arglist>
    </member>
    <member kind="friend">
      <type>friend class</type>
      <name>SimpleIterator</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommentn0</anchor>
      <arglist></arglist>
    </member>
    <member kind="friend">
      <type>friend class</type>
      <name>Iterator</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Prototype.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1VorbisCommentn1</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>FLAC::Metadata::SimpleIterator</name>
    <filename>classFLAC_1_1Metadata_1_1SimpleIterator.html</filename>
    <member kind="function">
      <type>bool</type>
      <name>is_valid</name>
      <anchorfile>classFLAC_1_1Metadata_1_1SimpleIterator.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1SimpleIteratora2</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>init</name>
      <anchorfile>classFLAC_1_1Metadata_1_1SimpleIterator.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1SimpleIteratora3</anchor>
      <arglist>(const char *filename, bool read_only, bool preserve_file_stats)</arglist>
    </member>
    <member kind="function">
      <type>Status</type>
      <name>status</name>
      <anchorfile>classFLAC_1_1Metadata_1_1SimpleIterator.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1SimpleIteratora4</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>is_writable</name>
      <anchorfile>classFLAC_1_1Metadata_1_1SimpleIterator.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1SimpleIteratora5</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>next</name>
      <anchorfile>classFLAC_1_1Metadata_1_1SimpleIterator.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1SimpleIteratora6</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>prev</name>
      <anchorfile>classFLAC_1_1Metadata_1_1SimpleIterator.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1SimpleIteratora7</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>is_last</name>
      <anchorfile>classFLAC_1_1Metadata_1_1SimpleIterator.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1SimpleIteratora8</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>off_t</type>
      <name>get_block_offset</name>
      <anchorfile>classFLAC_1_1Metadata_1_1SimpleIterator.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1SimpleIteratora9</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>::FLAC__MetadataType</type>
      <name>get_block_type</name>
      <anchorfile>classFLAC_1_1Metadata_1_1SimpleIterator.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1SimpleIteratora10</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>unsigned</type>
      <name>get_block_length</name>
      <anchorfile>classFLAC_1_1Metadata_1_1SimpleIterator.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1SimpleIteratora11</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>get_application_id</name>
      <anchorfile>classFLAC_1_1Metadata_1_1SimpleIterator.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1SimpleIteratora12</anchor>
      <arglist>(FLAC__byte *id)</arglist>
    </member>
    <member kind="function">
      <type>Prototype *</type>
      <name>get_block</name>
      <anchorfile>classFLAC_1_1Metadata_1_1SimpleIterator.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1SimpleIteratora13</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>set_block</name>
      <anchorfile>classFLAC_1_1Metadata_1_1SimpleIterator.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1SimpleIteratora14</anchor>
      <arglist>(Prototype *block, bool use_padding=true)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>insert_block_after</name>
      <anchorfile>classFLAC_1_1Metadata_1_1SimpleIterator.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1SimpleIteratora15</anchor>
      <arglist>(Prototype *block, bool use_padding=true)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>delete_block</name>
      <anchorfile>classFLAC_1_1Metadata_1_1SimpleIterator.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1SimpleIteratora16</anchor>
      <arglist>(bool use_padding=true)</arglist>
    </member>
    <member kind="function" protection="protected">
      <type>void</type>
      <name>clear</name>
      <anchorfile>classFLAC_1_1Metadata_1_1SimpleIterator.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1SimpleIteratorb0</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>::FLAC__Metadata_SimpleIterator *</type>
      <name>iterator_</name>
      <anchorfile>classFLAC_1_1Metadata_1_1SimpleIterator.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1SimpleIteratorp0</anchor>
      <arglist></arglist>
    </member>
    <class kind="class">FLAC::Metadata::SimpleIterator::Status</class>
  </compound>
  <compound kind="class">
    <name>FLAC::Metadata::SimpleIterator::Status</name>
    <filename>classFLAC_1_1Metadata_1_1SimpleIterator_1_1Status.html</filename>
    <member kind="function">
      <type></type>
      <name>Status</name>
      <anchorfile>classFLAC_1_1Metadata_1_1SimpleIterator_1_1Status.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1SimpleIterator_1_1Statusa0</anchor>
      <arglist>(::FLAC__Metadata_SimpleIteratorStatus status)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>operator::FLAC__Metadata_SimpleIteratorStatus</name>
      <anchorfile>classFLAC_1_1Metadata_1_1SimpleIterator_1_1Status.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1SimpleIterator_1_1Statusa1</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>const char *</type>
      <name>as_cstring</name>
      <anchorfile>classFLAC_1_1Metadata_1_1SimpleIterator_1_1Status.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1SimpleIterator_1_1Statusa2</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>::FLAC__Metadata_SimpleIteratorStatus</type>
      <name>status_</name>
      <anchorfile>classFLAC_1_1Metadata_1_1SimpleIterator_1_1Status.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1SimpleIterator_1_1Statusp0</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>FLAC::Metadata::Chain</name>
    <filename>classFLAC_1_1Metadata_1_1Chain.html</filename>
    <member kind="function">
      <type>bool</type>
      <name>is_valid</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Chain.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Chaina2</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>Status</type>
      <name>status</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Chain.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Chaina3</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>read</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Chain.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Chaina4</anchor>
      <arglist>(const char *filename, bool is_ogg=false)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>read</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Chain.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Chaina5</anchor>
      <arglist>(FLAC__IOHandle handle, FLAC__IOCallbacks callbacks, bool is_ogg=false)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>check_if_tempfile_needed</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Chain.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Chaina6</anchor>
      <arglist>(bool use_padding)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>write</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Chain.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Chaina7</anchor>
      <arglist>(bool use_padding=true, bool preserve_file_stats=false)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>write</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Chain.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Chaina8</anchor>
      <arglist>(bool use_padding,::FLAC__IOHandle handle,::FLAC__IOCallbacks callbacks)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>write</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Chain.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Chaina9</anchor>
      <arglist>(bool use_padding,::FLAC__IOHandle handle,::FLAC__IOCallbacks callbacks,::FLAC__IOHandle temp_handle,::FLAC__IOCallbacks temp_callbacks)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>merge_padding</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Chain.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Chaina10</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>sort_padding</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Chain.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Chaina11</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" protection="protected" virtualness="virtual">
      <type>virtual void</type>
      <name>clear</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Chain.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Chainb0</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>::FLAC__Metadata_Chain *</type>
      <name>chain_</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Chain.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Chainp0</anchor>
      <arglist></arglist>
    </member>
    <member kind="friend">
      <type>friend class</type>
      <name>Iterator</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Chain.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Chainn0</anchor>
      <arglist></arglist>
    </member>
    <class kind="class">FLAC::Metadata::Chain::Status</class>
  </compound>
  <compound kind="class">
    <name>FLAC::Metadata::Chain::Status</name>
    <filename>classFLAC_1_1Metadata_1_1Chain_1_1Status.html</filename>
    <member kind="function">
      <type></type>
      <name>Status</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Chain_1_1Status.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Chain_1_1Statusa0</anchor>
      <arglist>(::FLAC__Metadata_ChainStatus status)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>operator::FLAC__Metadata_ChainStatus</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Chain_1_1Status.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Chain_1_1Statusa1</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>const char *</type>
      <name>as_cstring</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Chain_1_1Status.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Chain_1_1Statusa2</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>::FLAC__Metadata_ChainStatus</type>
      <name>status_</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Chain_1_1Status.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Chain_1_1Statusp0</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>FLAC::Metadata::Iterator</name>
    <filename>classFLAC_1_1Metadata_1_1Iterator.html</filename>
    <member kind="function">
      <type>bool</type>
      <name>is_valid</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Iterator.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Iteratora2</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>init</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Iterator.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Iteratora3</anchor>
      <arglist>(Chain &amp;chain)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>next</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Iterator.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Iteratora4</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>prev</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Iterator.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Iteratora5</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>::FLAC__MetadataType</type>
      <name>get_block_type</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Iterator.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Iteratora6</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>Prototype *</type>
      <name>get_block</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Iterator.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Iteratora7</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>set_block</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Iterator.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Iteratora8</anchor>
      <arglist>(Prototype *block)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>delete_block</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Iterator.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Iteratora9</anchor>
      <arglist>(bool replace_with_padding)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>insert_block_before</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Iterator.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Iteratora10</anchor>
      <arglist>(Prototype *block)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>insert_block_after</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Iterator.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Iteratora11</anchor>
      <arglist>(Prototype *block)</arglist>
    </member>
    <member kind="function" protection="protected" virtualness="virtual">
      <type>virtual void</type>
      <name>clear</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Iterator.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Iteratorb0</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>::FLAC__Metadata_Iterator *</type>
      <name>iterator_</name>
      <anchorfile>classFLAC_1_1Metadata_1_1Iterator.html</anchorfile>
      <anchor>FLAC_1_1Metadata_1_1Iteratorp0</anchor>
      <arglist></arglist>
    </member>
  </compound>
</tagfile>
