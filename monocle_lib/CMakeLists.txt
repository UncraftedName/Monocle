set(SRC_FILES
	"src/game/source_math_5135.asm"
	"src/game/source_math.cpp"
	"src/game/source_math_strings.cpp"
	"src/teleport_chain/ent_to_portal.cpp"
	"src/teleport_chain/generate.cpp"
	"src/teleport_chain/debug/csv_precision_compare.cpp"
	"src/teleport_chain/debug/gv_flow_control.cpp"
	"src/teleport_chain/debug/minidump.cpp"
)

add_library(monocle_lib STATIC ${SRC_FILES})

target_compile_features(monocle_lib PUBLIC cxx_std_23)

target_compile_options(monocle_lib PRIVATE
	$<$<COMPILE_LANGUAGE:C,CXX>:/W4;/wd4820>
)

target_include_directories(monocle_lib PRIVATE
	src
)

target_link_options(monocle_lib PUBLIC
	/NATVIS:${CMAKE_CURRENT_LIST_DIR}/dbg.natvis
)
