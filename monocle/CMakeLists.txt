add_executable(monocle_personal "src/main.cpp")
target_link_libraries(monocle_personal monocle_lib)
target_compile_options(monocle_personal PRIVATE /W4 /wd4505)

add_executable(monocle_test "src/test_main.cpp")
target_link_libraries(monocle_test monocle_lib catch)
target_compile_options(monocle_test PRIVATE /W4)

add_test(NAME monocle_test COMMAND monocle_test)
