TOC
1. Introduction
2. Building Kodi for the Raspberry Pi
3. Building Kodi using buildroot environment

-----------------------------------------------------------------------------
1. Introduction
-----------------------------------------------------------------------------

You can build Kodi for the Raspberry Pi in different ways. This document
shows two different methods. The first assumes that you want to run Kodi
on top of an image like Raspbian, the second shows how to create an entire
image which includes Linux.

-----------------------------------------------------------------------------
2. Building Kodi for the Raspberry Pi
-----------------------------------------------------------------------------

The following steps were tested with Ubuntu 16.04 x64. (Note that building on
a 32 bit machine requires slightly different setting).

The following commands build for newer Raspberry Pi 2 generation. In order to
build for the first Raspberry Pi, the commands have to be adapted to use
`--with-platform=raspberry-pi` instead of `--with-platform=raspberry-pi2`.

    $ sudo apt-get install git autoconf curl g++ zlib1g-dev libcurl4-openssl-dev gawk gperf libtool autopoint swig default-jre bison make

    $ RPI_DEV=$PWD
    $ git clone https://github.com/raspberrypi/tools
    $ git clone https://github.com/raspberrypi/firmware
    $ git clone https://github.com/xbmc/xbmc

    $ mkdir kodi-bcm
    $ cd xbmc/tools/depends
    $ ./bootstrap
    $ ./configure --host=arm-linux-gnueabihf \
       --prefix=$RPI_DEV/kodi-bcm \
       --with-toolchain=$RPI_DEV/tools/arm-bcm2708/arm-rpi-4.9.3-linux-gnueabihf \
       --with-firmware=$RPI_DEV/firmware \
       --with-platform=raspberry-pi2 \
       --disable-debug

    $ make
    $ cd ../..

    $ make -C tools/depends/target/cmakebuildsys
    $ cd build
    $ make
    $ make install

-----------------------------------------------------------------------------
3. Building Kodi using buildroot environment
-----------------------------------------------------------------------------

Installing and setting up the buildroot environment:

Create a top level directory where you checkout Kodi and buildroot.

For example :

    $ mkdir /opt/kodi-raspberrypi
    $ cd /opt/kodi-raspberrypi

Checkout kodi :

    $ git clone https://github.com/xbmc/xbmc.git kodi

Checkout buildroot :

    $ git clone https://github.com/huceke/buildroot-rbp.git

    $ cd /opt/kodi-raspberrypi/buildroot-rbp

Follow the instructions in README.rbp how to build the system and Kodi.

