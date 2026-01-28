get_target_property(CATCH_DIR catch SOURCE_DIR)
get_target_property(MONOCLE_LIB_DIR monocle_lib SOURCE_DIR)
include_directories(
	src
	"${CATCH_DIR}/src"
	"${MONOCLE_LIB_DIR}/src"
)

SET(MAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/src/main.cpp")
SET(MAIN_TEST_FILE "${CMAKE_CURRENT_LIST_DIR}/src/test_main.cpp")

file(GLOB SRC_FILES
	"${CMAKE_CURRENT_LIST_DIR}/src/*.cpp"
)
list(REMOVE_ITEM SRC_FILES ${MAIN_FILE} ${MAIN_TEST_FILE})


add_executable(monocle_personal ${SRC_FILES} ${MAIN_FILE})
target_link_options(monocle_personal PRIVATE /SAFESEH:NO)
add_executable(monocle_test ${SRC_FILES} ${MAIN_TEST_FILE})
target_link_options(monocle_test PRIVATE /SAFESEH:NO)
target_link_libraries(monocle_personal PRIVATE monocle_lib)
target_link_libraries(monocle_test PRIVATE monocle_lib)
target_link_libraries(monocle_test PRIVATE catch)
add_test(NAME monocle_test COMMAND monocle_test)
