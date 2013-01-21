find_all_sources(vis_milkdrop mdrop_SRC)
set(eval_SRC
  vis_milkdrop/evallib/Cal_tab.c
  vis_milkdrop/evallib/Cal_tab.h
  vis_milkdrop/evallib/cfunc.c
  vis_milkdrop/evallib/Compiler.c
  vis_milkdrop/evallib/Compiler.h
  vis_milkdrop/evallib/eval.c
  vis_milkdrop/evallib/eval.h
  vis_milkdrop/evallib/gettok.c
  vis_milkdrop/evallib/Lex.h
  vis_milkdrop/evallib/Lextab.c
  vis_milkdrop/evallib/Llsave.c
  vis_milkdrop/evallib/Yylex.c
)

set_source_files_properties(
  vis_milkdrop/evallib/Compiler.c
  PROPERTIES COMPILE_FLAGS "/DNDEBUG /WX- /MP /Od /Oy- /GF- /Gm- /EHa /MT /GS /Gy- /arch:SSE /fp:precise /Zc:wchar_t /Zc:forScope /wd\"4996\""
)

set_source_files_properties(vis_milkdrop/evallib/Cal_tab.c PROPERTIES LANGUAGE "C")

add_definitions(
  -DWIN32
  -D_WINDOWS
  -D_USRDLL
  -DPLUGIN_EXPORTS
  -DHAS_DX
  -D_CRT_SECURE_NO_WARNINGS
  -D_MBCS
)

add_library(Milkdrop_win32dx MODULE ${mdrop_SRC} ${eval_SRC} MilkdropXBMC.cpp XMLDocument.cpp)
target_link_libraries(Milkdrop_win32dx ${CONFIG_LIBRARY_D3DX9})
install(TARGETS Milkdrop_win32dx LIBRARY DESTINATION ${RESOURCEPATH}/addons/visualization.milkdrop)
