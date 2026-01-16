set(SRC_FILES
	src/source_math.asm
	src/source_math.cpp
	src/source_math_strings.cpp
	src/vag_logic.cpp
	src/chain_graphviz.cpp
)

# Redirect MASM output object file to fix: 'cannot open file : monocle_lib.dir\Debug\/src/source_math.asm.obj'
# TODO I have no idea why this is necessary.
set_source_files_properties(src/source_math.asm PROPERTIES
	COMPILE_FLAGS "/Fo$(IntDir)source_math.obj"
	LANGUAGE ASM_MASM
)

add_library(monocle_lib STATIC ${SRC_FILES})

target_compile_features(monocle_lib PUBLIC cxx_std_23)

target_compile_options(monocle_lib PRIVATE
	$<$<COMPILE_LANGUAGE:C,CXX>:/W4;/wd4820>
)

target_include_directories(monocle_lib PRIVATE
	src
)
