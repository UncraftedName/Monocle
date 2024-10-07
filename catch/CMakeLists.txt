project(catch C)
set(CMAKE_CXX_STANDARD 23)

add_compile_definitions(CATCH_AMALGAMATED_CUSTOM_MAIN)

include_directories(
	src
)

file(GLOB SRC_FILES
	"${PROJECT_SOURCE_DIR}/src/*.cpp"
)

add_library(catch STATIC ${SRC_FILES})
