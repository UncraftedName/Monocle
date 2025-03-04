project(ag_math C ASM_MASM)
set(CMAKE_CXX_STANDARD 23)

if (MSVC)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /wd4820")
endif()

option(CHAIN_WITH_GRAPHVIZ "add the option of exporting teleport chains with graphviz" ON)
set(GRAPHVIZ_ROOT "G:/programming/_libs/Graphviz-12.2.1-win32" CACHE PATH "Graphviz root install path")

if (CHAIN_WITH_GRAPHVIZ)
	message("Using GRAPHVIZ root path: ${GRAPHVIZ_ROOT}")
	set(GRAPHVIZ_BIN "${GRAPHVIZ_ROOT}/bin")
	set(GRAPHVIZ_LIB "${GRAPHVIZ_ROOT}/lib")
	set(GRAPHVIZ_INCLUDE "${GRAPHVIZ_ROOT}/include/graphviz")

	set(REQUIRED_GRAPHVIZ_DLLS
		"${GRAPHVIZ_BIN}/cgraph.dll"
		"${GRAPHVIZ_BIN}/gvc.dll"
		"${GRAPHVIZ_BIN}/cdt.dll"
		"${GRAPHVIZ_BIN}/zlib1.dll"
		"${GRAPHVIZ_BIN}/pathplan.dll"
		"${GRAPHVIZ_BIN}/xdot.dll"
		"${GRAPHVIZ_BIN}/libexpat.dll"
		"${GRAPHVIZ_BIN}/gvplugin_dot_layout.dll"
		"${GRAPHVIZ_BIN}/gvplugin_core.dll"
	)
	set(REQUIRED_GRAPHVIZ_LIBS
		"${GRAPHVIZ_LIB}/cgraph.lib"
		"${GRAPHVIZ_LIB}/gvc.lib"
		"${GRAPHVIZ_LIB}/gvplugin_dot_layout.lib"
		"${GRAPHVIZ_LIB}/gvplugin_core.lib"
	)
	set(REQUIRED_GRAPHVIZ_FILES
		"${GRAPHVIZ_INCLUDE}/cgraph.h"
		"${GRAPHVIZ_INCLUDE}/gvc.h"
		"${GRAPHVIZ_INCLUDE}/gvplugin.h"
		${REQUIRED_GRAPHVIZ_LIBS}
		${REQUIRED_GRAPHVIZ_DLLS}
	)
	foreach(FILE_PATH IN LISTS REQUIRED_GRAPHVIZ_FILES)
		if (NOT EXISTS ${FILE_PATH})
			message(WARNING "Disabling graphviz because ${FILE_PATH} was not found")
			set(CHAIN_WITH_GRAPHVIZ OFF)
			break()
		endif()
	endforeach()
endif()
if (CHAIN_WITH_GRAPHVIZ)
	add_compile_definitions(CHAIN_WITH_GRAPHVIZ)
endif()

include_directories(
	src
	${GRAPHVIZ_INCLUDE}
)

file(GLOB SRC_FILES
	"${PROJECT_SOURCE_DIR}/src/*.cpp"
	"${PROJECT_SOURCE_DIR}/src/*.c"
)

file(GLOB SRC_ASM_FILES
	"${PROJECT_SOURCE_DIR}/src/*.asm"
)
set_source_files_properties(${SRC_ASM_FILES} PROPERTIES LANGUAGE ASM_MASM)

add_library(ag_math STATIC ${SRC_FILES} ${SRC_ASM_FILES})
if (CHAIN_WITH_GRAPHVIZ)
	target_link_libraries(ag_math PRIVATE ${REQUIRED_GRAPHVIZ_LIBS})
	target_compile_definitions(ag_math PRIVATE GVDLL)

	foreach(DLL ${REQUIRED_GRAPHVIZ_DLLS})
		add_custom_command(TARGET ag_math POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DLL} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
			COMMENT "Copying ${DLL} to ${TARGET_DIR}"
		)
endforeach()
endif()
