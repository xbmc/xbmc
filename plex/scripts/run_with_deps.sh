#!/bin/sh

export PATH=$1:$PATH
shift
prg=$1
shift
$prg "$@"


