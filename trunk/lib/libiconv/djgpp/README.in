This is a port of GNU Libiconv @VER@ to MSDOS/DJGPP.

1.:     DJGPP specific changes.
        =======================
        There are no DJGPP specific changes. This package should
        configure and compile out-of-the-box.
        Please read the documentation to become familiar with this
        product.


2.:     Installing the binary package.
        ==============================

2.1.:   Copy the binary distribution into the top DJGPP installation
        directory and unzip the binary distribution running *ONE* of
        the following commands:
          unzip32 licv@packageversion@b.zip      or
          djtarx licv@packageversion@b.zip       or
          pkunzip -d licv@packageversion@b.zip



3.:     Building the binaries from sources.
        ===================================

3.1.:   To build the binaries you will need the following binary packages:
          djdev203.zip (patchlevel 2),
          bshNNNb.zip, gccNNNb.zip, bnuNNNb.zip, makNNNb.zip, filNNNb.zip,
          shlNNNb.zip, txtNNNb.zip, txiNNNb.zip, grepNNNb.zip, sedNNNb.zip,
          and difNNN.zip

        NNN represents the latest version number of the binary packages. All
        this packages can be found in the current/v2gnu/ directory of any
        ftp.delorie.com mirror.

3.2.:   Create a temporary directory and copy the source package: licv@packageversion@s.zip
        into the temporary directory. If you download the source distribution
        from one of the DJGPP archives, just unzip it preserving the directory
        structure, runnig ONE of the following commands:
          unzip32 licv@packageversion@s.zip      or
          djtarx licv@packageversion@s.zip       or
          pkunzip -d licv@packageversion@s.zip

        Source distributions downloaded from one of the GNU FTP sites need
        some more work to unpack. First, you MUST use the `djtar' program
        to unzip the package. That's because some file names in the official
        distributions need to be changed to avoid problems on the various
        platforms supported by DJGPP. `djtar' can rename files on the fly
        given a file with name mappings. The distribution includes a file
        `djgpp/fnchange.lst' with the necessary mappings. So you need first
        to retrieve that file, and then invoke `djtar' to unpack the
        distribution. Here's how:

          djtar -x -p -o @V@/djgpp/fnchange.lst @V@.tar.gz > lst
          djtar -x -n lst @V@.tar.gz

        (The name of the distribution archive and the top-level directory will
        be different for versions other than @VER@.)

3.3.:   If you have downloaded the source package from one of the GNU FTP sites
        you will have to configure the package running the command:
          djgpp\config.bat

3.4.:   If you have downloaded the source package from one of the delorie FTP
        sites the package is already preconfigured for djdev203 or later. In
        any case, to build the products you must run the following command:
          make

        After the compilation has finished, you can check the products
        running the command:
          make check

        To install the products run the command:
          make install

        This will install the products (iconv.exe iconv.h localcharset.h libconv.a
        libcharset.a iconv.1 iconv.3 iconv_open.3 iconv_close.3) into your DJGPP
        installation tree. As usual, prefix is defined as "/dev/env/DJDIR".
        If you prefer to install into same other directory run the command:
          make install prefix=z:/some/other/dir

        Of course, you should replace "z:/some/other/dir" by an appropriate path
        that will meet your requeriments.

3.5.:   If for some reason you want to reconfigure the package cd into the top
        srcdir (libiconv.@treeversion@) and run the following commands:
          del djgpp\config.cache
          make distclean
          djgpp\config

        Please note that you *MUST* delete the config.cache file in the djgpp
        subdir or you will not really reconfigure the sources because the
        configuration informations will be read from the cache file instead
        of being newly computed.
        To build the programs in a directory other than where the sources are,
        you must add the parameter that specifies the source directory,
        e.g:
          x:\src\gnu\libiconv.@treeversion@\djgpp\config x:/src/gnu/libiconv.@treeversion@

        Lets assume you want to build the binaries in a directory placed on a 
        different drive (z:\build in this case) from where the sources are,
        then you will run the following commands:
          z:
          md \build
          cd \build
          x:\src\gnu\libiconv.@treeversion@\djgpp\config x:/src/gnu/libiconv.@treeversion@

        You *MUST* use forward slashes to specify the source directory.
        After having configured the package run the folowing commands to create
        the binaries and docs and install them:
          make
          make check
          make install

        Send suggestions and bug reports concerning the DJGPP port to
        comp.os.msdos.djgpp or djgpp@delorie.com. Libiconv specific bugs
        must be reported to <bug-gnu-libiconv@gnu.org>.


          Guerrero, Juan Manuel <juan.guerrero@gmx.de>
