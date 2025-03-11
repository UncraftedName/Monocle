project(ag_math C ASM_MASM)
set(CMAKE_CXX_STANDARD 23)

if (MSVC)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /wd4820")
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
	
	foreach(DLL ${REQUIRED_GRAPHVIZ_DLLS})
		add_custom_command(TARGET ag_math POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DLL} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
			COMMENT "Copying ${DLL} to ${TARGET_DIR}"
		)
	endforeach()
endif()
