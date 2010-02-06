#!/bin/sh
N=0
R=""
MAX=50
while [[ $N -le $MAX && "x$R" == "x" ]]
do
  R=$(git log -1 --pretty=format:%b HEAD~$N | sed -e 's/.*@\([0-9]\+\) .*/\1/')
  N=$[$N+1]
done
if [[ "x$R" != "x" ]]; then
  if [[ $N > 1 ]]; then
    R=$R\+$((N-1))
  fi
  echo $R
fi
