/***************************************************************

adv2wav

	(c)2001 BERO

	http://www.geocities.co.jp/Playtown/2004/
	bero@geocities.co.jp

	adx info from: http://ku-www.ss.titech.ac.jp/~yatsushi/adx.html

***************************************************************/

// Modified by KM

//#define	BASEVOL	0x11e0
//#define	BASEVOL	0x4000


#define	BASEVOL	0x4500
#define CLIP(x) if (x > 32767) x = 32767; else if (x < -32768) x = -32768
#define AND8(d) if (d &8) d -= 16

#include <stdio.h>
#include <string.h>
#include <fcntl.h>

long read_long(unsigned char *p)
{
	return (p[0]<<24)|(p[1]<<16)|(p[2]<<8)|p[3];
}

int read_word(unsigned char *p)
{
	return (p[0]<<8)|p[1];
}

typedef struct {
	int s1,s2;
} PREV;


void convert(short *out, unsigned char *in, PREV *prev)
{
	int scale = ((in[0]<<8)|(in[1]));
	int i;
	int s0, s1, s2, d;
//	int over=0;

	in+=2;
	s1 = prev->s1;
	s2 = prev->s2;
	for(i=0;i<16;++i) {
		d = in[i]>>4;
		AND8(d);
		s0 = (BASEVOL * d * scale + 0x7298 * s1 - 0x3350 * s2)>>14;
	//	if (abs(s0)>32767) over=1;
		CLIP(s0);
		*out++ = s0;
		s2 = s1;
		s1 = s0;
		d = in[i]&15;
		AND8(d);
		s0 = (BASEVOL*d*scale + 0x7298 * s1 - 0x3350 * s2)>>14;
	//	if (abs(s0)>32767) over=1;
		CLIP(s0);
		*out++ = s0;
		s2 = s1;
		s1 = s0;
	}
	prev->s1 = s1;
	prev->s2 = s2;

//	if (over) putchar('.');
}


//unsigned int adx_get_size(int pos, FILE* in){
//
//unsigned char buf[18*2];
//int channel, freq, size;
//
//		static struct
//		{
//			char hdr1[4];
//			int totalsize;
//			char hdr2[8];
//			int hdrsize;
//			short format;
//			short channel;
//			int freq;
//			int byte_per_sec;
//			short blocksize;
//			short bits;
//			char hdr3[4];
//			int datasize;
//		} wavhdr = {"RIFF", 0,
//		"WAVEfmt ", 0x10, 0x01, 2, 44100, 44100*2*2, 2*2, 16, "data" };
//
//wavhdr.channel = channel;
//wavhdr.freq = freq;
//wavhdr.blocksize = channel * sizeof (short);
//wavhdr.byte_per_sec = freq * wavhdr.blocksize;
//wavhdr.datasize = size * wavhdr.blocksize;
//
//fseek(in, pos, 0);
//fread(buf, 1, 16, in);
//
//channel = channels;
//freq = sampleRate;
//size = read_long(buf +12);
//
//offset = read_word(buf +2) -2;
//fseek(in, offset + pos, SEEK_SET);
//fread(buf+1, 1, 6, in);
//
//if (buf[0] != 0x80 || memcmp(buf+1,"(c)CRI",6))
//	return -1;
//
//wavhdr.channel = channel;
//wavhdr.freq = freq;
//wavhdr.blocksize = channel * sizeof (short);
//wavhdr.byte_per_sec = freq * wavhdr.blocksize;
//wavhdr.datasize = size * wavhdr.blocksize;
//wavhdr.totalsize = wavhdr.datasize + sizeof (wavhdr) -8;
//wavhdr.totalsize = wavhdr.datasize + sizeof (wavhdr) -8;
//
//int size = read_long(buf +12);
//
//
//}



void adx2wav(const short channels, const int sampleRate, 
			 const int pos, FILE* in, FILE* out)
//const char* inName, const char* outName)
{

		unsigned char buf[18*2];
		short outbuf[32*2];
		int offset;
		int channel, freq, size, wsize;
		PREV prev[2];

		static struct
		{
			char hdr1[4];
			int totalsize;
			char hdr2[8];
			int hdrsize;
			short format;
			short channel;
			int freq;
			int byte_per_sec;
			short blocksize;
			short bits;
			char hdr3[4];
			int datasize;
		//} wavhdr = {"RIFF", 0,
		//"WAVEfmt ", 0x10, 0x01, 2, 44100, 44100*2*2, 2*2, 16, "data" };
		} wavhdr = {'R', 'I', 'F', 'F', 0,
		'W', 'A', 'V', 'E', 'f', 'm', 't', ' ', 
		0x10, 0x01, 2, 44100, 44100*2*2, 2*2, 16, 'd', 'a', 't', 'a'};

		//FILE* in; FILE* out;
		//in = fopen(inName, "rb");
		fseek(in, pos, 0);
		fread(buf, 1, 16, in);

		channel = channels;
		freq = sampleRate;
		size = read_long(buf +12);

		offset = read_word(buf +2) -2;
		fseek(in, offset + pos, SEEK_SET);
		fread(buf+1, 1, 6, in);

		if (buf[0] != 0x80 || memcmp(buf+1,"(c)CRI",6))
		{
			return;
		}

		wavhdr.channel = channel;
		wavhdr.freq = freq;
		wavhdr.blocksize = (short)channel * sizeof (short);
		wavhdr.byte_per_sec = freq * wavhdr.blocksize;
		wavhdr.datasize = size * wavhdr.blocksize;
		wavhdr.totalsize = wavhdr.datasize + sizeof (wavhdr) -8;
		wavhdr.totalsize = wavhdr.datasize + sizeof (wavhdr) -8;

		//out = fopen(outName, "wb");
		fwrite(&wavhdr, 1, sizeof (wavhdr), out);

		prev[0].s1 = 0;
		prev[0].s2 = 0;
		prev[1].s1 = 0;
		prev[1].s2 = 0;

		if (channel == 1)
		while(size)
		{
			fread(buf, 1, 18, in);
			convert(outbuf, buf, prev);
			if (size > 32) wsize = 32; else wsize = size;
				size -= wsize;
			fwrite(outbuf, 1, wsize*2, out);
		}
		else if (channel == 2)
		while(size)
		{
			short tmpbuf[32*2];
			int i;

			fread(buf, 1, 18*2, in);
			convert(tmpbuf, buf, prev);
			convert(tmpbuf + 32, buf + 18, prev + 1);
			for(i=0; i<32; ++i)
			{
				outbuf[i*2]   = tmpbuf[i];
				outbuf[i*2+1] = tmpbuf[i+32];
			}
			if (size>32) wsize=32; else wsize = size;
				size -= wsize;
			fwrite(outbuf, 1, wsize * 2 * 2, out);
		}

		//fwrite(outbuf, 1, sizeof(outbuf), out);//fix
		//fclose(in);
		//fclose(out);
}

