cmake_minimum_required(VERSION 3.15)

project(libjsonpath
  VERSION 0.1.0
  LANGUAGES CXX
  DESCRIPTION "A JSONPath parser"
  HOMEPAGE_URL "https://github.com/jg-rp/libjsonpath"
)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/config.hpp.in
  ${CMAKE_CURRENT_SOURCE_DIR}/include/libjsonpath/config.hpp
)

set(INSTALL_GTEST OFF)

# libjsonpath_compiler_flags
add_library(libjsonpath_compiler_flags INTERFACE)

target_compile_features(libjsonpath_compiler_flags INTERFACE cxx_std_17)

if (MSVC)
    # warning level 4
    target_compile_options(libjsonpath_compiler_flags INTERFACE /W4)
else()
    # additional warnings
    set(compiler_options
      -Wall
      -Wextra
      -Wpedantic
      -Wshadow
      -Wnon-virtual-dtor
    )

    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
      # using regular Clang or AppleClang
      list(APPEND compiler_options -Wunreachable-code-aggressive)
    endif()

    target_compile_options(libjsonpath_compiler_flags INTERFACE ${compiler_options})
endif()



# libjsonpath
add_library(jsonpath
  src/libjsonpath/lex.cpp
  src/libjsonpath/tokens.cpp
  src/libjsonpath/parse.cpp
  src/libjsonpath/jsonpath.cpp
  src/libjsonpath/utils.cpp
)

target_link_libraries(jsonpath PUBLIC libjsonpath_compiler_flags)

target_include_directories(jsonpath PUBLIC 
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>  
)

install(TARGETS jsonpath libjsonpath_compiler_flags DESTINATION lib)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/libjsonpath DESTINATION include)


# Fetch Google Test
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
  libjsonpath_compiler_flags
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
  libjsonpath_compiler_flags
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
  libjsonpath_compiler_flags
  GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(lexer_tests)
gtest_discover_tests(parser_tests)
gtest_discover_tests(error_tests)
