/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#ifndef C_API_ADDONINSTANCE_INPUTSTREAM_STREAMCODEC_H
#define C_API_ADDONINSTANCE_INPUTSTREAM_STREAMCODEC_H

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  //==============================================================================
  /// @defgroup cpp_kodi_addon_inputstream_Defs_StreamEncryption_STREAMCODEC_PROFILE enum STREAMCODEC_PROFILE
  /// @ingroup cpp_kodi_addon_inputstream_Defs_StreamCodec
  /// @brief **The standard defines several sets of capabilities.**\n
  /// Which are referred to as profiles, targeting specific classes of applications.
  ///
  ///@{
  enum STREAMCODEC_PROFILE
  {
    /// @brief Unknown codec profile
    CodecProfileUnknown = 0,

    /// @brief If a codec profile is not required
    CodecProfileNotNeeded,

    /// @brief **H264** Baseline Profile (BP, 66)\n
    /// \n
    /// Primarily for low-cost applications that require additional data loss
    /// robustness, this profile is used in some videoconferencing and mobile
    /// applications. This profile includes all features that are supported
    /// in the Constrained Baseline Profile, plus three additional features
    /// that can be used for loss robustness (or for other purposes such as
    /// low-delay multi-point video stream compositing). The importance of
    /// this profile has faded somewhat since the definition of the Constrained
    /// Baseline Profile in 2009. All Constrained Baseline Profile bitstreams
    /// are also considered to be Baseline Profile bitstreams, as these two
    /// profiles share the same profile identifier code value.
    H264CodecProfileBaseline,

    /// @brief **H264** Main Profile (MP, 77)\n
    /// \n
    /// This profile is used for standard-definition digital TV broadcasts that
    /// use the MPEG-4 format as defined in the
    /// [DVB standard](http://www.etsi.org/deliver/etsi_ts/101100_101199/101154/01.09.01_60/ts_101154v010901p.pdf).
    /// It is not, however, used for high-definition television broadcasts, as the
    /// importance of this profile faded when the High Profile was developed
    /// in 2004 for that application.
    H264CodecProfileMain,

    /// @brief **H264** Extended Profile (XP, 88)\n
    /// \n
    /// Intended as the streaming video profile, this profile has relatively high
    /// compression capability and some extra tricks for robustness to data losses
    /// and server stream switching.
    H264CodecProfileExtended,

    /// @brief **H264** High Profile (HiP, 100)\n
    /// \n
    /// The primary profile for broadcast and disc storage applications,
    /// particularly for high-definition television applications (for example,
    /// this is the profile adopted by the [Blu-ray Disc](https://en.wikipedia.org/wiki/Blu-ray_Disc)
    /// storage format and the [DVB](https://en.wikipedia.org/wiki/Digital_Video_Broadcasting)
    /// HDTV broadcast service).
    H264CodecProfileHigh,

    /// @brief **H264** High 10 Profile (Hi10P, 110)\n
    /// \n
    /// Going beyond typical mainstream consumer product capabilities, this
    /// profile builds on top of the High Profile, adding support for up to 10
    /// bits per sample of decoded picture precision.
    H264CodecProfileHigh10,

    /// @brief **H264** High 4:2:2 Profile (Hi422P, 122)\n
    /// \n
    /// Primarily targeting professional applications that use interlaced video,
    /// this profile builds on top of the High 10 Profile, adding support for the
    /// 4:2:2 chroma sampling format while using up to 10 bits per sample of
    /// decoded picture precision.
    H264CodecProfileHigh422,

    /// @brief **H264** High 4:4:4 Predictive Profile (Hi444PP, 244)\n
    /// \n
    /// This profile builds on top of the High 4:2:2 Profile, supporting up to
    /// 4:4:4 chroma sampling, up to 14 bits per sample, and additionally
    /// supporting efficient lossless region coding and the coding of each
    /// picture as three separate color planes.
    H264CodecProfileHigh444Predictive,

    /// @brief **VP9** profile 0\n
    /// \n
    /// There are several variants of the VP9 format (known as "coding profiles"),
    /// which successively allow more features; profile 0 is the basic variant,
    /// requiring the least from a hardware implementation.
    ///
    /// [Color depth](https://en.wikipedia.org/wiki/Color_depth): 8 bit/sample,
    /// [chroma subsampling](https://en.wikipedia.org/wiki/Chroma_subsampling): 4:2:0
    VP9CodecProfile0 = 20,

    /// @brief **VP9** profile 1\n
    /// \n
    /// [Color depth](https://en.wikipedia.org/wiki/Color_depth): 8 bit,
    /// [chroma subsampling](https://en.wikipedia.org/wiki/Chroma_subsampling): 4:2:0, 4:2:2, 4:4:4
    VP9CodecProfile1,

    /// @brief **VP9** profile 2\n
    /// \n
    /// [Color depth](https://en.wikipedia.org/wiki/Color_depth): 10–12 bit,
    /// [chroma subsampling](https://en.wikipedia.org/wiki/Chroma_subsampling): 4:2:0
    VP9CodecProfile2,

    /// @brief **VP9** profile 3\n
    /// \n
    /// [Color depth](https://en.wikipedia.org/wiki/Color_depth): 10–12 bit,
    /// [chroma subsampling](https://en.wikipedia.org/wiki/Chroma_subsampling): 4:2:0, 4:2:2, 4:4:4,
    /// see [VP9 Bitstream & Decoding Process Specification](https://storage.googleapis.com/downloads.webmproject.org/docs/vp9/vp9-bitstream-specification-v0.6-20160331-draft.pdf)
    VP9CodecProfile3,

    /// @brief **AV1** Main profile\n
    /// \n
    /// [Color depth](https://en.wikipedia.org/wiki/Color_depth): 8–10 bit,
    /// [chroma subsampling](https://en.wikipedia.org/wiki/Chroma_subsampling): 4:2:0
    /// see [AV1 specification](https://aomedia.org/av1/specification/)
    AV1CodecProfileMain,

    /// @brief **AV1** High profile\n
    /// \n
    /// [Color depth](https://en.wikipedia.org/wiki/Color_depth): 8–10 bit,
    /// [chroma subsampling](https://en.wikipedia.org/wiki/Chroma_subsampling): 4:2:0, 4:4:4
    /// see [AV1 specification](https://aomedia.org/av1/specification/)
    AV1CodecProfileHigh,

    /// @brief **AV1** Professional profile\n
    /// \n
    /// [Color depth](https://en.wikipedia.org/wiki/Color_depth): 8–12 bit,
    /// [chroma subsampling](https://en.wikipedia.org/wiki/Chroma_subsampling): 4:2:0, 4:2:2, 4:4:4
    /// see [AV1 specification](https://aomedia.org/av1/specification/)
    AV1CodecProfileProfessional,

    // Audio codec profiles

    /// @brief **AAC** Main Profile
    AACCodecProfileMAIN,

    /// @brief **AAC** Low Profile
    AACCodecProfileLOW,

    /// @brief **AAC** SSR Profile
    AACCodecProfileSSR,

    /// @brief **AAC** LTP Profile
    AACCodecProfileLTP,

    /// @brief **High Efficiency AAC** Profile
    AACCodecProfileHE,

    /// @brief **High Efficiency AAC** v2 Profile
    AACCodecProfileHEV2,

    /// @brief **AAC** LD Profile
    AACCodecProfileLD,

    /// @brief **AAC** ELD Profile
    AACCodecProfileELD,

    /// @brief **MPEG2 AAC** LOW Profile
    MPEG2AACCodecProfileLOW,

    /// @brief **MPEG2 High Efficiency AAC** Profile
    MPEG2AACCodecProfileHE,

    /// @brief **DTS** Profile
    DTSCodecProfile,

    /// @brief **DTS** Extended Surround Profile
    DTSCodecProfileES,

    /// @brief **DTS** 96/24 (96kHz / 24bit) Profile
    DTSCodecProfile9624,

    /// @brief **DTS** High-Definition High Resolution Audio Profile
    DTSCodecProfileHDHRA,

    /// @brief **DTS** High-Definition Master Audio Profile
    DTSCodecProfileHDMA,

    /// @brief **DTS** High-Definition Express Profile
    DTSCodecProfileHDExpress,

    /// @brief **DTS** High-Definition Master Audio + DTS:X Profile
    DTSCodecProfileHDMAX,

    /// @brief **DTS** High-Definition Master Audio + DTS:X IMAX Profile
    DTSCodecProfileHDMAIMAX,

    /// @brief **Dolby Digital** Atmos Profile
    DDPlusCodecProfileAtmos,
  };
  ///@}
  //------------------------------------------------------------------------------

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* !C_API_ADDONINSTANCE_INPUTSTREAM_STREAMCODEC_H */
