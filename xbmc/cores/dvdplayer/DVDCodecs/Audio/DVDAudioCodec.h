
#pragma once

enum CodecID;
struct AVStream;

class CDVDAudioCodec
{
public:

  CDVDAudioCodec() {}
  virtual ~CDVDAudioCodec() {}
   
  /*
   * Open the decoder, returns true on success
   */
  virtual bool Open(CodecID codecID, int iChannels, int iSampleRate, int iBits) = 0;
  
  /*
   * Dispose, Free all resources
   */
  virtual void Dispose() = 0;
  
  /*
   * returns bytes used or -1 on error
   * 
   */
  virtual int Decode(BYTE* pData, int iSize) = 0;
  
  /*
   * returns nr of bytes used or -1 on error
   * the data is valid until the next Decode call
   */
  virtual int GetData(BYTE** dst) = 0;
  
  /*
   * resets the decoder
   */
  virtual void Reset() = 0;
  
  /*
   * returns the nr of channels for the decoded audio stream
   */
  virtual int GetChannels() = 0;
  
  /*
   * returns the samplerate for the decoded audio stream
   */
  virtual int GetSampleRate() = 0;
  
  /*
   * returns the bitspersample for the decoded audio stream (eg 16 bits)
   */
  virtual int GetBitsPerSample() = 0;
  
  /*
   * returns if the codec requests to use passtrough
   */
  virtual bool NeedPasstrough() { return false; }
};
