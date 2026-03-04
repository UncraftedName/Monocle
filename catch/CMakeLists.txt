add_compile_definitions(CATCH_AMALGAMATED_CUSTOM_MAIN)

add_library(catch STATIC "src/catch_amalgamated.cpp")

target_include_directories(catch PUBLIC src)
