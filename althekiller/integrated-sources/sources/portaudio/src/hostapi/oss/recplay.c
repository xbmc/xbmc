/*
 * recplay.c
 * Phil Burk
 * Minimal record and playback test.
 * 
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#ifndef __STDC__
/* #include <getopt.h> */
#endif /* __STDC__ */
#include <fcntl.h>
#ifdef __STDC__
#include <string.h>
#else /* __STDC__ */
#include <strings.h>
#endif /* __STDC__ */
#include <sys/soundcard.h>

#define NUM_BYTES   (64*1024)
#define BLOCK_SIZE   (4*1024)

#define AUDIO "/dev/dsp"

char buffer[NUM_BYTES];

int audioDev = 0;

main (int argc, char *argv[])
{
    int   numLeft;
    char *ptr;
    int   num;
    int   samplesize;

    /********** RECORD ********************/
    /* Open audio device. */
    audioDev = open (AUDIO, O_RDONLY, 0);
    if (audioDev == -1)
    {
        perror (AUDIO);
        exit (-1);
    }

    /* Set to 16 bit samples. */
    samplesize = 16;
    ioctl(audioDev, SNDCTL_DSP_SAMPLESIZE, &samplesize);
    if (samplesize != 16)
    {
        perror("Unable to set the sample size.");
        exit(-1);
    }

    /* Record in blocks */
    printf("Begin recording.\n");
    numLeft = NUM_BYTES;
    ptr = buffer;
    while( numLeft >= BLOCK_SIZE )
    {
        if ( (num = read (audioDev, ptr, BLOCK_SIZE)) < 0 )
        {
            perror (AUDIO);
            exit (-1);
        }
        else
        {
            printf("Read %d bytes\n", num);
            ptr += num;
            numLeft -= num;
        }
    }

    close( audioDev );

    /********** PLAYBACK ********************/
    /* Open audio device for writing. */
    audioDev = open (AUDIO, O_WRONLY, 0);
    if (audioDev == -1)
    {
        perror (AUDIO);
        exit (-1);
    }

    /* Set to 16 bit samples. */
    samplesize = 16;
    ioctl(audioDev, SNDCTL_DSP_SAMPLESIZE, &samplesize);
    if (samplesize != 16)
    {
        perror("Unable to set the sample size.");
        exit(-1);
    }

    /* Play in blocks */
    printf("Begin playing.\n");
    numLeft = NUM_BYTES;
    ptr = buffer;
    while( numLeft >= BLOCK_SIZE )
    {
        if ( (num = write (audioDev, ptr, BLOCK_SIZE)) < 0 )
        {
            perror (AUDIO);
            exit (-1);
        }
        else
        {
            printf("Wrote %d bytes\n", num);
            ptr += num;
            numLeft -= num;
        }
    }

    close( audioDev );
}
