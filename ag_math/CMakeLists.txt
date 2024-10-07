project(ag_math C ASM_MASM)
set(CMAKE_CXX_STANDARD 23)

include_directories(
	src
)

SET(MAIN_FILE "${PROJECT_SOURCE_DIR}/src/main.cpp")
SET(MAIN_TEST_FILE "${PROJECT_SOURCE_DIR}/src/test_main.cpp")

file(GLOB SRC_FILES
	"${PROJECT_SOURCE_DIR}/src/*.cpp"
)
list(REMOVE_ITEM SRC_FILES ${MAIN_FILE} ${MAIN_TEST_FILE})

file(GLOB SRC_ASM_FILES
	"${PROJECT_SOURCE_DIR}/src/*.asm"
)
set_source_files_properties(${SRC_ASM_FILES} PROPERTIES LANGUAGE ASM_MASM)

add_executable(ag_math ${SRC_FILES} ${SRC_ASM_FILES} ${MAIN_FILE})
add_executable(ag_math_test ${SRC_FILES} ${SRC_ASM_FILES} ${MAIN_TEST_FILE})
target_link_libraries(ag_math_test PRIVATE catch)
add_test(NAME ag_math_test COMMAND ag_math_test)
