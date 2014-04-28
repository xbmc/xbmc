/*
 * This file is part of libdvdread.
 *
 * libdvdread is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * libdvdread is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with libdvdread; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "dvdread/ifo_types.h"
#include "dvdread/ifo_read.h"
#include "dvdread/ifo_print.h"

/* Put this in some other file / package?  It's used in nav_print too. */
static void ifo_print_time(int level, dvd_time_t *dtime) {
  const char *rate;
  assert((dtime->hour>>4) < 0xa && (dtime->hour&0xf) < 0xa);
  assert((dtime->minute>>4) < 0x7 && (dtime->minute&0xf) < 0xa);
  assert((dtime->second>>4) < 0x7 && (dtime->second&0xf) < 0xa);
  assert((dtime->frame_u&0xf) < 0xa);

  printf("%02x:%02x:%02x.%02x",
	 dtime->hour,
	 dtime->minute,
	 dtime->second,
	 dtime->frame_u & 0x3f);
  switch((dtime->frame_u & 0xc0) >> 6) {
  case 1:
    rate = "25.00";
    break;
  case 3:
    rate = "29.97";
    break;
  default:
    if(dtime->hour == 0 && dtime->minute == 0
       && dtime->second == 0 && dtime->frame_u == 0)
      rate = "no";
    else
      rate = "(please send a bug report)";
    break;
  }
  printf(" @ %s fps", rate);
}

void dvdread_print_time(dvd_time_t *dtime) {
  ifo_print_time(5, dtime);
}

/* Put this in some other file / package?  It's used in nav_print too.
   Possibly also by the vm / navigator. */
static void ifo_print_cmd(int row, vm_cmd_t *command) {
  int i;

  printf("(%03d) ", row + 1);
  for(i=0;i<8;i++)
    printf("%02x ", command->bytes[i]);
  printf("| ");
#if 0
  //disabled call of dvdnav function
  vm_print_mnemonic(command);
#endif
  printf("\n");
}

static void ifo_print_video_attributes(int level, video_attr_t *attr) {

  /* The following test is shorter but not correct ISO C,
     memcmp(attr,my_friendly_zeros, sizeof(video_attr_t)) */
  if(attr->mpeg_version == 0
     && attr->video_format == 0
     && attr->display_aspect_ratio == 0
     && attr->permitted_df == 0
     && attr->unknown1 == 0
     && attr->line21_cc_1 == 0
     && attr->line21_cc_2 == 0
     && attr->video_format == 0
     && attr->letterboxed == 0
     && attr->film_mode == 0) {
    printf("-- Unspecified --");
    return;
  }

  switch(attr->mpeg_version) {
  case 0:
    printf("mpeg1, ");
    break;
  case 1:
    printf("mpeg2, ");
    break;
  default:
    printf("(please send a bug report), ");
  }

  switch(attr->video_format) {
  case 0:
    printf("ntsc, ");
    break;
  case 1:
    printf("pal, ");
    break;
  default:
    printf("(please send a bug report), ");
  }

  switch(attr->display_aspect_ratio) {
  case 0:
    printf("4:3, ");
    break;
  case 3:
    printf("16:9, ");
    break;
  default:
    printf("(please send a bug report), ");
  }

  // Wide is always allowed..!!!
  switch(attr->permitted_df) {
  case 0:
    printf("pan&scan+letterboxed, ");
    break;
  case 1:
    printf("only pan&scan, "); //??
    break;
  case 2:
    printf("only letterboxed, ");
    break;
  case 3:
    printf("not specified, ");
    break;
  default:
    printf("(please send a bug report), ");
  }

  printf("U%x, ", attr->unknown1);
  assert(!attr->unknown1);

  if(attr->line21_cc_1 || attr->line21_cc_2) {
    printf("NTSC CC ");
    if(attr->line21_cc_1)
      printf("1, ");
    if(attr->line21_cc_2)
      printf("2, ");
  }

  {
    int height = 480;
    if(attr->video_format != 0)
      height = 576;
    switch(attr->picture_size) {
    case 0:
      printf("720x%d, ", height);
      break;
    case 1:
      printf("704x%d, ", height);
      break;
    case 2:
      printf("352x%d, ", height);
      break;
    case 3:
      printf("352x%d, ", height/2);
      break;
    default:
      printf("(please send a bug report), ");
    }
  }

  if(attr->letterboxed) {
    printf("source letterboxed, ");
  }

  if(attr->film_mode) {
    printf("film. ");
  } else {
    printf("video. "); //camera
  }
}

static void ifo_print_audio_attributes(int level, audio_attr_t *attr) {

  if(attr->audio_format == 0
     && attr->multichannel_extension == 0
     && attr->lang_type == 0
     && attr->application_mode == 0
     && attr->quantization == 0
     && attr->sample_frequency == 0
     && attr->channels == 0
     && attr->lang_extension == 0
     && attr->unknown1 == 0
     && attr->unknown3 == 0) {
    printf("-- Unspecified --");
    return;
  }

  switch(attr->audio_format) {
  case 0:
    printf("ac3 ");
    if(attr->quantization != 3)
      printf("(please send a bug report) ac3 quant/drc not 3 (%d)", attr->quantization);
    break;
  case 1:
    printf("(please send a bug report) ");
    break;
  case 2:
    printf("mpeg1 ");
  case 3:
    printf("mpeg2ext ");
    switch(attr->quantization) {
      case 0:
        printf("no drc ");
        break;
      case 1:
        printf("drc ");
        break;
      default:
        printf("(please send a bug report) mpeg reserved quant/drc  (%d)", attr->quantization);
    }
    break;
  case 4:
    printf("lpcm ");
    switch(attr->quantization) {
      case 0:
        printf("16bit ");
        break;
      case 1:
        printf("20bit ");
        break;
      case 2:
        printf("24bit ");
        break;
      case 3:
        printf("(please send a bug report) lpcm reserved quant/drc  (%d)", attr->quantization);
      break;
    }
    break;
  case 5:
    printf("(please send a bug report) ");
    break;
  case 6:
    printf("dts ");
    if(attr->quantization != 3)
      printf("(please send a bug report) dts quant/drc not 3 (%d)", attr->quantization);
    break;
  default:
    printf("(please send a bug report) ");
  }

  if(attr->multichannel_extension)
    printf("multichannel_extension ");

  switch(attr->lang_type) {
  case 0:
    // not specified
    assert(attr->lang_code == 0 || attr->lang_code == 0xffff);
    break;
  case 1:
    printf("%c%c ", attr->lang_code>>8, attr->lang_code & 0xff);
    break;
  default:
    printf("(please send a bug report) ");
  }

  switch(attr->application_mode) {
  case 0:
    // not specified
    break;
  case 1:
    printf("karaoke mode ");
    break;
  case 2:
    printf("surround sound mode ");
    break;
  default:
    printf("(please send a bug report) ");
  }

  switch(attr->quantization) {
  case 0:
    printf("16bit ");
    break;
  case 1:
    printf("20bit ");
    break;
  case 2:
    printf("24bit ");
    break;
  case 3:
    printf("drc ");
    break;
  default:
    printf("(please send a bug report) ");
  }

  switch(attr->sample_frequency) {
  case 0:
    printf("48kHz ");
    break;
  case 1:
    printf("??kHz ");
    break;
  default:
    printf("sample_frequency %i (please send a bug report) ",
	   attr->sample_frequency);
  }

  printf("%dCh ", attr->channels + 1);

  switch(attr->lang_extension) {
  case 0:
    printf("Not specified ");
    break;
  case 1: // Normal audio
    printf("Normal Caption ");
    break;
  case 2: // visually impaired
    printf("Audio for visually impaired ");
    break;
  case 3: // Directors 1
    printf("Director's comments 1 ");
    break;
  case 4: // Directors 2
    printf("Director's comments 2 ");
    break;
    //case 4: // Music score ?
  default:
    printf("(please send a bug report) ");
  }

  printf("%d ", attr->unknown1);
  printf("%d ", attr->unknown3);
}

static void ifo_print_subp_attributes(int level, subp_attr_t *attr) {

  if(attr->type == 0
     && attr->lang_code == 0
     && attr->zero1 == 0
     && attr->zero2 == 0
     && attr->lang_extension== 0) {
    printf("-- Unspecified --");
    return;
  }

  printf("type %02x ", attr->type);

  if(isalpha((int)(attr->lang_code >> 8))
     && isalpha((int)(attr->lang_code & 0xff))) {
    printf("%c%c ", attr->lang_code >> 8, attr->lang_code & 0xff);
  } else {
    printf("%02x%02x ", 0xff & (unsigned)(attr->lang_code >> 8),
	   0xff & (unsigned)(attr->lang_code & 0xff));
  }

  printf("%d ", attr->zero1);
  printf("%d ", attr->zero2);

  switch(attr->lang_extension) {
  case 0:
    printf("Not specified ");
    break;
  case 1:
    printf("Caption with normal size character ");
    break;
  case 2:
    printf("Caption with bigger size character ");
    break;
  case 3:
    printf("Caption for children ");
    break;
  case 4:
    printf("reserved ");
    break;
  case 5:
    printf("Closed Caption with normal size character ");
    break;
  case 6:
    printf("Closed Caption with bigger size character ");
    break;
  case 7:
    printf("Closed Caption for children ");
    break;
  case 8:
    printf("reserved ");
    break;
  case 9:
    printf("Forced Caption");
    break;
  case 10:
    printf("reserved ");
    break;
  case 11:
    printf("reserved ");
    break;
  case 12:
    printf("reserved ");
    break;
  case 13:
    printf("Director's comments with normal size character ");
    break;
  case 14:
    printf("Director's comments with bigger size character ");
    break;
  case 15:
    printf("Director's comments for children ");
    break;
  default:
    printf("(please send a bug report) ");
  }

}


static void ifoPrint_USER_OPS(user_ops_t *user_ops) {
  uint32_t uops;
  unsigned char *ptr = (unsigned char *)user_ops;

  uops  = (*ptr++ << 24);
  uops |= (*ptr++ << 16);
  uops |= (*ptr++ << 8);
  uops |= (*ptr++);

  if(uops == 0) {
    printf("None\n");
  } else if(uops == 0x01ffffff) {
    printf("All\n");
  } else {
    if(user_ops->title_or_time_play)
      printf("Title or Time Play, ");
    if(user_ops->chapter_search_or_play)
      printf("Chapter Search or Play, ");
    if(user_ops->title_play)
      printf("Title Play, ");
    if(user_ops->stop)
      printf("Stop, ");
    if(user_ops->go_up)
      printf("Go Up, ");
    if(user_ops->time_or_chapter_search)
      printf("Time or Chapter Search, ");
    if(user_ops->prev_or_top_pg_search)
      printf("Prev or Top PG Search, ");
    if(user_ops->next_pg_search)
      printf("Next PG Search, ");
    if(user_ops->forward_scan)
      printf("Forward Scan, ");
    if(user_ops->backward_scan)
      printf("Backward Scan, ");
    if(user_ops->title_menu_call)
      printf("Title Menu Call, ");
    if(user_ops->root_menu_call)
      printf("Root Menu Call, ");
    if(user_ops->subpic_menu_call)
      printf("SubPic Menu Call, ");
    if(user_ops->audio_menu_call)
      printf("Audio Menu Call, ");
    if(user_ops->angle_menu_call)
      printf("Angle Menu Call, ");
    if(user_ops->chapter_menu_call)
      printf("Chapter Menu Call, ");
    if(user_ops->resume)
      printf("Resume, ");
    if(user_ops->button_select_or_activate)
      printf("Button Select or Activate, ");
    if(user_ops->still_off)
      printf("Still Off, ");
    if(user_ops->pause_on)
      printf("Pause On, ");
    if(user_ops->audio_stream_change)
      printf("Audio Stream Change, ");
    if(user_ops->subpic_stream_change)
      printf("SubPic Stream Change, ");
    if(user_ops->angle_change)
      printf("Angle Change, ");
    if(user_ops->karaoke_audio_pres_mode_change)
      printf("Karaoke Audio Pres Mode Change, ");
    if(user_ops->video_pres_mode_change)
      printf("Video Pres Mode Change, ");
    printf("\n");
  }
}


static void ifoPrint_VMGI_MAT(vmgi_mat_t *vmgi_mat) {

  printf("VMG Identifier: %.12s\n", vmgi_mat->vmg_identifier);
  printf("Last Sector of VMG: %08x\n", vmgi_mat->vmg_last_sector);
  printf("Last Sector of VMGI: %08x\n", vmgi_mat->vmgi_last_sector);
  printf("Specification version number: %01x.%01x\n",
	 vmgi_mat->specification_version >> 4,
	 vmgi_mat->specification_version & 0xf);
  /* Byte 2 of 'VMG Category' (00xx0000) is the Region Code */
  printf("VMG Category: %08x (Region Code=%02x)\n", vmgi_mat->vmg_category, ((vmgi_mat->vmg_category >> 16) & 0xff) ^0xff);
  printf("VMG Number of Volumes: %i\n", vmgi_mat->vmg_nr_of_volumes);
  printf("VMG This Volume: %i\n", vmgi_mat->vmg_this_volume_nr);
  printf("Disc side %i\n", vmgi_mat->disc_side);
  printf("VMG Number of Title Sets %i\n", vmgi_mat->vmg_nr_of_title_sets);
  printf("Provider ID: %.32s\n", vmgi_mat->provider_identifier);
  printf("VMG POS Code: %08x", (uint32_t)(vmgi_mat->vmg_pos_code >> 32));
  printf("%08x\n", (uint32_t)vmgi_mat->vmg_pos_code);
  printf("End byte of VMGI_MAT: %08x\n", vmgi_mat->vmgi_last_byte);
  printf("Start byte of First Play PGC (FP PGC): %08x\n",
	 vmgi_mat->first_play_pgc);
  printf("Start sector of VMGM_VOBS: %08x\n", vmgi_mat->vmgm_vobs);
  printf("Start sector of TT_SRPT: %08x\n", vmgi_mat->tt_srpt);
  printf("Start sector of VMGM_PGCI_UT: %08x\n", vmgi_mat->vmgm_pgci_ut);
  printf("Start sector of PTL_MAIT: %08x\n", vmgi_mat->ptl_mait);
  printf("Start sector of VTS_ATRT: %08x\n", vmgi_mat->vts_atrt);
  printf("Start sector of TXTDT_MG: %08x\n", vmgi_mat->txtdt_mgi);
  printf("Start sector of VMGM_C_ADT: %08x\n", vmgi_mat->vmgm_c_adt);
  printf("Start sector of VMGM_VOBU_ADMAP: %08x\n",
	 vmgi_mat->vmgm_vobu_admap);
  printf("Video attributes of VMGM_VOBS: ");
  ifo_print_video_attributes(5, &vmgi_mat->vmgm_video_attr);
  printf("\n");
  printf("VMGM Number of Audio attributes: %i\n",
	 vmgi_mat->nr_of_vmgm_audio_streams);
  if(vmgi_mat->nr_of_vmgm_audio_streams > 0) {
    printf("\tstream %i status: ", 1);
    ifo_print_audio_attributes(5, &vmgi_mat->vmgm_audio_attr);
    printf("\n");
  }
  printf("VMGM Number of Sub-picture attributes: %i\n",
	 vmgi_mat->nr_of_vmgm_subp_streams);
  if(vmgi_mat->nr_of_vmgm_subp_streams > 0) {
    printf("\tstream %2i status: ", 1);
    ifo_print_subp_attributes(5, &vmgi_mat->vmgm_subp_attr);
    printf("\n");
  }
}


static void ifoPrint_VTSI_MAT(vtsi_mat_t *vtsi_mat) {
  int i;

  printf("VTS Identifier: %.12s\n", vtsi_mat->vts_identifier);
  printf("Last Sector of VTS: %08x\n", vtsi_mat->vts_last_sector);
  printf("Last Sector of VTSI: %08x\n", vtsi_mat->vtsi_last_sector);
  printf("Specification version number: %01x.%01x\n",
	 vtsi_mat->specification_version>>4,
	 vtsi_mat->specification_version&0xf);
  printf("VTS Category: %08x\n", vtsi_mat->vts_category);
  printf("End byte of VTSI_MAT: %08x\n", vtsi_mat->vtsi_last_byte);
  printf("Start sector of VTSM_VOBS:  %08x\n", vtsi_mat->vtsm_vobs);
  printf("Start sector of VTSTT_VOBS: %08x\n", vtsi_mat->vtstt_vobs);
  printf("Start sector of VTS_PTT_SRPT: %08x\n", vtsi_mat->vts_ptt_srpt);
  printf("Start sector of VTS_PGCIT:    %08x\n", vtsi_mat->vts_pgcit);
  printf("Start sector of VTSM_PGCI_UT: %08x\n", vtsi_mat->vtsm_pgci_ut);
  printf("Start sector of VTS_TMAPT:    %08x\n", vtsi_mat->vts_tmapt);
  printf("Start sector of VTSM_C_ADT:      %08x\n", vtsi_mat->vtsm_c_adt);
  printf("Start sector of VTSM_VOBU_ADMAP: %08x\n",vtsi_mat->vtsm_vobu_admap);
  printf("Start sector of VTS_C_ADT:       %08x\n", vtsi_mat->vts_c_adt);
  printf("Start sector of VTS_VOBU_ADMAP:  %08x\n", vtsi_mat->vts_vobu_admap);

  printf("Video attributes of VTSM_VOBS: ");
  ifo_print_video_attributes(5, &vtsi_mat->vtsm_video_attr);
  printf("\n");

  printf("VTSM Number of Audio attributes: %i\n",
	 vtsi_mat->nr_of_vtsm_audio_streams);
  if(vtsi_mat->nr_of_vtsm_audio_streams > 0) {
    printf("\tstream %i status: ", 1);
    ifo_print_audio_attributes(5, &vtsi_mat->vtsm_audio_attr);
    printf("\n");
  }

  printf("VTSM Number of Sub-picture attributes: %i\n",
	 vtsi_mat->nr_of_vtsm_subp_streams);
  if(vtsi_mat->nr_of_vtsm_subp_streams > 0) {
    printf("\tstream %2i status: ", 1);
    ifo_print_subp_attributes(5, &vtsi_mat->vtsm_subp_attr);
    printf("\n");
  }

  printf("Video attributes of VTS_VOBS: ");
  ifo_print_video_attributes(5, &vtsi_mat->vts_video_attr);
  printf("\n");

  printf("VTS Number of Audio attributes: %i\n",
	 vtsi_mat->nr_of_vts_audio_streams);
  for(i = 0; i < vtsi_mat->nr_of_vts_audio_streams; i++) {
    printf("\tstream %i status: ", i);
    ifo_print_audio_attributes(5, &vtsi_mat->vts_audio_attr[i]);
    printf("\n");
  }

  printf("VTS Number of Subpicture attributes: %i\n",
	 vtsi_mat->nr_of_vts_subp_streams);
  for(i = 0; i < vtsi_mat->nr_of_vts_subp_streams; i++) {
    printf("\tstream %2i status: ", i);
    ifo_print_subp_attributes(5, &vtsi_mat->vts_subp_attr[i]);
    printf("\n");
  }
}


static void ifoPrint_PGC_COMMAND_TBL(pgc_command_tbl_t *cmd_tbl) {
  int i;

  if(cmd_tbl == NULL) {
    printf("No Command table present\n");
    return;
  }

  printf("Number of Pre commands: %i\n", cmd_tbl->nr_of_pre);
  for(i = 0; i < cmd_tbl->nr_of_pre; i++) {
    ifo_print_cmd(i, &cmd_tbl->pre_cmds[i]);
  }

  printf("Number of Post commands: %i\n", cmd_tbl->nr_of_post);
  for(i = 0; i < cmd_tbl->nr_of_post; i++) {
    ifo_print_cmd(i, &cmd_tbl->post_cmds[i]);
  }

  printf("Number of Cell commands: %i\n", cmd_tbl->nr_of_cell);
  for(i = 0; i < cmd_tbl->nr_of_cell; i++) {
    ifo_print_cmd(i, &cmd_tbl->cell_cmds[i]);
  }
}


static void ifoPrint_PGC_PROGRAM_MAP(pgc_program_map_t *program_map, int nr) {
  int i;

  if(program_map == NULL) {
    printf("No Program map present\n");
    return;
  }

  for(i = 0; i < nr; i++) {
    printf("Program %3i Entry Cell: %3i\n", i + 1, program_map[i]);
  }
}


static void ifoPrint_CELL_PLAYBACK(cell_playback_t *cell_playback, int nr) {
  int i;

  if(cell_playback == NULL) {
    printf("No Cell Playback info present\n");
    return;
  }

  for(i=0;i<nr;i++) {
    printf("Cell: %3i ", i + 1);

    dvdread_print_time(&cell_playback[i].playback_time);
    printf("\t");

    if(cell_playback[i].block_mode || cell_playback[i].block_type) {
      const char *s;
      switch(cell_playback[i].block_mode) {
      case 0:
	s = "not a"; break;
      case 1:
	s = "the first"; break;
      case 2:
      default:
	s = ""; break;
      case 3:
	s = "last"; break;
      }
      printf("%s cell in the block ", s);

      switch(cell_playback[i].block_type) {
      case 0:
	printf("not part of the block ");
	break;
      case 1:
	printf("angle block ");
	break;
      case 2:
      case 3:
        printf("(send bug report) ");
	break;
      }
    }
    if(cell_playback[i].seamless_play)
      printf("presented seamlessly ");
    if(cell_playback[i].interleaved)
      printf("cell is interleaved ");
    if(cell_playback[i].stc_discontinuity)
      printf("STC_discontinuty ");
    if(cell_playback[i].seamless_angle)
      printf("only seamless angle ");
    if(cell_playback[i].playback_mode)
      printf("only still VOBUs ");
    if(cell_playback[i].restricted)
      printf("restricted cell ");
    if(cell_playback[i].unknown2)
      printf("Unknown 0x%x ", cell_playback[i].unknown2);
    if(cell_playback[i].still_time)
      printf("still time %d ", cell_playback[i].still_time);
    if(cell_playback[i].cell_cmd_nr)
      printf("cell command %d", cell_playback[i].cell_cmd_nr);

    printf("\n\tStart sector: %08x\tFirst ILVU end  sector: %08x\n",
	   cell_playback[i].first_sector,
	   cell_playback[i].first_ilvu_end_sector);
    printf("\tEnd   sector: %08x\tLast VOBU start sector: %08x\n",
	   cell_playback[i].last_sector,
	   cell_playback[i].last_vobu_start_sector);
  }
}

static void ifoPrint_CELL_POSITION(cell_position_t *cell_position, int nr) {
  int i;

  if(cell_position == NULL) {
    printf("No Cell Position info present\n");
    return;
  }

  for(i=0;i<nr;i++) {
    printf("Cell: %3i has VOB ID: %3i, Cell ID: %3i\n", i + 1,
	   cell_position[i].vob_id_nr, cell_position[i].cell_nr);
  }
}


static void ifoPrint_PGC(pgc_t *pgc) {
  int i;

  if (!pgc) {
    printf("None\n");
    return;
  }
  printf("Number of Programs: %i\n", pgc->nr_of_programs);
  printf("Number of Cells: %i\n", pgc->nr_of_cells);
  /* Check that time is 0:0:0:0 also if nr_of_programs==0 */
  printf("Playback time: ");
  dvdread_print_time(&pgc->playback_time); printf("\n");

  /* If no programs/no time then does this mean anything? */
  printf("Prohibited user operations: ");
  ifoPrint_USER_OPS(&pgc->prohibited_ops);

    for(i = 0; i < 8; i++) {
      if(pgc->audio_control[i] & 0x8000) { /* The 'is present' bit */
	printf("Audio stream %i control: %04x\n",
	       i, pgc->audio_control[i]);
      }
    }

  for(i = 0; i < 32; i++) {
    if(pgc->subp_control[i] & 0x80000000) { /* The 'is present' bit */
      printf("Subpicture stream %2i control: %08x: 4:3=%d, Wide=%d, Letterbox=%d, Pan-Scan=%d\n",
	     i, pgc->subp_control[i],
	     (pgc->subp_control[i] >>24) & 0x1f,
	     (pgc->subp_control[i] >>16) & 0x1f,
	     (pgc->subp_control[i] >>8) & 0x1f,
	     (pgc->subp_control[i] ) & 0x1f);
    }
  }

  printf("Next PGC number: %i\n", pgc->next_pgc_nr);
  printf("Prev PGC number: %i\n", pgc->prev_pgc_nr);
  printf("GoUp PGC number: %i\n", pgc->goup_pgc_nr);
  if(pgc->nr_of_programs != 0) {
    printf("Still time: %i seconds (255=inf)\n", pgc->still_time);
    printf("PG Playback mode %02x\n", pgc->pg_playback_mode);
  }

  if(pgc->nr_of_programs != 0) {
    for(i = 0; i < 16; i++) {
      printf("Color %2i: %08x\n", i, pgc->palette[i]);
    }
  }

  /* Memory offsets to div. tables. */
  ifoPrint_PGC_COMMAND_TBL(pgc->command_tbl);
  ifoPrint_PGC_PROGRAM_MAP(pgc->program_map, pgc->nr_of_programs);
  ifoPrint_CELL_PLAYBACK(pgc->cell_playback, pgc->nr_of_cells);
  ifoPrint_CELL_POSITION(pgc->cell_position, pgc->nr_of_cells);
}


static void ifoPrint_TT_SRPT(tt_srpt_t *tt_srpt) {
  int i;

  printf("Number of TitleTrack search pointers: %i\n",
	 tt_srpt->nr_of_srpts);
  for(i=0;i<tt_srpt->nr_of_srpts;i++) {
    printf("Title Track index %i\n", i + 1);
    printf("\tTitle set number (VTS): %i",
	   tt_srpt->title[i].title_set_nr);
    printf("\tVTS_TTN: %i\n", tt_srpt->title[i].vts_ttn);
    printf("\tNumber of PTTs: %i\n", tt_srpt->title[i].nr_of_ptts);
    printf("\tNumber of angles: %i\n",
	   tt_srpt->title[i].nr_of_angles);

    printf("\tTitle playback type: (%02x)\n",
	   *(uint8_t *)&(tt_srpt->title[i].pb_ty));
    printf("\t\t%s\n",
           tt_srpt->title[i].pb_ty.multi_or_random_pgc_title ? "Random or Shuffle" : "Sequential");
    if (tt_srpt->title[i].pb_ty.jlc_exists_in_cell_cmd) printf("\t\tJump/Link/Call exists in cell cmd\n");
    if (tt_srpt->title[i].pb_ty.jlc_exists_in_prepost_cmd) printf("\t\tJump/Link/Call exists in pre/post cmd\n");
    if (tt_srpt->title[i].pb_ty.jlc_exists_in_button_cmd) printf("\t\tJump/Link/Call exists in button cmd\n");
    if (tt_srpt->title[i].pb_ty.jlc_exists_in_tt_dom) printf("\t\tJump/Link/Call exists in tt_dom cmd\n");
    printf("\t\tTitle or time play:%d\n", tt_srpt->title[i].pb_ty.title_or_time_play);
    printf("\t\tChapter search or play:%d\n", tt_srpt->title[i].pb_ty.chapter_search_or_play);

    printf("\tParental ID field: %04x\n",
	   tt_srpt->title[i].parental_id);
    printf("\tTitle set starting sector %08x\n",
	   tt_srpt->title[i].title_set_sector);
  }
}


static void ifoPrint_VTS_PTT_SRPT(vts_ptt_srpt_t *vts_ptt_srpt) {
  int i, j;
  printf(" nr_of_srpts %i last byte %i\n",
	 vts_ptt_srpt->nr_of_srpts,
	 vts_ptt_srpt->last_byte);
  for(i=0;i<vts_ptt_srpt->nr_of_srpts;i++) {
    for(j=0;j<vts_ptt_srpt->title[i].nr_of_ptts;j++) {
      printf("VTS_PTT_SRPT - Title %3i part %3i: PGC: %3i PG: %3i\n",
	     i + 1, j + 1,
	     vts_ptt_srpt->title[i].ptt[j].pgcn,
	     vts_ptt_srpt->title[i].ptt[j].pgn );
    }
  }
}


static void hexdump(uint8_t *ptr, int len) {
  while(len--)
    printf("%02x ", *ptr++);
}

static void ifoPrint_PTL_MAIT(ptl_mait_t *ptl_mait) {
  int i, j;

  printf("Number of Countries: %i\n", ptl_mait->nr_of_countries);
  printf("Number of VTSs: %i\n", ptl_mait->nr_of_vtss);
  //printf("Last byte: %i\n", ptl_mait->last_byte);

  for(i = 0; i < ptl_mait->nr_of_countries; i++) {
    printf("Country code: %c%c\n",
	   ptl_mait->countries[i].country_code >> 8,
	   ptl_mait->countries[i].country_code & 0xff);
    /*
      printf("Start byte: %04x %i\n",
      ptl_mait->countries[i].pf_ptl_mai_start_byte,
      ptl_mait->countries[i].pf_ptl_mai_start_byte);
    */
    /* This seems to be pointing at a array with 8 2byte fields per VTS
       ? and one extra for the menu? always an odd number of VTSs on
       all the dics I tested so it might be padding to even also.
       If it is for the menu it probably the first entry.  */
    for(j=0;j<8;j++) {
      hexdump( (uint8_t *)ptl_mait->countries - PTL_MAIT_COUNTRY_SIZE
	       + ptl_mait->countries[i].pf_ptl_mai_start_byte
	       + j*(ptl_mait->nr_of_vtss+1)*2, (ptl_mait->nr_of_vtss+1)*2);
      printf("\n");
    }
  }
}

static void ifoPrint_VTS_TMAPT(vts_tmapt_t *vts_tmapt) {
  unsigned int timeunit;
  int i, j;

  printf("Number of VTS_TMAPS: %i\n", vts_tmapt->nr_of_tmaps);
  printf("Last byte: %i\n", vts_tmapt->last_byte);

  for(i = 0; i < vts_tmapt->nr_of_tmaps; i++) {
    printf("TMAP %i (number matches title PGC number.)\n", i + 1);
    printf("  offset %d relative to VTS_TMAPTI\n", vts_tmapt->tmap_offset[i]);
    printf("  Time unit (seconds): %i\n", vts_tmapt->tmap[i].tmu);
    printf("  Number of entries: %i\n", vts_tmapt->tmap[i].nr_of_entries);
    timeunit = vts_tmapt->tmap[i].tmu;
    for(j = 0; j < vts_tmapt->tmap[i].nr_of_entries; j++) {
      unsigned int ac_time = timeunit * (j + 1);
      printf("Time: %2i:%02i:%02i  VOBU Sector: 0x%08x %s\n",
             ac_time / (60 * 60), (ac_time / 60) % 60, ac_time % 60,
             vts_tmapt->tmap[i].map_ent[j] & 0x7fffffff,
             (vts_tmapt->tmap[i].map_ent[j] >> 31) ? "discontinuity" : "");
    }
  }
}

static void ifoPrint_C_ADT(c_adt_t *c_adt) {
  int i, entries;

  printf("Number of VOBs in this VOBS: %i\n", c_adt->nr_of_vobs);
  //entries = c_adt->nr_of_vobs;
  entries = (c_adt->last_byte + 1 - C_ADT_SIZE)/sizeof(c_adt_t);

  for(i = 0; i < entries; i++) {
    printf("VOB ID: %3i, Cell ID: %3i   ",
	   c_adt->cell_adr_table[i].vob_id, c_adt->cell_adr_table[i].cell_id);
    printf("Sector (first): 0x%08x   (last): 0x%08x\n",
	   c_adt->cell_adr_table[i].start_sector,
	   c_adt->cell_adr_table[i].last_sector);
  }
}


static void ifoPrint_VOBU_ADMAP(vobu_admap_t *vobu_admap) {
  int i, entries;

  entries = (vobu_admap->last_byte + 1 - VOBU_ADMAP_SIZE)/4;
  for(i = 0; i < entries; i++) {
    printf("VOBU %5i  First sector: 0x%08x\n", i + 1,
	   vobu_admap->vobu_start_sectors[i]);
  }
}

static const char *ifo_print_menu_name(int type) {
  const char *menu_name;
  menu_name="";
  switch (type) {
  case 2:
    menu_name="Title";
    break;
  case 3:
    menu_name = "Root";
    break;
  case 4:
    menu_name = "Sub-Picture";
    break;
  case 5:
    menu_name = "Audio";
    break;
  case 6:
    menu_name = "Angle";
    break;
  case 7:
    menu_name = "PTT (Chapter)";
    break;
  default:
    menu_name = "Unknown";
    break;
  }
  return &menu_name[0];
}

/* pgc_type=1 for menu, 0 for title. */
static void ifoPrint_PGCIT(pgcit_t *pgcit, int pgc_type) {
  int i;

  printf("\nNumber of Program Chains: %3i\n", pgcit->nr_of_pgci_srp);
  for(i = 0; i < pgcit->nr_of_pgci_srp; i++) {
    printf("\nProgram (PGC): %3i\n", i + 1);
    if (pgc_type) {
       printf("PGC Category: Entry PGC %d, Menu Type=0x%02x:%s (Entry id 0x%02x), ",
            pgcit->pgci_srp[i].entry_id >> 7,
            pgcit->pgci_srp[i].entry_id & 0xf,
            ifo_print_menu_name(pgcit->pgci_srp[i].entry_id & 0xf),
            pgcit->pgci_srp[i].entry_id);
    } else {
       printf("PGC Category: %s VTS_TTN:0x%02x (Entry id 0x%02x), ",
            pgcit->pgci_srp[i].entry_id >> 7 ? "At Start of" : "During",
            pgcit->pgci_srp[i].entry_id & 0xf,
            pgcit->pgci_srp[i].entry_id);
    }
    printf("Parental ID mask 0x%04x\n", pgcit->pgci_srp[i].ptl_id_mask);
    ifoPrint_PGC(pgcit->pgci_srp[i].pgc);
  }
}


static void ifoPrint_PGCI_UT(pgci_ut_t *pgci_ut) {
  int i, menu;

  printf("Number of Menu Language Units (PGCI_LU): %3i\n", pgci_ut->nr_of_lus);
  for(i = 0; i < pgci_ut->nr_of_lus; i++) {
    printf("\nMenu Language Unit %d\n", i+1);
    printf("\nMenu Language Code: %c%c\n",
	   pgci_ut->lu[i].lang_code >> 8,
	   pgci_ut->lu[i].lang_code & 0xff);

    menu = pgci_ut->lu[i].exists;
    printf("Menu Existence: %02x: ", menu);
    if (menu == 0) {
      printf("No menus ");
    }
    if (menu & 0x80) {
      printf("Root ");
      menu^=0x80;
    }
    if (menu & 0x40) {
      printf("Sub-Picture ");
      menu^=0x40;
    }
    if (menu & 0x20) {
      printf("Audio ");
      menu^=0x20;
    }
    if (menu & 0x10) {
      printf("Angle ");
      menu^=0x10;
    }
    if (menu & 0x08) {
      printf("PTT ");
      menu^=0x08;
    }
    if (menu > 0) {
      printf("Unknown extra menus ");
      menu^=0x08;
    }
    printf("\n");
    ifoPrint_PGCIT(pgci_ut->lu[i].pgcit, 1);
  }
}


static void ifoPrint_VTS_ATTRIBUTES(vts_attributes_t *vts_attributes) {
  int i;

  printf("VTS_CAT Application type: %08x\n", vts_attributes->vts_cat);

  printf("Video attributes of VTSM_VOBS: ");
  ifo_print_video_attributes(5, &vts_attributes->vtsm_vobs_attr);
  printf("\n");
  printf("Number of Audio streams: %i\n",
	 vts_attributes->nr_of_vtsm_audio_streams);
  if(vts_attributes->nr_of_vtsm_audio_streams > 0) {
    printf("\tstream %i attributes: ", 1);
    ifo_print_audio_attributes(5, &vts_attributes->vtsm_audio_attr);
    printf("\n");
  }
  printf("Number of Subpicture streams: %i\n",
	 vts_attributes->nr_of_vtsm_subp_streams);
  if(vts_attributes->nr_of_vtsm_subp_streams > 0) {
    printf("\tstream %2i attributes: ", 1);
    ifo_print_subp_attributes(5, &vts_attributes->vtsm_subp_attr);
    printf("\n");
  }

  printf("Video attributes of VTSTT_VOBS: ");
  ifo_print_video_attributes(5, &vts_attributes->vtstt_vobs_video_attr);
  printf("\n");
  printf("Number of Audio streams: %i\n",
	 vts_attributes->nr_of_vtstt_audio_streams);
  for(i = 0; i < vts_attributes->nr_of_vtstt_audio_streams; i++) {
    printf("\tstream %i attributes: ", i);
    ifo_print_audio_attributes(5, &vts_attributes->vtstt_audio_attr[i]);
    printf("\n");
  }

  printf("Number of Subpicture streams: %i\n",
	 vts_attributes->nr_of_vtstt_subp_streams);
  for(i = 0; i < vts_attributes->nr_of_vtstt_subp_streams; i++) {
    printf("\tstream %2i attributes: ", i);
    ifo_print_subp_attributes(5, &vts_attributes->vtstt_subp_attr[i]);
    printf("\n");
  }
}


static void ifoPrint_VTS_ATRT(vts_atrt_t *vts_atrt) {
  int i;

  printf("Number of Video Title Sets: %3i\n", vts_atrt->nr_of_vtss);
  for(i = 0; i < vts_atrt->nr_of_vtss; i++) {
    printf("\nVideo Title Set %i\n", i + 1);
    ifoPrint_VTS_ATTRIBUTES(&vts_atrt->vts[i]);
  }
}


void ifo_print(dvd_reader_t *dvd, int title) {
  ifo_handle_t *ifohandle;
  printf("Local ifo_print\n");
  ifohandle = ifoOpen(dvd, title);
  if(!ifohandle) {
    fprintf(stderr, "Can't open info file for title %d\n", title);
    return;
  }


  if(ifohandle->vmgi_mat) {

    printf("VMG top level\n-------------\n");
    ifoPrint_VMGI_MAT(ifohandle->vmgi_mat);

    printf("\nFirst Play PGC\n--------------\n");
    if(ifohandle->first_play_pgc)
      ifoPrint_PGC(ifohandle->first_play_pgc);
    else
      printf("No First Play PGC present\n");

    printf("\nTitle Track search pointer table\n");
    printf(  "------------------------------------------------\n");
    ifoPrint_TT_SRPT(ifohandle->tt_srpt);

    printf("\nMenu PGCI Unit table\n");
    printf(  "--------------------\n");
    if(ifohandle->pgci_ut) {
      ifoPrint_PGCI_UT(ifohandle->pgci_ut);
    } else {
      printf("No PGCI Unit table present\n");
    }

    printf("\nParental Management Information table\n");
    printf(  "------------------------------------\n");
    if(ifohandle->ptl_mait) {
      ifoPrint_PTL_MAIT(ifohandle->ptl_mait);
    } else {
      printf("No Parental Management Information present\n");
    }

    printf("\nVideo Title Set Attribute Table\n");
    printf(  "-------------------------------\n");
    ifoPrint_VTS_ATRT(ifohandle->vts_atrt);

    printf("\nText Data Manager Information\n");
    printf(  "-----------------------------\n");
    if(ifohandle->txtdt_mgi) {
      //ifo_print_TXTDT_MGI(&(vmgi->txtdt_mgi));
    } else {
      printf("No Text Data Manager Information present\n");
    }

    printf("\nMenu Cell Address table\n");
    printf(  "-----------------\n");
    if(ifohandle->menu_c_adt) {
      ifoPrint_C_ADT(ifohandle->menu_c_adt);
    } else {
      printf("No Menu Cell Address table present\n");
    }

    printf("\nVideo Manager Menu VOBU address map\n");
    printf(  "-----------------\n");
    if(ifohandle->menu_vobu_admap) {
      ifoPrint_VOBU_ADMAP(ifohandle->menu_vobu_admap);
    } else {
      printf("No Menu VOBU address map present\n");
    }
  }


  if(ifohandle->vtsi_mat) {

    printf("VTS top level\n-------------\n");
    ifoPrint_VTSI_MAT(ifohandle->vtsi_mat);

    printf("\nPart of Title Track search pointer table\n");
    printf(  "----------------------------------------------\n");
    ifoPrint_VTS_PTT_SRPT(ifohandle->vts_ptt_srpt);

    printf("\nPGCI Unit table\n");
    printf(  "--------------------\n");
    ifoPrint_PGCIT(ifohandle->vts_pgcit, 0);

    printf("\nMenu PGCI Unit table\n");
    printf(  "--------------------\n");
    if(ifohandle->pgci_ut) {
      ifoPrint_PGCI_UT(ifohandle->pgci_ut);
    } else {
      printf("No Menu PGCI Unit table present\n");
    }

    printf("\nVTS Time Map table\n");
    printf(  "-----------------\n");
    if(ifohandle->vts_tmapt) {
      ifoPrint_VTS_TMAPT(ifohandle->vts_tmapt);
    } else {
      printf("No VTS Time Map table present\n");
    }

    printf("\nMenu Cell Address table\n");
    printf(  "-----------------\n");
    if(ifohandle->menu_c_adt) {
      ifoPrint_C_ADT(ifohandle->menu_c_adt);
    } else {
      printf("No Cell Address table present\n");
    }

    printf("\nVideo Title Set Menu VOBU address map\n");
    printf(  "-----------------\n");
    if(ifohandle->menu_vobu_admap) {
      ifoPrint_VOBU_ADMAP(ifohandle->menu_vobu_admap);
    } else {
      printf("No Menu VOBU Address map present\n");
    }

    printf("\nCell Address table\n");
    printf(  "-----------------\n");
    ifoPrint_C_ADT(ifohandle->vts_c_adt);

    printf("\nVideo Title Set VOBU address map\n");
    printf(  "-----------------\n");
    ifoPrint_VOBU_ADMAP(ifohandle->vts_vobu_admap);
  }

  ifoClose(ifohandle);
}
