cmake_minimum_required(VERSION 3.20)
project(rix CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)          
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Eigen3 REQUIRED)


add_library(rixc_lib
    src/rix_tokenizer.cpp
    src/dim_expr.cpp
    src/rix_type.cpp
    src/ast.cpp
    src/parser.cpp
    src/unify.cpp
    src/symbol_table.cpp
    src/function_table.cpp
    src/type_checker.cpp
    src/cpp_writer.cpp
)
target_include_directories(rixc_lib PUBLIC include)
target_link_libraries(rixc_lib PUBLIC Eigen3::Eigen)

add_executable(rixc src/main.cpp)
target_link_libraries(rixc PRIVATE rixc_lib)


enable_testing()

function(rix_add_test name)
    add_executable(${name} test/${name}.cpp)
    target_link_libraries(${name} PRIVATE rixc_lib)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

rix_add_test(test_dim_expr)
rix_add_test(test_rix_type)
rix_add_test(test_ast)
rix_add_test(test_parser)
rix_add_test(test_unify)
rix_add_test(test_tables)
rix_add_test(test_type)
rix_add_test(test_cpp_writer)
