/*****************************************************************************
 * Copyright(c) 2006 Broadcom Corporation, all rights reserved.
 * Contains proprietary and confidential information.
 *
 * This source file is the property of Broadcom Corporation, and
 * may not be copied or distributed in any isomorphic form without
 * the prior written consent of Broadcom Corporation.
 *
 *
 *  Name: bc_drv_if.h
 *
 *  Description: Driver Interface API.
 *
 *  AU
 *
 *  HISTORY:
 *
 ****************************************************************************/
#ifndef _BCM_DRV_IF_H_
#define _BCM_DRV_IF_H_

#include "bc_dts_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
******************************************************************************

                            Theory of operation


    The Device Interface Library (DIL) allows application level code, such
as a DirectShow filter, to access the Broadcom MediaPC 70010 High Definition
decoder driver to provide hardware decoding for MPEG-2, H.264 (AVC) and VC-1
streams.

    In the Microsoft DirectShow system, the overall system graph would look 
like the following:

+--------+  +---------------+  +---------------+  +--------------------+
| Source |->| Demultiplexer |->| Audio decoder |->| DirectSound Device |
+--------+  +---------------+  +---------------+  +--------------------+
                    |
                    |  +-------------------------+  +----------------+
                    +->| Broadcom decoder filter |->| Video Renderer |
                       +-------------------------+  +----------------+
                                  |    |  
                            +----------------+
                            |  Broadcom DIL  |
                            +----------------+
                                  |    |
                            +-----------------+
                            | Broadcom Driver |
                            +-----------------+

    From the view of the caller, the DIL will accept compressed video streams
and will output decoded video frames or fields to seperate Y and UV buffers.
The DIL is responsible solely for decoding video and has no responsibilities
for audio nor for rendering, as shown in the above diagram.  Audio/video
sychronization is assisted by feeding the DIL with timestamps so that it
may pass those timestamps along with the decoded video.  The timestamped
output video will then be presented at the appropriate time by the renderer.

A minimal implementation would be:

    HANDLE              hBRCMhandle;
    U8                  input_buffer[INPUT_SIZE];
    U8                  y_output_buffer[WIDTH*HEIGHT];
    U8                  uv_output_buffer[WIDTH*HEIGHT];
    BC_DTS_PROC_OUT     sProcOutData = { fill in your values here };
    BC_PIC_INFO_BLOCK   sPIB = { fill in your values here };

    // Acquire handle for device.
    DtsDeviceOpen(&hBRCMhandle, 0);
     
    // Elemental stream.
    DtsOpenDecoder(hBRCMhandle, 0);
    
    // H.264, Enable FGT SEI, do not parse metadata, no forced progressive out
    DtsSetVideoParams(hBRCMhandle,0,1,0,0,0);

    // Tell decoder to wait for input from host. (PC)
    DtsStartDecoder(hBRCMhandle);       

    // Input buffer address, input buffer size, no timestamp, Unencrypted
    DtsProcInput(hBRCMhandle,input_buffer,sizeof(input_buffer),0,0);

    // Tell PC to wait for data from decoder.
    DtsStartCapture(hBRCMhandle);       

    // 16ms timeout, pass pointer to PIB then get the decoded picture.
    DtsProcOutput(hBRCMhandle,16,&sPIB);

    // Stop the decoder.
    DtsStopDecoder(hBRCMhandle);

    // Close the decoder
    DtsCloseDecoder(hBRCMhandle);

    // Release handle for device.
    DtsDeviceClose(hBRCMhandle);

******************************************************************************
*****************************************************************************/

    // DLL Export
#if defined(_WIN32)
    #include <windows.h>
    #include <stdio.h>
    #define DRVIFLIB_API __declspec(dllexport)
#else
    #define DRVIFLIB_API
#endif

/*****************************************************************************
Function name:
    
    DtsDeviceOpen

Description:

    Opens a handle to the decoder device that will be used to address that
    unique instance of the decoder for all subsequent operations.

    Must be called once when the application opens the decoder for use.

Parameters:

    *hDevice    Pointer to device handle that will be filled in after the
                device is successfully opened. [OUTPUT]

    mode        Controls the mode in which the device is opened.
                Currently only mode 0 (normal playback) is supported.
                All other values will return BC_STS_INV_ARG.

Return:

    Returns BC_STS_SUCCESS or error codes as appropriate.

*****************************************************************************/
DRVIFLIB_API BC_STATUS DtsDeviceOpen(
    HANDLE  *hDevice,
    U32     mode
    );

/*****************************************************************************

Function name:

    DtsDeviceClose

Description:

    Close the handle to the decoder device.

    Must be called once when the application closes the decoder after use.

Parameters:

    hDevice     Handle to device. This is obtained via a prior call to
                DtsDeviceOpen

Return:

    Returns BC_STS_SUCCESS or error codes as appropriate.

*****************************************************************************/
DRVIFLIB_API BC_STATUS 
DtsDeviceClose(
    HANDLE hDevice
    );

/*****************************************************************************

Function name:

    DtsGetVersion

Description:

    Get version information from the driver as well as API library.
    Version numbers are maintained in <Major>.<Minor>.<Revision> format.
    Example ?01.23.4567

    The device must have been previously opened for this call to succeed.
    The individual components of the revision number are available as follows:

    o Major     (8 Bits) : Bit 31 ?24
    o Minor     (8 Bits) : Bit 23 ?16
    o Revision (16 Bits) : Bits 15 ?Bit 0.

Parameters:

    hDevice     Handle to device. This is obtained via a prior call to
                DtsDeviceOpen
    DrVer       Device driver version
    DilVer      Driver interface library version

Return:
    The revision numbers from the currently loaded driver as well as the
    driver interface API library.
    
    BC_STS_SUCCESS will be returned on successful completion.

*****************************************************************************/
DRVIFLIB_API BC_STATUS 
DtsGetVersion(
    HANDLE  hDevice,
    U32     *DrVer,
    U32     *DilVer
    );

/*****************************************************************************

Function name:

    DtsGetFWVersionFromFile

Description:

    Get version information from the Firmware Bin file when FW is not running
    Version numbers in FW are maintained in <Major>.<Minor>.<Spl Revision> format.
    the return value will be of the format:
    (Major << 16) | (Minor<<8) | Spl_rev ?012345

    The individual components of the revision number are available as follows:

    o Major     (8 Bits) : Bit 24 ?16
    o Minor     (8 Bits) : Bit 16 ?8
    o Revision (16 Bits) : Bits 8 ?0.

Parameters:

    hDevice     Handle to device. This is obtained via a prior call to
                DtsDeviceOpen
    StreamVer   Stream FW version
    DecVer      VDEC FW version
    Rsvd        Reserved for future use

Return:
    The Stream FW Version umbers from the FW bin file in the install directory
    
    BC_STS_SUCCESS will be returned on successful completion.

*****************************************************************************/
DRVIFLIB_API BC_STATUS 
DtsGetFWVersionFromFile(
    HANDLE  hDevice,
    U32     *StreamVer,
    U32     *DecVer,
    char    *fname
    );

/*****************************************************************************

Function name:

    DtsGetFWVersion

Description:

    Get version information from the Firmware. The version information is obtained 
    from Bin file when the flag is not set. When the flag is set, a FW command is 
    issued to get the version numbers. 
    Version numbers in FW are maintained in <Major>.<Minor>.<Spl Revision> format.
    Version number will be returned in the following format
    (Major << 16) | (Minor<<8) | Spl_rev ?012345

    The individual components of the revision number are available as follows:

    o Major     (8 Bits) : Bit 24 ?16
    o Minor     (8 Bits) : Bit 16 ?8
    o Revision (16 Bits) : Bits 8 ?Bit 0.

Parameters:

    hDevice     Handle to device. This is obtained via a prior call to
                DtsDeviceOpen
    StreamVer   Stream FW version
    DecVer      VDEC FW version
    HwVer       Hardware version 
    Rsvd        Reserved for future use
    flag        Reseved for future use

Return:
    The Stream FW Version number, VDEC FW version and Hwrev
    
    BC_STS_SUCCESS will be returned on successful completion.

*****************************************************************************/
DRVIFLIB_API BC_STATUS 
DtsGetFWVersion(
    HANDLE  hDevice,
    U32     *StreamVer,
    U32     *DecVer,
    U32     *HwVer,
    char    *fname,
    U32     flag
    );


/*****************************************************************************

Function name:

    DtsOpenDecoder

Description:
    
    Open the decoder for playback operations and sets appropriate parameters
    for decode of input video data.

    The device must have been previously opened for this call to succeed.

Parameters:

    hDevice     Handle to device. This is obtained via a prior call to
                DtsDeviceOpen.
    StreamType  Currently supported streams are:
                        Elementary Streams with no timestamp management (0)
                        Transport Streams (2)
                        Elementary Streams with timestamp management (6)
                All other values will return BC_STS_INV_ARG.

Return:

    BC_STS_SUCCESS will be returned on successful completion.

*****************************************************************************/
DRVIFLIB_API BC_STATUS 
DtsOpenDecoder(
    HANDLE  hDevice,
    U32     StreamType
    );

/*****************************************************************************

Function name:

    DtsCloseDecoder
    
Description:

    Close the decoder. No further pictures will be produced and all input
    will be ignored.

    The device must have been previously opened for this call to succeed.
    This function closes the decoder and cleans up the state of the driver
    and the library. All pending pictures will be dropped and all outstanding
    transfers to and from the decoder will be aborted.

Parameters:

    hDevice     Handle to device. This is obtained via a prior call to
                DtsDeviceOpen.

Return:

    BC_STS_SUCCESS will be returned on successful completion.

*****************************************************************************/
DRVIFLIB_API BC_STATUS 
DtsCloseDecoder(
    HANDLE  hDevice
    );

/*****************************************************************************

Function name:

    DtsStartDecoder
    
Description:

    Start the actual processing of input data. Before this command the
    decoder will ignore all of the presented input data.

    DtsOpenDecoder must always be followed by a DtsStartDecoder for the
    decoder to start processing input data. The device must have been
    previously opened for this call to succeed. In addition the video
    parameters for codec must have been set via a call to DtsSetVideoParams.

Parameters:

    hDevice     Handle to device. This is obtained via a prior call to
                DtsDeviceOpen.

Return:

    BC_STS_SUCCESS will be returned on successful completion.

*****************************************************************************/
DRVIFLIB_API BC_STATUS 
DtsStartDecoder(
    HANDLE  hDevice
    );


/*****************************************************************************

Function name:

    DtsSetVideoParams
    
Description:

    Sets various codec parameters that would be used by a subsequent call
    to DtsStartDecoder.

    DtsSetVideoParams must always be called before DtsStartDecoder for the
    decoder to start processing input data. The device must have been
    previously opened for this call to succeed.

Parameters:
    hDevice         Handle to device. This is obtained via a prior call to
                    DtsDeviceOpen.
    videoAlg        Video Codec to be enabled to decode stream.
                    H.264 (0), VC-1 (4) and MPEG-2 (1) currently supported.
                    All other values will return BC_STS_INV_ARG
    FGTEnable       Enable processing of FGT SEI.
    MetaDataEnable  Enable retrieval of picture metadata to be sent to video
                    pipeline.
    Progressive     Instruct decoder to always try to send back progressive
                    frames. If input content is 1080p, the decoder will
                    ignore pull-down flags and always give 1080p output.
                    If 1080i content is processed, the decoder will return
                    1080i data. When this flag is not set, the decoder will
                    use pull-down information in the input stream to decide
                    the decoded data format.
    OptFlags        In this field bits 0:3 are used pass default frame rate, bits 4:5 are for operation mode (used to indicate
					Blu-ray mode to the decoder) and bit 6 is for the flag mpcOutPutMaxFRate which when set tells the 
					FW to output at the max rate for the resolution and ignore the frame rate determined from the stream.
					Bit 7 is set to indicate that this is Adobe mode and the driver will be peeked to get timestamps ahead of time

Return:

    BC_STS_SUCCESS will be returned on successful completion.

*****************************************************************************/
DRVIFLIB_API BC_STATUS 
DtsSetVideoParams(
    HANDLE  hDevice,
    U32     videoAlg,
    BOOL    FGTEnable,
    BOOL    MetaDataEnable,
    BOOL    Progressive,
    U32     OptFlags
    );

/*****************************************************************************

Function name:

    DtsGetVideoParams
    
Description:

    Returns various codec parameters that would be used by a subsequent call
    to DtsStartDecoder. These parameters are either default values or were
    set via a prior call to DtsSetVideoParams

    The device must have been previously opened for this call to succeed.

Parameters:

    hDevice         Handle to device. This is obtained via a prior call to
                    DtsDeviceOpen.
    *videoAlg       See DtsSetVideoParams. [OUTPUT]
    *FGTEnable      See DtsSetVideoParams. [OUTPUT]
    *MetaDataEnable See DtsSetVideoParams. [OUTPUT]
    *Progressive    See DtsSetVideoParams. [OUTPUT]
	Reserved        This field is reserved for possible future expansion.
                    Set to 0.

Return:

    BC_STS_SUCCESS will be returned on successful completion.

*****************************************************************************/
DRVIFLIB_API BC_STATUS 
DtsGetVideoParams(
    HANDLE  hDevice,
    U32     *videoAlg,
    BOOL    *FGTEnable,
    BOOL    *MetaDataEnable,
    BOOL    *Progressive,
    U32     Reserved
    );

/*****************************************************************************

Function name:

    DtsFormatChange

Description:

    Changes codec type and parameters.

    The device must have been previously opened for this call to succeed.
    This function should be used only for mid-stream format changes.
    DtsStartDecoder must have been called before for this function to succeed.

Parameters:

    hDevice     Handle to device. This is obtained via a prior call to
                DtsDeviceOpen.
    videoAlg    Video Codec to be enabled to decode stream.
                H.264 (0), VC-1 (4) and MPEG-2 (1) currently supported. All
                other values will return BC_STS_INV_ARG
    FGTEnable   Enable processing of FGT SEI.
    Progressive Instruct decoder to always try to send back progressive
                frames. If input content is 1080p, the decoder will ignore
                pull-down flags and always give 1080p output. If 1080i
                content is processed, the decoder will return 1080i data.
                When this flag is not set, the decoder will use pull-down
                information in the input stream to decide the decoded data
                format.
    Reserved    This field is reserved for possible future expansion.
                Set to 0.

Return:

    BC_STS_SUCCESS will be returned on successful completion.

*****************************************************************************/
DRVIFLIB_API BC_STATUS 
DtsFormatChange(
    HANDLE  hDevice,
    U32     videoAlg,
    BOOL    FGTEnable,
    BOOL    MetaDataEnable,
    BOOL    Progressive,
    U32     Reserved
    );

/*****************************************************************************

Function name:

    DtsStopDecoder
    
Description:

    Stop the decoder.

    The device must have been previously opened for this call to succeed.
    This function will clean up any pending operations and stop the decoder.
    Internal state is still maintained and the decoder can be restarted.
    Any pending pictures will be dropped.

Parameters:

    hDevice     Handle to device. This is obtained via a prior call to
                DtsDeviceOpen.

Return:

    BC_STS_SUCCESS will be returned on successful completion.

*****************************************************************************/
DRVIFLIB_API BC_STATUS 
DtsStopDecoder(
    HANDLE  hDevice
    );

/*****************************************************************************

Function name:

    DtsPauseDecoder
    
Description:

    Pause the decoder. The paused picture will be repeated by decoder.

    The device must have been previously opened for this call to succeed.
    In addition the decoder must have been started as well. If the decoder
    is open but not started, this function will return BC_STS_DEC_NOT_STARTED.
    If the decoder has not been opened this function will return
    BC_STS_DEC_NOT_OPEN.

Parameters:
    
    hDevice     Handle to device. This is obtained via a prior call to
                DtsDeviceOpen.

Return:

    BC_STS_SUCCESS will be returned on successful completion.

*****************************************************************************/
DRVIFLIB_API BC_STATUS 
DtsPauseDecoder(
    HANDLE  hDevice
    );

/*****************************************************************************

Function name:

    DtsResumeDecoder
    
Description:

    Unpause the decoder from a previous paused condition.

    The device must have been previously opened for this call to succeed.
    If the decoder was not paused previously, this function will return
    without affecting the decoder with a BC_STS_SUCCESS status. If the
    decoder is open but not started, this function will return
    BC_STS_DEC_NOT_STARTED.

Parameters:

    hDevice     Handle to device. This is obtained via a prior call to
                DtsDeviceOpen.

Return:

    BC_STS_SUCCESS will be returned on successful completion.

*****************************************************************************/
DRVIFLIB_API BC_STATUS 
DtsResumeDecoder(
    HANDLE  hDevice
    );

/*****************************************************************************

Function name:

    DtsSetVideoPID
    
Description:

    Sets the video PID in the input Transport Stream that the decoder
    needs to process.

    The device must have been previously opened for this call to succeed.

Parameters:

    hDevice Handle to device. This is obtained via a prior call to
            DtsDeviceOpen.
    PID     PID value that decoder needs to process.

Return:

    BC_STS_SUCCESS will be returned on successful completion.

*****************************************************************************/
DRVIFLIB_API BC_STATUS 
DtsSetVideoPID(
    HANDLE  hDevice,
    U32     pid
    );


/*****************************************************************************

Function name:

    StartCaptureImmidiate
    
Description:

    Instruct the driver to start capturing decoded frames for output.

    The device must have been previously opened for this call to succeed.
    This function must be called before the first call to DtsProcInput.
    This function instructs the receive path in the driver to start waiting
    for valid data to be presented from the decoder.

Parameters:
    hDevice     Handle to device. This is obtained via a prior call to
                DtsDeviceOpen.

Return:

    BC_STS_SUCCESS will be returned on successful completion.

*****************************************************************************/
DRVIFLIB_API BC_STATUS 
DtsStartCaptureImmidiate(
    HANDLE  hDevice,
    U32     Reserved
    );


/*****************************************************************************

Function name:

    StartCapture
    
Description:

    Instruct the driver to start capturing decoded frames for output.

    The device must have been previously opened for this call to succeed.
    This function must be called before the first call to DtsProcInput.
    This function instructs the receive path in the driver to start waiting
    for valid data to be presented from the decoder.

Parameters:
    hDevice     Handle to device. This is obtained via a prior call to
                DtsDeviceOpen.

Return:

    BC_STS_SUCCESS will be returned on successful completion.

*****************************************************************************/
DRVIFLIB_API BC_STATUS 
DtsStartCapture(
    HANDLE  hDevice
    );

/*****************************************************************************

Function name:

    FlushRxCapture

Description:

    ***This function is deprecated and is for temporary use only.***
    
    Flush the driver’s queue of pictures and stops the capture process. These
    functions will be replaced with automatic Stop (End of Sequence) detection.

    The device must have been previously opened for this call to succeed.

Parameters:

    hDevice     Handle to device. This is obtained via a prior call to
                DtsDeviceOpen.
Return:

    BC_STS_SUCCESS will be returned on successful completion.

*****************************************************************************/
DRVIFLIB_API BC_STATUS 
DtsFlushRxCapture(
    HANDLE  hDevice,
    BOOL    bDiscardOnly
    );

/*****************************************************************************

Function name:

    DtsProcOutput
    
Description:

    Returns one decoded picture to the caller.

    The device must have been previously opened for this call to succeed.

    == NOTE ====
        For PIB AND 100% output encryption/scrambling on Bcm LINK hardware
    use ProcOutputNoCopy() Interace. This interface will not support 
    PIB encryption.


Parameters:

    hDevice         Handle to device. This is obtained via a prior call to
                    DtsDeviceOpen.
    milliSecWait    Timeout parameter. DtsProcOutput will fail is no picture
                    is received in this time.
    *pOut           This is a pointer to the BC_DTS_PROC_OUT structure that is
                    allocated by the caller. The decoded picture is returned
                    in this structure. This structure is described in the
                    data structures section. The actual data buffer to be
                    filled with the decoded data is allocated by the caller.
                    Data is copied from the decoder to the buffers before this
                    function returns. [INPUT/OUTPUT]

Return:

    BC_STS_SUCCESS will be returned on successful completion.

*****************************************************************************/
DRVIFLIB_API BC_STATUS 
DtsProcOutput(
    HANDLE  hDevice,
    U32     milliSecWait,
    BC_DTS_PROC_OUT *pOut
    );

/*****************************************************************************

Function name:

    DtsProcOutputNoCopy
    
Description:

    Returns one decoded picture to the caller. Functionality of this API()
    is very similar to ProcOutPut() API. This API will not copy the video data
    to caller's buffers but provides the source buffer pointers in pOut structure.

    This is more secure and preferred method for BCM's Link hardware. The actual
    format conversion/copy routines are provided as part of the Filter/Security
    layer source code. Using this method, all the clear data handling will be
    done by bcmDFilter or bcmSec layers which are expected to be in Player's 
    tamper resistant area. 

    == NOTE ====
     1) DtsReleaseOutputBuffs() interface must be called to release the buffers 
        back to DIL if return Status is BC_STS_SUCCESS.

     2) Only this interface supports PIB and full 100% output encryption/Scrambling. 


    The device must have been previously opened for this call to succeed.

Parameters:

    hDevice         Handle to device. This is obtained via a prior call to
                    DtsDeviceOpen.
    milliSecWait    Timeout parameter. DtsProcOoutput will fail is no picture
                    is received in this time.
    *pOut           This is a pointer to the BC_DTS_PROC_OUT structure that is
                    allocated by the caller. The decoded picture is returned
                    in this structure. 

Return:

    BC_STS_SUCCESS will be returned on successful completion.

*****************************************************************************/
DRVIFLIB_API BC_STATUS 
DtsProcOutputNoCopy(
    HANDLE  hDevice,
    U32     milliSecWait,
    BC_DTS_PROC_OUT *pOut
    );

/*****************************************************************************

Function name:

    DtsReleaseOutputBuffs
    
Description:

    Release Buffers acquired during ProcOutputNoCopy() interface. 

Parameters:

    hDevice         Handle to device. This is obtained via a prior call to
                    DtsDeviceOpen.
    Reserved        Reserved. Set to NULL.

    fChange         FALSE.

Return:

    BC_STS_SUCCESS will be returned on successful completion.

*****************************************************************************/
DRVIFLIB_API BC_STATUS 
DtsReleaseOutputBuffs(
    HANDLE  hDevice,
    PVOID   Reserved,
    BOOL    fChange
    );


/*****************************************************************************

Function name:

    DtsProcInput

Description:

    Sends compressed (coded) data to the decoder for processing.

    The device must have been previously opened for this call to succeed.
    In addition, suitable keys must have been exchanged for decryption and
    decode to be successful.

Parameters:

    hDevice     Handle to device. This is obtained via a prior call to
                DtsDeviceOpen.
    pUserData   Pointer to data buffer that holds the data to be transferred.
                [INPUT]
    sizeInBytes Size in Bytes of data available to be sent to the decoder for
                processing.
    Timestamp   Optional timestamp information attached to the media sample
                that is available in the buffer. If timestamp is present
                (i.e. non-zero), then this will be reflected in the output
                sample (picture) produced from the contents of this buffer.
				Timestamp should be in units of 100 ns.
    Encrypted   Flag to indicate that the data transfer is not in the clear
                and that the decoder needs to decrypt before it can decode
                the data.  Note that due to complexity, it is preferred that
                the application writer uses the higher level
                dts_pre_proc_input() call if encypted content will be sent.

Return:

    BC_STS_SUCCESS will be returned on successful completion.

*****************************************************************************/
DRVIFLIB_API BC_STATUS
DtsProcInput(
    HANDLE  hDevice,
    U8      *pUserData,
    U32     ulSizeInBytes,
    U64     timeStamp,
    BOOL    encrypted
    );

/*****************************************************************************

Function name:

    DtsGetColorPrimaries

Description:

    Returns color primaries information from the stream being processed.

    The device must have been previously opened for this call to succeed.
    In addition at least one picture must have been successfully decoded and
    returned back from the decoder.

Parameters:

    hDevice         Handle to device. This is obtained via a prior call to
                    DtsDeviceOpen.
    colorPrimaries  Pointer to U32 to receive the color primaries information.
                    The values returned are described in the previous section
                    regarding the picture metadata. [OUTPUT]

Return:

    BC_STS_SUCCESS will be returned on successful completion.

*****************************************************************************/
DRVIFLIB_API BC_STATUS
DtsGetColorPrimaries(
    HANDLE  hDevice,
    U32     *colorPrimaries
    );

/*****************************************************************************

Function name:

    DtsFlushInput

Description:

    Flushes the current channel and causes the decoder to stop accessing input
    data.  Based on the flush mode parameter, the channel will be flushed from
    the current point in the input data or from the current processing point.

    The device must have been previously opened for this call to succeed.

Parameters:

    hDevice         Handle to device. This is obtained via a prior call to
                    DtsDeviceOpen.
    Mode            0   Flush at the current input point. use to drain the
                        input FIFO . All the data that has been received will
                        be decoded.
                    1   Flush at the current processing point. All the decoded
                        frames will be presented but no more data from the
                        input will be decoded.
                    2   Flushes all the decoder buffers, input, decoded and
                        to be decoded.
                    3   Cancels the pending TX Request from the DIL/driver
					4	Flushes all the decoder buffers, input, decoded and
						to be decoded data. Also flushes the drivers buffers

Return:

    BC_STS_SUCCESS will be returned on successful completion.

*****************************************************************************/
DRVIFLIB_API BC_STATUS 
DtsFlushInput(
    HANDLE  hDevice,
    U32     Mode
    );

/*****************************************************************************

Function name:

    DtsGetRegConfig

Description:

    Read the "DbgOptions" registry key and return it's value.

    The device must have been previously opened for this call to succeed.

Parameters:

    hDevice         Handle to device. This is obtained via a prior call to
                    DtsDeviceOpen.
    *cfg            Pointer to BC_REG_CONFIG to receive registry key value.

Return:

    BC_STS_SUCCESS will be returned on successful completion.

*****************************************************************************/
DRVIFLIB_API BC_STATUS 
DtsGetRegConfig(
    HANDLE          hDevice,
    BC_REG_CONFIG   *cfg
    );

/*****************************************************************************

Function name:

    DtsSetRegConfig

Description:

    Set DbgOptions.

    The device must have been previously opened for this call to succeed.

Parameters:

    hDevice         Handle to device. This is obtained via a prior call to
                    DtsDeviceOpen.
    *cfg            Pointer to BC_REG_CONFIG to receive registry key value.

Return:

    BC_STS_SUCCESS will be returned on successful completion.

*****************************************************************************/
DRVIFLIB_API BC_STATUS 
DtsSetRegConfig(
    HANDLE          hDevice,
    BC_REG_CONFIG   *cfg
    );

/*****************************************************************************

Function name:

    DtsSetRateChange

Description:

    Sets the decoder playback speed and direction of playback.

    The device must have been previously opened for this call to succeed.

Parameters:

    hDevice         Handle to device. This is obtained via a prior call to
                    DtsDeviceOpen.
    rate            Inverse of speed x 10000.
                    Examples:
                        1/2x playback speed = 20000
                        1x   playback speed = 10000
                        2x   playback speed = 5000

    direction       Playback direction.
                    0   Forward direction.
                    1   Reverse direction.

Return:

    BC_STS_SUCCESS will be returned on successful completion.

*****************************************************************************/
DRVIFLIB_API BC_STATUS 
DtsSetRateChange(
    HANDLE  hDevice,
    U32     rate,
    U8      direction
    );

//Set FF Rate for Catching Up
/*****************************************************************************

Function name:

    DtsSetFFRate

Description:

    Sets the decoder playback FF speed

    The device must have been previously opened for this call to succeed.

Parameters:

    hDevice         Handle to device. This is obtained via a prior call to
                    DtsDeviceOpen.
    rate            Inverse of speed x 10000.
                    Examples:
                        1/2x playback speed = 20000
                        1x   playback speed = 10000
                        2x   playback speed = 5000

Return:

    BC_STS_SUCCESS will be returned on successful completion.

*****************************************************************************/
DRVIFLIB_API BC_STATUS 
DtsSetFFRate(
    HANDLE  hDevice,
    U32     rate
    );

/*****************************************************************************

Function name:

    DtsSetSkipPictureMode

Description:

    This command sets the decoder to only decode selected picture types.

    The device must have been previously opened for this call to succeed.

Parameters:

    hDevice         Handle to device. This is obtained via a prior call to
                    DtsDeviceOpen.   

    SkipMode        0   IPB, All pictures are decoded.

                    1   IP decoding, This mode skips all non reference pictures.

                    2   I decoding, This mode skips all P/B pictures and only decodes 
                        I pictures.
Return:

    BC_STS_SUCCESS will be returned on successful completion.

*****************************************************************************/
DRVIFLIB_API BC_STATUS 
DtsSetSkipPictureMode(
    HANDLE hDevice,
    U32 SkipMode
    );

/*****************************************************************************

Function name:

    DtsSetIFrameTrickMode

Description:

    This command sets the decoder to decode only I Frames for FF and FR.

    Use this API for I Frame only trick mode play back in either direction. The 
    application/Up stream filter  determines the speed of the playback by 
    means of Skip on the input compressed data.

    The device must have been previously opened for this call to succeed.

Parameters:

    hDevice         Handle to device. This is obtained via a prior call to
                    DtsDeviceOpen.   

Return:

    BC_STS_SUCCESS will be returned on successful completion.

*****************************************************************************/
DRVIFLIB_API BC_STATUS 
DtsSetIFrameTrickMode( 
    HANDLE  hDevice
    ); 

/*****************************************************************************

Function name:

    DtsStepDecoder
    
Description:

    This function forwards one frame.

    The device must have been opened must be in paused 
    state previously for this call to succeed.

Parameters:

    hDevice     Handle to device. This is obtained via a prior call to
                DtsDeviceOpen.

Return:

    BC_STS_SUCCESS will be returned on successful completion.

*****************************************************************************/
DRVIFLIB_API BC_STATUS 
DtsStepDecoder(
    HANDLE  hDevice
    );

/*****************************************************************************

Function name:

    DtsPassContentKeys

Description:

    Pass encryption related key information to HW.

    The device must have been previously opened for this call to succeed.

Parameters:

    hDevice         Handle to device. This is obtained via a prior call to
                    DtsDeviceOpen.
    buff            Encrypted buffer with data.
    len             Length of the buffer.

Return:

    BC_STS_SUCCESS will be returned on successful completion.

*****************************************************************************/
DRVIFLIB_API BC_STATUS 
DtsPassContentKeys(
    HANDLE  hDevice,
    U8      *buff,
    U32     len,
    U32     flags
    );

/*****************************************************************************

Function name:

    DtsPassSessionKey

Description:

    Pass encryption related key information to HW.

    The device must have been previously opened for this call to succeed.

Parameters:

    hDevice         Handle to device. This is obtained via a prior call to
                    DtsDeviceOpen.
    buff            Encrypted buffer with data.
    len             Length of the buffer.

Return:

    BC_STS_SUCCESS will be returned on successful completion.

*****************************************************************************/
DRVIFLIB_API BC_STATUS 
DtsPassSessionKey(
    HANDLE  hDevice,
    U8      *buff,
    U32     len,
    U32     flags
    );

/*****************************************************************************

Function name:

    DtsIs422Supported
    
Description:

    This function returns whether 422 YUV mode is supported or not.

    The device must have been opened previously for this call to succeed.

Parameters:

    hDevice     Handle to device. This is obtained via a prior call to
                DtsDeviceOpen.
    bSupported  1 - 422 is supported
                0 - 422 is not supported.

Return:

    BC_STS_SUCCESS will be returned on successful completion.

*****************************************************************************/
DRVIFLIB_API BC_STATUS 
DtsIs422Supported(
    HANDLE  hDevice,
    U8*     bSupported
    );

/*****************************************************************************

Function name:

    DtsSet422Mode
    
Description:

    This function sets the 422 mode to either YUY2 or UYVY.

    The device must have been opened previously and must support 422 mode for
    this call to succeed.

    Use "DtsIs422Supported" to find whether 422 mode is supported.

Parameters:

    hDevice     Handle to device. This is obtained via a prior call to
                DtsDeviceOpen.
    422Mode     0 - set the YUV mode to YUY2
                1 - set the YUV mode to UYVY

Return:

    BC_STS_SUCCESS will be returned on successful completion.

*****************************************************************************/
DRVIFLIB_API BC_STATUS 
DtsSet422Mode(
    HANDLE  hDevice,
    U8      Mode422
    );

/*****************************************************************************

Function name:

    DtsGetDILPath
    
Description:

    This is a helper function to return DIL's Path.

    The device must have been previously opened for this call to succeed.

Parameters:

    hDevice     Handle to device. This is obtained via a prior call to
                DtsDeviceOpen.

    DilPath     Buffer to hold DIL path info upto 256 bytes.

Return:

    BC_STS_SUCCESS will be returned on successful completion.

*****************************************************************************/

DRVIFLIB_API BC_STATUS 
DtsGetDILPath(
    HANDLE  hDevice,
    S8      *DilPath,
    U32     size
    );

/*****************************************************************************

Function name:

    DtsGetX509Cert
    
Description:

    This function returns X509 certificate read from hardware.

    The device must have been previously opened for this call to succeed.

    This API returns BC_STS_INSUFF_RES if the buffer sizes provided are not
    sufficient. When this status is returned, the cert_len and sig_len fields
    will specify the actual buffer sizes needed.

    If returned Status is BC_STS_SUCCESS, cert_len and sig_len are updated with
    actual data buffer used.

Parameters:

    hDevice     Handle to device. This is obtained via a prior call to
                DtsDeviceOpen.

    cert_buff   Buffer to hold x509 certificate.
    
    cert_len    Lenght of the cert buffer.

    sig         Buffer to hold signature.

    sig_len     Length of the signature buffer. 

    ser_num     Optional Serial number (8 bytes) for house keeping.

    flags       Optional flags field.

Return:

    BC_STS_SUCCESS will be returned on successful completion.

*****************************************************************************/
DRVIFLIB_API BC_STATUS 
DtsGetX509Cert(
    HANDLE  hDevice,
    U8      *cert_buff,
    U32     *cert_len,
    U8      *sig,
    U32     *sig_len,
    U8      *serial_num,
    U32     flags
    );

/*****************************************************************************

Function name:

    DtsDropPictures

Description:

    This command sets the decoder to skip one or more non-reference (B) pictures
    in the input data stream.  This is used for when the audio is ahead of
    video and the application needs to cause video to move ahead to catch up.
    Reference pictures are not skipped.

    The device must have been previously opened for this call to succeed.

Parameters:

    hDevice         Handle to device. This is obtained via a prior call to
                    DtsDeviceOpen.   

    Pictures        The number of non-reference pictures to drop.

Return:

    BC_STS_SUCCESS will be returned on successful completion.

*****************************************************************************/
DRVIFLIB_API BC_STATUS 
DtsDropPictures(
    HANDLE  hDevice,
    U32     Pictures
    );

/*****************************************************************************

Function name:

    DtsGetDriverStatus

Description:

    This command returns various statistics related to the driver and DIL.

    The device must have been previously opened for this call to succeed.

Parameters:

    hDevice         Handle to device. This is obtained via a prior call to
                    DtsDeviceOpen.   

    *pStatus        Pointer to BC_DTS_STATUS to receive driver status.

Return:

    BC_STS_SUCCESS will be returned on successful completion.

*****************************************************************************/
DRVIFLIB_API BC_STATUS 
DtsGetDriverStatus(
    HANDLE          hDevice,
	BC_DTS_STATUS   *pStatus
    );


#ifdef __cplusplus
}
#endif

#endif
