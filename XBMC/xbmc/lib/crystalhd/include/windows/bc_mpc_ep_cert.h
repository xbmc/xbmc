/********************************************************************
 * Copyright(c) 2006 Broadcom Corporation, all rights reserved.
 * Contains proprietary and confidential information.
 *
 * This source file is the property of Broadcom Corporation, and
 * may not be copied or distributed in any isomorphic form without
 * the prior written consent of Broadcom Corporation.
 *
 *
 *  Name: bc_mpc_ep_cert.h
 *
 *  Description: Header file for eeprom contents for RSA and X509.
 *               This header file defines the common format for
 *               Gensig (both client and server), FW and Production
 *               data base. 
 *
 *               NOTE: Any changes to this format will impact,
 *                     gensig (both server and client), Firmware
 *                     and the production data base.
 *
 *  AU 
 *
 *
 *  HISTORY:
 *
 *******************************************************************/

#ifndef _BC_MPC_EPCERT_
#define _BC_MPC_EPCERT_

/* Note: _globals are adjusted to align the EPROM data to 3K size. 
 *       Adjust cert_buff size for any new fields to be added.. 
 */
enum _mpc_epcert_globals{
	BC_MAX_EP_GSIG_SIZE			= 0xC00,		/* Max Eprom size to be used */ 
	BC_EP_AES_BLOCK_LEN			= 16,			/* HW's AES block length */
	BC_MAX_RSA_KEY_SZ			= 128,			/* RSA key (1024 bits) */
	BC_MAX_RSA_EXP				= 4,			/* RSA exponent */
	BC_MAX_X509_SZ				= 2048+272,		/* 2K+272 for cert.  */
	BC_GSIG_SHA1_DIG_SZ			= 20,			/* SHA1 Digest size */
	BC_PKCS1_PAD_SIZE			= 128,			/* PKCS1 Padding Size */
	BC_MPC_SER_NUM_SZ			= 8,			/* Unique per board serial number */
};

typedef struct _bc_mpc_ep_priv_key_s{
	unsigned char	n[BC_MAX_RSA_KEY_SZ+4];
	unsigned char	e[BC_MAX_RSA_EXP];
	unsigned char	d[BC_MAX_RSA_KEY_SZ+4];
	unsigned char	p[BC_MAX_RSA_KEY_SZ/2 +4];
	unsigned char	q[BC_MAX_RSA_KEY_SZ/2 +4];
	unsigned char	dmp1[BC_MAX_RSA_KEY_SZ/2 +4];
	unsigned char	dmq1[BC_MAX_RSA_KEY_SZ/2 +4];
	unsigned char	iqmp[BC_MAX_RSA_KEY_SZ/2 +4];
}bc_mpc_ep_priv_key_t;

typedef struct _bc_mpc_ep_x509_cert_s{
	unsigned char	cert_buff[BC_MAX_X509_SZ];			/* x509 as a byte array */
	unsigned char	cert_sig[BC_MAX_RSA_KEY_SZ];		/* x509 signature. */
	unsigned char	serial_number[BC_MPC_SER_NUM_SZ];	/* Unique serial number per board. */
	unsigned int	cert_size;
	unsigned int	reserved;
}bc_mpc_ep_x509_cert_t;

/*
 * This is the structure gensig burn into eeprom. This structure 
 * is populated based on RSA private key and X509 cert read from 
 * PEM file format. 
 *
 * Currently Gensig-Client is converting PEM files into this format
 * for run time use. If need be, this functionality can be pushed
 * into Gensig-Server side.
 *
 */
typedef struct _bc_mpc_epcert_s{
	bc_mpc_ep_priv_key_t	fw_key;					/* Private Key to be used by Fw after decryption. */
	bc_mpc_ep_x509_cert_t	x509_cert;				/* Cert to be passed to host after decryption. */
}bc_mpc_epcert_t;

/*
 * This is the cipher data format structure after the 
 * hardware eprom copy engine moves from eeprom to
 * dram.
 */
typedef struct _bc_mpc_eprom_cipher_s{
	unsigned char	ep_rand_num[BC_EP_AES_BLOCK_LEN];
	unsigned char	ep_aes_key[BC_EP_AES_BLOCK_LEN];
	bc_mpc_epcert_t	epcert;
}bc_mpc_eprom_cipher_t;

#ifdef WIN32
/*
 * ====== Signing option Included ==================
 *
 * Library function to populate epcert from pem files.
 * sign_priv key will be used to sign the x509 data
 * and will append the signature to x509_cert.cert_sig
 * array.
 *
 * sign_pub is an optional parameter. If specified, 
 * signature will be verified for implicit sanity 
 * check.
 *
 */
extern int bc_gsig_get_epdata(
	char *sign_priv_key,	/* Private Key file to sign */
	char *sign_pub_key,		/* Public key to verify signature */
	char *cert,				/* Eprom X509 cert file name in PEM format */
	char *privkey,			/* Eprom Private key file name in PEM format */
	bc_mpc_epcert_t *epdata /* epdata to populate */
);

/*
 * ====== Signing option excluded ==================
 *
 * This method is to avoid passing the Signing Private
 * around. The actual signing of the eprom data can be
 * performed with OpenSSL command line option. After 
 * generating the signature through OpenSSL command line
 * Option, the signature file can be passed on to this 
 * function to prepare the eprom data as per the format.
 *
 *
 * Library function to populate epcert from pem files.
 * sig_file data will be loaded into epdata structure
 * without actually performing the signing. 
 *
 * sign_pub is an optional parameter. If specified, 
 * signature will be verified for implicit sanity 
 * check.
 *
 */
extern int bc_gsig_verify_populate_epdata(
	char *sig_file,			/* Signature File */
	char *sign_pub_key,		/* Public key to verify signature */
	char *cert,				/* Eprom X509 cert file name in PEM format */
	char *privkey,			/* Eprom Private key file name in PEM format */
	bc_mpc_epcert_t *epdata /* epdata to populate */
);

#endif


#endif
