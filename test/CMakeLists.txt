

find_library(core_lib core ${root}/lib)
add_compile_options(-g)
add_executable(unit unit.cpp)
target_link_libraries(unit ${core_lib} pthread)