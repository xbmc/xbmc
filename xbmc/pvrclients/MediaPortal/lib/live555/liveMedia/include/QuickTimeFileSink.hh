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
// A sink that generates a QuickTime file from a composite media session
// C++ header

#ifndef _QUICKTIME_FILE_SINK_HH
#define _QUICKTIME_FILE_SINK_HH

#ifndef _MEDIA_SESSION_HH
#include "MediaSession.hh"
#endif

class QuickTimeFileSink: public Medium {
public:
  static QuickTimeFileSink* createNew(UsageEnvironment& env,
				      MediaSession& inputSession,
				      char const* outputFileName,
				      unsigned bufferSize = 20000,
				      unsigned short movieWidth = 240,
				      unsigned short movieHeight = 180,
				      unsigned movieFPS = 15,
				      Boolean packetLossCompensate = False,
				      Boolean syncStreams = False,
				      Boolean generateHintTracks = False,
				      Boolean generateMP4Format = False);

  typedef void (afterPlayingFunc)(void* clientData);
  Boolean startPlaying(afterPlayingFunc* afterFunc,
                       void* afterClientData);

  unsigned numActiveSubsessions() const { return fNumSubsessions; }

private:
  QuickTimeFileSink(UsageEnvironment& env, MediaSession& inputSession,
		    char const* outputFileName, unsigned bufferSize,
		    unsigned short movieWidth, unsigned short movieHeight,
		    unsigned movieFPS, Boolean packetLossCompensate,
		    Boolean syncStreams, Boolean generateHintTracks,
		    Boolean generateMP4Format);
      // called only by createNew()
  virtual ~QuickTimeFileSink();

  Boolean continuePlaying();
  static void afterGettingFrame(void* clientData, unsigned frameSize,
				unsigned numTruncatedBytes,
				struct timeval presentationTime,
				unsigned durationInMicroseconds);
  static void onSourceClosure(void* clientData);
  void onSourceClosure1();
  static void onRTCPBye(void* clientData);
  void completeOutputFile();

private:
  friend class SubsessionIOState;
  MediaSession& fInputSession;
  FILE* fOutFid;
  unsigned fBufferSize;
  Boolean fPacketLossCompensate;
  Boolean fSyncStreams, fGenerateMP4Format;
  struct timeval fNewestSyncTime, fFirstDataTime;
  Boolean fAreCurrentlyBeingPlayed;
  afterPlayingFunc* fAfterFunc;
  void* fAfterClientData;
  unsigned fAppleCreationTime;
  unsigned fLargestRTPtimestampFrequency;
  unsigned fNumSubsessions, fNumSyncedSubsessions;
  struct timeval fStartTime;
  Boolean fHaveCompletedOutputFile;

private:
  ///// Definitions specific to the QuickTime file format:

  unsigned addWord64(u_int64_t word);
  unsigned addWord(unsigned word);
  unsigned addHalfWord(unsigned short halfWord);
  unsigned addByte(unsigned char byte) {
    putc(byte, fOutFid);
    return 1;
  }
  unsigned addZeroWords(unsigned numWords);
  unsigned add4ByteString(char const* str);
  unsigned addArbitraryString(char const* str,
			      Boolean oneByteLength = True);
  unsigned addAtomHeader(char const* atomName);
  unsigned addAtomHeader64(char const* atomName);
      // strlen(atomName) must be 4
  void setWord(int64_t filePosn, unsigned size);
  void setWord64(int64_t filePosn, u_int64_t size);

  unsigned movieTimeScale() const {return fLargestRTPtimestampFrequency;}

  // Define member functions for outputting various types of atom:
#define _atom(name) unsigned addAtom_##name()
  _atom(ftyp); // for MP4 format files
  _atom(moov);
      _atom(mvhd);
  _atom(iods); // for MP4 format files
      _atom(trak);
          _atom(tkhd);
          _atom(edts);
              _atom(elst);
          _atom(tref);
              _atom(hint);
          _atom(mdia);
              _atom(mdhd);
              _atom(hdlr);
              _atom(minf);
                  _atom(smhd);
                  _atom(vmhd);
                  _atom(gmhd);
                      _atom(gmin);
                  unsigned addAtom_hdlr2();
                  _atom(dinf);
                      _atom(dref);
                          _atom(alis);
                  _atom(stbl);
                      _atom(stsd);
                          unsigned addAtom_genericMedia();
                          unsigned addAtom_soundMediaGeneral();
                          _atom(ulaw);
                          _atom(alaw);
                          _atom(Qclp);
                              _atom(wave);
                                  _atom(frma);
                                  _atom(Fclp);
                                  _atom(Hclp);
                          _atom(mp4a);
//                            _atom(wave);
//                                _atom(frma);
                                  _atom(esds);
                                  _atom(srcq);
                          _atom(h263);
                          _atom(avc1);
                              _atom(avcC);
                          _atom(mp4v);
                          _atom(rtp);
                              _atom(tims);
                      _atom(stts);
                      _atom(stss);
                      _atom(stsc);
                      _atom(stsz);
                      _atom(co64);
          _atom(udta);
              _atom(name);
              _atom(hnti);
                  _atom(sdp);
              _atom(hinf);
                  _atom(totl);
                  _atom(npck);
                  _atom(tpay);
                  _atom(trpy);
                  _atom(nump);
                  _atom(tpyl);
                  _atom(dmed);
                  _atom(dimm);
                  _atom(drep);
                  _atom(tmin);
                  _atom(tmax);
                  _atom(pmax);
                  _atom(dmax);
                  _atom(payt);
  unsigned addAtom_dummy();

private:
  unsigned short fMovieWidth, fMovieHeight;
  unsigned fMovieFPS;
  int64_t fMDATposition;
  int64_t fMVHD_durationPosn;
  unsigned fMaxTrackDurationM; // in movie time units
  class SubsessionIOState* fCurrentIOState;
};

#endif
