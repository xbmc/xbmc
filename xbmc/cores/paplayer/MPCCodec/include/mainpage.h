/**
   \mainpage libmpcdec documentation

   \section whats what is libmpcdec
   libmpcdec is a library that decodes musepack compressed audio data.  Musepack
   is a free, high performance, high quality lossy audio compression codec.  For
   more information on musepack visit http://www.musepack.net.

   \section using using libmpcdec

   Using libmpcdec is very straightforward.  There are typically four things you must
   do to use libmpcdec in your application.

   \subsection step1 step 1: implement an mpc_reader to provide raw data to the decoder library
   The role of the mpc_reader is to provide raw mpc stream data to the mpc decoding library.
   This data can come from a file, a network socket, or any other source you wish.

   See the documentation of 
   \link mpc_reader_t mpc_reader \endlink 
   for more information.

   \subsection step2 step2: read the streaminfo properties structure from the stream
   This is a simple matter of calling the streaminfo_init() and streaminfo_read() functions, 
   supplying your mpc_reader as a source of raw data.  This reads the stream properties header from the 
   mpc stream.  This information will be used to prime the decoder for decoding in
   the next step.

   \subsection step3 step 3: initialize an mpc_decoder with your mpc_reader source
   This is just a matter of calling the mpc_decoder_setup() and mpc_decoder_initialize()
   functions with your mpc_decoder, mpc_reader data source and streaminfo information.

   \subsection step4 step 4: iteratively read raw sample data from the mpc decoder
   Once you've initialized the decoding library you just iteratively call the 
   mpc_decoder_decode routine until it indicates that the entire stream has been read.

   For a simple example of all of these steps see the sample application distributed with
   libmpcdec in src/sample.cpp.
*/
