#!/bin/bash
# Arguments -v for valgrind

usage()
{
cat <<EOF
  usage $0 options

  OPTIONS:

  -h  usage
  -v  run the tests using valgrind
EOF
}

valgrind=0
while getopts "h:v" OPTION
do
  case $OPTION in
    h)
      usage
      exit 1
      ;;
    v)
      valgrind=1
      ;;
  esac
done

for i in `find . -name *_test`
do
  if [ $valgrind -eq 1 ]
  then
    echo valgrind $i
    valgrind $i
  else
    echo $i
    $i
  fi

  if [ $? != 0 ];
  then
    echo "$i failed!"
    exit 1
  fi
done
