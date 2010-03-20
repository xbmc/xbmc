
#ifndef _PARSER_AUX_H
#define _PARSER_AUX_H


/***************************************/
/*  Functions used by ASN.1 parser     */
/***************************************/
node_asn *MHD__asn1_add_node (unsigned int type);

node_asn *MHD__asn1_set_value (node_asn * node, const void *value,
                               unsigned int len);

node_asn *MHD__asn1_set_name (node_asn * node, const char *name);

node_asn *MHD__asn1_set_right (node_asn * node, node_asn * right);

node_asn *MHD__asn1_set_down (node_asn * node, node_asn * down);

void MHD__asn1_remove_node (node_asn * node);

void MHD__asn1_delete_list (void);

void MHD__asn1_delete_list_and_nodes (void);

char *MHD__asn1_ltostr (long v, char *str);

node_asn *MHD__asn1_find_up (node_asn * node);

MHD__asn1_retCode MHD__asn1_change_integer_value (ASN1_TYPE node);

MHD__asn1_retCode MHD__asn1_expand_object_id (ASN1_TYPE node);

MHD__asn1_retCode MHD__asn1_check_identifier (ASN1_TYPE node);


#endif
