
/*************************************************/
/* File: structure.h                             */
/* Description: list of exported object by       */
/*   "structure.c"                               */
/*************************************************/

#ifndef _STRUCTURE_H
#define _STRUCTURE_H

node_asn *MHD__asn1_copy_structure3 (node_asn * source_node);


node_asn *MHD__asn1_add_node_only (unsigned int type);

node_asn *MHD__asn1_find_left (node_asn * node);

#endif
