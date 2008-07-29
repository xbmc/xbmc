#include <math.h>
#include "coding.h"
#include "../util.h"

/* SDX2 - 2:1 Squareroot-delta-exact compression */

/* for (i=-128;i<128;i++) squares[i+128]=i<0?(-i*i)*2:(i*i)*2); */
static int16_t squares[256] = {
-32768,-32258,-31752,-31250,-30752,-30258,-29768,-29282,-28800,-28322,-27848,
-27378,-26912,-26450,-25992,-25538,-25088,-24642,-24200,-23762,-23328,-22898,
-22472,-22050,-21632,-21218,-20808,-20402,-20000,-19602,-19208,-18818,-18432,
-18050,-17672,-17298,-16928,-16562,-16200,-15842,-15488,-15138,-14792,-14450,
-14112,-13778,-13448,-13122,-12800,-12482,-12168,-11858,-11552,-11250,-10952,
-10658,-10368,-10082, -9800, -9522, -9248, -8978, -8712, -8450, -8192, -7938,
 -7688, -7442, -7200, -6962, -6728, -6498, -6272, -6050, -5832, -5618, -5408,
 -5202, -5000, -4802, -4608, -4418, -4232, -4050, -3872, -3698, -3528, -3362,
 -3200, -3042, -2888, -2738, -2592, -2450, -2312, -2178, -2048, -1922, -1800,
 -1682, -1568, -1458, -1352, -1250, -1152, -1058,  -968,  -882,  -800,  -722,
  -648,  -578,  -512,  -450,  -392,  -338,  -288,  -242,  -200,  -162,  -128,
   -98,   -72,   -50,   -32,   -18,    -8,    -2,     0,     2,     8,    18,
    32,    50,    72,    98,   128,   162,   200,   242,   288,   338,   392,
   450,   512,   578,   648,   722,   800,   882,   968,  1058,  1152,  1250,
  1352,  1458,  1568,  1682,  1800,  1922,  2048,  2178,  2312,  2450,  2592,
  2738,  2888,  3042,  3200,  3362,  3528,  3698,  3872,  4050,  4232,  4418,
  4608,  4802,  5000,  5202,  5408,  5618,  5832,  6050,  6272,  6498,  6728,
  6962,  7200,  7442,  7688,  7938,  8192,  8450,  8712,  8978,  9248,  9522,
  9800, 10082, 10368, 10658, 10952, 11250, 11552, 11858, 12168, 12482, 12800,
 13122, 13448, 13778, 14112, 14450, 14792, 15138, 15488, 15842, 16200, 16562,
 16928, 17298, 17672, 18050, 18432, 18818, 19208, 19602, 20000, 20402, 20808,
 21218, 21632, 22050, 22472, 22898, 23328, 23762, 24200, 24642, 25088, 25538,
 25992, 26450, 26912, 27378, 27848, 28322, 28800, 29282, 29768, 30258, 30752,
 31250, 31752, 32258
};

void decode_sdx2(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do) {

	int32_t hist = stream->adpcm_history1_32;

	int i;
	int32_t sample_count;
	
	for (i=first_sample,sample_count=0; i<first_sample+samples_to_do; i++,sample_count+=channelspacing) {
        int8_t sample_byte = read_8bit(stream->offset+i,stream->streamfile);
        int16_t sample;

        if (!(sample_byte & 1)) hist = 0;
        sample = hist + squares[sample_byte+128];

		hist = outbuf[sample_count] = clamp16(sample);
	}
	stream->adpcm_history1_32=hist;
}

void decode_sdx2_int(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do) {

	int32_t hist = stream->adpcm_history1_32;

	int i;
	int32_t sample_count;
	
	for (i=first_sample,sample_count=0; i<first_sample+samples_to_do; i++,sample_count+=channelspacing) {
        int8_t sample_byte = read_8bit(stream->offset+i*channelspacing,stream->streamfile);
        int16_t sample;

        if (!(sample_byte & 1)) hist = 0;
        sample = hist + squares[sample_byte+128];

		hist = outbuf[sample_count] = clamp16(sample);
	}
	stream->adpcm_history1_32=hist;
}
