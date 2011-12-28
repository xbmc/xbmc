#ifndef _FFDSHOW_CONSTANTS_
#define _FFDSHOW_CONSTANTS_

#define FFDSHOW_NAME_L     L"ffdshow Video Decoder"
#define FFDSHOWRAW_NAME_L  L"ffdshow raw video filter"
#define FFDSHOWVFW_NAME_L  L"ffdshow VFW decoder helper"
#define FFDSHOWSUBTITLES_NAME_L  L"ffdshow subtitles filter"
#define FFDSHOWDXVA_NAME_L  L"ffdshow DXVA Video Decoder"

#define IDFF_autoPreset            1  //automatic preset loading enabled
#define IDFF_trayIcon              3  //is tray icon visible
#define IDFF_trayIconType       3381
#define IDFF_trayIconChanged    3382
#define IDFF_trayIconExt          69  //show extended tray icon tip
//#define IDFF_trayHwnd             34
//#define IDFF_cfgDlgHwnd            4  //handle of configuration dialog
#define IDFF_autoPresetFileFirst   5  //try to load preset from file
#define IDFF_autoLoadedFromFile    6
#define IDFF_filterMode            7
 #define IDFF_FILTERMODE_PLAYER        1
 #define IDFF_FILTERMODE_CONFIG        2
 #define IDFF_FILTERMODE_PROC          4
 #define IDFF_FILTERMODE_VFW           8
 #define IDFF_FILTERMODE_VIDEO       256
 #define IDFF_FILTERMODE_VIDEORAW    512
 #define IDFF_FILTERMODE_AUDIO      1024
 #define IDFF_FILTERMODE_ENC        2048
 #define IDFF_FILTERMODE_AUDIORAW   4096
 #define IDFF_FILTERMODE_VIDEOSUBTITLES  8192
 #define IDFF_FILTERMODE_VIDEODXVA  16384
#define IDFF_currentFrame         14
#define IDFF_decodingFps          15
#define IDFF_buildHistogram       16
//#define IDFF_AVIfourcc            18
//#define IDFF_AVIaspectX           27
//#define IDFF_AVIaspectY           28
#define IDFF_AVIcolorspace        96
//#define IDFF_subFlnmChanged       20
#define IDFF_workaroundBugs       25
#define IDFF_errorConcealment     70
#define IDFF_errorRecognition      71
#define IDFF_fastMpeg2            90
#define IDFF_fastH264             91
//#define IDFF_autodetect24P       3490
#define IDFF_libtheoraPostproc    92
//#define IDFF_subCurrentFlnm       29
//#define IDFF_lastFrameLength      41
#define IDFF_movieDuration        38
#define IDFF_installPath          35
#define IDFF_availableCpuFlags    77 // CPU flags supported by CPU
#define IDFF_allowedCpuFlags      78 // CPU flags allowed by user
#define IDFF_cpuFlags             36 // IDFF_availableCpuFlags&IDFF_allowedCpuFlags
#define IDFF_notreg               47
#define IDFF_multipleInstances    66 // 0 - allow multiple instances of ffdshow in graph to be connected to each other, 1 - only check previous filter, 2 - check all filters in graph
#define IDFF_isBlacklist          93
#define IDFF_isWhitelist         3372
#define IDFF_compManagerMode     3383
#define IDFF_isCompMgr           3384
#define IDFF_isCompMgrChanged    3385
#define IDFF_allowDPRINTF        3488
#define IDFF_allowDPRINTFchanged 3489
#define IDFF_blacklist            94
#define IDFF_whitelist           3371
//#define IDFF_xvidInited           68
#define IDFF_defaultMerit         72
#define IDFF_subCurLang           73
#define IDFF_addToROT             74
#define IDFF_streamsOptionsMenu   75
#define IDFF_dvdproc              76
//#define IDFF_ac3drc               79
//#define IDFF_ac3SPDIF             3386
//#define IDFF_dtsdrc               80
#define IDFF_supDVDdec            81
//#define IDFF_neroavc              82
//#define IDFF_unicodeOS            87
#define IDFF_numLAVCdecThreads    97
#define IDFF_multiThreadDec       98
#define IDFF_dontQueueInWMP       99
#define IDFF_dropOnDelay         124
#define IDFF_dropOnDelayTime     125
#define IDFF_h264skipOnDelay     126
#define IDFF_h264skipOnDelayTime 127
#define IDFF_useQueueOnlyIn      128
#define IDFF_useQueueOnlyInList  129
#define IDFF_QueueCount          130 // typo
#define IDFF_queueCount          130
#define IDFF_queueVMR9YV12       3484

#define IDFF_outputdebug     43
#define IDFF_outputdebugfile 44
#define IDFF_debugfile       45
#define IDFF_errorbox        46

#define IDFF_dlgRestorePos         9
#define IDFF_dlgPosX              10
#define IDFF_dlgPosY              11
#define IDFF_lvWidth0             12
#define IDFF_showHints            13
#define IDFF_lastPage              2  //last active page in configuration dialog
#define IDFF_defaultPreset        19
#define IDFF_lvKeysWidth0         21
#define IDFF_lvKeysWidth1         22
#define IDFF_lang                 32
#define IDFF_alwaysOnTop          67  // was 35 - conflicted with IDFF_installPath
#define IDFF_applying             37
#define IDFF_lvCodecsWidth0       39
#define IDFF_lvCodecsWidth1       40
#define IDFF_lvCodecsWidth2       48
#define IDFF_lvCodecsSelected     42
#define IDFF_dlgCustColor0        50
#define IDFF_dlgCustColor1        51
#define IDFF_dlgCustColor2        52
#define IDFF_dlgCustColor3        53
#define IDFF_dlgCustColor4        54
#define IDFF_dlgCustColor5        55
#define IDFF_dlgCustColor6        56
#define IDFF_dlgCustColor7        57
#define IDFF_dlgCustColor8        58
#define IDFF_dlgCustColor9        59
#define IDFF_dlgCustColor10       60
#define IDFF_dlgCustColor11       61
#define IDFF_dlgCustColor12       62
#define IDFF_dlgCustColor13       63
#define IDFF_dlgCustColor14       64
#define IDFF_dlgCustColor15       65
#define IDFF_lvConvolverWidth0    83
#define IDFF_lvConvolverWidth1    84
#define IDFF_lvConvolverWidth2    85
#define IDFF_lvConvolverSelected  86
#define IDFF_dlgDecCurrentPage    88
#define IDFF_dlgEncCurrentPage    89
#define IDFF_dlgVolumeDb          95

#define IDFF_presetAutoloadSize          1902 //should preset be autoloaded depending on movie dimensions
#define IDFF_presetAutoloadSizeXmin      1903
#define IDFF_presetAutoloadSizeXmax      1904
#define IDFF_presetAutoloadSizeCond      1905 //0 - and, 1 - or
#define IDFF_presetAutoloadSizeYmin      1906
#define IDFF_presetAutoloadSizeYmax      1907
#define IDFF_presetAutoloadLogic         3485

#define IDFF_presetAutoloadScreenSize          3517 //should preset be autoloaded depending on movie dimensions
#define IDFF_presetAutoloadScreenSizeXmin      3518
#define IDFF_presetAutoloadScreenSizeXmax      3519
#define IDFF_presetAutoloadScreenSizeCond      3520 //0 - and, 1 - or
#define IDFF_presetAutoloadScreenSizeYmin      3521
#define IDFF_presetAutoloadScreenSizeYmax      3522

/*
#define IDFF_presetAutoloadFlnm          1901 //should preset be autoloaded depending on filename
#define IDFF_presetAutoloadExt           1908 //should preset be autoloaded depending on file ext
#define IDFF_presetAutoloadExts          1909 //extensions
#define IDFF_presetAutoloadExe           1910
#define IDFF_presetAutoloadExes          1911
#define IDFF_presetAutoloadVolumeName    1912
#define IDFF_presetAutoloadVolumeNames   1913
#define IDFF_presetAutoloadVolumeSerial  1914
#define IDFF_presetAutoloadVolumeSerials 1915
*/

#define IDFF_filterPostproc         100
#define IDFF_isPostproc             106
#define IDFF_showPostproc           120
#define IDFF_orderPostproc          109
#define IDFF_fullPostproc           111
#define IDFF_halfPostproc           121
#define IDFF_postprocMethod         114 //0 - mplayer, 1 - nic, 2 - mplayer+nic, 3 - none, 4 - spp
#define IDFF_postprocMethodNicFirst 117
#define IDFF_ppqual                 101 //postprocessing quality set by user (active when not autoq)
#define IDFF_autoq                  102 //is automatic postprocessing control enabled?
#define IDFF_ppIsCustom             103 //custom postprocessing settings are enabled
#define IDFF_ppcustom               104 //custom postprocessing settings
#define IDFF_currentq               105
#define IDFF_deblockMplayerAccurate 123
#define IDFF_deblockStrength        110
#define IDFF_levelFixLum            107
//#define IDFF_levelFixChrom          108
#define IDFF_fullYrange             112
#define IDFF_postprocNicXthresh     115
#define IDFF_postprocNicYthresh     116
#define IDFF_postprocSPPmode        119

#define IDFF_filterDeinterlace         1400
#define IDFF_isDeinterlace             1401
#define IDFF_showDeinterlace           1418
#define IDFF_orderDeinterlace          1424
#define IDFF_fullDeinterlace           1402
#define IDFF_deinterlaceAlways         3493
#define IDFF_swapFields                1409
#define IDFF_deinterlaceMethod         1403
#define IDFF_tomocompSE                1407
#define IDFF_tomocompVF                1414
#define IDFF_dscalerDIflnm             1412
#define IDFF_dscalerDIcfg              1413
#define IDFF_frameRateDoublerThreshold 1416
#define IDFF_frameRateDoublerSE        1417
#define IDFF_kernelDeintThreshold      1420
#define IDFF_kernelDeintSharp          1421
#define IDFF_kernelDeintTwoway         1422
#define IDFF_kernelDeintMap            1423
#define IDFF_kernelDeintLinked         1428
#define IDFF_dgbobMode                 1425
#define IDFF_dgbobThreshold            1426
#define IDFF_dgbobAP                   1427
#define IDFF_yadifMode                 3494
#define IDFF_yadifFieldOrder           3495

#define IDFF_isDecimate            1410
#define IDFF_decimateRatio         1411

#define IDFF_filterPictProp       200
#define IDFF_isPictProp           205
#define IDFF_showPictProp         217
#define IDFF_orderPictProp        207
#define IDFF_fullPictProp         213
#define IDFF_halfPictProp         218
#define IDFF_lumGain              201  //luminance gain
#define IDFF_lumOffset            202  //luminance offset
#define IDFF_gammaCorrection      206  //gamma correction (*100)
#define IDFF_gammaCorrectionR     214  //red gamma correction (*100)
#define IDFF_gammaCorrectionG     215  //green gamma correction (*100)
#define IDFF_gammaCorrectionB     216  //blue gamma correction (*100)
#define IDFF_hue                  203  //hue
#define IDFF_saturation           204  //saturation
#define IDFF_colorizeStrength     219
#define IDFF_colorizeColor        220
#define IDFF_colorizeChromaonly   221
#define IDFF_pictPropLevelFix     222
#define IDFF_pictPropLevelFixFull 223
#define IDFF_scanlineEffect       224

#define IDFF_filterLevels       1600
#define IDFF_isLevels           1601
#define IDFF_showLevels         1611
#define IDFF_orderLevels        1602
#define IDFF_fullLevels         1603
#define IDFF_halfLevels         1612
#define IDFF_levelsMode         1613 // 0 - classic, 1 - Ylevels, 2 - YlevelsG, 3 - YlevelsS, 4 - YlevelsC
#define IDFF_levelsInMin        1604
#define IDFF_levelsGamma        1605
#define IDFF_levelsInMax        1606
#define IDFF_levelsOutMin       1607
#define IDFF_levelsOutMax       1608
#define IDFF_levelsOnlyLuma     1609
#define IDFF_levelsFullY        1610
#define IDFF_levelsYmaxDelta    1637
#define IDFF_levelsYthreshold   1638
#define IDFF_levelsYtemporal    1639
#define IDFF_levelsPosterize    1614
#define IDFF_levelsInAuto       1615
#define IDFF_levelsNumPoints    1616
#define IDFF_levelsPoint0x      1617
#define IDFF_levelsPoint0y      1618
#define IDFF_levelsPoint1x      1619
#define IDFF_levelsPoint1y      1620
#define IDFF_levelsPoint2x      1621
#define IDFF_levelsPoint2y      1622
#define IDFF_levelsPoint3x      1623
#define IDFF_levelsPoint3y      1624
#define IDFF_levelsPoint4x      1625
#define IDFF_levelsPoint4y      1626
#define IDFF_levelsPoint5x      1627
#define IDFF_levelsPoint5y      1628
#define IDFF_levelsPoint6x      1629
#define IDFF_levelsPoint6y      1630
#define IDFF_levelsPoint7x      1631
#define IDFF_levelsPoint7y      1632
#define IDFF_levelsPoint8x      1633
#define IDFF_levelsPoint8y      1634
#define IDFF_levelsPoint9x      1635
#define IDFF_levelsPoint9y      1636

#define IDFF_flip                301 //should output video be flipped?
#define IDFF_idct                601 //IDCT function user (0 - libavcodec simple 16383, 1 - libavcodec normal, 2 - reference, 3 - skal's)
#define IDFF_softTelecine       3511
#define IDFF_videoDelay         1801 //video delay in ms
#define IDFF_isVideoDelayEnd    1802
#define IDFF_videoDelayEnd      1803
#define IDFF_hwOverlayAspect    1351 //0 - VIDEOINFOHEADER, 1 - VIDEOINFOHEADER2, 2 - VIDEOINFOHEADER2, then VIDEOINFOHEADER
#define IDFF_grayscale           602 //only grayscale decoding - faster

#define IDFF_filterSharpen         400
#define IDFF_isSharpen             401 //is xshapen filter active?
#define IDFF_showSharpen           427
#define IDFF_orderSharpen          407
#define IDFF_fullSharpen           408
#define IDFF_halfSharpen           428
#define IDFF_sharpenMethod         406 //0 - xsharpen, 1 - unsharp, 2 - msharpen, 4 - asharp, 5 - mplayer
#define IDFF_xsharp_strength       402 //xsharpen filter strength
#define IDFF_xsharp_threshold      403 //xsharpen filter threshold
#define IDFF_unsharp_strength      404 //unsharp filter strength
#define IDFF_unsharp_threshold     405 //unsharp filter threshold
#define IDFF_msharp_strength       413
#define IDFF_msharp_threshold      414
#define IDFF_msharpHQ              415
#define IDFF_msharpMask            416
#define IDFF_asharpT               423
#define IDFF_asharpD               424
#define IDFF_asharpB               425
#define IDFF_asharpHQBF            426
#define IDFF_mplayerSharpLuma      440
#define IDFF_mplayerSharpChroma    441

#define IDFF_filterWarpsharp       430
#define IDFF_isWarpsharp           431
#define IDFF_showWarpsharp         442
#define IDFF_orderWarpsharp        432
#define IDFF_fullWarpsharp         433
#define IDFF_halfWarpsharp         443
#define IDFF_warpsharpMethod       434 //0 - warpsharp, 1 - aWarpSharp
#define IDFF_warpsharpDepth        419
#define IDFF_warpsharpThreshold    420
#define IDFF_awarpsharpDepth       435
#define IDFF_awarpsharpThresh      436
#define IDFF_awarpsharpBlur        437
#define IDFF_awarpsharpCM          438 //0 - none, 1 - downsampled, 2 - independant
#define IDFF_awarpsharpBM          439 //0 - hq 3-pass, 1 - fast 3-pass, 2 - fast 1-pass

#define IDFF_filterDCT    450
#define IDFF_isDCT        451
#define IDFF_showDCT      462
#define IDFF_orderDCT     452
#define IDFF_fullDCT      453
#define IDFF_halfDCT      463
#define IDFF_dctMode      464
#define IDFF_dct0         454
#define IDFF_dct1         455
#define IDFF_dct2         456
#define IDFF_dct3         457
#define IDFF_dct4         458
#define IDFF_dct5         459
#define IDFF_dct6         460
#define IDFF_dct7         461
#define IDFF_dctQuant     465
#define IDFF_dctMatrix0   466
#define IDFF_dctMatrix1   467
#define IDFF_dctMatrix2   468
#define IDFF_dctMatrix3   469
#define IDFF_dctMatrix4   470
#define IDFF_dctMatrix5   471
#define IDFF_dctMatrix6   472
#define IDFF_dctMatrix7   473
#define IDFF_dctMatrix8   474
#define IDFF_dctMatrix9   475
#define IDFF_dctMatrix10  476
#define IDFF_dctMatrix11  477
#define IDFF_dctMatrix12  478
#define IDFF_dctMatrix13  479
#define IDFF_dctMatrix14  480
#define IDFF_dctMatrix15  481

#define IDFF_filterNoise                500
#define IDFF_isNoise                    501 //is noising filter active?
#define IDFF_showNoise                  512
#define IDFF_orderNoise                 506
#define IDFF_fullNoise                  507
#define IDFF_halfNoise                  513
#define IDFF_noiseMethod                505 //0 - my noise, 1 - avih noise, 2 - mplayer
#define IDFF_uniformNoise               502 //is uniform noise active (applies only to luma noise now)?
#define IDFF_noisePattern               510
#define IDFF_noiseAveraged              511
#define IDFF_noiseStrength              503 //luma noise strength
#define IDFF_noiseStrengthChroma        504 //chroma noise strength
#define IDFF_noiseFlickerA              514
#define IDFF_noiseFlickerF              515
#define IDFF_noiseShakeA                516
#define IDFF_noiseShakeF                517
#define IDFF_noiseLinesA                518
#define IDFF_noiseLinesF                519
#define IDFF_noiseLinesC                524
#define IDFF_noiseLinesTransparency     520
#define IDFF_noiseScratchesA            521
#define IDFF_noiseScratchesF            522
#define IDFF_noiseScratchesC            525
#define IDFF_noiseScratchesTransparency 523

#define IDFF_filterResize        700
#define IDFF_showResize          751
#define IDFF_isResize            701 //is resizing active (or will be resizing active)
#define IDFF_orderResize         722
#define IDFF_fullResize          723
#define IDFF_resizeMode          728 //0 - exact size, 1 - aspect ratio , 2 - multiply of , 3 - multiply
#define IDFF_resizeDx            702 //new width
#define IDFF_resizeSpecifyHorizontalSizeOnly 703
#define IDFF_is_resizeDy_0       703 //deprecated. same as IDFF_resizeSpecifyHorizontalSizeOnly
#define IDFF_resizeDy            705 //new height
#define IDFF_resizeDy_real       766
#define IDFF_resizeMultOf        764
#define IDFF_resizeA1            729
#define IDFF_resizeA2            730
#define IDFF_resizeMult1000      753
#define IDFF_resizeSARinternally 3380
#define IDFF_resizeOutDeviceA1   3408
#define IDFF_resizeOutDeviceA2   3409
#define IDFF_resizeIf            733 //0 - always, 1 - size, 2 - number of pixels
#define IDFF_resizeIfXcond       734 //-1 - less, 1 - more
#define IDFF_resizeIfXval        735 //width to be compared to
#define IDFF_resizeIfYcond       736 //-1 - less, 1 - more
#define IDFF_resizeIfYval        737 //height to be compared to
#define IDFF_resizeIfXYcond      738 //0 - and, 1 - or
#define IDFF_resizeIfPixCond     739 //-1 - less, 1 - more
#define IDFF_resizeIfPixVal      740
#define IDFF_bordersInside       755
#define IDFF_bordersUnits        756 //0 - percent, 1 - pixels
#define IDFF_bordersLocked       743
#define IDFF_bordersPercentX     741
#define IDFF_bordersX IDFF_bordersPercentX
#define IDFF_bordersPercentY     742
#define IDFF_bordersY IDFF_bordersPercentY
#define IDFF_bordersPixelsX      757
#define IDFF_bordersPixelsY      758
#define IDFF_bordersDivX         3404
#define IDFF_bordersDivY         3405
#define IDFF_bordersBrightness   3487

#define IDFF_isAspect            704 //0 - no aspect ratio correctio, 1 - keep original aspect, 2 - aspect ratio is set in IDFF_aspectRatio
#define IDFF_aspectRatio         707 //aspect ratio (<<16)

#define IDFF_resizeMethodLuma          706
#define IDFF_resizeMethodChroma        759
#define IDFF_resizeMethodsLocked       763
#define IDFF_resizeInterlaced          748 // 0 - progressive, 1 - interlaced, 2 - use picture type information
#define IDFF_resizeAccurateRounding    731
#define IDFF_resizeBicubicParamSet     3503
#define IDFF_resizeBicubicBurring      3504
#define IDFF_resizeBicubicRinging      3505
#define IDFF_resizeBicubicLumaParam    724
#define IDFF_resizeBicubicChromaParam  760
#define IDFF_resizeGaussLumaParam      726
#define IDFF_resizeGaussChromaParam    761
#define IDFF_resizeLanczosLumaParam    727
#define IDFF_resizeLanczosChromaParam  762
#define IDFF_resizeGblurLum            708 // *100
#define IDFF_resizeGblurChrom          709 // *100
#define IDFF_resizeSharpenLum         710 // *100
#define IDFF_resizeSharpenChrom       711 // *100
#define IDFF_resizeSharpenLum2         3506 // *100
#define IDFF_resizeSharpenChrom2       3507 // *100
#define IDFF_resizeSimpleWarpXparam    749 // simple warped resize X param *1000
#define IDFF_resizeSimpleWarpYparam    750 // simple warped resize Y param *1000

#define IDFF_filterCropNzoom     747
#define IDFF_isCropNzoom         712
#define IDFF_showCropNzoom       752
#define IDFF_orderCropNzoom      754
#define IDFF_fullCropNzoom       765
#define IDFF_cropNzoomMode       713 //0 - zoom, 1 - crop, 2 - pan&scan, 3 - autocrop vert, 4 - auto crop horiz, 5 - autocrop vert & horiz
#define IDFF_magnificationX      714
#define IDFF_cropLeft            715
#define IDFF_cropRight           716
#define IDFF_cropTop             717
#define IDFF_cropBottom          718
#define IDFF_magnificationY      720
#define IDFF_magnificationLocked 721
#define IDFF_panscanZoom         744 //0 - orig size, 100 max zoom
#define IDFF_panscanX            745 //-100..100 - center x position
#define IDFF_panscanY            746 //-100..100 - center y position
#define IDFF_cropTolerance       3417 // Autocrop tolerance
#define IDFF_cropRefreshDelay    3418 // Autocrop refresh interval
#define IDFF_cropStopScan        3419 // Autocrop stop scanning after x ms
#define IDFF_autoCropTop         3420
#define IDFF_autoCropBottom      3421
#define IDFF_autoCropLeft        3422
#define IDFF_autoCropRight       3423

#define IDFF_filterSubtitles         800
#define IDFF_isSubtitles             801
#define IDFF_showSubtitles           828
#define IDFF_orderSubtitles          815
#define IDFF_fullSubtitles           817
#define IDFF_subFilename             821
#define IDFF_subTempFilename        3402
#define IDFF_subPosX                 810
#define IDFF_subPosY                 811
#define IDFF_subAlign                827 //0 - old ffdshow mode, 1 - left, 2 - center, 3 - right
#define IDFF_subExpand               825 //0 - don't expand, 1 - 4:3, 2 - 16:9, xxxxyyyy - custom
#define IDFF_subIsExpand             858
#define IDFF_subDelay                812
#define IDFF_subSpeed                813
#define IDFF_subSpeed2               830
#define IDFF_subAutoFlnm             814
#define IDFF_subSearchDir            822
#define IDFF_subSearchExt            862
#define IDFF_subSearchHeuristic      856
#define IDFF_subWatch                826
#define IDFF_subStereoscopic         833
#define IDFF_subStereoscopicPar      834 // stereoscopic parallax <-10%,10%> of picture width
#define IDFF_subDefLang              836
#define IDFF_subDefLang2             852
#define IDFF_subVobsub               835
#define IDFF_subVobsubAA             837
#define IDFF_subVobsubAAswgauss      851
#define IDFF_subVobsubChangePosition 849
#define IDFF_subVobsubScale          850
#define IDFF_subLinespacing          838
//#define IDFF_subTimeOverlap          839
#define IDFF_subIsMinDuration        840
#define IDFF_subMinDurationType      841 //0 - subtitle, 1 - line, 2 - character
#define IDFF_subMinDurationSub       842
#define IDFF_subMinDurationLine      843
#define IDFF_subMinDurationChar      844
#define IDFF_subTextpin              845
#define IDFF_subTextpinSSA           861
#define IDFF_subShowEmbedded         857 //id of displayed embedded subtitle, 0 if none
//#define IDFF_subFoundEmbedded        859
#define IDFF_subFix                  846
#define IDFF_subFixLang              847
#define IDFF_subFixDict              848
#define IDFF_subOpacity              853
#define IDFF_subSplitBorder          855
#define IDFF_subCC                   860
#define IDFF_subWordWrap             3403
#define IDFF_subExtendedTags         3492 // Enable HTML tags in SSA subs and SSA tags in SRT subs
#define IDFF_subSSAOverridePlacement 3496
#define IDFF_subSSAMaintainInside    3497
#define IDFF_subSSAUseMovieDimensions 3498

#define IDFF_fontName                820
#define IDFF_fontCharset             802
#define IDFF_fontAutosize            823
#define IDFF_fontAutosizeVideoWindow 829
#define IDFF_fontSizeP               803
//#define IDFF_fontSize            803
#define IDFF_fontSizeA               824
#define IDFF_fontWeight              804
#define IDFF_fontOutlineWidth        3387
#define IDFF_fontOpaqueBox           3400
#define IDFF_fontSpacing             808
#define IDFF_fontColor               809
#define IDFF_fontOutlineColor        3391
#define IDFF_fontShadowColor         3392
#define IDFF_fontSplitting           831
#define IDFF_fontXscale              832 // *100, multiplier of character width
#define IDFF_fontYscale              3411
#define IDFF_fontOverrideScale       3414
#define IDFF_fontAspectAuto          3415
#define IDFF_fontFast                854
#define IDFF_fontShadowMode          3374 // 0 - Glowing, 1 - classic gradient, 2 - classic
#define IDFF_fontShadowSize          3375
#define IDFF_fontBodyAlpha           3389
#define IDFF_fontOutlineAlpha        3390
#define IDFF_fontShadowAlpha         3376
#define IDFF_fontBlur                3406

#define IDFF_filterBlur              900
#define IDFF_isBlur                  901
#define IDFF_showBlur                936
#define IDFF_orderBlur               903
#define IDFF_fullBlur                905
#define IDFF_halfBlur                937
#define IDFF_blurIsSoften            921
#define IDFF_blurStrength            902
#define IDFF_blurIsTempSmooth        922
#define IDFF_tempSmooth              904
#define IDFF_tempSmoothColor         912
#define IDFF_blurIsSmoothLuma        923
#define IDFF_smoothStrengthLuma      908
#define IDFF_blurIsSmoothChroma      926
#define IDFF_smoothStrengthChroma    909
#define IDFF_blurIsGradual           924
#define IDFF_gradualStrength         913
#define IDFF_blurIsMplayerTNR        925
#define IDFF_mplayerTNR1             915
#define IDFF_mplayerTNR2             916
#define IDFF_mplayerTNR3             917
#define IDFF_blurIsMplayer           927
#define IDFF_mplayerBlurRadius       928
#define IDFF_mplayerBlurLuma         929
#define IDFF_mplayerBlurChroma       930
#define IDFF_blurIsDenoise3d         931
#define IDFF_denoise3Dluma           932
#define IDFF_denoise3Dchroma         933
#define IDFF_denoise3Dtime           934
#define IDFF_denoise3Dhq             935

#define IDFF_filterOffset       1100
#define IDFF_isOffset           1101
#define IDFF_showOffset         1110
#define IDFF_orderOffset        1102
#define IDFF_fullOffset         1109
#define IDFF_halfOffset         1111
#define IDFF_offsetY_X          1103
#define IDFF_offsetY_Y          1104
#define IDFF_offsetU_X          1105
#define IDFF_offsetU_Y          1106
#define IDFF_offsetV_X          1107
#define IDFF_offsetV_Y          1108
#define IDFF_transfFlip         1112
#define IDFF_transfMirror       1113

#define IDFF_filterGradFun     1150
#define IDFF_isGradFun         1151
#define IDFF_showGradFun       1152
#define IDFF_orderGradFun      1153
#define IDFF_fullGradFun       1154
#define IDFF_halfGradFun       1155
#define IDFF_gradFunThreshold  1156

#define IDFF_filterVis          1200
#define IDFF_isVis              1201
#define IDFF_showVis            1206
#define IDFF_orderVis           1202
#define IDFF_visMV              1203
#define IDFF_visQuants          1204
#define IDFF_visGraph           1205

#define IDFF_filterAvisynth          1250
#define IDFF_isAvisynth              1251
#define IDFF_showAvisynth            1260
#define IDFF_orderAvisynth           1252
#define IDFF_fullAvisynth            1253
#define IDFF_avisynthFfdshowSource   1259
#define IDFF_avisynthApplyPulldown   1261
#define IDFF_avisynthInYV12          1255
#define IDFF_avisynthInYUY2          1256
#define IDFF_avisynthInRGB24         1257
#define IDFF_avisynthInRGB32         1258
#define IDFF_avisynthEnableBuffering 1262
#define IDFF_avisynthBufferAhead     1263
#define IDFF_avisynthBufferBack      1264
#define IDFF_avisynthScript          1254 // Use this if you want to manipulate the setting through external API.
#define IDFF_avisynthScriptMULTI_SZ  1265 // Used to save data. If both IDFF_avisynthScript and IDFF_avisynthScriptMULTI_SZ exist, IDFF_avisynthScriptMULTI_SZ is used. If only one of these exists, the exsiting one is used.

#define IDFF_filterOSD             3527
#define IDFF_isOSD                 1501
#define IDFF_orderOSD              3528
#define IDFF_showOSD               3529
#define IDFF_OSDfontName           1509
#define IDFF_OSDfontCharset        1502
#define IDFF_OSDfontSize           1503
#define IDFF_OSDfontWeight         1504
#define IDFF_OSDfontOutlineWidth   3388
#define IDFF_OSDfontOpaqueBox      3401
#define IDFF_OSDfontSpacing        1507
#define IDFF_OSDfontColor          1508
#define IDFF_OSDfontOutlineColor   3393
#define IDFF_OSDfontShadowColor    3394
#define IDFF_OSDfontBodyAlpha      3395
#define IDFF_OSDfontOutlineAlpha   3396
#define IDFF_OSDfontShadowAlpha    3397
#define IDFF_OSDfontShadowSize     3398
#define IDFF_OSDfontShadowMode     3399
#define IDFF_OSDfontBlur           3407
#define IDFF_OSDfontXscale         1532
#define IDFF_OSDfontYscale         3412
#define IDFF_OSDfontAspectAuto     3413
//#define IDFF_OSDfontFast           1535
  #define IDFF_OSDtype_inputSize          1520
  #define IDFF_OSDtype_inputAspect        1550
  #define IDFF_OSDtype_inputSizeAspect    1552
  #define IDFF_OSDtype_outputSize         1521
  #define IDFF_OSDtype_outputAspect       1551
  #define IDFF_OSDtype_cpuUsage           1522
  #define IDFF_OSDtype_TimeOnffdshow      1565
  #define IDFF_OSDtype_systemTime         1523
  #define IDFF_OSDtype_meanQuant          1524
  #define IDFF_OSDtype_currentFrameTime   1525
  #define IDFF_OSDtype_remainingFrameTime 1526
  #define IDFF_OSDtype_movieTime          1527
  #define IDFF_OSDtype_bps                1529
  #define IDFF_OSDtype_encoderInfo        1531
  #define IDFF_OSDtype_shortMsg           1528
  #define IDFF_OSDtype_shortInfo          1536
  #define IDFF_OSDtype_gmcWarpPoints      1539
  #define IDFF_OSDtype_movieSource        33
  #define IDFF_OSD_userformat             3491
   #define IDFF_MOVIE_NONE     0
   #define IDFF_MOVIE_LAVC     1
   //#define IDFF_MOVIE_XVID   2
   #define IDFF_MOVIE_THEO     3
   #define IDFF_MOVIE_RAW      4
   #define IDFF_MOVIE_LIBMPEG2 5
   #define IDFF_MOVIE_MPLAYER  6
   #define IDFF_MOVIE_LIBMAD   7
   #define IDFF_MOVIE_LIBFAAD  8
   #define IDFF_MOVIE_XVID4    9
   #define IDFF_MOVIE_AVIS    10
   #define IDFF_MOVIE_WMV9    12
   #define IDFF_MOVIE_SKAL    13
   #define IDFF_MOVIE_X264    14
   #define IDFF_MOVIE_LIBA52  15
   //#define IDFF_MOVIE_SPDIF   16
   #define IDFF_MOVIE_LIBDTS  17
   #define IDFF_MOVIE_TREMOR  18
   #define IDFF_MOVIE_REALAAC 19
   #define IDFF_MOVIE_FFMPEG_MT 21
   #define IDFF_MOVIE_FFMPEG_DXVA 22
   #define IDFF_MOVIE_MAX     22 // should equal highest value of codecs above (see TglobalSettings.cpp)
  #define IDFF_OSDtype_accurDeblock        30
  #define IDFF_OSDtype_outputFOURCC      1540
  #define IDFF_OSDtype_inputFPS          1541
  #define IDFF_OSDtype_sourceFlnm        1542
  #define IDFF_OSDtype_inputFOURCC       1543
  #define IDFF_OSDtype_inCodecString     1544
  #define IDFF_OSDtype_outCodecString    1545
  #define IDFF_OSDtype_outSpeakersConfig 1549
  #define IDFF_frameType                   31
  #define IDFF_frameSize                   41
  #define IDFF_OSDtype_frameMD5          1548
  #define IDFF_OSDtype_frameTimestamps   1553
  #define IDFF_OSDtype_frameDuration     1554
  #define IDFF_OSDtype_h264_poc          3523
  #define IDFF_OSDtype_exeflnm           1556
  #define IDFF_OSDtype_activePresetName  3483
  #define IDFF_OSDtype_sourceFlnmWOpath  1557
  #define IDFF_OSDtype_sampleFrequency   1558
  #define IDFF_OSDtype_nchannels         1559
  #define IDFF_OSDtype_audioSampleFormat 1560
  #define IDFF_OSDtype_audioJitter       1561
  #define IDFF_OSDtype_idct              3370
  #ifdef OSDTIMETABALE
   #define IDFF_OSDtype_timetable        1562
  #endif
  #define IDFF_OSDtype_QueueCount        1563
  #define IDFF_OSDtype_Late              1564
  #define IDFF_OSDtype_AviSynth_Info     1566
//#define IDFF_OSDuser               1511 //don't use these, use shortOSDmessageAbsolute(msg,duration,x,y) instead
//#define IDFF_OSDuserPx             1512
//#define IDFF_OSDuserPy             1513
//#define IDFF_OSDcurPreset          1530
#define IDFF_OSDposX               1533
#define IDFF_OSDposY               1534
//#define IDFF_OSDstartPreset        1537
#define IDFF_OSDdurationVisible    1538 //in frames
#define IDFF_OSDisSave             1546
#define IDFF_OSDsaveFlnm           1547
#define IDFF_OSDsaveOnly           1555
#define IDFF_OSDformat             3530
#define IDFF_OSDisAutoHide         3531

#define IDFF_filterGrab    2000
#define IDFF_isGrab        2001
#define IDFF_showGrab      2013
#define IDFF_orderGrab     2002
#define IDFF_fullGrab      2003
#define IDFF_grabPath      2004
#define IDFF_grabPrefix    2005
#define IDFF_grabDigits    2006
#define IDFF_grabFormat    2007 //0 - jpeg, 1 - bmp, 2 - png
#define IDFF_grabMode      2008 //0 - all frames, 1 - one frame (grabFrameNum), 2 - range (grabFrameNum1-grabFrameNum2)
#define IDFF_grabFrameNum  2009
#define IDFF_grabFrameNum1 2010
#define IDFF_grabFrameNum2 2011
#define IDFF_grabQual      2012 //0..100
#define IDFF_grabStep      2014
//#define IDFF_grabNow       2015

#define IDFF_filterLogoaway          1450
#define IDFF_isLogoaway              1451
#define IDFF_showLogoaway            1452
#define IDFF_orderLogoaway           1453
#define IDFF_fullLogoaway            1454
#define IDFF_logoawayX               1455
#define IDFF_logoawayY               1456
#define IDFF_logoawayDx              1457
#define IDFF_logoawayDy              1458
#define IDFF_logoawayMode            1459
#define IDFF_logoawayBlur            1460
#define IDFF_logoawayPointnw         1461
#define IDFF_logoawayPointne         1462
#define IDFF_logoawayPointsw         1463
#define IDFF_logoawayPointse         1464
#define IDFF_logoawayBordn_mode      1465
#define IDFF_logoawayBorde_mode      1466
#define IDFF_logoawayBords_mode      1467
#define IDFF_logoawayBordw_mode      1468
#define IDFF_logoawayVhweight        1469
#define IDFF_logoawaySolidcolor      1470
#define IDFF_logoawayLumaOnly        1471
#define IDFF_logoawayParamBitmapFlnm 1473

#define IDFF_filterBitmap   1650
#define IDFF_isBitmap       1651
#define IDFF_showBitmap     1652
#define IDFF_orderBitmap    1653
#define IDFF_fullBitmap     1654
#define IDFF_bitmapFlnm     1655
#define IDFF_bitmapPosx     1656
#define IDFF_bitmapPosy     1657
#define IDFF_bitmapPosmode  1658
#define IDFF_bitmapAlign    1659
#define IDFF_bitmapMode     1660
#define IDFF_bitmapStrength 1661

#define IDFF_overlayControl     2100
#define IDFF_isOverlayControl   2101
#define IDFF_overlayOrder       2109 // read-only
#define IDFF_overlayBrightness  2102
#define IDFF_overlayContrast    2103
#define IDFF_overlayHue         2104
#define IDFF_overlaySaturation  2105
#define IDFF_overlaySharpness   2106
#define IDFF_overlayGamma       2107
#define IDFF_overlayColorEnable 2108
#define IDFF_overlayReset       3486

#define IDFF_isKeys             1701
#define IDFF_keysAlways         1702
#define IDFF_keysShortOsd       1707
#define IDFF_keysSeek1          1708
#define IDFF_keysSeek2          1709

#define IDFF_isRemote           1750
#define IDFF_remoteMessageMode  1751 // 0 - use RegisterWindowMessage, 1 - let user specify message number (default WM_APP+18)
#define IDFF_remoteMessageUser  1752
#define IDFF_remoteAcceptKeys   1753 // remote API window accepts keyboard messages

#define IDFF_isMouse            1770
#define IDFF_mouseAlways        1771

#define IDFF_xvid               1001 //are AVIs with this FOURCC played by ffdshow?
#define IDFF_div3               1002
#define IDFF_mp4v               1003
#define IDFF_dx50               1004
#define IDFF_fvfw               1022
#define IDFF_mp43               1005
#define IDFF_mp42               1006
#define IDFF_mp41               1007
#define IDFF_h263               1008
#define IDFF_h264               1047
#define IDFF_h261               1065
#define IDFF_wmv1               1011
#define IDFF_wmv2               1017
#define IDFF_wmv3               1042
#define IDFF_wvc1               1090
#define IDFF_cavs               1091
#define IDFF_vp5                1093
#define IDFF_vp6                1094
#define IDFF_vp6f               1095
#define IDFF_rt21               1096
#define IDFF_vixl               1097
#define IDFF_aasc               1098
#define IDFF_qtrpza             1099
#define IDFF_mjpg               1014
#define IDFF_dvsd               1015
#define IDFF_hfyu               1016
#define IDFF_cyuv               1019
#define IDFF_mpg1               1012
#define IDFF_mpg2               1013
#define IDFF_mpegAVI            1021
#define IDFF_asv1               1020
#define IDFF_vcr1               1040
#define IDFF_rle                1041
#define IDFF_theo               1023
#define IDFF_rv10               1037
#define IDFF_ffv1               1038
#define IDFF_vp3                1039
#define IDFF_tscc               1060
#define IDFF_rawv               1009 // 0 - unsupported, 1 - all, 2 - all YUV, 3 - all RGB, else FOURCC of accepted colorspace
#define IDFF_alternateUncompressed 3410 // Enable alternate method to enable FFDShow on uncompressed streams (for Vista VMP11 and VMC)
#define IDFF_isDyInterlaced     1330 // enable height dependant interlaced colorspace conversions
#define IDFF_dyInterlaced       1331
#define IDFF_svq1               1025
#define IDFF_svq3               1026
#define IDFF_cram               1027
#define IDFF_iv32               1034
#define IDFF_cvid               1035
#define IDFF_mszh               1044
#define IDFF_zlib               1045
#define IDFF_flv1               1049
#define IDFF_8bps               1050
#define IDFF_png1               1051
#define IDFF_qtrle              1052
#define IDFF_duck               1053
#define IDFF_qpeg               1064
#define IDFF_loco               1066
#define IDFF_wnv1               1067
#define IDFF_cscd               1072
#define IDFF_zmbv               1073
#define IDFF_ulti               1074
#define IDFF_wma7               1028
#define IDFF_wma8               1029
#define IDFF_mp2                1030
#define IDFF_mp3                1031
#define IDFF_ac3                1032
#define IDFF_eac3               1088
#define IDFF_dts                1057
#define IDFF_dtsinwav           1059
#define IDFF_aac                1033
#define IDFF_amr                1046
#define IDFF_iadpcm             1054
#define IDFF_msadpcm            1061
#define IDFF_otherAdpcm         1069
#define IDFF_law                1062
#define IDFF_gsm                1063
#define IDFF_flac               1055
#define IDFF_tta                1068
#define IDFF_qdm2               1070
#define IDFF_mace               1075
#define IDFF_truespeech         1071
#define IDFF_vorbis             1058
#define IDFF_lpcm               1056
#define IDFF_fps1               1077
#define IDFF_snow               1078
#define IDFF_ra                 1079
#define IDFF_imc                1080
#define IDFF_mss2               1081
#define IDFF_wvp2               1082
#define IDFF_em2v               1083
#define IDFF_avrn               1084
#define IDFF_cdvc               1085
#define IDFF_atrac3             1086
#define IDFF_nellymoser         1087
#define IDFF_wavpack            1089 //1088 is eac3
#define IDFF_rawa               1036
#define IDFF_avisV              1043
#define IDFF_avisA              1048
#define IDFF_mlp                1097
#define IDFF_truehd             3515
#define IDFF_rv40               1332
#define IDFF_rv30               1333
#define IDFF_cook               1334

#define IDFF_filterOutputVideo   1357
#define IDFF_hwOverlayOld        1317
#define IDFF_setSARinOutSample   1350 // 0 - VIDEOINFOHEADER, 1 - VIDEOINFOHEADER2, 2 - first try VIDEOINFOHEADER2, then VIDEOINFOHEADER
#define IDFF_hwOverlay           1350 // deprecated.
#define IDFF_outI420             1309
#define IDFF_outYV12             1301
#define IDFF_outYUY2             1302
#define IDFF_outYVYU             1303
#define IDFF_outUYVY             1304
#define IDFF_outNV12             1360
#define IDFF_outRGB32            1305
#define IDFF_outRGB24            1306
#define IDFF_outRGB555           1307
#define IDFF_outRGB565           1308
#define IDFF_outClosest          1356
#define IDFF_outDV               1358
#define IDFF_outDVnorm           1359
#define IDFF_allowOutChange      1352
#define IDFF_outChangeCompatOnly 1354
#define IDFF_hwDeinterlaceOld    1318
#define IDFF_setDeintInOutSample 1353
#define IDFF_hwDeinterlace       1353 // deprecated.
#define IDFF_hwDeintMethod       1361
#define IDFF_hwDeintFieldOrder   3500
#define IDFF_avisynthYV12_RGB    1355 // deprecated. Use IDFF_highQualityRGB instead.
#define IDFF_highQualityRGB      1355
#define IDFF_RGB_dithering       3516
#define IDFF_cspOptionsIturBt            1362 // 0: 601, 1:709
#define IDFF_cspOptionsBlackCutoff       3424
#define IDFF_cspOptionsWhiteCutoff       3425
#define IDFF_cspOptionsChromaCutoff      3426
//#define IDFF_cspOptionsInterlockChroma   3427
#define IDFF_cspOptionsCutoffMode        3428 // deprecated, use IDFF_cspOptionsInputLevelsMode.
#define IDFF_cspOptionsInputLevelsMode   3428
#define IDFF_cspOptionsOutputLevelsMode  3512
#define IDFF_cspOptionsRgbInterlaceMode  3508

#define IDFF_filterOutputAudio      1314
#define IDFF_outsfs                 1310
#define IDFF_outAC3bitrate          1313

#define IDFF_aoutConnectTo          1315
#define IDFF_aoutConnectToOnlySpdif 1316
#define IDFF_aoutMultichannelDevice 3377
#define IDFF_aoutMultichannelDeviceId 3378
#define IDFF_aoutAC3EncodeMode      3379
#define IDFF_aoutpassthroughAC3     3501
#define IDFF_aoutpassthroughDTS     3502
#define IDFF_aoutpassthroughTRUEHD  3524
#define IDFF_aoutpassthroughDTSHD   3525
#define IDFF_aoutpassthroughEAC3    3526
#define IDFF_aoutpassthroughPCMConnection 3533
#define IDFF_aoutUseIEC61937        3532
#define IDFF_aoutpassthroughDeviceId 3534

#define IDFF_dithering          1311
#define IDFF_noiseShaping       1312
#define IDFF_audio_decoder_DRC  3509
#define IDFF_audio_decoder_DRC_Level 3510
#define IDFF_audio_decoder_JitterCorrection 3535
#define IDFF_audio_decoder_delay  3540

#define IDFF_filterDScaler        2200
#define IDFF_isDScaler            2201
#define IDFF_showDScaler          2206
#define IDFF_orderDScaler         2202
#define IDFF_fullDScaler          2203
#define IDFF_halfDScaler          2207
#define IDFF_dscalerFltflnm       2204
#define IDFF_dscalerCfg           2205
#define IDFF_dscalerPath          2208

#define IDFF_filterPerspective        2300
#define IDFF_isPerspective            2301
#define IDFF_showPerspective          2314
#define IDFF_orderPerspective         2302
#define IDFF_fullPerspective          2303
#define IDFF_halfPerspective          2315
#define IDFF_perspectiveIsSrc         2313
#define IDFF_perspectiveX1            2304
#define IDFF_perspectiveY1            2305
#define IDFF_perspectiveX2            2306
#define IDFF_perspectiveY2            2307
#define IDFF_perspectiveX3            2308
#define IDFF_perspectiveY3            2309
#define IDFF_perspectiveX4            2310
#define IDFF_perspectiveY4            2311
#define IDFF_perspectiveInterpolation 2312

//----------------------- audio decoding -------------------------
#define IDFF_preferredsfs         2399
#define IDFF_alwaysextensible     2398
#define IDFF_allowOutStream       2397
#define IDFF_vorbisgain           2396

#define IDFF_filterVolume         2400
#define IDFF_isVolume             2401
#define IDFF_showVolume           2405
#define IDFF_orderVolume          2402
#define IDFF_volume               2403
#define IDFF_volumeL              2408
#define IDFF_volumeC              2409
#define IDFF_volumeR              2410
#define IDFF_volumeSL             2411
#define IDFF_volumeSR             2412
#define IDFF_volumeAL             3479
#define IDFF_volumeAR             3480
#define IDFF_volumeLFE            2413
#define IDFF_volumeLmute          2415 //1 - mute, 2 - solo
#define IDFF_volumeCmute          2416
#define IDFF_volumeRmute          2417
#define IDFF_volumeSLmute         2418
#define IDFF_volumeSRmute         2419
#define IDFF_volumeALmute         3481
#define IDFF_volumeARmute         3482
#define IDFF_volumeLFEmute        2420
#define IDFF_volumeNormalize      2404
#define IDFF_maxNormalization     2406 //*100
//#define IDFF_volumeNormalizeBufferLength      3368
#define IDFF_volumeNormalizeResetOnSeek       3369
#define IDFF_volumeNormalizeRegainVolume      3499
//#define IDFF_currentNormalization 2407 //*100
#define IDFF_showCurrentVolume    2414

#define IDFF_filterEQ  2430
#define IDFF_isEQ      2431
#define IDFF_showEQ    2446
#define IDFF_orderEQ   2432
#define IDFF_eq0       2433
#define IDFF_eq1       2434
#define IDFF_eq2       2435
#define IDFF_eq3       2436
#define IDFF_eq4       2437
#define IDFF_eq5       2438
#define IDFF_eq6       2440
#define IDFF_eq7       2441
#define IDFF_eq8       2442
#define IDFF_eq9       2443
#define IDFF_eqLowdb   2444
#define IDFF_eqHighdb  2445
#define IDFF_eq0freq   2447
#define IDFF_eq1freq   2448
#define IDFF_eq2freq   2449
#define IDFF_eq3freq   2450
#define IDFF_eq4freq   2451
#define IDFF_eq5freq   2452
#define IDFF_eq6freq   2453
#define IDFF_eq7freq   2454
#define IDFF_eq8freq   2455
#define IDFF_eq9freq   2456
#define IDFF_eqSuper   2457

#define IDFF_filterWinamp2     2460
#define IDFF_isWinamp2         2461
#define IDFF_showWinamp2       2466
#define IDFF_orderWinamp2      2462
#define IDFF_winamp2flnm       2463
#define IDFF_winamp2filtername 2464
#define IDFF_winamp2dir        2465
#define IDFF_winamp2allowMultichannelOnlyIn 3380

#define IDFF_filterFreeverb   2500
#define IDFF_isFreeverb       2501
#define IDFF_showFreeverb     2502
#define IDFF_orderFreeverb    2503
#define IDFF_freeverbRoomsize 2504
#define IDFF_freeverbDamp     2505
#define IDFF_freeverbWet      2506
#define IDFF_freeverbDry      2507
#define IDFF_freeverbWidth    2508
#define IDFF_freeverbMode     2509

#define IDFF_filterResample  2530
#define IDFF_isResample      2531
#define IDFF_showResample    2532
#define IDFF_orderResample   2533
#define IDFF_resampleFreq    2534
#define IDFF_resampleMode    2535
#define IDFF_resampleIf      2536 // 0 - always, 1 - conditional
#define IDFF_resampleIfCond  2537 // -1 - less, 1 - more
#define IDFF_resampleIfFreq  2538

#define IDFF_filterMixer       2480
#define IDFF_isMixer           2481
#define IDFF_showMixer         2483
#define IDFF_orderMixer        2484
#define IDFF_mixerOut          2482
#define IDFF_normalizeMatrix   2485
#define IDFF_mixerMatrix00     2486 // *100000
#define IDFF_mixerMatrix01     2487
#define IDFF_mixerMatrix02     2488
#define IDFF_mixerMatrix03     2489
#define IDFF_mixerMatrix04     2490
#define IDFF_mixerMatrix05     2491
#define IDFF_mixerMatrix06     3445
#define IDFF_mixerMatrix07     3446
#define IDFF_mixerMatrix08     3471
#define IDFF_mixerMatrix10     2492
#define IDFF_mixerMatrix11     2493
#define IDFF_mixerMatrix12     2494
#define IDFF_mixerMatrix13     2495
#define IDFF_mixerMatrix14     2496
#define IDFF_mixerMatrix15     2497
#define IDFF_mixerMatrix16     3447
#define IDFF_mixerMatrix17     3448
#define IDFF_mixerMatrix18     3472
#define IDFF_mixerMatrix20     2498
#define IDFF_mixerMatrix21     2499
#define IDFF_mixerMatrix22     2600
#define IDFF_mixerMatrix23     2601
#define IDFF_mixerMatrix24     2602
#define IDFF_mixerMatrix25     2603
#define IDFF_mixerMatrix26     3449
#define IDFF_mixerMatrix27     3450
#define IDFF_mixerMatrix28     3473
#define IDFF_mixerMatrix30     2604
#define IDFF_mixerMatrix31     2605
#define IDFF_mixerMatrix32     2606
#define IDFF_mixerMatrix33     2607
#define IDFF_mixerMatrix34     2608
#define IDFF_mixerMatrix35     2609
#define IDFF_mixerMatrix36     3451
#define IDFF_mixerMatrix37     3452
#define IDFF_mixerMatrix38     3474
#define IDFF_mixerMatrix40     2610
#define IDFF_mixerMatrix41     2611
#define IDFF_mixerMatrix42     2612
#define IDFF_mixerMatrix43     2613
#define IDFF_mixerMatrix44     2614
#define IDFF_mixerMatrix45     2615
#define IDFF_mixerMatrix46     3453
#define IDFF_mixerMatrix47     3454
#define IDFF_mixerMatrix48     3475
#define IDFF_mixerMatrix50     2616
#define IDFF_mixerMatrix51     2617
#define IDFF_mixerMatrix52     2618
#define IDFF_mixerMatrix53     2619
#define IDFF_mixerMatrix54     2620
#define IDFF_mixerMatrix55     2621
#define IDFF_mixerMatrix56     3455
#define IDFF_mixerMatrix57     3456
#define IDFF_mixerMatrix58     3476
#define IDFF_mixerMatrix60     3429
#define IDFF_mixerMatrix61     3430
#define IDFF_mixerMatrix62     3431
#define IDFF_mixerMatrix63     3432
#define IDFF_mixerMatrix64     3433
#define IDFF_mixerMatrix65     3434
#define IDFF_mixerMatrix66     3435
#define IDFF_mixerMatrix67     3436
#define IDFF_mixerMatrix68     3477
#define IDFF_mixerMatrix70     3437
#define IDFF_mixerMatrix71     3438
#define IDFF_mixerMatrix72     3439
#define IDFF_mixerMatrix73     3440
#define IDFF_mixerMatrix74     3441
#define IDFF_mixerMatrix75     3442
#define IDFF_mixerMatrix76     3443
#define IDFF_mixerMatrix77     3444
#define IDFF_mixerMatrix78     3461
#define IDFF_mixerMatrix80     3462
#define IDFF_mixerMatrix81     3463
#define IDFF_mixerMatrix82     3464
#define IDFF_mixerMatrix83     3465
#define IDFF_mixerMatrix84     3466
#define IDFF_mixerMatrix85     3467
#define IDFF_mixerMatrix86     3468
#define IDFF_mixerMatrix87     3469
#define IDFF_mixerMatrix88     3470

#define IDFF_customMatrix      2622
#define IDFF_mixerExpandStereo 2623
#define IDFF_mixerVoiceControl 2624
#define IDFF_mixerClev         2626
#define IDFF_mixerSlev         2627
#define IDFF_mixerLFElev       2628
#define IDFF_headphone_dim     2625

#define IDFF_filterDolbyDecoder 2650
#define IDFF_isDolbyDecoder     2651
#define IDFF_showDolbyDecoder   2652
#define IDFF_orderDolbyDecoder  2653
#define IDFF_dolbyDecoderDelay  2654

#define IDFF_filterDelay  2670
#define IDFF_isDelay      2671
#define IDFF_showDelay    2672
#define IDFF_orderDelay   2673
#define IDFF_delayL       2674
#define IDFF_delayC       2675
#define IDFF_delayR       2676
#define IDFF_delaySL      2677
#define IDFF_delaySR      2678
#define IDFF_delayBC      3478
#define IDFF_delayLFE     2679
#define IDFF_delayAL      3457
#define IDFF_delayAR      3458

#define IDFF_filterFir      2700
#define IDFF_isFir          2701
#define IDFF_showFir        2702
#define IDFF_orderFir       2703
#define IDFF_firTaps        2704
#define IDFF_firType        2705 // 0 - lowpass, 1 - highpass, 2 - band pass, 3 - band stop
#define IDFF_firFreq        2706
#define IDFF_firWidth       2707 // for bandpass and bandstop
#define IDFF_firWindow      2708 // 0 - box, 1 - triangle, 2 - hamming, 3 - hanning, 4 - blackman, 5 - flattop, 6 - kaiser
#define IDFF_firKaiserBeta  2709 // *1000
#define IDFF_showCurrentFFT 2710
#define IDFF_firIsUserDisplayMaxFreq 2711
#define IDFF_firUserDisplayMaxFreq   2712
//#define IDFF_firCurrentFreq          2713

#define IDFF_filterAudioDenoise    2550
#define IDFF_isAudioDenoise        2551
#define IDFF_showAudioDenoise      2552
#define IDFF_orderAudioDenoise     2553
#define IDFF_audioDenoiseThreshold 2554

#define IDFF_isAudioSwitcher 2570

#define IDFF_filterCrystality           2750
#define IDFF_isCrystality               2751
#define IDFF_showCrystality             2752
#define IDFF_orderCrystality            2753
#define IDFF_crystality_bext_level      2754
#define IDFF_crystality_echo_level      2755
#define IDFF_crystality_stereo_level    2756
#define IDFF_crystality_filter_level    2757
#define IDFF_crystality_feedback_level  2758
#define IDFF_crystality_harmonics_level 2759

#define IDFF_filterLFEcrossover 2770
#define IDFF_isLFEcrossover     2771
#define IDFF_showLFEcrossover   2772
#define IDFF_orderLFEcrossover  2773
#define IDFF_LFEcrossoverFreq   2774
#define IDFF_LFEcrossoverGain   2775
#define IDFF_LFEcutLR           2776

#define IDFF_filterChannelSwap  2800
#define IDFF_isChannelSwap      2801
#define IDFF_showChannelSwap    2802
#define IDFF_orderChannelSwap   2803
#define IDFF_channelSwapL       2804
#define IDFF_channelSwapR       2805
#define IDFF_channelSwapC       2806
#define IDFF_channelSwapSL      2807
#define IDFF_channelSwapREAR    2808
#define IDFF_channelSwapSR      2809
#define IDFF_channelSwapLFE     2810
#define IDFF_channelSwapAL      3459
#define IDFF_channelSwapAR      3460

#define IDFF_filterConvolver          2850
#define IDFF_isConvolver              2851
#define IDFF_showConvolver            2852
#define IDFF_orderConvolver           2853
#define IDFF_convolverMappingMode     2858
#define IDFF_convolverFile            2854
#define IDFF_convolverFileL           2859
#define IDFF_convolverFileR           2860
#define IDFF_convolverFileC           2861
#define IDFF_convolverFileSL          2862
#define IDFF_convolverFileSR          2863
#define IDFF_convolverFileLFE         2864
#define IDFF_convolverMixingStrength  2855
#define IDFF_convolverLevelAdjustDB   2856 // *10
#define IDFF_convolverLevelAdjustAuto 2857

// DXVA
#define IDFF_dec_DXVA_H264             3536
#define IDFF_dec_DXVA_VC1              3537
#define IDFF_dec_DXVA_CompatibilityMode 3538
#define IDFF_dec_DXVA_PostProcessingMode 3539

//----------------------- encoding -------------------------
#define IDFF_numthreads      3322
#define IDFF_enc_mode        3000
#define IDFF_enc_bitrate1000 3001
#define IDFF_enc_desiredSize 3002
#define IDFF_enc_quant       3003
#define IDFF_enc_qual        3004

#define IDFF_enc_codecId 3005
#define IDFF_enc_fourcc  3006

#define IDFF_enc_max_key_interval 3007
#define IDFF_enc_min_key_interval 3008
#define IDFF_enc_keySceneChange   3332
#define IDFF_enc_globalHeader     3009
#define IDFF_enc_part             3010
#define IDFF_enc_interlacing      3011
#define IDFF_enc_interlacing_tff  3358
#define IDFF_enc_gray             3012
#define IDFF_enc_isBframes        3013
#define IDFF_enc_max_b_frames     3014
#define IDFF_enc_b_dynamic        3015
#define IDFF_enc_b_refine         3364
#define IDFF_enc_packedBitstream  3016
#define IDFF_enc_dx50bvop         3017
#define IDFF_enc_aspectMode       3018
#define IDFF_enc_sarX1000         3019
#define IDFF_enc_sarY1000         3020
#define IDFF_enc_darX1000         3327
#define IDFF_enc_darY1000         3328
#define IDFF_enc_H263Pflags       3311

#define IDFF_enc_huffyuv_csp  3021
#define IDFF_enc_huffyuv_pred 3022
#define IDFF_enc_huffyuv_ctx  3336

#define IDFF_enc_ljpeg_csp  3305

#define IDFF_enc_ffv1_coder   3023
#define IDFF_enc_ffv1_context 3024
#define IDFF_enc_ffv1_csp     3025
#define IDFF_enc_ffv1_key_interval 3373

#define IDFF_enc_wmv9_kfsecs 3026
#define IDFF_enc_wmv9_ivtc   3027
#define IDFF_enc_wmv9_deint  3028
#define IDFF_enc_wmv9_cplx   3029
#define IDFF_enc_wmv9_crisp  3030
#define IDFF_enc_wmv9_aviout 3031

#define IDFF_enc_forceIncsp 3032
#define IDFF_enc_incsp      3033
#define IDFF_enc_isProc     3034
#define IDFF_enc_flip       3035
#define IDFF_enc_dropFrames 3331

#define IDFF_enc_storeAVI        3036
#define IDFF_enc_storeExt        3037
#define IDFF_enc_storeExtFlnm    3038
#define IDFF_enc_ownStoreExt     3216
#define IDFF_enc_muxer           3039
#define IDFF_enc_ff1_stats_mode  3040
#define IDFF_enc_ff1_stats_flnm  3041
#define IDFF_enc_isFPSoverride   3042
#define IDFF_enc_fpsOverrideNum  3043
#define IDFF_enc_fpsOverrideDen  3044

#define IDFF_enc_me_hq                   3045
#define IDFF_enc_me_4mv                  3046
#define IDFF_enc_me_qpel                 3047
#define IDFF_enc_me_gmc                  3048
#define IDFF_enc_me_mv0                  3217
#define IDFF_enc_me_cbp_rd               3218
#define IDFF_enc_me_cmp                  3049
#define IDFF_enc_me_cmp_chroma           3050
#define IDFF_enc_me_subcmp               3051
#define IDFF_enc_me_subcmp_chroma        3052
#define IDFF_enc_mb_cmp                  3053
#define IDFF_enc_mb_cmp_chroma           3054
#define IDFF_enc_dia_size                3055
#define IDFF_enc_me_last_predictor_count 3056
#define IDFF_enc_me_prepass              3057
#define IDFF_enc_me_precmp               3058
#define IDFF_enc_me_precmp_chroma        3059
#define IDFF_enc_dia_size_pre            3060
#define IDFF_enc_me_subq                 3061
#define IDFF_enc_me_nsse_weight          3321
#define IDFF_enc_me_iterative            3366

#define IDFF_enc_xvid_motion_search        3062
#define IDFF_enc_is_xvid_me_custom         3063
#define IDFF_enc_xvid_me_custom            3064
#define IDFF_enc_xvid_me_inter4v           3065
#define IDFF_enc_xvid_vhq                  3066
#define IDFF_enc_xvid_vhq_modedecisionbits 3067
#define IDFF_enc_is_xvid_vhq_custom        3068
#define IDFF_enc_xvid_vhq_custom           3069

#define IDFF_enc_skalSearchMetric          3310

#define IDFF_enc_quant_type                3070
#define IDFF_enc_qmatrix_intra_custom0     3272
#define IDFF_enc_qmatrix_intra_custom1     3273
#define IDFF_enc_qmatrix_intra_custom2     3274
#define IDFF_enc_qmatrix_intra_custom3     3275
#define IDFF_enc_qmatrix_intra_custom4     3276
#define IDFF_enc_qmatrix_intra_custom5     3277
#define IDFF_enc_qmatrix_intra_custom6     3278
#define IDFF_enc_qmatrix_intra_custom7     3279
#define IDFF_enc_qmatrix_intra_custom8     3280
#define IDFF_enc_qmatrix_intra_custom9     3281
#define IDFF_enc_qmatrix_intra_custom10    3282
#define IDFF_enc_qmatrix_intra_custom11    3283
#define IDFF_enc_qmatrix_intra_custom12    3284
#define IDFF_enc_qmatrix_intra_custom13    3285
#define IDFF_enc_qmatrix_intra_custom14    3286
#define IDFF_enc_qmatrix_intra_custom15    3287
#define IDFF_enc_qmatrix_inter_custom0     3288
#define IDFF_enc_qmatrix_inter_custom1     3289
#define IDFF_enc_qmatrix_inter_custom2     3290
#define IDFF_enc_qmatrix_inter_custom3     3291
#define IDFF_enc_qmatrix_inter_custom4     3292
#define IDFF_enc_qmatrix_inter_custom5     3293
#define IDFF_enc_qmatrix_inter_custom6     3294
#define IDFF_enc_qmatrix_inter_custom7     3295
#define IDFF_enc_qmatrix_inter_custom8     3296
#define IDFF_enc_qmatrix_inter_custom9     3297
#define IDFF_enc_qmatrix_inter_custom10    3298
#define IDFF_enc_qmatrix_inter_custom11    3299
#define IDFF_enc_qmatrix_inter_custom12    3300
#define IDFF_enc_qmatrix_inter_custom13    3301
#define IDFF_enc_qmatrix_inter_custom14    3302
#define IDFF_enc_qmatrix_inter_custom15    3303
#define IDFF_enc_qmatrix_intra4x4Y_custom0 3342
#define IDFF_enc_qmatrix_intra4x4Y_custom1 3343
#define IDFF_enc_qmatrix_intra4x4Y_custom2 3344
#define IDFF_enc_qmatrix_intra4x4Y_custom3 3345
#define IDFF_enc_qmatrix_inter4x4Y_custom0 3346
#define IDFF_enc_qmatrix_inter4x4Y_custom1 3347
#define IDFF_enc_qmatrix_inter4x4Y_custom2 3348
#define IDFF_enc_qmatrix_inter4x4Y_custom3 3349
#define IDFF_enc_qmatrix_intra4x4C_custom0 3350
#define IDFF_enc_qmatrix_intra4x4C_custom1 3351
#define IDFF_enc_qmatrix_intra4x4C_custom2 3352
#define IDFF_enc_qmatrix_intra4x4C_custom3 3353
#define IDFF_enc_qmatrix_inter4x4C_custom0 3354
#define IDFF_enc_qmatrix_inter4x4C_custom1 3355
#define IDFF_enc_qmatrix_inter4x4C_custom2 3356
#define IDFF_enc_qmatrix_inter4x4C_custom3 3357
#define IDFF_enc_q_i_min                   3104
#define IDFF_enc_q_i_max                   3105
#define IDFF_enc_i_quant_factor            3106
#define IDFF_enc_i_quant_offset            3107
#define IDFF_enc_q_p_min                   3108
#define IDFF_enc_q_p_max                   3109
#define IDFF_enc_q_b_min                   3110
#define IDFF_enc_q_b_max                   3111
#define IDFF_enc_q_mb_min                  3112
#define IDFF_enc_q_mb_max                  3113
#define IDFF_enc_trellisquant              3114
#define IDFF_enc_qns                       3304
#define IDFF_enc_b_quant_factor            3115
#define IDFF_enc_b_quant_offset            3116
#define IDFF_enc_isInterQuantBias          3117
#define IDFF_enc_interQuantBias            3118
#define IDFF_enc_isIntraQuantBias          3119
#define IDFF_enc_intraQuantBias            3120
#define IDFF_enc_dct_algo                  3121
#define IDFF_enc_mpeg2_dc_prec             3193

#define IDFF_enc_ff1_vratetol        3122
#define IDFF_enc_ff1_vqcomp          3123
#define IDFF_enc_ff1_vqblur1         3124
#define IDFF_enc_ff1_vqblur2         3125
#define IDFF_enc_ff1_vqdiff          3126
#define IDFF_enc_ff1_rc_squish       3127
#define IDFF_enc_ff1_rc_max_rate1000 3312
#define IDFF_enc_ff1_rc_min_rate1000 3313
#define IDFF_enc_ff1_rc_buffer_size  3314

#define IDFF_enc_dv_profile   3367

#define IDFF_enc_svcd_scan_offset   3315

#define IDFF_enc_xvid_rc_reaction_delay_factor 3128
#define IDFF_enc_xvid_rc_averaging_period      3129
#define IDFF_enc_xvid_rc_buffer                3130

#define IDFF_enc_isCreditsStart     3131
#define IDFF_enc_isCreditsEnd       3132
#define IDFF_enc_creditsStartBegin  3133
#define IDFF_enc_creditsStartEnd    3134
#define IDFF_enc_creditsEndBegin    3135
#define IDFF_enc_creditsEndEnd      3136
#define IDFF_enc_credits_mode       3137
#define IDFF_enc_credits_percent    3138
#define IDFF_enc_credits_quant_i    3139
#define IDFF_enc_credits_quant_p    3140
#define IDFF_enc_credits_size_start 3141
#define IDFF_enc_credits_size_end   3142
#define IDFF_enc_graycredits        3143

#define IDFF_enc_stats1flnm                       3144
#define IDFF_enc_stats2flnm                       3145
#define IDFF_enc_xvid2pass_use_write              3146
#define IDFF_enc_twopass_max_bitrate              3147
#define IDFF_enc_twopass_max_overflow_improvement 3148
#define IDFF_enc_twopass_max_overflow_degradation 3149
#define IDFF_enc_keyframe_boost                   3150
#define IDFF_enc_kftreshold                       3151
#define IDFF_enc_kfreduction                      3152
#define IDFF_enc_curve_compression_high           3153
#define IDFF_enc_curve_compression_low            3154
#define IDFF_enc_bitrate_payback_delay            3155
#define IDFF_enc_bitrate_payback_method           3156
#define IDFF_enc_use_alt_curve                    3157
#define IDFF_enc_alt_curve_type                   3158
#define IDFF_enc_alt_curve_high_dist              3159
#define IDFF_enc_alt_curve_low_dist               3160
#define IDFF_enc_alt_curve_use_auto               3161
#define IDFF_enc_alt_curve_auto_str               3162
#define IDFF_enc_alt_curve_min_rel_qual           3163
#define IDFF_enc_alt_curve_use_auto_bonus_bias    3164
#define IDFF_enc_alt_curve_bonus_bias             3165

#define IDFF_enc_xvid_lum_masking             3166
#define IDFF_enc_xvid_chromaopt               3167
#define IDFF_enc_isElimLum                    3168
#define IDFF_enc_elimLumThres                 3169
#define IDFF_enc_isElimChrom                  3170
#define IDFF_enc_elimChromThres               3171
#define IDFF_enc_is_lavc_nr                   3221
#define IDFF_enc_lavc_nr                      3222
#define IDFF_enc_is_ff_lumi_masking           3172
#define IDFF_enc_ff_lumi_masking1000          3173
#define IDFF_enc_is_ff_temporal_cplx_masking  3174
#define IDFF_enc_ff_temporal_cplx_masking1000 3175
#define IDFF_enc_is_ff_spatial_cplx_masking   3176
#define IDFF_enc_ff_spatial_cplx_masking1000  3177
#define IDFF_enc_is_ff_p_masking              3178
#define IDFF_enc_ff_p_masking1000             3179
#define IDFF_enc_is_ff_dark_masking           3180
#define IDFF_enc_ff_dark_masking1000          3181
#define IDFF_enc_is_ff_border_masking         3334
#define IDFF_enc_ff_border_masking1000        3335
#define IDFF_enc_ff_naq                       3182
#define IDFF_enc_isSkalMasking                3308
#define IDFF_enc_skalMaskingAmp               3309

#define IDFF_enc_theo_hq               3196
#define IDFF_enc_theo_sharpness        3329
#define IDFF_enc_theo_noisesensitivity 3330

#define IDFF_enc_raw_fourcc  3197

#define IDFF_enc_x264_max_ref_frames              3316
#define IDFF_enc_x264_cabac                       3317
#define IDFF_enc_x264_me_inter                    3319
#define IDFF_enc_x264_me_intra                    3320
#define IDFF_enc_x264_me_subpelrefine             3323
#define IDFF_enc_x264_interlaced                  3224
#define IDFF_enc_x264_i_deblocking_filter_alphac0 3325
#define IDFF_enc_x264_i_deblocking_filter_beta    3326
#define IDFF_enc_x264_i_direct_mv_pred            3333
#define IDFF_enc_x264_mv_range                    3337
#define IDFF_enc_x264_b_bframe_pyramid            3338
#define IDFF_enc_x264_b_aud                       3339
#define IDFF_enc_x264_me_method                   3340
#define IDFF_enc_x264_me_range                    3341
#define IDFF_enc_x264_mixed_ref                   3359
#define IDFF_enc_x264_aq_strength100              3360
#define IDFF_enc_x264_b_dct_decimate              3361

#define IDFF_enc_working               3198
#define IDFF_enc_fpsRate               3206
#define IDFF_enc_fpsScale              3207
#define IDFF_enc_psnr                  3199
#define IDFF_enc_showGraph             3204
//#define IDFF_enc_h_graph               3205

#define IDFF_dlgBpsFps1000  3208
#define IDFF_dlgBpsLen      3209
#define IDFF_dlgPerfectDlgX 3210
#define IDFF_dlgPerfectDlgY 3211
#define IDFF_dlgEncGraph    3219
#define IDFF_dlgEncAbout    3220

//next:3541

#endif
