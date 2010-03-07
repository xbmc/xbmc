This tutorial is for developers who want to learn how they can add HTTP serving 
capabilities to their applications with the @emph{GNU libmicrohttpd} library,
abbreviated @emph{MHD}.  The reader will learn how to
implement basic HTTP functions from simple executable
sample programs that implement various features.

The text is supposed to be a supplement to the API reference manual of 
@emph{GNU libmicrohttpd} and for that reason does not explain many of the parameters.
Therefore, the reader should always consult the manual to find the exact meaning
of the functions used in the tutorial.  Furthermore, the reader is 
encouraged to study the relevant @emph{RFCs}, which document the HTTP standard.

@emph{GNU libmicrohttpd} is assumed to be already installed.  This tutorial
is written for version @value{VERSION}.  At the time being, 
this tutorial has only been tested on @emph{GNU/Linux} machines even though
efforts were made not to rely on anything that would prevent the samples from being
built on similar systems.
