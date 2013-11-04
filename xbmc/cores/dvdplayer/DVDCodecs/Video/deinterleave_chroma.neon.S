#ifdef __ARM_NEON__
	.text
	.align 2
	.global deinterleave_chroma_neon
	.type   deinterleave_chroma_neon, %function

#define U			r0
#define V			r1
#define OWIDTH		r2
#define UV			r3
#define IWIDTH		r4
#define HEIGHT		r5
#define COUNT		r6
#define PAD			lr

deinterleave_chroma_neon:
    .fnstart
	push	{r4-r6,lr}							// save r4,r5,r6,lr in stack. 4 words
	ldr     IWIDTH, [sp, #4*4]					// copy 5-th stack element to r4 (IWIDTH)
	ldr     HEIGHT, [sp, #5*4]					// copy 6-th stack element to r5 (HEIGHT)

	sub		PAD,	IWIDTH,	OWIDTH, lsl #1		// compute right padding PAD = IWIDTH - OWIDTH*2
	cmp		HEIGHT,	#0							// compare HEIGHT == 0
process_line:
	pople	{r4-r6,pc}							// if last comparsion result <=0 then pop r4,r5,r6, lr from stack will be written to pc (that means exit and continue code called .S)
	mov		COUNT,	OWIDTH						// COUNT = OWIDTH
process_block:
	pld		[UV]								// Preload data from UV (signal processor that this data will most likely be loaded soon)
	vld2.u8	{d0,d1},[UV]!						// load 16-byte block from UV setting d0 as odd and d1 as even half-blocks, increment UV address by 16
	vst1.u8	{d0},	[U]!						// unload 8-byte block d0 to U, increment U address by 8
	vst1.u8	{d1},	[V]!						// unload 8-byte block d1 to V, increment V address by 8
	subs	COUNT,	COUNT,	#8					// COUNT = COUNT - 8, compare COUNT to 0
	bgt		process_block						// if last comparsion result > 0 then jump to label process_block

	add		UV,	UV,	PAD							// UV = UV + PAD, skip padding left in processed line from right
	subs	HEIGHT,	#1							// substract 1 from HEIGHT, compare HEIGHT to 0
	b		process_line						// jump to label process_line
	.fnend
#endif
