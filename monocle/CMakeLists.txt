add_executable(monocle_personal "src/main.cpp")
target_link_libraries(monocle_personal monocle_lib)

add_executable(monocle_test "src/test_main.cpp")
target_link_libraries(monocle_test monocle_lib catch)

add_test(NAME monocle_test COMMAND monocle_test)
