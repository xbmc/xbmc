#!/usr/bin/python
#############################################################
# tdbutil
# 
# Purpose:
#   Contains functions that are used to pack and unpack data
# from Samba's tdb databases.  Samba sometimes represents complex
# data structures as a single value in a database.  These functions
# allow other python scripts to package data types into a single python
# string and unpackage them.
#
#
# XXXXX: This code is no longer used; it's just here for testing
# compatibility with the new (much faster) C implementation.
#
############################################################## 
import string

def pack(format,list):
   retstring = ''
   listind = 0
   
   # Cycle through format entries
   for type in format:
      # Null Terminated String
      if (type == 'f' or type == 'P'):
         retstring = retstring + list[listind] + "\000"
      # 4 Byte Number
      if (type == 'd'):
         retstring = retstring + PackNum(list[listind],4)
      # 2 Byte Number
      if (type == 'w'):
         retstring = retstring + PackNum(list[listind],2)
      # Pointer Value
      if (type == 'p'):
         if (list[listind]):
            retstring = retstring + PackNum(1,4)
         else:
            retstring = retstring + PackNum(0,4)
      # Buffer and Length
      if (type == 'B'):
         # length
         length = list[listind]
         retstring = retstring + PackNum(length,4)
         length = int(length)
         listind = listind + 1
         # buffer
         retstring = retstring + list[listind][:length]
         
      listind = listind + 1
      
   return retstring

def unpack(format,buffer):
   retlist = []
   bufind = 0
   
   lasttype = ""
   for type in format:
      # Pointer Value
      if (type == 'p'):
         newvalue = UnpackNum(buffer[bufind:bufind+4])
         bufind = bufind + 4
         if (newvalue):
            newvalue = 1L
         else:
            newvalue = 0L
         retlist.append(newvalue)
      # Previous character till end of data
      elif (type == '$'):
         if (lasttype == 'f'):
            while (bufind < len(buffer)):
               newstring = ''
               while (buffer[bufind] != '\000'):
                  newstring = newstring + buffer[bufind]
                  bufind = bufind + 1
               bufind = bufind + 1
               retlist.append(newstring)
      # Null Terminated String
      elif (type == 'f' or type == 'P'):
         newstring = ''
         while (buffer[bufind] != '\000'):
            newstring = newstring + buffer[bufind]
            bufind = bufind + 1
         bufind = bufind + 1
         retlist.append(newstring)
      # 4 Byte Number
      elif (type == 'd'):
         newvalue = UnpackNum(buffer[bufind:bufind+4])
         bufind = bufind + 4
         retlist.append(newvalue)
      # 2 Byte Number
      elif (type == 'w'):
         newvalue = UnpackNum(buffer[bufind:bufind+2])
         bufind = bufind + 2
         retlist.append(newvalue)
      # Length and Buffer
      elif (type == 'B'):
         # Length
         length = UnpackNum(buffer[bufind:bufind+4])
         bufind = bufind + 4 
         retlist.append(length)
         length = int(length)
         # Buffer
         retlist.append(buffer[bufind:bufind+length])
         bufind = bufind + length
         
      lasttype = type

   return ((retlist,buffer[bufind:]))

def PackNum(myint,size):
    retstring = ''
    size = size * 2
    hint = hex(myint)[2:]

    # Check for long notation
    if (hint[-1:] == 'L'):
       hint = hint[:-1]
    
    addon = size - len(hint)
    for i in range(0,addon):
       hint = '0' + hint
    
    while (size > 0):
       val = string.atoi(hint[size-2:size],16)
       retstring = retstring + chr(val)
       size = size - 2
    
    return retstring
   
def UnpackNum(buffer):
   size = len(buffer)
   mystring = ''

   for i in range(size-1,-1,-1):
      val = hex(ord(buffer[i]))[2:]
      if (len(val) == 1):
         val = '0' + val
      mystring = mystring + val
   if (len(mystring) > 4):
      return string.atol(mystring,16)
   else:
      return string.atoi(mystring,16)
