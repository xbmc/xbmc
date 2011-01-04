#
#    "wcc386.exe" wrapper
#    Copyright (C) 2004 Keishi Suenaga <s_kesihi@mutt.freemail.ne.jp>
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License Version 2
#    along with this program; if not, write to the Free Software
#    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
#    wcc386_w.sh:
#       wcc386 wrapper script.
#       Make wcc386 to accespt GNU autotools like calls.
#

#!/bin/sh

foo0=`echo $@|perl -pe 's/($s)(-O.)($s)/$1 $3/'|perl -pe 's/($s)(-D)($S)/$1-d$3/g' -|perl -pe 's/($s)(-I)($S)/$1-i=$3/g' -|perl -pe 's/\\//\\\\/g' -`

###############################################################################
#                                                                             #
# compilelink()  parameters     foo0  the list of command line                #
#                               fname filename of exe file                    #
#                               compileonly  do not invoke wlink              #
#                                                                             #
###############################################################################

compilelink(){
     complist=" "
     clist=" "
     liblist=" "
     objlist=" "
     rmobjlist=" "
     for foo in $foo0 ;do
       case $foo in
       *.c)
         if test "x$fname" = "x " ;then
           fname=`echo $foo|perl -pe 's/(.*)\.c/$1/' -`
         fi
         clist="$clist $foo"
         ;;
       *.cpp)
         if test "x$fname" = "x " ;then
           fname=`echo $foo|perl -pe 's/(.*)\.cpp/$1/' -`
         fi
         clist="$clist $foo"
         ;;
       *.obj)
         objlist="$objlist file $foo"
         rmobjlist="$rmobjlist $foo"
         ;;
       *.lib)
         liblist="$liblist Library $foo"
         ;;
       -l*)
         echo "Ignoreing $foo"
         ;;
       *)
       complist="$complist $foo"
       esac
    done
    if test "x$clist" != "x "; then
      for foo in $clist ; do
        if ! wcc386 -zq $foo $complist; then exit -1; fi
        bar=`echo $foo|perl -pe 's/(.*)\.c.*/$1/' -`.obj
        objlist="$objlist file $bar"
        rmobjlist="$rmobjlist $bar"
      done
    fi
    if test "x$compileonly" != xyes; then
      if ! wlink  op q $objlist $liblist  Name "$fname".exe; then exit -1; fi
      rm $rmobjlist
    fi
}

case $foo0 in
"")
  wcc386
  ;;
*"-p "*|*" -p"*)
  if ! wcc386 -zq $foo0; then exit -1; fi
  ;;
*"-c "*|*" -c"*)
  foo=`echo $foo0|perl -pe 's/-c / /' -|perl -pe 's/ -c$/ /' -`  
  case $foo0 in
  *"-o "*)
    bar=`echo $foo|perl -pe 's/-o /-fo=/' -`
    if ! wcc386 -zq $bar; then exit -1; fi
    ;;
  *)
    foo0=$foo
    compileonly=yes
    compilelink
    ;;
  esac
  ;;
*)
  case $foo0 in
  *"-o "*)
    echo $foo0
     bar=" "
     found=" "
     for foo in $foo0 ;do
       case $foo in
       -o)
	     found=yes 
         ;;
       *)
         if test "x$found" = xyes; then
           fname=`echo $foo|perl -pe 's/(.*)\.exe/$1/' -`
	       found=no
	     else
	       bar="$bar $foo"
         fi
         ;;
       esac
    done
    foo0=$bar
    if test "x$fname" = x; then
      echo "wcc386_w Error"
      exit -1;
    fi
    compilelink
    ;;
  *)
     fname=" "
     objfname=" "
     for foo in $foo0 ;do
       case $foo in
       *.c)
         if test "x$fname" = "x " ;then
           fname=`echo $foo|perl -pe 's/(.*)\.c/$1/' -`
         fi
         ;;
       *.cpp)
         if test "x$fname" = "x " ;then
           fname=`echo $foo|perl -pe 's/(.*)\.cpp/$1/' -`
         fi
         ;;
       *.obj)
         if test "x$objfname" = "x " ;then
           objfname=`echo $foo|perl -pe 's/(.*)\.obj/$1/' -`
         fi
         ;;

       *)
       esac
    done
    if test "x$fname" = "x " && test "x$objfname" = "x "; then
      echo "wcc386_w Error"
      exit -1;
    fi
    if test "x$fname" = "x "; then
      fname=$objfname
    fi
    compilelink
    ;;
 esac
 ;;
esac

exit 0;
