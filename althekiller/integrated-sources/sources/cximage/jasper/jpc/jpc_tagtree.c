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
 * Tag Tree Library
 *
 * $Id$
 */

/******************************************************************************\
* Includes.
\******************************************************************************/

#include <limits.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "jasper/jas_malloc.h"

#include "jpc_tagtree.h"

/******************************************************************************\
* Prototypes.
\******************************************************************************/

static jpc_tagtree_t *jpc_tagtree_alloc(void);

/******************************************************************************\
* Code for creating and destroying tag trees.
\******************************************************************************/

/* Create a tag tree. */

jpc_tagtree_t *jpc_tagtree_create(int numleafsh, int numleafsv)
{
	int nplh[JPC_TAGTREE_MAXDEPTH];
	int nplv[JPC_TAGTREE_MAXDEPTH];
	jpc_tagtreenode_t *node;
	jpc_tagtreenode_t *parentnode;
	jpc_tagtreenode_t *parentnode0;
	jpc_tagtree_t *tree;
	int i;
	int j;
	int k;
	int numlvls;
	int n;

	assert(numleafsh > 0 && numleafsv > 0);

	if (!(tree = jpc_tagtree_alloc())) {
		return 0;
	}
	tree->numleafsh_ = numleafsh;
	tree->numleafsv_ = numleafsv;

	numlvls = 0;
	nplh[0] = numleafsh;
	nplv[0] = numleafsv;
	do {
		n = nplh[numlvls] * nplv[numlvls];
		nplh[numlvls + 1] = (nplh[numlvls] + 1) / 2;
		nplv[numlvls + 1] = (nplv[numlvls] + 1) / 2;
		tree->numnodes_ += n;
		++numlvls;
	} while (n > 1);

	if (!(tree->nodes_ = jas_malloc(tree->numnodes_ * sizeof(jpc_tagtreenode_t)))) {
		return 0;
	}

	/* Initialize the parent links for all nodes in the tree. */

	node = tree->nodes_;
	parentnode = &tree->nodes_[tree->numleafsh_ * tree->numleafsv_];
	parentnode0 = parentnode;

	for (i = 0; i < numlvls - 1; ++i) {
		for (j = 0; j < nplv[i]; ++j) {
			k = nplh[i];
			while (--k >= 0) {
				node->parent_ = parentnode;
				++node;
				if (--k >= 0) {
					node->parent_ = parentnode;
					++node;
				}
				++parentnode;
			}
			if ((j & 1) || j == nplv[i] - 1) {
				parentnode0 = parentnode;
			} else {
				parentnode = parentnode0;
				parentnode0 += nplh[i];
			}
		}
	}
	node->parent_ = 0;

	/* Initialize the data values to something sane. */

	jpc_tagtree_reset(tree);

	return tree;
}

/* Destroy a tag tree. */

void jpc_tagtree_destroy(jpc_tagtree_t *tree)
{
	if (tree->nodes_) {
		jas_free(tree->nodes_);
	}
	jas_free(tree);
}

static jpc_tagtree_t *jpc_tagtree_alloc()
{
	jpc_tagtree_t *tree;

	if (!(tree = jas_malloc(sizeof(jpc_tagtree_t)))) {
		return 0;
	}
	tree->numleafsh_ = 0;
	tree->numleafsv_ = 0;
	tree->numnodes_ = 0;
	tree->nodes_ = 0;

	return tree;
}

/******************************************************************************\
* Code.
\******************************************************************************/

/* Copy state information from one tag tree to another. */

void jpc_tagtree_copy(jpc_tagtree_t *dsttree, jpc_tagtree_t *srctree)
{
	int n;
	jpc_tagtreenode_t *srcnode;
	jpc_tagtreenode_t *dstnode;

	/* The two tag trees must have similar sizes. */
	assert(srctree->numleafsh_ == dsttree->numleafsh_ &&
	  srctree->numleafsv_ == dsttree->numleafsv_);

	n = srctree->numnodes_;
	srcnode = srctree->nodes_;
	dstnode = dsttree->nodes_;
	while (--n >= 0) {
		dstnode->value_ = srcnode->value_;
		dstnode->low_ = srcnode->low_;
		dstnode->known_ = srcnode->known_;
		++dstnode;
		++srcnode;
	}
}

/* Reset all of the state information associated with a tag tree. */

void jpc_tagtree_reset(jpc_tagtree_t *tree)
{
	int n;
	jpc_tagtreenode_t *node;

	n = tree->numnodes_;
	node = tree->nodes_;

	while (--n >= 0) {
		node->value_ = INT_MAX;
		node->low_ = 0;
		node->known_ = 0;
		++node;
	}
}

/* Set the value associated with the specified leaf node, updating
the other nodes as necessary. */

void jpc_tagtree_setvalue(jpc_tagtree_t *tree, jpc_tagtreenode_t *leaf,
  int value)
{
	jpc_tagtreenode_t *node;

	/* Avoid compiler warnings about unused parameters. */
	tree = 0;

	assert(value >= 0);

	node = leaf;
	while (node && node->value_ > value) {
		node->value_ = value;
		node = node->parent_;
	}
}

/* Get a particular leaf node. */

jpc_tagtreenode_t *jpc_tagtree_getleaf(jpc_tagtree_t *tree, int n)
{
	return &tree->nodes_[n];
}

/* Invoke the tag tree encoding procedure. */

int jpc_tagtree_encode(jpc_tagtree_t *tree, jpc_tagtreenode_t *leaf,
  int threshold, jpc_bitstream_t *out)
{
	jpc_tagtreenode_t *stk[JPC_TAGTREE_MAXDEPTH - 1];
	jpc_tagtreenode_t **stkptr;
	jpc_tagtreenode_t *node;
	int low;

	/* Avoid compiler warnings about unused parameters. */
	tree = 0;

	assert(leaf);
	assert(threshold >= 0);

	/* Traverse to the root of the tree, recording the path taken. */
	stkptr = stk;
	node = leaf;
	while (node->parent_) {
		*stkptr++ = node;
		node = node->parent_;
	}

	low = 0;
	for (;;) {
		if (low > node->low_) {
			/* Deferred propagation of the lower bound downward in
			  the tree. */
			node->low_ = low;
		} else {
			low = node->low_;
		}

		while (low < threshold) {
			if (low >= node->value_) {
				if (!node->known_) {
					if (jpc_bitstream_putbit(out, 1) == EOF) {
						return -1;
					}
					node->known_ = 1;
				}
				break;
			}
			if (jpc_bitstream_putbit(out, 0) == EOF) {
				return -1;
			}
			++low;
		}
		node->low_ = low;
		if (stkptr == stk) {
			break;
		}
		node = *--stkptr;

	}
	return (leaf->low_ < threshold) ? 1 : 0;

}

/* Invoke the tag tree decoding procedure. */

int jpc_tagtree_decode(jpc_tagtree_t *tree, jpc_tagtreenode_t *leaf,
  int threshold, jpc_bitstream_t *in)
{
	jpc_tagtreenode_t *stk[JPC_TAGTREE_MAXDEPTH - 1];
	jpc_tagtreenode_t **stkptr;
	jpc_tagtreenode_t *node;
	int low;
	int ret;

	/* Avoid compiler warnings about unused parameters. */
	tree = 0;

	assert(threshold >= 0);

	/* Traverse to the root of the tree, recording the path taken. */
	stkptr = stk;
	node = leaf;
	while (node->parent_) {
		*stkptr++ = node;
		node = node->parent_;
	}

	low = 0;
	for (;;) {
		if (low > node->low_) {
			node->low_ = low;
		} else {
			low = node->low_;
		}
		while (low < threshold && low < node->value_) {
			if ((ret = jpc_bitstream_getbit(in)) < 0) {
				return -1;
			}
			if (ret) {
				node->value_ = low;
			} else {
				++low;
			}
		}
		node->low_ = low;
		if (stkptr == stk) {
			break;
		}
		node = *--stkptr;
	}

	return (node->value_ < threshold) ? 1 : 0;
}

/******************************************************************************\
* Code for debugging.
\******************************************************************************/

void jpc_tagtree_dump(jpc_tagtree_t *tree, FILE *out)
{
	jpc_tagtreenode_t *node;
	int n;

	node = tree->nodes_;
	n = tree->numnodes_;
	while (--n >= 0) {
		fprintf(out, "node %p, parent %p, value %d, lower %d, known %d\n",
		  (void *) node, (void *) node->parent_, node->value_, node->low_,
		  node->known_);
		++node;
	}
}
