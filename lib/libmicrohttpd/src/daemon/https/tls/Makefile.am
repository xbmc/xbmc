AM_CPPFLAGS = \
-I$(top_srcdir)/src/include \
-I$(top_srcdir)/src/daemon/ \
-I$(top_srcdir)/src/daemon/https \
-I$(top_srcdir)/src/daemon/https/tls \
-I$(top_srcdir)/src/daemon/https/lgl \
-I$(top_srcdir)/src/daemon/https/minitasn1 \
-I$(top_srcdir)/src/daemon/https/x509 \
 @LIBGCRYPT_CFLAGS@ 

if USE_COVERAGE
  AM_CFLAGS = -fprofile-arcs -ftest-coverage
endif

noinst_LTLIBRARIES = libtls.la

libtls_la_SOURCES = \
auth_cert.c \
auth_dhe.c \
auth_dh_common.c \
auth_rsa.c \
auth_rsa_export.c \
debug.c \
defines.h \
ext_cert_type.c \
ext_max_record.c \
ext_server_name.c \
gnutls_alert.c \
gnutls_algorithms.c \
gnutls_asn1_tab.c \
gnutls_auth.c \
gnutls_buffers.c \
gnutls_cert.c \
gnutls_cipher.c \
gnutls_cipher_int.c \
gnutls_constate.c \
gnutls_datum.c \
gnutls_dh.c \
gnutls_dh_primes.c \
gnutls_errors.c \
gnutls_extensions.c \
gnutls_global.c \
gnutls_handshake.c \
gnutls_hash_int.c \
gnutls_kx.c \
gnutls_mem.c \
gnutls_mpi.c \
gnutls_num.c \
gnutls_pk.c \
gnutls_priority.c \
gnutls_record.c \
gnutls_rsa_export.c \
gnutls_sig.c \
gnutls_state.c \
gnutls_str.c \
gnutls_supplemental.c \
gnutls_ui.c \
gnutls_x509.c \
pkix_asn1_tab.c \
x509_b64.c \
auth_cert.h \
auth_dh_common.h \
debug.h \
defines.h \
ext_cert_type.h \
ext_max_record.h \
ext_server_name.h \
gnutls_algorithms.h \
gnutls_auth.h \
gnutls_auth_int.h \
gnutls_buffers.h \
gnutls_cert.h \
gnutls_cipher.h \
gnutls_cipher_int.h \
gnutls_constate.h \
gnutls_datum.h \
gnutls_dh.h \
gnutls_errors.h \
gnutls_extensions.h \
gnutls_global.h \
gnutls_handshake.h \
gnutls_hash_int.h \
gnutls_int.h \
gnutls_kx.h \
gnutls_mem.h \
gnutls_mpi.h \
gnutls_num.h \
gnutls_pk.h \
gnutls_record.h \
gnutls_rsa_export.h \
gnutls_sig.h \
gnutls_state.h \
gnutls_str.h \
gnutls_supplemental.h \
gnutls_x509.h \
x509_b64.h \
memmem.c \
str-two-way.h
libtls_la_LIBADD = @LIBGCRYPT_LIBS@

