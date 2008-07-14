/*
  Copyright (c) 2005, The Musepack Development Team
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met:

  * Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above
  copyright notice, this list of conditions and the following
  disclaimer in the documentation and/or other materials provided
  with the distribution.

  * Neither the name of the The Musepack Development Team nor the
  names of its contributors may be used to endorse or promote
  products derived from this software without specific prior
  written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
#include <assert.h>
#include <time.h>

#include <mpcdec/mpcdec.h>

/*
  The data bundle we pass around with our reader to store file
  position and size etc. 
*/
typedef struct reader_data_t {
    FILE *file;
    long size;
    mpc_bool_t seekable;
} reader_data;

/*
  Our implementations of the mpc_reader callback functions.
*/
mpc_int32_t
read_impl(void *data, void *ptr, mpc_int32_t size)
{
    reader_data *d = (reader_data *) data;
    return fread(ptr, 1, size, d->file);
}

mpc_bool_t
seek_impl(void *data, mpc_int32_t offset)
{
    reader_data *d = (reader_data *) data;
    return d->seekable ? !fseek(d->file, offset, SEEK_SET) : false;
}

mpc_int32_t
tell_impl(void *data)
{
    reader_data *d = (reader_data *) data;
    return ftell(d->file);
}

mpc_int32_t
get_size_impl(void *data)
{
    reader_data *d = (reader_data *) data;
    return d->size;
}

mpc_bool_t
canseek_impl(void *data)
{
    reader_data *d = (reader_data *) data;
    return d->seekable;
}

#define WFX_SIZE (2+2+4+4+2+2)

#ifdef MPC_FIXED_POINT
static int
shift_signed(MPC_SAMPLE_FORMAT val, int shift)
{
    if (shift > 0)
        val <<= shift;
    else if (shift < 0)
        val >>= -shift;
    return (int)val;
}
#endif

class WavWriter {
  public:
    WavWriter(FILE * p_output, unsigned p_nch, unsigned p_bps,
              unsigned p_srate)
    : m_file(p_output), m_nch(p_nch), m_bps(p_bps), m_srate(p_srate) {
        assert(m_bps == 16 || m_bps == 24);

        WriteRaw("RIFF", 4);
        WriteDword(0);          //fix this in destructor

        WriteRaw("WAVE", 4);
        WriteRaw("fmt ", 4);
        WriteDword(WFX_SIZE);

        WriteWord(1);           //WAVE_FORMAT_PCM
        WriteWord(m_nch);
        WriteDword(m_srate);
        WriteDword(m_srate * m_nch * (m_bps >> 3));
        WriteWord(m_nch * (m_bps >> 3));
        WriteWord(m_bps);
        /*
           WORD  wFormatTag; 
           WORD  nChannels; 
           DWORD nSamplesPerSec; 
           DWORD nAvgBytesPerSec; 
           WORD  nBlockAlign; 
           WORD  wBitsPerSample; 
         */
        WriteRaw("data", 4);
        WriteDword(0);          //fix this in destructor

        m_data_bytes_written = 0;
    } mpc_bool_t WriteSamples(const MPC_SAMPLE_FORMAT * p_buffer, unsigned p_size) {
        unsigned n;
        int clip_min = -1 << (m_bps - 1),
            clip_max = (1 << (m_bps - 1)) - 1, float_scale = 1 << (m_bps - 1);
        for (n = 0; n < p_size; n++) {
            int val;
#ifdef MPC_FIXED_POINT
            val =
                shift_signed(p_buffer[n],
                             m_bps - MPC_FIXED_POINT_SCALE_SHIFT);
#else
            val = (int)(p_buffer[n] * float_scale);
#endif
            if (val < clip_min)
                val = clip_min;
            else if (val > clip_max)
                val = clip_max;
            if (!WriteInt(val, m_bps))
                return false;
        }
        m_data_bytes_written += p_size * (m_bps >> 3);
        return true;
    }

    ~WavWriter() {
        if (m_data_bytes_written & 1) {
            char blah = 0;
            WriteRaw(&blah, 1);
            m_data_bytes_written++;
        }
        Seek(4);
        WriteDword((unsigned long)(m_data_bytes_written + 4 + 8 + WFX_SIZE +
                                   8));
        Seek(8 + 4 + 8 + WFX_SIZE + 4);
        WriteDword(m_data_bytes_written);
    }

  private:

    mpc_bool_t Seek(unsigned p_offset) {
        return !fseek(m_file, p_offset, SEEK_SET);
    }

    mpc_bool_t WriteRaw(const void *p_buffer, unsigned p_bytes) {
        return fwrite(p_buffer, 1, p_bytes, m_file) == p_bytes;
    }

    mpc_bool_t WriteDword(unsigned long p_val) {
        return WriteInt(p_val, 32);
    }
    mpc_bool_t WriteWord(unsigned short p_val) {
        return WriteInt(p_val, 16);
    }

    // write a little-endian number properly
    mpc_bool_t WriteInt(unsigned int p_val, unsigned p_width_bits) {
        unsigned char temp;
        unsigned shift = 0;
        assert((p_width_bits % 8) == 0);
        do {
            temp = (unsigned char)((p_val >> shift) & 0xFF);
            if (!WriteRaw(&temp, 1))
                return false;
            shift += 8;
        } while (shift < p_width_bits);
        return true;
    }

    unsigned m_nch, m_bps, m_srate;
    FILE *m_file;
    unsigned m_data_bytes_written;
};


static void
usage(const char *exename)
{
    printf
        ("Usage: %s <infile.mpc> [<outfile.wav>]\nIf <outfile.wav> is not specified, decoder will run in benchmark mode.\n",
         exename);
}

int
main(int argc, char **argv)
{
    if (argc != 2 && argc != 3) {
        if (argc > 0)
            usage(argv[0]);
        return 0;
    }

    FILE *input = fopen(argv[1], "rb");
    FILE *output = 0;
    if (input == 0) {
        usage(argv[0]);
        printf("Error opening input file: \"%s\"\n", argv[1]);
        return 1;
    }

    if (argc == 3) {
        output = fopen(argv[2], "wb");
        if (output == 0) {
            fclose(input);
            usage(argv[0]);
            printf("Error opening output file: \"%s\"\n", argv[2]);
            return 1;
        }
    }

    /* initialize our reader_data tag the reader will carry around with it */
    reader_data data;
    data.file = input;
    data.seekable = true;
    fseek(data.file, 0, SEEK_END);
    data.size = ftell(data.file);
    fseek(data.file, 0, SEEK_SET);

    /* set up an mpc_reader linked to our function implementations */
    mpc_decoder decoder;
    mpc_reader reader;
    reader.read = read_impl;
    reader.seek = seek_impl;
    reader.tell = tell_impl;
    reader.get_size = get_size_impl;
    reader.canseek = canseek_impl;
    reader.data = &data;

    /* read file's streaminfo data */
    mpc_streaminfo info;
    mpc_streaminfo_init(&info);
    if (mpc_streaminfo_read(&info, &reader) != ERROR_CODE_OK) {
        printf("Not a valid musepack file: \"%s\"\n", argv[1]);
        return 1;
    }

    /* instantiate a decoder with our file reader */
    mpc_decoder_setup(&decoder, &reader);
    if (!mpc_decoder_initialize(&decoder, &info)) {
        printf("Error initializing decoder.\n", argv[1]);
        return 1;
    }

    /* decode the file */
    printf("Decoding from:\n%s\nTo:\n%s\n", argv[1],
           output ? argv[2] : "N/A");
    WavWriter *wavwriter =
        output ? new WavWriter(output, 2, 16, info.sample_freq) : 0;
    MPC_SAMPLE_FORMAT sample_buffer[MPC_DECODER_BUFFER_LENGTH];
    clock_t begin, end;
    begin = clock();
    unsigned total_samples = 0;
    mpc_bool_t successful = FALSE;
    for (;;) {
        unsigned status = mpc_decoder_decode(&decoder, sample_buffer, 0, 0);
        if (status == (unsigned)(-1)) {
            //decode error
            printf("Error decoding file.\n");
            break;
        }
        else if (status == 0)   //EOF
        {
            successful = true;
            break;
        }
        else                    //status>0
        {
            total_samples += status;
            if (wavwriter) {
                if (!wavwriter->
                    WriteSamples(sample_buffer, status * /* stereo */ 2)) {
                    printf("Write error.\n");
                    break;
                }
            }
        }
    }

    end = clock();

    if (wavwriter) {
        delete wavwriter;
    }

    if (successful) {
        printf("\nFinished.\nTotal samples decoded: %u.\n", total_samples);
        unsigned ms = (end - begin) * 1000 / CLOCKS_PER_SEC;
        unsigned ratio =
            (unsigned)((double)total_samples / (double)info.sample_freq /
                       ((double)ms / 1000.0) * 100.0);
        printf("Time: %u ms (%u.%02ux).\n", ms, ratio / 100, ratio % 100);
    }

    return 0;
}
