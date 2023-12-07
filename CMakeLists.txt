cmake_minimum_required(VERSION 3.0.0)
project(libjsonpath VERSION 0.1.0 LANGUAGES CXX)

# GoogleTest requires at least C++14
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
    # warning level 4
    add_compile_options(/W4)
else()
    # additional warnings
    add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Wnon-virtual-dtor)
endif()

add_executable(libjsonpath
  dev.cpp
  src/libjsonpath/lex.cpp
  src/libjsonpath/tokens.cpp
  src/libjsonpath/parse.cpp
  src/libjsonpath/jsonpath.cpp
  src/libjsonpath/utils.cpp
)

target_include_directories(libjsonpath PUBLIC 
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>  
)


include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/f8d7d77c06936315286eb55f8de22cd23c188571.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
enable_testing()

# Lexer tests
add_executable(
  lexer_tests
  tests/libjsonpath/lex.test.cpp
  src/libjsonpath/lex.cpp
  src/libjsonpath/tokens.cpp
  src/libjsonpath/utils.cpp
)

target_include_directories(lexer_tests PUBLIC 
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>  
)

target_link_libraries(
  lexer_tests
  GTest::gtest_main
)

# Parser tests
add_executable(
  parser_tests
  tests/libjsonpath/parse.test.cpp
  src/libjsonpath/jsonpath.cpp
  src/libjsonpath/tokens.cpp
  src/libjsonpath/lex.cpp
  src/libjsonpath/parse.cpp
  src/libjsonpath/utils.cpp
)


target_include_directories(parser_tests PUBLIC 
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>  
)

target_link_libraries(
  parser_tests
  GTest::gtest_main
)

# Error tests
add_executable(
  error_tests
  tests/libjsonpath/errors.test.cpp
  src/libjsonpath/jsonpath.cpp
  src/libjsonpath/tokens.cpp
  src/libjsonpath/lex.cpp
  src/libjsonpath/parse.cpp
  src/libjsonpath/utils.cpp
)

target_include_directories(error_tests PUBLIC 
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>  
)

target_link_libraries(
  error_tests
  GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(lexer_tests parser_tests error_tests)

# set(CPACK_PROJECT_NAME ${PROJECT_NAME})
# set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
# include(CPack)