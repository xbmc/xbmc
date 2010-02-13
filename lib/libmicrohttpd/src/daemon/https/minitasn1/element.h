
#ifndef _ELEMENT_H
#define _ELEMENT_H


MHD__asn1_retCode MHD__asn1_append_sequence_set (node_asn * node);

MHD__asn1_retCode MHD__asn1_convert_integer (const char *value,
                                             unsigned char *value_out,
                                             int value_out_size, int *len);

void MHD__asn1_hierarchical_name (node_asn * node, char *name, int name_size);

#endif
