qstrip=$(strip $(subst ",,$(1)))#"))
-include $(BUILDROOT)/.config
-include $(BUILDROOT)/toolchain/gcc/Makefile.in
BASE_DIR=$(BUILDROOT)/output
TOOLCHAIN_EXTERNAL_DIR=$(BASE_DIR)/external-toolchain
ARCH=$(call qstrip,$(BR2_ARCH))
HOST_DIR=$(call qstrip,$(BR2_HOST_DIR))
HOST=$(call qstrip,$(BR2_TOOLCHAIN_EXTERNAL_PREFIX))
PKG_CONFIG_HOST_BINARY=$(HOST_DIR)/usr/bin/pkg-config

ifndef HOSTAR
HOSTAR:=ar
endif
ifndef HOSTAS
HOSTAS:=as
endif
ifndef HOSTCC
HOSTCC:=gcc
HOSTCC:=$(shell which $(HOSTCC) || type -p $(HOSTCC) || echo gcc)
HOSTCC_NOCCACHE:=$(HOSTCC)
endif
ifndef HOSTCXX
HOSTCXX:=g++
HOSTCXX:=$(shell which $(HOSTCXX) || type -p $(HOSTCXX) || echo g++)
HOSTCXX_NOCCACHE:=$(HOSTCXX)
endif
ifndef HOSTFC
HOSTFC:=gfortran
endif
ifndef HOSTCPP
HOSTCPP:=cpp
endif
ifndef HOSTLD
HOSTLD:=ld
endif
ifndef HOSTLN
HOSTLN:=ln
endif
ifndef HOSTNM
HOSTNM:=nm
endif
HOSTAR:=$(shell which $(HOSTAR) || type -p $(HOSTAR) || echo ar)
HOSTAS:=$(shell which $(HOSTAS) || type -p $(HOSTAS) || echo as)
HOSTFC:=$(shell which $(HOSTLD) || type -p $(HOSTLD) || echo || which g77 || type -p g77 || echo gfortran)
HOSTCPP:=$(shell which $(HOSTCPP) || type -p $(HOSTCPP) || echo cpp)
HOSTLD:=$(shell which $(HOSTLD) || type -p $(HOSTLD) || echo ld)
HOSTLN:=$(shell which $(HOSTLN) || type -p $(HOSTLN) || echo ln)
HOSTNM:=$(shell which $(HOSTNM) || type -p $(HOSTNM) || echo nm)

all:
-include $(BUILDROOT)/package/Makefile.in
-include $(BUILDROOT)/package/Makefile.autotools.in
-include $(BUILDROOT)/package/Makefile.cmake.in
-include $(BUILDROOT)/package/Makefile.package.in
