/*
    TiMidity++ -- MIDI to WAVE converter and player
    Copyright (C) 1999-2002 Masanao Izumo <mo@goice.co.jp>
    Copyright (C) 1995 Tuukka Toivonen <tt@cgs.fi>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef __GOGO_A_H__
#define __GOGO_A_H__

typedef struct gogo_opts_t_ {
	int optENCODEMODE;
	int optBITRATE1;
	int optBITRATE2;
	int optINPFREQ;
	int optOUTFREQ;
	long optSTARTOFFSET;
	int optUSEPSY;
	int optUSELPF16;
	int optUSECPUOPT;
	int optUSEMMX;
	int optUSE3DNOW;
	int optUSEKNI;
	int optUSEE3DNOW;
	int optUSESSE;
	int optUSECMOV;
	int optUSEEMMX;
	int optUSESSE2;
	int optADDTAGnum;
	int optADDTAG_len[64];
	char *optADDTAG_buf[64];
	int optEMPHASIS;
	int optVBR;	// 0..9
	int optCPU;	// 1..
	int optBYTE_SWAP;
	int opt8BIT_PCM;
	int optMONO_PCM;
	int optTOWNS_SND;
	int optTHREAD_PRIORITY;
	int optREADTHREAD_PRIORITY;
	int optOUTPUT_FORMAT;
	int optENHANCEDFILTER_A;
	int optENHANCEDFILTER_B;
	int optVBRBITRATE_low;
	int optVBRBITRATE_high;
	int optMSTHRESHOLD_threshold;
	int optMSTHRESHOLD_mspower;
	int optVERIFY;
	char optOUTPUTDIR[1024];
	char output_name[1024];
} gogo_opts_t;
extern volatile gogo_opts_t gogo_opts;

extern void gogo_opts_init(void);
extern void gogo_opts_reset(void);
extern void gogo_opts_reset_tag(void);
extern int commandline_to_argc_argv(char *commandline, int *argc, char ***argv);

// ID3 TAG 128bytes
typedef struct mp3_id3_tag_t_ {
	char tag[3];		// "ID3"
	char title[30];
	char artist[30];
	char album[30];
	char year[4];
	char comment[30];
	unsigned char genre;
} mp3_id3_tag_t;

#endif /* __GOGO_A_H__ */
