/*
** some function for MP4 file based on libmp4v2 from mpeg4ip project
*/
#include <mp4.h>
#include <faad.h>

const char *mp4AudioNames[]=
  {
    "MPEG-1 Audio Layers 1,2 or 3",
    "MPEG-2 low biterate (MPEG-1 extension) - MP3",
    "MPEG-2 AAC Main Profile",
    "MPEG-2 AAC Low Complexity profile",
    "MPEG-2 AAC SSR profile",
    "MPEG-4 audio (MPEG-4 AAC)",
    0
  };

const u_int8_t mp4AudioTypes[] =
  {
    MP4_MPEG1_AUDIO_TYPE,		// 0x6B
    MP4_MPEG2_AUDIO_TYPE,		// 0x69
    MP4_MPEG2_AAC_MAIN_AUDIO_TYPE,	// 0x66
    MP4_MPEG2_AAC_LC_AUDIO_TYPE,	// 0x67
    MP4_MPEG2_AAC_SSR_AUDIO_TYPE,	// 0x68
    MP4_MPEG4_AUDIO_TYPE,		// 0x40
    0
  };

/* MPEG-4 Audio types from 14496-3 Table 1.5.1 (from mp4.h)*/
const char *mpeg4AudioNames[]=
  {
    "!!!!MPEG-4 Audio track Invalid !!!!!!!",
    "MPEG-4 AAC Main profile",
    "MPEG-4 AAC Low Complexity profile",
    "MPEG-4 AAC SSR profile",
    "MPEG-4 AAC Long Term Prediction profile",
    "MPEG-4 AAC Scalable",
    "MPEG-4 CELP",
    "MPEG-4 HVXC",
    "MPEG-4 Text To Speech",
    "MPEG-4 Main Synthetic profile",
    "MPEG-4 Wavetable Synthesis profile",
    "MPEG-4 MIDI Profile",
    "MPEG-4 Algorithmic Synthesis and Audio FX profile"
  };

int getAACTrack(MP4FileHandle file)
{
  int numTracks = MP4GetNumberOfTracks(file, NULL, 0);
  int i=0;

  for(i=0;i<numTracks;i++){
    MP4TrackId trackID = MP4FindTrackId(file, i, NULL, 0);
    const char *trackType = MP4GetTrackType(file, trackID);
    if(!strcmp(trackType, MP4_AUDIO_TRACK_TYPE)){//we found audio track !
      int j=0;
      u_int8_t audiotype = MP4GetTrackAudioType(file, trackID);
      while(mp4AudioTypes[j]){ // what kind of audio is ?
	if(mp4AudioTypes[j] == audiotype){
	  if(mp4AudioTypes[j] == MP4_MPEG4_AUDIO_TYPE){//MPEG4 audio ok
	    audiotype = MP4GetTrackAudioMpeg4Type(file, trackID);
	    printf("%d-%s\n", audiotype, mpeg4AudioNames[audiotype]);
	    return (trackID);
	  }
	  else{
	    printf("%s\n", mp4AudioNames[j]);
	    if (mp4AudioTypes[j]== MP4_MPEG2_AAC_LC_AUDIO_TYPE ||
		mp4AudioTypes[j]== MP4_MPEG2_AAC_MAIN_AUDIO_TYPE ||
		mp4AudioTypes[j]== MP4_MPEG2_AAC_SSR_AUDIO_TYPE)
	      return(trackID);
	    return(-1);
	  }
	}
	j++;
      }
    }
  }
    return(-1);
}

int getAudioTrack(MP4FileHandle file)
{
  int numTracks = MP4GetNumberOfTracks(file, NULL,0);
  int i=0;

  for(i=0;i<numTracks;i++){
    MP4TrackId trackID = MP4FindTrackId(file, i, NULL, 0);
    const char *trackType = MP4GetTrackType(file, trackID);
    if(!strcmp(trackType, MP4_AUDIO_TRACK_TYPE)){
      return(trackID);
    }
  }
  return(-1);
}

int getVideoTrack(MP4FileHandle file)
{
  int numTracks = MP4GetNumberOfTracks(file, NULL, 0);
  int i=0;

  for(i=0;i<numTracks; i++){
    MP4TrackId trackID = MP4FindTrackId(file, i, NULL, 0);
    const char *trackType = MP4GetTrackType(file, trackID);
    if(!strcmp(trackType, MP4_VIDEO_TRACK_TYPE)){
      return (trackID);
    }
  }
  return(-1);
}

void getMP4info(char* file)
{
  MP4FileHandle	mp4file;
  MP4Duration	trackDuration;
  int numTracks;
  int i=0;

  if(!(mp4file = MP4Read(file,0)))
    return;
  //MP4Dump(mp4file, 0, 0);
  numTracks = MP4GetNumberOfTracks(mp4file, NULL, 0);
  g_print("there are %d track(s)\n", numTracks);
  for(i=0;i<numTracks;i++){
    MP4TrackId trackID = MP4FindTrackId(mp4file, i, NULL, 0);
    const char *trackType = MP4GetTrackType(mp4file, trackID);
    printf("Track %d, %s", trackID, trackType);
    if(!strcmp(trackType, MP4_AUDIO_TRACK_TYPE)){//we found audio track !
      int j=0;
      u_int8_t audiotype = MP4GetTrackAudioType(mp4file, trackID);
      while(mp4AudioTypes[j]){ // what kind of audio is ?
	if(mp4AudioTypes[j] == audiotype){
	  if(mp4AudioTypes[j] == MP4_MPEG4_AUDIO_TYPE){
	    audiotype = MP4GetTrackAudioMpeg4Type(mp4file, trackID);
	    g_print(" %s", mpeg4AudioNames[audiotype]);
	  }
	  else{
	    printf(" %s", mp4AudioNames[j]);
	  }
	  g_print(" duration :%d",
		 MP4ConvertFromTrackDuration(mp4file, trackID,
					     MP4GetTrackDuration(mp4file,
								 trackID),
					     MP4_MSECS_TIME_SCALE));
	}
	j++;
      }
    }
    printf("\n");
  }
  MP4Close(mp4file);
}
