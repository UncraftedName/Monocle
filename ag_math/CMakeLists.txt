project(ag_math C ASM_MASM)
set(CMAKE_CXX_STANDARD 23)

include_directories(
	src
)

file(GLOB SRC_FILES
	"${PROJECT_SOURCE_DIR}/src/*.cpp"
)

file(GLOB SRC_ASM_FILES
	"${PROJECT_SOURCE_DIR}/src/*.asm"
)
set_source_files_properties(${SRC_ASM_FILES} PROPERTIES LANGUAGE ASM_MASM)

add_library(ag_math STATIC ${SRC_FILES} ${SRC_ASM_FILES})
