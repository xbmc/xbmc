#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <semaphore.h>
#include "bc_dts_types.h"
#include "bc_ldil_if.h"
#include <iostream>
#include <fstream>
#include <sys/shm.h>

#define TRY_CALL_1(func, p1, errmsg) \
  if (BC_STS_SUCCESS != func(p1)) \
    throw errmsg;

#define TRY_CALL_2(func, p1, p2, errmsg) \
  if (BC_STS_SUCCESS != func(p1, p2)) \
    throw errmsg;

#define TRY_CALL_5(func, p1, p2, p3, p4, p5, errmsg) \
  if (BC_STS_SUCCESS != func(p1, p2, p3, p4, p5)) \
    throw errmsg;

#define TRY_CALL_6(func, p1, p2, p3, p4, p5, p6, errmsg) \
  if (BC_STS_SUCCESS != func(p1, p2, p3, p4, p5, p6)) \
    throw errmsg;

#define OUTPUT_PROC_TIMEOUT 2000

int main()
{
  BC_STATUS ret;
  HANDLE device = 0;
  std::fstream inFile;
  try
  {
    printf("starting up\n");
    // Initialize the Link and Decoder devices
    U32 mode = DTS_PLAYBACK_MODE | DTS_LOAD_FILE_PLAY_FW | DTS_SKIP_TX_CHK_CPB | DTS_DFLT_RESOLUTION(vdecRESOLUTION_720p29_97);
    ret = DtsDeviceOpen(&device, mode);
    if (ret != BC_STS_SUCCESS) {
      printf("crap, DtsDeviceOpen failed\n");
      throw "Failed to open device";
    }
    ret = DtsOpenDecoder(device, BC_STREAM_TYPE_ES);
    if (ret != BC_STS_SUCCESS) {
      printf("crap, DtsOpenDecoder failed\n");
      throw "Failed to open decoder";
    }
    ret = DtsSetVideoParams(device, BC_VID_ALGO_H264, FALSE, FALSE, TRUE, 0x80000000 | vdecFrameRate23_97);
    if (ret != BC_STS_SUCCESS) {
      printf("crap, DtsSetVideoParams failed\n");
      throw "Failed to set video params";
    }
    ret = DtsSet422Mode(device, MODE422_YUY2);
    if (ret != BC_STS_SUCCESS) {
      printf("crap, DtsSet422Mode failed\n");
      throw "Failed to set 422 mode";
    }
    ret = DtsStartDecoder(device);
    if (ret != BC_STS_SUCCESS) {
      printf("crap, DtsStartDecoder failed\n");
      throw "Failed to start decoder";
    }
    ret = DtsStartCapture(device);
    if (ret != BC_STS_SUCCESS) {
      printf("crap, DtsStartCapture failed\n");
      throw "Failed to start capture";
    }
#if 0
    TRY_CALL_2(DtsDeviceOpen, &device, mode, "Failed to open device");
    TRY_CALL_2(DtsOpenDecoder, device, BC_STREAM_TYPE_ES, "Failed to open decoder");
    TRY_CALL_6(DtsSetVideoParams, device, BC_VID_ALGO_H264, FALSE, FALSE, TRUE, 0x80000000 | vdecFrameRate23_97, "Failed to set video params");
    TRY_CALL_2(DtsSet422Mode, device, MODE422_YUY2, "Failed to set 422 mode");
    TRY_CALL_1(DtsStartDecoder, device, "Failed to start decoder");
    TRY_CALL_1(DtsStartCapture, device, "Failed to start capture");
#endif
    printf("try calls done\n");

    // Open the input stream
    inFile.open("/home/jarod/src/bcm70012/testing/test_video.264", std::ios::in | std::ios::binary);
    if (!inFile.is_open())
      throw "Unable to open input file";
    else
      printf("file opened successfully\n");

    // Create a 4-byte aligned input buffer
    U8 oddBytes = 0;
    U32 inputLen = 32768;
    U8* input = (U8*)malloc(inputLen+4);
    printf("Input Buffer: %p\n", input);
    if(((uintptr_t)input)%4)
      oddBytes = 4 - ((U8)((uintptr_t)input % 4));
    U8* input_aligned = input + oddBytes;
    printf("Aligned Input Buffer: %p, Offset = %d\n", input_aligned, oddBytes);

    // Create a 4-byte aligned output buffer
    U32 ysize = 4147200; // 1920 x 1080
    U32 uvsize = 0;
    U8* rawBuf =  (U8*)malloc(ysize + uvsize + 4);
    U8* alignedBuf = rawBuf;
    if(((uintptr_t)rawBuf)%4)
    {
      oddBytes = 4 - ((U8)((uintptr_t)rawBuf % 4));
      alignedBuf = rawBuf + oddBytes;
      printf("Aligned Buffer: %p, Offset = %d\n", alignedBuf, oddBytes);
    }

    // If UV is in use, it's data immediately follows Y
    U8* ybuf = alignedBuf;
    printf("Y Buffer: %p\n", ybuf);
    U8* uvbuf = NULL;
    if (uvsize)
    {
      uvbuf = alignedBuf + ysize;
      printf("UV Buffer: %p\n", ybuf);
    }

    bool needData = true;
    U32 bytesRead = 0;
    bool formatChanged = false;

    // Open the output stream
    //std::fstream outFile;
    //outFile.open("/home/davilla/dozer/dump.yuv", std::ios::binary | std::ios::out);
    U32 chunksSent = 0;
    U32 bytesSent = 0;
    U32 picsDecoded = 0;
    U32 lastDecoded = 0xFF;
    for (;;)
    {
      for (int i = 0; i < 2; i++)
      {
        // Read from input file if previously-read data was sent successfully
        if (needData)
        {
          inFile.read((char*)input, inputLen);
          if (inFile.fail())
          {
            printf("Read %d pictures\n", picsDecoded);
            throw "Unable to read input file";
          }
          else if (inFile.eof())
            throw "Reached end of input file";

          bytesRead += inputLen;
        }

        // Push input data to driver
        ret = DtsProcInput(device, input, inputLen, 0, FALSE);
        if (ret == BC_STS_SUCCESS)
        {
          chunksSent++;
          bytesSent += inputLen;
        }
        else
          printf("DtsProcInput returned %d\n", ret);
        usleep(1000);
        needData = (ret == BC_STS_SUCCESS); // Only need more data if the send succeeded
      }

      // Prepare output structure
      BC_DTS_PROC_OUT output;
      memset(&output, 0, sizeof(BC_DTS_PROC_OUT));
      output.PicInfo.width = 1920;
      output.PicInfo.height = 1080;
      output.Ybuff = ybuf;
      output.YbuffSz = ysize/4;
      output.UVbuff = uvbuf;
      output.UVbuffSz = uvsize/4;
      output.PoutFlags = BC_POUT_FLAGS_SIZE;

      // Request decoded data from the driver
      ret = DtsProcOutput(device, OUTPUT_PROC_TIMEOUT, &output);
      if (ret == BC_STS_SUCCESS)
      {
        if (!(output.PoutFlags & BC_POUT_FLAGS_PIB_VALID))
        {
          printf("Invalid PIB received. Skipping picture. Flags: 0x%08x\n", output.PoutFlags);
          continue;
        }
        picsDecoded++;
        if (output.PicInfo.picture_number == lastDecoded)
        {
          /*BC_DTS_STATUS stat;
          if (BC_STS_SUCCESS == DtsGetDriverStatus(device, &stat))
          {
            printf("Driver Status\n-------------------\n", stat.ReadyListCount);
            printf("ReadyListCount: %u\n", stat.ReadyListCount);
            printf("FreeListCount: %u\n", stat.FreeListCount);
            printf("FramesDropped: %u\n", stat.FramesDropped);
            printf("FramesCaptured: %u\n", stat.FramesCaptured);
            printf("FramesRepeated: %u\n", stat.FramesRepeated);
            printf("InputCount: %u (ChunksSent: %u)\n", stat.ReadyListCount, chunksSent);
            printf("InputTotalSize: %llu (BytesSent: %u)\n", stat.InputTotalSize, bytesSent);
            printf("InputBusyCount: %u\n", stat.InputBusyCount);
            printf("PIBMissCount: %u\n", stat.PIBMissCount);
          }*/
          continue;
        }
        lastDecoded = output.PicInfo.picture_number;
        printf("Received Output. Bytes In: %d, Y: %d, UV: %d, Number: %d, H: %d, W: %d, Flags: 0x%08x\n", bytesSent, output.YBuffDoneSz, output.UVBuffDoneSz, output.PicInfo.picture_number, output.PicInfo.height, output.PicInfo.width, output.PoutFlags);
/*
        std::fstream picFile;
        char picName[255];
        sprintf(picName, "/home/davilla/dozer/frames/picture_%d.yuv", picsDecoded);
        picFile.open(picName, std::ios::binary | std::ios::out);
        picFile.write((const char*)output.Ybuff, ysize);
        output.PicInfo.picture_number -= 3; // Adjust for start-up pictures
        picFile.close();
        //outFile.write((const char*)output.Ybuff, ysize);
*/
      }
      else if (ret == BC_STS_FMT_CHANGE)
      {
        printf("Format Change Detected. Flags: 0x%08x\n", output.PoutFlags);
        if ((output.PoutFlags & BC_POUT_FLAGS_PIB_VALID) && (output.PoutFlags & BC_POUT_FLAGS_FMT_CHANGE))
        {
          // Read format data from driver
          printf("New Format\n----------------------------------\n");
          printf("\tTimeStamp: %llu\n", output.PicInfo.timeStamp);
          printf("\tPicture Number: %lu\n", output.PicInfo.picture_number);
          printf("\tWidth: %lu\n", output.PicInfo.width);
          printf("\tHeight: %lu\n", output.PicInfo.height);
          printf("\tChroma: 0x%03x\n", output.PicInfo.chroma_format);
          printf("\tPulldown: %lu\n", output.PicInfo.pulldown);
          printf("\tFlags: 0x%08x\n", output.PicInfo.flags);
          printf("\tFrame Rate/Res: %lu\n", output.PicInfo.frame_rate);
          printf("\tAspect Ratio: %lu\n", output.PicInfo.aspect_ratio);
          printf("\tColor Primaries: %lu\n", output.PicInfo.colour_primaries);
          printf("\tMetaData: %lu\n", output.PicInfo.picture_meta_payload);
          printf("\tSession Number: %lu\n", output.PicInfo.sess_num);
          printf("\tTimeStamp: %lu\n", output.PicInfo.ycom);
          printf("\tCustom Aspect: %lu\n", output.PicInfo.custom_aspect_ratio_width_height);
          printf("\tFrames to Drop: %lu\n", output.PicInfo.n_drop);
          printf("\tH264 Valid Fields: 0x%08x\n", output.PicInfo.other.h264.valid);
        }
        // TODO: Handle change
      }
      else if (ret == BC_STS_TIMEOUT)
      {
        printf("Timeout in DtsProcOutput. Accum Bytes: %d\n", bytesRead);
      }
      else if (ret == BC_STS_IO_XFR_ERROR)
      {
        printf("I/O Transfer Error.\n");
      }
      else if (ret == BC_STS_IO_ERROR)
      {
        printf("I/O Error.\n");
      }
      else if (ret == BC_STS_BUSY)
      {
        printf("Busy.\n");
      }
      else
      {
        printf("DtsProcOutput return an unknown status: %d.\n", ret);
        return 0;
      }
    }
  }
  catch(char* msg)
  {
    printf("%s\n", msg);
  }
  catch (...)
  {
    printf("An unknown exception was thrown\n");
  }

  inFile.close();
  DtsStopDecoder(device);
  DtsCloseDecoder(device);
  DtsDeviceClose(device);
  return 0;
}


