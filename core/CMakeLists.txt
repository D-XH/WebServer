
file(GLOB src_lists "*/*.cpp")

add_compile_options(-g)
add_executable(server main.cc ${src_lists} ${inc_lists})

add_library(core SHARED ${src_lists} ${inc_lists})

set(LIBRARY_OUTPUT_PATH ${root}/lib)

target_link_libraries(server pthread)