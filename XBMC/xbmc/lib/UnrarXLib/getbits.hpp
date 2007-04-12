#ifndef _RAR_GETBITS_
#define _RAR_GETBITS_

class BitInput
{
  public:
    enum BufferSize {MAX_SIZE=0x8000};
  protected:
    int InAddr,InBit;
  public:
    BitInput();
    ~BitInput();

    byte *InBuf;

    void InitBitInput()
    {
      InAddr=InBit=0;
    }
    void addbits(int Bits)
    {
      Bits+=InBit;
      InAddr+=Bits>>3;
      InBit=Bits&7;
    }
    unsigned int getbits()
    {
      unsigned int BitField=(uint)InBuf[InAddr] << 16;
      BitField|=(uint)InBuf[InAddr+1] << 8;
      BitField|=(uint)InBuf[InAddr+2];
      BitField >>= (8-InBit);
      return(BitField & 0xffff);
    }
    void faddbits(int Bits);
    unsigned int fgetbits();
};
#endif
