/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// "liveMedia"
// Copyright (c) 1996-2010 Live Networks, Inc.  All rights reserved.
// Inclusion of header files representing the interface
// for the entire library
//
// Programs that use the library can include this header file,
// instead of each of the individual media header files

#ifndef _LIVEMEDIA_HH
#define _LIVEMEDIA_HH

#include "MPEG1or2AudioRTPSink.hh"
#include "MP3ADURTPSink.hh"
#include "MPEG1or2VideoRTPSink.hh"
#include "MPEG4ESVideoRTPSink.hh"
#include "AMRAudioFileSink.hh"
#include "H264VideoFileSink.hh"
#include "BasicUDPSink.hh"
#include "MPEG1or2VideoHTTPSink.hh"
#include "GSMAudioRTPSink.hh"
#include "H263plusVideoRTPSink.hh"
#include "H264VideoRTPSink.hh"
#include "DVVideoRTPSource.hh"
#include "DVVideoRTPSink.hh"
#include "DVVideoStreamFramer.hh"
#include "H264VideoStreamFramer.hh"
#include "JPEGVideoRTPSink.hh"
#include "SimpleRTPSink.hh"
#include "uLawAudioFilter.hh"
#include "MPEG2IndexFromTransportStream.hh"
#include "MPEG2TransportStreamTrickModeFilter.hh"
#include "ByteStreamMultiFileSource.hh"
#include "BasicUDPSource.hh"
#include "SimpleRTPSource.hh"
#include "MPEG1or2AudioRTPSource.hh"
#include "MPEG4LATMAudioRTPSource.hh"
#include "MPEG4LATMAudioRTPSink.hh"
#include "MPEG4ESVideoRTPSource.hh"
#include "MPEG4GenericRTPSource.hh"
#include "MP3ADURTPSource.hh"
#include "QCELPAudioRTPSource.hh"
#include "AMRAudioRTPSource.hh"
#include "JPEGVideoRTPSource.hh"
#include "JPEGVideoSource.hh"
#include "MPEG1or2VideoRTPSource.hh"
#include "MPEG2TransportStreamFromPESSource.hh"
#include "MPEG2TransportStreamFromESSource.hh"
#include "MPEG2TransportStreamFramer.hh"
#include "ADTSAudioFileSource.hh"
#include "H261VideoRTPSource.hh"
#include "H263plusVideoRTPSource.hh"
#include "H264VideoRTPSource.hh"
#include "MP3HTTPSource.hh"
#include "MP3ADU.hh"
#include "MP3ADUinterleaving.hh"
#include "MP3Transcoder.hh"
#include "MPEG1or2DemuxedElementaryStream.hh"
#include "MPEG1or2AudioStreamFramer.hh"
#include "H263plusVideoStreamFramer.hh"
#include "AC3AudioStreamFramer.hh"
#include "AC3AudioRTPSource.hh"
#include "AC3AudioRTPSink.hh"
#include "MPEG4GenericRTPSink.hh"
#include "MPEG1or2VideoStreamDiscreteFramer.hh"
#include "MPEG4VideoStreamDiscreteFramer.hh"
#include "DeviceSource.hh"
#include "AudioInputDevice.hh"
#include "WAVAudioFileSource.hh"
#include "RTSPServer.hh"
#include "RTSPOverHTTPServer.hh"
#include "RTSPClient.hh"
#include "SIPClient.hh"
#include "QuickTimeFileSink.hh"
#include "QuickTimeGenericRTPSource.hh"
#include "AVIFileSink.hh"
#include "PassiveServerMediaSubsession.hh"
#include "MPEG4VideoFileServerMediaSubsession.hh"
#include "WAVAudioFileServerMediaSubsession.hh"
#include "AMRAudioFileServerMediaSubsession.hh"
#include "AMRAudioFileSource.hh"
#include "AMRAudioRTPSink.hh"
#include "MP3AudioFileServerMediaSubsession.hh"
#include "MPEG1or2VideoFileServerMediaSubsession.hh"
#include "MPEG1or2FileServerDemux.hh"
#include "MPEG2TransportFileServerMediaSubsession.hh"
#include "H263plusVideoFileServerMediaSubsession.hh"
#include "ADTSAudioFileServerMediaSubsession.hh"
#include "DVVideoFileServerMediaSubsession.hh"
#include "DarwinInjector.hh"

#endif
