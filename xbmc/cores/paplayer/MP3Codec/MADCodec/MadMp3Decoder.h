#ifndef _MadMp3Decoder_H_
#define _MadMp3Decoder_H_

#ifdef _LINUX
#include <unistd.h>
#define __declspec(x)
#define __cdecl
#endif

#include <mad.h>
#include "dec_if.h"

enum madx_sig {
	ERROR_OCCURED,
	MORE_INPUT,
	FLUSH_BUFFER,
	CALL_AGAIN,
  SKIP_FRAME
};

#ifdef _LINUX
#include <strings.h>

static void ZeroMemory(void* d, size_t l)
{
  bzero(d, l);
}
#endif


class MadMp3Decoder : public IAudioDecoder
{
public:
  MadMp3Decoder();
  virtual ~MadMp3Decoder();

  // returns -1 on error, 0 on success (done with data in 'in'), 1 on success
  // but to pass 'in' again next time around.
  virtual int decode(void *in, int in_len, 
                      void *out, int *out_len, // out_len is read and written to
                      unsigned int out_fmt[8]); // out_fmt is written to
                                                  // ex: 'PCM ', srate, nch, bps
                                                  // or 'NONE' :)
  virtual void flush();
private:

  struct madx_house {
    struct mad_stream stream;
    struct mad_frame  frame;
    struct mad_synth  synth;
    mad_timer_t       timer;
    unsigned long     frame_cnt;
    unsigned char*    output_ptr;
  };

  struct madx_stat {
    size_t 		     write_size;
    size_t		     readsize;
    size_t		     remaining;
    size_t 		     framepcmsize;
    bool           flushed;
  };

  int madx_init (madx_house *mxhouse );
  madx_sig madx_read (	unsigned char *in_buffer, unsigned char *out_buffer, madx_house *mxhouse, madx_stat *mxstat, int maxwrite, bool discard = false);
  void madx_deinit( madx_house *mxhouse );


  unsigned char* m_InputBufferInt;
  unsigned char* m_InputBufferRemain;
  int m_BytesDecoded;
  bool m_HaveData;
  unsigned int m_formatdata[8];
  unsigned char  flushcnt;

  madx_house mxhouse;
  madx_stat  mxstat;
  madx_sig   mxsig;
};

extern "C" __declspec(dllexport) IAudioDecoder* __cdecl CreateAudioDecoder(unsigned int, IAudioOutput **);

#endif
