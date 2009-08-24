#ifndef BITSTREAM_H_
#define BITSTREAM_H_

class CSimpleBitstreamReader
{
public:
  CSimpleBitstreamReader(unsigned char* pData, unsigned int dataLen);
  bool ReadBit();
  char ReadChar(int bits);
  short ReadShort(int bits);
  int32_t ReadInt32(int bits);
  int64_t ReadInt64(int bits); 
  unsigned char* GetCurrentPointer();
  unsigned int GetBitOffset();
  unsigned int GetBytesLeft();
  unsigned int UpdatePosition(int bits);
protected:   
  char GetBits(char byte, int offset, int bits);
  bool ReadBits(unsigned int bits, unsigned char* pOut);

  unsigned char* m_pData;
  unsigned int m_BytesLeft;
  unsigned int m_BitOffset;
};

#endif /*BITSTREAM_H_*/
