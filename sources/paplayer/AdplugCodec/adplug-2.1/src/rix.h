/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2006 Simon Peter, <dn.tlp@gmx.net>, et al.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * rix.h - Softstar RIX OPL Format Player by palxex <palxex.ys168.com>
 *                                           BSPAL <BSPAL.ys168.com>
 */

#include "player.h"

class CrixPlayer: public CPlayer
{
 public:
  static CPlayer *factory(Copl *newopl);

  CrixPlayer(Copl *newopl);
  ~CrixPlayer();

  bool load(const std::string &filename, const CFileProvider &fp);
  bool update();
  void rewind(int subsong);
  float getrefresh();
  unsigned int getsubsongs();

  std::string gettype()
    { return std::string("Softstar RIX OPL Music Format"); };

 protected:	
  typedef struct {
    unsigned char v[14];
  } ADDT;

  int flag_mkf;
  unsigned char *file_buffer;
  unsigned char *buf_addr;  /* rix files' f_buffer */
  unsigned short f_buffer[300];//9C0h-C18h
  unsigned short a0b0_data2[11];
  unsigned char a0b0_data3[18];
  unsigned char a0b0_data4[18];
  unsigned char a0b0_data5[96];
  unsigned char addrs_head[96];
  unsigned short insbuf[28];
  unsigned short displace[11];
  ADDT reg_bufs[18];
  unsigned long pos,length;
  unsigned char index;

  static const unsigned char adflag[18];
  static const unsigned char reg_data[18];
  static const unsigned char ad_C0_offs[18];
  static const unsigned char modify[28];
  static const unsigned char bd_reg_data[124];
  static unsigned char for40reg[18];
  static unsigned short mus_time;
  unsigned int I,T;
  unsigned short mus_block;
  unsigned short ins_block;
  unsigned char rhythm;
  unsigned char music_on;
  unsigned char pause_flag;
  unsigned short band;
  unsigned char band_low;
  unsigned short e0_reg_flag;
  unsigned char bd_modify;
  int sustain;
  int play_end;

#define ad_08_reg() ad_bop(8,0)    /**/
  inline void ad_20_reg(unsigned short);              /**/
  inline void ad_40_reg(unsigned short);              /**/
  inline void ad_60_reg(unsigned short);              /**/
  inline void ad_80_reg(unsigned short);              /**/
  inline void ad_a0b0_reg(unsigned short);            /**/
  inline void ad_a0b0l_reg(unsigned short,unsigned short,unsigned short); /**/
  inline void ad_a0b0l_reg_(unsigned short,unsigned short,unsigned short); /**/
  inline void ad_bd_reg();                  /**/
  inline void ad_bop(unsigned short,unsigned short);                     /**/
  inline void ad_C0_reg(unsigned short);              /**/
  inline void ad_E0_reg(unsigned short);              /**/
  inline unsigned short ad_initial();                 /**/
  inline unsigned short ad_test();                    /**/
  inline void crc_trans(unsigned short,unsigned short);         /**/
  inline void data_initial();               /* done */
  inline void init();                       /**/
  inline void ins_to_reg(unsigned short,unsigned short*,unsigned short);  /**/
  inline void int_08h_entry();    /**/
  inline void music_ctrl();                 /**/
  inline void Pause();                      /**/
  inline void prepare_a0b0(unsigned short,unsigned short);      /**/
  inline void rix_90_pro(unsigned short);             /**/
  inline void rix_A0_pro(unsigned short,unsigned short);        /**/
  inline void rix_B0_pro(unsigned short,unsigned short);        /**/
  inline void rix_C0_pro(unsigned short,unsigned short);        /**/
  inline void rix_get_ins();                /**/
  inline unsigned short rix_proc();                   /**/
  inline void set_new_int();
  inline void switch_ad_bd(unsigned short);           /**/
};
