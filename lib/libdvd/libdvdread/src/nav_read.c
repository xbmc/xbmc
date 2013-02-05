/*
 * Copyright (C) 2000, 2001, 2002, 2003 Håkan Hjort <d95hjort@dtek.chalmers.se>
 *
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

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "bswap.h"
#include "dvdread/nav_types.h"
#include "dvdread/nav_read.h"
#include "dvdread_internal.h"
#include "dvdread/bitreader.h"

#define getbits_init dvdread_getbits_init
#define getbits dvdread_getbits

void navRead_PCI(pci_t *pci, unsigned char *buffer) {
  int32_t i, j;
  getbits_state_t state;
  if (!getbits_init(&state, buffer)) abort(); /* Passed NULL pointers */

  /* pci pci_gi */
  pci->pci_gi.nv_pck_lbn = getbits(&state, 32 );
  pci->pci_gi.vobu_cat = getbits(&state, 16 );
  pci->pci_gi.zero1 = getbits(&state, 16 );
  pci->pci_gi.vobu_uop_ctl.zero = getbits(&state, 7 );
  pci->pci_gi.vobu_uop_ctl.video_pres_mode_change         = getbits(&state, 1 );

  pci->pci_gi.vobu_uop_ctl.karaoke_audio_pres_mode_change = getbits(&state, 1 );
  pci->pci_gi.vobu_uop_ctl.angle_change                   = getbits(&state, 1 );
  pci->pci_gi.vobu_uop_ctl.subpic_stream_change           = getbits(&state, 1 );
  pci->pci_gi.vobu_uop_ctl.audio_stream_change            = getbits(&state, 1 );
  pci->pci_gi.vobu_uop_ctl.pause_on                       = getbits(&state, 1 );
  pci->pci_gi.vobu_uop_ctl.still_off                      = getbits(&state, 1 );
  pci->pci_gi.vobu_uop_ctl.button_select_or_activate      = getbits(&state, 1 );
  pci->pci_gi.vobu_uop_ctl.resume                         = getbits(&state, 1 );

  pci->pci_gi.vobu_uop_ctl.chapter_menu_call              = getbits(&state, 1 );
  pci->pci_gi.vobu_uop_ctl.angle_menu_call                = getbits(&state, 1 );
  pci->pci_gi.vobu_uop_ctl.audio_menu_call                = getbits(&state, 1 );
  pci->pci_gi.vobu_uop_ctl.subpic_menu_call               = getbits(&state, 1 );
  pci->pci_gi.vobu_uop_ctl.root_menu_call                 = getbits(&state, 1 );
  pci->pci_gi.vobu_uop_ctl.title_menu_call                = getbits(&state, 1 );
  pci->pci_gi.vobu_uop_ctl.backward_scan                  = getbits(&state, 1 );
  pci->pci_gi.vobu_uop_ctl.forward_scan                   = getbits(&state, 1 );

  pci->pci_gi.vobu_uop_ctl.next_pg_search                 = getbits(&state, 1 );
  pci->pci_gi.vobu_uop_ctl.prev_or_top_pg_search          = getbits(&state, 1 );
  pci->pci_gi.vobu_uop_ctl.time_or_chapter_search         = getbits(&state, 1 );
  pci->pci_gi.vobu_uop_ctl.go_up                          = getbits(&state, 1 );
  pci->pci_gi.vobu_uop_ctl.stop                           = getbits(&state, 1 );
  pci->pci_gi.vobu_uop_ctl.title_play                     = getbits(&state, 1 );
  pci->pci_gi.vobu_uop_ctl.chapter_search_or_play         = getbits(&state, 1 );
  pci->pci_gi.vobu_uop_ctl.title_or_time_play             = getbits(&state, 1 );
  pci->pci_gi.vobu_s_ptm = getbits(&state, 32 );
  pci->pci_gi.vobu_e_ptm = getbits(&state, 32 );
  pci->pci_gi.vobu_se_e_ptm = getbits(&state, 32 );
  pci->pci_gi.e_eltm.hour   = getbits(&state, 8 );
  pci->pci_gi.e_eltm.minute = getbits(&state, 8 );
  pci->pci_gi.e_eltm.second = getbits(&state, 8 );
  pci->pci_gi.e_eltm.frame_u = getbits(&state, 8 );
  for(i = 0; i < 32; i++)
    pci->pci_gi.vobu_isrc[i] = getbits(&state, 8 );

  /* pci nsml_agli */
  for(i = 0; i < 9; i++)
    pci->nsml_agli.nsml_agl_dsta[i] = getbits(&state, 32 );

  /* pci hli hli_gi */
  pci->hli.hl_gi.hli_ss = getbits(&state, 16 );
  pci->hli.hl_gi.hli_s_ptm = getbits(&state, 32 );
  pci->hli.hl_gi.hli_e_ptm = getbits(&state, 32 );
  pci->hli.hl_gi.btn_se_e_ptm = getbits(&state, 32 );
  pci->hli.hl_gi.zero1 = getbits(&state, 2 );
  pci->hli.hl_gi.btngr_ns = getbits(&state, 2 );
  pci->hli.hl_gi.zero2 = getbits(&state, 1 );
  pci->hli.hl_gi.btngr1_dsp_ty = getbits(&state, 3 );
  pci->hli.hl_gi.zero3 = getbits(&state, 1 );
  pci->hli.hl_gi.btngr2_dsp_ty = getbits(&state, 3 );
  pci->hli.hl_gi.zero4 = getbits(&state, 1 );
  pci->hli.hl_gi.btngr3_dsp_ty = getbits(&state, 3 );
  pci->hli.hl_gi.btn_ofn = getbits(&state, 8 );
  pci->hli.hl_gi.btn_ns = getbits(&state, 8 );
  pci->hli.hl_gi.nsl_btn_ns = getbits(&state, 8 );
  pci->hli.hl_gi.zero5 = getbits(&state, 8 );
  pci->hli.hl_gi.fosl_btnn = getbits(&state, 8 );
  pci->hli.hl_gi.foac_btnn = getbits(&state, 8 );

  /* pci hli btn_colit */
  for(i = 0; i < 3; i++)
    for(j = 0; j < 2; j++)
      pci->hli.btn_colit.btn_coli[i][j] = getbits(&state, 32 );

  /* NOTE: I've had to change the structure from the disk layout to get
   * the packing to work with Sun's Forte C compiler. */

  /* pci hli btni */
  for(i = 0; i < 36; i++) {
    pci->hli.btnit[i].btn_coln = getbits(&state, 2 );
    pci->hli.btnit[i].x_start = getbits(&state, 10 );
    pci->hli.btnit[i].zero1 = getbits(&state, 2 );
    pci->hli.btnit[i].x_end = getbits(&state, 10 );

    pci->hli.btnit[i].auto_action_mode = getbits(&state, 2 );
    pci->hli.btnit[i].y_start = getbits(&state, 10 );
    pci->hli.btnit[i].zero2 = getbits(&state, 2 );
    pci->hli.btnit[i].y_end = getbits(&state, 10 );

    pci->hli.btnit[i].zero3 = getbits(&state, 2 );
    pci->hli.btnit[i].up = getbits(&state, 6 );
    pci->hli.btnit[i].zero4 = getbits(&state, 2 );
    pci->hli.btnit[i].down = getbits(&state, 6 );
    pci->hli.btnit[i].zero5 = getbits(&state, 2 );
    pci->hli.btnit[i].left = getbits(&state, 6 );
    pci->hli.btnit[i].zero6 = getbits(&state, 2 );
    pci->hli.btnit[i].right = getbits(&state, 6 );
    /* pci vm_cmd */
    for(j = 0; j < 8; j++)
      pci->hli.btnit[i].cmd.bytes[j] = getbits(&state, 8 );
  }


#ifndef NDEBUG
  /* Asserts */

  /* pci pci gi */
  CHECK_VALUE(pci->pci_gi.zero1 == 0);

  /* pci hli hli_gi */
  CHECK_VALUE(pci->hli.hl_gi.zero1 == 0);
  CHECK_VALUE(pci->hli.hl_gi.zero2 == 0);
  CHECK_VALUE(pci->hli.hl_gi.zero3 == 0);
  CHECK_VALUE(pci->hli.hl_gi.zero4 == 0);
  CHECK_VALUE(pci->hli.hl_gi.zero5 == 0);

  /* Are there buttons defined here? */
  if((pci->hli.hl_gi.hli_ss & 0x03) != 0) {
    CHECK_VALUE(pci->hli.hl_gi.btn_ns != 0);
    CHECK_VALUE(pci->hli.hl_gi.btngr_ns != 0);
  } else {
    CHECK_VALUE((pci->hli.hl_gi.btn_ns != 0 && pci->hli.hl_gi.btngr_ns != 0)
                || (pci->hli.hl_gi.btn_ns == 0 && pci->hli.hl_gi.btngr_ns == 0));
  }

  /* pci hli btnit */
  for(i = 0; i < pci->hli.hl_gi.btngr_ns; i++) {
    for(j = 0; j < (36 / pci->hli.hl_gi.btngr_ns); j++) {
      int n = (36 / pci->hli.hl_gi.btngr_ns) * i + j;
      CHECK_VALUE(pci->hli.btnit[n].zero1 == 0);
      CHECK_VALUE(pci->hli.btnit[n].zero2 == 0);
      CHECK_VALUE(pci->hli.btnit[n].zero3 == 0);
      CHECK_VALUE(pci->hli.btnit[n].zero4 == 0);
      CHECK_VALUE(pci->hli.btnit[n].zero5 == 0);
      CHECK_VALUE(pci->hli.btnit[n].zero6 == 0);

      if (j < pci->hli.hl_gi.btn_ns) {
        CHECK_VALUE(pci->hli.btnit[n].x_start <= pci->hli.btnit[n].x_end);
        CHECK_VALUE(pci->hli.btnit[n].y_start <= pci->hli.btnit[n].y_end);
        CHECK_VALUE(pci->hli.btnit[n].up <= pci->hli.hl_gi.btn_ns);
        CHECK_VALUE(pci->hli.btnit[n].down <= pci->hli.hl_gi.btn_ns);
        CHECK_VALUE(pci->hli.btnit[n].left <= pci->hli.hl_gi.btn_ns);
        CHECK_VALUE(pci->hli.btnit[n].right <= pci->hli.hl_gi.btn_ns);
        /* vmcmd_verify(pci->hli.btnit[n].cmd); */
      } else {
        int k;
        CHECK_VALUE(pci->hli.btnit[n].btn_coln == 0);
        CHECK_VALUE(pci->hli.btnit[n].auto_action_mode == 0);
        CHECK_VALUE(pci->hli.btnit[n].x_start == 0);
        CHECK_VALUE(pci->hli.btnit[n].y_start == 0);
        CHECK_VALUE(pci->hli.btnit[n].x_end == 0);
        CHECK_VALUE(pci->hli.btnit[n].y_end == 0);
        CHECK_VALUE(pci->hli.btnit[n].up == 0);
        CHECK_VALUE(pci->hli.btnit[n].down == 0);
        CHECK_VALUE(pci->hli.btnit[n].left == 0);
        CHECK_VALUE(pci->hli.btnit[n].right == 0);
        for (k = 0; k < 8; k++)
          CHECK_VALUE(pci->hli.btnit[n].cmd.bytes[k] == 0); /* CHECK_ZERO? */
      }
    }
  }
#endif /* !NDEBUG */
}

void navRead_DSI(dsi_t *dsi, unsigned char *buffer) {
  int i;
  getbits_state_t state;
  if (!getbits_init(&state, buffer)) abort(); /* Passed NULL pointers */

  /* dsi dsi gi */
  dsi->dsi_gi.nv_pck_scr = getbits(&state, 32 );
  dsi->dsi_gi.nv_pck_lbn = getbits(&state, 32 );
  dsi->dsi_gi.vobu_ea = getbits(&state, 32 );
  dsi->dsi_gi.vobu_1stref_ea = getbits(&state, 32 );
  dsi->dsi_gi.vobu_2ndref_ea = getbits(&state, 32 );
  dsi->dsi_gi.vobu_3rdref_ea = getbits(&state, 32 );
  dsi->dsi_gi.vobu_vob_idn = getbits(&state, 16 );
  dsi->dsi_gi.zero1 = getbits(&state, 8 );
  dsi->dsi_gi.vobu_c_idn = getbits(&state, 8 );
  dsi->dsi_gi.c_eltm.hour   = getbits(&state, 8 );
  dsi->dsi_gi.c_eltm.minute = getbits(&state, 8 );
  dsi->dsi_gi.c_eltm.second = getbits(&state, 8 );
  dsi->dsi_gi.c_eltm.frame_u = getbits(&state, 8 );

  /* dsi sml pbi */
  dsi->sml_pbi.category = getbits(&state, 16 );
  dsi->sml_pbi.ilvu_ea = getbits(&state, 32 );
  dsi->sml_pbi.ilvu_sa = getbits(&state, 32 );
  dsi->sml_pbi.size = getbits(&state, 16 );
  dsi->sml_pbi.vob_v_s_s_ptm = getbits(&state, 32 );
  dsi->sml_pbi.vob_v_e_e_ptm = getbits(&state, 32 );
  for(i = 0; i < 8; i++) {
    dsi->sml_pbi.vob_a[i].stp_ptm1 = getbits(&state, 32 );
    dsi->sml_pbi.vob_a[i].stp_ptm2 = getbits(&state, 32 );
    dsi->sml_pbi.vob_a[i].gap_len1 = getbits(&state, 32 );
    dsi->sml_pbi.vob_a[i].gap_len2 = getbits(&state, 32 );
  }

  /* dsi sml agli */
  for(i = 0; i < 9; i++) {
    dsi->sml_agli.data[ i ].address = getbits(&state, 32 );
    dsi->sml_agli.data[ i ].size = getbits(&state, 16 );
  }

  /* dsi vobu sri */
  dsi->vobu_sri.next_video = getbits(&state, 32 );
  for(i = 0; i < 19; i++)
    dsi->vobu_sri.fwda[i] = getbits(&state, 32 );
  dsi->vobu_sri.next_vobu = getbits(&state, 32 );
  dsi->vobu_sri.prev_vobu = getbits(&state, 32 );
  for(i = 0; i < 19; i++)
    dsi->vobu_sri.bwda[i] = getbits(&state, 32 );
  dsi->vobu_sri.prev_video = getbits(&state, 32 );

  /* dsi synci */
  for(i = 0; i < 8; i++)
    dsi->synci.a_synca[i] = getbits(&state, 16 );
  for(i = 0; i < 32; i++)
    dsi->synci.sp_synca[i] = getbits(&state, 32 );


  /* Asserts */

  /* dsi dsi gi */
  CHECK_VALUE(dsi->dsi_gi.zero1 == 0);
}
