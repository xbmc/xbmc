/*
 * Copyright (C) 2003, 2004, 2005, 2007 Free Software Foundation
 *
 * Author: Nikos Mavrogiannopoulos
 *
 * This file is part of GNUTLS.
 *
 * The GNUTLS library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA
 *
 */

#ifndef COMMON_H
# define COMMON_H

#include <gnutls.h>
#include <gnutls_algorithms.h>

#define MAX_STRING_LEN 512

#define GNUTLS_XML_SHOW_ALL 1

#define PEM_CRL "X509 CRL"
#define PEM_X509_CERT "X509 CERTIFICATE"
#define PEM_X509_CERT2 "CERTIFICATE"
#define PEM_PKCS7 "PKCS7"
#define PEM_PKCS12 "PKCS12"

/* public key algorithm's OIDs
 */
#define PK_PKIX1_RSA_OID "1.2.840.113549.1.1.1"
#define PK_DSA_OID "1.2.840.10040.4.1"
#define PK_GOST_R3410_94_OID "1.2.643.2.2.20"
#define PK_GOST_R3410_2001_OID "1.2.643.2.2.19"

/* signature OIDs
 */
#define SIG_DSA_SHA1_OID "1.2.840.10040.4.3"
#define SIG_RSA_MD5_OID "1.2.840.113549.1.1.4"
#define SIG_RSA_MD2_OID "1.2.840.113549.1.1.2"
#define SIG_RSA_SHA1_OID "1.2.840.113549.1.1.5"
#define SIG_RSA_SHA256_OID "1.2.840.113549.1.1.11"
#define SIG_RSA_SHA384_OID "1.2.840.113549.1.1.12"
#define SIG_RSA_SHA512_OID "1.2.840.113549.1.1.13"
#define SIG_RSA_RMD160_OID "1.3.36.3.3.1.2"
#define SIG_GOST_R3410_94_OID "1.2.643.2.2.4"
#define SIG_GOST_R3410_2001_OID "1.2.643.2.2.3"

int MHD__gnutls_x509_der_encode (ASN1_TYPE src, const char *src_name,
                                 MHD_gnutls_datum_t * res, int str);

int MHD__gnutls_x509_export_int (ASN1_TYPE MHD__asn1_data,
                                 MHD_gnutls_x509_crt_fmt_t format,
                                 char *pem_header, unsigned char *output_data,
                                 size_t * output_data_size);

int MHD__gnutls_x509_read_value (ASN1_TYPE c, const char *root,
                                 MHD_gnutls_datum_t * ret, int str);

int MHD__gnutls_x509_decode_and_read_attribute (ASN1_TYPE MHD__asn1_struct,
                                                const char *where, char *oid,
                                                int oid_size,
                                                MHD_gnutls_datum_t * value,
                                                int multi, int octet);

int MHD__gnutls_x509_get_pk_algorithm (ASN1_TYPE src, const char *src_name,
                                       unsigned int *bits);

int MHD__gnutls_asn1_copy_node (ASN1_TYPE * dst, const char *dst_name,
                                ASN1_TYPE src, const char *src_name);

#endif
