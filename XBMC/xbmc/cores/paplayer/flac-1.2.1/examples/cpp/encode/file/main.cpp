/* example_cpp_encode_file - Simple FLAC file encoder using libFLAC
 * Copyright (C) 2007  Josh Coalson
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

/*
 * This example shows how to use libFLAC++ to encode a WAVE file to a FLAC
 * file.  It only supports 16-bit stereo files in canonical WAVE format.
 *
 * Complete API documentation can be found at:
 *   http://flac.sourceforge.net/api/
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include "FLAC++/metadata.h"
#include "FLAC++/encoder.h"

class OurEncoder: public FLAC::Encoder::File {
public:
	OurEncoder(): FLAC::Encoder::File() { }
protected:
	virtual void progress_callback(FLAC__uint64 bytes_written, FLAC__uint64 samples_written, unsigned frames_written, unsigned total_frames_estimate);
};

#define READSIZE 1024

static unsigned total_samples = 0; /* can use a 32-bit number due to WAVE size limitations */
static FLAC__byte buffer[READSIZE/*samples*/ * 2/*bytes_per_sample*/ * 2/*channels*/]; /* we read the WAVE data into here */
static FLAC__int32 pcm[READSIZE/*samples*/ * 2/*channels*/];
static FLAC__int32 *pcm_[2] = { pcm, pcm+READSIZE };

int main(int argc, char *argv[])
{
	bool ok = true;
	OurEncoder encoder;
	FLAC__StreamEncoderInitStatus init_status;
	FLAC__StreamMetadata *metadata[2];
	FLAC__StreamMetadata_VorbisComment_Entry entry;
	FILE *fin;
	unsigned sample_rate = 0;
	unsigned channels = 0;
	unsigned bps = 0;

	if(argc != 3) {
		fprintf(stderr, "usage: %s infile.wav outfile.flac\n", argv[0]);
		return 1;
	}

	if((fin = fopen(argv[1], "rb")) == NULL) {
		fprintf(stderr, "ERROR: opening %s for output\n", argv[1]);
		return 1;
	}

	/* read wav header and validate it */
	if(
		fread(buffer, 1, 44, fin) != 44 ||
		memcmp(buffer, "RIFF", 4) ||
		memcmp(buffer+8, "WAVEfmt \020\000\000\000\001\000\002\000", 16) ||
		memcmp(buffer+32, "\004\000\020\000data", 8)
	) {
		fprintf(stderr, "ERROR: invalid/unsupported WAVE file, only 16bps stereo WAVE in canonical form allowed\n");
		fclose(fin);
		return 1;
	}
	sample_rate = ((((((unsigned)buffer[27] << 8) | buffer[26]) << 8) | buffer[25]) << 8) | buffer[24];
	channels = 2;
	bps = 16;
	total_samples = (((((((unsigned)buffer[43] << 8) | buffer[42]) << 8) | buffer[41]) << 8) | buffer[40]) / 4;
   
	/* check the encoder */
	if(!encoder) {
		fprintf(stderr, "ERROR: allocating encoder\n");
		fclose(fin);
		return 1;
	}

	ok &= encoder.set_verify(true);
	ok &= encoder.set_compression_level(5);
	ok &= encoder.set_channels(channels);
	ok &= encoder.set_bits_per_sample(bps);
	ok &= encoder.set_sample_rate(sample_rate);
	ok &= encoder.set_total_samples_estimate(total_samples);

	/* now add some metadata; we'll add some tags and a padding block */
	if(ok) {
		if(
			(metadata[0] = FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT)) == NULL ||
			(metadata[1] = FLAC__metadata_object_new(FLAC__METADATA_TYPE_PADDING)) == NULL ||
			/* there are many tag (vorbiscomment) functions but these are convenient for this particular use: */
			!FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "ARTIST", "Some Artist") ||
			!FLAC__metadata_object_vorbiscomment_append_comment(metadata[0], entry, /*copy=*/false) || /* copy=false: let metadata object take control of entry's allocated string */
			!FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "YEAR", "1984") ||
			!FLAC__metadata_object_vorbiscomment_append_comment(metadata[0], entry, /*copy=*/false)
		) {
			fprintf(stderr, "ERROR: out of memory or tag error\n");
			ok = false;
		}

		metadata[1]->length = 1234; /* set the padding length */

		ok = encoder.set_metadata(metadata, 2);
	}

	/* initialize encoder */
	if(ok) {
		init_status = encoder.init(argv[2]);
		if(init_status != FLAC__STREAM_ENCODER_INIT_STATUS_OK) {
			fprintf(stderr, "ERROR: initializing encoder: %s\n", FLAC__StreamEncoderInitStatusString[init_status]);
			ok = false;
		}
	}

	/* read blocks of samples from WAVE file and feed to encoder */
	if(ok) {
		size_t left = (size_t)total_samples;
		while(ok && left) {
			size_t need = (left>READSIZE? (size_t)READSIZE : (size_t)left);
			if(fread(buffer, channels*(bps/8), need, fin) != need) {
				fprintf(stderr, "ERROR: reading from WAVE file\n");
				ok = false;
			}
			else {
				/* convert the packed little-endian 16-bit PCM samples from WAVE into an interleaved FLAC__int32 buffer for libFLAC */
				size_t i;
				for(i = 0; i < need*channels; i++) {
					/* inefficient but simple and works on big- or little-endian machines */
					pcm[i] = (FLAC__int32)(((FLAC__int16)(FLAC__int8)buffer[2*i+1] << 8) | (FLAC__int16)buffer[2*i]);
				}
				/* feed samples to encoder */
				ok = encoder.process_interleaved(pcm, need);
			}
			left -= need;
		}
	}

	ok &= encoder.finish();

	fprintf(stderr, "encoding: %s\n", ok? "succeeded" : "FAILED");
	fprintf(stderr, "   state: %s\n", encoder.get_state().resolved_as_cstring(encoder));

	/* now that encoding is finished, the metadata can be freed */
	FLAC__metadata_object_delete(metadata[0]);
	FLAC__metadata_object_delete(metadata[1]);

	fclose(fin);

	return 0;
}

void OurEncoder::progress_callback(FLAC__uint64 bytes_written, FLAC__uint64 samples_written, unsigned frames_written, unsigned total_frames_estimate)
{
#ifdef _MSC_VER
	fprintf(stderr, "wrote %I64u bytes, %I64u/%u samples, %u/%u frames\n", bytes_written, samples_written, total_samples, frames_written, total_frames_estimate);
#else
	fprintf(stderr, "wrote %llu bytes, %llu/%u samples, %u/%u frames\n", bytes_written, samples_written, total_samples, frames_written, total_frames_estimate);
#endif
}
