/*
 * MP4/AAC decoder for xmms
 *
 * OPTIONNAL need
 * --------------
 * libid3 (3.8.x - www.id3.org)
*/

#include <pthread.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>

#if defined(HAVE_BMP)
#include <bmp/plugin.h>
#include <bmp/util.h>
#include <bmp/configfile.h>
#include <bmp/titlestring.h>
#else
#include <xmms/plugin.h>
#include <xmms/util.h>
#include <xmms/configfile.h>
#include <xmms/titlestring.h>
#endif /*HAVE_BMP*/

#include "neaacdec.h"
#include "mp4ff.h"

#define MP4_DESCRIPTION	"MP4 & MPEG2/4-AAC audio player - 1.2.x"
#define MP4_VERSION	"ver. 0.5-faad2-version - 22 August 2004"
#define MP4_ABOUT	"Written by ciberfred"
#define BUFFER_SIZE	FAAD_MIN_STREAMSIZE*64

static void	mp4_init(void);
static void	mp4_about(void);
static void	mp4_play(char *);
static void	mp4_stop(void);
static void	mp4_pause(short);
static void	mp4_seek(int);
static int	mp4_getTime(void);
static void	mp4_cleanup(void);
static void	mp4_getSongTitle(char *, char **, int *);
static void	mp4_getSongInfo(char *);
static int	mp4_isFile(char *);
static void*	mp4Decode(void *);

InputPlugin mp4_ip =
  {
    0,	// handle
    0,	// filename
    MP4_DESCRIPTION,
    mp4_init,
    mp4_about,
    0,	// configuration
    mp4_isFile,
    0,	//scandir
    mp4_play,
    mp4_stop,
    mp4_pause,
    mp4_seek,
    0,	// set equalizer
    mp4_getTime,
    0,	// get volume
    0,
    mp4_cleanup,
    0,	// obsolete
    0,	// send visualisation data
    0,	// set player window info
    0,	// set song title text
    mp4_getSongTitle,	// get song title text
    mp4_getSongInfo, // info box
    0,	// to output plugin
  };

typedef struct  _mp4cfg{
  gshort        file_type;
#define FILE_UNKNOW     0
#define FILE_MP4        1
#define FILE_AAC        2
}               Mp4Config;

static Mp4Config	mp4cfg;
static gboolean		bPlaying = FALSE;
static pthread_t	decodeThread;
static pthread_mutex_t	mutex = PTHREAD_MUTEX_INITIALIZER;
static int		seekPosition = -1;

// Functions from mp4_utils.c
extern int getAACTrack(mp4ff_t *infile);
extern mp4ff_callback_t *getMP4FF_cb(FILE *mp4file);
extern char *getMP4title(mp4ff_t *infile, char *filename);
extern void getMP4info(char* filename, FILE *mp4file);

InputPlugin *get_iplugin_info(void)
{
  return(&mp4_ip);
}

static void mp4_init(void)
{
  memset(&decodeThread, 0, sizeof(pthread_t));
  mp4cfg.file_type = FILE_UNKNOW;
  seekPosition = -1;
  return;
}

static void mp4_play(char *filename)
{
  bPlaying = TRUE;
  pthread_create(&decodeThread, 0, mp4Decode, g_strdup(filename));
  return;
}

static void mp4_stop(void)
{
  if(bPlaying){
    bPlaying = FALSE;
    pthread_join(decodeThread, NULL);
    memset(&decodeThread, 0, sizeof(pthread_t));
    mp4_ip.output->close_audio();
  }
}

static int	mp4_isFile(char *filename)
{
  if(filename){
    gchar*	extention;

    extention = strrchr(filename, '.');
    if(extention &&
       (!strncasecmp(extention, ".mp4", 4) ||	// official extention
	!strncasecmp(extention, ".m4a", 4) ||	// Apple mp4 extention
	!strncasecmp(extention, ".aac", 4)	// old MPEG2/4-AAC extention
	)){
      return (1);
    }
  }
  return(0);
}

static void	mp4_about(void)
{
  static GtkWidget *aboutbox;

  if(aboutbox!=NULL)
    return;
  aboutbox = xmms_show_message("About MP4 AAC player plugin",
			       "libfaad2-" FAAD2_VERSION "\n"
			       "plugin version: " MP4_VERSION "\n"
			       MP4_ABOUT,
			       "Ok", FALSE, NULL, NULL);
  gtk_signal_connect(GTK_OBJECT(aboutbox), "destroy",
                     GTK_SIGNAL_FUNC(gtk_widget_destroyed),
                     &aboutbox);
}

static void	mp4_pause(short flag)
{
  mp4_ip.output->pause(flag);
}

static void	mp4_seek(int time)
{
  seekPosition = time;
  while(bPlaying && seekPosition!=-1)
    xmms_usleep(10000);
}

static int	mp4_getTime(void)
{
  if(!bPlaying)
    return (-1);
  else
    return (mp4_ip.output->output_time());
}

static void	mp4_cleanup(void)
{
}

void mp4_get_file_type(FILE *mp4file)
{
  unsigned char header[10] = {0};

  fseek(mp4file, 0, SEEK_SET);
  fread(header, 1, 8, mp4file);
  if(header[4]=='f' &&
     header[5]=='t' &&
     header[6]=='y' &&
     header[7]=='p'){
    mp4cfg.file_type = FILE_MP4;
  }else{
    mp4cfg.file_type = FILE_AAC;
  }
}

static void	mp4_getSongTitle(char *filename, char **title, int *len) {
  FILE* mp4file;

  (*title) = NULL;
  (*len) = -1;

  if((mp4file = fopen(filename, "rb"))){
    mp4_get_file_type(mp4file);
    fseek(mp4file, 0, SEEK_SET);
    if(mp4cfg.file_type == FILE_MP4){
      mp4ff_callback_t*	mp4cb;
      mp4ff_t*		infile;
      gint		mp4track;

      mp4cb = getMP4FF_cb(mp4file);
      if ((infile = mp4ff_open_read_metaonly(mp4cb)) &&
          ((mp4track = getAACTrack(infile)) >= 0)){
        (*title) = getMP4title(infile, filename);

        double track_duration = mp4ff_get_track_duration(infile, mp4track);
        unsigned long time_scale = mp4ff_time_scale(infile, mp4track);
        unsigned long length = (track_duration * 1000 / time_scale);
        (*len) = length;
      }
      if(infile) mp4ff_close(infile);
      if(mp4cb) g_free(mp4cb);
    }
    else{
      // Check AAC ID3 tag...
    }
    fclose(mp4file);
  }
}

static void	mp4_getSongInfo(char *filename)
{
  FILE* mp4file;
  if((mp4file = fopen(filename, "rb"))){
    if (mp4cfg.file_type == FILE_UNKNOW)
      mp4_get_file_type(mp4file);
    fseek(mp4file, 0, SEEK_SET);
    if(mp4cfg.file_type == FILE_MP4)
      getMP4info(filename, mp4file);
    else if(mp4cfg.file_type == FILE_AAC)
      /*
       * check the id3tagv2
      */
      ;
    fclose(mp4file);
  }
}

static void *mp4Decode(void *args)
{
  FILE*		mp4file;

  pthread_mutex_lock(&mutex);
  seekPosition = -1;
  bPlaying = TRUE;

  if(!(mp4file = fopen(args, "rb"))){
    g_print("MP4!AAC - Can't open file\n");
    g_free(args);
    bPlaying = FALSE;
    pthread_mutex_unlock(&mutex);
    pthread_exit(NULL);

  }
  mp4_get_file_type(mp4file);
  fseek(mp4file, 0, SEEK_SET);
  if(mp4cfg.file_type == FILE_MP4){// We are reading a MP4 file
    mp4ff_callback_t*	mp4cb;
    mp4ff_t*		infile;
    gint		mp4track;

    mp4cb = getMP4FF_cb(mp4file);
    if(!(infile = mp4ff_open_read(mp4cb))){
      g_print("MP4 - Can't open file\n");
      goto end;
    }

    if((mp4track = getAACTrack(infile)) < 0){
      /*
       * TODO: check here for others Audio format.....
       *
      */
      g_print("Unsupported Audio track type\n");
      g_free(args);
      fclose(mp4file);
      bPlaying = FALSE;
      pthread_mutex_unlock(&mutex);
      pthread_exit(NULL);
    }else{
      NeAACDecHandle	decoder;
      unsigned char	*buffer	= NULL;
      guint		bufferSize = 0;
      gulong		samplerate;
      guchar		channels;
      //guint		avgBitrate;
      //MP4Duration	duration;
      int		msDuration;
      int		numSamples;
      int		sampleID = 0;
      unsigned int	framesize;
      mp4AudioSpecificConfig mp4ASC;
      gchar		*xmmstitle;

      decoder = NeAACDecOpen();
      mp4ff_get_decoder_config(infile, mp4track, &buffer, &bufferSize);
      if(NeAACDecInit2(decoder, buffer, bufferSize, &samplerate, &channels)<0){
          goto end;
      }
      if(buffer){
	framesize = 1024;
	if(NeAACDecAudioSpecificConfig(buffer, bufferSize, &mp4ASC) >= 0){
	  if(mp4ASC.frameLengthFlag == 1) framesize = 960;
	  if(mp4ASC.sbr_present_flag == 1) framesize *= 2;
	}
	g_free(buffer);
      }
      if(channels == 0){
	g_print("Number of Channels not supported\n");
	goto end;
      }

      //duration = MP4GetTrackDuration(mp4file, mp4track);
      //msDuration = MP4ConvertFromTrackDuration(mp4file, mp4track,
      //				       duration,MP4_MSECS_TIME_SCALE);

      //msDuration = mp4ff_get_track_duration(infile, mp4track);
      //printf("%d\n", msDuration);

      //numSamples = MP4GetTrackNumberOfSamples(mp4file, mp4track);
      numSamples = mp4ff_num_samples(infile, mp4track);
      {
	float f = 1024.0;
	if(mp4ASC.sbr_present_flag == 1)
	  f = f * 2.0;
	msDuration = ((float)numSamples*(float)(f-1.0)/
		      (float)samplerate)*1000;
      }
      xmmstitle = getMP4title(infile, args);
      mp4_ip.output->open_audio(FMT_S16_NE, samplerate, channels);
      mp4_ip.output->flush(0);
      mp4_ip.set_info(xmmstitle, msDuration, -1, samplerate/1000, channels);
      g_print("MP4 - %d channels @ %ld Hz\n", channels, samplerate);

      while(bPlaying){
	void*			sampleBuffer;
	faacDecFrameInfo	frameInfo;
	gint			rc;

	if(seekPosition!=-1){
	  /*
	  duration = MP4ConvertToTrackDuration(mp4file,
					       mp4track,
					       seekPosition*1000,
					       MP4_MSECS_TIME_SCALE);
	  sampleID = MP4GetSampleIdFromTime(mp4file, mp4track, duration, 0);
          */
          float f = 1024.0;
	  if(mp4ASC.sbr_present_flag == 1)
	    f = f * 2.0;
          sampleID = (float)seekPosition*(float)samplerate/(float)(f-1.0);
	  mp4_ip.output->flush(seekPosition*1000);
	  seekPosition = -1;
	}
	buffer=NULL;
	bufferSize=0;
	rc = mp4ff_read_sample(infile, mp4track, sampleID++,
			       &buffer, &bufferSize);
	//g_print("%d/%d\n", sampleID-1, numSamples);
	if((rc==0) || (buffer== NULL)){
	  g_print("MP4: read error\n");
	  sampleBuffer = NULL;
	  sampleID=0;
	  mp4_ip.output->buffer_free();
            goto end;
	}else{
	  sampleBuffer = NeAACDecDecode(decoder, &frameInfo, buffer, bufferSize);
	  if(frameInfo.error > 0){
	    g_print("MP4: %s\n",
		    faacDecGetErrorMessage(frameInfo.error));
	    goto end;
	  }
	  if(buffer){
	    g_free(buffer); buffer=NULL; bufferSize=0;
	  }
	  while(bPlaying && mp4_ip.output->buffer_free()<frameInfo.samples<<1)
	    xmms_usleep(30000);
	}
	mp4_ip.add_vis_pcm(mp4_ip.output->written_time(),
			   FMT_S16_NE,
			   channels,
			   frameInfo.samples<<1,
			   sampleBuffer);
	mp4_ip.output->write_audio(sampleBuffer, frameInfo.samples<<1);
	if(sampleID >= numSamples){
          break;
	}
      }
      while(bPlaying && mp4_ip.output->buffer_playing() && mp4_ip.output->buffer_free()){
	xmms_usleep(10000);
      }
end:
      mp4_ip.output->close_audio();
      g_free(args);
      NeAACDecClose(decoder);
      if(infile) mp4ff_close(infile);
      if(mp4cb) g_free(mp4cb);
      bPlaying = FALSE;
      fclose(mp4file);
      pthread_mutex_unlock(&mutex);
      pthread_exit(NULL);
    }
  }else{
    // WE ARE READING AN AAC FILE
    FILE		*file = NULL;
    NeAACDecHandle	decoder = 0;
    guchar		*buffer = 0;
    gulong		bufferconsumed = 0;
    gulong		samplerate = 0;
    guchar		channels;
    gulong		buffervalid = 0;
    TitleInput*		input;
    gchar		*temp = g_strdup(args);
    gchar		*ext  = strrchr(temp, '.');
    gchar		*xmmstitle = NULL;
    NeAACDecConfigurationPtr config;

    if((file = fopen(args, "rb")) == 0){
      g_print("AAC: can't find file %s\n", args);
      bPlaying = FALSE;
      pthread_mutex_unlock(&mutex);
      pthread_exit(NULL);
    }
    if((decoder = NeAACDecOpen()) == NULL){
      g_print("AAC: Open Decoder Error\n");
      fclose(file);
      bPlaying = FALSE;
      pthread_mutex_unlock(&mutex);
      pthread_exit(NULL);
    }
    config = NeAACDecGetCurrentConfiguration(decoder);
    config->useOldADTSFormat = 0;
    NeAACDecSetConfiguration(decoder, config);
    if((buffer = g_malloc(BUFFER_SIZE)) == NULL){
      g_print("AAC: error g_malloc\n");
      fclose(file);
      bPlaying = FALSE;
      NeAACDecClose(decoder);
      pthread_mutex_unlock(&mutex);
      pthread_exit(NULL);
    }
    if((buffervalid = fread(buffer, 1, BUFFER_SIZE, file))==0){
      g_print("AAC: Error reading file\n");
      g_free(buffer);
      fclose(file);
      bPlaying = FALSE;
      NeAACDecClose(decoder);
      pthread_mutex_unlock(&mutex);
      pthread_exit(NULL);
    }
    XMMS_NEW_TITLEINPUT(input);
    input->file_name = g_basename(temp);
    input->file_ext = ext ? ext+1 : NULL;
    input->file_path = temp;
    if(!strncmp(buffer, "ID3", 3)){
      gint size = 0;

      fseek(file, 0, SEEK_SET);
      size = (buffer[6]<<21) | (buffer[7]<<14) | (buffer[8]<<7) | buffer[9];
      size+=10;
      fread(buffer, 1, size, file);
      buffervalid = fread(buffer, 1, BUFFER_SIZE, file);
    }
    xmmstitle = xmms_get_titlestring(xmms_get_gentitle_format(), input);
    if(xmmstitle == NULL)
      xmmstitle = g_strdup(input->file_name);
    if(temp) g_free(temp);
    if(input->performer) g_free(input->performer);
    if(input->album_name) g_free(input->album_name);
    if(input->track_name) g_free(input->track_name);
    if(input->genre) g_free(input->genre);
    g_free(input);
    bufferconsumed = NeAACDecInit(decoder,
				 buffer,
				 buffervalid,
				 &samplerate,
				 &channels);
    if(mp4_ip.output->open_audio(FMT_S16_NE,samplerate,channels) == FALSE){
      g_print("AAC: Output Error\n");
      g_free(buffer); buffer=0;
      faacDecClose(decoder);
      fclose(file);
      mp4_ip.output->close_audio();
      /*
      if(positionTable){
	g_free(positionTable); positionTable=0;
      }
      */
      g_free(xmmstitle);
      bPlaying = FALSE;
      pthread_mutex_unlock(&mutex);
      pthread_exit(NULL);
    }
    //if(bSeek){
    //mp4_ip.set_info(xmmstitle, lenght*1000, -1, samplerate, channels);
      //}else{
    mp4_ip.set_info(xmmstitle, -1, -1, samplerate, channels);
      //}
    mp4_ip.output->flush(0);

    while(bPlaying && buffervalid > 0){
      NeAACDecFrameInfo	finfo;
      unsigned long	samplesdecoded;
      char*		sample_buffer = NULL;
      /*
	if(bSeek && seekPosition!=-1){
	fseek(file, positionTable[seekPosition], SEEK_SET);
	bufferconsumed=0;
	buffervalid = fread(buffer, 1, BUFFER_SIZE, file);
	aac_ip.output->flush(seekPosition*1000);
	seekPosition=-1;
	}
      */
      if(bufferconsumed > 0){
	memmove(buffer, &buffer[bufferconsumed], buffervalid-bufferconsumed);
	buffervalid -= bufferconsumed;
	buffervalid += fread(&buffer[buffervalid], 1,
			     BUFFER_SIZE-buffervalid, file);
	bufferconsumed = 0;
      }
      sample_buffer = NeAACDecDecode(decoder, &finfo, buffer, buffervalid);
      if(finfo.error){
	config = NeAACDecGetCurrentConfiguration(decoder);
	if(config->useOldADTSFormat != 1){
	  NeAACDecClose(decoder);
	  decoder = NeAACDecOpen();
	  config = NeAACDecGetCurrentConfiguration(decoder);
	  config->useOldADTSFormat = 1;
	  NeAACDecSetConfiguration(decoder, config);
	  finfo.bytesconsumed=0;
	  finfo.samples = 0;
	  NeAACDecInit(decoder,
		      buffer,
		      buffervalid,
		      &samplerate,
		      &channels);
	}else{
	  g_print("FAAD2 Warning %s\n", NeAACDecGetErrorMessage(finfo.error));
	  buffervalid = 0;
	}
      }
      bufferconsumed += finfo.bytesconsumed;
      samplesdecoded = finfo.samples;
      if((samplesdecoded<=0) && !sample_buffer){
	g_print("AAC: error sample decoding\n");
	continue;
      }
      while(bPlaying && mp4_ip.output->buffer_free() < (samplesdecoded<<1)){
	xmms_usleep(10000);
      }
      mp4_ip.add_vis_pcm(mp4_ip.output->written_time(),
			 FMT_S16_LE, channels,
			 samplesdecoded<<1, sample_buffer);
      mp4_ip.output->write_audio(sample_buffer, samplesdecoded<<1);
    }
    while(bPlaying && mp4_ip.output->buffer_playing()){
      xmms_usleep(10000);
    }
    mp4_ip.output->buffer_free();
    mp4_ip.output->close_audio();
    bPlaying = FALSE;
    g_free(buffer);
    NeAACDecClose(decoder);
    g_free(xmmstitle);
    fclose(file);
    seekPosition = -1;
    /*
    if(positionTable){
      g_free(positionTable); positionTable=0;
    }
    */
    bPlaying = FALSE;
    pthread_mutex_unlock(&mutex);
    pthread_exit(NULL);

  }
}
