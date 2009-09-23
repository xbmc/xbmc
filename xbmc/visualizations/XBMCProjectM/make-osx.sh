#!/bin/bash

# needs cmake installed from MacPorts

# this will have fail during link but that's ok
make -f Makefile.osx projectm

# now build ProjectM.vis from the object files
make -f Makefile.osx