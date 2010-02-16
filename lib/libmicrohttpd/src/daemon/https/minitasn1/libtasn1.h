/*
 *      Copyright (C) 2004, 2005, 2006 Free Software Foundation
 *      Copyright (C) 2002 Fabio Fiorina
 *
 * This file is part of LIBTASN1.
 *
 * LIBTASN1 is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * LIBTASN1 is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with LIBTASN1; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA
 *
 */

#ifndef LIBTASN1_H
# define LIBTASN1_H

#include <stdio.h>              /* for FILE* */

#ifdef __cplusplus
extern "C"
{
#endif

#define LIBTASN1_VERSION "1.2"

#include <sys/types.h>
#include <time.h>

#define MAX_NAME_SIZE 128       /* maximum number of characters of a name */
  /* inside a file with ASN1 definitons     */
#define MAX_ERROR_DESCRIPTION_SIZE 128  /* maximum number of characters */
  /* of a description message     */
  /* (null character included)    */


  typedef int MHD__asn1_retCode;        /* type returned by libtasn1 functions */

  /*****************************************/
  /*  Errors returned by libtasn1 functions */
  /*****************************************/
#define ASN1_SUCCESS               0
#define ASN1_FILE_NOT_FOUND        1
#define ASN1_ELEMENT_NOT_FOUND     2
#define ASN1_IDENTIFIER_NOT_FOUND  3
#define ASN1_DER_ERROR             4
#define ASN1_VALUE_NOT_FOUND       5
#define ASN1_GENERIC_ERROR         6
#define ASN1_VALUE_NOT_VALID       7
#define ASN1_TAG_ERROR             8
#define ASN1_TAG_IMPLICIT          9
#define ASN1_ERROR_TYPE_ANY        10
#define ASN1_SYNTAX_ERROR          11
#define ASN1_MEM_ERROR		   12
#define ASN1_MEM_ALLOC_ERROR	   13
#define ASN1_DER_OVERFLOW          14
#define ASN1_NAME_TOO_LONG         15
#define ASN1_ARRAY_ERROR           16
#define ASN1_ELEMENT_NOT_EMPTY     17

/*************************************/
/* Constants used in MHD__asn1_visit_tree */
/*************************************/
#define ASN1_PRINT_NAME             1
#define ASN1_PRINT_NAME_TYPE        2
#define ASN1_PRINT_NAME_TYPE_VALUE  3
#define ASN1_PRINT_ALL              4

/*****************************************/
/* Constants returned by MHD__asn1_read_tag   */
/*****************************************/
#define ASN1_CLASS_UNIVERSAL        0x00        /* old: 1 */
#define ASN1_CLASS_APPLICATION      0x40        /* old: 2 */
#define ASN1_CLASS_CONTEXT_SPECIFIC 0x80        /* old: 3 */
#define ASN1_CLASS_PRIVATE          0xC0        /* old: 4 */
#define ASN1_CLASS_STRUCTURED       0x20

/*****************************************/
/* Constants returned by MHD__asn1_read_tag   */
/*****************************************/
#define ASN1_TAG_BOOLEAN          0x01
#define ASN1_TAG_INTEGER          0x02
#define ASN1_TAG_SEQUENCE         0x10
#define ASN1_TAG_SET              0x11
#define ASN1_TAG_OCTET_STRING     0x04
#define ASN1_TAG_BIT_STRING       0x03
#define ASN1_TAG_UTCTime          0x17
#define ASN1_TAG_GENERALIZEDTime  0x18
#define ASN1_TAG_OBJECT_ID        0x06
#define ASN1_TAG_ENUMERATED       0x0A
#define ASN1_TAG_NULL             0x05
#define ASN1_TAG_GENERALSTRING    0x1B

/******************************************************/
/* Structure definition used for the node of the tree */
/* that represent an ASN.1 DEFINITION.                */
/******************************************************/

  struct node_asn_struct
  {
    char *name;                 /* Node name */
    unsigned int type;          /* Node type */
    unsigned char *value;       /* Node value */
    int value_len;
    struct node_asn_struct *down;       /* Pointer to the son node */
    struct node_asn_struct *right;      /* Pointer to the brother node */
    struct node_asn_struct *left;       /* Pointer to the next list element */
  };

  typedef struct node_asn_struct node_asn;

  typedef node_asn *ASN1_TYPE;

#define ASN1_TYPE_EMPTY  NULL

  struct static_struct_asn
  {
    const char *name;           /* Node name */
    unsigned int type;          /* Node type */
    const void *value;          /* Node value */
  };

  typedef struct static_struct_asn ASN1_ARRAY_TYPE;



  /***********************************/
  /*  Functions definitions          */
  /***********************************/

  MHD__asn1_retCode MHD__asn1_parser2tree (const char *file_name,
                                           ASN1_TYPE * definitions,
                                           char *errorDescription);

  MHD__asn1_retCode MHD__asn1_parser2array (const char *inputFileName,
                                            const char *outputFileName,
                                            const char *vectorName,
                                            char *errorDescription);

  MHD__asn1_retCode MHD__asn1_array2tree (const ASN1_ARRAY_TYPE * array,
                                          ASN1_TYPE * definitions,
                                          char *errorDescription);

  MHD__asn1_retCode MHD__asn1_create_element (ASN1_TYPE definitions,
                                              const char *source_name,
                                              ASN1_TYPE * element);

  MHD__asn1_retCode MHD__asn1_delete_structure (ASN1_TYPE * structure);

  MHD__asn1_retCode MHD__asn1_write_value (ASN1_TYPE node_root,
                                           const char *name,
                                           const void *ivalue, int len);

  MHD__asn1_retCode MHD__asn1_read_value (ASN1_TYPE root, const char *name,
                                          void *ivalue, int *len);

  MHD__asn1_retCode MHD__asn1_der_coding (ASN1_TYPE element, const char *name,
                                          void *ider, int *len,
                                          char *ErrorDescription);

  MHD__asn1_retCode MHD__asn1_der_decoding (ASN1_TYPE * element,
                                            const void *ider, int len,
                                            char *errorDescription);

  MHD__asn1_retCode MHD__asn1_der_decoding_startEnd (ASN1_TYPE element,
                                                     const void *ider,
                                                     int len,
                                                     const char *name_element,
                                                     int *start, int *end);

  /* DER utility functions. */

  int MHD__asn1_get_tag_der (const unsigned char *der, int der_len,
                             unsigned char *cls, int *len,
                             unsigned long *tag);

  void MHD__asn1_octet_der (const unsigned char *str, int str_len,
                            unsigned char *der, int *der_len);

  MHD__asn1_retCode MHD__asn1_get_octet_der (const unsigned char *der,
                                             int der_len, int *ret_len,
                                             unsigned char *str, int str_size,
                                             int *str_len);

  void MHD__asn1_bit_der (const unsigned char *str, int bit_len,
                          unsigned char *der, int *der_len);

  MHD__asn1_retCode MHD__asn1_get_bit_der (const unsigned char *der,
                                           int der_len, int *ret_len,
                                           unsigned char *str, int str_size,
                                           int *bit_len);

  signed long MHD__asn1_get_length_der (const unsigned char *der, int der_len,
                                        int *len);

  void MHD__asn1_length_der (unsigned long int len, unsigned char *ans,
                             int *ans_len);

  /* Other utility functions. */

  ASN1_TYPE MHD__asn1_find_node (ASN1_TYPE pointer, const char *name);

#ifdef __cplusplus
}
#endif

#endif                          /* LIBTASN1_H */
