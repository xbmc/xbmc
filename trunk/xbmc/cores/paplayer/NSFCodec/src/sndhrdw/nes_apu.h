/*
** Nofrendo (c) 1998-2000 Matthew Conte (matt@conte.com)
**
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of version 2 of the GNU Library General 
** Public License as published by the Free Software Foundation.
**
** This program is distributed in the hope that it will be useful, 
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
** Library General Public License for more details.  To obtain a 
** copy of the GNU Library General Public License, write to the Free 
** Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** Any permitted reproduction of these routines, in whole or in part,
** must bear this legend.
**
**
** nes_apu.h
**
** NES APU emulation header file
** $Id: nes_apu.h,v 1.12 2000/07/04 04:54:48 matt Exp $
*/

#ifndef _NES_APU_H_
#define _NES_APU_H_

#ifdef __GNUC__
#define  INLINE      static inline
#elif defined(WIN32)
#define  INLINE      static __inline
#else
#define  INLINE      static
#endif

/* define this for realtime generated noise */
#define  REALTIME_NOISE

#define  APU_WRA0       0x4000
#define  APU_WRA1       0x4001
#define  APU_WRA2       0x4002
#define  APU_WRA3       0x4003
#define  APU_WRB0       0x4004
#define  APU_WRB1       0x4005
#define  APU_WRB2       0x4006
#define  APU_WRB3       0x4007
#define  APU_WRC0       0x4008
#define  APU_WRC2       0x400A
#define  APU_WRC3       0x400B
#define  APU_WRD0       0x400C
#define  APU_WRD2       0x400E
#define  APU_WRD3       0x400F
#define  APU_WRE0       0x4010
#define  APU_WRE1       0x4011
#define  APU_WRE2       0x4012
#define  APU_WRE3       0x4013

#define  APU_OAMDMA     0x4014
#define  APU_SMASK      0x4015
#define  APU_JOY0       0x4016
#define  APU_JOY1       0x4017

/* length of generated noise */
#define  APU_NOISE_32K  0x7FFF
#define  APU_NOISE_93   93

#define  APU_BASEFREQ   (NES_MASTER_CLOCK / 12)

/* to/from 16.16 fixed point */
#define  APU_TO_FIXED(x)    ((x) << 16)
#define  APU_FROM_FIXED(x)  ((x) >> 16)


/* channel structures */
/* As much data as possible is precalculated,
** to keep the sample processing as lean as possible
*/
 
typedef struct rectangle_s
{
   uint8 regs[4];

   boolean enabled;
   
   int32 phaseacc;
   int32 freq;
   int32 output_vol;
   boolean fixed_envelope;
   boolean holdnote;
   uint8 volume;

   int32 sweep_phase;
   int32 sweep_delay;
   boolean sweep_on;
   uint8 sweep_shifts;
   uint8 sweep_length;
   boolean sweep_inc;
   int32 freq_limit;

   int32 env_phase;
   int32 env_delay;
   uint8 env_vol;

   int vbl_length;
   uint8 adder;
   int duty_flip;
} rectangle_t;

/*
enum
{
   COUNTMODE_LOAD,
   COUNTMODE_COUNT
};
*/

typedef struct triangle_s
{
   uint8 regs[3];

   boolean enabled;

   int32 freq;
   int32 phaseacc;
   int32 output_vol;

   uint8 adder;

   boolean holdnote;
   boolean counter_started;
   /* quasi-hack */
   int write_latency;

//   boolean countmode;

   int vbl_length;
   int linear_length;

} triangle_t;


typedef struct noise_s
{
   uint8 regs[3];

   boolean enabled;

   int32 freq;
   int32 phaseacc;
   int32 output_vol;

   int32 env_phase;
   int32 env_delay;
   uint8 env_vol;
   boolean fixed_envelope;
   boolean holdnote;

   uint8 volume;

   int vbl_length;

#ifdef REALTIME_NOISE
   uint8 xor_tap;
#else
   boolean short_sample;
   int cur_pos;
#endif /* REALTIME_NOISE */
} noise_t;

typedef struct dmc_s
{
   uint8 regs[4];

   /* bodge for timestamp queue */
   boolean enabled;
   
   int32 freq;
   int32 phaseacc;
   int32 output_vol;

   uint32 address;
   uint32 cached_addr;
   int dma_length;
   int cached_dmalength;
   uint8 cur_byte;

   boolean looping;
   boolean irq_gen;
   boolean irq_occurred;

} dmc_t;

enum
{
   APU_FILTER_NONE,
   APU_FILTER_LOWPASS,
   APU_FILTER_WEIGHTED
};

typedef struct
{
   uint32 min_range, max_range;
   uint8 (*read_func)(uint32 address);
} apu_memread;

typedef struct
{
   uint32 min_range, max_range;
   void (*write_func)(uint32 address, uint8 value);
} apu_memwrite;

/* external sound chip stuff */
typedef struct apuext_s
{
   void  (*init)(void);
   void  (*shutdown)(void);
   void  (*reset)(void);
   int32 (*process)(void);
   apu_memread *mem_read;
   apu_memwrite *mem_write;
} apuext_t;


/* APU queue structure */
#define  APUQUEUE_SIZE  4096
#define  APUQUEUE_MASK  (APUQUEUE_SIZE - 1)

/* apu ring buffer member */
typedef struct apudata_s
{
   uint32 timestamp, address;
   uint8 value;
} apudata_t;


typedef struct apu_s
{
   rectangle_t rectangle[2];
   triangle_t triangle;
   noise_t noise;
   dmc_t dmc;
   uint8 enable_reg;

   apudata_t queue[APUQUEUE_SIZE];
   int q_head, q_tail;
   uint32 elapsed_cycles;

   void *buffer; /* pointer to output buffer */
   int num_samples;

   boolean mix_enable[6];
   int filter_type;

   int32 cycle_rate;

   int sample_rate;
   int sample_bits;
   int refresh_rate;

   void (*process)(void *buffer, int num_samples);

   /* external sound chip */
   apuext_t *ext;
} apu_t;


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Function prototypes */
extern apu_t *apu_create(int sample_rate, int refresh_rate, int sample_bits, boolean stereo);
extern void apu_destroy(apu_t *apu);
extern void apu_setext(apu_t *apu, apuext_t *ext);
extern void apu_setfilter(int filter_type);
extern void apu_process(void *buffer, int num_samples);
extern void apu_reset(void);
extern void apu_setchan(int chan, boolean enabled);
extern int32 apu_getcyclerate(void);
extern apu_t *apu_getcontext(void);
extern void apu_setcontext(apu_t* src_apu);

extern uint8 apu_read(uint32 address);
extern void apu_write(uint32 address, uint8 value);

/* for visualization */
extern void apu_getpcmdata(void **data, int *num_samples, int *sample_bits);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _NES_APU_H_ */

/*
** $Log: nes_apu.h,v $
** Revision 1.12  2000/07/04 04:54:48  matt
** minor changes that helped with MAME
**
** Revision 1.11  2000/07/03 02:18:53  matt
** much better external module exporting
**
** Revision 1.10  2000/06/26 05:00:37  matt
** cleanups
**
** Revision 1.9  2000/06/23 03:29:28  matt
** cleaned up external sound inteface
**
** Revision 1.8  2000/06/20 04:06:16  matt
** migrated external sound definition to apu module
**
** Revision 1.7  2000/06/20 00:07:35  matt
** added convenience members to apu_t struct
**
** Revision 1.6  2000/06/09 16:49:02  matt
** removed all floating point from sound generation
**
** Revision 1.5  2000/06/09 15:12:28  matt
** initial revision
**
*/

