#!/usr/bin/env ruby
#
#  Copyright (C) 2008 Elan Feingold (elan at bluemandrill dot com)
#
#  This Program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2, or (at your option)
#  any later version.
#
#  This Program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with GNU Make; see the file COPYING.  If not, write to
#  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
#  http://www.gnu.org/copyleft/gpl.html
#
#

require 'bit-struct'

# For example, "_calloc" => "_wrap_calloc"
prefix = '___wrap_'
mappings = {
#'calloc' => true,
'clearerr' => true,
'close' => true,
'fclose' => true,
'fdopen' => true,
'feof' => true,
'ferror' => true,
'fflush' => true,
'fgetc' => true,
'fgetpos' => true,
'fgets' => true,
'fileno' => true,
'flockfile' => true,
'fopen' => true,
'fopen64' => true,
'fprintf' => true,
'fputc' => true,
'fputs' => true,
'fread' => true,
#'free' => true,
'freopen' => true,
'fseek' => true,
'fsetpos' => true,
'fstatvfs64' => true,
'ftell' => true,
'ftrylockfile' => true,
'funlockfile' => true,
'fwrite' => true,
'getc_unlocked' => true,
'ioctl' => true,
'lseek' => true,
'lseek64' => true,
#'malloc' => true,
'open' => true,
'open64' => true,
'popen' => true,
'printf' => true,
'read' => true,
#'realloc' => true,
'rewind' => true,
'stat' => true,
'fstat' => true,
'ungetc' => true,
'vfprintf' => true,
'write' => true,
'putc' => '___wrap__IO_putc',
'getc' => '___wrap__IO_getc',
'getc_unlocked' => '___wrap__IO_getc_unlocked'
}

prefix_python = '___py_wrap_'
mappings_python = {
  'getcwd' => true, 'chdir' => true, 'access' => true, 'unlink' => true, 'chmod' => true, 
  'rmdir' => true, 'utime' => true, 'rename' => true, 'mkdir' => true,  
  'opendir' => true, 'dlopen' => true, 'dlclose' => true, 'dlsym' => true,
  'lstat' => true
}

LC_SYMTAB = 0x02
LC_SEGMENT = 0x01

class MachHeader < BitStruct
  hex_octets  :magic,       32, "Magic Number"
  unsigned    :cputype,     32, "CPU Type", :endian => :native
  unsigned    :cpusubtype,  32, "CPU Subtype", :endian => :native
  unsigned    :filetype,    32, "File Type", :endian => :native
  unsigned    :ncmds,       32, "Number of commands", :endian => :native
  unsigned    :sizeofcmds,  32, "Size of commands", :endian => :native
  unsigned    :flags,       32, "Flags", :endian => :native
  rest        :data,            "Data"
end

class LoadCommand < BitStruct
  unsigned    :cmd,         32, "Command", :endian => :native
  unsigned    :cmdsize,     32, "Command Size", :endian => :native
end

class SymtabCommand < BitStruct
  unsigned    :cmd,         32, "Command", :endian => :native
  unsigned    :cmdsize,     32, "Command Size", :endian => :native
  unsigned    :symoff,      32, "Symbol Offset", :endian => :native
  unsigned    :nsyms,       32, "Number of Symbols", :endian => :native
  unsigned    :stroff,      32, "String table offset", :endian => :native
  unsigned    :strsize,     32, "Size of string table", :endian => :native
end

class SegmentCommand < BitStruct
  unsigned    :cmd,         32, "Command", :endian => :native
  unsigned    :cmdsize,     32, "Command Size", :endian => :native
  char        :segname,    16*8, "Segment name", :endian => :native
  unsigned    :vmaddr,      32, "VM Adddress", :endian => :native
  unsigned    :vmsize,      32, "VM Size", :endian => :native
  unsigned    :fileoff,     32, "File Offset", :endian => :native
  unsigned    :filesize,    32, "File Size", :endian => :native
end

class SymtabEntry < BitStruct
  unsigned  :strtableoffset, 32, "String table offset", :endian => :native
  unsigned  :debuggingEntry,  3, "Debugging entry", :endian => :native
  unsigned  :privateExternal, 1, "Is Private Enternal", :endian => :native
  unsigned  :type,            3, "Type bits", :endian => :native
  unsigned  :external,        1, "External symbol", :endian => :native
  unsigned  :sectionNumber,   8, "Section number", :endian => :native
  unsigned  :description,    16, "Description", :endian => :native
  unsigned  :value,          32, "Value", :endian => :native
end

# Select which mapping to use.
if ARGV.size() > 1 and ARGV[1].index('libpython') == 0
  puts "Using Python mappings."
  mappings = mappings_python
  #prefix = prefix_python
end

data = open(ARGV[0]).read
puts "Input file was #{data.length} bytes long."

# Parse the header.
header = MachHeader.new(data)
sym_cmd = nil

# String table.
string_table = nil
string_table_offset = nil
string_table_map = {}
offset_map = {}

# Symbol table.
symbol_table = nil
symbol_table_offset = nil
symbols = []

# Link segment.
link_cmd = nil
link_cmd_offset = nil

# Walk through all the commands.
offset = data.size - header.data.size
header.ncmds.times do |i|
  load_cmd = LoadCommand.new(data[offset..-1])
  
  if load_cmd.cmd == LC_SEGMENT
    seg_cmd = SegmentCommand.new(data[offset..-1])
    if seg_cmd.segname.index('__LINKEDIT') == 0
      puts "Found LINKEDIT segment at offset #{offset}"
      link_cmd = seg_cmd
      link_cmd_offset = offset
    end
  end
  
  if load_cmd.cmd == LC_SYMTAB
    # Parse the symbol table command.
    sym_cmd = SymtabCommand.new(data[offset..-1])
    symbol_table_offset = offset
    
    # Parse the string table, store with offsets.
    string_table_offset = sym_cmd.stroff
    string_table = data[sym_cmd.stroff..sym_cmd.stroff+sym_cmd.strsize-1]
    i = 0
    string_table.split("\x00", -1).each do |s|
      string_table_map[i] = s
      i += s.length + 1
    end
    
    # Parse the symbol table.
    symbol_table = data[sym_cmd.symoff..-1]
    i = 0
    puts "Symbol table has #{sym_cmd.nsyms} symbols."
    sym_cmd.nsyms.times do |n|
      symbols << SymtabEntry.new(symbol_table[i..i+11])
      i += 12
    end
    
    # Now go through and make renames to the symbols.
    size_diff = 0
    
    string_table_map.keys.sort.each do |i|
      orig_sym = string_table_map[i]
      
      # Store the offset mapping.
      offset_map[i] = (i + size_diff)
      
      if orig_sym.length > 1
        sym = orig_sym[1..-1].gsub('$UNIX2003','')
        if mappings.has_key?(sym)
          if mappings[sym] != true
            string_table_map[i] = mappings[sym]
          else
            string_table_map[i] = "#{prefix}#{sym}"
          end
          puts "   - Mapping: #{orig_sym} to #{string_table_map[i]} (offset #{i} -> #{i + size_diff})"
                          
          # Accumulate the offset difference.
          size_diff += string_table_map[i].length - orig_sym.length
        end
      end
    end
  end
  
  offset += load_cmd.cmdsize
end

# OK, now lets rewrite the symbol table. Offsets may have changed, but the size doesn't.
new_symbol_table = ''
i = 0
symbols.each do |symbol|
  puts "  - Mapped #{i} symbols..." if i % 10000 == 0 and i > 0
  symbol.strtableoffset = offset_map[symbol.strtableoffset] if symbol.strtableoffset > 1
  new_symbol_table << symbol
  i += 1
end

# OK, now lets rewrite the string table. The size will be different if mappings have occurred.
new_string_table = string_table_map.keys.sort.collect { |i| string_table_map[i] }.join("\x00")

# Next, modify the LC_SYMTAB header.
size_diff = new_string_table.length - sym_cmd.strsize
sym_cmd.strsize = new_string_table.length

# Lastly, modify the LINKEDIT segment if it exists.
if link_cmd
  puts "Size changed by #{size_diff} bytes, rewriting LINKEDIT segment."
  link_cmd.filesize += size_diff
  SegmentCommand.round_byte_length.times { |i| data[link_cmd_offset + i] = link_cmd[i] }
end

# Create the new file in memory. First, copy the new symbol table header into place.
24.times { |i| data[symbol_table_offset + i] = sym_cmd[i] }

# Now copy the new symbol table.
new_symbol_table.length.times { |i| data[sym_cmd.symoff + i] = new_symbol_table[i] }

# Finally, add the new string table.
data = data[0..string_table_offset-1] + new_string_table

puts "Output file is #{data.length} bytes long."
open("output.so", "wb").write(data)
