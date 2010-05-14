#!/bin/sh
N=0
R=""
MAX=50
while [ $N -le $MAX ] && [ "x$R" = "x" ]
do
  R=$(git log -1 --pretty=format:%b HEAD~$N | awk '$2 ~ /@([0-9]+)$/ {sub(".*@", "", $2); print $2}')
  N=$(($N+1))
done
if [ "x$R" != "x" ]; then
  if [ $N -gt 1 ]; then
    R=$R\+$((N-1))
  fi
  echo $R
fi
