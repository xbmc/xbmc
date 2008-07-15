/****************************************************************************

XboxMediaCenter MadMp3Decoder, September 2005 by chadoe

Parts based on madxlib (c) 2005 J. A. Robson which was based on the madlld/madlldlib sources. Madlld comments have been
preserved and noted.


Original Licensing [from madlld.c] (c) 2001--2004 Bertrand Petit							
																			
	Redistribution and use in source and binary forms, with or without		
	modification, are permitted provided that the following conditions		
	are met:																	
																			
	1. Redistributions of source code must retain the above copyright		
	notice, this list of conditions and the following disclaimer.			
																			
	2. Redistributions in binary form must reproduce the above				
	copyright notice, this list of conditions and the following			
	disclaimer in the documentation and/or other materials provided		
	with the distribution.												
																			
	3. Neither the name of the author nor the names of its contributors		
	may be used to endorse or promote products derived from this			
	software without specific prior written permission.					
																			
	THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS''		
	AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED		
	TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A			
	PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR		
	CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,				
	SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT			
	LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF			
	USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND		
	ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,		
	OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT		
	OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF		
	SUCH DAMAGE.																

****************************************************************************/

#include "MadMp3Decoder.h"
#include "stdafx.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

#define BITSPERSAMPLE	32

static inline float mad_scale_float(mad_fixed_t sample) {
  return (float)(sample/(float)(1L << MAD_F_FRACBITS));
}

MadMp3Decoder::MadMp3Decoder()
{
  ZeroMemory(&mxhouse, sizeof(madx_house));
  ZeroMemory(&mxstat,  sizeof(madx_stat));
  
  m_InputBufferInt    = NULL;
  m_InputBufferRemain = NULL;
  m_HaveData = false;
  flushcnt = 0;
  madx_init(&mxhouse);
}

MadMp3Decoder::~MadMp3Decoder()
{
  madx_deinit(&mxhouse);
  if (m_InputBufferInt)
  {
    delete[] m_InputBufferInt;
    m_InputBufferInt = NULL;
  }
  if (m_InputBufferRemain)
  {
    delete[] m_InputBufferRemain;
    m_InputBufferRemain = NULL;
  }
}

int MadMp3Decoder::decode(
  void        *in,
  int          in_len,
  void        *out,
  int         *out_len, // out_len is read and written to
  unsigned int out_fmt[8]
) {
  unsigned char* input  = (unsigned char*)in;
  unsigned char* output = (unsigned char*)out;
  mxstat.readsize = 0;
  if (!m_HaveData)
  {
    mxstat.readsize = in_len + mxstat.remaining;
    if (m_InputBufferInt)
    {
      delete[] m_InputBufferInt;
      m_InputBufferInt = NULL;
    }
    m_InputBufferInt = new unsigned char[mxstat.readsize];
    if (mxstat.remaining)
    {
      memcpy(m_InputBufferInt, m_InputBufferRemain, mxstat.remaining);
    }
    memcpy(m_InputBufferInt + mxstat.remaining, input, in_len);
    mxstat.remaining = 0;
		mad_stream_buffer( &mxhouse.stream, m_InputBufferInt, (unsigned long)mxstat.readsize );
		mxhouse.stream.error = (mad_error)0;
    mad_stream_sync(&mxhouse.stream);
    if ((mxstat.flushed) && (flushcnt == 2))
    {
      int skip;

      skip = 2;
      do {
	      if (mad_frame_decode(&mxhouse.frame, &mxhouse.stream) == 0) {
	        if (--skip == 0)
	          mad_synth_frame(&mxhouse.synth, &mxhouse.frame);
	      }
	      else if (!MAD_RECOVERABLE(mxhouse.stream.error))
	        break;
      }
      while (skip);
      mxstat.flushed = false;
    }
  }
  int maxtowrite = *out_len;
  *out_len = 0;
  mxsig = ERROR_OCCURED;
  while ((mxsig != FLUSH_BUFFER) && (*out_len + mxstat.framepcmsize < (size_t)maxtowrite))
  {
    mxsig = madx_read(m_InputBufferInt, output + *out_len, &mxhouse, &mxstat, maxtowrite);
    switch (mxsig)
    {
    case ERROR_OCCURED: 
      *out_len = 0;
      m_HaveData = false;
      return -1;
    case MORE_INPUT: 
      //store remaining bytes for next input....
      if (m_InputBufferRemain)
      {
        delete[] m_InputBufferRemain;
        m_InputBufferRemain = NULL;
      }
      if (mxstat.remaining > 0)
      {
        m_InputBufferRemain = new unsigned char[mxstat.remaining];
        memcpy(m_InputBufferRemain, mxhouse.stream.next_frame, mxstat.remaining);
      }
      m_HaveData = false;
      return 0;
    case FLUSH_BUFFER:
      out_fmt[2] = mxhouse.synth.pcm.channels;
      out_fmt[1] = mxhouse.synth.pcm.samplerate;
      out_fmt[3] = BITSPERSAMPLE;
      out_fmt[4] = mxhouse.frame.header.bitrate;
      *out_len += (int)mxstat.write_size;
      mxstat.write_size = 0;
      break;
    }
  }
  if (!mxhouse.stream.next_frame || (mxhouse.stream.bufend - mxhouse.stream.next_frame <= 0))
  {
    m_HaveData = false;
    return 0;
  }
  m_HaveData = true;
  return 1;
}

void MadMp3Decoder::flush()
{
	mad_frame_mute(&mxhouse.frame);
	mad_synth_mute(&mxhouse.synth);
	mad_stream_finish(&mxhouse.stream);
	mad_stream_init(&mxhouse.stream);
  ZeroMemory(&mxstat, sizeof(madx_stat)); 
  mxstat.flushed = true;
  if (flushcnt < 2) flushcnt++;
  m_HaveData = false;
  
  if (m_InputBufferInt)
  {
    delete[] m_InputBufferInt;
    m_InputBufferInt = NULL;
  }
  if (m_InputBufferRemain)
  {
    delete[] m_InputBufferRemain;
    m_InputBufferRemain = NULL;
  }
}

int MadMp3Decoder::madx_init (madx_house *mxhouse )
{
	// Initialize libmad structures 
	mad_stream_init(&mxhouse->stream);
  mxhouse->stream.options = MAD_OPTION_IGNORECRC;
	mad_frame_init(&mxhouse->frame);
	mad_synth_init(&mxhouse->synth);
	mad_timer_reset(&mxhouse->timer);	

	return(1);
}

madx_sig MadMp3Decoder::madx_read (	unsigned char *in_buffer, unsigned char *out_buffer, madx_house *mxhouse, madx_stat *mxstat, int maxwrite, bool discard)
{
	mxhouse->output_ptr = out_buffer;

	if( mad_frame_decode(&mxhouse->frame, &mxhouse->stream) )
	{
		if( !MAD_RECOVERABLE(mxhouse->stream.error) )
		{
			if( mxhouse->stream.error == MAD_ERROR_BUFLEN )
			{		
				//printf("Need more input (%s)",	mad_stream_errorstr(&mxhouse->stream));
				mxstat->remaining = mxhouse->stream.bufend - mxhouse->stream.next_frame;

				return(MORE_INPUT);			
			}
			else			// Error returned
			{
				printf("(MAD)Unrecoverable frame level error (%s).", mad_stream_errorstr(&mxhouse->stream));
				return(ERROR_OCCURED); 
			}
		}
    //this error also happens at eof as it can't find the next frame.
		//printf("(MAD)Recoverable frame level error (%s)",	mad_stream_errorstr(&mxhouse->stream));
    return(SKIP_FRAME); 
	}

	mad_synth_frame( &mxhouse->synth, &mxhouse->frame );
	
  mxstat->framepcmsize = mxhouse->synth.pcm.length * mxhouse->synth.pcm.channels * (int)BITSPERSAMPLE/8;
	mxhouse->frame_cnt++;
	mad_timer_add( &mxhouse->timer, mxhouse->frame.header.duration );
  float *data_f = (float *)mxhouse->output_ptr;
  if (!discard)
  {
    for( int i=0; i < mxhouse->synth.pcm.length; i++ )
	  {
		  // Left channel
      *data_f++ = mad_scale_float(mxhouse->synth.pcm.samples[0][i]);
      mxhouse->output_ptr += sizeof(float);
      // Right channel
		  if(MAD_NCHANNELS(&mxhouse->frame.header)==2)
      {
        *data_f++ = mad_scale_float(mxhouse->synth.pcm.samples[1][i]);
        mxhouse->output_ptr += sizeof(float);
      }
	  }
    // Tell calling code buffer size
    mxstat->write_size = mxhouse->output_ptr - out_buffer;
  }
	return(FLUSH_BUFFER);
}

void MadMp3Decoder::madx_deinit( madx_house *mxhouse )
{
	mad_synth_finish(&mxhouse->synth);
	mad_frame_finish(&mxhouse->frame);
	mad_stream_finish(&mxhouse->stream);
}


extern "C" __declspec(dllexport) IAudioDecoder* __cdecl CreateAudioDecoder(unsigned int, IAudioOutput **)
{
  return new MadMp3Decoder();
}
