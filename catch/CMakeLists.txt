add_compile_definitions(CATCH_AMALGAMATED_CUSTOM_MAIN)

include_directories(
	src
)

file(GLOB SRC_FILES
	"${CMAKE_CURRENT_LIST_DIR}/src/*.cpp"
)

add_library(catch STATIC ${SRC_FILES})
