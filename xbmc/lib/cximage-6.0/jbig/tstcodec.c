/*
 *  A sequence of test procedures for this JBIG implementation
 * 
 *  Run this test sequence after each modification on the JBIG library.
 *
 *  Markus Kuhn -- http://www.cl.cam.ac.uk/~mgk25/
 *
 *  $Id: tstcodec.c,v 1.14 2004-06-11 15:17:06+01 mgk25 Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "jbig.h"

#define TESTBUF_SIZE 400000L
#define TESTPIC_SIZE 477995L

#define FAILED "F\bFA\bAI\bIL\bLE\bED\bD"
#define PASSED "PASSED"

unsigned char *testbuf;
unsigned char *testpic;

long testbuf_len;


static void *checkedmalloc(size_t n)
{
  void *p;
  
  if ((p = malloc(n)) == NULL) {
    fprintf(stderr, "Sorry, not enough memory available!\n");
    exit(1);
  }
  
  return p;
}

static void testbuf_write(int v, void *dummy)
{
  if (testbuf_len < TESTBUF_SIZE)
    testbuf[testbuf_len++] = v;
  (void) dummy;
  return;
}


static void testbuf_writel(unsigned char *start, size_t len, void *dummy)
{
  if (testbuf_len < TESTBUF_SIZE) {
    if (testbuf_len + len < TESTBUF_SIZE)
      memcpy(testbuf + testbuf_len, start, len);
    else
      memcpy(testbuf + testbuf_len, start, TESTBUF_SIZE - testbuf_len);
  }
  testbuf_len += len;

#ifdef DEBUG
  {
    unsigned char *p;
    unsigned sum = 0;
    
    for (p = start; p - start < (ptrdiff_t) len; sum = (sum ^ *p++) << 1);
    printf("  testbuf_writel: %4d bytes, checksum %04x\n",
	   len, sum & 0xffff);
  }
#endif

  (void) dummy;
  return;
}


/*
 * Store the artificial test image defined in T.82, clause 7.2.1 at
 * pic. The image requires 477995 bytes of memory, is 1960 x 1951 pixels
 * large and has one plane.
 */ 
static void testimage(unsigned char *pic)
{
  unsigned long i, j, sum;
  unsigned int prsg, repeat[8];
  unsigned char *p;
  
  memset(pic, 0, TESTPIC_SIZE);
  p = pic;
  prsg = 1;
  for (j = 0; j < 1951; j++)
    for (i = 0; i < 1960; i++) {
      if (j >= 192) {
	if (j < 1023 || ((i >> 3) & 3) == 0) {
	  sum = (prsg & 1) + ((prsg >> 2) & 1) + ((prsg >> 11) & 1) +
	    ((prsg >> 15) & 1);
	  prsg = (prsg << 1) + (sum & 1);
	  if ((prsg & 3) == 0) {
	    *p |= 1 << (7 - (i & 7));
	    repeat[i & 7] = 1;
	  } else {
	    repeat[i & 7] = 0;
	  }
	} else {
	  if (repeat[i & 7])
	    *p |= 1 << (7 - (i & 7));
	}
      }
      if ((i & 7) == 7) ++p;
    }

  /* verify test image */
  sum = 0;
  for (i = 0; i < TESTPIC_SIZE; i++)
    for (j = 0; j < 8; j++)
      sum += (pic[i] >> j) & 1;
  if (sum != 861965L)
    printf("WARNING: Artificial test image has %lu (not 861965) "
	   "foreground pixels!\n", sum);

  return;
}
  

/*
 * Perform a full test cycle with one set of parameters. Encode an image
 * and compare the length of the result with correct_length. Then decode
 * the image again both in one single chunk or byte by byte and compare
 * the results with the original input image.
 */
static int test_cycle(unsigned char **orig_image, int width, int height,
		      int options, int order, int layers, int planes,
		      unsigned long l0, int mx, long correct_length,
		      const char *test_id)
{
  struct jbg_enc_state sje;
  struct jbg_dec_state sjd;
  int trouble = 0;
  long l;
  size_t plane_size;
  int i, result;
  unsigned char **image;

  plane_size = ((width + 7) / 8) * height;
  image = (unsigned char **) checkedmalloc(planes * sizeof(unsigned char *));
  for (i = 0; i < planes; i++) {
    image[i] = (unsigned char *) checkedmalloc(plane_size);
    memcpy(image[i], orig_image[i], plane_size);
  }

  printf("\nTest %s.1: Encoding ...\n", test_id);
  testbuf_len = 0;
  jbg_enc_init(&sje, width, height, planes, image, testbuf_writel, NULL);
  jbg_enc_layers(&sje, layers);
  jbg_enc_options(&sje, order, options, l0, mx, 0);
  jbg_enc_out(&sje);
  jbg_enc_free(&sje);
  for (i = 0; i < planes; i++)
    free(image[i]);
  free(image);
  printf("Encoded BIE has %6ld bytes: ", testbuf_len);
  if (correct_length >= 0)
    if (testbuf_len == correct_length)
      puts(PASSED);
    else {
      trouble++;
      printf(FAILED ", correct would have been %ld\n", correct_length);
    }
  else
    puts("");
  
  printf("Test %s.2: Decoding whole chunk ...\n", test_id);
  jbg_dec_init(&sjd);
  result = jbg_dec_in(&sjd, testbuf, testbuf_len, NULL);
  if (result != JBG_EOK) {
    printf("Decoder complained with return value %d: " FAILED "\n"
	   "Cause: '%s'\n", result, jbg_strerror(result, JBG_EN));
    trouble++;
  } else {
    printf("Image comparison: ");
    result = 1;
    for (i = 0; i < planes; i++) {
      if (memcmp(orig_image[i], sjd.lhp[layers & 1][i],
		 ((width + 7) / 8) * height)) {
	result = 0;
	trouble++;
	printf(FAILED " for plane %d\n", i);
      }
    }
    if (result)
      puts(PASSED);
  }
  jbg_dec_free(&sjd);

  printf("Test %s.3: Decoding with single-byte feed ...\n", test_id);
  jbg_dec_init(&sjd);
  result = JBG_EAGAIN;
  for (l = 0; l < testbuf_len; l++) {
    result = jbg_dec_in(&sjd, testbuf + l, 1, NULL);
    if (l < testbuf_len - 1 && result != JBG_EAGAIN) {
      printf("Decoder complained with return value %d at byte %ld: " FAILED
	     "\nCause: '%s'\n", result, l, jbg_strerror(result, JBG_EN));
      trouble++;
      break;
    }
  }
  if (l == testbuf_len) {
    if (result != JBG_EOK) {
      printf("Decoder complained with return value %d at final byte: " FAILED
	     "\nCause: '%s'\n", result, jbg_strerror(result, JBG_EN));
      trouble++;
    } else {
      printf("Image comparison: ");
      result = 1;
      for (i = 0; i < planes; i++) {
	if (memcmp(orig_image[i], sjd.lhp[layers & 1][i],
		   ((width + 7) / 8) * height)) {
	  result = 0;
	  trouble++;
	  printf(FAILED " for plane %d\n", i);
	}
      }
      if (result)
	puts(PASSED);
    }
  }

  jbg_dec_free(&sjd);
  puts("");
  
  return trouble != 0;
}


int main(int argc, char **argv)
{
  int trouble, problems = 0;
  struct jbg_arenc_state *se;
  struct jbg_ardec_state *sd;
  long i;
  int pix, order, layers;
  char test[10];
  size_t st;
  unsigned char *pp;
  unsigned char *ppp[4];

  int t82pix[16] = {
    0x05e0, 0x0000, 0x8b00, 0x01c4, 0x1700, 0x0034, 0x7fff, 0x1a3f,
    0x951b, 0x05d8, 0x1d17, 0xe770, 0x0000, 0x0000, 0x0656, 0x0e6a
  };
  int t82cx[16] = {
    0x0fe0, 0x0000, 0x0f00, 0x00f0, 0xff00, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
  };
  unsigned char t82sde[32] = {
    0x69, 0x89, 0x99, 0x5c, 0x32, 0xea, 0xfa, 0xa0,
    0xd5, 0xff, 0x00, 0x52, 0x7f, 0xff, 0x00, 0xff,
    0x00, 0xff, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x3f,
    0xff, 0x00, 0x2d, 0x20, 0x82, 0x91, 0xff, 0x02
  };

  /* three 23x5 pixel test images with the letters JBIG */
  unsigned char jbig_normal[15*4] = {
    0x7c, 0xe2, 0x38, 0x04, 0x92, 0x40, 0x04, 0xe2, 0x5c, 0x44,
    0x92, 0x44, 0x38, 0xe2, 0x38,
    0x7c, 0xe2, 0x38, 0x04, 0x92, 0x40, 0x04, 0xe2, 0x5c, 0x44,
    0x92, 0x44, 0x38, 0xe2, 0x38,
    0x7c, 0xe2, 0x38, 0x04, 0x92, 0x40, 0x04, 0xe2, 0x5c, 0x44,
    0x92, 0x44, 0x38, 0xe2, 0x38,
    0x7c, 0xe2, 0x38, 0x04, 0x92, 0x40, 0x04, 0xe2, 0x5c, 0x44,
    0x92, 0x44, 0x38, 0xe2, 0x38
  };
  unsigned char jbig_upsidedown[15*4] = {
    0x38, 0xe2, 0x38, 0x44, 0x92, 0x44, 0x04, 0xe2, 0x5c, 0x04,
    0x92, 0x40, 0x7c, 0xe2, 0x38,
    0x38, 0xe2, 0x38, 0x44, 0x92, 0x44, 0x04, 0xe2, 0x5c, 0x04,
    0x92, 0x40, 0x7c, 0xe2, 0x38,
    0x38, 0xe2, 0x38, 0x44, 0x92, 0x44, 0x04, 0xe2, 0x5c, 0x04,
    0x92, 0x40, 0x7c, 0xe2, 0x38,
    0x38, 0xe2, 0x38, 0x44, 0x92, 0x44, 0x04, 0xe2, 0x5c, 0x04,
    0x92, 0x40, 0x7c, 0xe2, 0x38
  };
  unsigned char jbig_inverse[15*4] = {
    0xff^0x7c, 0xff^0xe2, 0xfe^0x38, 0xff^0x04, 0xff^0x92,
    0xfe^0x40, 0xff^0x04, 0xff^0xe2, 0xfe^0x5c, 0xff^0x44,
    0xff^0x92, 0xfe^0x44, 0xff^0x38, 0xff^0xe2, 0xfe^0x38,
    0xff^0x7c, 0xff^0xe2, 0xfe^0x38, 0xff^0x04, 0xff^0x92,
    0xfe^0x40, 0xff^0x04, 0xff^0xe2, 0xfe^0x5c, 0xff^0x44,
    0xff^0x92, 0xfe^0x44, 0xff^0x38, 0xff^0xe2, 0xfe^0x38,
    0xff^0x7c, 0xff^0xe2, 0xfe^0x38, 0xff^0x04, 0xff^0x92,
    0xfe^0x40, 0xff^0x04, 0xff^0xe2, 0xfe^0x5c, 0xff^0x44,
    0xff^0x92, 0xfe^0x44, 0xff^0x38, 0xff^0xe2, 0xfe^0x38,
    0xff^0x7c, 0xff^0xe2, 0xfe^0x38, 0xff^0x04, 0xff^0x92,
    0xfe^0x40, 0xff^0x04, 0xff^0xe2, 0xfe^0x5c, 0xff^0x44,
    0xff^0x92, 0xfe^0x44, 0xff^0x38, 0xff^0xe2, 0xfe^0x38
  };
  int orders[] = {
    0,
    JBG_ILEAVE,
    JBG_ILEAVE | JBG_SMID,
#if 0
    JBG_SEQ,
    JBG_SEQ | JBG_SMID,
    JBG_SEQ | JBG_ILEAVE,
    JBG_HITOLO,
    JBG_HITOLO | JBG_ILEAVE,
    JBG_HITOLO | JBG_ILEAVE | JBG_SMID,
    JBG_HITOLO | JBG_SEQ,
    JBG_HITOLO | JBG_SEQ | JBG_SMID,
    JBG_HITOLO | JBG_SEQ | JBG_ILEAVE
#endif
  };

  printf("\nAutomatic JBIG Compatibility Test Suite\n"
	 "---------------------------------------\n\n"
	 "JBIG-KIT Version " JBG_VERSION
	 " -- This test will take a few minutes.\n\n\n");

  /* allocate test buffer memory */
  testbuf = (unsigned char *) checkedmalloc(TESTBUF_SIZE);
  testpic = (unsigned char *) checkedmalloc(TESTPIC_SIZE);
  se = (struct jbg_arenc_state *) checkedmalloc(sizeof(struct jbg_arenc_state));
  sd = (struct jbg_ardec_state *) checkedmalloc(sizeof(struct jbg_ardec_state));

  /* test a few properties of the machine architecture */
  testbuf[0] = 42;
  testbuf[0x10000L] = 0x42;
  st = 1 << 16;
  testbuf[st]++;
  pp = testbuf + 0x4000;
  pp += 0x4000;
  pp += 0x4000;
  pp += 0x4000;
  if (testbuf[0] != 42 || *pp != 0x43) {
    printf("Porting error detected:\n\n"
	   "Pointer arithmetic with this compiler has not at least 32 bits!\n"
	   "Are you sure, you have not compiled this program on an 8-bit\n"
	   "or 16-bit architecture? This compiler mode can obviously not\n"
           "handle arrays with a size of more than 65536 bytes. With this\n"
           "memory model, JBIG-KIT can only handle very small images and\n"
           "not even this compatibility test suite will run. :-(\n\n");
    exit(1);
  }

  /* only supported command line option:
   * output file name for exporting test image */
  if (argc > 1) {
    FILE *f;

    puts("Generating test image ...");
    testimage(testpic);
    printf("Storing in '%s' ...\n", argv[1]);
    
    /* write out test image as PBM file */
    f = fopen(argv[1], "wb");
    if (!f) abort();
    fprintf(f, "P4\n");
#if 0
    fprintf(f, "# Test image as defined in ITU-T T.82, clause 7.2.1\n");
#endif
    fprintf(f, "1960 1951\n");
    fwrite(testpic, 1, TESTPIC_SIZE, f);
    fclose(f);
    exit(0);
  }

#if 1
  puts("1) Arithmetic encoder test sequence from ITU-T T.82, clause 7.1\n"
       "---------------------------------------------------------------\n");
  arith_encode_init(se, 0);
  testbuf_len = 0;
  se->byte_out = testbuf_write;
  for (i = 0; i < 16 * 16; i++)
    arith_encode(se, (t82cx[i >> 4] >> ((15 - i) & 15)) & 1,
		 (t82pix[i >> 4] >> ((15 - i) & 15)) & 1);
  arith_encode_flush(se);
  printf("result of encoder:\n  ");
  for (i = 0; i < testbuf_len && i < TESTBUF_SIZE; i++)
    printf("%02x", testbuf[i]);
  printf("\nexpected result:\n  ");
  for (i = 0; i < 30; i++)
    printf("%02x", t82sde[i]);
  printf("\n\nTest 1: ");
  if (testbuf_len != 30 || memcmp(testbuf, t82sde, 30)) {
    problems++;
    printf(FAILED);
  } else
    printf(PASSED);
  printf("\n\n");


  puts("2) Arithmetic decoder test sequence from ITU-T T.82, clause 7.1\n"
       "---------------------------------------------------------------\n");
  printf("Test 2.1: Decoding whole chunk ...\n");
  arith_decode_init(sd, 0);
  sd->pscd_ptr = t82sde;
  sd->pscd_end = t82sde + 32;
  trouble = 0;
  for (i = 0; i < 16 * 16 && !trouble; i++) {
    pix = arith_decode(sd, (t82cx[i >> 4] >> ((15 - i) & 15)) & 1);
    if (pix < 0) {
      printf("Problem at Pixel %ld, result code %d.\n\n", i+1, sd->result);
      trouble++;
      break;
    }
    if (pix != ((t82pix[i >> 4] >> ((15 - i) & 15)) & 1)) {
      printf("Wrong PIX answer at Pixel %ld.\n\n", i+1);
      trouble++;
      break;
    }
  }
  if (!trouble && sd->result != JBG_READY) {
    printf("Result is %d instead of JBG_READY.\n\n", sd->result);
    trouble++;
  }
  printf("Test result: ");
  if (trouble) {
    problems++;
    puts(FAILED);
  } else
    puts(PASSED);
  printf("\n");

  printf("Test 2.2: Decoding with single byte feed ...\n");
  arith_decode_init(sd, 0);
  pp = t82sde;
  sd->pscd_ptr = pp;
  sd->pscd_end = pp + 1;
  trouble = 0;
  for (i = 0; i < 16 * 16 && !trouble; i++) {
    pix = arith_decode(sd, (t82cx[i >> 4] >> ((15 - i) & 15)) & 1);
    while ((sd->result == JBG_MORE || sd->result == JBG_MARKER) &&
	   sd->pscd_end < t82sde + 32) {
      pp++;
      sd->pscd_end = pp + 1;
      if (sd->result == JBG_MARKER)
	sd->pscd_ptr = pp - 1;
      else
	sd->pscd_ptr = pp;
      pix = arith_decode(sd, (t82cx[i >> 4] >> ((15 - i) & 15)) & 1);
    }
    if (pix < 0) {
      printf("Problem at Pixel %ld, result code %d.\n\n", i+1, sd->result);
      trouble++;
      break;
    }
    if (pix != ((t82pix[i >> 4] >> ((15 - i) & 15)) & 1)) {
      printf("Wrong PIX answer at Pixel %ld.\n\n", i+1);
      trouble++;
      break;
    }
  }
  if (!trouble && sd->result != JBG_READY) {
    printf("Result is %d instead of JBG_READY.\n\n", sd->result);
    trouble++;
  }
  printf("Test result: ");
  if (trouble) {
    problems++;
    puts(FAILED);
  } else
    puts(PASSED);
  printf("\n");
  
  puts("3) Parametric algorithm test sequence from ITU-T T.82, clause 7.2\n"
       "-----------------------------------------------------------------\n");
  puts("Generating test image ...");
  testimage(testpic);
  putchar('\n');
  pp = testpic;

  puts("Test 3.1: TPBON=0, Mx=0, LRLTWO=0, L0=1951");
  problems += test_cycle(&pp, 1960, 1951, JBG_DELAY_AT,
			 0, 0, 1, 1951, 0, 317384L, "3.1");
  puts("Test 3.2: TPBON=0, Mx=0, LRLTWO=1, L0=1951");
  problems += test_cycle(&pp, 1960, 1951, JBG_DELAY_AT | JBG_LRLTWO,
			 0, 0, 1, 1951, 0, 317132L, "3.2");
  puts("Test 3.3: TPBON=1, DPON=1, TPDON=1, Mx=8, LRLTWO=0, L0=128");
  problems += test_cycle(&pp, 1960, 1951, JBG_DELAY_AT | JBG_TPBON,
			 0, 0, 1, 128, 8, 253653L, "3.3");
  puts("Test 3.4: TPBON=1, DPON=1, TPDON=1, Mx=8, LRLTWO=0, L0=2, 6 layers");
  problems += test_cycle(&pp, 1960, 1951,
			 JBG_DELAY_AT | JBG_TPBON | JBG_TPDON | JBG_DPON,
			 0, 6, 1, 2, 8, 279314L, "3.4");
#if 0
  puts("Test 3.5: as TEST 4 but with order bit SEQ set");
  problems += test_cycle(&pp, 1960, 1951,
			 JBG_DELAY_AT | JBG_TPBON | JBG_TPDON | JBG_DPON,
			 JBG_SEQ, 6, 1, 2, 8, 279314L, "3.5");
#endif
#endif

  puts("4) Additional regression tests\n"
       "------------------------------\n");
  ppp[0] = jbig_normal;
  ppp[1] = jbig_upsidedown;
  ppp[2] = jbig_inverse;
  ppp[3] = jbig_inverse;

  i = 0;
  for (layers = 0; layers <= 3; layers++)
    for (order = 0; order < (int) (sizeof(orders)/sizeof(int)); order++) {
      sprintf(test, "4.%ld", ++i);
      printf("Test %s: order=%d, %d layers, 4 planes", test, orders[order],
	     layers);
      problems += test_cycle(ppp, 23, 5*4, JBG_TPBON | JBG_TPDON | JBG_DPON,
			     orders[order], layers, 4, 2, 8, -1, test);
    }


  printf("\nTest result summary: the library has %s the test suite.\n\n",
	 problems ? FAILED : PASSED);
  if (problems)
    puts("This is bad. If you cannot identify the problem yourself, please "
	 "send\nthis output plus a detailed description of your "
	 "compile environment\n(OS, compiler, version, options, etc.) to "
	 "Markus Kuhn <http://www.cl.cam.ac.uk/~mgk25/>.");
  else
    puts("Congratulations, everything is fine.\n");

  return problems != 0;
}
