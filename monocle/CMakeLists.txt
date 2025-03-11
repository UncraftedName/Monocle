project(monocle C)
set(CMAKE_CXX_STANDARD 23)

get_target_property(CATCH_DIR catch SOURCE_DIR)
get_target_property(AG_MATH_DIR ag_math SOURCE_DIR)
include_directories(
	src
	"${CATCH_DIR}/src"
	"${AG_MATH_DIR}/src"
	"${GRAPHVIZ_INCLUDE}"
)

SET(MAIN_FILE "${PROJECT_SOURCE_DIR}/src/main.cpp")
SET(MAIN_TEST_FILE "${PROJECT_SOURCE_DIR}/src/test_main.cpp")

file(GLOB SRC_FILES
	"${PROJECT_SOURCE_DIR}/src/*.cpp"
)
list(REMOVE_ITEM SRC_FILES ${MAIN_FILE} ${MAIN_TEST_FILE})


add_executable(monocle ${SRC_FILES} ${MAIN_FILE})
add_executable(monocle_test ${SRC_FILES} ${MAIN_TEST_FILE})
target_link_libraries(monocle PRIVATE ag_math)
target_link_libraries(monocle_test PRIVATE ag_math)
target_link_libraries(monocle_test PRIVATE catch)
add_test(NAME monocle_test COMMAND monocle_test)
