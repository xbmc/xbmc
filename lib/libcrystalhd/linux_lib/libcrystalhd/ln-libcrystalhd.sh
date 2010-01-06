#!/bin/bash
#
# Author: Prasad Bolisetty
#
# Script to create symlink for crystalhd soname.
# 
# TBD:: Add install option. For now ==> ${PWD} 
# 

ln -sf libcrystalhd.so.1.0 libcrystalhd.so
ln -sf libcrystalhd.so.1.0 libcrystalhd.so.1

