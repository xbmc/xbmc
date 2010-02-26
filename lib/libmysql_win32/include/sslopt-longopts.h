/* Copyright (C) 2000 MySQL AB

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

#ifdef HAVE_OPENSSL

  {"ssl", OPT_SSL_SSL,
   "Enable SSL for connection (automatically enabled with other flags). Disable with --skip-ssl.",
 (uchar **) &opt_use_ssl, (uchar **) &opt_use_ssl, 0, GET_BOOL, NO_ARG, 0, 0, 0,
   0, 0, 0},
  {"ssl-ca", OPT_SSL_CA,
   "CA file in PEM format (check OpenSSL docs, implies --ssl).",
   (uchar **) &opt_ssl_ca, (uchar **) &opt_ssl_ca, 0, GET_STR, REQUIRED_ARG,
   0, 0, 0, 0, 0, 0},
  {"ssl-capath", OPT_SSL_CAPATH,
   "CA directory (check OpenSSL docs, implies --ssl).",
   (uchar **) &opt_ssl_capath, (uchar **) &opt_ssl_capath, 0, GET_STR, REQUIRED_ARG,
   0, 0, 0, 0, 0, 0},
  {"ssl-cert", OPT_SSL_CERT, "X509 cert in PEM format (implies --ssl).",
   (uchar **) &opt_ssl_cert, (uchar **) &opt_ssl_cert, 0, GET_STR, REQUIRED_ARG,
   0, 0, 0, 0, 0, 0},
  {"ssl-cipher", OPT_SSL_CIPHER, "SSL cipher to use (implies --ssl).",
   (uchar **) &opt_ssl_cipher, (uchar **) &opt_ssl_cipher, 0, GET_STR, REQUIRED_ARG,
   0, 0, 0, 0, 0, 0},
  {"ssl-key", OPT_SSL_KEY, "X509 key in PEM format (implies --ssl).",
   (uchar **) &opt_ssl_key, (uchar **) &opt_ssl_key, 0, GET_STR, REQUIRED_ARG,
   0, 0, 0, 0, 0, 0},
#ifdef MYSQL_CLIENT
  {"ssl-verify-server-cert", OPT_SSL_VERIFY_SERVER_CERT,
   "Verify server's \"Common Name\" in its cert against hostname used when connecting. This option is disabled by default.",
   (uchar **) &opt_ssl_verify_server_cert, (uchar **) &opt_ssl_verify_server_cert,
    0, GET_BOOL, NO_ARG, 0, 0, 0, 0, 0, 0},
#endif
#endif /* HAVE_OPENSSL */
