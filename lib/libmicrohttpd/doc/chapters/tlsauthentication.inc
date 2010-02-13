We left the basic authentication chapter with the unsatisfactory conclusion that
any traffic, including the credentials, could be intercepted by anyone between
the browser client and the server. Protecting the data while it is sent over 
unsecured lines will be the goal of this chapter.

Since version 0.4, the @emph{MHD} library includes support for encrypting the
traffic by employing SSL/TSL. If @emph{GNU libmicrohttpd} has been configured to
support these, encryption and decryption can be applied transparently on the
data being sent, with only minimal changes to the actual source code of the example.


@heading Preparation

First, a private key for the server will be generated. With this key, the server
will later be able to authenticate itself to the client---preventing anyone else
from stealing the password by faking its identity. The @emph{OpenSSL} suite, which
is available on many operating systems, can generate such a key. For the scope of
this tutorial, we will be content with a 1024 bit key:
@verbatim
> openssl genrsa -out server.key 1024
@end verbatim
@noindent

In addition to the key, a certificate describing the server in human readable tokens
is also needed. This certificate will be attested with our aforementioned key. In this way,
we obtain a self-signed certificate, valid for one year. 

@verbatim
> openssl req -days 365 -out server.pem -new -x509 -key server.key
@end verbatim
@noindent

To avoid unnecessary error messages in the browser, the certificate needs to
have a name that matches the @emph{URI}, for example, "localhost" or the domain.
If you plan to have a publicly reachable server, you will need to ask a trusted third party,
called @emph{Certificate Authority}, or @emph{CA}, to attest the certificate for you. This way,
any visitor can make sure the server's identity is real.

Whether the server's certificate is signed by us or a third party, once it has been accepted
by the client, both sides will be communicating over encrypted channels. From this point on,
it is the client's turn to authenticate itself. But this has already been implemented in the basic 
authentication scheme.


@heading Changing the source code

We merely have to extend the server program so that it loads the two files into memory,

@verbatim
int
main ()
{
  struct MHD_Daemon *daemon;
  char *key_pem;
  char *cert_pem;

  key_pem = load_file (SERVERKEYFILE);
  cert_pem = load_file (SERVERCERTFILE);

  if ((key_pem == NULL) || (cert_pem == NULL))
  {
    printf ("The key/certificate files could not be read.\n");
    return 1;
  }
@end verbatim
@noindent

and then we point the @emph{MHD} daemon to it upon initalization. 
@verbatim

  daemon = MHD_start_daemon (MHD_USE_SELECT_INTERNALLY |  MHD_USE_SSL, PORT, NULL, NULL,
                             &answer_to_connection, NULL, 
                             MHD_OPTION_HTTPS_MEM_KEY, key_pem,
                             MHD_OPTION_HTTPS_MEM_CERT, cert_pem,
                             MHD_OPTION_END);

  if (NULL == daemon)
    {
      printf ("%s\n", cert_pem);
  
      free (key_pem);
      free (cert_pem);
  
      return 1;
    }
@end verbatim
@noindent


The rest consists of little new besides some additional memory cleanups.
@verbatim

  getchar ();

  MHD_stop_daemon (daemon);
  free (key_pem);
  free (cert_pem);
  
  return 0;
}
@end verbatim
@noindent


The rather unexciting file loader can be found in the complete example @code{tlsauthentication.c}.

@heading Remarks
@itemize @bullet
@item
While the standard @emph{HTTP} port is 80, it is 443 for @emph{HTTPS}. The common internet browsers assume
standard @emph{HTTP} if they are asked to access other ports than these. Therefore, you will have to type 
@code{https://localhost:8888} explicitly when you test the example, or the browser will not know how to
handle the answer properly.

@item
The remaining weak point is the question how the server will be trusted initially. Either a @emph{CA} signs the
certificate or the client obtains the key over secure means. Anyway, the clients have to be aware (or configured) 
that they should not accept certificates of unknown origin.

@item
The introduced method of certificates makes it mandatory to set an expiration date---making it less feasible to
hardcode certificates in embedded devices. 

@item
The cryptographic facilities consume memory space and computing time. For this reason, websites usually consists
both of uncritically @emph{HTTP} parts and secured @emph{HTTPS}.

@end itemize


