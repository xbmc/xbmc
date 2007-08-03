/*
 * Copyright (c) 1999-2000 Image Power, Inc. and the University of
 *   British Columbia.
 * Copyright (c) 2001-2003 Michael David Adams.
 * All rights reserved.
 */

/* __START_OF_JASPER_LICENSE__
 * 
 * JasPer Software License
 * 
 * IMAGE POWER JPEG-2000 PUBLIC LICENSE
 * ************************************
 * 
 * GRANT:
 * 
 * Permission is hereby granted, free of charge, to any person (the "User")
 * obtaining a copy of this software and associated documentation, to deal
 * in the JasPer Software without restriction, including without limitation
 * the right to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the JasPer Software (in source and binary forms),
 * and to permit persons to whom the JasPer Software is furnished to do so,
 * provided further that the License Conditions below are met.
 * 
 * License Conditions
 * ******************
 * 
 * A.  Redistributions of source code must retain the above copyright notice,
 * and this list of conditions, and the following disclaimer.
 * 
 * B.  Redistributions in binary form must reproduce the above copyright
 * notice, and this list of conditions, and the following disclaimer in
 * the documentation and/or other materials provided with the distribution.
 * 
 * C.  Neither the name of Image Power, Inc. nor any other contributor
 * (including, but not limited to, the University of British Columbia and
 * Michael David Adams) may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * 
 * D.  User agrees that it shall not commence any action against Image Power,
 * Inc., the University of British Columbia, Michael David Adams, or any
 * other contributors (collectively "Licensors") for infringement of any
 * intellectual property rights ("IPR") held by the User in respect of any
 * technology that User owns or has a right to license or sublicense and
 * which is an element required in order to claim compliance with ISO/IEC
 * 15444-1 (i.e., JPEG-2000 Part 1).  "IPR" means all intellectual property
 * rights worldwide arising under statutory or common law, and whether
 * or not perfected, including, without limitation, all (i) patents and
 * patent applications owned or licensable by User; (ii) rights associated
 * with works of authorship including copyrights, copyright applications,
 * copyright registrations, mask work rights, mask work applications,
 * mask work registrations; (iii) rights relating to the protection of
 * trade secrets and confidential information; (iv) any right analogous
 * to those set forth in subsections (i), (ii), or (iii) and any other
 * proprietary rights relating to intangible property (other than trademark,
 * trade dress, or service mark rights); and (v) divisions, continuations,
 * renewals, reissues and extensions of the foregoing (as and to the extent
 * applicable) now existing, hereafter filed, issued or acquired.
 * 
 * E.  If User commences an infringement action against any Licensor(s) then
 * such Licensor(s) shall have the right to terminate User's license and
 * all sublicenses that have been granted hereunder by User to other parties.
 * 
 * F.  This software is for use only in hardware or software products that
 * are compliant with ISO/IEC 15444-1 (i.e., JPEG-2000 Part 1).  No license
 * or right to this Software is granted for products that do not comply
 * with ISO/IEC 15444-1.  The JPEG-2000 Part 1 standard can be purchased
 * from the ISO.
 * 
 * THIS DISCLAIMER OF WARRANTY CONSTITUTES AN ESSENTIAL PART OF THIS LICENSE.
 * NO USE OF THE JASPER SOFTWARE IS AUTHORIZED HEREUNDER EXCEPT UNDER
 * THIS DISCLAIMER.  THE JASPER SOFTWARE IS PROVIDED BY THE LICENSORS AND
 * CONTRIBUTORS UNDER THIS LICENSE ON AN ``AS-IS'' BASIS, WITHOUT WARRANTY
 * OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, WITHOUT LIMITATION,
 * WARRANTIES THAT THE JASPER SOFTWARE IS FREE OF DEFECTS, IS MERCHANTABLE,
 * IS FIT FOR A PARTICULAR PURPOSE OR IS NON-INFRINGING.  THOSE INTENDING
 * TO USE THE JASPER SOFTWARE OR MODIFICATIONS THEREOF FOR USE IN HARDWARE
 * OR SOFTWARE PRODUCTS ARE ADVISED THAT THEIR USE MAY INFRINGE EXISTING
 * PATENTS, COPYRIGHTS, TRADEMARKS, OR OTHER INTELLECTUAL PROPERTY RIGHTS.
 * THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE JASPER SOFTWARE
 * IS WITH THE USER.  SHOULD ANY PART OF THE JASPER SOFTWARE PROVE DEFECTIVE
 * IN ANY RESPECT, THE USER (AND NOT THE INITIAL DEVELOPERS, THE UNIVERSITY
 * OF BRITISH COLUMBIA, IMAGE POWER, INC., MICHAEL DAVID ADAMS, OR ANY
 * OTHER CONTRIBUTOR) SHALL ASSUME THE COST OF ANY NECESSARY SERVICING,
 * REPAIR OR CORRECTION.  UNDER NO CIRCUMSTANCES AND UNDER NO LEGAL THEORY,
 * WHETHER TORT (INCLUDING NEGLIGENCE), CONTRACT, OR OTHERWISE, SHALL THE
 * INITIAL DEVELOPER, THE UNIVERSITY OF BRITISH COLUMBIA, IMAGE POWER, INC.,
 * MICHAEL DAVID ADAMS, ANY OTHER CONTRIBUTOR, OR ANY DISTRIBUTOR OF THE
 * JASPER SOFTWARE, OR ANY SUPPLIER OF ANY OF SUCH PARTIES, BE LIABLE TO
 * THE USER OR ANY OTHER PERSON FOR ANY INDIRECT, SPECIAL, INCIDENTAL, OR
 * CONSEQUENTIAL DAMAGES OF ANY CHARACTER INCLUDING, WITHOUT LIMITATION,
 * DAMAGES FOR LOSS OF GOODWILL, WORK STOPPAGE, COMPUTER FAILURE OR
 * MALFUNCTION, OR ANY AND ALL OTHER COMMERCIAL DAMAGES OR LOSSES, EVEN IF
 * SUCH PARTY HAD BEEN INFORMED, OR OUGHT TO HAVE KNOWN, OF THE POSSIBILITY
 * OF SUCH DAMAGES.  THE JASPER SOFTWARE AND UNDERLYING TECHNOLOGY ARE NOT
 * FAULT-TOLERANT AND ARE NOT DESIGNED, MANUFACTURED OR INTENDED FOR USE OR
 * RESALE AS ON-LINE CONTROL EQUIPMENT IN HAZARDOUS ENVIRONMENTS REQUIRING
 * FAIL-SAFE PERFORMANCE, SUCH AS IN THE OPERATION OF NUCLEAR FACILITIES,
 * AIRCRAFT NAVIGATION OR COMMUNICATION SYSTEMS, AIR TRAFFIC CONTROL, DIRECT
 * LIFE SUPPORT MACHINES, OR WEAPONS SYSTEMS, IN WHICH THE FAILURE OF THE
 * JASPER SOFTWARE OR UNDERLYING TECHNOLOGY OR PRODUCT COULD LEAD DIRECTLY
 * TO DEATH, PERSONAL INJURY, OR SEVERE PHYSICAL OR ENVIRONMENTAL DAMAGE
 * ("HIGH RISK ACTIVITIES").  LICENSOR SPECIFICALLY DISCLAIMS ANY EXPRESS
 * OR IMPLIED WARRANTY OF FITNESS FOR HIGH RISK ACTIVITIES.  USER WILL NOT
 * KNOWINGLY USE, DISTRIBUTE OR RESELL THE JASPER SOFTWARE OR UNDERLYING
 * TECHNOLOGY OR PRODUCTS FOR HIGH RISK ACTIVITIES AND WILL ENSURE THAT ITS
 * CUSTOMERS AND END-USERS OF ITS PRODUCTS ARE PROVIDED WITH A COPY OF THE
 * NOTICE SPECIFIED IN THIS SECTION.
 * 
 * __END_OF_JASPER_LICENSE__
 */

/*
 * Tier 2 Decoder
 *
 * $Id$
 */

/******************************************************************************\
* Includes.
\******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "jasper/jas_types.h"
#include "jasper/jas_fix.h"
#include "jasper/jas_malloc.h"
#include "jasper/jas_math.h"
#include "jasper/jas_stream.h"
#include "jasper/jas_debug.h"

#include "jpc_bs.h"
#include "jpc_dec.h"
#include "jpc_cs.h"
#include "jpc_mqdec.h"
#include "jpc_t2dec.h"
#include "jpc_t1cod.h"
#include "jpc_math.h"

/******************************************************************************\
*
\******************************************************************************/

long jpc_dec_lookahead(jas_stream_t *in);
static int jpc_getcommacode(jpc_bitstream_t *in);
static int jpc_getnumnewpasses(jpc_bitstream_t *in);
static int jpc_dec_decodepkt(jpc_dec_t *dec, jas_stream_t *pkthdrstream, jas_stream_t *in, int compno, int lvlno,
  int prcno, int lyrno);

/******************************************************************************\
* Code.
\******************************************************************************/

static int jpc_getcommacode(jpc_bitstream_t *in)
{
	int n;
	int v;

	n = 0;
	for (;;) {
		if ((v = jpc_bitstream_getbit(in)) < 0) {
			return -1;
		}
		if (jpc_bitstream_eof(in)) {
			return -1;
		}
		if (!v) {
			break;
		}
		++n;
	}

	return n;
}

static int jpc_getnumnewpasses(jpc_bitstream_t *in)
{
	int n;

	if ((n = jpc_bitstream_getbit(in)) > 0) {
		if ((n = jpc_bitstream_getbit(in)) > 0) {
			if ((n = jpc_bitstream_getbits(in, 2)) == 3) {
				if ((n = jpc_bitstream_getbits(in, 5)) == 31) {
					if ((n = jpc_bitstream_getbits(in, 7)) >= 0) {
						n += 36 + 1;
					}
				} else if (n >= 0) {
					n += 5 + 1;
				}
			} else if (n >= 0) {
				n += 2 + 1;
			}
		} else if (!n) {
			n += 2;
		}
	} else if (!n) {
		++n;
	}

	return n;
}

static int jpc_dec_decodepkt(jpc_dec_t *dec, jas_stream_t *pkthdrstream, jas_stream_t *in, int compno, int rlvlno,
  int prcno, int lyrno)
{
	jpc_bitstream_t *inb;
	jpc_dec_tcomp_t *tcomp;
	jpc_dec_rlvl_t *rlvl;
	jpc_dec_band_t *band;
	jpc_dec_cblk_t *cblk;
	int n;
	int m;
	int i;
	jpc_tagtreenode_t *leaf;
	int included;
	int ret;
	int numnewpasses;
	jpc_dec_seg_t *seg;
	int len;
	int present;
	int savenumnewpasses;
	int mycounter;
	jpc_ms_t *ms;
	jpc_dec_tile_t *tile;
	jpc_dec_ccp_t *ccp;
	jpc_dec_cp_t *cp;
	int bandno;
	jpc_dec_prc_t *prc;
	int usedcblkcnt;
	int cblkno;
	uint_fast32_t bodylen;
	bool discard;
	int passno;
	int maxpasses;
	int hdrlen;
	int hdroffstart;
	int hdroffend;

	/* Avoid compiler warning about possible use of uninitialized
	  variable. */
	bodylen = 0;

	discard = (lyrno >= dec->maxlyrs);

	tile = dec->curtile;
	cp = tile->cp;
	ccp = &cp->ccps[compno];

	/*
	 * Decode the packet header.
	 */

	/* Decode the SOP marker segment if present. */
	if (cp->csty & JPC_COD_SOP) {
		if (jpc_dec_lookahead(in) == JPC_MS_SOP) {
			if (!(ms = jpc_getms(in, dec->cstate))) {
				return -1;
			}
			if (jpc_ms_gettype(ms) != JPC_MS_SOP) {
				jpc_ms_destroy(ms);
				fprintf(stderr, "missing SOP marker segment\n");
				return -1;
			}
			jpc_ms_destroy(ms);
		}
	}

hdroffstart = jas_stream_getrwcount(pkthdrstream);

	if (!(inb = jpc_bitstream_sopen(pkthdrstream, "r"))) {
		return -1;
	}

	if ((present = jpc_bitstream_getbit(inb)) < 0) {
		return 1;
	}
	JAS_DBGLOG(10, ("\n", present));
	JAS_DBGLOG(10, ("present=%d ", present));

	/* Is the packet non-empty? */
	if (present) {
		/* The packet is non-empty. */
		tcomp = &tile->tcomps[compno];
		rlvl = &tcomp->rlvls[rlvlno];
		bodylen = 0;
		for (bandno = 0, band = rlvl->bands; bandno < rlvl->numbands;
		  ++bandno, ++band) {
			if (!band->data) {
				continue;
			}
			prc = &band->prcs[prcno];
			if (!prc->cblks) {
				continue;
			}
			usedcblkcnt = 0;
			for (cblkno = 0, cblk = prc->cblks; cblkno < prc->numcblks;
			  ++cblkno, ++cblk) {
				++usedcblkcnt;
				if (!cblk->numpasses) {
					leaf = jpc_tagtree_getleaf(prc->incltagtree, usedcblkcnt - 1);
					if ((included = jpc_tagtree_decode(prc->incltagtree, leaf, lyrno + 1, inb)) < 0) {
						return -1;
					}
				} else {
					if ((included = jpc_bitstream_getbit(inb)) < 0) {
						return -1;
					}
				}
				JAS_DBGLOG(10, ("\n"));
				JAS_DBGLOG(10, ("included=%d ", included));
				if (!included) {
					continue;
				}
				if (!cblk->numpasses) {
					i = 1;
					leaf = jpc_tagtree_getleaf(prc->numimsbstagtree, usedcblkcnt - 1);
					for (;;) {
						if ((ret = jpc_tagtree_decode(prc->numimsbstagtree, leaf, i, inb)) < 0) {
							return -1;
						}
						if (ret) {
							break;
						}
						++i;
					}
					cblk->numimsbs = i - 1;
					cblk->firstpassno = cblk->numimsbs * 3;
				}
				if ((numnewpasses = jpc_getnumnewpasses(inb)) < 0) {
					return -1;
				}
				JAS_DBGLOG(10, ("numnewpasses=%d ", numnewpasses));
				seg = cblk->curseg;
				savenumnewpasses = numnewpasses;
				mycounter = 0;
				if (numnewpasses > 0) {
					if ((m = jpc_getcommacode(inb)) < 0) {
						return -1;
					}
					cblk->numlenbits += m;
					JAS_DBGLOG(10, ("increment=%d ", m));
					while (numnewpasses > 0) {
						passno = cblk->firstpassno + cblk->numpasses + mycounter;
	/* XXX - the maxpasses is not set precisely but this doesn't matter... */
						maxpasses = JPC_SEGPASSCNT(passno, cblk->firstpassno, 10000, (ccp->cblkctx & JPC_COX_LAZY) != 0, (ccp->cblkctx & JPC_COX_TERMALL) != 0);
						if (!discard && !seg) {
							if (!(seg = jpc_seg_alloc())) {
								return -1;
							}
							jpc_seglist_insert(&cblk->segs, cblk->segs.tail, seg);
							if (!cblk->curseg) {
								cblk->curseg = seg;
							}
							seg->passno = passno;
							seg->type = JPC_SEGTYPE(seg->passno, cblk->firstpassno, (ccp->cblkctx & JPC_COX_LAZY) != 0);
							seg->maxpasses = maxpasses;
						}
						n = JAS_MIN(numnewpasses, maxpasses);
						mycounter += n;
						numnewpasses -= n;
						if ((len = jpc_bitstream_getbits(inb, cblk->numlenbits + jpc_floorlog2(n))) < 0) {
							return -1;
						}
						JAS_DBGLOG(10, ("len=%d ", len));
						if (!discard) {
							seg->lyrno = lyrno;
							seg->numpasses += n;
							seg->cnt = len;
							seg = seg->next;
						}
						bodylen += len;
					}
				}
				cblk->numpasses += savenumnewpasses;
			}
		}

		jpc_bitstream_inalign(inb, 0, 0);

	} else {
		if (jpc_bitstream_inalign(inb, 0x7f, 0)) {
			fprintf(stderr, "alignment failed\n");
			return -1;
		}
	}
	jpc_bitstream_close(inb);

	hdroffend = jas_stream_getrwcount(pkthdrstream);
	hdrlen = hdroffend - hdroffstart;
	if (jas_getdbglevel() >= 5) {
		fprintf(stderr, "hdrlen=%lu bodylen=%lu \n", (unsigned long) hdrlen,
		  (unsigned long) bodylen);
	}

	if (cp->csty & JPC_COD_EPH) {
		if (jpc_dec_lookahead(pkthdrstream) == JPC_MS_EPH) {
			if (!(ms = jpc_getms(pkthdrstream, dec->cstate))) {
				fprintf(stderr, "cannot get (EPH) marker segment\n");
				return -1;
			}
			if (jpc_ms_gettype(ms) != JPC_MS_EPH) {
				jpc_ms_destroy(ms);
				fprintf(stderr, "missing EPH marker segment\n");
				return -1;
			}
			jpc_ms_destroy(ms);
		}
	}

	/* decode the packet body. */

	if (jas_getdbglevel() >= 1) {
		fprintf(stderr, "packet body offset=%06ld\n", (long) jas_stream_getrwcount(in));
	}

	if (!discard) {
		tcomp = &tile->tcomps[compno];
		rlvl = &tcomp->rlvls[rlvlno];
		for (bandno = 0, band = rlvl->bands; bandno < rlvl->numbands;
		  ++bandno, ++band) {
			if (!band->data) {
				continue;
			}
			prc = &band->prcs[prcno];
			if (!prc->cblks) {
				continue;
			}
			for (cblkno = 0, cblk = prc->cblks; cblkno < prc->numcblks;
			  ++cblkno, ++cblk) {
				seg = cblk->curseg;
				while (seg) {
					if (!seg->stream) {
						if (!(seg->stream = jas_stream_memopen(0, 0))) {
							return -1;
						}
					}
#if 0
fprintf(stderr, "lyrno=%02d, compno=%02d, lvlno=%02d, prcno=%02d, bandno=%02d, cblkno=%02d, passno=%02d numpasses=%02d cnt=%d numbps=%d, numimsbs=%d\n", lyrno, compno, rlvlno, prcno, band - rlvl->bands, cblk - prc->cblks, seg->passno, seg->numpasses, seg->cnt, band->numbps, cblk->numimsbs);
#endif
					if (seg->cnt > 0) {
						if (jpc_getdata(in, seg->stream, seg->cnt) < 0) {
							return -1;
						}
						seg->cnt = 0;
					}
					if (seg->numpasses >= seg->maxpasses) {
						cblk->curseg = seg->next;
					}
					seg = seg->next;
				}
			}
		}
	} else {
		if (jas_stream_gobble(in, bodylen) != JAS_CAST(int, bodylen)) {
			return -1;
		}
	}
	return 0;
}

/********************************************************************************************/
/********************************************************************************************/

int jpc_dec_decodepkts(jpc_dec_t *dec, jas_stream_t *pkthdrstream, jas_stream_t *in)
{
	jpc_dec_tile_t *tile;
	jpc_pi_t *pi;
	int ret;

	tile = dec->curtile;
	pi = tile->pi;
	for (;;) {
if (!tile->pkthdrstream || jas_stream_peekc(tile->pkthdrstream) == EOF) {
		switch (jpc_dec_lookahead(in)) {
		case JPC_MS_EOC:
		case JPC_MS_SOT:
			return 0;
			break;
		case JPC_MS_SOP:
		case JPC_MS_EPH:
		case 0:
			break;
		default:
			return -1;
			break;
		}
}
		if ((ret = jpc_pi_next(pi))) {
			return ret;
		}
if (dec->maxpkts >= 0 && dec->numpkts >= dec->maxpkts) {
	fprintf(stderr, "warning: stopping decode prematurely as requested\n");
	return 0;
}
		if (jas_getdbglevel() >= 1) {
			fprintf(stderr, "packet offset=%08ld prg=%d cmptno=%02d "
			  "rlvlno=%02d prcno=%03d lyrno=%02d\n", (long)
			  jas_stream_getrwcount(in), jpc_pi_prg(pi), jpc_pi_cmptno(pi),
			  jpc_pi_rlvlno(pi), jpc_pi_prcno(pi), jpc_pi_lyrno(pi));
		}
		if (jpc_dec_decodepkt(dec, pkthdrstream, in, jpc_pi_cmptno(pi), jpc_pi_rlvlno(pi),
		  jpc_pi_prcno(pi), jpc_pi_lyrno(pi))) {
			return -1;
		}
++dec->numpkts;
	}

	return 0;
}

jpc_pi_t *jpc_dec_pi_create(jpc_dec_t *dec, jpc_dec_tile_t *tile)
{
	jpc_pi_t *pi;
	int compno;
	jpc_picomp_t *picomp;
	jpc_pirlvl_t *pirlvl;
	jpc_dec_tcomp_t *tcomp;
	int rlvlno;
	jpc_dec_rlvl_t *rlvl;
	int prcno;
	int *prclyrno;
	jpc_dec_cmpt_t *cmpt;

	if (!(pi = jpc_pi_create0())) {
		return 0;
	}
	pi->numcomps = dec->numcomps;
	if (!(pi->picomps = jas_malloc(pi->numcomps * sizeof(jpc_picomp_t)))) {
		jpc_pi_destroy(pi);
		return 0;
	}
	for (compno = 0, picomp = pi->picomps; compno < pi->numcomps; ++compno,
	  ++picomp) {
		picomp->pirlvls = 0;
	}

	for (compno = 0, tcomp = tile->tcomps, picomp = pi->picomps;
	  compno < pi->numcomps; ++compno, ++tcomp, ++picomp) {
		picomp->numrlvls = tcomp->numrlvls;
		if (!(picomp->pirlvls = jas_malloc(picomp->numrlvls *
		  sizeof(jpc_pirlvl_t)))) {
			jpc_pi_destroy(pi);
			return 0;
		}
		for (rlvlno = 0, pirlvl = picomp->pirlvls; rlvlno <
		  picomp->numrlvls; ++rlvlno, ++pirlvl) {
			pirlvl->prclyrnos = 0;
		}
		for (rlvlno = 0, pirlvl = picomp->pirlvls, rlvl = tcomp->rlvls;
		  rlvlno < picomp->numrlvls; ++rlvlno, ++pirlvl, ++rlvl) {
/* XXX sizeof(long) should be sizeof different type */
			pirlvl->numprcs = rlvl->numprcs;
			if (!(pirlvl->prclyrnos = jas_malloc(pirlvl->numprcs *
			  sizeof(long)))) {
				jpc_pi_destroy(pi);
				return 0;
			}
		}
	}

	pi->maxrlvls = 0;
	for (compno = 0, tcomp = tile->tcomps, picomp = pi->picomps, cmpt =
	  dec->cmpts; compno < pi->numcomps; ++compno, ++tcomp, ++picomp,
	  ++cmpt) {
		picomp->hsamp = cmpt->hstep;
		picomp->vsamp = cmpt->vstep;
		for (rlvlno = 0, pirlvl = picomp->pirlvls, rlvl = tcomp->rlvls;
		  rlvlno < picomp->numrlvls; ++rlvlno, ++pirlvl, ++rlvl) {
			pirlvl->prcwidthexpn = rlvl->prcwidthexpn;
			pirlvl->prcheightexpn = rlvl->prcheightexpn;
			for (prcno = 0, prclyrno = pirlvl->prclyrnos;
			  prcno < pirlvl->numprcs; ++prcno, ++prclyrno) {
				*prclyrno = 0;
			}
			pirlvl->numhprcs = rlvl->numhprcs;
		}
		if (pi->maxrlvls < tcomp->numrlvls) {
			pi->maxrlvls = tcomp->numrlvls;
		}
	}

	pi->numlyrs = tile->cp->numlyrs;
	pi->xstart = tile->xstart;
	pi->ystart = tile->ystart;
	pi->xend = tile->xend;
	pi->yend = tile->yend;

	pi->picomp = 0;
	pi->pirlvl = 0;
	pi->x = 0;
	pi->y = 0;
	pi->compno = 0;
	pi->rlvlno = 0;
	pi->prcno = 0;
	pi->lyrno = 0;
	pi->xstep = 0;
	pi->ystep = 0;

	pi->pchgno = -1;

	pi->defaultpchg.prgord = tile->cp->prgord;
	pi->defaultpchg.compnostart = 0;
	pi->defaultpchg.compnoend = pi->numcomps;
	pi->defaultpchg.rlvlnostart = 0;
	pi->defaultpchg.rlvlnoend = pi->maxrlvls;
	pi->defaultpchg.lyrnoend = pi->numlyrs;
	pi->pchg = 0;

	pi->valid = 0;

	return pi;
}

long jpc_dec_lookahead(jas_stream_t *in)
{
	uint_fast16_t x;
	if (jpc_getuint16(in, &x)) {
		return -1;
	}
	if (jas_stream_ungetc(in, x & 0xff) == EOF ||
	  jas_stream_ungetc(in, x >> 8) == EOF) {
		return -1;
	}
	if (x >= JPC_MS_INMIN && x <= JPC_MS_INMAX) {
		return x;
	}
	return 0;
}
