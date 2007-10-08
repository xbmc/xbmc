/*
 * Really Slick XScreenSavers
 * Copyright (C) 2002-2006  Michael Chapman
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *****************************************************************************
 *
 * This is a Linux port of the Really Slick Screensavers,
 * Copyright (C) 2002 Terence M. Welsh, available from www.reallyslick.com
 */
#include <common.hh>

#if HAVE_SOUND

#include <cerrno>
#include <dlopen.hh>
#include <oggsound.hh>

#include <vorbis/vorbisfile.h>

namespace OV {
#if USE_DLOPEN
	typedef int (*CLEAR)(OggVorbis_File *vf);
	CLEAR ov_clear;
	typedef vorbis_info * (*INFO)(OggVorbis_File *vf,int link);
	INFO ov_info;
	typedef int (*OPEN)(FILE *f,OggVorbis_File *vf,char *initial,long ibytes);
	OPEN ov_open;
	typedef ogg_int64_t (*PCM_TOTAL)(OggVorbis_File *vf,int i);
	PCM_TOTAL ov_pcm_total;
	typedef long (*READ)(OggVorbis_File *vf,char *buffer,int length,
		int bigendianp,int word,int sgned,int *bitstream);
	READ ov_read;
	typedef long (*SEEKABLE)(OggVorbis_File *vf);
	SEEKABLE ov_seekable;
	typedef long (*STREAMS)(OggVorbis_File *vf);
	STREAMS ov_streams;
#else
	using ::ov_clear;
	using ::ov_info;
	using ::ov_open;
	using ::ov_pcm_total;
	using ::ov_read;
	using ::ov_seekable;
	using ::ov_streams;
#endif
};

void OGG::load(FILE* in) {
	OggVorbis_File ovf;
	int result;

	if ((result = OV::ov_open(in, &ovf, NULL, 0)) != 0) {
		switch (result) {
		case OV_EREAD:
			throw Exception("Could not read file");
		case OV_ENOTVORBIS:
			throw Exception("Not an Ogg Vorbis file");
		case OV_EVERSION:
			throw Exception("Ogg Vorbis version mismatch");
		case OV_EBADHEADER:
			throw Exception("Error in Ogg Vorbis header");
		case OV_EFAULT:
			throw Exception("Internal Ogg Vorbis fault");
		default:
			throw Exception(
				stdx::oss() << "Unknown Ogg Vorbis error (" << result << ")"
			);
		}
	}

	try {
		if (!OV::ov_seekable(&ovf))
			throw Exception("Ogg Vorbis file not seekable");

		if (OV::ov_streams(&ovf) != 1)
			throw Exception("Multi-streamed Ogg Vorbis files not yet supported");

		vorbis_info* vi = OV::ov_info(&ovf, -1);
		ALenum format;
		switch (vi->channels) {
		case 1:
			format = AL_FORMAT_MONO16;
			break;
		case 2:
			format = AL_FORMAT_STEREO16;
			break;
		default:
			throw Exception(
				stdx::oss() << "Unsupported number of channels: " << vi->channels
			);
		}
		if (long((unsigned int)vi->rate) != vi->rate)
			throw Exception(
				stdx::oss() << "Unsupported sample rate: " << vi->rate
			);

		std::vector<char> buffer(vi->channels * 2 * OV::ov_pcm_total(&ovf, -1));
		std::vector<char>::iterator pos = buffer.begin();
		while (true) {
			int ignored;
			long result = OV::ov_read(&ovf, &*pos, buffer.end() - pos, 0, 2, 1, &ignored);
			if (result < 0)
				throw Exception("Ogg Vorbis file corrupted");
			if (result == 0)
				break;
			pos += result;
			if (pos > buffer.end())
				throw Exception("libvorbisfile overshot the buffer");
		}

		_buffer = Common::resources->manage(new Buffer(
			format, &buffer.front(), pos - buffer.begin(), vi->rate
		));

		OV::ov_clear(&ovf);
	} catch (...) {
		OV::ov_clear(&ovf);
		throw;
	}
}

OGG::OGG(
	const std::string& filename, float ref, float delay
) : Sound(ref, delay) {
#if USE_DLOPEN
	try {
		const Library* libvorbisfile = Common::resources->manage(new Library("libvorbisfile"));
		OV::ov_clear     = (OV::CLEAR)(*libvorbisfile)("ov_clear");
		OV::ov_info      = (OV::INFO)(*libvorbisfile)("ov_info");
		OV::ov_open      = (OV::OPEN)(*libvorbisfile)("ov_open");
		OV::ov_pcm_total = (OV::PCM_TOTAL)(*libvorbisfile)("ov_pcm_total");
		OV::ov_read      = (OV::READ)(*libvorbisfile)("ov_read");
		OV::ov_seekable  = (OV::SEEKABLE)(*libvorbisfile)("ov_seekable");
		OV::ov_streams   = (OV::STREAMS)(*libvorbisfile)("ov_streams");
	} catch (Common::Exception e) {
		throw Sound::Unavailable();
	}
#endif

	if (filename.empty())
		throw Exception("Empty filename");

	FILE* in = NULL;
	if (filename[0] != '/')
		in = std::fopen((Common::resourceDir + '/' + filename).c_str(), "rb");

	if (!in)
		in = std::fopen(filename.c_str(), "rb");

	if (!in)
		throw Exception(std::strerror(errno));

	load(in);
	// File is closed by ov_clear
}

#endif // HAVE_SOUND
