#!/usr/local/bin/ruby

# DLL のダイナミックロード用ツール
# １行目 クラス名
# ２行目 dll 名
# ３行目以降 関数プロトタイプ
# なファイル source を
# ruby dllutl.rb < source
# としてやれば、その関数を dll からダイナミックロードしてくれる
# ソースを自動作成してくれる。ただし、あまり複雑な関数プロトタイプには
# 未対応。特に () がらみ。
#
# 適用：Public Domain
#
# Version 0.1p
#
# Daisuke Aoki <dai@y7.net>

def arg_ana(args)
  list = []
  temp = ""
  if args.index(",")==nil
    temp = args
  else
    temp = args.split(/,/)
  end
  for i in temp
    if i == "void"
      return []
    end
    if i =~ /\s*(.*)\s+(\S+)\s*/
      rest = $2
      if rest =~ /(\s*\**\s*\**\s*\**\s*\**\s*)(.*)/
        list << $2
      else
        return nil
      end
    else
      return nil
    end
  end
  return list
end

class_name = "default_name"
dll_name = "default_name.dll"
class_name = gets.chomp
dll_name = gets.chomp

funclist = []
while gets
#    if $_ =~ /(\s*[^\s]\s+\**)([^\(]+)(\(.*\))\;\s*/
#    if $_ =~ /(\s*\S+\s+)([^\(]+)(\(.*\))\;\s*/
    if $_ =~ /^#.*/
      next
    end
    rest = $_
    part_prev = ""
    part_func = ""
    part_post = ""
    if rest =~ /\s*(.*)\s*(\(.*?\))\s*;/
      rest = $1
      part_post << $2
      if rest =~ /\s*(.*)\s+(\S+)\s*/
        part_prev << $1
        rest = $2
        if rest =~ /(\s*\**\s*\**\s*\**\s*\**\s*)(.*)/
          part_prev << $1
          part_func << $2
#          printf("<%s><%s><%s>\n",part_prev,part_func,part_post)
        end
      end
    end
    if part_func!=""
      funclist << [part_prev,part_func,part_post]
    end
end

print "/***************************************************************\n"
printf " name: %s  dll: %s \n",class_name,dll_name
print "***************************************************************/\n"

print "\n"
printf "extern int load_%s(void);\n",class_name
printf "extern void free_%s(void);\n",class_name
print "\n"

for i in funclist
  printf "typedef %s(*type_%s)%s;\n",i[0],i[1],i[2]
end
printf "\nstatic struct %s_ {\n",class_name
for i in funclist
  printf "\t type_%s %s;\n",i[1],i[1]
end
printf "} %s;\n\n",class_name

printf "static volatile HANDLE h_%s = NULL;\n\n",class_name

printf "void free_%s(void)
{
\tif(h_%s){
\t\tFreeLibrary(h_%s);
\t\th_%s = NULL;
\t}
}

",class_name,class_name,class_name,class_name

printf "int load_%s(void)
{
\tif(!h_%s){
\t\th_%s = LoadLibrary(\"%s\");
\t\tif(!h_%s) return -1;
\t}
",class_name,class_name,class_name,dll_name,class_name
for i in funclist
  printf "\t%s.%s = (type_%s)GetProcAddress(h_%s,\"%s\");\n",class_name,i[1],i[1],class_name,i[1]
  printf "\tif(!%s.%s){ free_%s(); return -1; }\n",class_name,i[1],class_name
end
printf "\treturn 0;\n}\n\n"

for i in funclist
  printf "%s %s%s
{
\tif(h_%s){
",i[0],i[1],i[2],class_name
  if i[0] =~ /\s*void\s*/
    printf "\t\t%s.%s(",class_name,i[1]
  else
    printf "\t\treturn %s.%s(",class_name,i[1]
  end
  args = ""
  if i[2] =~ /\((.*)\)/
    args = $1.strip
  end
  arglist = arg_ana(args)
  if arglist == nil
    print "\n@@@ BAD @@@\n"
    exit
  end
  num = 0
  for j in arglist
    if j != "void"
      print "," if num!=0
      print j
      num += 1
    end
  end
  print ");\n"
  if i[0] =~ /\s*void\s*/
    printf "\t}\n"
  else
    printf "\t}\n\treturn (%s)0;\n",i[0]
  end
  printf "}\n\n"
end

print "/***************************************************************/\n"

