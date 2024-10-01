project(ag_math)
set(CMAKE_CPP_STANDARD 23)

include_directories(
	src
)

file(GLOB SRC_FILES
	"${PROJECT_SOURCE_DIR}/src/*.cpp"
)

add_executable(ag_test ${SRC_FILES})
